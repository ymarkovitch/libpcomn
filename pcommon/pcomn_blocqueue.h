/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
#ifndef __PCOMN_BLOCQUEUE_H
#define __PCOMN_BLOCQUEUE_H
/*******************************************************************************
 FILE         :   pcomn_blocqueue.h
 COPYRIGHT    :   Yakov Markovitch, 2020

 DESCRIPTION  :   Bounded blocking concurrent queues.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   6 Feb 2020
*******************************************************************************/
/** @file
 Thread-safe multi-producer multi-consumer bounded queue capable to block threads on
 attempt to take elements from an empty queue or to put into a full queue.
*******************************************************************************/
#include "pcomn_semaphore.h"
#include "pcomn_utils.h"
#include "pcomn_except.h"
#include "pcomn_bitops.h"
#include "pcomn_safeptr.h"

#include <list>
#include <new>

namespace pcomn {

namespace detail {
template<typename> class list_cbqueue ;
template<typename> class ring_cbqueue ;
} // end of namespace pcomn::detail

/*******************************************************************************
 Forward declaration
*******************************************************************************/
template<typename T, typename ConcurrentContainer = detail::list_cbqueue<T>>
class blocking_queue ;

/***************************************************************************//**
 Template aliases for the blocking_queue vith various underlying containers.

 blocking_queue<T>: the underlying container is based on std::list and implies
                    (very short) locking.

 blocking_list_queue<T>: alias of blocking_queue<T>.

 blocking_ring_queue<T>: the underlying container is a ring queue with locking
                         (still extremely fast) push and lock-free pop.
*******************************************************************************/
/**@{*/

template<typename T>
using blocking_list_queue = blocking_queue<T> ;

template<typename T>
using blocking_ring_queue = blocking_queue<T, detail::ring_cbqueue<T>> ;

/**@}*/

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
    /// This value is the absolute maximum the queue can support, even if underlying
    /// container suports larger max_size().
    ///
    /// @note Circa 1G slots.
    constexpr static size_t max_allowed_capacity()
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
    struct SlotsKind : bool_value<SlotsKind> { using bool_value::bool_value ; } ;
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
    template<typename R, typename P>
    using duration = std::chrono::duration<R, P> ;

    template<typename C, typename D>
    using time_point = std::chrono::time_point<C, D> ;

protected:
    explicit blocqueue_controller(unsigned capacity) ;
    virtual ~blocqueue_controller() = default ;

    counting_semaphore &slots(SlotsKind kind) const noexcept { return _slots[(bool)kind] ; }

    bool ensure_state_at_most(State max_allowed_state, RaiseError raise_on_closed = RAISE_ERROR) const
    {
        const bool state_ok = _state.load(std::memory_order_acquire) <= max_allowed_state ;
        if (!state_ok && raise_on_closed)
            raise_closed() ;
        return state_ok ;
    }

    bool close_push_end(TimeoutKind kind, std::chrono::nanoseconds d) ;
    void close_both_ends() ;

    /// @return the count of actually acquired empty slots.
    int start_pop(unsigned requested_count,
                  TimeoutKind, std::chrono::nanoseconds,
                  RaiseError raise_on_closed) ;

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
        if (unlikely(!inrange(new_capacity, 1U, (unsigned)max_allowed_capacity())))
            invalid_capacity(new_capacity) ;
        return new_capacity ;
    }

    static unsigned validate_acquire_count(unsigned count, const char *queue_end)
    {
        if (unlikely(count > max_allowed_capacity()))
            invalid_acquire_count(count, queue_end) ;
        return count ;
    }

    __noreturn __cold
    static void invalid_capacity(unsigned capacity, unsigned maxcap = max_allowed_capacity()) ;

private:
    virtual void change_data_capacity(unsigned new_capacity) = 0 ;

    unsigned max_empty_slots() const noexcept
    {
        return capacity() + blocqueue_controller::max_allowed_capacity() ;
    }

    __noreturn __cold static void invalid_acquire_count(unsigned count,
                                                        const char *queue_end) ;
    // Note: no __cold
    __noreturn static void raise_closed() ;
} ;

/***************************************************************************//**
 Thread-safe multi-producer multi-consumer bounded queue, capable to block threads
 on taking elements from an empty queue or putting into a full queue.

 This is a multiple-producer-multi-consumer queue with optionally specified
 maximum capacity.

 One can *close* the queue either partially (the sending end) or completely (both
 ends). close() can be called multiple times, once a closed queue remains closed.

 As the pushing end of the queue is closed, the queue throws sequence_closed
 exception on any attempt to push an item. Once after that the queue is empty,
 _any_ popping function except for pop_opt() and pop_opt_some() will throw
 pcomn::sequence_closed. Note that this also applies to all try_... functions,
 so e.g. try_pop() for a _closed_ queue will not return false but will throw
 pcomn::sequence_closed instead.

 Closing both ends is amounting to closing the sending end and clearing the queue at the
 same time.

 @param T The type of the stored elements.
 @param ConcurrentContainer The type of the underlying container to use to store the elements.
 @parblock
 The container must provide the following functions:

    - push(T): return value ignored
    - emplace(A...): return value ignored
    - pop(): return value must be ConvertibleTo(T)
    - pop_many(unsigned): return value must be SequenceContainer(T)
    - change_capacity(unsigned): return value ignored

 push(), emplace(), pop(), pop_many() must be thread-safe with respect to each other,
 and to change_capacity(), _and_ to itself (so that e.g. pop() shall be safely
 called from different threads concurrently).

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
    typedef std::remove_cvref_t<decltype(std::declval<container_type>().pop_many(1U))> value_list ;
    typedef fwd::optional<value_type>   optional_value ;

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

    /***************************************************************************
     capacity
    ***************************************************************************/
    using ancestor::capacity ;
    using ancestor::change_capacity ;
    using ancestor::size ;

    size_t max_capacity() const
    {
        return std::min<size_t>(max_data_capacity<container_type>(0),
                                ancestor::max_allowed_capacity()) ;
    }

    /***************************************************************************
     close
    ***************************************************************************/
    /// Immediately close both the push and pop ends of the queue.
    /// All the items still in the queue will be lost.
    void close() { this->close_both_ends() ; }

    /// Immediately close the pushing end of the queue and return.
    ///
    /// @return true if it is happens so that the queue is empty and close_push() also
    ///   managed to close the pop end of the queue.
    ///
    /// @note This call is equivalent to close_push_wait_empty(0ns).
    ///
    bool close_push() { return this->close_push_end(TimeoutKind::RELATIVE, {}) ; }

    /// Close the pushing end of the queue and block until specified `abs_time` has been
    /// reached or the queue is empty, whichever comes first.
    ///
    /// @return true if the queue is empty, false if the timeout is reached.
    ///
    template<typename Duration>
    bool close_push_wait_empty(std::chrono::time_point<std::chrono::steady_clock, Duration> &abs_timeout)
    {
        return this->close_push_end(TimeoutKind::ABSOLUTE, abs_timeout.time_since_epoch()) ;
    }

    /// Close the pushing end of the queue and block block until specified
    /// `timeout_duration` has elapsed or the queue becomes empty, whichever
    /// comes first.
    ///
    /// @return true if the queue is empty, false if the timeout is reached.
    ///
    template<typename R, typename P>
    bool close_push_wait_empty(const duration<R, P> &timeout_duration)
    {
        return this->close_push_end(TimeoutKind::RELATIVE, timeout_duration) ;
    }

    /***************************************************************************
     push
    ***************************************************************************/
    void push(const value_type &value) { put_item({}, {}, value) ; }
    void push(value_type &&value) { put_item({}, {}, std::move(value)) ; }

    template<typename... Args>
    void emplace(Args &&...a)  { emplace_item({}, {}, std::forward<Args>(a)...) ; }

    bool try_push(const value_type &value) { return put_item(TimeoutKind::RELATIVE, {}, value) ; }
    bool try_push(value_type &&value) { return put_item(TimeoutKind::RELATIVE, {}, std::move(value)) ; }

    template<typename R, typename P>
    bool try_push_for(const value_type &value, const duration<R, P> &rel_time)
    {
        return put_item(timeout_kind(rel_time), rel_time, value) ;
    }

    template<typename R, typename P>
    bool try_push_for(value_type &&value, const duration<R, P> &rel_time)
    {
        return put_item(timeout_kind(rel_time), rel_time, std::move(value)) ;
    }

    template<typename Clock, typename Duration>
    bool try_push_until(const value_type &value, const time_point<Clock, Duration> &abs_time)
    {
        return put_item(timeout_kind(abs_time), abs_time.time_since_epoch(), value) ;
    }

    template<typename Clock, typename Duration>
    bool try_push_until(value_type &&value, const time_point<Clock, Duration> &abs_time)
    {
        return put_item(timeout_kind(abs_time), abs_time.time_since_epoch(), std::move(value)) ;
    }

    /***************************************************************************
     pop
    ***************************************************************************/
    /// Atomically remove and return an item from the front of the queue.
    ///
    /// Blocks if the queue is empty until either an item is pushed into the queue
    /// or the queue is closed.
    /// @throws pcomn::sequence_closed when the queue is closed.
    ///
    value_type pop() ;

    /// Atomically remove and return an item from the front of the queue.
    ///
    /// Blocks if the queue is empty until either an item is pushed into the queue
    /// or the queue is closed.
    /// In contrast to pop(), does _not_ throw exception when the queue is closed,
    /// instead returns empty optional_value().
    ///
    optional_value pop_opt() ;

    /// Pop several items at once, between 1 and (greedily) `maxcount`.
    ///
    /// Blocks if the queue is empty until either items are pushed into the queue
    /// or the queue is closed.
    ///
    /// @param maxcount The maximum count of items to pop from the queue; must be !=0, or
    ///  std::invalid_argument will be thrown.
    ///
    /// @throws pcomn::sequence_closed when the queue is closed.
    /// @throws std::invalid_argument when maxcount==0.
    ///
    /// @note To get all the items currently in the queue pass -1.
    ///
    value_list pop_some(unsigned maxcount)     { return get_some_items(maxcount, RAISE_ERROR) ; }
    value_list pop_opt_some(unsigned maxcount) { return get_some_items(maxcount, DONT_RAISE_ERROR) ; }

    optional_value try_pop() { return get_item(TimeoutKind::RELATIVE, {}) ; }

    template<typename R, typename P>
    optional_value try_pop_for(const duration<R, P> &rel_time)
    {
        return get_item(timeout_kind(rel_time), rel_time) ;
    }

    template<typename Clock, typename Duration>
    optional_value try_pop_until(const time_point<Clock, Duration> &abs_time)
    {
        return get_item(timeout_kind(abs_time), abs_time.time_since_epoch()) ;
    }

    value_list try_pop_some(unsigned maxcount)
    {
        return get_some_items(maxcount, TimeoutKind::RELATIVE, {}, RAISE_ERROR) ;
    }

    template<typename R, typename P>
    value_list try_pop_some_for(unsigned maxcount, const duration<R, P> &rel_time)
    {
        return get_some_items(maxcount, TimeoutKind::RELATIVE, rel_time, RAISE_ERROR) ;
    }

    template<typename Clock, typename Duration>
    value_list try_pop_some_until(unsigned maxcount, const time_point<Clock, Duration> &abs_time)
    {
        return get_some_items(maxcount, timeout_kind(abs_time), abs_time.time_since_epoch(), RAISE_ERROR) ;
    }

    value_list try_pop_opt_some(unsigned maxcount)
    {
        return get_some_items(maxcount, TimeoutKind::RELATIVE, {}, DONT_RAISE_ERROR) ;
    }

    template<typename R, typename P>
    value_list try_pop_opt_some_for(unsigned maxcount, const duration<R, P> &rel_time)
    {
        return get_some_items(maxcount, TimeoutKind::RELATIVE, rel_time, DONT_RAISE_ERROR) ;
    }

    template<typename Clock, typename Duration>
    value_list try_pop_opt_some_until(unsigned maxcount, const time_point<Clock, Duration> &abs_time)
    {
        return get_some_items(maxcount, timeout_kind(abs_time), abs_time.time_since_epoch(), DONT_RAISE_ERROR) ;
    }

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
    auto max_data_capacity(...) const { return max_allowed_capacity() ; }

	template<typename V>
    bool put_item(TimeoutKind kind, std::chrono::nanoseconds timeout, V &&value) ;

	template<typename... Args>
    bool emplace_item(TimeoutKind kind, std::chrono::nanoseconds timeout, Args &&...args) ;

    optional_value get_item(TimeoutKind kind, std::chrono::nanoseconds timeout) ;
    value_list get_some_items(unsigned count, RaiseError raise_on_closed) ;
    value_list get_some_items(unsigned count, TimeoutKind kind, std::chrono::nanoseconds timeout,
                              RaiseError raise_on_closed) ;

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
    auto handle_pop(RaiseError raise_on_closed,
                    unsigned requested_count, QueueHandler &&queue_handler,
                    TimeoutKind kind = {}, std::chrono::nanoseconds timeout = {})
    {
        typedef decltype(pop_from_data(1U, std::forward<QueueHandler>(queue_handler)))
            result_type ;

        const int acquired_count = start_pop(requested_count, kind, timeout, raise_on_closed) ;

        if (acquired_count <= 0)
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
    typedef std::list<value_type> value_list ;

    explicit list_cbqueue(unsigned /*initial capacity: ignored*/) {}
    explicit list_cbqueue(const unipair<unsigned> &capacities) :
        _max_size(capacities.second)
    {
        NOXCHECK(capacities.first <= capacities.second) ;
        NOXCHECK(_max_size) ;
    }

    void push(const value_type &v)
    {
        append_values(value_list(1, v)) ;
    }

    void push(value_type &&v)
    {
        emplace(std::move(v)) ;
    }

    template<typename... Args>
    void emplace(Args &&...a)
    {
        value_list vlist ;
        vlist.emplace_back(std::forward<Args>(a)...) ;
        append_values(std::move(vlist)) ;
    }

    value_type pop() { return std::move(pop_many(1).front()) ; }

    value_list pop_many(unsigned count) ;

    void change_capacity(unsigned /*new capacity: ignored*/) {}

    size_t max_size() const { return _max_size ; }

private:
    std::mutex   _data_mutex ;
    value_list   _data ;
    const size_t _max_size = _data.max_size() ;

private:
    void append_values(value_list &&values)
    {
        // list::splice throws nothing, no need for scoped lock
        _data_mutex.lock() ;
        _data.splice(_data.end(), std::move(values), values.begin(), values.end()) ;
        _data_mutex.unlock() ;
    }
} ;

/***************************************************************************//**
 Nonblocking bounded-capacity MPMC ring-buffer queue for use with blocking_queue
 template.

 @note This is not a full-featured queue: since the blocking_queue wrapper enures no
 overflow/underflow takes place it never checks whether the queue is sufficiently
 full (for pop()/pop_many()) or sufficiently empty (for push()).
*******************************************************************************/
template<typename T>
class ring_cbqueue final : private std::allocator<T> {
    typedef std::allocator<T> allocator_type ;
public:
    typedef T value_type ;

    explicit ring_cbqueue(unsigned init_capacity) :
        _capacity_mask((PCOMN_VERIFY(init_capacity), (1U << bitop::log2ceil(init_capacity)) - 1))
    {}

    explicit ring_cbqueue(const unipair<unsigned> &capacities) :
        ring_cbqueue((PCOMN_VERIFY(capacities.second >= capacities.first), capacities.second))
    {}

    ~ring_cbqueue()
    {
        uint64_t i = _deq_pos.load(std::memory_order_acquire) ;
        const uint64_t e = _enq_pos ;

        NOXCHECK(i <= e) ;
        NOXCHECK(e - i <= max_size()) ;

        while (i < e)
            destroy(item(i++)) ;

        allocator_type::deallocate(_items, max_size()) ;
    }

    void push(const value_type &v) { push_item(value_type(v)) ; }

    void push(value_type &&v) noexcept { push_item(std::move(v)) ; }

    template<typename... Args>
    void emplace(Args &&...a) { push_item(value_type(std::forward<Args>(a)...)) ; }

    value_type pop() noexcept
    {
        value_type * const v = item(_deq_pos.fetch_add(1, std::memory_order_acq_rel)) ;
        value_type result (std::move(*v)) ;

        destroy(v) ;
        return result ;
    }

    std::vector<value_type> pop_many(unsigned count) ;

    size_t max_size() const noexcept { return _capacity_mask + 1 ; }

    void change_capacity(uint64_t new_capacity)
    {
        ensure_le<std::out_of_range>(new_capacity, max_size(),
                                     "The requested ring_cbqueue capacity is too big.") ;
    }

private:
    const uint64_t      _capacity_mask ;
    value_type * const  _items = allocator_type::allocate(max_size()) ;

    alignas(cacheline_t) std::atomic<uint64_t> _deq_pos {0} ;
    std::mutex _enq_mutex ;
    uint64_t   _enq_pos {0} ;

private:
    value_type *item(uint64_t index) const noexcept
    {
        return &_items[index & _capacity_mask] ;
    }

    void push_item(value_type &&v) noexcept
    {
        _enq_mutex.lock() ;
        new (item(_enq_pos++)) value_type(std::move(v)) ;
        _enq_mutex.unlock() ;
    }

} ;
} // end of namespace pcomn::detail


/*******************************************************************************
 blocking_queue
*******************************************************************************/
template<typename T, typename C>
template<typename V>
bool blocking_queue<T,C>::put_item(TimeoutKind kind, std::chrono::nanoseconds timeout, V &&value)
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
template<typename... Args>
bool blocking_queue<T,C>::emplace_item(TimeoutKind kind, std::chrono::nanoseconds timeout, Args &&...args)
{
    return
        handle_push(1,
                    [&](auto &data, unsigned)
                    {
                        data.emplace(std::forward<Args>(args)...) ;
                        return true ;
                    },
                    kind, timeout) ;
}

template<typename T, typename C>
auto blocking_queue<T,C>::pop() -> value_type
{
    return
        handle_pop(RAISE_ERROR, 1, [](auto &data, unsigned)
        {
            return data.pop() ;
        }) ;
}

template<typename T, typename C>
auto blocking_queue<T,C>::pop_opt() -> optional_value
{
    return
        handle_pop(DONT_RAISE_ERROR, 1, [](auto &data, unsigned)
        {
            return optional_value(data.pop()) ;
        }) ;
}

template<typename T, typename C>
auto blocking_queue<T,C>::get_some_items(unsigned count, RaiseError raise_on_closed) -> value_list
{
    return
        handle_pop(raise_on_closed, count, [](auto &data, unsigned acquired)
        {
            return data.pop_many(acquired) ;
        }) ;
}

template<typename T, typename C>
auto blocking_queue<T,C>::get_some_items(unsigned count,
                                         TimeoutKind kind, std::chrono::nanoseconds timeout,
                                         RaiseError raise_on_closed)  -> value_list
{
    return
        handle_pop(raise_on_closed, count, [](auto &data, unsigned acquired)
        {
            return data.pop_many(acquired) ;
        },
        kind, timeout) ;
}

template<typename T, typename C>
auto blocking_queue<T,C>::get_item(TimeoutKind kind, std::chrono::nanoseconds timeout) -> optional_value
{
    return
        handle_pop(RAISE_ERROR,
                   1, [](auto &data, unsigned) { return optional_value(data.pop()) ; },
                   kind, timeout) ;
}

/*******************************************************************************
 detail::list_cbqueue
*******************************************************************************/
template<typename T>
auto detail::list_cbqueue<T>::pop_many(unsigned count) -> value_list
{
    value_list result ;

    // No allocations, no throws, no need for scoped lock
    _data_mutex.lock() ;

    const size_t data_size = _data.size() ;
    NOXCHECK(count <= data_size) ;

    if (count == data_size)
        result = std::move(_data) ;

    else
    {
        size_t tail_count = data_size - count ;
        decltype(_data.begin()) range_end ;

        // Minimize list traversing steps count
        if (count <= tail_count)
            std::advance(range_end = _data.begin(), count) ;
        else
            for (range_end = _data.end() ; tail_count-- ; --range_end) ;

        // Splice from the front (two pointer assignments, that's all!)
        result.splice(result.end(), _data, _data.begin(), range_end) ;
    }

    _data_mutex.unlock() ;

    return result ;
}

/*******************************************************************************
 detail::ring_cbqueue
*******************************************************************************/
template<typename T>
auto detail::ring_cbqueue<T>::pop_many(unsigned count) -> std::vector<value_type>
{
    NOXCHECK(count) ;

    std::vector<value_type> result ;
    result.reserve(count) ;

    const uint64_t start_index = _deq_pos.fetch_add(count, std::memory_order_acq_rel) ;
    const uint64_t end_index = start_index + count ;

    for (uint64_t i = start_index ; i < end_index ; ++i)
    {
        value_type * const v = item(i) ;
        result.emplace_back(std::move(*v)) ;
        destroy(v) ;
    }
    return result ;
}

} // end of namespace pcomn

#endif /* __PCOMN_BLOCQUEUE_H */
