/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_SYNCOBJ_H
#define __PCOMN_SYNCOBJ_H
/*******************************************************************************
 FILE         :   pcomn_syncobj.h
 COPYRIGHT    :   Yakov Markovitch, 1997-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Synchronisation primitives

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   13 Jan 1997
*******************************************************************************/
/** @file
 Synchronization primitives missing in STL: shared mutex, primitive semaphore
 (event_mutex)
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcomn_assert.h>

#include <mutex>
#include <type_traits>

#include PCOMN_PLATFORM_HEADER(pcomn_native_syncobj.h)

/******************************************************************************/
/** Polymorphic scope lock

 Let's 'mymutex' is a variable of any type T for which pcomn::PTGuarg<T> is defined.
 To create a lexical scope guard variable 'myguardvar' for mymutex, use the following:
 @code
 PCOMN_SCOPE_LOCK(myguardvar, mymutex) ;
 // Code under the mutex follows
 bla ;
 @endcode

 @param guard_varname   The name of local guard variable.
 @param lock_expr       The expression that should return PTScopeGuard<T> value.
*******************************************************************************/
#define PCOMN_SCOPE_LOCK(guard_varname, lock_expr, ...)                 \
   std::lock_guard<typename std::remove_reference<decltype((lock_expr))>::type> guard_varname ((lock_expr), ##__VA_ARGS__)

#define PCOMN_SCOPE_XLOCK(guard_varname, lock_expr, ...)                 \
   std::unique_lock<typename std::remove_reference<decltype((lock_expr))>::type> guard_varname ((lock_expr), ##__VA_ARGS__)

#define PCOMN_SCOPE_R_LOCK(guard_varname, lock_expr, ...)               \
   pcomn::shared_lock<typename std::remove_reference<decltype((lock_expr))>::type> guard_varname ((lock_expr), ##__VA_ARGS__)

#define PCOMN_SCOPE_W_LOCK(guard_varname, lock_expr, ...)   PCOMN_SCOPE_LOCK(guard_varname, (lock_expr), ##__VA_ARGS__)
#define PCOMN_SCOPE_W_XLOCK(guard_varname, lock_expr, ...)  PCOMN_SCOPE_XLOCK(guard_varname, (lock_expr), ##__VA_ARGS__)

namespace pcomn {

/******************************************************************************/
/** Simple binary Dijkstra semaphore; nonrecursive mutex that allow both self-locking
 and unlocking by another thread (not only by the "owner" thread that acquired the lock).
*******************************************************************************/
class event_mutex {
      PCOMN_NONCOPYABLE(event_mutex) ;
      PCOMN_NONASSIGNABLE(event_mutex) ;
   public:
      explicit event_mutex() :
         _native_lock()
      {}

      explicit event_mutex(bool acquired) :
         _native_lock()
      {
         if (acquired)
            _native_lock.lock() ;
      }

      /// Acquire lock.
      /// If the lock is held by @em any thread (including itself), wait for it to be
      /// released.
      void lock() { _native_lock.lock() ; }

      /// Try to acquire lock.
      /// This call never blocks.
      /// @return true, if this thread has successfully acquired the lock; false, if the
      /// lock is already held by any thread, including itself.
      bool try_lock() { return _native_lock.try_lock() ; }

      /// Release the lock.
      void unlock() { _native_lock.unlock()  ; }

   private:
      NativeThreadLock _native_lock ;
} ;

/******************************************************************************/
/** Read-write mutex on top of a native (platform) read-write mutex for those platforms
 that have such native mutex (e.g. POSIX Threads).
*******************************************************************************/
class shared_mutex {
      shared_mutex(const shared_mutex&) = delete ;
      void operator=(const shared_mutex&) = delete ;
   public:
      shared_mutex() = default ;

      void lock() { _lock.lock() ; }
      bool try_lock() { return _lock.try_lock() ; }
      void unlock() { _lock.unlock() ; }

      void lock_shared() { _lock.lock_shared() ; }
      bool try_lock_shared() { return _lock.try_lock_shared() ; }
      void unlock_shared() { _lock.unlock_shared() ; }
   private:
      NativeRWMutex _lock ;
} ;

/******************************************************************************/
/** General-purpose shared (read-write) mutex ownership wrapper allowing  deferred
 locking and transfer of lock ownership.

 Locking a shared_lock locks the associated mutex in shared (read) mode.
 To lock it in exclusive (write) mode, std::unique_lock can be used.
*******************************************************************************/
template<typename Mutex>
class shared_lock {
      shared_lock(const shared_lock&) = delete ;
      void operator=(const shared_lock&) = delete ;
   public:
      typedef Mutex mutex_type;

      constexpr shared_lock() noexcept : _lock(), _owns(false) {}
      explicit shared_lock(mutex_type &m) : _lock(&m), _owns(true)
      {
         _lock->lock_shared() ;
      }
      constexpr shared_lock(mutex_type &m, std::defer_lock_t) noexcept : _lock(&m), _owns(false) {}
      constexpr shared_lock(mutex_type &m, std::try_to_lock_t) : _lock(&m), _owns(_lock->try_lock_shared()) {}
      constexpr shared_lock(mutex_type &m, std::adopt_lock_t) : _lock(&m), _owns(true) {}

      ~shared_lock() { unlock_nocheck() ; }

      shared_lock(shared_lock &&other) noexcept : _lock(other._lock), _owns(other._owns)
      {
         other._lock = 0 ;
         other._owns = false ;
      }

      shared_lock &operator=(shared_lock &&other) noexcept
      {
         unlock_nocheck() ;
         shared_lock(std::move(other)).swap(*this) ;
         return *this ;
      }

      void lock()
      {
         ensure_nonempty() ;
         _lock->lock_shared() ;
         _owns = true ;
      }

      bool try_lock()
      {
         ensure_nonempty() ;
         return
            _owns = mutex()->try_lock_shared() ;
      }

      void unlock()
      {
         if (!mutex())
            return ;

         if (!owns_lock())
            throw_system_error(std::errc::operation_not_permitted,
                               "Attempt to unlock already unlocked mutex") ;
         mutex()->unlock_shared() ;
         _owns = false ;
      }

      void swap(shared_lock &other) noexcept
      {
         std::swap(_lock, other._lock) ;
         std::swap(_owns, other._owns) ;
      }

      mutex_type *release() noexcept
      {
         mutex_type * const ret = mutex() ;
         _lock = 0 ;
         _owns = false ;
         return ret;
      }

      bool owns_lock() const noexcept { return _owns ; }
      explicit operator bool() const noexcept { return owns_lock() ; }
      mutex_type *mutex() const noexcept { return _lock ; }

   private:
      mutex_type *_lock ;
      bool        _owns ;

      void ensure_nonempty() const
      {
         if (!mutex())
            throw_system_error(std::errc::operation_not_permitted, "NULL mutex pointer") ;
      }
      void unlock_nocheck()
      {
         if (owns_lock())
         {
            mutex()->unlock_shared() ;
            _owns = false ;
         }
      }
} ;

} // end of namespace pcomn

#endif /* __PCOMN_SYNCOBJ_H */
