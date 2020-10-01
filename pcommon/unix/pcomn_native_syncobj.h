/*-*- mode: c++; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_NATIVE_SYNCOBJ_H
#define __PCOMN_NATIVE_SYNCOBJ_H
/*******************************************************************************
 FILE         :   pcomn_native_syncobj.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2020. All rights reserved.

 DESCRIPTION  :   Native synchronization objects for Unices

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   16 Feb 2008
*******************************************************************************/
/** @file
 Define native synchronization objects, like read-write mutex, etc., for Unix.

  @li PCOMN_HAS_NATIVE_RWMUTEX
*******************************************************************************/
#include <pcomn_platform.h>
#ifndef PCOMN_PL_UNIX
#error This header supports only Unix.
#endif

#include <pcommon.h>
#include <pcomn_assert.h>
#include <pcomn_except.h>
#include <pcomn_atomic.h>
#include <pcomn_sys.h>

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
   // The log2 of the apporximate count of CPU clocks per pause operation.
   constexpr size_t pause_clk = 3 ;
   for (size_t i = (cycle_count + (1U << pause_clk) - 1) >> pause_clk ; i-- ; pause_cpu()) ;
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
/// FutexWait is the set of ORable flags to specify futex_wait behaviour.
enum class FutexWait : uint8_t {
   RelTime = 0,      /**< Wait for the specified duration (default). */
   AbsTime = 1,      /**< Wait until the specified point in time. */

   SteadyClock = 0,  /**< Use CLOCK_MONOTONIC. */
   SystemClock = 2   /**< Use CLOCK_REALTIME; ignored for RelTime. */
} ;

PCOMN_DEFINE_FLAG_ENUM(FutexWait) ;

constexpr inline bool operator!(FutexWait f) { return !(uint8_t)f ; }

inline int futex(void *addr1, int32_t op, int32_t val, struct timespec *timeout, void *addr2, int32_t val3) noexcept
{
   return syscall(SYS_futex, addr1, op, val, timeout, addr2, val3) ;
}

template<typename I>
inline int futex(I *self, int32_t op, int32_t value) noexcept
{
   PCOMN_STATIC_CHECK(atomic_type_v<I> && sizeof(I) == 4 && !std::is_const<I>()) ;

   return futex(self, op, value, NULL, NULL, 0) ;
}

template<typename I>
inline int futex(I *self, int32_t op, int32_t value, int32_t val2) noexcept
{
   PCOMN_STATIC_CHECK(atomic_type_v<I> && sizeof(I) == 4 && !std::is_const<I>()) ;

   return futex(self, op, value, (struct timespec *)(intptr_t)val2, NULL, 0) ;
}

/// Atomically test that *self still contains the expected_value, and if so, sleep
/// waiting for a FUTEX_WAKE operation on `self`.
///
/// @note Ihe operation is FUTEX_WAIT_PRIVATE.
/// @note Can be interrupted by a signal, in which case it returns EINTR.
///
template<typename I>
inline int futex_wait(I *self, int32_t expected_value) noexcept
{
   return futex(self, FUTEX_WAIT_PRIVATE, expected_value) ;
}

/// Test that `*self` still contains `expected_value`, and if so, sleeps waiting for
/// a futex_wake() on `self` or until `timeout` expired.
///
/// @note Ihe operation is FUTEX_WAIT_PRIVATE (on FutexWait::RelIime `flags`)
///  or FUTEX_WAIT_BITSET_PRIVATE (on FutexWait::AbsTime `flags`).
///
/// @note Can be interrupted by a signal before wake or timeot, in which case
/// it returns EINTR.
///
int futex_wait_with_timeout(void *self, int32_t expected_value, FutexWait flags, struct timespec timeout) ;

/// Test that `*self` still contains `expected_value`, and if so, sleeps waiting for
/// a futex_wake() on `self` or until `timeout` expired.
///
/// @note Ihe operation is FUTEX_WAIT_PRIVATE (on FutexWait::RelIime `flags`)
///  or FUTEX_WAIT_BITSET_PRIVATE (on FutexWait::AbsTime `flags`).
///
/// @note Can be interrupted by a signal before wake or timeot, in which case
/// it returns EINTR.
///
template<typename I>
inline int futex_wait(I *self, int32_t expected_value, FutexWait flags, struct timespec timeout)
{
   PCOMN_STATIC_CHECK(atomic_type_v<I> && sizeof(I) == 4 && !std::is_const<I>()) ;

   return futex_wait_with_timeout(self, expected_value, flags, timeout) ;
}

/// Wake at most `max_waked_count` of the waiters that are waiting (e.g., inside
/// futex_wait()) on the futex word at the address `self`.
///
/// @note When `max_waked_count` is not specified, this is futex_wake_any().
///
template<typename I>
inline int futex_wake(I *self, int32_t max_waked_count = 1)
{
   return futex(self, FUTEX_WAKE_PRIVATE, max_waked_count) ;
}

/// Wake all the waiters that are waiting futex_wait()) on the futex word
/// at the address `self`.
template<typename I>
inline int futex_wake_all(I *self)
{
   return futex_wake(self, std::numeric_limits<int32_t>::max()) ;
}

#endif // PCOMN_PL_LINUX

#define PCOMN_HAS_NATIVE_RWMUTEX 1
/***************************************************************************//**
 Read-write mutex for POSIX Threads.
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

/***************************************************************************//**
 File lock; provides read-write mutex logic
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
