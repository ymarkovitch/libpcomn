/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
#ifndef __PCOMN_UNITTEST_MT_H
#define __PCOMN_UNITTEST_MT_H
/*******************************************************************************
 FILE         :   pcomn_unittest_mt.h
 COPYRIGHT    :   Yakov Markovitch, 2014-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Helpers for multi-threaded testing with CPPUnit, googlemock, etc.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   6 Feb 2014
*******************************************************************************/
#ifndef CPPUNIT_USE_SYNC_LOGSTREAM

#ifdef CPPUNIT_LOGSTREAM
#error Ensure pcomn_unittest_mt.h is #included first in the unit test source file.
#endif /* CPPUNIT_LOGSTREAM */

#define CPPUNIT_USE_SYNC_LOGSTREAM
#endif

#include "pcomn_unittest.h"

#include <thread>
#include <mutex>
#include <future>
#include <atomic>
#include <list>
#include <deque>
#include <vector>

#include <pthread.h>

namespace pcomn {
namespace unit {

/*******************************************************************************
 watchdog
*******************************************************************************/
class watchdog {
   public:
    explicit watchdog(std::chrono::milliseconds timeout) :
         _timeout(timeout)
      {}

      __noinline void arm()
      {
         CPPUNIT_ASSERT(!_watchdog.joinable()) ;

         _armed.lock() ;

         _watchdog = std::thread([&]{
            if (!_armed.try_lock_for(_timeout))
            {
               CPPUNIT_LOG_LINE("ERROR: THE TEST DEADLOCKED") ;
               exit(3) ;
            }
         }) ;
      }

      __noinline void disarm()
      {
         CPPUNIT_ASSERT(_watchdog.joinable()) ;

         _armed.unlock() ;
         _watchdog.join() ;
         _watchdog = {} ;
      }

   private:
      std::chrono::milliseconds  _timeout ;
      std::timed_mutex           _armed ;
      std::thread                _watchdog ;
} ;

/***************************************************************************//**
 Primitive queue to "feed" test worker; if the queue is empty, the consumer
 gets T()
*******************************************************************************/
template<typename T>
class consumer_feeder {
   public:
      consumer_feeder() = default ;
      consumer_feeder(consumer_feeder &&source) :
         _queue(std::move(source._queue))
      {}

      void push_back(const T &value) { (std::unique_lock<std::mutex>(_mutex), _queue.push_back(value)) ; }
      void push_back(T &&value) { (std::unique_lock<std::mutex>(_mutex), _queue.push_back(std::move(value))) ; }

    T pop_front() ;

   private:
      std::mutex   _mutex ;
      std::list<T> _queue ;
} ;

template<typename T>
T consumer_feeder<T>::pop_front()
{
    std::unique_lock<std::mutex> lock (_mutex) ;
    if (_queue.empty())
    {
        lock.unlock() ;
        return T() ;
    }
    T result (std::move(_queue.front())) ;
    _queue.pop_front() ;
    return result ;
}

/*******************************************************************************

*******************************************************************************/
class ThreadPack {
      PCOMN_NONCOPYABLE(ThreadPack) ;
      PCOMN_NONASSIGNABLE(ThreadPack) ;

   public:
      typedef std::function<void()> work_type ;

      explicit ThreadPack(unsigned nthreads)
      {
         PCOMN_VERIFY(nthreads) ;
         PCOMN_VERIFY(nthreads < 1024) ;
         _results.resize(nthreads) ;
         PCOMN_ENSURE_ENOERR(pthread_barrier_init(&_barrier, NULL, nthreads + 1), "pthread_barrier_init") ;

         // Here we take a (very slight) risk of exception during initialization of
         // _workers vector; this will inevitably result in destruction of joinable
         // thread(s) (all threads created here immediately stop waiting on _barrier thus
         // have no chance to finish).
         // This will cause process termination but we can tolerate this, since this
         // class is for unittests, anyway.
         for (unsigned count = nthreads ; count ; --count)
            _workers.emplace_back(*this) ;
      }

      ~ThreadPack()
      {
         cancel() ;
         pthread_barrier_destroy(&_barrier) ;
      }

      void submit_work(unsigned thread_idx, work_type &&fn)
      {
         ensure_active("submit work to") ;
         std::unique_lock<std::mutex>(_mutex) ;

         Worker &worker = _workers.at(thread_idx) ;
         std::packaged_task<void()> task (std::move(fn)) ;
         std::future<void> result (task.get_future()) ;

         worker._feeder.push_back(std::move(task)) ;
         _results[thread_idx] = std::move(result) ;
      }

      void submit_work(unsigned thread_idx, const work_type &fn)
      {
         submit_work(thread_idx, std::move(fn)) ;
      }

      void launch()
      {
         ensure_active("launch") ;
         ++_launch_count ;
         barrier_wait(_barrier) ;
         for (auto &r: _results)
            if (r.valid()) r.get() ;
      }

      bool cancel()
      {
         if (_cancelled)
            return false ;
         _cancelled = true ;
         ++_launch_count ;
         barrier_wait(_barrier) ;
         for (auto &w: _workers)
            w._thread.join() ;
         return true ;
      }

   private:
      void ensure_active(const char *action_desc) const
      {
         PCOMN_THROW_IF(_cancelled, std::logic_error, "Attempt to %s cancelled ThreadPack", action_desc) ;
      }

      static void barrier_wait(pthread_barrier_t &barrier)
      {
         const int result = pthread_barrier_wait(&barrier) ;
         PCOMN_ENSURE_ENOERR(result == PTHREAD_BARRIER_SERIAL_THREAD ? 0 : result, "pthread_barrier_wait") ;
      }

      struct Worker {
            PCOMN_NONCOPYABLE(Worker) ;
            PCOMN_NONASSIGNABLE(Worker) ;

            explicit Worker(ThreadPack &master) :
               _master(master),
               _thread(pcomn::bind_thisptr(&Worker::run, this))
            {}

            void run()
            {
               for (;;)
               {
                  barrier_wait(_master._barrier) ;
                  ++_loop_count ;
                  if (_master._cancelled && _loop_count == _master._launch_count)
                     break ;

                  auto task = _feeder.pop_front() ;
                  if (task.valid())
                     task() ;
               }
            }

            consumer_feeder<std::packaged_task<void()> > _feeder ;
            std::atomic_int                              _loop_count {0} ;
            ThreadPack &                                 _master ;
            std::thread                                  _thread ;
      } ;

   private:
      pthread_barrier_t                 _barrier ;
      bool                              _cancelled {false} ;
      std::atomic_int                   _launch_count {0} ;
      std::deque<Worker>                _workers ;  /* std::deque instead of std::vector
                                                     to avoid copy/move constructor
                                                     requirement for Worker */
      std::vector<std::future<void> >   _results ;

} ;

} // end of namespace pcomn::unit
} // end of namespace pcomn

#endif /* __PCOMN_UNITTEST_MT_H */
