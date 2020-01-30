/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_semaphore.cpp
 COPYRIGHT    :   Yakov Markovitch, 2020

 DESCRIPTION  :   Process-private fast counting semaphore.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   28 Jan 2020
*******************************************************************************/
#include "pcomn_semaphore.h"

#include <semaphore.h>

namespace pcomn {

/*******************************************************************************
 counting_semaphore
*******************************************************************************/
// Try to capture `count` tokens w/o blocking.
unsigned counting_semaphore::try_acquire_in_userspace(int32_t mincount, int32_t maxcount)
{
    if (!(mincount | maxcount))
        return 0 ;

    sem_data d (_data.load(std::memory_order_relaxed)) ;
    unsigned acquired_count ;
    do
    {
        if (d._token_count < mincount)
            return 0 ;

        sem_data newdata = d ;

        acquired_count = std::min(newdata._token_count, maxcount) ;
        newdata._token_count -= acquired_count ;
    }
    while(!_data.compare_exchange_strong(d._value, acquired_count, std::memory_order_acq_rel)) ;

    return acquired_count ;
}

unsigned counting_semaphore::try_acquire(int32_t count) noexcept
{
    return try_acquire_in_userspace(count, count) ;
}

unsigned counting_semaphore::try_acquire_some(int32_t maxcount) noexcept
{
    return try_acquire_in_userspace(1, maxcount) ;
}

unsigned counting_semaphore::acquire(int32_t count)
{
    if (try_acquire_in_userspace(count, count))
        return count ;

    // Check in into the set of waiting threads, we're going to sleep.
    sem_data d (_data.fetch_add(sem_data(0, 1)._value, std::memory_order_relaxed)) ;

    for(;;)
    {
        if (d._token_count >= count)
        {
            NOXCHECK(d._waiting_count) ;

            // Probably enough available tokens: try both to grab the tokens _and_ check
            // out from the set of waiting threads.
            const sem_data new_data (d._token_count - count, d._waiting_count - 1) ;

            if (_data.compare_exchange_strong(d._value, new_data._value))
            {
                // The only exit from this function.
                return count ;
            }

            // Bad luck, loop again.
            // Note the d value is reloaded by CAS call above.
            continue ;
        }

        // If not enough tokens available, go to sleep.
        const int result = sys::futex_wait(&_sdata._token_count, d._token_count) ;

        PCOMN_ENSURE_POSIX(result == EAGAIN || result == EINTR ? 0 : result, "FUTEX_WAIT") ;

        // Reload the data value
        d = _data.load(std::memory_order_relaxed) ;
    }
}

#if 0
unsigned counting_semaphore::acquire_some(int32_t maxcount) ;

void counting_semaphore::release(int32_t count)
{
    if (!count)
        return ;

    PCOMN_ASSERT_ARG(count > 0) ;


}
#endif

}  // end of namespace pcomn
