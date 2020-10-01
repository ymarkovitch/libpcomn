/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
#ifndef __NETEXCEPT_H
#define __NETEXCEPT_H
/*******************************************************************************
 FILE         :   netexcept.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2020. All rights reserved.
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
    explicit name(const std::string &message) : ancestor(message.c_str()) {} \
    name(const std::string &message, int errcode) : ancestor(message, errcode) {} \
}

/***************************************************************************//**
 The base class for network errors.
*******************************************************************************/
class network_error {
public:
    virtual ~network_error() = default ;
    virtual const char *what() const noexcept = 0 ;

    int code() const noexcept { return get_code() ; }

protected:
    virtual int get_code() const noexcept = 0 ;
} ;

/***************************************************************************//**
 Exception class: general network error.
*******************************************************************************/
class network_exception : public virtual network_error, public system_error {
    typedef system_error ancestor ;
public:
    network_exception() = default ;
    explicit network_exception(const std::string &message) : ancestor(message.c_str()) {}
    network_exception(const std::string &message, int errcode) : ancestor(message, errcode) {}

    const char *what() const noexcept override { return ancestor::what() ; }

private:
    int get_code() const noexcept final { return ancestor::code() ; }
} ;

/***************************************************************************//**
 Exception class: network operation timeout.

 @note Both the base classes, timeout_error (directly) and network_error
 (indirectly), are based upon environment_error; as long as both use virtual
 inheritance, it is OK.
*******************************************************************************/
class operation_timeout final : public virtual network_error, public timeout_error {
    typedef timeout_error ancestor ;
public:
    using ancestor::ancestor ;
    const char *what() const noexcept override { return ancestor::what() ; }

private:
    int get_code() const noexcept final { return ancestor::code() ; }
} ;

/******************************************************************************/
/** Exception class: socket error.
*******************************************************************************/
NET_DEFINE_EXCEPTION(socket_error, network_exception) ;

/******************************************************************************/
/** Exception class connection_error indicates an error while connect()ing to
 a server; this is a client exception.
*******************************************************************************/
NET_DEFINE_EXCEPTION(connection_error, socket_error) ;

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
