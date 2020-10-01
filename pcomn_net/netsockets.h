/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
#ifndef __NET_SOCKETS_H
#define __NET_SOCKETS_H
/*******************************************************************************
 FILE         :   netsockets.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Socket classes.

 CREATION DATE:   27 Jan 2008
*******************************************************************************/
/** @file
    Classes and functions for working with network addresses.
*******************************************************************************/
#include "netexcept.h"

#include <pcomn_netaddr.h>
#include <pcomn_handle.h>
#include <pcomn_safeptr.h>
#include <pcomn_smartptr.h>
#include <pcomn_string.h>

#include <iostream>

#ifdef PCOMN_PL_POSIX
/*******************************************************************************
 Unix
*******************************************************************************/

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <poll.h>

#else
/*******************************************************************************
 Winsock
*******************************************************************************/

#include <winsock2.h>
#include <ws2tcpip.h>

#endif

namespace pcomn {
namespace net {

class basic_socket ;
class stream_socket ;
class server_socket ;
class udp_socket ;

const unsigned DEFAULT_BACKLOG = 10 ;

enum ErrFlags {
    ALLOW_EAGAIN    = 0x0001, /**< Don't throw exception on EAGAIN, return -1 instead */
    ALLOW_EINTR     = 0x0002  /**< Don't repeat current call on EINTR, return -1 instead */
} ;

inline bool is_socket(int fd)
{
    return fd >= 0 && isfdtype(fd, S_IFSOCK) == 1 ;
}

/******************************************************************************/
/** Socket wrapper.
*******************************************************************************/
class basic_socket : private fd_safehandle {
    typedef fd_safehandle ancestor ;
public:
    using ancestor::get ;
    using ancestor::handle ;
    using ancestor::release ;

    virtual ~basic_socket()
    {}

    /// Get full socket location (address + port).
    sock_address sock_addr() const
    {
        sock_address result ;
        if (is_created())
        {
            socklen_t namelen = result.addrsize() ;
            PCOMN_THROW_MSG_IF(getsockname(get(), result.as_sockaddr(), &namelen) == -1,
                               socket_error, "getsockname") ;
            NOXCHECK(namelen == result.addrsize()) ;
        }
        return result ;
    }

    /// Poll the socket.
    /// @return true, if polling is successful; false on timeout.
    /// @throw socket_error, if error occured while polling.
    /// @note This method does @em not throw any exceptions when timeout occurs, but
    /// returns false instead.
    bool poll(unsigned events, int timeout = 0) ;

    bool is_created() const { return good() ; }

    /// Close the underlying socket and release the object.
    /// Invariant of this method: after the call, is_created() is always false.
    /// @note This operation is atomic and thread-safe.
    /// @note This method doesn't throw exceptions because it can be called  from a destructor.
    bool close(bool crash = false) throw() ;

    void bind(const sock_address &addr)
    {
        PCOMN_THROW_MSG_IF(::bind(check_handle(), addr.as_sockaddr(), addr.addrsize()), socket_error, "bind") ;
    }

    bool shutdown(int which_end = SHUT_RDWR) throw()
    {
        return is_created() && ::shutdown(handle(), which_end) == 0 ;
    }

    template<typename T>
    bool getopt(int level, int optname, T &value) const
    {
        socklen_t optlen = sizeof value ;
        return !getsockopt(handle(), level, optname, (char *)&value, &optlen) ;
    }

    template<typename T>
    T safe_getopt(int level, int optname)
    {
        T result ;
        PCOMN_THROW_MSG_IF(!getopt(level, optname, result), socket_error, "getsockopt") ;
        return result ;
    }

    template<typename T>
    bool setopt(int level, int optname, const T &value)
    {
        return !setsockopt(handle(), level, optname, (const char *)&value, sizeof value) ;
    }

    template<typename T>
    basic_socket &safe_setopt(int level, int optname, const T &value)
    {
        PCOMN_THROW_MSG_IF(!setopt(level, optname, value), socket_error, "setsockopt") ;
        return *this ;
    }

    std::pair<unsigned, unsigned> buffers() const
    {
        std::pair<unsigned, unsigned> result (0, 0) ;
        if (is_created())
        {
            getopt(SOL_SOCKET, SO_RCVBUF, result.first) ;
            getopt(SOL_SOCKET, SO_SNDBUF, result.second) ;
        }
        return result ;
    }

    std::pair<unsigned, unsigned> set_buffers(int receive_buffer, int transmit_buffer)
    {
        if (receive_buffer >= 0)
            setopt(SOL_SOCKET, SO_RCVBUF, receive_buffer) ;
        if (transmit_buffer >= 0)
            setopt(SOL_SOCKET, SO_SNDBUF, transmit_buffer) ;
        return buffers() ;
    }

    bool is_bound() const
    {
        sockaddr addr ;
        socklen_t addrlen = sizeof addr ;
        return
            is_created() && !getsockname(get(), &addr, &addrlen) ;
    }

    /// Get socket type (e.g. SOCK_STREAM).
    int type() const
    {
        int result = 0 ;
        if (is_created())
            getopt(SOL_SOCKET, SO_TYPE, result) ;
        return result ;
    }

    int create()
    {
        PCOMN_THROW_MSG_IF(is_created(), std::logic_error, "The socket is already created.") ;
        const int s = create_socket() ;
        NOXCHECK(is_socket(s)) ;
        reset(s) ;
        return s ;
    }

    void swap(basic_socket &other) { ancestor::swap(other) ; }

    static int create(int domain, int type)
    {
        const int s = ::socket(domain, type, 0) ;
        PCOMN_THROW_MSG_IF(s == -1, socket_error, "Cannot create a socket.") ;
        return s ;
    }

protected:
    explicit basic_socket(int fd) :
        ancestor(fd)
    {
        PCOMN_THROW_MSG_IF(fd >= 0 && !is_socket(fd), std::invalid_argument,
                           "The handle '%d' is either invalid or not a socket.", fd) ;
    }

    int check_handle() const
    {
        int result = handle() ;
        PCOMN_THROW_MSG_IF(result == traits_type::invalid_handle(), socket_error,
                           "The socket is either not yet created or already closed.") ;
        return result ;
    }

    int ensure_handle() { return is_created() ? handle() : create() ; }

    virtual int create_socket() = 0 ;

    static void init_network() ;
} ;

/******************************************************************************/
/** Server socket, i.e. a socket listening ot a port and accepting connections.

 Note that there is no 'accept' method; to accept a connection, one should construct a
 stream_socket object using a server_socket as stream_socket constructor parameter.

 @code
 server_socket server (57777, 64) ;
 // ... any code

 stream_socket *accepted = new stream_socket(server) ; // accept a connection
 @endcode
*******************************************************************************/
class server_socket : public basic_socket {
    typedef basic_socket ancestor ;
    friend class stream_socket ;
public:
    /// Create a bound socket for accepting connections.
    /// @note The resulting socket is bound but not yet listening.
    explicit server_socket(const sock_address &addr, bool reuse_addr = true) :
        ancestor(create_socket())
    {
        if (reuse_addr)
            safe_setopt(SOL_SOCKET, SO_REUSEADDR, 1) ;
        bind(addr) ;
    }

    server_socket &listen(int backlog = 5)
    {
        init_network() ;
        PCOMN_THROW_MSG_IF(::listen(check_handle(), backlog) == -1, socket_error, "listen") ;
        return *this ;
    }

    /// Accept a connection.
    ///
    /// If the server socket is in blocking mode and there is no ALLOW_EINTR set in
    /// @a errflags, never returns NULL. I.e., for a blocking server socket this method
    /// either returns a valid connected stream socket or throws an exception.
    /// If the socket is in _non_-blocking mode, returns NULL if there is no connections
    /// to accept (i.e. ::accept() returned EAGAIN/EWOULDBLOCK); otherwise behaves the
    /// same as in the blocking mode.
    ///
    stream_socket *accept(sock_address *accepted_addr = NULL, unsigned errflags = 0) ;

protected:
    int create_socket() { return create(PF_INET, SOCK_STREAM) ; }
private:
    // errflags is an OR-combination of ErrFlags. If 0, accept_connection()
    // always returns valid socket descriptor (or throws exception).
    // If ALLOW_EAGAIN if set, returns -1 if ::accept() returned EAGAIN (this can happen
    // only for a non-blocking socket).
    // If ALLOW_EINTR if set, returns -1 if ::accept() returned EINTR due to a signal.
    int accept_connection(sock_address *addr, unsigned errflags) ;
} ;

/*******************************************************************************/
/** Data socket, i.e. a socket through which data can flow; base class for both stream and
    datagram sockets.

    There are two kinds of sockets: those that are for transmitting/receiving data and those
    that are for accepting connections. The former are data sockets, the latter are server
    sockets.
*******************************************************************************/
class data_socket : public PRefCount, public basic_socket {
    typedef basic_socket ancestor ;
public:
    void connect(const sock_address &peer_addr, int timeout = -1) ;

    /// Get full peer socket location (address + port).
    sock_address peer_addr(bool throw_on_error = true) const
    {
        sock_address result ;
        if (!is_created())
            return result ;
        socklen_t socklen = result.addrsize() ;
        if (getpeername(handle(), result.as_sockaddr(), &socklen) == -1)
            PCOMN_THROW_MSG_IF(throw_on_error, socket_error, "getpeername") ;
        return result ;
    }

    /// Test whether it's possible to read from the socket immediately
    /// @returns true if at least 1 byte can be read immediately; false if socket would block
    bool ready_to_receive(int timeout = 0) { return poll(POLLIN, timeout) ; }

    /// Test whether it's possible to write into the socket immediately
    bool ready_to_transmit(int timeout = 0) { return poll(POLLOUT, timeout) ; }

    basic_socket &bind(const sock_address &) ;

    bool is_connected() const
    {
        sockaddr addr ;
        socklen_t addrlen = sizeof addr ;
        return
            is_created() && !getpeername(get(), &addr, &addrlen) ;
    }

protected:
    explicit data_socket(int fd) :
        ancestor(fd)
    {}
} ;

/******************************************************************************/
/** Connection-oriented socket wrapper.
*******************************************************************************/
class stream_socket : public data_socket {
    typedef data_socket ancestor ;
public:
    /// Create stream socket object from OS socket descriptor.
    explicit stream_socket(int sockd = -1) :
        ancestor(sockd)
    {}

    /// Accept connection from a server socket.
    explicit stream_socket(server_socket &server, sock_address *accepted_addr = NULL) :
        ancestor(server.accept_connection(accepted_addr, 0))
    {}

    /// Receive data from a socket.
    /// @param buffer   A pointer to a buffer to place received data into.
    /// @param size     Size of a buffer.
    /// @param timeout  Upper limit on the time for which receive will block, in
    /// @b milliseconds; negative value means an infinite timeout.
    /// @param flags    RECV(2) flags, like MSG_PEEK, MSG_DONTWAIT, etc.
    size_t receive(void *buffer, size_t size, int timeout = -1, unsigned flags = 0)
    {
        PCOMN_ENSURE_ARG(buffer) ;
        PCOMN_THROW_MSG_IF(timeout >= 0 && !ready_to_receive(timeout), operation_timeout, "recv") ;
        return
            ensure_receive(::recv(handle(), buffer, size, flags), "recv") ;
    }

    size_t receive(iovec *begin, iovec *end, int timeout = -1)
    {
        if (begin == end)
            return 0 ;
        ensure_iovec(begin, end) ;
        PCOMN_THROW_MSG_IF(timeout >= 0 && !ready_to_receive(timeout), operation_timeout, "readv") ;
        return
            ensure_receive(::readv(handle(), begin, end - begin), "readv") ;
    }

    template<unsigned n>
    size_t receive(iovec (&vec)[n], int timeout = -1)
    {
        return receive(vec + 0, vec + n, timeout) ;
    }

    size_t transmit(const void *buffer, size_t size, int timeout = -1)
    {
        PCOMN_ENSURE_ARG(buffer) ;
        PCOMN_THROW_MSG_IF(timeout >= 0 && !ready_to_transmit(timeout), operation_timeout, "send") ;
        // Note the returned value can be less than size if the client cancelled receiving.
        return
            ensure_transmit(::send(handle(), buffer, size, 0), "send") ;
    }

    size_t transmit(const iovec *begin, const iovec *end, int timeout = -1)
    {
        if (begin == end)
            return 0 ;
        ensure_iovec(begin, end) ;
        PCOMN_THROW_MSG_IF(timeout >= 0 && !ready_to_transmit(timeout), operation_timeout, "writev") ;
        return
            ensure_transmit(::writev(handle(), begin, end - begin), "writev") ;
    }

    template<unsigned n>
    size_t transmit(const iovec (&vec)[n], int timeout = -1)
    {
        return transmit(vec + 0, vec + n, timeout) ;
    }

    template<typename S>
    enable_if_strchar_t<S, char, size_t> transmit(const S &buffer, int timeout = -1)
    {
        return transmit(str::cstr(buffer), str::len(buffer), timeout) ;
    }

    size_t transmit_file(int fd, size_t size, int64_t offset = -1, int timeout = -1)
    {
        PCOMN_THROW_MSG_IF(timeout >= 0 && !ready_to_transmit(timeout), operation_timeout, "sendfile") ;
        return
            ensure_transmit(::sendfile64(handle(), fd, offset < 0 ? NULL : &offset, size), "sendfile64") ;
    }

protected:
    int create_socket() { return create(PF_INET, SOCK_STREAM) ; }

private:
    // Check send/write/sendfile result and, if error occurred, throw corresponding
    // exception; particularly, throw receiver_closed on ECONNRESET or EPIPE.
    static size_t ensure_transmit(ssize_t result, const char *fname)
    {
        if (result < 0)
            throw_transmit_error(fname) ;
        return result ;
    }

    static size_t ensure_receive(ssize_t result, const char *fname)
    {
        if (result < 0)
            throw_receive_error(fname) ;
        return result ;
    }

    static void throw_transmit_error(const char *fname) ;
    static void throw_receive_error(const char *fname) ;

    static void ensure_iovec(const iovec *begin, const iovec *end)
    {
        PCOMN_ENSURE_ARG(begin) ;
        PCOMN_ENSURE_ARG(end) ;
    }
} ;

/******************************************************************************/
/** Client stream socket.
*******************************************************************************/
class client_socket : public stream_socket {
    typedef stream_socket ancestor ;
public:
    explicit client_socket(int sockd = -1) :
        ancestor(sockd)
    {}

    /// Create a client socket that connects to the specified server.
    /// @param peer_addr The endpoint address to connect to.
    /// @param timeout   Connection timeout in microseconds.
    client_socket(const sock_address &peer_addr, int timeout = -1)
    {
        connect(peer_addr, timeout) ;
    }
} ;

/******************************************************************************/
/** Connectionless datagram socket.
*******************************************************************************/
class udp_socket : public data_socket {
    typedef data_socket ancestor ;
public:
    explicit udp_socket(int sockd = -1) ;

    /// Create a datagram socket on particular port and addrress of a given network
    /// interface.
    explicit udp_socket(const sock_address &addr, bool unicast = false) ;

    /// Receive a datagram.
    /// @return Datagram data and address of the peer that the datagram received from.
    /// If a timeout occurs, both the returned string and location are empty.
    ///
    /// @param timeout Timeout in milliseconds. -1 means to block until a message is
    /// received.
    std::pair<std::string, sock_address> read(int timeout=-1) ;

    /// Send a packet to a specified peer.
    /// @return Size sent. Value 0 means that there is not enough memory/buffers to send
    /// data; client code can attempt to resend.
    size_t send_message(const void *buffer, size_t size, const sock_address &peer_addr) ;
    std::pair<size_t, sock_address> recv_message(void *buffer, size_t size, int timeout = -1, unsigned flags = 0) ;
} ;

/*******************************************************************************
 server_socket
*******************************************************************************/
inline stream_socket *server_socket::accept(sock_address *accepted_addr, unsigned errflags)
{
    const int sockfd = accept_connection(accepted_addr, errflags) ;
    return sockfd == -1 ? NULL : new stream_socket(sockfd) ;
}

/*******************************************************************************
 Intrusive shared pointers to data sockets.
*******************************************************************************/
/// Intrusive shared pointer to a data_socket.
typedef shared_intrusive_ptr<data_socket>    data_socket_ptr ;
/// Intrusive shared pointer to a stream_socket.
typedef shared_intrusive_ptr<stream_socket>  stream_socket_ptr ;
/// Intrusive shared pointer to a client_socket.
typedef shared_intrusive_ptr<stream_socket>  client_socket_ptr ;
/// Intrusive shared pointer to a udp_socket.
typedef shared_intrusive_ptr<udp_socket>     udp_socket_ptr ;

/*******************************************************************************
 swap overloads for sockets
*******************************************************************************/
PCOMN_DEFINE_SWAP(basic_socket) ;
PCOMN_DEFINE_SWAP(server_socket) ;
PCOMN_DEFINE_SWAP(data_socket) ;
PCOMN_DEFINE_SWAP(stream_socket) ;
PCOMN_DEFINE_SWAP(client_socket) ;
PCOMN_DEFINE_SWAP(udp_socket) ;

/*******************************************************************************
 Ostream output.
*******************************************************************************/
inline std::ostream& operator<<(std::ostream &os, const basic_socket &sock)
{
    os << "handle:" << sock.handle()
       << " address:" ;
    if (sock.is_bound())
        return os << sock.sock_addr() ;
    return os << "unbound" ;
}

inline std::ostream& operator<<(std::ostream &os, const data_socket &sock)
{
    os << *static_cast<const basic_socket *>(&sock) << " peer:" ;
    if (sock.is_connected())
        return os << sock.peer_addr(false) ;
    return os << "disconnected" ;
}

} // end of namespace pcomn::net
} // end of namespace pcomn

#endif /* __NET_SOCKETS_H */
