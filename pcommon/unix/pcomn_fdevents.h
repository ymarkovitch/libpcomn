/*-*- mode: c++; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_FDEVENTS_H
#define __PCOMN_FDEVENTS_H
/*******************************************************************************
 FILE         :   pcomn_fdevents.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Linux file descriptor events (epoll, eventfd, etc.)

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   12 Jun 2008
*******************************************************************************/
#include <pcomn_except.h>

#include <sys/epoll.h>

#ifndef EPOLLRDHUP
#define EPOLLRDHUP (0x2000)
#endif

namespace pcomn {

inline int epoll_control(int epoll_fd, int op, int fd, uint32_t events)
{
   struct epoll_event ev ;

   ev.events = events ;
   ev.data.fd = fd ;
   return epoll_ctl(epoll_fd, op, fd, &ev) ;
}

inline int epoll_add(int epoll_fd, int fd, uint32_t events)
{
   return epoll_control(epoll_fd, EPOLL_CTL_ADD, fd, events) ;
}

inline void epoll_addx(int epoll_fd, int fd, uint32_t events)
{
   PCOMN_ENSURE_POSIX(epoll_add(epoll_fd, fd, events), "epoll_ctl") ;
}

inline int epoll_del(int epoll_fd, int fd)
{
   return epoll_control(epoll_fd, EPOLL_CTL_DEL, fd, 0) ;
}

inline void epoll_delx(int epoll_fd, int fd)
{
   PCOMN_ENSURE_POSIX(epoll_del(epoll_fd, fd), "epoll_ctl") ;
}

inline int epoll_mod(int epoll_fd, int fd, uint32_t events)
{
   return epoll_control(epoll_fd, EPOLL_CTL_MOD, fd, events) ;
}

inline void epoll_modx(int epoll_fd, int fd, uint32_t events)
{
   PCOMN_ENSURE_POSIX(epoll_mod(epoll_fd, fd, events), "epoll_ctl") ;
}

inline unsigned epoll_waitx(int epoll_fd, epoll_event *events, size_t maxevents, int timeout = -1)
{
   int res ;
   while ((res = epoll_wait(epoll_fd, events, maxevents, timeout) < 0) && errno == EINTR) ;
   return
      PCOMN_ENSURE_POSIX(res, "epoll_wait") ;
}

template<size_t maxevents>
inline unsigned epoll_waitx(int epoll_fd, epoll_event (&events)[maxevents], int timeout = -1)
{
   return epoll_waitx(epoll_fd, events, maxevents, timeout) ;
}

} // end of namespace pcomn

#endif /* __PCOMN_FDEVENTS_H */
