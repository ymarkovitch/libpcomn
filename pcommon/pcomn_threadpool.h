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
#include <pcomn_syncobj.h>
#include <pcomn_meta.h>

#include <functional>
#include <thread>
#include <atomic>
#include <memory>
#include <future>
#include <queue>
#include <list>

namespace pcomn {

/***************************************************************************//**
 A resizable thread pool to run any callable objects with signature `ret func()`.

 The resulting task value or exception is available through std::future<ret>.
*******************************************************************************/
class threadpool {
    PCOMN_NONCOPYABLE(threadpool) ;
    PCOMN_NONASSIGNABLE(threadpool) ;

    typedef std::atomic<bool>               atomic_flag ;
    typedef std::shared_ptr<atomic_flag>    atomic_flag_ptr ;
    typedef std::function<void()>           packed_task ;

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

    /// Empty the task queue.
    void clear_queue()
    {
        PCOMN_SCOPE_LOCK(lock, _pool_mutex) ;
        flush_task_queue() ;
    }

    /// Stop the pool.
    /// All threads are joined and deleted.
    /// @param complete_pending_tasks If true, complete all the tasks in the queue before
    /// the stop; otherwise clear the queue immediately, ignore all the pending tasks.
    ///
    void stop(bool complete_pending_tasks = false) ;

    /// Put the callable object into the task queue for subsequent execution.
    /// The result of execution (return value or exception) is available through the
    /// returned std::future<> object.
    template<typename F>
    auto enqueue(F &&task) ->std::future<decltype(task())>
    {
        typedef decltype(task()) task_result ;

        auto ptask = std::packaged_task<task_result>(std::forward<F>(task)) ;
        std::future<task_result> result (ptask.get_future()) ;

        PCOMN_SCOPE_LOCK(lock, _pool_mutex) ;

        _task_queue.emplace(std::move(ptask)) ;
        _pool_condvar.notify_one() ;

        return result ;
    }

private:
    std::atomic<unsigned> _idle_count {0} ;  /* Count of idle threads. */
    atomic_flag           _retire {false} ;
    atomic_flag           _stop   {false} ;

    mutable std::mutex      _pool_mutex ;
    std::condition_variable _pool_condvar ;

    std::queue<packed_task> _task_queue ;
    std::list<std::pair<std::thread, atomic_flag_ptr>> _threads ;

private:
    void start_thread() ;
    void flush_task_queue() ;
} ;
}

#endif /* __PCOMN_THREADPOOL_H */
