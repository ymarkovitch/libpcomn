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

 Process-private fast counting semaphore.
*******************************************************************************/
#include "pcomn_platform.h"

#if !(defined(PCOMN_PL_LINUX) && defined(PCOMN_PL_X86))
#error Lightweight semaphores are currently supported only on x86-64 Linux
#endif

#include "pcomn_syncobj.h"
#include "pcomn_meta.h"

#include <atomic>

namespace pcomn {

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
    explicit counting_semaphore(int32_t init_count = 0) : _data(init_count) {}

    /// Get the maximum value the semaphore count can have.
    static constexpr int32_t max_count() { return std::numeric_limits<int32_t>::max() ; }

    /// Decrement the internal counter by the specified amount, even if the result is <0.
    /// Never blocks.
    ///
    /// @return 1 on success (counter decremented), 0 otherwise.
    ///
    int32_t borrow(unsigned count) ;

    /// Attempt to decrement the internal counter by 1 as long as the result is >=0.
    /// Never blocks.
    ///
    /// @return 1 on success (counter decremented), 0 otherwise.
    ///
    unsigned try_acquire() noexcept { return try_acquire(1) ; }

    /// Attempt to decrement the internal counter by the specified amount as long as the
    /// result is >=0.
    /// Never blocks.
    ///
    /// @return `count` on success (counter decremented), 0 otherwise.
    /// @note Either decrements by full `count` or not at all; @see try_acquire_some().
    ///
    unsigned try_acquire(unsigned count) noexcept
    {
        return try_acquire_in_userspace(count, count) ;
    }

    /// Acquire between 0 and (greedily) `maxcount`, inclusive, never blocks.
    ///
    /// @return Actually acquired amount (<=`maxcount`).
    ///
    unsigned try_acquire_some(unsigned maxcount) noexcept
    {
        return try_acquire_in_userspace(1, maxcount) ;
    }

    unsigned acquire() { return acquire(1) ; }

    unsigned acquire(unsigned count) ;

    /// Acquire between 1 and (greedily) `maxcount`.
    ///
    /// If internal counter is >0, acquires min(counter,maxcount), otherwise blocks
    /// until the intenal counter becomes positive.
    ///
    /// @return Actually acquired amount (<=`maxcount`).
    ///
    unsigned acquire_some(unsigned maxcount) ;

    void release() { release(1) ; }

    void release(unsigned count) ;

private:
    union sem_data {
        constexpr sem_data(uint64_t v = 0) noexcept : _value(v) {}

        constexpr sem_data(int32_t count, uint32_t wcount) noexcept :
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
        std::atomic<uint64_t> _data {} ;
        sem_data              _sdata ;
    } ;

private:
    unsigned try_acquire_in_userspace(unsigned mincount, unsigned maxcount) noexcept ;
    unsigned acquire_with_lock(int32_t mincount, int32_t maxcount) ;
    bool data_cas(sem_data &expected, const sem_data &desired) noexcept ;
} ;

}  // end of namespace pcomn

#endif /* __PCOMN_SEMAPHORE_H */
