/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
#ifndef __HTTP_MESSAGE_H
#define __HTTP_MESSAGE_H
/*******************************************************************************
 FILE         :   http_message.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   HTTP request and response objects.

 CREATION DATE:   8 Mar 2008
*******************************************************************************/
#include <pcomn_http/http_exceptions.h>
#include <pcomn_http/http_constants.h>

#include <pcommon.h>
#include <pcomn_uri.h>
#include <pcomn_binstream.h>
#include <pcomn_string.h>
#include <pcomn_utils.h>
#include <pcomn_regex.h>

#include <map>
#include <time.h>

namespace pcomn {
namespace http {

using uri::query_dictionary ;
using uri::URI ;

typedef struct tm tm_t ;

const char *strtotime(const char *str, tm_t &value) ;

/// Convert GMT time into the RFC1123-compliant format
inline std::string timetostr(const tm_t &value)
{
    char buf[32] ;
    strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S GMT", &value) ;
    return std::string(buf) ;
}

/******************************************************************************/
/** "HTTP-oriented" dictionary, wrapper around the standard mapping, to use as an HTTP
 message "headers container".
 Adds methods to insert/assign and retrieve integer and time values thus programmer
 needn't bother about converting these to and from string.
*******************************************************************************/
class headers_dictionary : private uri::query_dictionary {
    typedef uri::query_dictionary ancestor ;
public:
    using ancestor::clear ;
    using ancestor::const_iterator ;
    using ancestor::begin ;
    using ancestor::end ;
    using ancestor::empty ;
    using ancestor::size ;
    using ancestor::find ;
    using ancestor::get ;
    using ancestor::has_key ;

    /// Insert a new header/value pair.
    /// Does @em not replace existing header.
    /// @return true, if there if were was no @a header in the dictionary (new header
    /// added); false, if @a header is already present.
    bool insert(const std::string &header, const std::string &value)
    {
        return ancestor::insert(header, value) ;
    }

    /// Insert a new header/value pair, where value is integer.
    /// Does @em not replace existing header.
    /// @return true, if there if were was no @a header in the dictionary (new header
    /// added); false, if @a header is already present.
    bool insert(const std::string &header, int value)
    {
        return ancestor::insert(header, value) ;
    }

    /// Insert a new time value.
    /// The time value is automatically converted into a string of the RFC 1123 format
    /// before inserting.
    bool insert(const std::string &header, const tm_t &value)
    {
        return insert(header, timetostr(value)) ;
    }

    const std::string &assign(const std::string &header, const std::string &value)
    {
        return (*this)[header] = value ;
    }

    const std::string &assign(const std::string &header, int value)
    {
        return assign(header, strprintf("%d", value)) ;
    }

    const std::string &assign(const std::string &header, const tm_t &value)
    {
        return assign(header, timetostr(value)) ;
    }

    void erase(const std::string &header) { ancestor::erase(header) ; }

} ;

/******************************************************************************/
/** The HTTPMessage class specifies the common interface to an HTTP message, both
HTTP request and HTTP response.

 It provides various means for setting/requesting HTTP message headers and properties,
 such as the HTTP version.
 This is an abstract class. The concrete derived classes are Request and
 HTTPResponse.

 To set HTTP headers, use the set_header() family of members, e.g.:
 @code
 message.set_header("connection", "close") ;
 message.set_header("keep-alive", 300) ;
 @endcode

 To request HTTP headers, use the headers() member, which returns a constant reference to
 the message's internal headers dictionary, e.g:
 @code
 tm_t message_date ;
 message.headers().find_date("date", message_date) ;
 std::string lang (message.headers().get("accept-language")) ;
 @endcode
*******************************************************************************/
class HTTPMessage {
public:
    enum Flags {
        MSGFChunkedTransfer = 0x00001,   /**< The "Transfer-Encoding: chunked" header is present */
        MSGFCloseConnection = 0x00002,   /**< The "Connection: close" header is present */
        MSGFKeepAlive       = 0x00004,   /**< The "Keep-Alive" header is present */

        MSGFUserDefinedFlags   = 0xFFFF0000, /**< This is the "private flags area", i.e. top 16 bits
                                                  of flags can be set by the programmer. */
        MSGFAllowArbitraryHeaders = 0x10000  /**< Allow to set arbitrary (correctly-formed) headers,
                                                  not only those mentioned in the RFC. */
    } ;

    /// The destructor is only to provide a virtual stub for the derived classes
    virtual ~HTTPMessage() {}

    /// Get HTTP version.
    const std::pair<unsigned, unsigned> &version() const { return  _http_version ; }

    /// Set HTTP version.
    const std::pair<unsigned, unsigned> &set_version(unsigned major, unsigned minor)
    {
        return _http_version = std::pair<unsigned, unsigned>(major, minor) ;
    }

    /// Get the HTTP message headers dictionary.
    /// The dictionary contains HTTP header valuse, keyed by HTTP header names in lowercase.
    /// @note Please note that keys in that dictionary (i.e. header names) are @em always
    /// @em in @em lowercase (e.g. "content-type", "transfer-encoding").
    const headers_dictionary &headers() const { return _headers ; }

    /// Get the message's first line.
    /// For HTTPRequest it is a command line ("METHOD URI HTTP/1.x")
    /// For HTTPResponse it is a response line ("XXX message HTTP/1.x")
    const std::string &first_line() const
    {
        if (!_first_line.size())
            const_cast<HTTPMessage *>(this)->_first_line = get_first_line() ;
        return _first_line ;
    }

    /// Get the string representation of a message
    /// Returns string which can be printed or directly sent as an HTTP message.
    /// @param eolmode    Specifies whether to use the "\r\n" pair as a delimiter (RFC -
    /// compliant) or single "\n" (useful for printing, etc.).
    std::string str(EOLMode eolmode = eolmode_CRLF) const ;

    /// Get the value of the "Content-Type" header
    /// Equivalent of, but much faster than, headers().find("content-type").
    /// This function keeps track of set_header() calls as well as content() calls, i.e.
    /// set_header("content-type", "some/type") _will_ influence the result to be returned
    /// from this function.
    const std::string &content_type() const { return _content_type ; }

    /// Get the value of the "Content-Type" header.
    /// @see content_type
    size_t content_length() const { return _content_length ; }

    /// Get the value of the "Host" header.
    /// @see content_type
    const std::string &host() const { return _host ; }

    /// Check whether the message specifies chunked transfer encoding.
    bool is_chunked_transfer() const { return flags() & MSGFChunkedTransfer ; }

    /// Check whether this is the last message in HTTP session.
    /// In HTTP 1.1, the default connection mode is persistent and the last message in the
    /// session should have 'Connection: close' header.
    /// In HTTP 1.0, every message is "last", except when "Keep-Alive" header is present.
    bool is_last_message() const
    {
        return
            (flags() & HTTPMessage::MSGFCloseConnection) ||
            version().second == 0 && !(flags() & HTTPMessage::MSGFKeepAlive) ;
    }

    /// Indicates whether the message has any content (entity).
    bool has_content() const { return is_chunked_transfer() || content_length() ; }

    /// Get message flags.
    /// The flags are values from HTTPMessage::Flags, ORed together.
    bigflag_t flags() const { return _flags ; }

    /// Add/set/remove HTTP header.
    /// @param header The name of header. This parameter is case-insensitive, it is
    /// converted to lowercase automatically.
    /// @param value  The value of the header. If this parameter is an empty string,
    /// the corresponding  header is erased (i.e. completely removed from headers
    /// dictionary).
    ///
    /// @note If MSGFAllowArbitraryHeaders flag is not set, @a header should specify
    /// a valid HTTP 1.1 header, i.e. one of the headers specified in RFC2616, otherwise
    /// the function throws pcomn::http::invalid_header exception. If
    /// MSGFAllowArbitraryHeaders is set, any well-formed header is allowed.
    void set_header(const std::string &header, const std::string &value) ;

    void set_header(const std::string &header, unsigned value)
    {
        set_header(header, strprintf("%u", value)) ;
    }
    void set_header(const std::string &header, const tm_t &value)
    {
        set_header(header, timetostr(value)) ;
    }

    void erase_header(const std::string &header)
    {
        set_header(header, std::string()) ;
    }

    /// Set the content length for this message.
    /// Though the same effect can be achieved through direct set_header() calls, calling
    /// this function is more obvious and less error-prone.
    /// @param length The 'content-length' header value.
    /// @note If parameter @a length is HTTP_CHUNKED_CONTENT (-1), this function removes
    /// the Content-Length header and sets 'transfer-encoding: chunked'.
    void content_length(size_t length) ;

    /// Set the content type for this message.
    /// Though the same effect can be achieved through direct set_header() calls, calling
    /// this function is more obvious and less error-prone.
    /// @param type The 'content-type' header value.
    void content(const std::string &type) ;

    /// Set both the the content type and content length (and, possibly, transfer
    /// encoding) for the message.
    /// Combines content and content_length in one call.
    /// @param type The 'content-type' header value.
    /// @param length The 'content-length' header value.
    /// @note If parameter @a length is HTTP_CHUNKED_CONTENT (-1), this function removes
    /// the Content-Length header and sets 'transfer-encoding: chunked'.
    void content(const std::string &type, size_t length)
    {
        content(type) ;
        content_length(length) ;
    }

protected:
    HTTPMessage(bigflag_t flags = 0) ;

    virtual std::string get_first_line() const { return _first_line ; }
    virtual void final_check() {}

    void parse(binary_ibufstream &stream) ;
    void set_n_check_version(const char *str, const reg_match &major, const reg_match &minor) ;

    bigflag_t flags(bigflag_t value, bool on = true)
    {
        bigflag_t old = _flags ;
        set_flags(_flags, on, value) ;
        return old ;
    }

private:
    headers_dictionary  _headers ;
    bigflag_t           _flags ;
    std::pair<unsigned, unsigned> _http_version ;
    std::string         _first_line ;
    std::string         _last_line ;

    // We provide some specific fields separately (in addition to their raw values,
    // which are stored in the _headers dictionary)
    size_t      _content_length ;
    std::string _content_type ;
    std::string _host ;

    virtual void parse_first_line(const std::string &command_line) = 0 ;
    void parse_headers(binary_ibufstream &stream) ;
    bool next_header(binary_ibufstream &stream, std::string &header, std::string &value) ;
    const std::string &next_header_line(binary_ibufstream &stream) ;
    void check_special_header(const std::string *entry, const std::string &value) ;
} ;

/******************************************************************************/
/** HTTP request message.
*******************************************************************************/
class HTTPRequest : public HTTPMessage {
    typedef HTTPMessage ancestor ;
public:
    enum Method {
        HTTP_EXTENSION,
        HTTP_GET,
        HTTP_HEAD,
        HTTP_POST,
        HTTP_PUT,
        HTTP_DELETE,
        HTTP_TRACE,
        HTTP_CONNECT
    } ;

    enum Flags {
        MSGFAllowExtensionMethods = 0x40000, /**< Allow using methods other than defined in RFC */
        MSGFUseRelativeURI        = 0x80000  /**< Place relative URI (HTTP/1.0 style) into the request line */
    } ;

    /// Constructor: read incoming request from an input stream and parse it into the
    /// request object being constructed.
    /// This constructor is usually used by the HTTP server, to read a request from the
    /// connection.
    /// @param stream   An input stream to parse a request from.
    HTTPRequest(binary_ibufstream &stream, bigflag_t flags = 0) ;

    /// Constructor: create a request with given HTTP method, URL and flags.
    /// This constructor is usually used by the HTTP client, to prepare a request to a server.
    /// @param method
    /// @param uri
    /// @param flags
    HTTPRequest(Method method, const URI &uri, bigflag_t flags = 0) ;

    Method method() const { return _method ; }
    const std::string &method_name() const { return _method_name ; }

    /// Get the request's URI.
    const URI &uri() const { return _request_uri ; }

    /// Set new URI for the request.
    const URI &set_uri(const URI &new_uri) ;

    /// Get received URL-encoded query fields.
    const query_dictionary &query_fields() const { return _query_fields ; }

    using ancestor::flags ;
    bigflag_t flags(bigflag_t value, bool on = true)
    {
        return ancestor::flags(value & MSGFUseRelativeURI, on) ;
    }

protected:
    // Override ancestor methods.
    void final_check() ;

private:
    Method              _method ;
    std::string         _method_name ;
    URI                 _request_uri ; /* The full URI part of a request line */
    query_dictionary    _query_fields ;

    void check_method() ;

    // Override ancestor methods.
    std::string get_first_line() const ;
    void parse_first_line(const std::string &command_line) ;
} ;

/******************************************************************************/
/** HTTP response message.
*******************************************************************************/
class HTTPResponse : public HTTPMessage {
    typedef HTTPMessage ancestor ;
public:
    /// Constructor.
    /// @param response_code
    /// @param response_text
    /// @param flags
    explicit HTTPResponse(unsigned response_code,
                          const std::string &response_text = std::string(),
                          bigflag_t flags = 0) ;

    /// Constructor reads a response from the communication stream.
    /// According to principle "be as strict as possible to itself and as tolerant as possible to others",
    /// allows client by default to accept any correctly-formed headers (not only standard ones).
    explicit HTTPResponse(binary_ibufstream &stream, bigflag_t flags = MSGFAllowArbitraryHeaders) ;

    /// Get the response code.
    /// Returns "pure" HTTP response code (i.e. w/o HTTP_RSPFLAG_CLOSE or any other flag bits).
    unsigned code() const { return _code ; }

    /// Get the HTTP message.
    /// This is the third field in the response line (e.g. "OK" for "HTTP/1.1 200 OK").
    const std::string &message() const { return _message ; }

    /// Set HTTP response code.
    /// @param response_code HTTP response code, like 200, 403, etc. This parameter may be
    /// ORed with flag bits, like HTTP_RSPFLAG_CLOSE, the same way as @a response_code in
    /// the constructor.
    /// @throw std::invalid_argument if after stripping flag bits the resulting code is
    /// out of valid range (HTTP_RSP_MIN_CODE, HTTP_RSP_MAX_CODE).
    void code(unsigned response_code, const std::string &response_text = std::string()) ;

private:
    unsigned    _code ;
    std::string _message ;

    // Override ancestor methods.
    std::string get_first_line() const ;
    void parse_first_line(const std::string &response_line) ;
} ;

inline std::ostream &operator<<(std::ostream &os, const HTTPMessage &msg)
{
    return os << msg.str(eolmode_LF) ;
}

} // end of namespace pcomn::http
} // end of namespace pcomn

#endif /* __HTTP_MESSAGE_H */
