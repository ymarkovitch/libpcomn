/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
#ifndef __HTTP_EXCEPTIONS_H
#define __HTTP_EXCEPTIONS_H
/*******************************************************************************
 FILE         :   http_exceptions.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   HTTP exceptions hierarchy

 CREATION DATE:   3 Mar 2008
*******************************************************************************/
#include <pcomn_string.h>
#include <pcomn_except.h>

#include <stdexcept>

namespace pcomn {
namespace http {

/******************************************************************************/
/** The base class for all HTTP runtime exceptions.
 @para Note that http_error and its descendants are considered "runtime errors" (i.e. not
 "logical errors"): these are not programmer errors, at least they are not avoidable by
 the programmer at that end of the protocol where they are detected and this exception is
 thrown.
 @endpara
*******************************************************************************/
class http_error : public std::runtime_error {
    typedef std::runtime_error ancestor ;
public:
    explicit http_error(const std::string &msg) :
        ancestor(msg)
    {}
} ;

/******************************************************************************/
/**
*******************************************************************************/
class http_logic_error : public std::logic_error {
    typedef std::logic_error ancestor ;
public:
    explicit http_logic_error(const std::string &msg) :
        ancestor(msg)
    {}
} ;

/******************************************************************************/
/** This exception can be thrown by both the client and the server to indicate that HTTP
 connection has been closed by the peer.
*******************************************************************************/
class http_connection_closed : public http_error {
    typedef http_error ancestor ;
public:
    explicit http_connection_closed(const std::string &msg) :
        ancestor(msg)
    {}
} ;

/******************************************************************************/
/** The base class for HTTP errors due to invalid request or response, such as
unknown/unsupported HTTP method, invalid header, etc.
*******************************************************************************/
class http_message_error : public http_error {
    typedef http_error ancestor ;
public:
    explicit http_message_error(const std::string &msg) :
        ancestor(msg)
    {}
} ;

/******************************************************************************/
/** Invalid HTTP header in HTTP message (either request or response).
*******************************************************************************/
class invalid_header : public http_message_error {
    typedef http_message_error ancestor ;
public:
    explicit invalid_header(const std::string &msg) :
        ancestor(msg)
    {}
} ;

/******************************************************************************/
/** HTTP request error.
 This is a base class for all exceptions to throw by an HTTP server upon receiving an
 invalid HTTP request.
*******************************************************************************/
class request_error : public http_message_error {
    typedef http_message_error ancestor ;
public:
    explicit request_error(const std::string &msg) :
        ancestor(msg)
    {}
} ;

/******************************************************************************/
/** Illegally formed HTTP request error.
*******************************************************************************/
class invalid_request : public request_error {
    typedef request_error ancestor ;
public:
    explicit invalid_request(const std::string &msg) :
        ancestor(msg)
    {}
} ;

/******************************************************************************/
/** Invalid/unknown HTTP method in an HTTP request.
*******************************************************************************/
class invalid_method : public invalid_request {
    typedef invalid_request ancestor ;
public:
    /// Constructor.
    /// @param method_name Method name that is considered invalide (e.g. "SET").
    explicit invalid_method(const std::string &method_name) :
        ancestor("Unknown HTTP method: " + method_name),
        _method(method_name)
    {}
    const char *method() const { return _method.c_str() ; }
private:
    const std::string &_method ;
} ;

/******************************************************************************/
/** Unsupported version of HTTP protocol in the request line.
 E.g. "GET /foo/bar HTTP/2.0"
*******************************************************************************/
class unsupported_version : public invalid_request {
    typedef invalid_request ancestor ;
public:
    unsupported_version(unsigned minor, unsigned major) :
        ancestor(strprintf("HTTP version %u.%u is not supported", minor, major)),
        _version(minor, major)
    {}
    const std::pair<unsigned, unsigned> &version() const { return _version ; }
private:
    std::pair<unsigned, unsigned> _version ;
} ;

/******************************************************************************/
/** HTTP response error.
 This is a base class for all exceptions to throw by an HTTP @em client upon receiving
 an invalid HTTP response from a server.
*******************************************************************************/
class response_error : public http_message_error {
public:
    typedef http_message_error ancestor ;

    explicit response_error(const std::string &msg) :
        ancestor(msg)
    {}
} ;

/******************************************************************************/
/** HTTP redirection error.
 An HTTP client throws this exception upon receivng a response with a redirection
 response code (3XX), which happens to be ill-formed: has no location header, has
 bad redirection URI, etc.
*******************************************************************************/
class redirection_error : public response_error {
    typedef response_error ancestor ;
public:
    explicit redirection_error(const std::string &msg) :
        ancestor(msg)
    {}
} ;

} // end of namespace pcomn::http
} // end of namespace pcomn

#endif /* __HTTP_EXCEPTIONS_H */
