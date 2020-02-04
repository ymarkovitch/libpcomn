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

namespace pcomn {

static inline void check_overflow(uint64_t count, const char *msg)
{
    PCOMN_ENSURE_POSIX(count <= (uint64_t)counting_semaphore::max_count() ? 0 : EOVERFLOW, msg) ;
}

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
unsigned counting_semaphore::try_acquire_in_userspace(unsigned minc, unsigned maxc) noexcept
{
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

unsigned counting_semaphore::acquire_with_lock(int32_t mincount, int32_t maxcount)
{
    // Check in to the set of waiting threads, we're going to sleep.
    sem_data old_data (_data.fetch_add(sem_data(0, 1)._value, std::memory_order_acq_rel)) ;

    for(;;)
    {
        const int32_t count = std::clamp(old_data._token_count, mincount, maxcount) ;

        if (old_data._token_count >= count)
        {
            // At least our thread is going to wait.
            NOXCHECK(old_data._waiting_count) ;

            // Probably enough available tokens: try both to grab the tokens _and_ check
            // out from the set of waiting threads.
            const sem_data new_data (old_data._token_count - count, old_data._waiting_count - 1) ;

            if (data_cas(old_data, new_data))
            {
                // The only exit from this function.
                return count ;
            }

            // Bad luck, loop again.
            // Note the old_data value is reloaded by CAS call above.
            continue ;
        }

        // If not enough tokens available, go to sleep.
        const int result = sys::futex_wait(&_sdata._token_count, old_data._token_count) ;

        PCOMN_ENSURE_POSIX(result == EAGAIN || result == EINTR ? 0 : result, "FUTEX_WAIT") ;

        // Reload the data value
        old_data = _data.load(std::memory_order_relaxed) ;
    }
}

unsigned counting_semaphore::acquire(unsigned exact_count)
{
    if (try_acquire_in_userspace(exact_count, exact_count))
        return exact_count ;

    check_overflow(exact_count, "counting_semaphore::acquire") ;

    return acquire_with_lock(exact_count, exact_count) ;
}

unsigned counting_semaphore::acquire_some(unsigned maxcount)
{
    if (const unsigned a = try_acquire_in_userspace(1, maxcount))
        return a ;

    return acquire_with_lock(1, std::min(maxcount, (unsigned)max_count())) ;
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
        check_overflow(-((int64_t)old_data._token_count - count), "counting_semaphore::borrow") ;

        new_data = old_data ;
        new_data._token_count -= count ;
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
        check_overflow((uint64_t)old_data._token_count + count, "counting_semaphore::release") ;

        new_data = old_data ;
        new_data._token_count += count ;
    }
    while(!data_cas(old_data, new_data)) ;

    // If there is any potentially waiting threads, wake at most count of them
    if (old_data._waiting_count)
        sys::futex_wake(&_sdata._token_count, std::min<unsigned>(count, old_data._waiting_count)) ;
}

}  // end of namespace pcomn
