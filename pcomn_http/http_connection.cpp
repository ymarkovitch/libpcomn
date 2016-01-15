/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
/*******************************************************************************
 FILE         :   http_connection.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Implementation of both client and server HTTP connections.

 CREATION DATE:   3 Mar 2008
*******************************************************************************/
#include <pcomn_http/http_connection.h>
#include <pcomn_http/http_diag.h>

#include <pcomn_buffer.h>
#include <pcomn_simplematrix.h>

#include <limits>
#include <algorithm>

#include <stdio.h>

static const char HTTP_END_CHUNKED_TRANSFER[] = "0\r\n\r\n" ;

static const char HTTP_DEFAULT_SERVER[] = "PCOMMON HTTP Server/0.2" ;
static const char HTTP_DEFAULT_CLIENT[] = "PCOMMON HTTP Client/0.1" ;
static const char HTTP_DEFAULT_CONTENT[] = "application/octet-stream" ;

static const std::string STR_CONNECTION("connection") ;
static const std::string STR_KEEP_ALIVE("keep-alive") ;
static const std::string STR_KEEP_ALIVE_TIMEOUT("300") ;

namespace pcomn {
namespace http {

static std::string hostinfo(const std::string &host, unsigned port)
{
    if (port == 0 || port == HTTP_PORT)
        return host ;
    return strprintf("%s:%u", host.c_str(), port) ;
}

/*******************************************************************************
                     class http_istream
 The input stream class which supports transparent HTTP message content reading.
 Hides details about chunked/contiguous transfer.
*******************************************************************************/
class http_istream : public svr::connection_ibufstream {
    typedef svr::connection_ibufstream ancestor ;
public:
    explicit http_istream(svr::BasicConnection &connection) :
        ancestor(connection),
        _content_length(0),
        _last_chunk(true)
    {}

    bool eof() const { return _last_chunk && !_content_length ; }

    bool is_unbound_content() const { return _content_length == HTTP_UNBOUND_CONTENT ; }

    size_t set_content_length(size_t new_size)
    {
        size_t old = _content_length ;
        _last_chunk = true ;
        if (new_size == HTTP_CHUNKED_CONTENT)
            init_next_chunk() ;
        else
            _content_length = new_size ;
        return old ;
    }

protected:
    size_t read_data(void *buffer, size_t size) ;

private:
    size_t   _content_length ;
    bool     _last_chunk ;
    bool     _unbound_content ;

    size_t init_next_chunk()
    {
        // If this is not a last chunk, read next chunk's size
        _content_length = get_next_chunk_size() ;
        if (!_content_length)
        {
            // OK, there was the last chunk.
            _last_chunk = true ;
            // Skip trailing strings (RFC permit us to safely ignore trailers)
            skip_trailers() ;
        }
        else
            _last_chunk = false ;
        return _content_length ;
    }

    size_t get_next_chunk_size()
    {
        NOXCHECK(!_content_length) ;
        PCOMN_THROW_MSG_IF(!_last_chunk && pcomn::readline(*this, eolmode_CRLF) != "\n",
                           http_message_error,
                           "Illegal chunked encoding in HTTP connection %lu: no CRLF after chunk data", connection().id()) ;

        return strtoul(pcomn::readline(*this, eolmode_CRLF).c_str(), NULL, 16) ;
    }

    void skip_trailers()
    {
        // Skip all strings up to and including first empty string
        std::string s ;
        do s = pcomn::readline(*this, eolmode_CRLF) ;
        while(!s.empty() && s != "\n") ;
    }
} ;

/*******************************************************************************
 http_istream
*******************************************************************************/
size_t http_istream::read_data(void *buffer, size_t size)
{
    size_t result = 0 ;
    while(!eof())
        if (_content_length || init_next_chunk())
            if (size)
            {
                const size_t last = ancestor::read_data(padd(buffer, result), std::min(size, _content_length)) ;
                NOXCHECK(last <= std::min(size, _content_length)) ;
                if (!is_unbound_content())
                {
                    PCOMN_THROW_MSG_IF(!last, http_connection_closed,
                                       "HTTP connection %lu has been closed by the peer", connection().id()) ;
                    _content_length -= last ;
                }
                else if (!last)
                    _content_length = 0 ;
                size -= last ;
                result += last ;
            }
            else
                break ;
    return result ;
}

/*******************************************************************************
 HTTPConnection
*******************************************************************************/
const size_t FILE_HEADER_REASONABLE_BUF_SIZE = 8192 ;

HTTPConnection::HTTPConnection(svr::BasicConnection &connection) :
    _connection(connection),
    _flags(0),
    _messages_received(0),
    _messages_sent(0),
    _pending_in(0),
    _pending_out(0),
    _input_stream(new http_istream(connection)),
    _default_content_typ(HTTP_DEFAULT_CONTENT)
{}

inline http_istream &HTTPConnection::input_stream()
{
    return *static_cast<http_istream *>(_input_stream.get()) ;
}

const HTTPMessage &HTTPConnection::receive_message(bool skip_rest_data, bool ignore_content)
{
    TRACEPX(CHTTP_Connection, DBGL_HIGHLEV, "HTTPConnection::receive_message("
            << (skip_rest_data ? "skip_rest_data)" : ")") << (!eoc() ? " There is pending data." : "")) ;

    if (!eoc())
        if (skip_rest_data)
            receive(NULL, -1) ;
        else
            throw std::logic_error("receive_message() when pending previous content data") ;

    try {
        _last_received = read_message() ;
    }
    catch(const eof_error &x) {
        throw http_connection_closed(strprintf("HTTP connection %lu has been closed by the peer", id())) ;
    }
    ++_messages_received ;

    TRACEPX(CHTTP_Connection, DBGL_VERBOSE, "Received message #" << messages_received() << ":\n" << last_message()) ;

    // Check if we have some content
    size_t new_content_length ;

    if (last_message().is_chunked_transfer())
        new_content_length = ignore_content ? 0 : HTTP_CHUNKED_CONTENT ;
    else if (last_message().content_length())
        new_content_length = ignore_content ? 0 : last_message().content_length() ;
    else if (!last_message().content_type().empty())
        new_content_length = HTTP_UNBOUND_CONTENT ;
    else
        new_content_length = 0 ;

    input_stream().set_content_length(new_content_length) ;
    return last_message() ;
}

size_t HTTPConnection::receive(char *buffer, size_t size)
{
    PCOMN_THROW_MSG_IF(buffer && size == (size_t)-1, std::invalid_argument,
                       "invalid buffer size. size==-1 is only possible if buffer==NULL") ;
    if (eoc())
        return 0 ;

    if (buffer)
        return input_stream().read(buffer, size) ;

    // We have to scoop out all stream data.
    size_t received = 0 ;
    char dummy_buf[2048] ;
    do {
        size_t just_received = input_stream().read(dummy_buf, P_ARRAY_COUNT(dummy_buf)) ;
        received += just_received ;
    }
    while(!eoc()) ;

    return received ;
}

bool HTTPConnection::eoc() const
{
    return const_cast<HTTPConnection *>(this)->input_stream().eof() ;
}

size_t HTTPConnection::send_message(HTTPMessage &response, const char *data, size_t size)
{
    TRACEPX(CHTTP_Connection, DBGL_LOWLEV, "Send message. data=" << (void *)data
            << " size=" << size << " pending_out=" << _pending_out << " :\n" << response) ;

    PCOMN_THROW_MSG_IF(is_closed(), std::logic_error,
                       "Attempt to send message on a closed connection") ;
    PCOMN_THROW_MSG_IF(!eot(), std::logic_error,
                       "Attempt to send new message when transmission of previous message's data is not completed") ;

    prepare_message(response) ;
    TRACEPX(CHTTP_Connection, DBGL_LOWLEV, "Send prepared message:\n" << response) ;
    connection().transmit(response.str()) ;
    // Set an approprite closed status
    flags(FClosed, response.is_last_message()) ;

    if (size != HTTP_IGNORE_CONTENT)
    {
        if (response.is_chunked_transfer())
            set_chunked_transmit() ;
        else
            // If requested transfer encoding is not chunked, we know the size of content in advance.
            _pending_out = response.content_length() ;

        if (data)
        {
            if (!is_transmit_chunked() && size == (size_t)-1)
                size = _pending_out ;
            if (size != (size_t)-1 && size)
                return transmit(data, size) ;
        }
    }
    return 0 ;
}

inline size_t HTTPConnection::send_impromptu(HTTPMessage &message, const char *data, size_t size)
{
    if (data && size && size != (size_t)-1)
        message.content(default_content(), size) ;
    else
    {
        data = NULL ;
        size = (size_t)-1 ;
    }
    return send_message(message, data, size) ;
}

HTTPMessage &HTTPConnection::prepare_message(HTTPMessage &message)
{
    // If there is content but no content-type set, set the default
    if (message.content_type().empty())
    {
        if (message.has_content())
            message.set_header("content-type", default_content()) ;
    }
    // If there is content-type but the content-length and transfer-encoding are not set,
    // set "transfer-encoding: chunked"
    else if (!message.has_content())
        message.content_length(HTTP_CHUNKED_CONTENT) ;

    return message ;
}

static size_t check_chunked_transmit(size_t size, size_t raw_size, size_t add_size)
{
    WARNPX(CHTTP_Connection, raw_size != add_size + size, DBGL_ALWAYS, "Incomplete chunk transmitted. sent="
           << raw_size << " requested_size=" << size << " add_size=" << add_size) ;

    if (raw_size != add_size + size)
        throw http_logic_error("Incomplete chunk transmitted") ;
    return size ;
}

inline size_t HTTPConnection::transmit_chunk(const iovec_t *begin, const iovec_t *end, size_t size)
{
    char clen[32] ;
    sprintf(clen, "%lx\r\n", (unsigned long)size) ;
    simple_vector<iovec_t> buffers (2 + (end - begin)) ;
    buffers.front() = make_iovec(clen, strlen(clen)) ;
    buffers.back() = make_iovec("\r\n", 2) ;
    std::copy(begin, end, buffers.begin() + 1) ;

    const size_t add_size = buffers.front().iov_len + buffers.back().iov_len ;
    const size_t raw_size = connection().transmit(buffers.begin(), buffers.size()) ;

    return check_chunked_transmit(size, raw_size, add_size) ;
}

// Though this call can be expressed through the transmit_chunk(begin, end, size_t size)
// we use the specialized version for the sake of simplicity and effectiveness
inline size_t HTTPConnection::transmit_chunk(const char *data, size_t size)
{
    char clen[32] ;
    sprintf(clen, "%lx\r\n", (unsigned long)size) ;
    const iovec_t buffers[3] = {
        COMMSVR_IOVEC(clen, strlen(clen)),
        COMMSVR_IOVEC((char *)data, size),
        COMMSVR_IOVEC((char *)"\r\n", 2)
    } ;
    const size_t add_size = buffers[0].iov_len + buffers[2].iov_len ;
    const size_t raw_size = connection().transmit(buffers) ;

    return check_chunked_transmit(size, raw_size, add_size) ;
}

inline size_t HTTPConnection::transmit_chunk(int fd, size_t size, int64_t offset)
{
    NOXPRECONDITION(fd > 0 && size) ;
    char clen[32] ;
    sprintf(clen, "%lx\r\n", (unsigned long)size) ;
    const std::pair<iovec_t, iovec_t> header_footer
        (make_iovec(clen, strlen(clen)), make_iovec("\r\n", 2)) ;

    const size_t add_size = header_footer.first.iov_len + header_footer.second.iov_len ;
    const size_t raw_size = connection().transmit(fd, header_footer, size, offset) ;

    return check_chunked_transmit(size, raw_size, add_size) ;
}

inline size_t HTTPConnection::transmit_chunk(int fd, size_t size, int64_t offset,
                                             const iovec_t &head, const iovec_t &tail)
{
    NOXPRECONDITION(fd > 0 && size) ;

    const size_t header_n_footer = head.iov_len + tail.iov_len ;

    // While we can make a very sophisticated algorithm, use the "quick-n-dirty" one
    if (header_n_footer > FILE_HEADER_REASONABLE_BUF_SIZE)
    {
        size_t sent = 0 ;
        if (head.iov_len)
            sent += transmit_chunk((const char *)head.iov_base, head.iov_len) ;
        sent += transmit_chunk(fd, size, offset) ;
        if (tail.iov_len)
            sent += transmit_chunk((const char *)tail.iov_base, tail.iov_len) ;
        return sent ;
    }

    char clen[32] ;
    char buffer[FILE_HEADER_REASONABLE_BUF_SIZE + sizeof clen + 2] ;
    std::pair<iovec_t, iovec_t> header_footer ;

    sprintf(clen, "%lx\r\n", (unsigned long)(size + header_n_footer)) ;
    const size_t llen = strlen(clen) ;
    const size_t add_size = llen + 2 ;

    strcpy(buffer, clen) ;
    memcpy(buffer + llen, head.iov_base, head.iov_len) ;
    header_footer.first = make_iovec(buffer, llen + head.iov_len) ;
    header_footer.second = make_iovec(memcpy(buffer + header_footer.first.iov_len, tail.iov_base, tail.iov_len),
                                      tail.iov_len+2) ;
    memcpy(buffer + header_footer.first.iov_len + tail.iov_len, "\r\n", 2) ;

    const size_t raw_size = connection().transmit(fd, header_footer, size, offset) ;

    return
        check_chunked_transmit(size + header_n_footer, raw_size, add_size) ;
}

inline size_t HTTPConnection::close_chunked_transmit()
{
    if (is_transmit_chunked())
    {
        close_transmit() ;
        connection().transmit(HTTP_END_CHUNKED_TRANSFER, P_ARRAY_COUNT(HTTP_END_CHUNKED_TRANSFER) - 1) ;
    }
    return 0 ;
}

size_t HTTPConnection::prepare_transmit(int fd, size_t size, int64_t offset, size_t addsize)
{
    if (eot())
        return 0 ;

    if (fd == -1)
        return close_chunked_transmit() ;

    PCOMN_THROW_MSG_IF(addsize > _pending_out && !is_transmit_chunked(),
                       std::out_of_range, "The requested transmit size is greater than pending output size") ;

    if (!size)
        if (!is_transmit_chunked())
            size = _pending_out - addsize ;
        else
        {
            const int64_t current_offset = offset < 0
                ? PCOMN_ENSURE_POSIX(lseek64(fd, 0, SEEK_CUR), "lseek64")
                : offset ;
            struct stat statbuf ;
            PCOMN_ENSURE_POSIX(fstat(fd, &statbuf), "fstat") ;
            size = statbuf.st_size < current_offset
                ? 0
                : std::min<int64_t>(statbuf.st_size - current_offset, std::numeric_limits<uint32_t>::max() - 2) ;
        }

    return size ;
}

size_t HTTPConnection::transmit(const char *data, size_t size)
{
    TRACEPX(CHTTP_Connection, DBGL_LOWLEV, "Transmit data=" << (void *)data << " size=" << size
            << " pending_out=" << _pending_out) ;

    if (!size)
        return close_chunked_transmit() ;

    PCOMN_ENSURE_ARG(data) ;
    if (eot())
        return 0 ;

    if (is_transmit_chunked())
        return transmit_chunk(data, size) ;

    const size_t transmitted = connection().transmit(data, std::min(size, _pending_out)) ;
    _pending_out -= transmitted ;

    return transmitted ;
}

size_t HTTPConnection::transmit(const iovec_t *begin, const iovec_t *end)
{
    NOXPRECONDITION(!begin == !end && begin <= end) ;

    // Calculate the overall size
    size_t overall_size = 0 ;
    for (const iovec_t *bufptr = begin ; bufptr != end ; ++bufptr)
        overall_size += bufptr->iov_len ;

    if (!overall_size)
        return 0 ;

    if (is_transmit_chunked())
        return transmit_chunk(begin, end, overall_size) ;

    // For the contiguous mode check for the overall size not to exceed the remaining content
    if (_pending_out < overall_size)
        throw std::out_of_range(
            "HTTPConnection::transmit(): The requested transmit size is greater than pending output size") ;

    const size_t transmitted = connection().transmit(begin, end) ;
    NOXCHECK(transmitted <= _pending_out) ;
    _pending_out -= transmitted ;

    return transmitted ;
}

size_t HTTPConnection::transmit(int fd, const std::pair<iovec_t, iovec_t> &header_footer, size_t size, int64_t offset)
{
    TRACEPX(CHTTP_Connection, DBGL_LOWLEV, "Transmit file=" << fd << " size=" << size << " offset="
            << offset << " (head, tail)=" << header_footer << " pending_out=" << _pending_out) ;

    if (!header_footer.first.iov_len && !header_footer.second.iov_len)
        return transmit(fd, size, offset) ;

    size = prepare_transmit(fd, size, offset, header_footer.first.iov_len + header_footer.second.iov_len) ;

    if (!size)
        return 0 ;

    if (is_transmit_chunked())
        return transmit_chunk(fd, size, offset, header_footer.first, header_footer.second) ;

    const size_t transmitted = connection().transmit(fd, header_footer, size, offset) ;
    NOXCHECK(transmitted <= _pending_out) ;
    _pending_out -= transmitted ;

    return transmitted ;
}

size_t HTTPConnection::transmit(int fd, size_t size, int64_t offset)
{
    TRACEPX(CHTTP_Connection, DBGL_LOWLEV, "Transmit file=" << fd << " size=" << size
            << " offset=" << offset << " pending_out=" << _pending_out) ;

    size = prepare_transmit(fd, size, offset) ;

    if (!size)
        return 0 ;

    if (is_transmit_chunked())
        return transmit_chunk(fd, size, offset) ;

    size_t transmitted = connection().transmit(fd, size, offset) ;
    NOXCHECK(transmitted <= _pending_out) ;
    _pending_out -= transmitted ;

    return transmitted ;
}

/*******************************************************************************
 HTTPServerConnection
*******************************************************************************/
HTTPServerConnection::HTTPServerConnection(svr::BasicConnection &connection) :
    ancestor(connection),
    _unanswered(0)
{
    set_server_name(HTTP_DEFAULT_SERVER) ;
}

const HTTPRequest &HTTPServerConnection::receive_request(bool skip_rest_data)
{
    PCOMN_THROW_MSG_IF(is_closed(), std::logic_error, "Connection closed") ;

    const HTTPRequest &received =
        static_cast<const HTTPRequest &>(ancestor::receive_message(skip_rest_data)) ;
    flags(FLastRequestHead, received.method() == HTTPRequest::HTTP_HEAD) ;

    return received ;
}

HTTPMessage *HTTPServerConnection::read_message()
{
    // We have to increment _unanswered in advance, because there is possibility of illegal
    // request, hence HTTPRequest constructor can throw exception, in which case we must provide
    // the server with the possibility of sending error response (_unanswered can be 0 before
    // HTTPRequest call but respond() doesn't allow for sending any response except for 100-Continue,
    // if _unanswered == 0)
    ++_unanswered ;
    return new HTTPRequest(input_stream()) ;
}

HTTPMessage &HTTPServerConnection::prepare_message(HTTPMessage &base_response)
{
    HTTPResponse &response = static_cast<HTTPResponse &>(base_response) ;
    if (response.code() == HTTP_RSP_CONTINUE)
        return response ;

    if (!unanswered())
        throw http_logic_error("Unbalanced response") ;

    // Stamp response with current date/time
    time_t timer ;
    time(&timer) ;
    response.set_header("date", *gmtime(&timer)) ;

    // Set server name
    if (server_name().size())
        response.set_header("server", server_name()) ;

    // If the client has requested non-persistent connection (or doesn't want
    // interact with the server any more), automatically mark this response
    // as the last one
    if (!response.is_last_message())
        if (last_message().is_last_message())
            response.set_header(STR_CONNECTION, "close") ;
        else if ((last_message().flags() & HTTPMessage::MSGFKeepAlive) &&
                 !(response.flags() & HTTPMessage::MSGFKeepAlive))
        {
            response.set_header(STR_CONNECTION, STR_KEEP_ALIVE) ;
            response.set_header(STR_KEEP_ALIVE, STR_KEEP_ALIVE_TIMEOUT) ;
        }

    // Handle content parameters
    ancestor::prepare_message(response) ;

    --_unanswered ;

    return response ;
}

size_t HTTPServerConnection::respond(unsigned code, const void *data, size_t size)
{
    HTTPResponse response (code) ;
    // According to RFC, 1xx responses cannot have content data
    if (response_is_continue(code))
    {
        data = NULL ;
        size = 0 ;
    }
    else if (!data && !size)
        response.content_length(0) ;

    return send_impromptu(response, (const char *)data, size) ;
}

/*******************************************************************************
 HTTPClientConnection
*******************************************************************************/
HTTPClientConnection::HTTPClientConnection(svr::BasicConnection &connection, unsigned flags_to_set) :
    ancestor(connection),
    _unanswered(0)
{
    flags(flags_to_set & FUserDefinedFlags) ;
    set_agent_name(HTTP_DEFAULT_CLIENT) ;
}

size_t HTTPClientConnection::request(HTTPRequest &message, const char *data, size_t size)
{
    // Since the darn MS IIS cannot sanely process Absolute URI (although it must! It claims to support HTTP/1.1),
    // use relative URI in the absence of a proxy
    message.flags(HTTPRequest::MSGFUseRelativeURI, !(flags() & FUseProxy)) ;
    size_t result = send_message(message, data, size) ;
    _host = message.host() ;
    flags(FLastRequestHead, message.method() == HTTPRequest::HTTP_HEAD) ;
    return result ;
}

size_t HTTPClientConnection::request(HTTPRequest::Method method, const URI &uri, const char *data, size_t size)
{
    HTTPRequest message (method, uri) ;
    if (data && !size)
        size = strlen(data) ;
    return request(message, data, size) ;
}

HTTPMessage *HTTPClientConnection::read_message()
{
    if (!unanswered())
        throw http_logic_error("HTTP Client: attempt to receive response without previous request.") ;

    --_unanswered ;
    HTTPResponse *received = new HTTPResponse(input_stream()) ;
    // We must ignore 1xx messages while counting answers. In fact, these are
    // kind of keep-alive messages.
    if (response_is_continue(received->code()))
        ++_unanswered ;
    else if (received->is_last_message())
        // If the server has sent the message with the "Connection: close",
        // the client has no rights to send any more requests
        flags(FClosed, true) ;

    return received ;
}

HTTPMessage &HTTPClientConnection::prepare_message(HTTPMessage &base_request)
{
    HTTPRequest &request = static_cast<HTTPRequest &>(base_request) ;

    // Issue some standard headers for better HTTP/1.1 compliance

    // We only want a Content-Encoding of "identity" since we don't
    // support encodings such as x-gzip or x-deflate.
    request.set_header("accept-encoding", "identity") ;

    // Even if 'host' isn't set explicitly, try to extract it from URI's hostinfo.
    // Only in case of failure set the default.
    if (request.version().second >= 1)
        if (!request.uri().host().empty())
            request.set_header("host", hostinfo(request.uri().host().stdstring(), request.uri().port())) ;
        else if (request.host().empty() && !host().empty())
            request.set_header("host", hostinfo(host(), peer().port())) ;

    request.set_header("user-agent", user_agent()) ;
    // For HTTP/1.1 messages add "Connection: keep-alive" header if there is no
    // "Connection: close". Though the HTTP/1.1 protocol supposes the connection is
    // persistent if the opposite is not said, there can be malicious proxy converting
    // the HTTP/1.1 message into the HTTP/1.0. So add "Connection: keep-alive" just in case
    if (!request.is_last_message() && request.version().second > 0)
    {
        request.set_header(STR_CONNECTION, STR_KEEP_ALIVE) ;
        request.set_header(STR_KEEP_ALIVE, STR_KEEP_ALIVE_TIMEOUT) ;
    }

    // Handle content parameters
    ancestor::prepare_message(base_request) ;

    ++_unanswered ;

    return request ;
}

} // end of namespace pcomn::http
} // end of namespace pcomn
