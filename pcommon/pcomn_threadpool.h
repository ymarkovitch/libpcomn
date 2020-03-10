/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
#ifndef __PCOMN_THREADPOOL_H
#define __PCOMN_THREADPOOL_H
/*******************************************************************************
 FILE         :   pcomn_threadpool.h
 COPYRIGHT    :   Yakov Markovitch, 2018-2020. All rights reserved.

 DESCRIPTION  :   A resizable threadpool.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   6 Nov 2018
*******************************************************************************/
#include "pcomn_pthread.h"
#include "pcomn_blocqueue.h"
#include "pcomn_meta.h"

#include <functional>
#include <atomic>
#include <memory>
#include <future>
#include <queue>
#include <list>

namespace pcomn {

/***************************************************************************//**
 Fixed-size thread pool for one-time execution of a batch of jobs.

 Unlike the usual thread pool, the object of this class does not allow adding
 jobs after the start of processing. The workflow is as follows:

  1. Create job_batch object with a specified number of threads.
  2. Form a batch of jobs, submitting _all_ tasks to be performed.
  3. Start processing either by calling either start() or wait() function.
  4. After start() or wait() no new jobs can be submitted, i.e. job_batch is "one-off".

 A job can be submitted with add_task() or add_job(). The difference is add_task()
 returns std::future<> object, thus allowing to obtain the result of execution,
 while add_job() returns void ("fire and forget").
 ******************************************************************************/
class job_batch {
    PCOMN_NONCOPYABLE(job_batch) ;
    PCOMN_NONASSIGNABLE(job_batch) ;
public:
    job_batch() ;
    explicit job_batch(unsigned threadcount) ;

    /// Wait until all the pending tasks from the queue have been completed.
    ~job_batch() ;

    /// Get the number of threads in the pool.
    size_t size() const ;

    /// Drop all the pending tasks from the task queue.
    ///
    unsigned clear_queue() ;

    /// Immediately stop the pool, drop all the pending jobs.
    /// All threads are joined and deleted.
    ///
    void stop() ;

    /// Append the callable object to the batch and get std::future<> to allow obtaining
    /// the result of execution.
    ///
    template<typename F>
    auto add_task(F &&task) ->std::future<decltype(task())>
    {
        typedef decltype(task()) task_result ;

        auto ptask = std::packaged_task<task_result()>(std::forward<F>(task)) ;
        std::future<task_result> result (ptask.get_future()) ;

        _task_queue.push(std::move(ptask)) ;

        return result ;
    }

    /// Append the callable object to the batch.
    ///
    template<typename F>
    auto add_job(F &&job) ->std::void_t<decltype(job())>
    {
        _task_queue.push(packed_job(std::packaged_task<void()>(std::forward<F>(job)))) ;
    }

private:
    class alignas(cacheline_t) packed_job final : std::function<void()> {
        typedef std::function<void()> ancestor ;
    public:
        using ancestor::ancestor ;
    } ;

private:
    std::atomic<unsigned> _idle_count {0} ;  /* Count of idle threads. */
    atomic_flag           _retire {false} ;
    atomic_flag           _stop   {false} ;

    mutable std::mutex _pool_mutex ;

    std::vector<packed_job>
    blocking_ring_queue<packed_job> _task_queue {4096} ;
    std::list<std::pair<pthread, atomic_flag_ptr>> _threads ;

private:
    void start_thread() ;
    void flush_task_queue() ;
} ;

/***************************************************************************//**
 A resizable thread pool to run any callable objects with signature `ret func()`.

 The resulting task value or exception is available through std::future<ret>.

 There are two kinds of work that can be submitted into the pool: a *task* and a *job*.
 The difference is we don't care about the resut of a job ("fire and forget"),
 so enqueue_job() returns void, while the result of the task can be obtained
 through std::future object returned by enqueue_task().
 ******************************************************************************/
class threadpool {
    PCOMN_NONCOPYABLE(threadpool) ;
    PCOMN_NONASSIGNABLE(threadpool) ;

    typedef std::atomic<bool>               atomic_flag ;
    typedef std::shared_ptr<atomic_flag>    atomic_flag_ptr ;

    // Threadpool's task queue item
    class packed_job final {
        template<typename=void, std::nullptr_t=nullptr> struct make ;

        template<std::nullptr_t dummy>
        struct make<void, dummy> {
            virtual ~make() = 0 ;
            virtual void operator()() = 0 ;
        } ;

        template<typename Function, std::nullptr_t dummy>
        struct make final : make<void> {
            make(Function &&f) : _f(std::move(f)) {}
            ~make() = default ;
            void operator()() { _f() ; }

        private:
            Function _f ;
            PCOMN_NONCOPYABLE(make) ;
        } ;

    public:
        template<typename R>
        packed_job(std::packaged_task<R()> &&task) :
            _make(new make<std::packaged_task<R()>>(std::move(task)))
        {}

        packed_job(packed_job &&) = default ;
        packed_job &operator=(packed_job &&) = default ;

        void operator()() const { (*_make)() ; }

    private:
        std::unique_ptr<make<>> _make ;
    } ;

public:
    threadpool() ;
    explicit threadpool(int threadcount) { resize(threadcount) ; }

    /// Wait until all the pending tasks from the queue have been completed.
    ~threadpool() ;

    /// Get the number of threads in the pool.
    size_t size() const
    {
        PCOMN_SCOPE_LOCK(lock, _pool_mutex) ;
        return _threads.size() ;
    }

    /// Get the count of currently idle threads.
    int idle_count() { return _idle_count ; }

    /// Change the count of threads in the pool.
    void resize(size_t threadcount) ;

    /// Drop all the pending tasks from the task queue.
    ///
    unsigned clear_queue() ;

    /// Stop the pool.
    /// All threads are joined and deleted.
    ///
    /// This function immediately closes the producing end of the pool's queue, so any
    /// attempt to add a new task to the pool leads to sequence_closed exception.
    /// All the task already being run will be completed; whether the pending (not yet
    /// started) task will be invoked depends on `complete_pending_tasks` argument.
    ///
    /// @param complete_pending_tasks If true, all the tasks already in the queue will be
    ///  invoked and completed before the stop; otherwise the queue is cleared
    ///  immediately and only already running tasks are completed.
    ///
    /// @note After this call the pool cannot be restarted.
    ///
    void stop(bool complete_pending_tasks = false) ;

    /// Put the callable object into the task queue for subsequent execution.
    /// The result of execution (return value or exception) is available through the
    /// returned std::future<> object.
    template<typename F>
    auto enqueue_task(F &&task) ->std::future<decltype(task())>
    {
        typedef decltype(task()) task_result ;

        auto ptask = std::packaged_task<task_result()>(std::forward<F>(task)) ;
        std::future<task_result> result (ptask.get_future()) ;

        _task_queue.push(std::move(ptask)) ;

        return result ;
    }

    template<typename F>
    auto enqueue_job(F &&job) ->std::void_t<decltype(job())>
    {
        _task_queue.push(packed_job(std::packaged_task<void()>(std::forward<F>(job)))) ;
    }

    auto qq()
    {
        enqueue_job([]{ return 2*2 ; }) ;
        return enqueue_task([]{ return 2*2 ; }) ;
    }

private:
    std::atomic<unsigned> _idle_count {0} ;  /* Count of idle threads. */
    atomic_flag           _retire {false} ;
    atomic_flag           _stop   {false} ;

    mutable std::mutex _pool_mutex ;

    blocking_ring_queue<packed_job> _task_queue {4096} ;
    std::list<std::pair<pthread, atomic_flag_ptr>> _threads ;

private:
    void start_thread() ;
    void flush_task_queue() ;
} ;

} // end of namespace pcomn

#endif /* __PCOMN_THREADPOOL_H */
