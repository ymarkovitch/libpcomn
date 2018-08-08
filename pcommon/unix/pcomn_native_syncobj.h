/*-*- mode: c++; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_NATIVE_SYNCOBJ_H
#define __PCOMN_NATIVE_SYNCOBJ_H
/*******************************************************************************
 FILE         :   pcomn_native_syncobj.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2018. All rights reserved.

 DESCRIPTION  :   Native synchronization objects for Unices

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   16 Feb 2008
*******************************************************************************/
/** @file
 Define native synchronization objects, like read-write mutex, etc., for Unix.

  @li PCOMN_HAS_NATIVE_PROMISE
  @li PCOMN_HAS_NATIVE_RWMUTEX
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

#ifdef PCOMN_PL_LINUX
// Futex support
#include <linux/futex.h>
#include <sys/syscall.h>
#include <sys/time.h>
// sched_getcpu()
#include <sched.h>
#endif

namespace pcomn {
namespace sys {

#ifdef PCOMN_PL_X86
/// Emit pause instruction to prevent excess processor bus usage.
/// For busy wait loops.
inline void pause_cpu() { asm volatile("pause\n": : :"memory") ; }

inline void pause_cpu(size_t cycle_count)
{
   // The apporximate count of CPU clocks per pause operation.
   // MUST be the power of 2.
   static const size_t pause_clk = 8 ;
   for (size_t i = cycle_count & pause_clk - 1 ; i-- ; pause_cpu()) ;
}

#else
inline void pause_cpu() {}
inline void pause_cpu(size_t)
{
}
#endif

#ifdef PCOMN_PL_LINUX
/******************************************************************************/
/** Get logical CPU(core) the calling thread is running on.
 Never fails: if underlying shched_getcpu fails, returns 0.
*******************************************************************************/
inline unsigned get_current_cpu_core()
{
   const int core = sched_getcpu() ;
   return core >= 0 ? core : 0 ;
}

/*******************************************************************************
 Futex API
*******************************************************************************/
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

#define PCOMN_HAS_NATIVE_PROMISE 1
/******************************************************************************/
/** Promise lock is a binary semaphore with only possible state change from locked
 to unlocked.

 The promise lock is constructed either in locked (default) or unlocked state and
 has two members: wait() and unlock().

 If promise lock is in locked state, all wait() callers are blocked until the it is
 switched to unlocked state; in unlocked state, wait() is no-op.

 unlock() is idempotent and can be called arbitrary number of times (i.e., the invariant
 is "after calling unlock() the lock is in unlocked state no matter what state it's been
 before in").

 @note The promise lock is @em not mutex in the sense there is no "ownership" of
 the lock: @em any thread may call unlock().
*******************************************************************************/
class native_promise_lock {
      PCOMN_NONCOPYABLE(native_promise_lock) ;
      PCOMN_NONASSIGNABLE(native_promise_lock) ;
   public:
      explicit constexpr native_promise_lock(bool initially_locked = true) :
         _locked(initially_locked)
      {}

      ~native_promise_lock() { unlock() ; }

      void wait()
      {
         while (atomic_op::load(&_locked, std::memory_order_acq_rel)
                && futex(&_locked, FUTEX_WAIT_PRIVATE, 1) == EINTR) ;
      }

      void unlock()
      {
         if (atomic_op::load(&_locked, std::memory_order_acquire) &&
             atomic_op::cas(&_locked, 1, 0, std::memory_order_release))

            futex(&_locked, FUTEX_WAKE_PRIVATE, INT_MAX) ;
      }

   private:
      int32_t _locked ;
} ;

#define PCOMN_HAS_NATIVE_BENAFORE 1
/******************************************************************************/
/** Classic binary Dijkstra semaphore; nonrecursive lock that allow both self-locking
 and unlocking by any thread (not only by the "owner" thread that acquired the lock).
*******************************************************************************/
class binary_semaphore {
      PCOMN_NONCOPYABLE(binary_semaphore) ;
      PCOMN_NONASSIGNABLE(binary_semaphore) ;
      static constexpr const int32_t ST_UNLOCKED = 0 ;
      static constexpr const int32_t ST_LOCKED   = 1 ;
      static constexpr const int32_t ST_LOCKWAIT = 2 ;
   public:
      constexpr binary_semaphore() :
         _state(ST_UNLOCKED)
      {}
      explicit constexpr binary_semaphore(bool acquire) :
         _state(!!acquire)
      {}

      ~binary_semaphore() { NOXCHECK(_state != ST_LOCKWAIT) ; }

      /// Acquire lock.
      /// If the lock is held by @em any thread (including itself), wait for it to be
      /// released.
      void lock()
      {
         int32_t expected = ST_UNLOCKED ;
         // Attempt to swap ST_UNLOCKED -> ST_LOCKED; on success, we are the first
         if (atomic_op::cas(&_state, &expected, ST_LOCKED, std::memory_order_acquire))
            // Locked, no contention; here _state is always ST_LOCKED
            return ;

         // Contended
         int32_t contended = atomic_op::xchg(&_state, ST_LOCKWAIT, std::memory_order_acquire) ;

         while (contended)
         {
            // Wait in the kernel (possibly)
            futex(&_state, FUTEX_WAIT_PRIVATE, ST_LOCKWAIT) ;
            contended = atomic_op::xchg(&_state, ST_LOCKWAIT, std::memory_order_acquire) ;
         }
         // Locked, there is contention; here _state is always ST_LOCKWAIT
      }

      /// Try to acquire the lock.
      /// This call never blocks and never takes a kernel call.
      /// @return true, if this thread has successfully acquired the lock; false, if the
      /// lock is already held by any thread, including itself.
      bool try_lock()
      {
         return atomic_op::cas(&_state, ST_UNLOCKED, ST_LOCKED) ;
      }

      /// Release the lock.
      void unlock()
      {
         switch (atomic_op::xchg(&_state, ST_UNLOCKED, std::memory_order_release))
         {
            case ST_UNLOCKED: // Let unlock be idempotent...
            case ST_LOCKED:   // There was no contention, no need to wake up threads in the kernel.
               return ;
         }

         // Someone probably still waits inside the kernel, it was ST_LOCKWAIT.
         // Wake up one thread.
         futex(&_state, FUTEX_WAKE_PRIVATE, 1) ;
      }

   private:
      int32_t _state ;
} ;

#endif // PCOMN_PL_LINUX

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
