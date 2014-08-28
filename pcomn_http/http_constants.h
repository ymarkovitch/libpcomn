/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
#ifndef __HTTP_CONSTANTS_H
#define __HTTP_CONSTANTS_H
/*******************************************************************************
 FILE         :   http_constants.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   HTTP constants: response codes, flags, etc.

 CREATION DATE:   8 Mar 2008
*******************************************************************************/
#include <stddef.h>

namespace pcomn {
namespace http {

/*******************************************************************************
 HTTP response codes
*******************************************************************************/
const unsigned HTTP_RSP_CODE_MIN = 100 ;
const unsigned HTTP_RSP_CODE_MAX = 599 ;

const unsigned HTTP_RSP_CONTINUE = 100 ;
const unsigned HTTP_RSP_SWITCHING_PROTOCOLS = 101 ;
const unsigned HTTP_RSP_PROCESSING = 102 ;
const unsigned HTTP_RSP_OK = 200 ;
const unsigned HTTP_RSP_CREATED = 201 ;
const unsigned HTTP_RSP_ACCEPTED = 202 ;
const unsigned HTTP_RSP_NONAUTHORITATIVE_INFORMATION = 203 ;
const unsigned HTTP_RSP_NO_CONTENT = 204 ;
const unsigned HTTP_RSP_RESET_CONTENT = 205 ;
const unsigned HTTP_RSP_PARTIAL_CONTENT = 206 ;
const unsigned HTTP_RSP_MULTISTATUS = 207 ;

const unsigned HTTP_RSP_MULTIPLE_CHOICES = 300 ;
const unsigned HTTP_RSP_MOVED_PERMANENTLY = 301 ;
const unsigned HTTP_RSP_FOUND = 302 ;
const unsigned HTTP_RSP_SEE_OTHER = 303 ;
const unsigned HTTP_RSP_NOT_MODIFIED = 304 ;
const unsigned HTTP_RSP_USE_PROXY = 305 ;
const unsigned HTTP_RSP_TEMPORARY_REDIRECT = 307 ;

const unsigned HTTP_RSP_BAD_REQUEST = 400 ;
const unsigned HTTP_RSP_AUTHORIZATION_REQUIRED = 401 ;
const unsigned HTTP_RSP_PAYMENT_REQUIRED = 402 ;
const unsigned HTTP_RSP_FORBIDDEN = 403 ;
const unsigned HTTP_RSP_NOT_FOUND = 404 ;
const unsigned HTTP_RSP_METHOD_NOT_ALLOWED = 405 ;
const unsigned HTTP_RSP_NOT_ACCEPTABLE = 406 ;
const unsigned HTTP_RSP_PROXY_AUTHENTICATION_REQUIRED = 407 ;
const unsigned HTTP_RSP_REQUEST_TIMEOUT = 408 ;
const unsigned HTTP_RSP_CONFLICT = 409 ;
const unsigned HTTP_RSP_GONE = 410 ;
const unsigned HTTP_RSP_LENGTH_REQUIRED = 411 ;
const unsigned HTTP_RSP_PRECONDITION_FAILED = 412 ;
const unsigned HTTP_RSP_REQUEST_ENTITY_TOO_LARGE = 413 ;
const unsigned HTTP_RSP_REQUEST_URI_TOO_LARGE = 414 ;
const unsigned HTTP_RSP_UNSUPPORTED_MEDIA_TYPE = 415 ;
const unsigned HTTP_RSP_REQUESTED_RANGE_NOT_SATISFIABLE = 416 ;
const unsigned HTTP_RSP_EXPECTATION_FAILED = 417 ;
const unsigned HTTP_RSP_UNPROCESSABLE_ENTITY = 422 ;
const unsigned HTTP_RSP_LOCKED = 423 ;
const unsigned HTTP_RSP_FAILED_DEPENDENCY = 424 ;

const unsigned HTTP_RSP_INTERNAL_SERVER_ERROR = 500 ;
const unsigned HTTP_RSP_METHOD_NOT_IMPLEMENTED = 501 ;
const unsigned HTTP_RSP_BAD_GATEWAY = 502 ;
const unsigned HTTP_RSP_SERVICE_TEMPORARILY_UNAVAILABLE = 503 ;
const unsigned HTTP_RSP_GATEWAY_TIMEOUT = 504 ;
const unsigned HTTP_RSP_HTTP_VERSION_NOT_SUPPORTED = 505 ;
const unsigned HTTP_RSP_VARIANT_ALSO_NEGOTIATES = 506 ;
const unsigned HTTP_RSP_INSUFFICIENT_STORAGE = 507 ;
const unsigned HTTP_RSP_NOT_EXTENDED = 510 ;

/*******************************************************************************
 Response flags
*******************************************************************************/
const unsigned HTTP_RSPFLAG_CLOSE = 0x10000 ;
const unsigned HTTP_RSPFLAG_FLAGS = 0xffff0000 ;

const unsigned HTTP_PORT = 80 ;
const unsigned HTTP_PROXY_PORT = 85 ;

const size_t HTTP_CHUNKED_CONTENT = (size_t)-1 ;
const size_t HTTP_UNBOUND_CONTENT = (size_t)-2 ;

const size_t HTTP_IGNORE_CONTENT = (size_t)-2 ;

/// The maximum number of redirections before the client to fail.
const unsigned HTTP_REDIRECT_MAX_TRIES = 5 ;

/*******************************************************************************
 HTTP response code categorizing functions.
*******************************************************************************/
inline bool response_is_continue(unsigned code) { return code >= 100 && code < 200 ; }
inline bool response_is_success(unsigned code) { return code >= 200 && code < 300 ; }
inline bool response_is_redirect(unsigned code) { return code >= 300 && code < 400 ; }
inline bool response_is_client_err(unsigned code) { return code >= 400 && code < 500 ; }
inline bool response_is_server_err(unsigned code) { return code >= 500 && code < 600 ; }

} // end of namespace pcomn::http
} // end of namespace pcomn

#endif /* __HTTP_CONSTANTS_H */
