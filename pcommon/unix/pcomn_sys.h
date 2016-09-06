/*-*- mode: c++; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_SYS_UNIX_H
#define __PCOMN_SYS_UNIX_H
/*******************************************************************************
 FILE         :   pcomn_sys.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   System routines for UNIX/Linux platforms

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   18 Nov 2008
*******************************************************************************/
/** @file
 System routines for UNIX/Linux platforms.
*******************************************************************************/
#include <pcomn_except.h>
#include <pcomn_alloca.h>
#include <pcomn_cstrptr.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/mman.h>

#include <limits.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>

namespace pcomn {
namespace sys {

inline const char *strlasterr()
{
   static thread_local char errbuf[128] ;
   *errbuf = errbuf[sizeof errbuf - 1] = 0 ;
   return strerror_r(errno, errbuf, sizeof errbuf - 1) ;
}

inline size_t pagesize()
{
   static const thread_local size_t sz = sysconf(_SC_PAGESIZE) ;
   return sz ;
}

inline fileoff_t filesize(int fd)
{
   struct stat st ;
   return fstat(fd, &st) == 0 ? (fileoff_t)st.st_size : (fileoff_t)-1 ;
}

inline fileoff_t filesize(const char *name)
{
   struct stat st ;
   return stat(name, &st) == 0 ? (fileoff_t)st.st_size : (fileoff_t)-1 ;
}

inline void *pagealloc()
{
   void * const mem = mmap(nullptr, 1, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0) ;
   return mem == MAP_FAILED ? nullptr : mem ;
}

inline void pagefree(void *p)
{
   if (p)
      munmap(p, 1) ;
}

#define _PCOMN_SYS_FSTATAT(statfn, dirfd, path, flags, raise)        \
   do {                                                              \
      fsstat result ;                                                \
      const int r = ::statfn((dirfd), (path), &(result), (flags)) ;  \
      if ((raise))                                                   \
         PCOMN_ENSURE_POSIX(r, #statfn) ;                            \
      else if (r == -1)                                              \
         (result).clear() ;                                          \
      return result ;                                                \
   } while(false)

inline fsstat filestatat(int dirfd, const char *path, RaiseError raise = DONT_RAISE_ERROR)
{
   _PCOMN_SYS_FSTATAT(fstatat, dirfd, path, 0, raise) ;
}

inline fsstat filestatat(const char *path, RaiseError raise = DONT_RAISE_ERROR)
{
   return filestatat(AT_FDCWD, path, raise) ;
}

inline fsstat linkstatat(int dirfd, const char *path, RaiseError raise = DONT_RAISE_ERROR)
{
   _PCOMN_SYS_FSTATAT(fstatat, dirfd, path, AT_SYMLINK_NOFOLLOW, raise) ;
}

inline fsstat linkstatat(const char *path, RaiseError raise = DONT_RAISE_ERROR)
{
   return linkstatat(AT_FDCWD, path, raise) ;
}

#undef _PCOMN_SYS_FSTATAT

inline fileoff_t filesize(int fd, const char *name)
{
   const fsstat &st = filestatat(fd, name) ;
   return st ? (fileoff_t)st.st_size : (fileoff_t)-1 ;
}

/// Indicate if two filestats denote the same file.
inline bool same_file(const struct stat &st1, const struct stat &st2)
{
   return
      st1.st_ino == st2.st_ino && st1.st_dev == st2.st_dev &&
      (st1.st_ino || st1.st_dev) ;
}

inline int fflags(int fd)
{
   return PCOMN_ENSURE_POSIX(fcntl(fd, F_GETFL), "fcntl") ;
}

/// Change O_APPEND, O_ASYNC, O_DIRECT, O_NOATIME, or O_NONBLOCK flags
inline void set_fflags(int fd, unsigned flags)
{
   PCOMN_ENSURE_POSIX(fcntl(fd, F_SETFL, flags), "fcntl") ;
}

inline int set_fflags(int fd, unsigned flags, unsigned mask)
{
   const int oldflags = fflags(fd) ;
   set_fflags(fd, (oldflags & ~mask) | (flags & mask)) ;
   return oldflags ;
}

/// Reopen a file by its descriptor
///
/// Does @em not duplicates file descriptor, but creates a new entry in the open file
/// table, so e.g. resulting descriptor can have completely independent state, offset,
/// flock locks, etc.
///
inline int reopen(int fd, int flags)
{
   int newfd ;
   if (fd < 0)
   {
      errno = EBADF ;
      newfd = -1 ;
   }
   else
   {
      char name[64] ;
      snprintf(name, sizeof name, "/proc/self/fd/%d", fd) ;
      newfd = open(name, flags) ;
   }
   PCOMN_ENSURE_POSIX(newfd, "pcomn::sys::reopen") ;
   return newfd ;
}

inline int reopen(int fd) { return reopen(fd, fflags(fd)) ; }

inline int opendirfd(const char *name, RaiseError raise = DONT_RAISE_ERROR)
{
  const int flags = O_RDONLY|O_NDELAY|O_DIRECTORY|O_LARGEFILE
#ifdef O_CLOEXEC
   |O_CLOEXEC
#endif
    ;
  const int fd = open(name, flags) ;
  if (raise && fd < 0)
     PCOMN_THROW_SYSERROR("open") ;
  return fd ;
}

inline int hardflush(int fd)
{
   return fsync(fd) ;
}

/// Daemonize the currently running programming
/// @param dir          Which dir to change to after becoming a daemon
/// @param stdinfile    A file to redirect stdin to
/// @param stdoutfile   A file to redirect stdout from
/// @param stderrfile   A file to redirect stderr to
_PCOMNEXP void daemonize(const char *dir = "/",
                         const char *stdinfile = "/dev/null",
                         const char *stdoutfile = "/dev/null",
                         const char *stderrfile = "/dev/null") ;

/*******************************************************************************
 Clock routines
*******************************************************************************/
typedef int64_t nanotime_t ;

/// Convert timespec structure to a 64-bit integer containing the number of nanoseconds
/// since epoch
inline nanotime_t timespec_to_nsec(const struct timespec &ts)
{
   return ts.tv_sec * 1000000000LL + ts.tv_nsec ;
}

/// Convert a 64-bit integer containing the number of nanoseconds since epoch to a
/// timespec structure
inline struct timespec nsec_to_timespec(nanotime_t t)
{
   const struct timespec ts = { t / 1000000000LL, t % 1000000000LL } ;
   return ts ;
}

#ifdef _POSIX_TIMERS

inline nanotime_t clock_getres(clockid_t clk_id)
{
   struct timespec res ;
   PCOMN_ENSURE_POSIX(clock_getres(clk_id, &res), "clock_getres") ;
   return timespec_to_nsec(res) ;
}

inline nanotime_t clock_gettime(clockid_t clk_id)
{
   struct timespec ts ;
   PCOMN_ENSURE_POSIX(clock_gettime(clk_id, &ts), "clock_gettime") ;
   return timespec_to_nsec(ts) ;
}

inline nanotime_t clock_realtime() { return clock_gettime(CLOCK_REALTIME) ; }

inline nanotime_t clock_uptime() { return clock_gettime(CLOCK_MONOTONIC) ; }

inline nanotime_t clock_cputime_process() { return clock_gettime(CLOCK_PROCESS_CPUTIME_ID) ; }

inline nanotime_t clock_cputime_thread()  { return clock_gettime(CLOCK_THREAD_CPUTIME_ID) ; }

#endif

inline unsigned long long thread_id()
{
   return pthread_self() ;
}

} // end of namespace pcomn::sys
} // end of namespace pcomn

#endif /* __PCOMN_SYS_UNIX_H */
