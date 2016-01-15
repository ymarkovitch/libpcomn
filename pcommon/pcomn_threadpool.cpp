/*-*- tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets: ((innamespace . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_threadpool.cpp
 COPYRIGHT    :   Yakov Markovitch, 2001-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Thread pool implementation

 CREATION DATE:   17 Jan 2001
*******************************************************************************/
#include <pcomn_threadpool.h>
#include <pcomn_utils.h>
#include <pcomn_diag.h>
#include <pcomn_function.h>
#include <pcomn_calgorithm.h>

#include <functional>
#include <algorithm>

namespace pcomn {

/*******************************************************************************
 ThreadPool
*******************************************************************************/
ThreadPool::ThreadPool(unsigned capacity) :
   _capacity((int)capacity),
   _size(0),
   _worker_stack_size(0),
   _worker_priority(BasicThread::PtyNormal),
   _stopping(false)
{}

ThreadPool::~ThreadPool()
{
   stop(0) ;
}

ThreadPool::task_type ThreadPool::pop()
{
   try {
      return task_queue().pop() ;
   }
   catch(const pcomn::object_closed &)
   {
      NOXCHECK(is_stopping()) ;
      // Process the case of the graceful shutdown
      _size = 0 ;
      throw ;
   }
}

inline void ThreadPool::_init_task_queue()
{
   _task_queue.reset(new task_queue_type(_capacity - size().first)) ;
}

void ThreadPool::_fini_task_queue(int timeout)
{
   _task_queue->close() ;
   if (timeout < 0)
      timeout = std::numeric_limits<int>::max() ;
   // Stay polling from time to time
   for (int interval = 0 ; _task_queue->size() && interval/1000 < timeout ; interval += 500)
      BasicThread::yield(500) ;
   _task_queue->capacity(0) ;
}

inline void ThreadPool::_clear_task_queue()
{
   // At this point we needn't bother about synchronization - this function is always
   // called in the 'main' thread context and to this moment all worker threads are down.
   _task_queue.reset() ;
}

bool ThreadPool::start(int init_threads, size_t stack_size, BasicThread::Priority priority)
{
   TRACEPX(PCOMN_ThreadPool, DBGL_ALWAYS, "Start request to " << this << " threads=" << init_threads) ;

   if (is_stopping())
      return false ;

   Guard guard (_lock) ;
   _worker_priority = priority ;
   _worker_stack_size = stack_size ;

   if (is_running())
   {
      TRACEPX(PCOMN_ThreadPool, DBGL_ALWAYS, "Pool " << this << " already started. Resizing...") ;
      if (size().second < init_threads)
         resize(init_threads) ;
      return false ;
   }

   NOXCHECK(_registered.empty()) ;
   if (init_threads > _capacity)
      throw std::invalid_argument("ThreadPool::start(): init_threads > maxsize") ;

   _init_task_queue() ;
   resize(init_threads) ;
   return true ;
}

void ThreadPool::push(const task_type &job)
{
   if (!is_running())
      throw pcomn::object_closed("ThreadPool") ;
   task_queue().push(job) ;
}

int ThreadPool::resize(int new_size)
{
   if (is_stopping())
      return 0 ;

   if (new_size <= 0)
      new_size = 1 ;
   if (new_size == _size)
      return new_size ;

   Guard guard (_lock) ;
   // Double-checking
   if (is_stopping() || !is_running())
      return 0 ;

   TRACEPX(PCOMN_ThreadPool, DBGL_ALWAYS, "Resizing the pool " << this << " threads=" << new_size) ;
   _size = new_size ;
   _balance() ;
   return size().second ;
}

bool ThreadPool::stop(int timeout)
{
   TRACEPX(PCOMN_ThreadPool, DBGL_ALWAYS, "Stop request to " << this) ;

   pcomn::vsaver<bool> stopping (_stopping) ;

   if (_init_stop())
   {
      // Send end-of-job signal to all working threads and finalize the queue
      // We cannot _delete_ the queue at this point because there may be still running
      // worker threads.
      _fini_task_queue(timeout) ;
      _size = 0 ;
      StopGuard stop_guard (_stop_lock) ;
      NOXCHECK(_registered.empty()) ;
      // All threads are dismissed, so inactivate this pool
      _clear_task_queue() ;
      // Wait for all threads to end
      std::for_each(_dismissed.begin(), _dismissed.end(), std::mem_fun(&BasicThread::join)) ;
      // Delete all thread objects
      pcomn::clear_icontainer(_dismissed) ;
   }

   TRACEPX(PCOMN_ThreadPool, DBGL_ALWAYS, "The pool " << this << " stopped.") ;

   return true ;
}

bool ThreadPool::_init_stop()
{
   // There is no much sense in double-checking for stop() call is a VERY rare and
   // exceptional occurence.
   Guard guard (_lock) ;
   if (!is_running() || is_stopping())
   {
      TRACEPX(PCOMN_ThreadPool, DBGL_ALWAYS, this << " is already stopped.") ;
      return false ;
   }

   TRACEPX(PCOMN_ThreadPool, DBGL_ALWAYS, "Stopping " << _size << " active threads.") ;
   // Acquire _stop_lock. The last thread will release it.
   _stop_lock.lock() ;
   _stopping = true ;
   return true ;
}

inline void ThreadPool::_register(Worker *worker)
{
   Guard guard (_lock) ;
   _registered.push_back(worker) ;

   TRACEPX(PCOMN_ThreadPool, DBGL_LOWLEV, "The thread " << *worker
           << " is registered in the pool " << this) ;
}

void ThreadPool::_dismiss(Worker *worker)
{
   TRACEPX(PCOMN_ThreadPool, DBGL_LOWLEV, "The worker  " << *worker
           << " dismissed from the pool " << this) ;

   // Unregister this thread from its pool
   _registered.remove(worker) ;
   if (is_stopping())
   {
      // If our pool is stopping, register this thread as a dismissed one
      _dismissed.push_back(worker) ;
      // This thread was the last active one, so allow the pool to wait & delete
      if (_registered.empty())
         _stop_lock.unlock() ;
   }
}

void ThreadPool::_balance()
{
   int new_workers = _size - size().second ;
   while(new_workers-- > 0)
   {
      std::unique_ptr<Worker> worker (_get_new_worker()) ;
      WARNPX(PCOMN_ThreadPool, !worker, DBGL_ALWAYS, "_get_new_worker() for the pool " << this << " returned NULL") ;
      if (!worker)
         break ;
      worker->owner(*this) ;
      worker->start() ;
      // Up to this point there could exception be thrown.
      // Now everything is settled, so release the guard
      worker.release() ;
   }
}

inline bool ThreadPool::maybe_dismiss(Worker *worker)
{
   Guard guard (_lock) ;

   // Check if there is any room in the pool for this thread
   std::pair<int, int> sz = size() ;
   if (sz.second > sz.first)
   {
      // We are not needed here...
      _dismiss(worker) ;
      return true ;
   }
   return false ;
}

inline void ThreadPool::dismiss(Worker *worker)
{
   Guard guard (_lock) ;
   _dismiss(worker) ;
}

inline void ThreadPool::balance()
{
   Guard guard (_lock) ;
   _balance() ;
}

ThreadPool::Worker *ThreadPool::_get_new_worker()
{
   return new Worker(*this) ;
}

/*******************************************************************************
 ThreadPool::Worker
*******************************************************************************/
#if defined(PCOMN_PL_WINDOWS) && defined(__BORLANDC__)
#  define THREAD_TRY __try {
#  define THREAD_EXCEPT(code) } __except(1) { code ; }
#else
#  define THREAD_TRY
#  define THREAD_EXCEPT(code)
#endif

int ThreadPool::Worker::exec()
{
   TRACEPX(PCOMN_ThreadPool, DBGL_LOWLEV, "Worker " << *this
           << " of the pool " << &owner() << " started") ;

   // Register itself in the owner's list
   owner()._register(this) ;

   THREAD_TRY ;
   try {
      // The main procesing cycle
      do {
         // Get an item to process
         _task = owner().pop() ;
         TRACEPX(PCOMN_ThreadPool, DBGL_VERBOSE, "Worker " << *this
                 << " of the pool " << &owner() << " got an item " << _task) ;
          run() ;
          // Erase the already processed item
          _task = 0 ;
      }
      while (!owner().maybe_dismiss(this)) ;
   }
   catch(...)
   {
      return retire() ;
   }
   THREAD_EXCEPT(return retire()) ;

   return 1 ;
}

int ThreadPool::Worker::run()
{
   // Ignore "empty" items - these are only intented for waking worker threads up
   return
      _task ? _task->accomplish() : 0 ;
}

int ThreadPool::Worker::retire()
{
   WARNPX(PCOMN_ThreadPool, !owner().is_stopping(), DBGL_ALWAYS,
          "Internal problems in the thread " << this << ". Retiring...") ;

   owner().dismiss(this) ;
   owner().balance() ;
   return 0 ;
}

} // end of namespace pcomn
