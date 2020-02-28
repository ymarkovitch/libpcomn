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

/*******************************************************************************
 blocking_queue<T>: the underlying container is based on std::list and implies
                    (very short) locking.

 blocking_ring_queue<T>: the underlying container is lock-free ring queue
                         (not wait-free!).
*******************************************************************************/
template<typename T, typename ConcurrentContainer = detail::list_cbqueue<T>>
class blocking_queue ;

template<typename T>
using blocking_ring_queue = blocking_queue<T, detail::ring_cbqueue<T>> ;

/***************************************************************************//**
 Base non-template class of the MPMC blocking queue template.
*******************************************************************************/
class blocqueue_controller {
    struct alignas(cacheline_t) semaphore : counting_semaphore {
        using counting_semaphore::counting_semaphore ;
    } ;

    template<typename R, typename P>
    using duration = std::chrono::duration<R, P> ;

    template<typename C, typename D>
    using time_point = std::chrono::time_point<C, D> ;

public:
    size_t capacity() const { return _capacity.load(std::memory_order_relaxed) ; }

    /// Get the maximum allowed queue capacity.
    ///
    /// @note Circa 1G slots.
    constexpr static size_t max_capacity()
    {
        // Reserve half of available range to ensure close() won't deadlock
        // (see close() source code)
        return semaphore::max_count()/2 ;
    }

    void change_capacity(unsigned new_capacity) ;

    /// Get the (approximate) current size of the queue, i.e. the count of pending
    /// (pushed but not yet popped) items in the queue.
    ///
    size_t size() const
    {
        const size_t count = slots(FULL).borrow(0) ;
        return _state.load(std::memory_order_acquire) == State::CLOSED ? 0 : count ;
    }

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
        FINALIZING,
        CLOSED
    } ;

private:
    std::recursive_mutex  _capmutex ;
    std::atomic<State>    _state {State::OPEN} ;
    std::atomic<unsigned> _capacity ;

    mutable semaphore _slots[2] ;

protected:
    explicit blocqueue_controller(unsigned capacity) ;
    virtual ~blocqueue_controller() = default ;

    counting_semaphore &slots(SlotsKind kind) const noexcept { return _slots[(bool)kind] ; }

    void ensure_state_at_most(State max_allowed_state) const
    {
        if (_state.load(std::memory_order_acquire) > max_allowed_state)
            raise_closed() ;
    }

    bool close_push_end(TimeoutKind kind, std::chrono::nanoseconds d) ;
    void close_both_ends() ;

    /// @return the count of actually acquired empty slots.
    unsigned start_pop(unsigned requested_count,
                       TimeoutKind, std::chrono::nanoseconds) ;

    bool finalize_pop(unsigned acquired_count) ;

    // Returns true if the queue becomes/is finalized.
    bool try_wait_empty_finalize_queue(TimeoutKind = TimeoutKind::RELATIVE,
                                       std::chrono::nanoseconds = {}) noexcept ;

    static constexpr TimeoutMode timeout_mode(TimeoutKind kind) noexcept
    {
        return
            kind == TimeoutKind::NONE       ? TimeoutMode::None :
            kind == TimeoutKind::RELATIVE   ? TimeoutMode::Period :
            TimeoutMode::SteadyClock ;
    }

    template<typename R, typename P>
    constexpr static TimeoutKind timeout_kind(const duration<R,P> &)
    {
        return TimeoutKind::RELATIVE ;
    }

    template<typename Clock, typename Duration>
    constexpr static TimeoutKind timeout_kind(const time_point<Clock,Duration> &)
    {
        static_assert(std::is_same_v<Clock, std::chrono::steady_clock>,
                      "Only steady_clock is supported for blocking_queue timeout specification.") ;
        return TimeoutKind::ABSOLUTE ;
    }

    static unsigned validate_capacity(unsigned new_capacity)
    {
        if (unlikely(!inrange(new_capacity, 1U, (unsigned)max_capacity())))
            invalid_capacity(new_capacity) ;
        return new_capacity ;
    }

    static unsigned validate_acquire_count(unsigned count, const char *queue_end)
    {
        if (unlikely(count > max_capacity()))
            invalid_acquire_count(count, queue_end) ;
        return count ;
    }

    __noreturn __cold
    static void invalid_capacity(unsigned capacity, unsigned maxcap = max_capacity()) ;

private:
    virtual void change_data_capacity(unsigned new_capacity) = 0 ;

    unsigned max_empty_slots() const noexcept
    {
        return capacity() + blocqueue_controller::max_capacity() ;
    }

    __noreturn __cold static void invalid_acquire_count(unsigned count,
                                                        const char *queue_end) ;
    // Note: no __cold
    __noreturn static void raise_closed() ;
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
template<typename T, typename ConcurrentContainer>
class blocking_queue : private blocqueue_controller {
    typedef blocqueue_controller ancestor ;

    typedef ConcurrentContainer container_type ;

public:
    typedef T                           value_type ;
    typedef fwd::optional<value_type>   optional_value ;

    typedef std::remove_cvref_t<decltype(std::declval<container_type>().pop_many(1U))> value_list ;

    /// Create a blocking queue with specified current capacity.
    /// @note `capacity` is also passed to underlying ConcurrentContainer constructor.
    explicit blocking_queue(unsigned capacity) :
        ancestor(capacity),
        _data(capacity)
    {}

    /// Create a blocking queue with specified current and maximum capacities.
    /// @note `capacities` is also passed to underlying ConcurrentContainer constructor.
    explicit blocking_queue(const unipair<unsigned> &capacities) :
        ancestor(ensure_le<std::invalid_argument>
                 (capacities.first, validate_capacity(capacities.second),
                  "Current capacity exceeds maximum capacity "
                  "in blocking_queue constructor arguments.")),
        _data(capacities)
    {}

    /// Immediately close both the push and pop ends of the queue.
    /// All the items still in the queue will be lost.
    void close() { this->close_both_ends() ; }

    /// Immediately close the push end of the queue and return.
    ///
    /// @return true if it is happens so that the queue is empty and close_push() also
    ///   managed to close the pop end of the queue.
    ///
    /// @note This call is equivalent to close_push_wait_empty(0ns).
    ///
    bool close_push() { return this->close_push_end(TimeoutKind::RELATIVE, {}) ; }

    template<typename Duration>
    bool close_push_wait_empty(std::chrono::time_point<std::chrono::steady_clock, Duration> &abs_timeout)
    {
        return this->close_push_end(TimeoutKind::ABSOLUTE, abs_timeout.time_since_epoch()) ;
    }

    template<typename R, typename P>
    bool close_push_wait_empty(const duration<R, P> &rel_timeout)
    {
        return this->close_push_end(TimeoutKind::RELATIVE, rel_timeout) ;
    }

    /***************************************************************************
     capacity
    ***************************************************************************/
    using ancestor::capacity ;
    using ancestor::max_capacity ;
    using ancestor::change_capacity ;
    using ancestor::size ;

    /***************************************************************************
     push
    ***************************************************************************/
    void push(const value_type &value) { put_item(value) ; }
    void push(value_type &&value) { put_item(std::move(value)) ; }

    template<typename FwdIterator>
    FwdIterator push_some(FwdIterator b, FwdIterator e) ;

    bool try_push(const value_type &value) { return put_item(value, TimeoutKind::RELATIVE, {}) ; }
    bool try_push(value_type &&value) { return put_item(std::move(value), TimeoutKind::RELATIVE, {}) ; }

    template<typename R, typename P>
    bool try_push_for(const value_type &value, const duration<R, P> &rel_time)
    {
        return put_item(value, timeout_kind(rel_time), rel_time) ;
    }

    template<typename R, typename P>
    bool try_push_for(value_type &&value, const duration<R, P> &rel_time)
    {
        return put_item(std::move(value), timeout_kind(rel_time), rel_time) ;
    }

    template<typename Clock, typename Duration>
    bool try_push_until(const value_type &value, const time_point<Clock, Duration> &abs_time)
    {
        return put_item(value, timeout_kind(abs_time), abs_time.time_since_epoch()) ;
    }

    template<typename Clock, typename Duration>
    bool try_push_until(value_type &&value, const time_point<Clock, Duration> &abs_time)
    {
        return put_item(std::move(value), timeout_kind(abs_time), abs_time.time_since_epoch()) ;
    }

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

    optional_value try_pop() { return try_get_item(TimeoutKind::RELATIVE, {}) ; }

    template<typename R, typename P>
    optional_value try_pop_for(const duration<R, P> &rel_time)
    {
        return try_get_item(timeout_kind(rel_time), rel_time) ;
    }

    template<typename Clock, typename Duration>
    optional_value try_pop_until(const time_point<Clock, Duration> &abs_time)
    {
        return try_get_item(timeout_kind(abs_time), abs_time.time_since_epoch()) ;
    }

    template<typename R, typename P>
    value_list try_pop_some_for(const duration<R, P> &rel_time) ;

    template<typename Clock, typename Duration>
    value_list try_pop_some_until(const time_point<Clock, Duration> &abs_time) ;

private:
    container_type _data ;

private:
    void change_data_capacity(unsigned new_capacity) final
    {
        const size_t maxcap = max_data_capacity<container_type>(0) ;

        if (unlikely(!inrange<size_t>(new_capacity, 1, maxcap)))
            invalid_capacity(new_capacity, maxcap) ;

        _data.change_capacity(new_capacity) ;
    }

	template<typename U>
    decltype(std::declval<U>().max_size())
    max_data_capacity(int) const { return _data.max_size() ; }

	template<typename U>
    auto max_data_capacity(...) const { return max_capacity() ; }

	template<typename V>
    bool put_item(V &&value, TimeoutKind kind = {}, std::chrono::nanoseconds timeout = {}) ;

    optional_value try_get_item(TimeoutKind kind, std::chrono::nanoseconds timeout) ;

    // Push handler
    template<typename QueueHandler>
    auto handle_push(unsigned requested_count, QueueHandler &&queue_handler,
                     TimeoutKind kind = {}, std::chrono::nanoseconds timeout = {})
    {
        validate_acquire_count(requested_count, "push") ;

        ensure_state_at_most(State::OPEN) ;

        // Acquire slots empty slots for push()
        const unsigned acquired_count =
            slots(EMPTY).universal_acquire(requested_count, timeout_mode(kind), timeout) ;

        // Take precautions in case ensure_state_at_most() or queue handler throws exception.
        auto rollback_guard (make_finalizer([&]
        {
            // Return the already acquired empty slots.
            slots(EMPTY).release(acquired_count) ;
            // We may be in FINALIZING state, check for the "quiscent" state: missing
            // transition to "quiscent" state during finalization may eventually lead
            // to deadlock in pop()
            try_wait_empty_finalize_queue() ;
        })) ;

        ensure_state_at_most(State::OPEN) ;

        const auto handle = [&]
        {
            return std::forward<QueueHandler>(queue_handler)(_data, acquired_count) ;
        } ;

        typedef decltype(handle()) result_type ;

        if (!acquired_count)
        {
            rollback_guard.release() ;
            return result_type() ;
        }

        result_type result (handle()) ;

        // Make sure slots remain acquired.
        rollback_guard.release() ;

        // Add new full slots for push()
        slots(FULL).release(acquired_count) ;

        return result ;
    }

    // Pop handler
    template<typename QueueHandler>
    auto handle_pop(unsigned requested_count, QueueHandler &&queue_handler,
                    TimeoutKind kind = {}, std::chrono::nanoseconds timeout = {})
    {
        typedef decltype(pop_from_data(1U, std::forward<QueueHandler>(queue_handler)))
            result_type ;

        const unsigned acquired_count = start_pop(requested_count, kind, timeout) ;

        if (!acquired_count)
            return result_type() ;

        // finalize_pop releases empty slots and, if there is a closing state, attempts
        // to finalize the queue.
        const auto finalizer (make_finalizer([&]{ finalize_pop(acquired_count) ; })) ;

        return pop_from_data(acquired_count,
                             std::forward<QueueHandler>(queue_handler)) ;
    }

    // Wrap queue_pop_handler with `noexcept`: std::terminate() is way better than the
    // deadlock.
    template<typename QueueHandler>
    auto pop_from_data(unsigned item_count, QueueHandler &&queue_pop_handler) noexcept
    {
        return std::forward<QueueHandler>(queue_pop_handler)(_data, item_count) ;
    }
} ;

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
    explicit ring_cbqueue(const unipair<unsigned> &capacities) ;

    void push(const value_type &) ;
    void push(value_type &&) ;

    template<typename FwdIterator>
    FwdIterator push_many(FwdIterator begin, FwdIterator end) ;

    value_type pop() ;
    std::vector<value_type> pop_many(unsigned count) ;

    size_t max_size() const { return (~_capacity_mask) + 1 ; }

    void change_capacity(size_t new_capacity)
    {
        ensure_le<std::out_of_range>(new_capacity, max_size(),
                                     "The requested ring_cbqueue capacity is too big.") ;
    }

private:
    const size_t _capacity_mask ;
} ;
} // end of namespace pcomn::detail


/*******************************************************************************
 blocking_queue
*******************************************************************************/
template<typename T, typename C>
template<typename V>
bool blocking_queue<T,C>::put_item(V &&value, TimeoutKind kind, std::chrono::nanoseconds timeout)
{
    return
        handle_push(1,
                    [&](auto &data, unsigned)
                    {
                        data.push(std::forward<V>(value)) ;
                        return true ;
                    },
                    kind, timeout) ;
}

template<typename T, typename C>
auto blocking_queue<T,C>::pop() -> value_type
{
    return
        handle_pop(1, [](auto &data, unsigned) { return data.pop() ; }) ;
}

template<typename T, typename C>
auto blocking_queue<T,C>::try_get_item(TimeoutKind kind, std::chrono::nanoseconds timeout)
    -> optional_value
{
    return
        handle_pop(1, [](auto &data, unsigned) { return optional_value(data.pop()) ; },
                   kind, timeout) ;
}

template<typename T, typename C>
auto blocking_queue<T,C>::pop_some(unsigned count) -> value_list
{
    return
        handle_pop(1, [](auto &data, unsigned acquired) { return data.pop_many(acquired) ; }) ;
}

} // end of namespace pcomn

#endif /* __PCOMN_BLOCQUEUE_H */
