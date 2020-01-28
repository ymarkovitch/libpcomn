/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_SYNCOBJ_H
#define __PCOMN_SYNCOBJ_H
/*******************************************************************************
 FILE         :   pcomn_syncobj.h
 COPYRIGHT    :   Yakov Markovitch, 1997-2020. All rights reserved.
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
#include <atomic>

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
   const std::lock_guard<std::remove_cvref_t<decltype((lock_expr))>> guard_varname ((lock_expr), ##__VA_ARGS__)

#define PCOMN_SCOPE_XLOCK(guard_varname, lock_expr, ...)                 \
   std::unique_lock<std::remove_cvref_t<decltype((lock_expr))>> guard_varname ((lock_expr), ##__VA_ARGS__)

#define PCOMN_SCOPE_R_LOCK(guard_varname, lock_expr, ...)               \
   pcomn::shared_lock<std::remove_cvref_t<decltype((lock_expr))>> guard_varname ((lock_expr), ##__VA_ARGS__)

#define PCOMN_SCOPE_W_LOCK(guard_varname, lock_expr, ...)   PCOMN_SCOPE_LOCK(guard_varname, (lock_expr), ##__VA_ARGS__)
#define PCOMN_SCOPE_W_XLOCK(guard_varname, lock_expr, ...)  PCOMN_SCOPE_XLOCK(guard_varname, (lock_expr), ##__VA_ARGS__)

namespace pcomn {

/******************************************************************************/
/** Get logical CPU(core) which the calling thread is running on.
 Never fails: if underlying OS API fails, returns 0.
*******************************************************************************/
inline unsigned current_cpu_core()
{
   return sys::get_current_cpu_core() ;
}

/// Probabilistically get logical CPU(core) which the calling thread is running on.
///
/// Caches the result of actual current_cpu_core() call in a thread_local variable and
/// refereshes it every 2^^N calls.
/// Since the scheduler tries to minimize thread movements between CPUs, the result is
/// correct in the most cases.
///
template<unsigned N>
unsigned probable_current_cpu_core()
{
   static_assert(N < 16, "The specified value of log2(calls) is too large.") ;

   static thread_local unsigned coreid_cache ;
   static thread_local unsigned call_count = 0 ;
   const unsigned mask = (1U << N) - 1 ;

   if (!(call_count++ & mask))
      coreid_cache = current_cpu_core() ;

   return coreid_cache ;
}

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
      sys::native_rw_mutex _lock ;
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

#if PCOMN_HAS_NATIVE_PROMISE
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
class promise_lock : private sys::native_promise_lock {
      typedef sys::native_promise_lock ancestor ;
      PCOMN_NONCOPYABLE(promise_lock) ;
      PCOMN_NONASSIGNABLE(promise_lock) ;
   public:
      /// Create a promise lock, initially locked by default.
      explicit constexpr promise_lock(bool initially_locked = true) :
         ancestor(initially_locked)
      {}

      ~promise_lock() = default ;

      using ancestor::wait ;
      using ancestor::unlock ;
} ;

#else // No native_promise_lock, fallback to standard (inefficient)
class promise_lock : private std::mutex {
      typedef std::mutex ancestor ;
      PCOMN_NONCOPYABLE(promise_lock) ;
      PCOMN_NONASSIGNABLE(promise_lock) ;
   public:
      explicit constexpr promise_lock(bool initially_locked = true) :
         _locked(initially_locked)
      {}

      ~promise_lock() = default ;

   private:
      std::cond
} ;
#endif // PCOMN_HAS_NATIVE_PROMISE

/******************************************************************************/
/** Identifier dispenser: requests range of integral numbers, then (atomically)
 allocates successive numbers from the range upon request.

 @param AtomicInt       Atomic integer type
 @param RangeProvider   Callable type; if r is RangeProvider then calling r without
 arguments must be valid and its result must be convertible to
 @p std::pair<AtomicInt,AtomicInt> (i.e. @p (std::pair<AtomicInt,AtomicInt>)r() must be
 a valid expression)

 @note range.second designates the "past-the-end" point in the range, the same way as
 STL containers/algorithms do, so for a nonempty range @p range.first<range.second
 RangeProvider @em must return @em only nonemty ranges. The starting point of a new range
 must @em follow the end of the previous range (not necessary immediately, gaps are
 allowed).
*******************************************************************************/
template<typename AtomicInt, typename RangeProvider>
class ident_dispenser {
      PCOMN_STATIC_CHECK(is_atomic<AtomicInt>::value) ;
   public:
      typedef AtomicInt type ;

      ident_dispenser(const RangeProvider &provider, type incval = 1) :
         _increment(PCOMN_ENSURE_ARG(incval)),
         _next_id(0),
         _range(0, 0),
         _provider(provider)
      {}

      type increment() const { return _increment ; }

      /// Atomically allocate new ID.
      type allocate_id()
      {
         type id ;
         do if ((id = _next_id) < _range.first || id >= _range.second)
            {
               PCOMN_SCOPE_LOCK(range_guard, _rangelock) ;

               if ((id = _next_id) < _range.first || id >= _range.second)
               {
                  const std::pair<type, type> newrange = _provider() ;
                  PCOMN_VERIFY(newrange.first >= _range.second && newrange.second > newrange.first) ;
                  _next_id = newrange.first ;
                  // Issue a memory barrier
                  atomic_op::load(&_next_id, std::memory_order_acq_rel) ;

                  _range = newrange ;
               }
            }
         while (!atomic_op::cas(&_next_id, id, id + _increment)) ;

         return id ;
      }

   private:
      std::mutex              _rangelock ;
      const type              _increment ;
      type                    _next_id ;
      std::pair<type, type>   _range ;
      RangeProvider           _provider ;
} ;

/******************************************************************************/
/** Generator of integral identifiers which are unique insdide a process run.

 Atomically allocates a range of integral numbers for a thread and then allocates
 successive numbers from that range upon request from this thread until the range
 depletion, when allocates new range.
 Range allocation is atomic (interlocked), but the ident allocation is needn't
 be atomic since it is thread-local, thus performing only one interlocked
 operation per range (by default, per 65536 identifiers allocated).
*******************************************************************************/
template<typename Tag, typename Int = uint64_t, Int blocksize = 0x10000U>
class local_ident_dispenser {
      PCOMN_STATIC_CHECK(is_atomic<Int>::value) ;
      PCOMN_STATIC_CHECK(blocksize > 0) ;
   public:
      typedef Int type ;

      /// Atomically allocate new ID.
      static type allocate_id()
      {
         if (_next_id == _range_end)
         {
            _next_id = _next_start.fetch_add(blocksize) ;
            _range_end = _next_id + blocksize ;
         }
         return _next_id++ ;
      }
   private:
      thread_local static type _next_id ;
      thread_local static type _range_end ;
      static std::atomic<type> _next_start ;
} ;

template<typename Tag, typename Int, Int bsz>
thread_local Int local_ident_dispenser<Tag, Int, bsz>::_next_id {0} ;
template<typename Tag, typename Int, Int bsz>
thread_local Int local_ident_dispenser<Tag, Int, bsz>::_range_end {0} ;
template<typename Tag, typename Int, Int bsz>
std::atomic<Int> local_ident_dispenser<Tag, Int, bsz>::_next_start {bsz} ;

} // end of namespace pcomn

#endif /* __PCOMN_SYNCOBJ_H */
