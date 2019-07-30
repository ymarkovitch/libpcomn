/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
#ifndef __NET_STREAMS_H
#define __NET_STREAMS_H
/*******************************************************************************
 FILE         :   netstreams.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Socket streams.

 CREATION DATE:   14 Jun 2008
*******************************************************************************/
/** @file
  pcomn::binary_istream and pcomn::binary_ostream wrappers over socket objects.
*******************************************************************************/
#include <pcomn_net/netsockets.h>
#include <pcomn_binstream.h>

namespace pcomn {
namespace net {

/******************************************************************************/
/** pcomn::binary_istream over pcomn::net::stream_socket.
*******************************************************************************/
class socket_istream : public binary_istream {
public:
    explicit socket_istream(const stream_socket_ptr &ssocket) :
        _ssocket(PCOMN_ENSURE_ARG(ssocket)),
        _timeout(-1)
    {}

    /// Get the socket read timeout, in milliseconds.
    /// @note It is -1 (infinite) upon socket_istream construction.
    int timeout() const { return _timeout ; }

    void set_timeout(int timeout) { _timeout = timeout ; }

protected:
    size_t read_data(void *buf, size_t size)
    {
        return ssocket().receive(buf, size, timeout()) ;
    }

    stream_socket &ssocket() { return *_ssocket ; }
private:
    const stream_socket_ptr _ssocket ;
    int                     _timeout ; /* Timeout in milliseconds */
} ;

/******************************************************************************/
/** pcomn::binary_ostream over pcomn::net::stream_socket.
*******************************************************************************/
class socket_ostream : public binary_ostream {
public:
    explicit socket_ostream(const stream_socket_ptr &ssocket) :
        _ssocket(PCOMN_ENSURE_ARG(ssocket)),
        _timeout(-1)
    {}

    /// Get the socket write timeout, in milliseconds.
    /// @note It is -1 (infinite) upon socket_ostream construction.
    int timeout() const { return _timeout ; }

    void set_timeout(int timeout) { _timeout = timeout ; }

protected:
    size_t write_data(const void *buf, size_t size)
    {
        return ssocket().transmit(buf, size, timeout()) ;
    }

    stream_socket &ssocket() { return *_ssocket ; }
private:
    const stream_socket_ptr _ssocket ;
    int                     _timeout ; /* Timeout in milliseconds */
} ;


} // end of namespace pcomn::net
} // end of namespace pcomn

#endif /* __NET_STREAMS_H */
