/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_semaphore.cpp
 COPYRIGHT    :   Yakov Markovitch, 2020

 DESCRIPTION  :   Process-private fast counting semaphore.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   28 Jan 2020
*******************************************************************************/
#include "pcomn_semaphore.h"
#include "pcomn_utils.h"

#include <semaphore.h>

using namespace pcomn::sys ;

namespace pcomn {

static constexpr inline FutexWait futex_wait_mode(TimeoutMode mode)
{
    // Always use absolute timeout to compensate for EINTR possibility.
    return FutexWait::AbsTime |
        ((mode == TimeoutMode::SystemClock) ? FutexWait::SystemClock : FutexWait::SteadyClock) ;
}

/*******************************************************************************
 counting_semaphore
*******************************************************************************/
inline
bool counting_semaphore::data_cas(data &expected, const data &desired) noexcept
{
    // Use release MO to ensure a thread acquiring/releasing tokens synchronizes
    // with us and other threads that manipulate tokens before.
    return _value.compare_exchange_strong(expected._value, desired._value, std::memory_order_acq_rel) ;
}

// Try to capture `count` tokens w/o blocking.
unsigned counting_semaphore::try_acquire_in_userspace(unsigned minc, unsigned maxc)
{
    check_overflow(minc, "counting_semaphore::acquire") ;

    if (!(minc | maxc) || (maxc < minc) || maxc > (unsigned)max_count())
        return 0 ;

    const int32_t mincount = minc ;
    const int32_t maxcount = maxc ;

    data old_data (_value.load(std::memory_order_relaxed)) ;
    data new_data ;
    unsigned acquired_count ;

    do
    {
        if (old_data._token_count < mincount)
            return 0 ;

        new_data = old_data ;

        acquired_count = std::min(new_data._token_count, maxcount) ;
        new_data._token_count -= acquired_count ;
    }
    while(!data_cas(old_data, new_data)) ;

    return acquired_count ;
}

unsigned counting_semaphore::acquire_with_lock(int32_t mincount, int32_t maxcount,
                                               TimeoutMode mode, std::chrono::nanoseconds timeout)
{
    const uint64_t waiting_one = data(0, 1)._value ;

    // Convert relative timeout to absolute: calculate the end of timeout period.
    // Always use absolute timeout to compensate for EINTR possibility.
    struct timespec timeout_point = timeout_timespec(mode, timeout) ;
    const FutexWait wait_mode = futex_wait_mode(mode) ;

    // Check in to the set of waiting threads, we're going to sleep.
    // Note we need a new value (preincrement, not postincrement).
    data old_data (_value.fetch_add(waiting_one, std::memory_order_acq_rel) + waiting_one) ;

    for(;;)
    {
        const int32_t desired_count = std::clamp(old_data._token_count, mincount, maxcount) ;

        if (old_data._token_count >= desired_count)
        {
            // At least our thread is waiting.
            NOXCHECK(old_data._waiting_count) ;

            // Probably enough available tokens: try both to grab the tokens _and_ check
            // out from the set of waiting threads.
            const data new_data (old_data._token_count - desired_count, old_data._waiting_count - 1) ;

            if (data_cas(old_data, new_data))
            {
                // The only exit from this function.
                return desired_count ;
            }

            // Bad luck, loop again.
            // Note the old_data value is reloaded by CAS call above.
            continue ;
        }

        // If not enough tokens available, go to sleep.
        const int result = mode == TimeoutMode::None
            ? futex_wait(&_data._token_count, old_data._token_count)
            : futex_wait(&_data._token_count, old_data._token_count, wait_mode, timeout_point) ;

        if (result < 0)
            switch(errno)
            {
                // Repeat an attempt.
                case EAGAIN: case EINTR: break ;
                // The wait is timed out, it's OK if timeout possibility is assumed.
                case ETIMEDOUT: if (mode != TimeoutMode::None)
                {
                    // Timeout, don't try more, check out from the set of waiting threads.
                    _value.fetch_sub(waiting_one, std::memory_order_acq_rel) ;
                    return 0 ;
                }

                // Error
                default: PCOMN_ENSURE_POSIX(-1, "FUTEX_WAIT") ;
            }

        // Reload the data value
        old_data = _value.load(std::memory_order_relaxed) ;
    }
}

unsigned counting_semaphore::acquire_with_timeout(unsigned mincount, unsigned maxcount,
                                                  TimeoutMode mode, std::chrono::nanoseconds timeout)
{
    maxcount = std::min(maxcount, (unsigned)max_count()) ;

    if (const unsigned acquired_count = try_acquire_in_userspace(mincount, maxcount))
        return acquired_count ;

    if (mode != TimeoutMode::None && timeout == std::chrono::nanoseconds())
        return 0 ;

    return acquire_with_lock(mincount, maxcount, mode, timeout) ;
}

int32_t counting_semaphore::borrow(unsigned count)
{
    // borrow() ensures at least `acquire` MO
    data old_data (_value.load(std::memory_order_acquire)) ;
    data new_data ;

    if (!count)
        return old_data._token_count ;

    do
    {
        const int64_t new_token_count = (int64_t)old_data._token_count - count ;
        check_overflow(-new_token_count, "counting_semaphore::borrow") ;

        new_data = old_data ;
        new_data._token_count = new_token_count ;
    }
    while(!data_cas(old_data, new_data)) ;

    return old_data._token_count ;
}

void counting_semaphore::release(unsigned count)
{
    if (!count)
        return ;

    data old_data (_value.load(std::memory_order_relaxed)) ;
    data new_data ;

    do
    {
        // Note new_token_count can be negative thanks to borrow()
        const int64_t new_token_count = (int64_t)old_data._token_count + count ;
        check_overflow(new_token_count, "counting_semaphore::release") ;

        new_data = old_data ;
        new_data._token_count = new_token_count ;
    }
    while(!data_cas(old_data, new_data)) ;

    // If there is any potentially waiting threads, wake at most count of them
    if (old_data._waiting_count)
        futex_wake(&_data._token_count, std::min<unsigned>(count, old_data._waiting_count)) ;
}

/*******************************************************************************
 binary_semaphore
*******************************************************************************/
bool binary_semaphore::lock_with_timeout(TimeoutMode mode, std::chrono::nanoseconds timeout)
{
    const uint64_t waiting_one = data(false, 1)._value ;

    // Convert relative timeout to absolute: calculate the end of timeout period.
    // Always use absolute timeout to compensate for EINTR possibility.
    struct timespec timeout_point = timeout_timespec(mode, timeout) ;
    const FutexWait wait_mode = futex_wait_mode(mode) ;

    data old_data ;
    // Check in to the set of waiting threads, we're going to sleep.
    old_data._value = _value.fetch_add(waiting_one, std::memory_order_acq_rel) + waiting_one ;

    do {

        while (old_data._locked)
        {
            // Benaphore is locked by someone else, go to sleep.
            const int result = mode == TimeoutMode::None
                ? futex_wait(&_data._locked, 1)
                : futex_wait(&_data._locked, 1, wait_mode, timeout_point) ;

            if (result < 0)
                switch(errno)
                {
                    // Repeat an attempt.
                    case EAGAIN: case EINTR: break ;

                    // The wait is timed out: it's OK if timeout possibility is assumed,
                    // otherwise this code is an error.
                    case ETIMEDOUT: if (mode != TimeoutMode::None)
                    {
                        // Check out from the set of waiting threads.
                        _value.fetch_sub(waiting_one, std::memory_order_acq_rel) ;
                        return false ;
                    }

                    // Error
                    default: PCOMN_ENSURE_POSIX(-1, "FUTEX_WAIT") ;
                }

            // Reload the data value
            old_data._value = _value.load(std::memory_order_relaxed) ;
            // Loop to while(old_data._locked)
        }

        // At least our thread is waiting.
        NOXCHECK(old_data._wcount) ;
    }
    // Get the lock _and_ check out from the set of waiting threads.
    while (!data_cas(old_data, data(true, old_data._wcount - 1))) ;

    return true ;
}

void binary_semaphore::unlock()
{
    data old_data (true, 0) ;

    // Load data through "optimistic" CAS
    if (data_cas(old_data, data(false, 0)))
        return ;

    while(old_data._locked)
    {
        const uint32_t wcount = old_data._wcount ;

        if (!data_cas(old_data, data(false, wcount)))
            continue ;

        if (wcount)
            // There are (potentially) waiting threads, wake one
            futex_wake(&_data._locked, 1) ;

        break ;
    }
}

}  // end of namespace pcomn
