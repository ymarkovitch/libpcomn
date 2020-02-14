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

PCOMN_STATIC_CHECK(3*blocqueue_controller::max_capacity() - 1 < counting_semaphore::max_count()) ;

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

void blocqueue_controller::change_capacity(unsigned new_capacity)
{
    ensure_range<std::out_of_range>(new_capacity, (size_t)1, max_capacity(),
                                    "Invalid capactity specified for blocking_queue") ;

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

void blocqueue_controller::close_both_ends()
{
    PCOMN_SCOPE_LOCK(caplock, _capmutex) ;

    State s = State::OPEN ;
    // CST order is intentional, don't "optimize"!
    do if (_state.compare_exchange_strong(s, State::CLOSED))
       {
           slots(EMPTY).release(blocqueue_controller::max_capacity()) ;
           slots(FULL).release(blocqueue_controller::max_capacity()) ;
           break ;
       }
    while (s != State::CLOSED) ;
}

bool blocqueue_controller::close_push_end(TimeoutKind kind, std::chrono::nanoseconds d)
{
    return true ;
}

unsigned blocqueue_controller::start_push(unsigned requested_count)
{
    const unsigned acquired_count = slots(EMPTY).acquire(requested_count) ;
    ensure_open() ;
    return acquired_count ;
}

void blocqueue_controller::finalize_push(unsigned acquired_count)
{
    slots(FULL).release(acquired_count) ;
}

unsigned blocqueue_controller::start_pop(unsigned requested_count)
{
    const unsigned acquired_count = slots(FULL).acquire_some(requested_count) ;

    State s = _state.load(std::memory_order_acquire) ;

    if (is_in(s, State::OPEN, State::POP_CLOSING))
    {
        return slots(FULL).acquire_some(requested_count) ;
    }

    conditional_throw<sequence_closed>(s == State::CLOSED) ;

    // Here s == PUSH_CLOSED
    if (!_state.compare_exchange_strong(s, State::POP_CLOSING))
    {
        conditional_throw<sequence_closed>(s != State::POP_CLOSING) ;
        return slots(FULL).acquire_some(requested_count) ;
    }

    // This thread is just changed queue state PUSH_CLOSED -> POP_CLOSING.
    // Ensure all the pushing threads can start_push and get sequence_closed:
    // "wide open the doors".
    slots(EMPTY).release(blocqueue_controller::max_capacity() - 1) ;

    // Wait until others pop all but the last remaining items.
    const unsigned doorway_width =
        _capacity.load(std::memory_order_acquire) + blocqueue_controller::max_capacity() - acquired_count ;

    slots(EMPTY).acquire(doorway_width) ;
    slots(EMPTY).release(doorway_width) ;

    s = State::POP_CLOSING ;
    ensure<sequence_closed>(_state.compare_exchange_strong(s, State::CLOSED)) ;

    return acquired_count ;
}

void blocqueue_controller::finalize_pop(unsigned acquired_count)
{
    slots(EMPTY).release(acquired_count) ;
}

} // end of namespace pcomn
