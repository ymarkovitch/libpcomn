/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_THREADPOOL_H
#define __PCOMN_THREADPOOL_H
/*******************************************************************************
 FILE         :   pcomn_threadpool.h
 COPYRIGHT    :   Yakov Markovitch, 2001-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Thread pool.

 CREATION DATE:   17 Jan 2001
*******************************************************************************/
#include <pcomn_thread.h>
#include <pcomn_syncqueue.h>

#include <list>
#include <memory>

namespace pcomn {

/******************************************************************************/
/** Generic thread pool.
*******************************************************************************/
class _PCOMNEXP ThreadPool {
   public:
      typedef TaskPtr task_type ;

      /// Create a thread pool object, but don't create and start worker threads.
      ///
      /// @param capacity     The maximum capacity of the pool, i.e. the (maximum allowed)
      /// sum of jobs, both running and waiting in the job queue. This is @em not the number
      /// of spawned worker threads: the number of spawned worker threads is
      /// specified in start() call (and cannot be greater than capacity.)
      explicit ThreadPool(unsigned capacity = 2048) ;
      virtual ~ThreadPool() ;

      // start()  -  put it to work.
      // Start worker_threads worker threads and set them to wait on the job queue
      // Parameters:
      //    worker_threads -  the initial number of worker threads. This value can be changed at
      //                      any moment calling resize(). Should not be greater then the value that
      //                      has been passed to this pool's constructor ('capacity').
      //    priority       -  the priority value for the worker threads. Can be changed only by restarting.
      //    stack_size     -  the initial stack size for every worker thread. 0 means default value (OS-dependent).
      //
      bool start(int worker_threads, size_t stack_size = 0, BasicThread::Priority priority = BasicThread::PtyNormal) ;

      /// Send the task into this pool's queue.
      void push(const task_type &task) ;

      /// Get current number of working threads.
      std::pair<int, int> size() const { return std::pair<int, int>(_size, _registered.size()) ; }

      int resize(int new_size) ;

      // stop()   -  wait for all busy worker thread to complete, then close all threads
      // Parameters:
      //    timeout  -  the time (in seconds) to wait for the input queue became empty.
      //                Since the "in" end of the task queue is closed immediately,
      //                this is the time to process the tasks already in the queue.
      //                If this parameter is <0 the stop is "graceful" (i.e. all
      //                remaining tasks are processed), if it is ==0 the stop is
      //                "crash" (i.e. the queue is closed AND cleared immediately).
      //
      bool stop(int timeout = -1) ;

      bool is_running() const { return !!_task_queue ; }
      bool is_stopping() const { return _stopping ; }

      BasicThread::Priority priority() const { return _worker_priority ; }

      /*************************************************************************
                           class ThreadPool::Worker
       The class of worker thread for the pool.
      *************************************************************************/
      class Worker : public BasicThread {
            friend class ThreadPool ;

            typedef BasicThread ancestor ;
         protected:
            Worker(ThreadPool &owner) :
               ancestor(JoinManually, owner._worker_stack_size, owner.priority()),
               _owner(&owner)
            {
               start(StartSuspended) ;
            }

            int exec() ;
            int run() ;

            ThreadPool &owner() { return *_owner ; }
            const task_type &task() { return _task ; }

         private:
            ThreadPool *  _owner ; /* The employer */
            task_type      _task ;  /* A task we have got (to accomplish) */

            void owner(ThreadPool &new_owner) { _owner = &new_owner ; }
            int retire() ;
      } ;
      friend class Worker ;

   private:
      typedef std::list<Worker *>                  thread_list_type ;
      typedef pcomn::synchronized_queue<task_type> task_queue_type ;

      typedef std::lock_guard<std::recursive_mutex> Guard ;
      typedef std::lock_guard<event_mutex>          StopGuard ;

      int   _capacity ; /* The maximum number of working threads allowed. */
      int   _size ;     /* The last requested size of a pool */
      int   _worker_stack_size ; /* The stack size for a worker thread */
      BasicThread::Priority  _worker_priority ;   /* The worker threads' priority */
      bool  _stopping ; /* This is set to true for the period of stopping */

      std::recursive_mutex _lock ;
      event_mutex          _stop_lock ;
      thread_list_type     _registered ;
      thread_list_type     _dismissed ;

      std::unique_ptr<task_queue_type> _task_queue ;  /* The job queue */

      // pop()   -  get the last job request item from the queue
      //
      task_type pop() ;
      task_queue_type &task_queue() { return *_task_queue ; }

      // _register() -  register a new worker in the pool
      //
      void _register(Worker *worker) ;

      // _dismiss() - get the worker thread out of the pool
      //
      void _dismiss(Worker *worker) ;
      void _balance() ;

      void dismiss(Worker *worker) ;
      bool maybe_dismiss(Worker *worker) ;
      void balance() ;

      // _get_new_worker()  -  provide new worker thread
      // This is a factory function that have to be implemented in a derived class.
      // It must return a pointer to a newly created worker thread object or NULL if it can't
      //
      virtual Worker *_get_new_worker() ;

      // _init_stop()   -  initiate pool shutdown.
      bool _init_stop() ;

      void _init_task_queue() ;
      void _fini_task_queue(int timeout) ;
      void _clear_task_queue() ;
} ;

} // end of namespace pcomn


#endif /* __PCOMN_THREADPOOL_H */
