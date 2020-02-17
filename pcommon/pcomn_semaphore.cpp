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

/*******************************************************************************
 counting_semaphore
*******************************************************************************/
inline bool counting_semaphore::data_cas(sem_data &expected, const sem_data &desired) noexcept
{
    // Use release MO to ensure a thread acquiring/releasing tokens synchronizes
    // with us and other threads that manipulate tokens before.
    return _data.compare_exchange_strong(expected._value, desired._value, std::memory_order_acq_rel) ;
}

// Try to capture `count` tokens w/o blocking.
unsigned counting_semaphore::try_acquire_in_userspace(unsigned minc, unsigned maxc)
{
    check_overflow(minc, "counting_semaphore::acquire") ;

    if (!(minc | maxc) || (maxc < minc) || maxc > (unsigned)max_count())
        return 0 ;

    const int32_t mincount = minc ;
    const int32_t maxcount = maxc ;

    sem_data old_data (_data.load(std::memory_order_relaxed)) ;
    sem_data new_data ;
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
    // Convert relative timeout to absolute
    const auto make_timepoint = [mode, timeout]() -> struct timespec
    {
        switch (mode)
        {
            case TimeoutMode::None: return {} ;
            case TimeoutMode::Period:
                return nsec_to_timespec(std::chrono::steady_clock::now().time_since_epoch() + timeout) ;
            default: break ;
        }
        return nsec_to_timespec(timeout) ;
    } ;

    // Always use absolute timeout to compensate for EINTR possibility.
    const FutexWait wait_mode = FutexWait::AbsTime |
        ((mode == TimeoutMode::SteadyClock) ? FutexWait::SteadyClock : FutexWait::SystemClock) ;
    // Calculate the end of timeout period.
    struct timespec timeout_point = make_timepoint() ;

    // Check in to the set of waiting threads, we're going to sleep.
    sem_data old_data (_data.fetch_add(sem_data(0, 1)._value, std::memory_order_acq_rel)) ;

    for(;;)
    {
        const int32_t desired_count = std::clamp(old_data._token_count, mincount, maxcount) ;

        if (old_data._token_count >= desired_count)
        {
            // At least our thread is waiting.
            NOXCHECK(old_data._waiting_count) ;

            // Probably enough available tokens: try both to grab the tokens _and_ check
            // out from the set of waiting threads.
            const sem_data new_data (old_data._token_count - desired_count, old_data._waiting_count - 1) ;

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
            ? futex_wait(&_sdata._token_count, old_data._token_count)
            : futex_wait(&_sdata._token_count, old_data._token_count,
                         wait_mode|FutexWait::AbsTime, timeout_point) ;

        if (result == ETIMEDOUT && mode != TimeoutMode::None)
            return 0 ;

        PCOMN_ENSURE_POSIX(result == EAGAIN || result == EINTR ? 0 : result, "FUTEX_WAIT") ;

        // Reload the data value
        old_data = _data.load(std::memory_order_relaxed) ;
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
    sem_data old_data (_data.load(std::memory_order_acquire)) ;
    sem_data new_data ;

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

    sem_data old_data (_data.load(std::memory_order_relaxed)) ;
    sem_data new_data ;

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
        futex_wake(&_sdata._token_count, std::min<unsigned>(count, old_data._waiting_count)) ;
}

}  // end of namespace pcomn
