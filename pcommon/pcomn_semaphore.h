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
 counting_semaphore
*******************************************************************************/
class counting_semaphore {
    PCOMN_NONCOPYABLE(counting_semaphore) ;
    PCOMN_NONASSIGNABLE(counting_semaphore) ;

public:
    explicit counting_semaphore(int32_t init_count = 0) : _data(init_count)
    {
        PCOMN_VERIFY(init_count >= 0) ;
    }

    /// Attempt to decrement the semaphore counter by 1, never blocks.
    /// @return 1 on success (counter decremented), 0 otherwise.
    ///
    unsigned try_acquire() noexcept { return try_acquire(1) ; }

    /// Attempt to decrement the semaphore counter by the specified amount,
    /// never blocks.
    ///
    /// @return `count` on success (counter decremented), 0 otherwise.
    /// @note Either decrements by full `count` or not at all; @see try_acquire_some().
    ///
    unsigned try_acquire(int32_t count) noexcept ;

    /// Acquire between 0 and (greedily) `maxcount`, inclusive, never blocks.
    ///
    /// @return Actually acquired amount (<=`maxcount`).
    ///
    unsigned try_acquire_some(int32_t maxcount) noexcept ;

    unsigned acquire() { return acquire(1) ; }

    unsigned acquire(int32_t count) ;

    unsigned acquire_some(int32_t maxcount) ;

    void release() { release(1) ; }

    void release(int32_t count) ;

private:
    union sem_data {
        constexpr sem_data(uint64_t v) noexcept : _value(v) {}

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
    unsigned try_acquire_in_userspace(int32_t mincount, int32_t maxcount) ;
} ;

}  // end of namespace pcomn

#endif /* __PCOMN_SEMAPHORE_H */
