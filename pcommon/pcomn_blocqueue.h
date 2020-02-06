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

#include <list>

namespace pcomn {

/***************************************************************************//**
 The blocking_queue is thread-safe to put elements into and take out of from,
 and capable to block threads on attempt to take elements from an empty queue
 or to put into a full queue.

 This is a multiple-producer-multi-consumer queue with optionally specified
 maximum capacity.

 @param T The type of the stored elements.
 @param ConcurrentContainer The type of the underlying container to use to store the elements.
 @parblock
 The container must provide the following functions:

    - push(T): return value ignored
    - push_many(Iterator begin, Iterator end): return value ignored
    - pop(): return value must be ConvertibleTo(T)
    - pop_many(): return value must be SequenceContainer(T)
    - change_capacity(unsigned): return value ignored

 push(), push_many(), pop(), pop_many() must be thread-safe with respect both
 to each other, to change_capacity(), _and_ to itself (so that e.g. pop() must
 be safe called from different threads concurrently).

 change_capacity() must be thread-safe wrt all other functions but needn't be
 thread-safe to itself.
 @endparblock
*******************************************************************************/
template<typename T, typename ConcurrentContainer>
class blocking_queue {
    typedef ConcurrentContainer container_type ;

    template<typename R, typename P>
    using duration = std::chrono::duration<R, P> ;

    template<typename C, typename D>
    using time_point = std::chrono::time_point<C, D> ;

public:
    typedef T                           value_type ;
    typedef std::list<value_type>       value_list ;
    typedef fwd::optional<value_type>   optional_value ;

    /// Create a blocking queue with specified maximum capacity.
    explicit blocking_queue(unsigned capacity) ;

    /***************************************************************************
     push
    ***************************************************************************/
    void push(const value_type &value) ;
    void push(value_type &&value) ;

    template<typename InputIterator>
    InputIterator push_some(InputIterator b, InputIterator e) ;

    bool try_push(const value_type &value) ;
    bool try_push(value_type &&value) ;

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

    template<typename InputIterator, typename R, typename P>
    InputIterator try_push_some_for(InputIterator b, InputIterator e,
                                    const duration<R, P> &rel_time) ;

    template<typename InputIterator, typename Clock, typename Duration>
    InputIterator try_push_some_until(InputIterator b, InputIterator e,
                                      const time_point<Clock, Duration> &abs_time) ;

    /***************************************************************************
     pop
    ***************************************************************************/
    value_type pop() ;
    value_list pop_some(unsigned count) ;

    optional_value try_pop() ;

    template<typename R, typename P>
    optional_value try_pop_for(const duration<R, P> &rel_time) ;

    template<typename Clock, typename Duration>
    optional_value try_pop_until(const time_point<Clock, Duration> &abs_time) ;

    template<typename R, typename P>
    value_list try_pop_some_for(const duration<R, P> &rel_time) ;

    template<typename Clock, typename Duration>
    value_list try_pop_some_until(const time_point<Clock, Duration> &abs_time) ;

    /***************************************************************************
     capacity
    ***************************************************************************/
    size_t capacity() const ;

    size_t remaining_capacity() const ;

    size_t size() const ;

    bool empty() const ;

    void change_capacity(unsigned new_capacity) ;

private:
    std::atomic<int32_t> _capacity ;

    alignas(cacheline_t) counting_semaphore _empty_slots ;
    alignas(cacheline_t) counting_semaphore _full_slots ;

    container_type _data ;
} ;

/*******************************************************************************

*******************************************************************************/
namespace detail {
template<typename T>
class list_cbqueue {
} ;

/*******************************************************************************

*******************************************************************************/
template<typename T>
class ring_cbqueue {
} ;
} // end of namespace pcomn::detail


} // end of namespace pcomn

#endif /* __PCOMN_BLOCQUEUE_H */
