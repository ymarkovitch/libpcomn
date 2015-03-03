/*-*- mode: c++; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_NATIVE_SYNCOBJ_H
#define __PCOMN_NATIVE_SYNCOBJ_H
/*******************************************************************************
 FILE         :   pcomn_native_syncobj.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2015. All rights reserved.

 DESCRIPTION  :   Native synchronization objects for Unices

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   16 Feb 2008
*******************************************************************************/
#include <pcommon.h>
#include <pcomn_assert.h>
#include <pcomn_except.h>
#include <pcomn_meta.h>
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

// This is very pthreads-implementation-specific, but we use it only for debugging anyway
#if defined(__GLIBC__)
// The lock should be unlocked
#define __PCOMN_CHECK_UNCLOCKED(pthread_mutex) NOXCHECK(!(pthread_mutex).__data.__owner)
#else
#define __PCOMN_CHECK_UNCLOCKED(pthread_mutex)
#endif

namespace pcomn {

class NativeCondVar ;

class NativeBasicMutex {
      PCOMN_NONCOPYABLE(NativeBasicMutex) ;
      PCOMN_NONASSIGNABLE(NativeBasicMutex) ;
   public:
      friend class NativeCondVar ;

      void lock()
      {
         const int result = pthread_mutex_lock(&_lock) ;
         NOXCHECK(!result) ;
         PCOMN_ENSURE_ENOERR(result, "pthread_mutex_lock") ;
      }

      bool try_lock()
      {
         const int result = pthread_mutex_trylock(&_lock) ;
         if (result == EBUSY)
            return false ;
         NOXCHECK(!result) ;
         PCOMN_ENSURE_ENOERR(result, "pthread_mutex_trylock") ;
         return true ;
      }

      bool unlock()
      {
         // Release should never _ever_ throw exceptions, it is very likely be called
         // from destructors.
         return pthread_mutex_unlock(&_lock) == 0 ;
      }

   protected:
      pthread_mutex_t _lock ;

      explicit NativeBasicMutex(const pthread_mutex_t &initializer) :
         _lock(initializer)
      {}

      ~NativeBasicMutex()
      {
         #ifdef __PCOMN_DEBUG
         const int mutex_destroy_errcode =
         #endif
            pthread_mutex_destroy(&_lock) ;

         // Violating this check most likely means destroying a still locked mutex
         NOXCHECK(mutex_destroy_errcode != EBUSY) ;
         NOXCHECK(!mutex_destroy_errcode) ;
      }
} ;

#ifdef PCOMN_PL_LINUX
/******************************************************************************/
/** Simple binary Dijkstra semaphore; nonrecursive mutex that allow both self-locking
 and unlocking by another thread (not only by the thread that had acquired the lock).
*******************************************************************************/
class NativeThreadLock : public NativeBasicMutex {
      typedef NativeBasicMutex ancestor ;

      static const pthread_mutex_t &initializer()
      {
         // Don't use PTHREAD_MUTEX_ERRORCHECK! We _need_ it to be able to lock itself and
         // to be unlocked by nonowner.
         static const pthread_mutex_t initializer_data = PTHREAD_MUTEX_INITIALIZER ;
         return initializer_data ;
      }

   public:
      NativeThreadLock() :
         ancestor(initializer())
      {}

      ~NativeThreadLock()
      {
         // This lock _may_ be destroyed in the locked state.
         // The code below is very Linux-pthreads specific, but this implementation of
         // NativeThreadLock is Linux-specific anyway
         NOXVERIFY(!_lock.__data.__lock || pthread_mutex_unlock(&_lock) == 0) ;
      }
} ;

typedef NativeThreadLock NativeNonrecursiveMutex ;

#else
#error PCommon synchronization objects are currently supported only for Linux and Windows.
#endif

#define PCOMN_HAS_NATIVE_RECURSIVE_LOCK 1
/******************************************************************************/
/** Recursive lock for POSIX Threads.
*******************************************************************************/
class NativeRecursiveMutex : public NativeBasicMutex {
      typedef NativeBasicMutex ancestor ;

      static const pthread_mutex_t &initializer()
      {
         // Nonstandard extension. Will use pthread_mutex_init on nonGlibc platforms.
         static const pthread_mutex_t initializer_data = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP ;
         return initializer_data ;
      }

   public:
      NativeRecursiveMutex() :
         ancestor(initializer())
      {}
} ;

#define PCOMN_HAS_NATIVE_CONDVAR 1
/******************************************************************************/
/** Condition variable for POSIX Threads.
*******************************************************************************/
class NativeCondVar {
      PCOMN_NONCOPYABLE(NativeCondVar) ;
      PCOMN_NONASSIGNABLE(NativeCondVar) ;
   public:
      NativeCondVar()
      {
         static const pthread_cond_t initializer = PTHREAD_COND_INITIALIZER ;
         _condvar = initializer ;
      }

      ~NativeCondVar()
      {
         #ifdef __PCOMN_DEBUG
         const int destroy_errcode =
         #endif
            pthread_cond_destroy(&_condvar) ;
         NOXCHECK(!destroy_errcode) ;
      }

      void wait(NativeNonrecursiveMutex &mutex)
      {
         PCOMN_ENSURE_ENOERR(pthread_cond_wait(&_condvar, &mutex._lock), "pthread_cond_wait") ;
      }

      void notify()
      {
         PCOMN_ENSURE_ENOERR(pthread_cond_signal(&_condvar), "pthread_cond_signal") ;
      }

      void notify_all()
      {
         PCOMN_ENSURE_ENOERR(pthread_cond_broadcast(&_condvar), "pthread_cond_broadcast") ;
      }

   private:
      pthread_cond_t _condvar ;
} ;

#define PCOMN_HAS_NATIVE_RWMUTEX 1
/******************************************************************************/
/** Read-write mutex for POSIX Threads.
*******************************************************************************/
class NativeRWMutex {
      PCOMN_NONCOPYABLE(NativeRWMutex) ;
      PCOMN_NONASSIGNABLE(NativeRWMutex) ;
   public:
      NativeRWMutex()
      {
         static const pthread_rwlock_t initializer = PTHREAD_RWLOCK_INITIALIZER ;
         _lock = initializer ;
      }

      ~NativeRWMutex()
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
      pthread_rwlock_t _lock ;

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
class NativeFileMutex {
      PCOMN_NONASSIGNABLE(NativeFileMutex) ;
   public:
      explicit NativeFileMutex(const char *filename, int flags = O_CREAT|O_RDONLY, int mode = 0600) :
         _fd(openfile(PCOMN_ENSURE_ARG(filename), flags, mode))
      {}

      explicit NativeFileMutex(const std::string &filename, int flags = O_CREAT|O_RDONLY, int mode = 0600) :
         _fd(openfile(filename.c_str(), flags, mode))
      {}

      NativeFileMutex(int fd, bool owned) :
         _fd(owned ? fd : (fd | ((unsigned)INT_MAX + 1)))
      {
         PCOMN_ASSERT_ARG(fd >= 0) ;
      }

      NativeFileMutex(const NativeFileMutex &other, int flags) :
         _fd(reopenfile(other.fd(), flags))
      {}

      ~NativeFileMutex()
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
         PCOMN_CHECK_POSIX(d, "NativeFileMutex cannot open '%s' for locking", name) ;
         return d ;
      }
      static int reopenfile(int fd, int flags)
      {
         char name[64] ;
         snprintf(name, sizeof name, "/proc/self/fd/%d", fd) ;
         return PCOMN_ENSURE_POSIX(open(name, flags), "NativeFileMutex::reopenfile") ;
      }
} ;

} // end of namespace pcomn

#endif /* __PCOMN_NATIVE_SYNCOBJ_H */
