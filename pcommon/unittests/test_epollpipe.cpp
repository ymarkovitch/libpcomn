/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)) -*-*/
/*******************************************************************************
 FILE         :   test_epollpipe.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   13 Jun 2008
*******************************************************************************/
#include <pcomn_thread.h>
#include <pcomn_handle.h>
#include <functional>

#include <sys/epoll.h>
#include <unistd.h>

using namespace pcomn ;

void epoll_add(int epoll_fd, int fd, uint32_t events)
{
   struct epoll_event ev ;

   ev.events = events ;
   ev.data.fd = fd ;
   PCOMN_ENSURE_POSIX(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev), "epoll_ctl") ;
}

int read_pipe(int pipefd)
{
   try {
      fd_safehandle epoll_fd (epoll_create(5)) ;
      epoll_add(epoll_fd, pipefd, EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP) ;

      epoll_event events[1] ;
      do
      {
         std::cout << "Waiting on pipe " << pipefd << std::endl ;
         int nfds = epoll_wait(epoll_fd, events, 1, -1) ;
         PCOMN_ENSURE_POSIX(nfds, "epoll_wait") ;
         std::cout << "Reading pipe " << events[0].data.fd << " events=" << HEXOUT(events[0].events) << std::endl ;
         if (events[0].events & EPOLLIN)
         {
            char buf[1024] ;
            int count = read(events[0].data.fd, buf, 3) ;
            PCOMN_ENSURE_POSIX(count, "read") ;
            buf[count] = 0 ;
            std::cout << count << " bytes: '" << buf << "'" << std::endl ;
         }
      }
      while (!(events[0].events & (EPOLLHUP | EPOLLRDHUP))) ;
      return 1 ;
   }
   catch (const std::exception &x) {
      std::cout << STDEXCEPTOUT(x) << std::endl ;
   }
   return 0 ;
}

int main(int, char *[])
{
   DIAG_INITTRACE("pcomntest.ini") ;

   int pipefd[2] ;

   PCOMN_ENSURE_POSIX(pipe(pipefd), "pipe") ;

   fd_safehandle write_fd (pipefd[1]) ;

   TaskThread pipe_reader (make_job(std::bind(std::ptr_fun(&read_pipe), pipefd[0]))) ;
   pipe_reader.start() ;
   PCOMN_ENSURE_POSIX(write(write_fd, "Foo", 3), "write") ;
   PCOMN_ENSURE_POSIX(write(write_fd, "Bar", 3), "write") ;
   write_fd.close() ;
   pipe_reader.join() ;
   return 0 ;
}
