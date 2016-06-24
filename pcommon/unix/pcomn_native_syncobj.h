/*-*- mode: c++; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_NATIVE_SYNCOBJ_H
#define __PCOMN_NATIVE_SYNCOBJ_H
/*******************************************************************************
 FILE         :   pcomn_native_syncobj.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2016. All rights reserved.

 DESCRIPTION  :   Native synchronization objects for Unices

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   16 Feb 2008
*******************************************************************************/
#include <pcommon.h>
#include <pcomn_assert.h>
#include <pcomn_except.h>
#include <pcomn_atomic.h>

#ifndef PCOMN_PL_UNIX
#error This header supports only Unix.
#endif

#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <fcntl.h>
#include <limits.h>
#include <unistd.h>

#ifde PCOMN_PL_LINUX
// Futex support
#include <sys/syscall.h>
#include <sys/time.h>
#endif

namespace pcomn {
namespace sys {

#ifdef PCOMN_PL_LINUX

inline int futex(void *addr1, int32_t op, int32_t val1, struct timespec *timeout, void *addr2, int32_t val3)
{
  return syscall(SYS_futex, addr1, op, val1, timeout, addr2, val3) ;
}

inline int futex(int32_t *self, int32_t op, int32_t value)
{
  return futex(self, op, value, NULL, NULL, 0) ;
}

inline int futex(int32_t *self, int32_t op, int32_t value, int32_t val2)
{
  return futex(self, op, value, (struct timespec *)(intptr_t)val2, NULL, 0) ;
}

#define PCOMN_HAS_NATIVE_LATCH 1
/******************************************************************************/
/** Simple binary Dijkstra semaphore; nonrecursive mutex that allow both self-locking
 and unlocking by another thread (not only by the thread that had acquired the lock).
*******************************************************************************/
class native_latch {
      PCOMN_NONCOPYABLE(native_latch) ;
      PCOMN_NONASSIGNABLE(native_latch) ;
   public:
      explicit constexpr native_latch(bool snap_locked = false) : _futex(snap_locked) {}
      ~native_latch() { unlock() ; }

      void wait()
      {
         int32_t v = atomic_op::load(&_futex, std::memory_order_acq_rel) ;
         if (v & 1)
            while (_futex == v && futex(&_futex, FUTEX_WAIT_PRIVATE, v) == EINTR) ;
      }

      void lock()
      {
         using atomic_op ;
         for (int32_t v = load(&_futex, std::memory_order_acq) ;
              !(v & 1) && !cas(&_futex, v, v+1) ;) ;
      }

      void unlock()
      {
         using atomic_op ;
         int32_t v = load(&_futex, std::memory_order_acq) ;
         if ((v & 1) && cas(&_futex, v, v+1) ;)
            futex(&_futex, FUTEX_WAKE_PRIVATE, INT_MAX) ;
      }

   private:
      volatile int32_t _futex ; /* LSBit==0: open, LSBit==1: closed */
} ;

#else
#error PCommon synchronization objects are currently supported only for Linux and Windows.
#endif

#define PCOMN_HAS_NATIVE_RWMUTEX 1
/******************************************************************************/
/** Read-write mutex for POSIX Threads.
*******************************************************************************/
class native_rw_mutex {
      PCOMN_NONCOPYABLE(native_rw_mutex) ;
      PCOMN_NONASSIGNABLE(native_rw_mutex) ;
   public:
      constexpr native_rw_mutex() = default ;

      ~native_rw_mutex()
      {
         const int destroy_errcode =
            pthread_rwlock_destroy(&_lock) ;

         (void)destroy_errcode ;
         // Violating this check most likely means destroying a still locked mutex
         NOXCHECK(destroy_errcode != EBUSY) ;
         NOXCHECK(!destroy_errcode) ;
      }

      void lock()
      {
         PCOMN_ENSURE_ENOERR(pthread_rwlock_wrlock(&_lock), "pthread_rwlock_wrlock") ;
      }

      bool try_lock()
      {
         const int result = pthread_rwlock_trywrlock(&_lock) ;
         if (result == EBUSY)
            return false ;
         PCOMN_ENSURE_ENOERR(result, "pthread_rwlock_trywrlock") ;
         return true ;
      }

      bool unlock() { return release_lock() ; }

      void lock_shared()
      {
         PCOMN_ENSURE_ENOERR(pthread_rwlock_rdlock(&_lock), "pthread_rwlock_rdlock") ;
      }

      bool try_lock_shared()
      {
         const int result = pthread_rwlock_tryrdlock(&_lock) ;
         if (result == EBUSY)
            return false ;
         PCOMN_ENSURE_ENOERR(result, "pthread_rwlock_tryrdlock") ;
         return true ;
      }

      bool unlock_shared() { return release_lock() ; }

   private:
      pthread_rwlock_t _lock = PTHREAD_RWLOCK_INITIALIZER ;

      bool release_lock()
      {
         // Release should never _ever_ throw exceptions, it is very likely be called
         // from destructors.
         return pthread_rwlock_unlock(&_lock) == 0 ;
      }
} ;

/******************************************************************************/
/** File lock; provides read-write mutex logic
*******************************************************************************/
class native_file_mutex {
      PCOMN_NONASSIGNABLE(native_file_mutex) ;
   public:
      explicit native_file_mutex(const char *filename, int flags = O_CREAT|O_RDONLY, int mode = 0600) :
         _fd(openfile(PCOMN_ENSURE_ARG(filename), flags, mode))
      {}

      explicit native_file_mutex(const std::string &filename, int flags = O_CREAT|O_RDONLY, int mode = 0600) :
         _fd(openfile(filename.c_str(), flags, mode))
      {}

      native_file_mutex(int fd, bool owned) :
         _fd(owned ? fd : (fd | ((unsigned)INT_MAX + 1)))
      {
         PCOMN_ASSERT_ARG(fd >= 0) ;
      }

      native_file_mutex(const native_file_mutex &other, int flags) :
         _fd(reopenfile(other.fd(), flags))
      {}

      ~native_file_mutex()
      {
         NOXVERIFY(!owned() || close(fd()) >= 0) ;
      }

      int fd() const { return _fd & ~((unsigned)INT_MAX + 1) ; }
      bool owned() const { return !(_fd & ((unsigned)INT_MAX + 1)) ; }

      void lock() { NOXVERIFY(acquire_lock(LOCK_EX)) ; }
      bool try_lock() { return acquire_lock(LOCK_EX|LOCK_NB) ; }
      bool unlock()
      {
         // Release should never _ever_ throw exceptions, it is very likely be called
         // from destructors.
         int err ;
         while((err = posix_errno(flock(fd(), LOCK_UN))) == EINTR) ;
         NOXCHECKX(!err, strerror(err)) ;
         return !err ;
      }

      void lock_shared() { NOXVERIFY(acquire_lock(LOCK_SH)) ; }
      bool try_lock_shared() { return acquire_lock(LOCK_SH|LOCK_NB) ; }
      bool unlock_shared() { return unlock() ; }

   private:
      const int _fd ;

      bool acquire_lock(int flags) const
      {
         int err ;
         while((err = posix_errno(flock(fd(), flags))) == EINTR) ;
         if (err == EWOULDBLOCK)
            return false ;
         PCOMN_ENSURE_ENOERR(err, "flock") ;
         return true ;
      }

      static int openfile(const char *name, int flags, int mode)
      {
         const int d = ::open(name, flags, mode) ;
         PCOMN_CHECK_POSIX(d, "native_file_mutex cannot open '%s' for locking", name) ;
         return d ;
      }
      static int reopenfile(int fd, int flags)
      {
         char name[64] ;
         snprintf(name, sizeof name, "/proc/self/fd/%d", fd) ;
         return PCOMN_ENSURE_POSIX(open(name, flags), "native_file_mutex::reopenfile") ;
      }
} ;

} // end of namespace pcomn::sys
} // end of namespace pcomn

#endif /* __PCOMN_NATIVE_SYNCOBJ_H */
