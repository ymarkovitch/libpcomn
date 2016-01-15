/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
/*******************************************************************************
 FILE         :   sockets.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Socket classes.

 CREATION DATE:   27 Jan 2008
*******************************************************************************/
#include <pcomn_net/netsockets.h>
#include <pcomn_net/netexcept.h>
#include <pcomn_atomic.h>

#include <signal.h>

namespace pcomn {
namespace net {

/*******************************************************************************
 basic_socket
*******************************************************************************/
bool basic_socket::poll(unsigned events, int timeout)
{
    pollfd pfd ;
    pfd.fd = check_handle() ;
    pfd.events = events ;
    pfd.revents = 0 ;
    const int result = ::poll(&pfd, 1, timeout) ;
    PCOMN_THROW_MSG_IF(result == -1, socket_error, "poll") ;
    return result ;
}

bool basic_socket::close(bool crash) throw()
{
   // This operation must be atomic; this allows us to achieve safety when
   // different threads are trying to close one socket simultaneously (this can be
   // the case, for example, during server shutdown)
   int sockd = release() ;

   if (sockd == traits_type::invalid_handle())
       // Already closed or hasn't been open
       return true ;

   // Request for crash close means we have to set l_onoff to 1 to enable lingering, but
   // set l_linger to 0 to timeout cancelling all I/O immediately if not all yet sent.
   if (crash)
   {
      linger opt = { 1, 0 } ;
      setsockopt(sockd, SOL_SOCKET, SO_LINGER, (char *)&opt, sizeof opt) ;
   }
   return traits_type::close(sockd) ;
}

void basic_socket::init_network()
{
    static atomic_t netinit_lock ;
    if (atomic_op::xchg(&netinit_lock, (atomic_t)1))
        return ;

    struct sigaction ignore ;
    fill_mem(ignore) ;
    ignore.sa_handler = SIG_IGN ;
    // Prevent socket applications from being killed due to a peer closed a socket!
    sigaction(SIGPIPE, &ignore, NULL) ;
    sigaction(SIGHUP, &ignore, NULL) ;
}

/*******************************************************************************
 data_socket
*******************************************************************************/
void data_socket::connect(const sock_address &peer_addr, int timeout)
{
    init_network() ;

    const int sockd = ensure_handle() ;
	const int sockflags = fcntl(sockd, F_GETFL) ;
    PCOMN_THROW_MSG_IF(sockflags == -1, socket_error, "fcntl") ;
    const bool temporary_nonblock = timeout >= 0 && !(sockflags & O_NONBLOCK) ;

    if (temporary_nonblock)
        PCOMN_THROW_MSG_IF(fcntl(sockd, F_SETFL, sockflags | O_NONBLOCK) == -1, socket_error, "fcntl")  ;

	// Connecting...
	int status = ::connect(sockd, peer_addr.as_sockaddr(), peer_addr.addrsize()) ;
	if (status)
		status = errno ;
    if (status == EINPROGRESS)
    {
        struct pollfd pfd ;
        pfd.fd = sockd ;
        pfd.revents = 0 ;
        pfd.events = POLLIN | POLLOUT ;
        switch (::poll(&pfd, 1, timeout))
        {
            case 0:
                status = ETIMEDOUT ;
                break ;
            case 1:
                status = (pfd.revents & POLLERR) ? ECONNREFUSED : 0 ;
                break ;
            default:
                status = errno ;
        }
    }
    if (temporary_nonblock)
        // Restore socket flags
        fcntl(sockd, F_SETFL, sockflags) ;

	if (status)
	{
        // Restore errno
        errno = status ;
        PCOMN_THROW_IF(status != ETIMEDOUT, connection_error, "Error connecting to %s",
                       peer_addr.str().c_str()) ;
        PCOMN_THROWF(operation_timeout, "Connection attempt to %s timed out in %d ms.",
                     peer_addr.str().c_str(), timeout) ;
	}
}

/*******************************************************************************
 stream_socket
*******************************************************************************/
void stream_socket::throw_transmit_error(const char *fname)
{
    switch (errno)
    {
        case ECONNRESET:
        case EPIPE:
            PCOMN_THROW_MSGF(receiver_closed,
                             "'%s' failed, the peer has closed the receiving end of the connection",
                             fname) ;

        default:
            PCOMN_THROW_MSGF(transmit_error, "'%s' failed", fname) ;
    }
}

void stream_socket::throw_receive_error(const char *fname)
{
    switch (errno)
    {
        case ECONNRESET:
            PCOMN_THROW_MSGF(sender_closed,
                             "'%s' failed, the peer has closed the sending end of the connection",
                             fname) ;

        default:
            PCOMN_THROW_MSGF(receive_error, "'%s' failed", fname) ;
    }
}

/*******************************************************************************
 server_socket
*******************************************************************************/
int server_socket::accept_connection(sock_address *addr, unsigned errflags)
{
    struct sockaddr *sa ;
    socklen_t addrlen ;
    if (addr)
    {
        sa = addr->as_sockaddr() ;
        addrlen = addr->addrsize() ;
    }
    else
    {
        sa = NULL ;
        addrlen = 0 ;
    }
    for (int sockd ;;)
    {
        sockd = ::accept(handle(), sa, &addrlen) ;
        if (sockd >= 0)
            return sockd ;

        const int err = errno ;
        if (err == EINTR)
            if (errflags & ALLOW_EINTR) break ;
            else continue ;

        else if (err == EAGAIN || err == EWOULDBLOCK)
            if (errflags & ALLOW_EAGAIN) break ;

        PCOMN_THROW_MSGF(socket_error, "accept") ;
    }
    return -1 ;
}

} // end of namespace pcomn::net
} // end of namespace pcomn
