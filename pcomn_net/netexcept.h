/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
#ifndef __NETEXCEPT_H
#define __NETEXCEPT_H
/*******************************************************************************
 FILE         :   netexcept.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Network exception classes.

 CREATION DATE:   27 Jan 2008
*******************************************************************************/
/** @file
  Network exception classes.
*******************************************************************************/
#include <pcomn_except.h>

namespace pcomn {
namespace net {

#define NET_DEFINE_EXCEPTION(name, base)                                \
class name : public base {                                              \
    typedef base ancestor ;                                             \
public:                                                                 \
    name() {}                                                           \
    explicit name(const std::string &message) : ancestor(message) {}    \
    name(const std::string &message, int errcode) : ancestor(message, errcode) {} \
}

/******************************************************************************/
/** Exception class: general network error.
*******************************************************************************/
NET_DEFINE_EXCEPTION(network_error, system_error) ;

/******************************************************************************/
/** Exception class: socket error.
*******************************************************************************/
NET_DEFINE_EXCEPTION(socket_error, network_error) ;

/******************************************************************************/
/** Exception class connection_error indicates an error while connect()ing to
 a server; this is a client exception.
*******************************************************************************/
NET_DEFINE_EXCEPTION(connection_error, socket_error) ;

/******************************************************************************/
/** Exception class: network operation timeout.

 @note Both the base classes, timeout_error (directly) and network_error
 (indirectly), are based upon environment_error; as long as both use virtual
 inheritance, it is OK.
*******************************************************************************/
class operation_timeout : public network_error, public timeout_error {
    typedef network_error ancestor ;
public:
    operation_timeout() : ancestor(std::string(), ETIMEDOUT) {}

    explicit operation_timeout(const std::string &message) :
        ancestor(message, ETIMEDOUT),
        timeout_error(message)
    {}
} ;

/******************************************************************************/
/** Base class for receive/read exceptions.
*******************************************************************************/
NET_DEFINE_EXCEPTION(receive_error, socket_error) ;

/******************************************************************************/
/** Indicates the peer has closed the sending end of the connection or has done
 shutdown of writing: ECONNRESET.
*******************************************************************************/
NET_DEFINE_EXCEPTION(sender_closed, receive_error) ;

/******************************************************************************/
/** Base class for send/transmit exceptions.
*******************************************************************************/
NET_DEFINE_EXCEPTION(transmit_error, socket_error) ;

/******************************************************************************/
/** Indicates the peer has closed the receiving end of the connection or has done
 shutdown of reading: either ECONNRESET or EPIPE.
*******************************************************************************/
NET_DEFINE_EXCEPTION(receiver_closed, transmit_error) ;

} // end of namespace pcomn::net
} // end of namespace pcomn

#endif /* __NETEXCEPT_H */
