/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_sys.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   System (platform) functions for UNIX

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   18 Nov 2008
*******************************************************************************/
#include <pcomn_platform.h>
#ifdef PCOMN_PL_UNIX
#include "../pcomn_sys.h"
#else
#error This file is only for UNIX platforms!
#endif

#include <pcomn_except.h>
#include <pcomn_fileutils.h>
#include <pcstring.h>

#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>

namespace pcomn {
namespace sys {

#ifdef PCOMN_PL_LINUX
unsigned cpu_core_count(unsigned *phys_sockets, unsigned *ht_count)
{
   const int cpuinfo = PCOMN_ENSURE_POSIX(open("/proc/cpuinfo", O_RDONLY), "open") ;

   char local_buf[16*1024] ;
   void *dynbuf = nullptr ;

   const ssize_t fsize = readfile(cpuinfo, local_buf, sizeof local_buf - 1, &dynbuf) ;

   const int result = fsize >= 0 ? 0 : errno ;

   close(cpuinfo) ;

   PCOMN_ENSURE_ENOERR(result, "read") ;

   const char * const data = dynbuf ? (const char *)dynbuf : local_buf ;
   const_cast<char &>(data[fsize]) = 0 ;

   unsigned cpu_sockets = 0 ;
   unsigned hyperthreads = 0 ;
   unsigned cores = 0 ;
   unsigned cpu_count = 0 ;

   int last_core = -1 ;
   int last_cpu = -1 ;

   for (const char *line = data, *line_end = (const char *)memchr(line, '\n', fsize), *data_end = data + fsize ;
        line_end ;
        line = line_end + 1, line_end = (const char *)memchr(line, '\n', data_end - line))
   {
      int c ;
      switch (*line)
      {
         case 'p':
            if (sscanf(line, "processor       : %d\n", &c) == 1)
            {
               ++cpu_count ;
            }
            else if (sscanf(line, "physical id     : %d\n", &c) == 1 && c != last_cpu)
            {
               ++cpu_sockets ;
               last_cpu = c ;
               last_core = -1 ;
            }
            break ;

         case 'c':
            if (sscanf(line, "core id         : %d\n", &c) == 1)
            {
               ++hyperthreads ;
               if (c != last_core)
               {
                  ++cores ;
                  last_core = c ;
               }
            }
            break ;
      }
   }

   // Set results
   if (phys_sockets)
      *phys_sockets = std::max(cpu_sockets, 1U) ;

   if (ht_count)
      *ht_count = hyperthreads ? hyperthreads : cpu_count ;

   free(dynbuf) ;

   return cores ? cores : cpu_count ;
}
#endif

int ensure_sys_err(int result, int code){
   if(result == -1) _exit(code);
   return result;
}

#define DAEMONIZE_ERR_BASE 30

void daemonize(const char *dir, const char *stdinfile, const char *stdoutfile, const char *stderrfile)
{
   umask(0) ;

   rlimit rl ;

   // Get the maximal number of possibly open files
   ensure_sys_err(getrlimit(RLIMIT_NOFILE, &rl), DAEMONIZE_ERR_BASE) ;

   // Fork
   int pid = 0;
   if (pid = ensure_sys_err(fork(), DAEMONIZE_ERR_BASE+1))
      // Parent process
      _exit(0) ;

   // Create a new session and become its leader
   ensure_sys_err(setsid(), DAEMONIZE_ERR_BASE+2) ;

   if (dir && *dir)
      ensure_sys_err(chdir(dir), DAEMONIZE_ERR_BASE+3) ;

   // Close all open file descriptors
   for (int fd = rl.rlim_max == RLIM_INFINITY ? 1024 : rl.rlim_max ; fd-- ; close(fd)) ;

   // Open new standard file descriptors
   ensure_sys_err(open(stdinfile, O_RDONLY) == STDIN_FILENO, DAEMONIZE_ERR_BASE+4);
   ensure_sys_err(open(stdoutfile, O_WRONLY|O_CREAT|O_APPEND, 0600) == STDOUT_FILENO, DAEMONIZE_ERR_BASE+5);
   ensure_sys_err(open(stderrfile, O_WRONLY|O_CREAT|O_APPEND, 0600) == STDERR_FILENO, DAEMONIZE_ERR_BASE+6);
}

} // end of namespace pcomn::sys
} // end of namespace pcomn
