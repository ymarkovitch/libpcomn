/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_blocqueue.cpp
 COPYRIGHT    :   Yakov Markovitch, 2020

 DESCRIPTION  :   Blocking concurrent queues.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   13 Feb 2020
*******************************************************************************/
#include "pcomn_blocqueue.h"

namespace pcomn {

PCOMN_STATIC_CHECK(2*blocqueue_controller::max_capacity() < counting_semaphore::max_count()) ;

/*******************************************************************************
 blocqueue_controller
*******************************************************************************/
blocqueue_controller::blocqueue_controller(unsigned capacity) :
    _capacity(validate_capacity(capacity))
{
    slots(EMPTY).release(capacity) ;
}

__noreturn __cold
void blocqueue_controller::invalid_capacity(unsigned new_capacity)
{
    throwf<std::out_of_range>
        ("Invalid capactity %u specified for blocking_queue, expected value between %u and %u",
         new_capacity, 1U, (unsigned)max_capacity()) ;
}

__noreturn __cold
void blocqueue_controller::invalid_acquire_count(unsigned count, const char *queue_end)
{
    throwf<std::out_of_range>
        ("Too big count %u is specified for %s operation on blocking_queue, maximum allowed is %u",
         count, queue_end, (unsigned)max_capacity()) ;
}

void blocqueue_controller::change_capacity(unsigned new_capacity)
{
    validate_capacity(new_capacity) ;

    PCOMN_SCOPE_LOCK(caplock, _capmutex) ;
    ensure_open() ;

    const int32_t diff = (int32_t)new_capacity - _capacity.load(std::memory_order_acquire) ;
    if (!diff)
        return ;

    change_data_capacity(new_capacity) ;

    _capacity.store(new_capacity, std::memory_order_release) ;

    if (diff > 0)
        // Capacity increased, raise the available empty slots count.
        slots(EMPTY).release(diff) ;

    else
        // Capacity reduced: in principle, we need to take (acquire) the difference
        // from the empty slots but this can block, so we borrow.
        slots(EMPTY).borrow(-diff) ;
}

inline
bool blocqueue_controller::atomic_test_and_set_closed() noexcept
{
    for (State s = _state.load(std::memory_order_relaxed) ; s != State::CLOSED ;)
    {
        // CST order is intentional, don't "optimize"!
        if (!_state.compare_exchange_strong(s, State::CLOSED))
            continue ;

        slots(FULL).release(blocqueue_controller::max_capacity()) ;
        return true ;
    }
    return false ;
}

inline
bool blocqueue_controller::wait_until_empty(unsigned full_slots_reserved,
                                            TimeoutKind kind,
                                            std::chrono::nanoseconds timeout) noexcept
{
    const unsigned empty_slots_wait_count = max_empty_slots() - full_slots_reserved ;

    if (!slots(EMPTY).universal_acquire(empty_slots_wait_count, timeout_mode(kind), timeout))
        return false ;

    slots(EMPTY).release(empty_slots_wait_count) ;
    return true ;
}

void blocqueue_controller::close_both_ends()
{
    PCOMN_SCOPE_LOCK(caplock, _capmutex) ;

    (close_push_end(TimeoutKind::NONE, {}) ||
     atomic_test_and_set_closed()) ;
}

bool blocqueue_controller::close_push_end(TimeoutKind kind, std::chrono::nanoseconds timeout)
{
    PCOMN_SCOPE_LOCK(caplock, _capmutex) ;

    State s = _state.load(std::memory_order_relaxed) ;

    if (s == State::CLOSED)
        return true ;

    if (s == State::OPEN)
    {
        // Memory fence
        _state.store(s = State::PUSH_CLOSED) ;

        // Allow any pushing thread: as _state is now PUSH_CLOSED, any push() caller
        // (AKA producer) will freely acquire(n) only to immediately notice that
        // the producing end is already closed, release(n), and to throw sequence_closed
        // (see start_pop()).
        //
        // Since now, the slots(EMPTY) count when the queue is empty is
        // capacity() + blocqueue_controller::max_capacity().

        slots(EMPTY).release(blocqueue_controller::max_capacity()) ;
    }

    if (slots(FULL).borrow(0))
    {
        return wait_until_empty(0, kind, timeout) ;
    }

    wait_until_empty() ;
    atomic_test_and_set_closed() ;

    return true ;
}

unsigned blocqueue_controller::start_pop(unsigned maxcount,
                                         TimeoutKind kind, std::chrono::nanoseconds timeout)
{
    validate_acquire_count(maxcount, "pop") ;

    ensure<sequence_closed>(_state.load(std::memory_order_acquire) != State::CLOSED) ;

    const unsigned acquired_count =
        slots(FULL).universal_acquire_some(maxcount, timeout_mode(kind), timeout) ;

    State s = _state.load(std::memory_order_acquire) ;

    if (s == State::OPEN || s == State::POP_CLOSING)
        return acquired_count ;

    auto checkin_guard (make_finalizer([&]{ slots(FULL).release(acquired_count) ; })) ;

    ensure<sequence_closed>(s != State::CLOSED) ;

    // Here s == PUSH_CLOSED
    if (_state.compare_exchange_strong(s, State::POP_CLOSING))
    {
        // This thread is just changed queue state PUSH_CLOSED -> POP_CLOSING.
        // Wait until others pop all but the last remaining items.

        checkin_guard.release() ;
        wait_until_empty(acquired_count) ;
        atomic_test_and_set_closed() ;

        return acquired_count ;
    }

    ensure<sequence_closed>(s == State::POP_CLOSING) ;
    checkin_guard.release() ;

    return acquired_count ;
}

} // end of namespace pcomn
