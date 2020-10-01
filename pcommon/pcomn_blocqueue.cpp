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

PCOMN_STATIC_CHECK(2*blocqueue_controller::max_allowed_capacity() < (size_t)counting_semaphore::max_count()) ;

/*******************************************************************************
 blocqueue_controller
*******************************************************************************/
constexpr blocqueue_controller::SlotsKind blocqueue_controller::EMPTY ;
constexpr blocqueue_controller::SlotsKind blocqueue_controller::FULL ;

blocqueue_controller::blocqueue_controller(unsigned capacity) :
    _capacity(validate_capacity(capacity))
{
    slots(EMPTY).release(capacity) ;
}

__noreturn __cold
void blocqueue_controller::invalid_capacity(unsigned new_capacity, unsigned maxcap)
{
    throwf<std::out_of_range>
        ("Invalid capactity %u specified for blocking_queue, expected value between %u and %u",
         new_capacity, 1U, maxcap) ;
}

__noreturn __cold
void blocqueue_controller::invalid_acquire_count(unsigned count, const char *queue_end)
{
    throwf<std::out_of_range>
        ("Too big count %u is specified for %s operation on blocking_queue, maximum allowed is %u",
         count, queue_end, (unsigned)max_allowed_capacity()) ;
}

void blocqueue_controller::change_capacity(unsigned new_capacity)
{
    validate_capacity(new_capacity) ;

    PCOMN_SCOPE_LOCK(caplock, _capmutex) ;

    ensure_state_at_most(State::OPEN) ;

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

bool blocqueue_controller::try_wait_empty_finalize_queue(TimeoutKind kind,
                                                         std::chrono::nanoseconds timeout) noexcept
{
    const unsigned quiscent_state_slots_count = max_empty_slots() ;

    // max_empty_slots() can be available at the EMPTY end _only_ in the "quiscent"
    // state, when there are zero FULL slots and there are nobody between semaphores.
    if (!slots(EMPTY).universal_acquire(quiscent_state_slots_count, timeout_mode(kind), timeout))
        return false ;

    State s = State::FINALIZING ;

    // Ensure slots(FULL) released only once, by exactly the caller that has managed
    // to transfer the _state from FINALIZING to CLOSED.
    if (_state.compare_exchange_strong(s, State::CLOSED))
    {
        slots(EMPTY).release(quiscent_state_slots_count) ;
        slots(FULL).release(max_allowed_capacity()) ;
        return true ;
    }

    PCOMN_VERIFY(s == State::CLOSED) ;

    slots(EMPTY).release(quiscent_state_slots_count) ;
    return true ;
}

void blocqueue_controller::close_both_ends()
{
    PCOMN_SCOPE_LOCK(caplock, _capmutex) ;

    if (close_push_end(TimeoutKind::RELATIVE, {}))
        return ;

    State s = _state.load(std::memory_order_acquire) ;

    while (s != State::CLOSED)
    {
        if (_state.compare_exchange_strong(s, State::CLOSED))
        {
            slots(FULL).release(max_allowed_capacity()) ;
            break ;
        }
    }
}

bool blocqueue_controller::close_push_end(TimeoutKind kind, std::chrono::nanoseconds timeout)
{
    if (_state.load(std::memory_order_acquire) == State::CLOSED)
        return true ;

    PCOMN_SCOPE_LOCK(caplock, _capmutex) ;

    State s = _state.load(std::memory_order_acquire) ;

    if (s == State::CLOSED)
        return true ;

    if (s == State::OPEN)
    {
        _state.store(s = State::FINALIZING, std::memory_order_release) ;

        // Allow any pushing thread: as _state is now PUSH_CLOSED, any push() caller
        // (AKA producer) will freely acquire(n) only to immediately notice that
        // the producing end is already closed, release(n), and to throw sequence_closed
        // (see start_pop()).
        //
        // A practical analogue of resizing by expanding by max_allowed_capacity():
        // the slots(EMPTY) count when the queue is empty becomes
        // capacity() + max_allowed_capacity().

        slots(EMPTY).release(max_allowed_capacity()) ;
    }

    PCOMN_VERIFY(s == State::FINALIZING) ;

    return try_wait_empty_finalize_queue(kind, timeout) ;
}

int blocqueue_controller::start_pop(unsigned maxcount,
                                    TimeoutKind kind, std::chrono::nanoseconds timeout,
                                    RaiseError raise_on_closed)
{
    ensure<std::out_of_range>(maxcount, "Zero count is not valid for pop_some()/try_pop_some() operations.") ;
    maxcount = std::min(size_t(maxcount), max_allowed_capacity()) ;

    // pop() works both in State::OPEN and State::FINALIZING
    if (!ensure_state_at_most(State::FINALIZING, raise_on_closed))
        return -1 ;

    const unsigned acquired_count =
        slots(FULL).universal_acquire_some(maxcount, timeout_mode(kind), timeout) ;

    auto checkin_guard (make_finalizer([&]{ slots(FULL).release(acquired_count) ; })) ;

    if (!ensure_state_at_most(State::FINALIZING, raise_on_closed))
        return -1 ;

    checkin_guard.release() ;

    return acquired_count ;
}

bool blocqueue_controller::finalize_pop(unsigned acquired_count)
{
    NOXCHECK(acquired_count <= max_allowed_capacity()) ;

    // Release popped slots
    slots(EMPTY).release(acquired_count) ;

    return
        _state.load(std::memory_order_acquire) == State::FINALIZING &&
        // Attempt to finalize the queue
        try_wait_empty_finalize_queue() ;
}

__noreturn void blocqueue_controller::raise_closed()
{
    throw sequence_closed() ;
}

} // end of namespace pcomn
