/*-*- mode: c++; tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_SYNCCOMPLEX_H
#define __PCOMN_SYNCCOMPLEX_H
/*******************************************************************************
 FILE         :   pcomn_synccomplex.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Complex synchronization objects - Read-Write mutex,
                  Producer-Consumer, etc. (should also dining philosophers be here?)

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   22 Nov 2000
*******************************************************************************/
#include <pcomn_syncobj.h>
#include <pcomn_atomic.h>
#include <pcomn_except.h>
#include <pcomn_utils.h>
#include <pcomn_sys.h>
#include <limits>
#include <algorithm>

namespace pcomn {

/******************************************************************************/
/** Producer-consumer lock
*******************************************************************************/
template<class Locker = event_mutex>
class PTProducerConsumer {
      typedef Locker lock_type ;
      typedef std::unique_lock<lock_type> guard_type ;

   public:
      PTProducerConsumer(int capacity = 1) :
         _produce_condvar(true),
         _consume_condvar(true),
         _capacity(capacity < 0 ? 0 : capacity),
         _empty_slots(_capacity),
         _reserved_for_produce(0),
         _reserved_for_consume(0),
         _producer_closed(false),
         _consumer_closed(false)
      {}

      bool acquire_produce(int items = 1) ;
      bool release_produce(int items = 0) ;

      bool acquire_consume(int items = 1) ;
      bool release_consume(int items = 0) ;

      /// Get current producer/consumer buffer capacity
      int capacity() const
      {
         guard_type guard (_count_lock) ;
         return _capacity ;
      }

      /// Set new capacity for the buffer
      int capacity(int new_capacity)
      {
         guard_type guard (_count_lock) ;
         if (new_capacity < 0)
            new_capacity = 0 ;
         int old_capacity = _capacity ;
         int were_empty = _empty_slots ;
         _empty_slots += new_capacity - old_capacity ;
         _capacity = new_capacity ;
         if (were_empty < _reserved_for_produce && _empty_slots >= _reserved_for_produce)
            _produce_condvar.unlock() ;

         return old_capacity ;
      }

      /// Get current queue size.
      int size() const
      {
         guard_type guard (_count_lock) ;
         return _full_slots() ;
      }

      bool close_producer()
      {
         guard_type count_guard (_count_lock) ;
         return _close_producer() ;
      }

      bool close()
      {
         guard_type count_guard (_count_lock) ;
         bool result = !_producer_closed || !_consumer_closed ;
         if (result)
         {
            _producer_closed  = _consumer_closed = true ;
            _check_n_release() ;
         }
         return result ;
      }

   private:
      mutable lock_type _count_lock ;

      lock_type   _produce_mutex ;
      lock_type   _produce_condvar ;

      lock_type   _consume_mutex ;
      lock_type   _consume_condvar ;

      int         _capacity ;
      int         _empty_slots ;

      int         _reserved_for_produce ;
      int         _reserved_for_consume ;
      bool        _producer_closed ;
      bool        _consumer_closed ;

      int _full_slots() const { return _capacity - _empty_slots ; }
      int _reserve_for_produce(int items)
      {
         guard_type guard (_count_lock) ;
         NOXCHECK(!_reserved_for_produce) ;
         if (items > _capacity)
            throw std::out_of_range("The number of items to produce is greater than producer/consumer capacity") ;
         _reserved_for_produce = items ;
         _check_produce() ;
         return _empty_slots - _reserved_for_produce ;
      }

      int _reserve_for_consume(int items)
      {
         guard_type guard (_count_lock) ;
         NOXCHECK(!_reserved_for_consume) ;
         if (items > _capacity)
            throw std::out_of_range("The number of items to consume is greater than producer/consumer capacity") ;
         _reserved_for_consume = items ;
         _check_consume() ;
         return _full_slots() - _reserved_for_consume ;
      }

      void _check_produce()
      {
         if (_producer_closed)
         {
            _reserved_for_produce = 0 ;
            _throw_close() ;
         }
      }

      void _check_consume()
      {
         if (_consumer_closed || _producer_closed && _consumer_waiting())
         {
            _reserved_for_consume = 0 ;
            _throw_close() ;
         }
      }

      bool _close_producer()
      {
         if (!_producer_closed)
         {
            _producer_closed = true ;
            _check_n_release() ;
            return true ;
         }
         return false ;
      }

      bool _close_consumer()
      {
         if (!_consumer_closed)
         {
            _consumer_closed = true ;
            _check_n_release() ;
            return true ;
         }
         return false ;
      }

      void _check_n_release()
      {
         if (_producer_waiting())
            _produce_condvar.unlock() ;
         if (_consumer_waiting())
            _consume_condvar.unlock() ;
      }

      bool _producer_waiting() const { return _reserved_for_produce > _empty_slots ; }
      bool _consumer_waiting() const { return _reserved_for_consume > _full_slots() ; }

      void _throw_close() ;

      PCOMN_NONCOPYABLE(PTProducerConsumer) ;
      PCOMN_NONASSIGNABLE(PTProducerConsumer) ;
} ;


template<class Locker>
bool PTProducerConsumer<Locker>::acquire_produce(int items)
{
   guard_type guard (_produce_mutex) ;
   try
   {
      if (_reserve_for_produce(items) < 0)
      {
         _produce_condvar.lock() ;
         _check_produce() ;
      }

      // Release lock from guard (for we don't want guard to unlock _produce_mutex -
      // it is to be unlocked by ProducerConsumerLock::unlock())
      guard.release() ;
   }
   catch(const pcomn::object_closed &)
   {
      return false ;
   }
   return true ;
}

template<class Locker>
bool PTProducerConsumer<Locker>::release_produce(int items)
{
   items = pcomn::midval(0, _reserved_for_produce, items) ;
   {
      guard_type count_guard (_count_lock) ;
      int were_full = _full_slots() ;
      _empty_slots -= _reserved_for_produce - items ;
      _reserved_for_produce = 0 ;
      if (were_full < _reserved_for_consume && _full_slots() >= _reserved_for_consume)
         _consume_condvar.unlock() ;
   }
   _produce_mutex.unlock() ;
   return true ;
}

template<class Locker>
bool PTProducerConsumer<Locker>::acquire_consume(int items)
{
   guard_type guard (_consume_mutex) ;
   try
   {
      if (_reserve_for_consume(items) < 0)
      {
         _consume_condvar.lock() ;
         _check_consume() ;
      }

      // Release lock from guard (for we don't want guard to unlock _consume_mutex -
      // it is to be unlocked by ProducerConsumerLock::unlock_shared())
      guard.release() ;
   }
   catch(const pcomn::object_closed &)
   {
      return false ;
   }
   return true ;
}

template<class Locker>
bool PTProducerConsumer<Locker>::release_consume(int items)
{
   items = pcomn::midval(0, _reserved_for_consume, items) ;
   {
      guard_type count_guard (_count_lock) ;
      int were_empty = _empty_slots ;
      _empty_slots += _reserved_for_consume - items ;
      _reserved_for_consume = 0 ;
      if (were_empty < _reserved_for_produce && _empty_slots >= _reserved_for_produce)
         _produce_condvar.unlock() ;
   }
   _consume_mutex.unlock() ;
   return true ;
}

template<class Locker>
void PTProducerConsumer<Locker>::_throw_close()
{
   throw pcomn::object_closed() ;
}

typedef PTProducerConsumer<> ProducerConsumerLock ;

/******************************************************************************/
/** Base class for specialization of read and write guards for producer-consumer lock.
 @internal
*******************************************************************************/
class ProducerConsumerGuard {
   protected:
      ProducerConsumerGuard(ProducerConsumerLock &pc, int reserve) :
         _lock(&pc),
         _reserved(std::max(reserve, 0))
      {}

      int process(int items = 1)
      {
         NOXPRECONDITION(items <= _reserved) ;
         return _reserved -= items ;
      }

      ProducerConsumerLock *_lock ;
      int                   _reserved ;
} ;

/*******************************************************************************
                     class ProducerGuard
*******************************************************************************/
class ProducerGuard : public ProducerConsumerGuard {
   public:
      ProducerGuard(ProducerConsumerLock &producer, int reserve = 1) :
         ProducerConsumerGuard(producer, reserve)
      {
         if (!_lock->acquire_produce(_reserved))
            throw object_closed() ;
      }

      ~ProducerGuard()
      {
         if (_lock)
            _lock->release_produce(_reserved) ;
      }

      int produce(int items = 1) { return process(items) ; }
} ;

/*******************************************************************************
                     class ConsumerGuard
*******************************************************************************/
class ConsumerGuard : public ProducerConsumerGuard {
   public:
      ConsumerGuard(ProducerConsumerLock &consumer, int reserve = 1) :
         ProducerConsumerGuard(consumer, reserve)
      {
         if (!_lock->acquire_consume(_reserved))
            throw object_closed() ;
      }

      ~ConsumerGuard()
      {
         if (_lock)
            _lock->release_consume(_reserved) ;
      }

      int consume(int items = 1) { return process(items) ; }
} ;

/******************************************************************************/
/** Identifier dispenser: requests range of integral numbers, then (atomically)
 allocates successive numbers from the range upon request.

 @param AtomicInt       Atomic integer type
 @param RangeProvider   Callable type; if r is RangeProvider then calling r without
 arguments must be valid and its result must be convertible to
 @p std::pair<AtomicInt,AtomicInt> (i.e. @p (std::pair<AtomicInt,AtomicInt>)r() must be
 a valid expression)

 @note range.second designates the "past-the-end" point in the range, the same way as
 STL containers/algorithms do, so for a nonempty range @p range.first<range.second
 RangeProvider @em must return @em only nonemty ranges. The starting point of a new range
 must @em follow the end of the previous range (not necessary immediately, gaps are
 allowed).
*******************************************************************************/
template<typename AtomicInt, typename RangeProvider>
class PTIdentDispenser {
      PCOMN_STATIC_CHECK(is_atomic<AtomicInt>::value) ;
   public:
      typedef AtomicInt type ;

      PTIdentDispenser(const RangeProvider &provider, type incval = 1) :
         _increment(PCOMN_ENSURE_ARG(incval)),
         _next_id(0),
         _range(0, 0),
         _provider(provider)
      {}

      type increment() const { return _increment ; }

      /// Atomically allocate new ID.
      type allocate_id()
      {
         type id ;
         do if ((id = _next_id) < _range.first || id >= _range.second)
            {
               PCOMN_SCOPE_LOCK(range_guard, _rangelock) ;

               if ((id = _next_id) < _range.first || id >= _range.second)
               {
                  const std::pair<type, type> newrange = _provider() ;
                  PCOMN_VERIFY(newrange.first >= _range.second && newrange.second > newrange.first) ;
                  _next_id = newrange.first ;
                  // Issue a memory barrier
                  atomic_op::get(&_next_id) ;

                  _range = newrange ;
               }
            }
         while (!atomic_op::cas(&_next_id, id, id + _increment)) ;

         return id ;
      }

   private:
      std::mutex              _rangelock ;
      const type              _increment ;
      type                    _next_id ;
      std::pair<type, type>   _range ;
      RangeProvider           _provider ;
} ;

} // end of namespace pcomn

#endif /* __PCOMN_SYNCCOMPLEX_H */
