/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
#ifndef __PCOMN_BLOCQUEUE_H
#define __PCOMN_BLOCQUEUE_H
/*******************************************************************************
 FILE         :   pcomn_blocqueue.h
 COPYRIGHT    :   Yakov Markovitch, 2020

 DESCRIPTION  :   Blocking concurrent queues.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   6 Feb 2020
*******************************************************************************/
/** @file
*******************************************************************************/
#include "pcomn_semaphore.h"
#include "pcomn_utils.h"
#include "pcomn_except.h"

#include <list>

namespace pcomn {

namespace detail {
template<typename> class list_cbqueue ;
template<typename> class ring_cbqueue ;
} // end of namespace pcomn::detail

/***************************************************************************//**
 Base non-template class of the MPMC blocking queue template.
*******************************************************************************/
class blocqueue_controller {
    struct alignas(cacheline_t) semaphore : counting_semaphore {
        using counting_semaphore::counting_semaphore ;
    } ;

public:
    size_t capacity() const { return _capacity.load(std::memory_order_relaxed) ; }

    /// Get the maximum allowed queue capacity.
    ///
    /// @note Circa 0.5G slots.
    constexpr static size_t max_capacity()
    {
        // Reserve 3/4 of available range to mark a queue as closed
        // (see close() source code)
        return semaphore::max_count()/4 ;
    }

    void change_capacity(unsigned new_capacity) ;

protected:
    struct SlotsKind : bool_value { using bool_value::bool_value ; } ;
    // Slot kinds
    constexpr static SlotsKind EMPTY {false} ;
    constexpr static SlotsKind FULL  {!EMPTY} ;

    enum class TimeoutKind {
        NONE,       /**< No timeout, wait until the pop end if closed */
        RELATIVE,   /**< Relative timeout (duration since steady_clock::now) */
        ABSOLUTE    /**< Absolute timeout in stready_clock */
    } ;

    enum class State : unsigned {
        OPEN,
        PUSH_CLOSED,
        POP_CLOSING,
        CLOSED
    } ;

private:
    std::mutex            _capmutex ;
    std::atomic<State>    _state {State::OPEN} ;
    std::atomic<unsigned> _capacity ;

    mutable semaphore _slots[2] ;

protected:
    explicit blocqueue_controller(unsigned capacity) ;
    virtual ~blocqueue_controller() = default ;

    counting_semaphore &slots(SlotsKind kind) noexcept { return _slots[(bool)kind] ; }

    void ensure_open() const
    {
        ensure<sequence_closed>(_state.load(std::memory_order_acquire) == State::OPEN) ;
    }

    bool close_push_end(TimeoutKind kind, std::chrono::nanoseconds d) ;
    void close_both_ends() ;

    /// @return the count of actually acquired empty slots.
    unsigned start_push(unsigned requested_count) ;
    void finalize_push(unsigned acquired_count) ;

    unsigned start_pop(unsigned requested_count) ;
    void finalize_pop(unsigned acquired_count) ;

private:
    virtual void change_data_capacity(unsigned new_capacity) = 0 ;

    static unsigned validate_capacity(unsigned new_capacity)
    {
        if (!inrange(new_capacity, 1U, (unsigned)max_capacity()))
            invalid_capacity(new_capacity) ;
        return new_capacity ;
    }

    __noreturn __cold static void invalid_capacity(unsigned new_capacity) ;
} ;

/***************************************************************************//**
 The blocking_queue is thread-safe to put elements into and take out of from,
 and capable to block threads on attempt to take elements from an empty queue
 or to put into a full queue.

 This is a multiple-producer-multi-consumer queue with optionally specified
 maximum capacity.

 One can *close* the queue either partially (the sending end) or completely (both
 ends). close() can be called multiple times, once a closed queue remains closed.

 As the sending end of the queue is closed, the queue throws sequence_closed
 exception on any attempt to push an item. Once the queue is empty, sequence_closed
 is thrown also on any attempt to pull an item. Closing both ends is amounting to
 closing the sending end and clearing the queue at the same time.

 @param T The type of the stored elements.
 @param ConcurrentContainer The type of the underlying container to use to store the elements.
 @parblock
 The container must provide the following functions:

    - push(T): return value ignored
    - push_many(Iterator begin, Iterator end): return value ignored
    - pop(): return value must be ConvertibleTo(T)
    - pop_many(unsigned): return value must be SequenceContainer(T)
    - change_capacity(unsigned): return value ignored

 push(), push_many(), pop(), pop_many() must be thread-safe with respect both
 to each other, to change_capacity(), _and_ to itself (so that e.g. pop() must
 be safe called from different threads concurrently).

 change_capacity() must be thread-safe wrt all other functions but needn't be
 thread-safe to itself.
 @endparblock
*******************************************************************************/
template<typename T, typename ConcurrentContainer = detail::list_cbqueue<T>>
class blocking_queue : private blocqueue_controller {
    typedef blocqueue_controller ancestor ;

    typedef ConcurrentContainer container_type ;

    template<typename R, typename P>
    using duration = std::chrono::duration<R, P> ;

    template<typename C, typename D>
    using time_point = std::chrono::time_point<C, D> ;

public:
    typedef T                           value_type ;
    typedef fwd::optional<value_type>   optional_value ;

    typedef std::remove_cvref_t<decltype(std::declval<container_type>().pop_many(1U))> value_list ;

    /// Create a blocking queue with specified capacity.
    explicit blocking_queue(unsigned capacity) ;

    /// Immediately close both the push and pop ends of the queue.
    /// All the items still in the queue will be lost.
    void close() { this->close_both_ends() ; }

    /// Immediately close the push end of the queue and return.
    void close_push() { this->close_push_end({}, {}) ; }

    template<typename Duration>
    bool close_push_wait_empty(std::chrono::time_point<std::chrono::steady_clock, Duration> &abs_timeout)
    {
        return this->close_push_end(ancestor::TimeoutKind::ABSOLUTE,
                                    abs_timeout.time_since_epoch()) ;
    }

    template<typename R, typename P>
    bool close_push_wait_empty(const std::chrono::duration<R, P> &rel_timeout)
    {
        return this->close_push_end(ancestor::TimeoutKind::RELATIVE, rel_timeout) ;
    }

    /***************************************************************************
     capacity
    ***************************************************************************/
    using ancestor::capacity ;
    using ancestor::max_capacity ;
    using ancestor::change_capacity ;

    /***************************************************************************
     push
    ***************************************************************************/
    void push(const value_type &value) { put_item(value) ; }
    void push(value_type &&value) { put_item(std::move(value)) ; }

    template<typename FwdIterator>
    FwdIterator push_some(FwdIterator b, FwdIterator e) ;

    bool try_push(const value_type &value) { return try_put_item(value) ; }
    bool try_push(value_type &&value) { return try_put_item(std::move(value)) ; }

    template<typename R, typename P>
    bool try_push_for(const value_type &value,
                      const duration<R, P> &rel_time) ;

    template<typename R, typename P>
    bool try_push_for(value_type &&value,
                      const duration<R, P> &rel_time) ;

    template<typename Clock, typename Duration>
    bool try_push_until(const value_type &value,
                        const time_point<Clock, Duration> &abs_time) ;

    template<typename Clock, typename Duration>
    bool try_push_until(value_type &&value,
                        const time_point<Clock, Duration> &abs_time) ;

    template<typename FwdIterator, typename R, typename P>
    FwdIterator try_push_some_for(FwdIterator b, FwdIterator e,
                                  const duration<R, P> &rel_time) ;

    template<typename FwdIterator, typename Clock, typename Duration>
    FwdIterator try_push_some_until(FwdIterator b, FwdIterator e,
                                    const time_point<Clock, Duration> &abs_time) ;

    /***************************************************************************
     pop
    ***************************************************************************/
    value_type pop() ;
    value_list pop_some(unsigned count) ;
    value_list try_pop_some(unsigned count) ;

    optional_value try_pop() ;

    template<typename R, typename P>
    optional_value try_pop_for(const duration<R, P> &rel_time) ;

    template<typename Clock, typename Duration>
    optional_value try_pop_until(const time_point<Clock, Duration> &abs_time) ;

    template<typename R, typename P>
    value_list try_pop_some_for(const duration<R, P> &rel_time) ;

    template<typename Clock, typename Duration>
    value_list try_pop_some_until(const time_point<Clock, Duration> &abs_time) ;

private:
    container_type _data ;

private:
    void change_data_capacity(unsigned new_capacity) final
    {
        _data.change_capacity(new_capacity) ;
    }

    template<typename V>
    bool put_item(V &&value) ;

    template<typename V>
    bool try_put_item(V &&value) ;

    // General queue handler, both to push and pop items
    template<typename SlotsHandler, typename QueueHandler>
    auto handle_queue(SlotsKind to_acquire, unsigned requested_count,
                      SlotsHandler &&acquire, QueueHandler &&queue_handler)
    {
        counting_semaphore &slots_to_acquire = slots(to_acquire) ;
        counting_semaphore &slots_to_release = slots(SlotsKind(!to_acquire)) ;

        // Acquire slots (empty slots for push(), full slots for pop())
        const unsigned acquired_count =
            std::forward<SlotsHandler>(acquire)(slots_to_acquire, requested_count) ;

        const auto handle = [&]
        {
            return std::forward<QueueHandler>(queue_handler)(_data, acquired_count) ;
        } ;

        typedef decltype(handle()) result_type ;

        if (!acquired_count)
        {
            ensure_open() ;
            return result_type() ;
        }

        // Take precautions in case ensure_open() or queue handler throws exception.
        auto checkin_guard (make_finalizer([&]{ slots_to_acquire.release(acquired_count) ; })) ;

        ensure_open() ;

        result_type result (handle()) ;

        // Make sure slots remain acquired.
        checkin_guard.release() ;

        // Add new full slots for push() or empty slots for pop()
        slots_to_release.release(acquired_count) ;

        return result ;
    }
} ;

template<typename T>
using blocking_ring_queue = blocking_queue<T, detail::ring_cbqueue<T>> ;

/*******************************************************************************

*******************************************************************************/
namespace detail {
template<typename T>
class list_cbqueue {
public:
    typedef T value_type ;

    explicit list_cbqueue(unsigned /*initial capacity: ignored*/) {}

    void push(const value_type &) ;
    void push(value_type &&) ;

    template<typename FwdIterator>
    FwdIterator push_many(FwdIterator begin, FwdIterator end) ;

    value_type pop() ;
    std::list<value_type> pop_many(unsigned count) ;

    void change_capacity(unsigned /*new capacity: ignored*/) {}

private:
    std::list<value_type> _data ;
} ;

/*******************************************************************************

*******************************************************************************/
template<typename T>
class ring_cbqueue {
public:
    typedef T value_type ;

    explicit ring_cbqueue(unsigned init_capacity) ;

    void push(const value_type &) ;
    void push(value_type &&) ;

    template<typename FwdIterator>
    FwdIterator push_many(FwdIterator begin, FwdIterator end) ;

    value_type pop() ;
    std::vector<value_type> pop_many(unsigned count) ;

    void change_capacity(size_t new_capacity)
    {
        ensure_le<std::out_of_range>(new_capacity, max_capacity(),
                                     "The requested ring_cbqueue capacity is too big.") ;
    }

private:
    const size_t _capacity_mask ;

    const size_t max_capacity() const { return (~_capacity_mask) + 1 ; }
} ;
} // end of namespace pcomn::detail


/*******************************************************************************
 blocking_queue
*******************************************************************************/
template<typename T, typename C>
template<typename V>
bool blocking_queue<T,C>::put_item(V &&value)
{
    return
        handle_queue(FULL, 1,
                     [](auto &slots, unsigned) { return slots.acquire() ; },
                     [&](auto &data, unsigned)
                     {
                         data.push(std::forward<V>(value)) ;
                         return true ;
                     }) ;
}

template<typename T, typename C>
template<typename V>
bool blocking_queue<T,C>::try_put_item(V &&value)
{
    return
        handle_queue(FULL, 1,
                     [](auto &slots, unsigned) { return slots.try_acquire() ; },
                     [&](auto &data, unsigned)
                     {
                         data.push(std::forward<V>(value)) ;
                         return true ;
                     }) ;
}

template<typename T, typename C>
auto blocking_queue<T,C>::pop() -> value_type
{
    return
        handle_queue(FULL, 1,
                     [](auto &slots, unsigned) { return slots.acquire() ; },
                     [](auto &data, unsigned) { return data.pop() ; }) ;
}

template<typename T, typename C>
auto blocking_queue<T,C>::try_pop() -> optional_value
{
    return
        handle_queue(FULL, 1,
                     [](auto &slots, unsigned) { return slots.try_acquire() ; },
                     [](auto &data, unsigned) { return optional_value(data.pop()) ; }) ;
}

} // end of namespace pcomn

#endif /* __PCOMN_BLOCQUEUE_H */
