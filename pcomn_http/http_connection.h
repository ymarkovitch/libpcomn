/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
#ifndef __HTTP_CONNECTION_H
#define __HTTP_CONNECTION_H
/*******************************************************************************
 FILE         :   http_connection.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   HTTP connections.

 CREATION DATE:   3 Mar 2008
*******************************************************************************/
/** @file
    HTTP protocol connections, both server and client side.
*******************************************************************************/
#include <pcomn_http/http_message.h>
#include <pcomn_csvr/commsvr_connection.h>

#include <pcomn_string.h>

namespace pcomn {
namespace http {

typedef svr::iovec_t iovec_t ;
class http_istream ;

/******************************************************************************/
/** This class provides the main interface to the HTTP protocol.

 All HTTP interaction between a client and a server is being done through objects
 of the classes derived from this.
 This is an abstract class. The concrete classes are HTTPServerConnection
 and HTTPClientConnection.
*******************************************************************************/
class HTTPConnection {
public:
    enum Flags
    {
        FClosed           =  0x00000001,
        FLastRequestHead  =  0x00000100, /* The last request sent was HEAD.
                                          * Content-Length in the following response must be ignored. */

        FUserDefinedFlags =  0xFFFF0000  /* This is the "private flags area", i.e. all (and only)
                                          * flags in the top 16 bits can be set by the programmer.
                                          */
    } ;

    /// Destructor is provided only to provide a virtual stub for derived classes.
    virtual ~HTTPConnection() {}

    /// Read message content.
    /// This function completely isolates caller from underlying transfer-encoding
    /// peculiarities, i.e. whether it simple or chunked doesn't matter; for a caller
    /// it looks like simple contiguous stream of data.
    ///
    /// @param buffer   The buffer to read content into. If this parameter is NULL,
    /// the @a size bytes of content will be simply skipped.
    /// @param size     The size of data to read.
    ///
    /// @return  The number of bytes read. The result is min(size, size_of_rest_of_contents);
    /// 0 indicates the end-of-content situation.
    size_t receive(char *buffer, size_t size) ;

    /// Test whether there is any content yet to receive ('eoc' stands for 'End Of  Content').
    /// @return true: the whole content pertaining to a last request has been received;
    /// false: there is some content data pending.
    bool eoc() const ;

    // eot() -  test whether there must be transmitted more content data pertaining
    //          to the last transmitted message. ('eot' stands for 'End Of Transmit')
    // Returns:
    //    true  -  whole content data pertaining to a last response has been transmitted.
    //             For contiguous transfer it means there were already transmitted content-length bytes;
    //             for chunked transfer it means there were transmit("", 0) (i.e. "close transmit") called.
    //    false -  there must be transmitted more data.
    //
    bool eot() const { return !_pending_out ; }

    bool has_incoming_data() const { return _input_stream->is_data_available() ; }

    /// Get the overall number of messages received through this connection.
    int messages_received() const { return _messages_received ; }
    /// Get the overall number of messages sent through this connection.
    int messages_sent() const { return _messages_sent ; }

    /// Get the last message received
    /// Please DON'T call this before the first receive_message() for the given
    /// connection.
    const HTTPMessage &last_message() const
    {
        NOXCHECK(_last_received) ;
        return *_last_received ;
    }

    /// Transmit content data.
    /// @param data  Data to transmit.
    /// @param size  The size of data to be transmitted.
    ///
    /// In the chunked mode every call to transmit sends one chunk and
    /// transmit(any_pointer,0) closes chunked transmit (i.e., after such a call you are
    /// free to call respond() again).
    /// In the non-chunked mode transmits 'size' bytes of data or the remaining part of
    /// content-length, whichever is smaller.
    ///
    /// @return  The size of transmitted data.
    ///   @li in the chunked mode the returned value is always equal to @a size;
    ///   @li in the contiguous mode the returned value can be less than @a size,
    ///   which means that specified @a size is greater than the remaining part of
    ///   content-length.
    size_t transmit(const char *data, size_t size) ;

    /// Transmit several buffers at once as content.
    ///
    /// In the chunked mode every transmit sends one HTTP chunk, no matter how many
    /// buffers specify the paramaeters.
    /// In the non-chunked mode, if the overall size of data to transmit that is specified
    /// by @a data_buffers is greater than the remaining part of content-length, throws
    /// std::out_of_range.
    ///
    /// @param begin    Start of array of buffer descriptors to transmit.
    /// @param end      End of array of buffer descriptors to transmit.
    ///
    /// @return The size of transmitted data.
    /// @throw std::out_of_range    Overall size of data to transmit that is specified
    /// by @a data_buffers is greater than the remaining part of content-length.
    size_t transmit(const iovec_t *begin, const iovec_t *end) ;

    /// Transmit a file or part thereof as content data.
    /// @param fd       File descriptor to be transmited. The descriptor requirements are
    /// the same as for sendfile.
    /// @param size     The size of data to transmit. If >0, specifies the actual size to
    /// tramsmit; if ==0, transmit to the end of file or to the eot(), whatever comes first.
    /// @param offset   The offset to transmit a file from. If >=0, specifies the actual
    /// offset; if <0, transmit the file from its current position.
    size_t transmit(int fd, size_t size = 0, int64_t offset = -1) ;

    // transmit()  -  transmit a file or part thereof with head/tail data.
    // Parameters:
    //    file  -  the file to be transmited. The handle requirements are the same as fo TransmitFile().
    //    size  -  >0    -  the size of data to transmit.
    //             ==0   -  transmit to the end of file (or to the eot(), whatever first)
    //
    size_t transmit(int fd,
                    const std::pair<iovec_t, iovec_t> &header_footer,
                    size_t bytes_to_send = 0,
                    int64_t from_offset = -1) ;

    unsigned flags() const { return _flags ; }

    // closed() -  check whether this connection is closed.
    // Connection became closed after sending response with "Connection: close".
    //
    bool is_closed() const { return flags() & FClosed ; }

    const std::string &agent_name() const { return _agent_name ; }
    void set_agent_name(const std::string &name) { _agent_name = name ; }

    const std::string &default_content() const { return _default_content_typ ; }
    void set_default_content(const std::string &name) { _default_content_typ = name ; }

    /// Get the id of the communication connection we are sitting on.
    unsigned long id() const { return connection().id() ; }

    /// Get the address of this connection's party.
    const net::sock_address &peer() const { return connection().peer() ; }

    /// Suspend execution for microsecond intervals.
    /// @param period The period in microseconds to suspend execution for.
    ///
    void usleep_for(unsigned long period) { connection().usleep_for(period) ; }

    /// Suspend execution until the given moment comes.
    ///
    /// @param moment The moment to awake (UTC in microseconds).
    /// @note If @a moment is earlier than the current time, behaves like usleep(0).
    ///
    void usleep_until(int64_t moment) { connection().usleep_until(moment) ; }

    /// Check whether the underlying communication connection is alive.
    /// If the connection is dead, throws an exception. If alive, does nothing.
    /// @see BasicConnection::check_connection
    void check_connection() const
    {
        const_cast<HTTPConnection *>(this)->connection().check_connection() ;
    }

protected:
    // Constructor.
    // Create an HTTP connection on top of the communications connection given.
    //
    explicit HTTPConnection(svr::BasicConnection &connection) ;

    /// Send response line and headers and possibly content data (or part thereof).
    ///
    /// @param message  Message descriptor. It is automatically stamped with the necessary
    /// headers (e.g. "Date:", "Server:", "User-Agent", etc. - depends on derived classes)
    /// through the prepare_message() (it is the reason why this parameter is non-const).
    ///
    /// @param  data    A pointer to message content or to the first part of it.
    ///
    /// @param  size    The size of content data or of the first chunk to be sent.
    /// If this parameter is -1 and the 'data' parameter is not NULL, the size is defined
    /// as the value of message.content_length(). If at that chunked transfer is requested
    /// (through the apppropriate message settings), this function throws
    /// std::invalid_argument because there is no way to deduce the size of a chunk.
    ///
    /// @return Number of bytes sent (including the response size).
    ///
    /// @note If the 'data' parameter is not NULL, the semantics of this call is _strictly_
    /// equivalent to the following:
    ///   @code
    ///   send_message(message) ;
    ///   transmit(data, size == -1 ? message.content_length() : size) ;
    ///   @endcode
    size_t send_message(HTTPMessage &message, const char *data = NULL, size_t size = (size_t)(-1)) ;

    size_t send_impromptu(HTTPMessage &message, const char *data, size_t size) ;

    // receive_message() -  receive an HTTP message (first line & headers)
    // Receives next HTTP message (which is a request for the server connection and
    // a response for the client, respectively) and returns a reference to the internally
    // stored message buffer (the same reference can be taken aftewards through
    // the last_message() call). Does NOT receive data (if present), which must be
    // retrieved through receive() aftewards.
    // Parameters:
    //    skip_rest_data -  defines what to do if there is any unreceived content
    //                         related to the previous received message:
    //                            false -  throw an exception;
    //                            true  -  skip (ignore) the rest of content completely.
    //
    const HTTPMessage &receive_message(bool skip_rest_data, bool ignore_content = false) ;

    virtual HTTPMessage *read_message() = 0 ;
    virtual HTTPMessage &prepare_message(HTTPMessage &message_to_send) ;

    svr::BasicConnection &connection() { return _connection ; }
    const svr::BasicConnection &connection() const { return _connection ; }
    http_istream &input_stream() ;

    unsigned flags(unsigned value, bool on = true)
    {
        unsigned old = _flags ;
        set_flags(_flags, on, value) ;
        return old ;
    }

private:
    svr::BasicConnection &  _connection ;        /* The communications connection we are sitting upon */
    bigflag_t               _flags ;
    int                     _messages_received ; /* The number of received messages */
    int                     _messages_sent ;     /* The number of sent messages */
    size_t                  _pending_in ;
    size_t                  _pending_out ;       /* The size of not-yet-sent content pertaining to current response.
                                                  * In case of transfer-mode: chunked, it is equal to (size_t)-1. */
    PTSafePtr<binary_ibufstream> _input_stream ;
    PTSafePtr<HTTPMessage>              _last_received ;

    std::string _agent_name ;           /* The name of a server (for the server connection) or
                                         * a client (for the client connection) */
    std::string _default_content_typ ;  /* The default type of entity content. */

    void set_chunked_transmit() { _pending_out = (size_t)-1 ; }
    size_t close_chunked_transmit() ;
    bool is_transmit_chunked() const { return _pending_out == (size_t)-1 ; }
    void close_transmit() { _pending_out = 0 ; }

    size_t transmit_chunk(const iovec_t *begin, const iovec_t *end, size_t size) ;
    size_t transmit_chunk(const char *data, size_t size) ;

    size_t transmit_chunk(int fd, size_t size, int64_t offset) ;
    size_t transmit_chunk(int fd, size_t size, int64_t offset,
                          const iovec_t &head, const iovec_t &tail) ;
    size_t prepare_transmit(int fd, size_t size, int64_t offset, size_t addsize = 0) ;
} ;

/******************************************************************************/
/**  This class provides the main interface to the server side of the HTTP protocol.

 All interaction with a particular client is being done through an object of this class.
 @see HTTPConnection for the most part of public interface.
*******************************************************************************/
class HTTPServerConnection : public HTTPConnection {
    typedef HTTPConnection ancestor ;
public:
    /// Constructor: creates a server-side HTTP connection on top of a communication connection.
    explicit HTTPServerConnection(svr::BasicConnection &connection) ;

    /// Receive an HTTP request (request line & headers).
    /// Receives next HTTP request and returns a reference to the internally stored
    /// request buffer (the same reference can be taken aftewards through the
    /// last_request() call). Does @em NOT receive data, the data must be retrieved through
    /// receive() aftewards.
    ///
    /// @param skip_rest_data  Defines what to do if there is any unreceived content
    /// related to the previous request pending: false -  throw an exception; true - skip
    /// (ignore) the rest of content completely.
    const HTTPRequest &receive_request(bool skip_rest_data = false) ;

    /// Get the number of the currently unanswered requests.
    /// An unanswered request is the one which haven't been respond()'ed yet.
    /// The situation is perfectly possible because you can issue several receive_request()'s
    /// before respond().
    int unanswered() const { return _unanswered ; }

    /// Get the overall number of requests recieved through this connection
    int requests_received() const { return ancestor::messages_received() ; }

    /// Get the last recieved request.
    /// Please DON'T call this before the first receive_request() for the connection.
    const HTTPRequest &last_request() const
    {
        return static_cast<const HTTPRequest &>(ancestor::last_message()) ;
    }

    /// Send response line and headers and possibly content data (or part thereof).
    /// Parameters:
    //    response -  response descriptor. It is automatically stamped with the "Date: " and
    //                "Server: " headers before being sent (it is the reason why this parameter
    //                is non-const). The function uses current GMT for "Date: " and the value
    //                returned by the server_name() call for "Server: ".
    //    data     -  a pointer to response content or to the first part of it.
    //    size     -  the size of content data or of the first chunk to be sent.
    //                If this parameter is -1 and the 'data' parameter is not NULL,
    //                the size is defined as the value of response.content_length().
    //                If at that chunked transfer is requested (through the apppropriate response
    //                descriptor settings), this function throws std::invalid_argument because
    //                there is no way to deduce the size of a chunk.
    // Returns:
    //    the number of bytes sent (including the response size)
    // Please note that if the 'data' parameter is not NULL, the semantics of this call is _strictly_
    // equivalent to the following:
    //    respond(response) ;
    //    transmit(data, size == -1 ? response.content_length() : size) ;
    //
    size_t respond(HTTPResponse &response, const void *data, size_t size)
    {
        if (flags() & FLastRequestHead)
            size = HTTP_IGNORE_CONTENT ;
        return send_message(response, (const char *)data, size) ;
    }

    size_t respond(HTTPResponse &response)
    {
        return respond(response, NULL, (size_t)-1) ;
    }

    /// Create a response with specified code and send it.
    /// @param code The response code (like 200, 403, etc.); may be combined with flags,
    /// @see HTTPMessage::Flags.
    /// @param data The data to send as message content.
    /// @param size The size of message content.
    size_t respond(unsigned code, const void *data, size_t size) ;

    size_t respond(unsigned code) { return respond(code, NULL, 0) ; }

    template<typename S>
    enable_if_strchar_t<S, char, size_t>
    respond(HTTPResponse &response, const S &str)
    {
        return respond(response, str::cstr(str), str::len(str)) ;
    }

    template<typename S>
    enable_if_strchar_t<S, char, size_t>
    respond(unsigned code, const S &str)
    {
        return respond(code, str::cstr(str), str::len(str)) ;
    }

    const std::string &server_name() const { return agent_name() ; }
    void set_server_name(const std::string &name) { set_agent_name(name) ; }

protected:
    // Override ancestor methods.
    HTTPMessage *read_message() ;
    HTTPMessage &prepare_message(HTTPMessage &response) ;
private:
    int _unanswered ; /* The number of unanswered requests */
} ;

/******************************************************************************/
/** This class provides the main interface to the client side of the HTTP protocol.

 All interaction with an HTTP server is being done through an object of this class.
 See HTTPConnection for the most part of public interface.
*******************************************************************************/
class HTTPClientConnection : public HTTPConnection {
    typedef HTTPConnection ancestor ;
public:
    enum Flags
    {
        FUseProxy = 0x00040000 /**< The connection is established through a proxy */
    } ;

    /// Constructor: creates a client-side HTTP connection on top of a raw communication
    /// connection.
    HTTPClientConnection(svr::BasicConnection &connection, unsigned flags = 0) ;

    /// Receive an HTTP response.
    /// Receives next HTTP response and returns a reference to the internally stored
    /// response object: the same reference can be taken aftewards using last_response().
    /// Does @em NOT receive data (if present): it must be retrieved through receive()
    /// aftewards.
    ///
    /// @param skip_rest_data  Specifies what to do if there is any unreceived content
    /// related to the previous response: if false, throw an exception; if true skip
    /// (ignore) the rest of content completely.
    /// @throw std::logic_error, if there is both data pending from the previous call and
    /// @a skip_rest_data is true.
    const HTTPResponse &receive_response(bool skip_rest_data = false)
    {
        return static_cast<const HTTPResponse &>(
            ancestor::receive_message(skip_rest_data, (flags() & FLastRequestHead) != 0)) ;
    }

    /// Get the number of currently unanswered requests.
    /// The request is unanswered if it has been already sent but there was no response
    /// from the server on that yet.
    /// The situation is perfectly possible because you can issue several requests
    /// before receiving responses ("pipelining").
    int unanswered() const { return _unanswered ; }

    /// Get the last request received from a server.
    /// Please DON'T call this before the first receive_response() for the given
    /// connection.
    const HTTPResponse &last_response() const
    {
        return static_cast<const HTTPResponse &>(ancestor::last_message()) ;
    }

    // request()   -  send to the server a request body (first line and headers)
    //                and possibly content data (or part thereof).
    // Parameters:
    //    message  -  a request descriptor. It is automatically stamped with the "Date: " and
    //                "User-Agent: " headers before being sent (it is the reason why this parameter
    //                is non-const). The function uses current GMT for "Date: " and the value
    //                returned by the agent_name() call for "User-Agent: ".
    //    data     -  a pointer to response content or to the first part of it.
    //    size     -  the size of content data or of the first chunk to be sent.
    //                If this parameter is -1 and the 'data' parameter is not NULL,
    //                the size is defined as the value of response.content_length().
    //                If at that chunked transfer is requested (through the apppropriate response
    //                descriptor settings), this function throws std::invalid_argument because
    //                there is no way to deduce the size of a chunk.
    // Returns:
    //    the number of bytes sent (request size + content size)
    // Please note that if the 'data' parameter is not NULL, the semantics of this call is _strictly_
    // equivalent to the following:
    //    request(message) ;
    //    transmit(data, size == -1 ? message.content_length() : size) ;
    //
    size_t request(HTTPRequest &message, const char *data = NULL, size_t size = (size_t)(-1)) ;

    /// Get the host the last request is sent to,
    /// Before the first request, this function returns an empty string.
    const std::string &host() const { return _host ; }

    /// Send an impromptu request.
    size_t request(HTTPRequest::Method method, const URI &uri, const char *data = NULL, size_t size = 0) ;

    /// Get the User-Agent string.
    /// All requests are sent with User-Agent header set to this value.
    const std::string &user_agent() const { return agent_name() ; }
    /// Set the User-Agent string.
    /// All subsequent requests will be sent with this user agent.
    void set_user_agent(const std::string &name) { set_agent_name(name) ; }

protected:
    // Override ancestor methods.
    HTTPMessage *read_message() ;
    HTTPMessage &prepare_message(HTTPMessage &request) ;
private:
    int _unanswered ; /* The number of unanswered requests */
    std::string _host ;
} ;

} // end of namespace pcomn::http
} // end of namespace pcomn

#endif /* __HTTP_CONNECTION_H */
