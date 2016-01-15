/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_SYNCQUEUE_H
#define __PCOMN_SYNCQUEUE_H
/*******************************************************************************
 FILE         :   pcomn_syncqueue.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   A synchronized queue.

 CREATION DATE:   17 Nov 2000
*******************************************************************************/
/** @file
  A synchronized queue template class.

The synchronized queue supports the producer/consumer pattern: arbitrary number of
producer threads can simultaneously push values into the queue, and arbitrary number of
consumer threads can pop values at the same time.
As the queue is empty it blocks the consumer(s); as it is full it blocks the producer(s).
*******************************************************************************/
#include <pcomn_synccomplex.h>
#include <list>
#include <queue>
#include <limits>

namespace pcomn {

/*******************************************************************************/
/** Syncronized producer-consumer queue.
 @par
 Allows for arbitrary number of producers to put items concurrently into the queue,
 while arbitrary number of consumers retrieve items from the other end of the queue.
 @par
 The queue has specified capacity: when the queue is empty, all arriving consumers
 block  until at least one item is pushed into the queue; when it is full, all arriving
 producers block  until at least one item is popped from the queue.
 @par
 While the initial capacity is specified in the constructor, it can be safely
 changed at any moment calling synchronized_queue::capacity, which is completely
 thread-safe.
*******************************************************************************/
template<typename T, class Container = std::queue<T, std::list<T> > >
class synchronized_queue {
   public:
      typedef T value_type ;

      explicit synchronized_queue(int maxitems = 1) :
         _pc_lock(maxitems < 0 ? std::numeric_limits<int>::max() - 1 : maxitems)
      {}

      bool empty () const { return !size() ; }
      size_t size() const
      {
         std::lock_guard<std::recursive_mutex> guard (_lock) ;
         return _container.size() ;
      }

      int capacity() const { return _pc_lock.capacity() ; }
      int capacity(int new_capacity) { return _pc_lock.capacity(new_capacity) ; }

      void push(const value_type &value)
      {
         ProducerGuard producer (_pc_lock) ;
         std::lock_guard<std::recursive_mutex> guard (_lock) ;
         _container.push(value) ;
         producer.produce() ;
      }

      value_type pop()
      {
         ConsumerGuard consumer (_pc_lock) ;
         std::lock_guard<std::recursive_mutex> guard (_lock) ;
         value_type result (_container.front()) ;
         _container.pop() ;
         consumer.consume() ;
         return result ;
      }

      void close() { _pc_lock.close_producer() ; }

      void terminate()
      {
         if (_pc_lock.close())
         {
            Container dummy ;
            pcomn_swap(_container, dummy) ;
         }
      }

   private:
      ProducerConsumerLock    _pc_lock ;
      mutable std::recursive_mutex _lock ;
      Container               _container ;
} ;

} // end of namespace pcomn

#endif /* __PCOMN_SYNCQUEUE_H */
