/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
/*******************************************************************************
 FILE         :   http_message.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   HTTP request and response objects.

 CREATION DATE:   8 Mar 2008
*******************************************************************************/
#include <pcomn_http/http_message.h>
#include <pcomn_http/http_diag.h>

#include <pcomn_regex.h>
#include <pcomn_syncobj.h>
#include <pcomn_utils.h>
#include <pcomn_config.h>
#include <pcstring.h>

#include <unordered_set>

typedef std::unordered_set<std::string> interned_stringset ;

#include <arpa/inet.h>
#include <ctype.h>
#include <stdio.h>

namespace pcomn {
namespace http {

#define HTTP_SEPARATORS_NOWS "][()<>@,;:\\\"/?={}\t"
#define HTTP_SEPARATORS HTTP_SEPARATORS_NOWS " "
#define HTTP_TOKEN "[^" HTTP_SEPARATORS "]+"

static const char REQUEST_REGEXP[] = "^(" HTTP_TOKEN ")[ \t]+([^ \t]+)[ \t]+HTTP/([0-9]).([0-9])[ \t]*\n?$" ;
static const char METHOD_REGEXP[] = "^((GET)|(HEAD)|(POST)|(PUT)|(DELETE)|(TRACE)|(CONNECT))$" ;
static const char HEADER_REGEXP[] = "^(" HTTP_TOKEN "[^" HTTP_SEPARATORS_NOWS "]*" HTTP_TOKEN "):[ \t]*([^ \t].*)\n?$" ;
static const char RESPONSE_REGEXP[] = "^HTTP/([0-9]).([0-9])[ \t]+([1-9][0-9][0-9])[ \t]+(.*)[ \t]*\n?$" ;

static const char TIME_RFC1123_REGEXP[] = "^([A-Z][a-z][a-z]), ([0-3][0-9]) ([A-Z][a-z][a-z]) ([0-9][0-9][0-9][0-9])"
    " ([0-2][0-9]):([0-5][0-9]):([0-5][0-9]) GMT" ;
static const char TIME_ASCTIME_REGEXP[] = "^([A-Z][a-z][a-z]) ([A-Z][a-z][a-z]) ([0-3]?[0-9]) ([0-9][0-9][0-9][0-9])"
    " ([0-2][0-9]):([0-5][0-9]):([0-5][0-9])" ;

static const unsigned HTTP_MINVER_MIN = 0 ;
static const unsigned HTTP_MINVER_MAX = 1 ;

static const unsigned HTTP_NUM_OF_COMMANDS = 7 ;

static const std::string __content_length ("content-length") ;
static const std::string __content_type ("content-type") ;
static const std::string __connection ("connection") ;
static const std::string __host ("host") ;
static const std::string __chunked ("chunked") ;
static const std::string __colon (": ") ;
static const std::string __unknown_message ("Unknown") ;

static const struct status_line_desc {
    unsigned    code ;
    const char *message ;

    bool operator<(const status_line_desc &other) const { return code < other.code ; }

} __status_lines[] = {
    {100, "Continue"},
    {101, "Switching Protocols"},
    {102, "Processing"},
    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    {203, "Non-Authoritative Information"},
    {204, "No Content"},
    {205, "Reset Content"},
    {206, "Partial Content"},
    {207, "Multi-Status"},

    {300, "Multiple Choices"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {303, "See Other"},
    {304, "Not Modified"},
    {305, "Use Proxy"},
    {306, "unused"},
    {307, "Temporary Redirect"},

    {400, "Bad Request"},
    {401, "Authorization Required"},
    {402, "Payment Required"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {406, "Not Acceptable"},
    {407, "Proxy Authentication Required"},
    {408, "Request Time-out"},
    {409, "Conflict"},
    {410, "Gone"},
    {411, "Length Required"},
    {412, "Precondition Failed"},
    {413, "Request Entity Too Large"},
    {414, "Request-URI Too Large"},
    {415, "Unsupported Media Type"},
    {416, "Requested Range Not Satisfiable"},
    {417, "Expectation Failed"},
    {418, "unused"},
    {419, "unused"},
    {420, "unused"},
    {421, "unused"},
    {422, "Unprocessable Entity"},
    {423, "Locked"},
    {424, "Failed Dependency"},

    {500, "Internal Server Error"},
    {501, "Method Not Implemented"},
    {502, "Bad Gateway"},
    {503, "Service Temporarily Unavailable"},
    {504, "Gateway Time-out"},
    {505, "HTTP Version Not Supported"},
    {506, "Variant Also Negotiates"},
    {507, "Insufficient Storage"},
    {508, "unused"},
    {509, "unused"},
    {510, "Not Extended"},

    {0, ""}
} ;

static const char * const __http_headers[] = {
    "date",
    "pragma",
    "trailer",
    "upgrade",
    "via",
    "warning",
    "accept",
    "accept-charset",
    "accept-encoding",
    "accept-language",
    "authorization",
    "expect",
    "from",
    "if-match",
    "if-modified-since",
    "if-none-match",
    "if-range",
    "if-unmodified-since",
    "max-forwards",
    "proxy-authorization",
    "range",
    "referer",
    "te",
    "user-agent",
    "accept-ranges",
    "age",
    "etag",
    "location",
    "proxy-authenticate",
    "retry-after",
    "server",
    "vary",
    "www-authenticate",
    "allow",
    "content-encoding",
    "content-language",
    "content-location",
    "content-md5",
    "content-range",
    "expires",
    "last-modified",
    "cache-control"
} ;

static const char *const __month_names[] =
{
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec"
} ;

// Month descriptions sorted by names
static const struct month_desc {
    union {
        char name[4] ;
        uint32_t nval ;
    } ;
    unsigned num ;

    bool operator<(const month_desc &rhs) const
    {
        return ntohl(nval) < ntohl(rhs.nval) ;
    }
} __month_descs[] =
{
    {{"Apr"}, 4},
    {{"Aug"}, 8},
    {{"Dec"}, 12},
    {{"Feb"}, 2},
    {{"Jan"}, 1},
    {{"Jul"}, 7},
    {{"Jun"}, 6},
    {{"Mar"}, 3},
    {{"May"}, 5},
    {{"Nov"}, 11},
    {{"Oct"}, 10},
    {{"Sep"}, 9},

    {{""}, 0}
} ;

static const std::string &get_method_name(HTTPRequest::Method method)
{
    static const std::string _method_names[] =
        {
            "GET",
            "HEAD",
            "POST",
            "PUT",
            "DELETE",
            "TRACE",
            "CONNECT"
        } ;
    static const std::string _extension ;
    return
        inrange(method, HTTPRequest::HTTP_GET, HTTPRequest::HTTP_CONNECT) ?
        _method_names[method - HTTPRequest::HTTP_GET] : _extension ;
} ;

/*******************************************************************************
 helper_data
*******************************************************************************/
class helper_data : private interned_stringset {
public:
    helper_data() :
        __regexp_request(REQUEST_REGEXP),
        __regexp_method(METHOD_REGEXP),
        __regexp_header(HEADER_REGEXP),
        __regexp_response(RESPONSE_REGEXP),
        __regexp_rfctime(TIME_RFC1123_REGEXP),
        __regexp_asctime(TIME_ASCTIME_REGEXP),

        __content_length_atom(intern("content-length")),
        __content_type_atom(intern("content-type")),
        __transfer_encoding_atom(intern("transfer-encoding")),
        __host_atom(intern("host")),
        __connection_atom(intern("connection")),
        __keepalive_atom(intern("keep-alive"))
    {
        std::for_each(__http_headers + 0, __http_headers + P_ARRAY_COUNT(__http_headers),
                      std::bind1st(std::mem_fun(&helper_data::intern), this)) ;
    }

    std::string status_line(unsigned code) const
    {
        const status_line_desc key = {code, NULL} ;
        const status_line_desc * const found =
            std::lower_bound(__status_lines + 0, __status_lines + P_ARRAY_COUNT(__status_lines) - 1, key) ;
        return found->code == code ? std::string(found->message) : std::string() ;
    }

    const regex __regexp_request ;
    const regex __regexp_method ;
    const regex __regexp_header ;
    const regex __regexp_response ;

    const regex __regexp_rfctime ;
    const regex __regexp_asctime ;

    const std::string * const __content_length_atom ;
    const std::string * const __content_type_atom ;
    const std::string * const __transfer_encoding_atom ;
    const std::string * const __host_atom ;
    const std::string * const __connection_atom ;
    const std::string * const __keepalive_atom ;

    unsigned month(const std::string &name) const
    {
        if (name.size() != 3)
            return 0 ;
        month_desc key ;
        memcpy(key.name, name.c_str(), 4) ;
        const month_desc *found =
            std::lower_bound(__month_descs+0, __month_descs+P_ARRAY_COUNT(__month_descs)-1, key) ;
        return !strcmp(found->name, key.name) ? found->num : 0 ;
    }

    const std::string *header(const std::string &name) const
    {
        const const_iterator found (find(name)) ;
        return found == end() ? NULL : &*found ;
    }

private:
    const std::string *intern(const char *header) ;
} ;

const std::string *helper_data::intern(const char *header)
{
    return &(*insert(header).first) ;
}

static const helper_data &http_helper()
{
    static const helper_data helper ;
    return helper ;
}

/*******************************************************************************
 HTTPMessage
*******************************************************************************/
HTTPMessage::HTTPMessage(bigflag_t flags) :
    _flags(flags & MSGFUserDefinedFlags),
    _http_version(1, 1),
    _content_length(0)
{}

void HTTPMessage::parse(binary_ibufstream &stream)
{
    eof_guard guard (stream, true) ;

    _first_line = pcomn::readline(stream, eolmode_CRLF) ;
    size_t ssz = _first_line.size() ;
    if (ssz && _first_line[ssz - 1] == '\n')
        _first_line.resize(ssz - 1) ;

    TRACEPX(CHTTP_Message, DBGL_VERBOSE, "Line='" << _first_line << '\'') ;

    parse_first_line(_first_line) ;
    parse_headers(stream) ;
}

inline const std::string &HTTPMessage::next_header_line(binary_ibufstream &stream)
{
    if (_last_line.empty())
    {
        _last_line = pcomn::readline(stream, eolmode_CRLF) ;
        TRACEPX(CHTTP_Message, DBGL_VERBOSE, "Head='" << str::rstrip(_last_line) << '\'') ;
    }
    return _last_line ;
}

bool HTTPMessage::next_header(binary_ibufstream &stream, std::string &header, std::string &value)
{
    next_header_line(stream) ;

    if (_last_line.empty() || _last_line[0] == '\n')
        return false ;

    reg_match matches[3] ;
    if (!http_helper().__regexp_header.is_matched(_last_line, matches))
        throw invalid_header("Invalid header line: " + _last_line) ;
    header = str::substr(_last_line, matches[1]) ;
    value = str::substr(_last_line, matches[2]) ;
    _last_line = std::string() ;
    while(next_header_line(stream).size() && strchr(" \t", _last_line[0]))
    {
        value += _last_line ;
        _last_line = std::string() ;
    }
    str::strip_inplace(value) ;

    return true ;
}

void HTTPMessage::parse_headers(binary_ibufstream &stream)
{
    std::string header ;
    std::string value ;
    while(next_header(stream, header, value))
        set_header(header, value) ;

    final_check() ;
}

inline void HTTPMessage::check_special_header(const std::string *entry, const std::string &value)
{
    const helper_data &hh = http_helper() ;

    if (entry == hh.__host_atom)
        _host = value ;

    else if (entry == hh.__content_length_atom)
        _content_length = value.empty() ? 0 : atol(value.c_str()) ;

    else if (entry == hh.__content_type_atom)
        _content_type = value ;

    else if (entry == hh.__transfer_encoding_atom)
        flags(MSGFChunkedTransfer, !stricmp(value.c_str(), "chunked")) ;

    else if (entry == hh.__connection_atom)
        // We set closing flag in the following two cases:
        //    set_header("connection", "close") (explicit request)
        //    erase_header("connection") and this is an HTTP/1.0 header
        flags(MSGFCloseConnection, !stricmp(value.c_str(), "close")) ;

    else if (entry == hh.__keepalive_atom)
        flags(MSGFKeepAlive, true) ;
}

void HTTPMessage::set_header(const std::string &header, const std::string &value)
{
    // Normalize header first (get it to lowercase), then search it among the
    // allowed (interned) headers.
    const std::string normalized_header (str::to_lower(header)) ;
    const std::string *header_key = http_helper().header(normalized_header) ;

    if (!header_key)
        if ((flags() & MSGFAllowArbitraryHeaders) &&
            normalized_header.find_first_of(HTTP_SEPARATORS) == std::string::npos)
            header_key = &normalized_header ;
        else {
            TRACEPX(CHTTP_Message, DBGL_NORMAL, "Ignore set unknown header: " << header) ;
            return;
        }

    if (!value.empty())
        _headers.assign(*header_key, value) ;
    else
        _headers.erase(*header_key) ;

    // Now check for some selected values (for _content_length, _content_type, etc.
    // to be synchronized properly).
    check_special_header(header_key, value) ;
}

void HTTPMessage::set_n_check_version(const char *str, const reg_match &major, const reg_match &minor)
{
    set_version(atoi(PSUBEXP_BEGIN(str, major)), atoi(PSUBEXP_BEGIN(str, minor))) ;
    if (version().first != 1 ||
        !inrange(version().second, (unsigned)HTTP_MINVER_MIN, (unsigned)HTTP_MINVER_MAX))
        throw unsupported_version(version().first, version().second) ;
}

std::string HTTPMessage::str(EOLMode eolmode) const
{
    const char * const delimiter = eolmode == eolmode_CRLF ? "\r\n" : "\n" ;

    std::string result (get_first_line()) ;
    result.append(delimiter) ;
    for(headers_dictionary::const_iterator iter (headers().begin()), end (headers().end()) ; iter != end ; ++iter)
    {
        // There are some servers/agents that for some misty reason require header names
        // in form "Accept-Encoding", etc., though the RFC directly states case-insensitivity
        // of headers.
        const size_t header_start = result.length() ;
        result.append(iter->first) ;
        for(size_t begpos = result.length() ;
            (begpos = result.find_last_of('-', begpos)) > header_start && begpos != std::string::npos ;
            --begpos)
            if (begpos < result.length() - 1)
                result[begpos+1] = toupper(result[begpos+1]) ;
        result[header_start] = toupper(result[header_start]) ;

        result.append(__colon)
            .append(iter->second)
            .append(delimiter) ;
    }
    return result.append(delimiter) ;
}

void HTTPMessage::content_length(size_t length)
{
    const helper_data &h = http_helper() ;

    if (length == HTTP_CHUNKED_CONTENT)
    {
        erase_header(*h.__content_length_atom) ;
        set_header(*h.__transfer_encoding_atom, __chunked) ;
    }
    else
    {
        erase_header(*h.__transfer_encoding_atom) ;
        set_header(*h.__content_length_atom, length) ;
    }
}

void HTTPMessage::content(const std::string &type)
{
    set_header(*http_helper().__content_type_atom, type) ;
}

/*******************************************************************************
 HTTPRequest
*******************************************************************************/
HTTPRequest::HTTPRequest(binary_ibufstream &stream, bigflag_t flags) :
    ancestor(flags),
    _method(HTTP_EXTENSION)
{
    parse(stream) ;
}

HTTPRequest::HTTPRequest(Method method, const URI &req_uri, bigflag_t flags) :
    ancestor(flags),
    _method(method),
    _method_name(get_method_name(method))
{
    NOXCHECK(!_method_name.empty()) ;
    set_uri(req_uri) ;
}

const URI &HTTPRequest::set_uri(const URI &uri)
{
    _request_uri = PCOMN_ENSURE_ARG(uri) ;
    // Save a URI query part into the internal dictionary
    if (!_request_uri.query().empty())
        uri::query_decode(_request_uri.query().begin(), _query_fields) ;
    return _request_uri ;
}

void HTTPRequest::parse_first_line(const std::string &command_line)
{
    reg_match matches[5] ; // A full box of...
    if (!http_helper().__regexp_request.is_matched(command_line, matches))
        throw invalid_request("Invalid request string: " + command_line) ;

    _method_name = str::substr(command_line, matches[1]) ;
    check_method() ;
    _request_uri = URI(str::substr(command_line, matches[2])) ;
    // Parse a URI query part, if exists
    if (!_request_uri.query().empty())
        uri::query_decode(_request_uri.query().begin(), _query_fields) ;

    set_n_check_version(command_line.c_str(), matches[3], matches[4]) ;
}

void HTTPRequest::check_method()
{
    reg_match matches[HTTP_NUM_OF_COMMANDS + 2] ;
    int meth_num = http_helper().__regexp_method.match(_method_name, matches) - matches ;
    if (meth_num)
        _method = (Method)(meth_num - 2) ;
    else if (flags() & MSGFAllowExtensionMethods)
        _method = HTTP_EXTENSION ;
    else
        throw invalid_method(_method_name) ;
}

std::string HTTPRequest::get_first_line() const
{
    const_cast<HTTPRequest *>(this)->_request_uri = URI(_request_uri, query_fields()) ;
    return
        strprintf("%s %s HTTP/%u.%u",
                  method_name().c_str(),
                  uri().str((flags() & MSGFUseRelativeURI) ? pcomn::uri::ABS_PATH : pcomn::uri::ABS_URL).c_str(),
                  version().first,
                  version().second) ;
}

void HTTPRequest::final_check()
{}

/*******************************************************************************
 HTTPResponse
*******************************************************************************/
HTTPResponse::HTTPResponse(unsigned response_code, const std::string &response_text, bigflag_t flags) :
    ancestor(flags),
    _code(0)
{
    code(response_code, response_text) ;
}

HTTPResponse::HTTPResponse(binary_ibufstream &stream, bigflag_t flags) :
    ancestor(flags),
    _code(0)
{
    parse(stream) ;
}

void HTTPResponse::parse_first_line(const std::string &response_line)
{
    reg_match matches[5] ;
    if (!http_helper().__regexp_response.is_matched(response_line, matches))
        throw response_error("Invalid response string: " + response_line) ;

    set_n_check_version(response_line.c_str(), matches[1], matches[2]) ;
    _code = atoi(PSUBEXP_BEGIN(response_line.c_str(), matches[3])) ;
    _message = str::substr(response_line, matches[4]) ;
}

std::string HTTPResponse::get_first_line() const
{
    return strprintf("HTTP/%u.%u %u %s", version().first, version().second, code(), message().c_str()) ;
}

void HTTPResponse::code(unsigned response_code, const std::string &response_text)
{
    unsigned flag_part = response_code & HTTP_RSPFLAG_FLAGS ;
    response_code &= ~HTTP_RSPFLAG_FLAGS ;

    if (!inrange(response_code, HTTP_RSP_CODE_MIN, HTTP_RSP_CODE_MAX))
        throw std::invalid_argument(strprintf("Illegal response code: %u", response_code)) ;

    _code = response_code ;
    _message = response_text.empty() ? http_helper().status_line(response_code) : response_text ;
    if (_message.empty())
        _message = __unknown_message ;

    if (flag_part & HTTP_RSPFLAG_CLOSE)
        set_header(__connection, "close") ;
}

const char *strtotime(const char *str, tm_t &value)
{
    static const char rfctime_fmt[] = "%3s, %2u %3s %4u %2u:%2u:%2u GMT" ;
    static const char asctime_fmt[] = "%3s %3s %u %2u:%2u:%2u %4u" ;

    reg_match match ;
    char month_name[4] ;
    char dow[4] ;
    unsigned year ;
    unsigned day ;
    unsigned hour ;
    unsigned minute ;
    unsigned second ;

    if (http_helper().__regexp_rfctime.is_matched(str, &match, 1))
        sscanf(PSUBEXP_BEGIN(str, match), rfctime_fmt, dow + 0, &day, month_name + 0, &year, &hour, &minute, &second) ;
    else if (http_helper().__regexp_asctime.is_matched(str, &match, 1))
        sscanf(PSUBEXP_BEGIN(str, match), asctime_fmt, dow + 0, month_name + 0, &day, &hour, &minute, &second, &year) ;
    else
        return NULL ;

    unsigned month = http_helper().month(month_name) ;
    if (!month)
        return NULL ;

    memset(&value, 0, sizeof value) ;
    value.tm_hour = hour ;
    value.tm_min = minute ;
    value.tm_sec = second ;
    value.tm_year = year ;
    value.tm_mon = month ;
    value.tm_mday = day ;

    return PSUBEXP_END(str, match) ;
}

} // end of namespace pcomn::http
} // end of namespace pcomn
