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
#include "pcomn_strslice.h"

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
  2. Form a batch of jobs by submitting _all_ the tasks to be performed.
  3. Start processing either by calling either run() or wait() function.
  4. After run() or wait() no new jobs can be submitted, i.e. job_batch is "one-off".

 A job can be submitted with add_task() or add_job(). The difference is add_task()
 returns std::future<> object, thus allowing to obtain the result of execution,
 while add_job() returns void ("fire and forget").
 ******************************************************************************/
class job_batch {
    PCOMN_NONCOPYABLE(job_batch) ;
    PCOMN_NONASSIGNABLE(job_batch) ;
    // Give threadpool access to job_batch::job and job_batch::task
    friend class threadpool ;
public:
    /// Create a batch with at most `threadcount` worker threads.
    ///
    /// Actual worker threads count depends on the count of added task/jobs and
    /// is finally decided at the run() call as `min(threadcount,taskcount)`.
    ///
    /// @param threadcount Maximal thread count.
    /// @param name Facilitates debugging/profiling/monitoring: every OS thread launched
    ///   by the batch will get this name (set by e.g. pthread_setname_np); note the
    ///   maximum name length is 15.
    ///
    /// @note Worker threads are actually started at the first run() call or at first
    /// call of any of wait(), try_wait(), wit_for(), wait_until().
    ///
    explicit job_batch(unsigned threadcount, const strslice &name = {}) :
        job_batch(threadcount, 1, name)
    {}

    /// Create a batch with at most `max_threadcount` worker threads.
    ///
    /// Actual worker threads count depends on the count of added task/jobs and
    /// is finally decided at the run() call as `min(max_threadcount,taskcount/jobs_per_thread)`.
    ///
    job_batch(unsigned max_threadcount, unsigned jobs_per_thread, const strslice &name = {}) ;

    /// Destructor waits until all the pending tasks from the queue have been completed.
    ~job_batch() ;

    /// Get the batch name, set by the constructor.
    /// Never NULL, may be empty.
    const char *name() const { return _name ; } ;

    /// Start processing jobs.
    ///
    /// It is safe and relatively cheap to call run() for already running batch,
    /// it is no-op then. Calling run() for an already finished batch or for
    /// an empty batch is also OK and immediately returns true.
    ///
    /// @return true if all the jobs are completed upon return from run(), false
    /// otherwise.
    ///
    bool run() ;

    /// Wait until all the jobs are finished.
    /// Automatically starts batch, i.e. calls run(), if the batch is not started.
    /// Can be called multtiple times incl. in parallel incl. with run().
    ///
    void wait() ;

    /// Check if all the jobs are completed.
    /// Never blocks.
    ///
    bool try_wait() { return wait_with_timeout(TimeoutMode::Period, {}) ; }

    /// Block until all the jobs become completed or `timeout_duration` has elapsed,
    /// whichever comes first.
    ///
    /// Uses std::chrono::steady_clock to measure a duration, thus immune to clock
    /// adjusments.
    /// If `timeout_duration` is zero, behaves like try_wait().
    ///
    /// @return true if all the jobs have been completed, false if timeout expired.
    ///
    template<typename R, typename P>
    bool wait_for(const std::chrono::duration<R, P> &timeout_duration)
    {
        return wait_with_timeout(TimeoutMode::Period, timeout_duration) ;
    }

    /// Block until all the jobs become completed or until specified `abs_time`
    /// has been reached, whichever comes first.
    ///
    /// Only allows std::chrono::steady_clock or std::chrono::system_clock to specify
    /// `abs_time`.
    /// If `abs_time` has already passed, behaves like try_wait().
    ///
    /// @return true if all the jobs have been completed, false if timeout expired.
    ///
    template<typename Clock, typename Duration>
    bool wait_until(const std::chrono::time_point<Clock, Duration> &abs_time)
    {
        return wait_with_timeout(timeout_mode_from_clock<Clock>(), abs_time.time_since_epoch()) ;
    }

    /// Get the number of threads in the pool.
    /// Not thread-safe wrt run().
    /// Before run() call is always 0.
    size_t size() const { return _threads.size() ; }

    /// Immediately drop all the pending jobs and stop threads.
    /// All threads are joined and deleted.
    ///
    void stop() ;

    /// Append the callable object to the batch and get std::future<> to allow obtaining
    /// the result of execution.
    ///
    template<typename F, typename... Args>
    auto add_task(F &&callable, Args &&... args)
    {
        using namespace std ;
        typedef task<decay_t<F>, decay_t<Args>...> task_type ;

        _jobs.reserve(_jobs.size() + 1) ;
        task_type *new_task = new task_type(forward<F>(callable), forward<Args>(args)...) ;
        _jobs.emplace_back(new_task) ;

        return new_task->get_future() ;
    }

    /// Append the callable object to the batch.
    ///
    template<typename F, typename... Args>
    void add_job(F &&callable, Args &&... args)
    {
        using namespace std ;

        _jobs.reserve(_jobs.size() + 1) ;
        _jobs.emplace_back(new job<decay_t<F>, decay_t<Args>...>
                           (forward<F>(callable), forward<Args>(args)...)) ;
    }

    friend std::ostream &operator<<(std::ostream &os, const job_batch &v)
    {
        v.print(os) ;
        return os ;
    }

private:
    struct assignment {
        virtual ~assignment() = default ;
        virtual void run() = 0 ;
        virtual void set_exception(std::exception_ptr &&) ;
    } ;

    template<typename F, typename... Args>
    struct packaged_job : assignment {

        template<typename A0, typename... An>
        packaged_job(A0 &&fn, An &&...args) :
            _function(std::forward<A0>(fn)),
            _args(std::forward<An>(args)...)
        {}

        decltype(auto) invoke()
        {
            return invoke_function(std::index_sequence_for<Args...>()) ;
        }

    private:
        template<size_t... I>
        decltype(auto) invoke_function(std::index_sequence<I...>)
        {
            return _function(std::get<I>(_args)...) ;
        }

    private:
        F                     _function ;
        std::tuple<Args...>   _args ;
    } ;

    /***************************************************************************
     job
    ***************************************************************************/
    template<typename F, typename... Args>
    class alignas(cacheline_t) job final : public packaged_job<F, Args...> {
        typedef packaged_job<F, Args...> ancestor ;

    public:
        using ancestor::ancestor ;
        void run() override { this->invoke() ; }
    } ;

    /***************************************************************************
     task
    ***************************************************************************/
    template<typename F, typename... Args>
    class alignas(cacheline_t) task final : public packaged_job<F, Args...> {
        typedef packaged_job<F, Args...> ancestor ;
    public:
        typedef decltype(std::declval<ancestor>().invoke()) result_type ;

        using ancestor::ancestor ;

        auto get_future() { return _promise.get_future() ; }

        void run() override
        {
            invoke_set_result(std::bool_constant<std::is_void_v<result_type>>()) ;
        }

        void set_exception(std::exception_ptr &&x) override
        {
            _promise.set_exception(x) ;
            ancestor::set_exception(std::move(x)) ;
        }

    private:
        std::promise<result_type> _promise ;

    private:
        void invoke_set_result(std::false_type)
        {
            _promise.set_value(this->invoke()) ;
        }

        void invoke_set_result(std::true_type)
        {
            this->invoke() ;
            _promise.set_value() ;
        }
    } ;

private:
    const char     _name[16] = {} ;
    const unsigned _max_threadcount ;
    const unsigned _jobs_per_thread = 1 ;

    mutable shared_mutex _pool_mutex ;

    promise_lock             _finished ;

    std::atomic<ssize_t>     _jobndx {0} ;
    std::atomic<ssize_t>     _pending {0} ;

    alignas(cacheline_t) std::vector<pthread> _threads ;
    std::vector<std::unique_ptr<assignment>>  _jobs ;

private:
    unsigned jobcount() const { return _jobs.size() ; }

    bool wait_with_timeout(TimeoutMode mode, std::chrono::nanoseconds timeout)
    {
        return run() || _finished.wait_with_timeout(mode, timeout) ;
    }

    void worker_thread_function(unsigned threadndx) ;

    void print(std::ostream &) const ;

    static void exec_task(assignment &) noexcept ;
} ;

/***************************************************************************//**
 A resizable thread pool to run any callable objects with signature `ret func()`.

 The resulting task value or exception is available through std::future<ret>.

 Two kinds of work can be submitted into the pool: a *task* and a *job*.
 The difference is we don't care about the resut of a job ("fire and forget"),
 so enqueue_job() returns void, while the result of the task can be obtained
 through std::future object returned by enqueue_task().

 When jobs/task are submitted they are placed into the task queue, from which
 worker threads extract them. The task queue is a blocking MPMC bounded queue
 with ring storage (blocking_ring_queue), so its *maximum* capacity must be
 initially specified in the threadpool constructor.

 @note While *all* the underlying ring queue memory is allocated in the constructor,
  it is not initialized immediately but on demand, entry by entry when tasks are
  actually added, so specifying large maximum capacity does not compromize performance.

 @note The size of a task queue entry is 8 bytes.
*******************************************************************************/
class threadpool {
    PCOMN_NONCOPYABLE(threadpool) ;
    PCOMN_NONASSIGNABLE(threadpool) ;

    template<typename F, typename... A> class task ;

    using assignment   = job_batch::assignment  ;

    template<typename F, typename... A>
    using packaged_job = job_batch::packaged_job<F, A...> ;

    template<typename F, typename... A>
    using job          = job_batch::job<F, A...> ;

    template<typename F, typename... A>
    using task_result_t = typename task<F, A...>::result_type ;

public:
    template<typename T> class task_result ;

    template<typename T>
    using task_result_ptr  = std::unique_ptr<task_result<T>> ;

    template<typename T>
    using result_queue = blocking_list_queue<task_result_ptr<T>> ;

    template<typename T>
    using result_queue_ptr = std::shared_ptr<result_queue<T>> ;

    /// Create a threadpool with specified thread count, name, and maximum task queue
    /// capacity.
    ///
    threadpool(size_t threadcount, const strslice &name, size_t max_capacity = 0) ;

    explicit threadpool(size_t threadcount, size_t max_capacity = 0) :
        threadpool(threadcount, {}, max_capacity)
    {}

    /// Close the pushing edn of the task/job queue and wait until all the pending tasks
    /// from the queue have been completed.
    ~threadpool() ;

    /// Get the count of threads in the pool.
    size_t size() const
    {
        return _thread_count.load(std::memory_order_relaxed).expected_count() ;
    }

    /// Get the pool capacitiy, which is the sum of thread count and task queue capacity.
    ///
    size_t capacity() const { return size() + _task_queue.capacity() ; }

    /// Get the (approximate) count of pending (pushed but not yet popped) items
    /// in the queue.
    ///
    size_t pending_count() const { return _task_queue.size() ; }

    /// Get the pool name, set by the constructor.
    /// Never NULL, may be empty.
    const char *name() const { return _name ; } ;

    void set_queue_capacity(unsigned new_capacity) ;

    size_t max_queue_capacity() const { return _task_queue.max_capacity() ; }

    /// Get the implementation-defined maximum thread count for a threadpool.
    /// Sanity constraint. Power of 2.
    static constexpr size_t max_threadcount() { return 2048 ; }

    /// Change the count of threads in the pool.
    void resize(size_t threadcount) ;

    /// Drop all the pending tasks from the task queue.
    ///
    /// After calling this function the thread pool is intact and ready to handle
    /// new tasks.
    /// @return Dropped tasks count.
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
    /// @note By default, stop() _drops_ all noncompleted tasks/jobs.
    ///
    void stop(bool complete_pending_tasks = false) ;

    template<typename F, typename... Args>
    void enqueue_job(F &&callable, Args &&... args)
    {
        _task_queue.push(task_ptr(new job<std::decay_t<F>, std::decay_t<Args>...>
                                  (std::forward<F>(callable), std::forward<Args>(args)...))) ;
    }

    /// Put the callable object into the task queue for subsequent execution.
    /// The result of execution (return value or exception) is available through the
    /// returned std::future<> object.
    template<typename F, typename... Args>
    std::future<task_result_t<F, Args...>> enqueue_task(F &&call, Args &&... args)
    {
        typedef task<std::decay_t<F>, std::decay_t<Args>...> task_type ;

        task_type *new_task = new task_type(std::forward<F>(call), std::forward<Args>(args)...) ;
        auto future_result (new_task->get_future()) ;

        _task_queue.push(task_ptr(new_task)) ;
        return future_result ;
    }

    template<typename F, typename... Args>
    void enqueue_task(const result_queue_ptr<task_result_t<F, Args...>> &output_funnel,
                      F &&call, Args &&... args)
    {
        typedef task<std::decay_t<F>, std::decay_t<Args>...> task_type ;

        task_type *new_task = new task_type(std::forward<F>(call), std::forward<Args>(args)...) ;
        auto future_result (new_task->get_future()) ;

        _task_queue.push(task_ptr(new_task)) ;
    }

    friend std::ostream &operator<<(std::ostream &os, const threadpool &v)
    {
        v.print(os) ;
        return os ;
    }

public:
    /***********************************************************************//**
     Custom future class.
    ***************************************************************************/
    template<typename T>
    class task_result : public std::future<T> {
        typedef std::future<T> ancestor ;
        template<typename F, typename... A> friend class task ;
    public:
        task_result() = default ;
        task_result(task_result &&) = default ;

        task_result(std::future<T> &&result, result_queue_ptr<T> &&result_queue = {}) :
            ancestor(std::move(result)),
            _queue_anchor(std::move(result_queue))
        {}

        task_result &operator=(task_result &&) = default ;

    private:
        result_queue_ptr<T> _queue_anchor ;
    } ;

private:
    // Threadpool's task queue item
    typedef std::unique_ptr<assignment> task_ptr ;
    typedef std::list<pthread> thread_list ;

    /***************************************************************************
     task
    ***************************************************************************/
    template<typename F, typename... Args>
    class alignas(cacheline_t) task final : public packaged_job<F, Args...> {
        typedef packaged_job<F, Args...> ancestor ;
    public:
        typedef decltype(std::declval<ancestor>().invoke()) result_type ;

        using ancestor::ancestor ;

        auto get_future() { return _promise.get_future() ; }

        void run() override
        {
            invoke_set_result(std::bool_constant<std::is_void_v<result_type>>()) ;
            enqueue_result() ;
        }

        void set_exception(std::exception_ptr &&x) override
        {
            _promise.set_exception(x) ;
            ancestor::set_exception(std::move(x)) ;
            enqueue_result() ;
        }

    private:
        std::promise<result_type>      _promise ;
        result_queue_ptr<result_type>  _result_queue ;

    private:
        void invoke_set_result(std::false_type)
        {
            _promise.set_value(this->invoke()) ;
        }

        void invoke_set_result(std::true_type)
        {
            this->invoke() ;
            _promise.set_value() ;
        }

        void enqueue_result()
        {
            if (!_result_queue)
                return ;

            auto result (std::make_unique<task_result<result_type>>(get_future(), std::move(_result_queue))) ;

            NOXCHECK(!_result_queue) ;

            result->_queue_anchor->push(std::move(result)) ;
            NOXCHECK(!result) ;
        }
    } ;

    union thread_count {
        constexpr thread_count() noexcept : _data(0) {}
        constexpr thread_count(int32_t running, int32_t diff) noexcept :
            _diff(diff), _running(running)
        {}

        int64_t expected_count() const { return (int64_t)_running + _diff ; }

        // Increment running threads count keeping expected_count() unchanged.
        thread_count &operator+=(int threads)
        {
            _diff -= threads ;
            _running += threads ;
            return *this ;
        }
        thread_count &operator-=(int threads) { return operator+=(-threads) ; }

        static constexpr thread_count stopped() { return {-1, 0} ; }

        uint64_t _data ;

        struct {
            int32_t _diff ;    /* The difference between the expected and current
                                * threads counts. */
            int32_t _running ; /* The actual count of currently running threads.
                                * When the pool is in the steady state it is equal to
                                * the size set by the last resize() or by the constructor,
                                * whatever is called last. */
        } ;
    } ;

    friend std::ostream &operator<<(std::ostream &os, const thread_count &v)
    {
        if (v._data == thread_count::stopped()._data)
            os << "{stopped}" ;
        else
            os << '{' << v.expected_count() << '/' << v._running << '/' << v._diff << '}' ;
        return os ;
    }

private:
    const char _name[16] = {} ;

    mutable std::mutex _pool_mutex ;

    std::atomic<thread_count> _thread_count {} ;

    thread_list        _threads ;
    thread_list        _dropped_thread ; /* Single-item list, a thread being dismissed is
                                          * moved here first. */
    std::atomic<bool>  _dropped {false} ; /* Indicates there is a _dropped_thread */

    blocking_ring_queue<task_ptr> _task_queue {estimate_max_capacity(0, 0)} ;

private:
    enum Dismiss : bool { CONTINUE, DISMISS } ;

    void worker_thread_function(thread_list::iterator) ;
    void start_thread() ;

    Dismiss handle_pool_resize(thread_list::iterator self) noexcept ;
    std::pair<bool, thread_count> check_dismiss_itself(thread_list::iterator self) noexcept ;
    bool check_launch_new_thread(thread_count current_count) ;

    void flush_task_queue() ;

    static unsigned estimate_max_capacity(size_t threadcount, size_t max_capacity) ;
    void print(std::ostream &) const ;
} ;

} // end of namespace pcomn

#endif /* __PCOMN_THREADPOOL_H */
