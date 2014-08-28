/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)) -*-*/
/*******************************************************************************
 FILE         :   test_epollpipe03.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   13 Jun 2008
*******************************************************************************/
#include <pcomn_thread.h>
#include <pcomn_handle.h>
#include <unix/pcomn_fdevents.h>
#include <functional>

#include <sys/epoll.h>
#include <unistd.h>

using namespace pcomn ;

int read_pipe(int epoll_fd, int timeout)
{
   try {
      epoll_event events[1] ;
      do
      {
         std::cout << pthread_self() << ": Waiting on epoll " << epoll_fd << std::endl ;
         if (!epoll_waitx(epoll_fd, events, timeout))
         {
            std::cout << pthread_self() << ": Timeout " << std::endl ;
            events[0].events = 0 ;
            continue ;
         }

         std::cout << pthread_self() << ": Reading pipe " << events[0].data.fd << " events=" << HEXOUT(events[0].events) << std::endl ;
         if (events[0].events & EPOLLIN)
         {
            char buf[1024] ;
            int count = read(events[0].data.fd, buf, 3) ;
            PCOMN_ENSURE_POSIX(count, "read") ;
            buf[count] = 0 ;
            std::cout << pthread_self() << ": " << count << " bytes: '" << buf << "'" << std::endl ;
         }
      }
      while (!events[0].events || (events[0].events & EPOLLIN)) ;
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
   fd_safehandle read_fd (pipefd[0]) ;

   fd_safehandle epoll_fd (epoll_create(5)) ;
   epoll_add(epoll_fd, read_fd, EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP) ;

   TaskThread pipe_reader1 (make_job(std::bind(std::ptr_fun(&read_pipe), epoll_fd.handle(), 1000))) ;
   TaskThread pipe_reader2 (make_job(std::bind(std::ptr_fun(&read_pipe), epoll_fd.handle(), 1000))) ;
   pipe_reader1.start() ;
   pipe_reader2.start() ;
   PCOMN_ENSURE_POSIX(write(write_fd, "Foo", 3), "write") ;
   sleep(2) ;
   PCOMN_ENSURE_POSIX(write(write_fd, "Bar", 3), "write") ;
   for (int i = 1 ; i < 100 ; ++i)
   {
      char buf[4] ;
      snprintf(buf, sizeof buf, "B%2.2d", i) ;
      PCOMN_ENSURE_POSIX(write(write_fd, buf, 3), "write") ;
   }
   write_fd.close() ;
   pipe_reader2.join() ;
   pipe_reader1.join() ;
   return 0 ;
}
