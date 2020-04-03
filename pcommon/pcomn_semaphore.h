/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
#ifndef __PCOMN_SEMAPHORE_H
#define __PCOMN_SEMAPHORE_H
/*******************************************************************************
 FILE         :   pcomn_semaphore.h
 COPYRIGHT    :   Yakov Markovitch, 2020

 DESCRIPTION  :   Process-private fast counting semaphore.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   28 Jan 2020
*******************************************************************************/
/** @file

 Process-private fast counting semaphore and classic binary Dijkstra semaphore
 (AKA benafore).
*******************************************************************************/
#include "pcomn_platform.h"

#if !(defined(PCOMN_PL_LINUX) && defined(PCOMN_PL_X86))
#error Lightweight semaphores are currently supported only on x86-64 Linux
#endif

#include "pcomn_syncobj.h"
#include "pcomn_meta.h"
#include "pcommon.h"

#include <atomic>

namespace pcomn {

class counting_semaphore ;
class binary_semaphore ;

/***************************************************************************//**
 Unlike a mutex a counting_semaphore is not tied to a thread - acquiring a
 semaphore can occur on a different thread than releasing the semaphore.

 A counting_semaphore contains an internal counter initialized by the constructor.

 This counter is decremented by calls to acquire(), acquire_some(), try_acquire(),
 try_acquire_some(), and related methods, and is incremented by calls to release().

 When the counter is zero, acquire() blocks until the counter is
 decremented, but try_acquire() does not block; try_acquire_for() and try_acquire_until()
 block until the counter is decremented or a timeout is reached.
*******************************************************************************/
class counting_semaphore {
    PCOMN_NONCOPYABLE(counting_semaphore) ;
    PCOMN_NONASSIGNABLE(counting_semaphore) ;

public:
    explicit counting_semaphore(unsigned init_count = 0) :
        _value(init_count)
    {
        check_overflow(init_count, "counting_semaphore") ;
    }

    /// Get the maximum value the semaphore count can have.
    static constexpr int32_t max_count() { return std::numeric_limits<int32_t>::max() ; }

    /// Decrement the internal counter by the specified amount, even if the result is <0.
    /// Never blocks.
    ///
    /// @return Value of internal count before decrementing.
    ///
    int32_t borrow(unsigned count) ;

    /// Acquire specified count of tokens.
    ///
    /// If internal counter is >=count, acquires tokens, otherwise blocks until
    /// the intenal counter becomes big enough.
    ///
    /// @return `count`.
    /// @note Establishes a full memory barrier.
    ///
    unsigned acquire(unsigned count)
    {
        return acquire_with_timeout(count, count, {}, {}) ;
    }

    /// Acquire single token.
    /// @return 1
    /// @note Establishes a full memory barrier.
    ///
    unsigned acquire() { return acquire(1) ; }

    /// Acquire between 1 and (greedily) `maxcount`.
    ///
    /// If internal counter is >0, acquires min(counter,maxcount), otherwise blocks
    /// until the internal counter becomes positive.
    ///
    /// @return Actually acquired amount (<=`maxcount`).
    /// @note Establishes a full memory barrier.
    ///
    unsigned acquire_some(unsigned maxcount)
    {
        return acquire_with_timeout(1, maxcount, {}, {}) ;
    }

    /// Attempt to decrement the internal counter by 1 as long as the result is >=0.
    /// Never blocks.
    ///
    /// @return 1 on success (counter decremented), 0 otherwise.
    ///
    /// @note If successful (nonzero return) establishes a full memory barrier.
    /// @note Obstruction-free.
    ///
    unsigned try_acquire() noexcept { return try_acquire(1) ; }

    /// Attempt to decrement the internal counter by the specified amount as long as the
    /// result is >=0.
    /// Never blocks.
    ///
    /// @return `count` on success (counter decremented), 0 otherwise.
    ///
    /// @note Either decrements by full `count` or not at all; @see try_acquire_some().
    /// @note If successful (nonzero return) establishes a full memory barrier.
    /// @note Obstruction-free.
    ///
    unsigned try_acquire(unsigned count)
    {
        return try_acquire_in_userspace(count, count) ;
    }

    /// Acquire with lock, or try acquire without lock, or try acquire with timeout of
    /// any kind (duration, or monotonic time, or system time).
    ///
    ///  - `mode` is None:   acquire(), `timeout` ignored
    ///  - `mode` is Period: try_acquire_for() or try_acquire(), `timeout` is relative
    ///  - `mode` is SteadyClock: try_acquire_until(), `timeout` is from epoch
    ///  - `mode` is SystemClock: try_acquire_until(), `timeout` is from epoch
    ///
    bool universal_acquire(unsigned count, TimeoutMode mode,
                           std::chrono::nanoseconds timeout)
    {
        return acquire_with_timeout(count, count, mode, timeout) ;
    }

    unsigned universal_acquire_some(unsigned maxcount, TimeoutMode mode,
                                    std::chrono::nanoseconds timeout)
    {
        return acquire_with_timeout(1, maxcount, mode, timeout) ;
    }

    /// @note Establishes a full memory barrier.
    /// @note Lock-free (not wait-free!)
    void release() { release(1) ; }

    /// @note Establishes a full memory barrier.
    /// @note Lock-free (not wait-free!)
    void release(unsigned count) ;

    /***************************************************************************
     Acquire with timeout
    ***************************************************************************/

    /// Acquire between 0 and (greedily) `maxcount`, inclusive, never blocks.
    ///
    /// @return Actually acquired amount (<=`maxcount`).
    ///
    unsigned try_acquire_some(unsigned maxcount) noexcept
    {
        return try_acquire_in_userspace(1, std::min<unsigned>(maxcount, max_count())) ;
    }

    template<typename R, typename P>
    unsigned try_acquire_for(const std::chrono::duration<R, P> &rel_time,
                             unsigned count = 1)
    {
        return acquire_with_timeout(count, count, TimeoutMode::Period, rel_time) ;
    }

    template<typename Clock, typename Duration>
    unsigned try_acquire_until(const std::chrono::time_point<Clock, Duration> &abs_time,
                               unsigned count = 1)
    {
        return acquire_with_timeout(count, count, timeout_mode<Clock>(), abs_time.time_since_epoch()) ;
    }

    template<typename R, typename P>
    unsigned try_acquire_some_for(const std::chrono::duration<R, P> &rel_time, unsigned maxcount)
    {
        return acquire_with_timeout(1, maxcount, TimeoutMode::Period, rel_time) ;
    }

    template<typename Clock, typename Duration>
    unsigned try_acquire_some_until(const std::chrono::time_point<Clock, Duration> &abs_time,
                                    unsigned maxcount)
    {
        return acquire_with_timeout(1, maxcount, timeout_mode<Clock>(), abs_time.time_since_epoch()) ;
    }

private:
    union data {
        constexpr data(uint64_t v = 0) noexcept : _value(v) {}

        constexpr data(int32_t count, uint32_t wcount) noexcept :
            _token_count(count),
            _waiting_count(wcount)
        {}

        uint64_t _value ;

        struct {
            int32_t  _token_count ; /* The count of remaining tokens, address is used
                                     * as a futex. */

            uint32_t _waiting_count ; /* The count of (potentially) blocked threads
                                  * (if a thread requests more than current token count,
                                  * it is blocked until token count becomes big enough).
                                  * We can avoid futex_wake when _waiting_count is 0. */
        } ;
    } ;
private:
    union {
        data                  _data ;
        std::atomic<uint64_t> _value {} ;
    } ;

private:
    static void check_overflow(int64_t count, const char *msg)
    {
        if (unlikely(count > counting_semaphore::max_count()))
            throw_syserror(std::errc::value_too_large, msg) ;
    }

    unsigned acquire_with_timeout(unsigned mincount, unsigned maxcount,
                                  TimeoutMode mode, std::chrono::nanoseconds timeout) ;

    unsigned try_acquire_in_userspace(unsigned mincount, unsigned maxcount) ;
    unsigned acquire_with_lock(int32_t mincount, int32_t maxcount,
                               TimeoutMode mode, std::chrono::nanoseconds timeout) ;
    bool data_cas(data &expected, const data &desired) noexcept ;

    template<typename Clock>
    constexpr static TimeoutMode timeout_mode() { return timeout_mode_from_clock<Clock>() ; }
} ;

/***************************************************************************//**
 Classic Dijkstra binary semaphore AKA benaphore: nonrecursive lock, which
 in contrast to mutex allows calling lock() and uinlock() from different threads
 (the thread that acquired the lock is not its "owner"), also allows self-locking.

 Provides the usual set of methods:
   - lock()
   - try_lock()
   - try_lock_for()
   - try_lock_until()
   - unlock()

 The unlock() is idempotent, calling it on unlocked benafore is no-op.
*******************************************************************************/
class binary_semaphore {
    PCOMN_NONCOPYABLE(binary_semaphore) ;
    PCOMN_NONASSIGNABLE(binary_semaphore) ;
public:
    /// Default constructor, creates an unlocked benaphore.
    constexpr binary_semaphore() noexcept : _data{} {}

    /// Create a benaphore with expicitly specified initial acquirement state.
    explicit constexpr binary_semaphore(bool acquired) noexcept : _data(acquired, 0) {}

    ~binary_semaphore() { NOXCHECK(_data._wcount == 0) ; }

    /// Acquire lock.
    /// If the lock is held by @em any thread (including itself), wait for it to be
    /// released.
    void lock() { try_lock() || lock_with_timeout({}, {}) ; }

    /// Try to acquire the lock.
    /// This call never blocks and never takes a kernel call.
    /// @return true, if this thread has successfully acquired the lock; false, if the
    /// lock is already held by any thread, including itself.
    bool try_lock() noexcept
    {
        return data_cas(as_mutable(data(false, 0)), data(true, 0)) ;
    }

    /// Release the lock.
    /// Idempotent, calling it on unlocked benafore is valid and no-op.
    ///
    void unlock() ;

    /// Try to acquire the lock, block until specified timeout_duration has elapsed
    /// or the lock is acquired, whichever comes first.
    ///
    /// Uses std::chrono::steady_clock to measure a duration, thus immune to clock
    /// adjusments. May block for longer than timeout_duration due to scheduling or
    /// resource contention delays.
    ///
    /// If timeout_duration is zero, behaves like try_lock().
    ///
    /// @return true on successful lock acquisition, false when the timeout expired.
    ///
    template<typename R, typename P>
    bool try_lock_for(const std::chrono::duration<R, P> &timeout_duration)
    {
        return lock_with_timeout(TimeoutMode::Period, timeout_duration) ;
    }

    /// Try to acquire the lock, block until specified `abs_time` has been reached or
    /// the lock is acquired, whichever comes first.
    ///
    /// Only allows std::chrono::steady_clock or std::chrono::system_clock to specify
    /// `abs_time`.
    /// If `abs_time` has already passed, behaves like try_lock() but still
    /// can make a system call in the presence of contention.
    ///
    /// @return true on successful lock acquisition, false when the timeout is reached.
    ///
    template<typename Clock, typename Duration>
    bool try_lock_until(const std::chrono::time_point<Clock, Duration> &abs_time)
    {
        return lock_with_timeout(timeout_mode_from_clock<Clock>(), abs_time.time_since_epoch()) ;
    }

protected:
    bool lock_with_timeout(TimeoutMode mode, std::chrono::nanoseconds timeout) ;

private:
    union data {
        constexpr data() : _value(0) {}
        explicit constexpr data(bool locked, uint32_t wcount) noexcept :
            _locked(locked),
            _wcount(wcount)
        {}

        uint64_t _value ;
        struct {
            int32_t  _locked ; /* Bool value, 0 or 1, address is used as a futex. */
            uint32_t _wcount ; /* The count of (potentially) blocked threads.
                                * We can avoid futex_wake when _wcount is 0. */
        } ;
    } ;

private:
    union {
        data                  _data ;
        std::atomic<uint64_t> _value ;
    } ;

private:
    bool data_cas(data &expected, const data &desired) noexcept
    {
        return _value.compare_exchange_strong(expected._value, desired._value, std::memory_order_acq_rel) ;
    }
} ;

}  // end of namespace pcomn

#endif /* __PCOMN_SEMAPHORE_H */
