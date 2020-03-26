/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_threadpool.cpp
 COPYRIGHT    :   Yakov Markovitch, 2018-2020. All rights reserved.

 DESCRIPTION  :   A resizable threadpool.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   6 Nov 2018
*******************************************************************************/
#include "pcomn_threadpool.h"
#include "pcomn_diag.h"

#define SUPPRESS_EXCEPTION(...) try { __VA_ARGS__ ; } catch (...) \
    { LOGPXERR(PCOMN_ThreadPool, "Suppressed exception at " << PCOMN_PRETTY_FUNCTION << ": " << oexception()) ; }

namespace pcomn {

template<size_t n>
static __noinline void init_threadname(char (&dest)[n], const strslice &name, const char *msghead)
{
    if (!name)
        return ;

    PCOMN_THROW_IF(name.size() >= n, std::out_of_range,
                   "%s " P_STRSLICEQF " is too long, maximum allowed length is %u.",
                   msghead, P_STRSLICEV(name), unsigned(n - 1)) ;

    memslicemove(dest + 0, name) ;
}

/*******************************************************************************
 job_batch
*******************************************************************************/
job_batch::job_batch(unsigned max_threadcount, unsigned jobs_per_thread, const strslice &name) :
    _max_threadcount(PCOMN_ENSURE_ARG(max_threadcount)),
    _jobs_per_thread(PCOMN_ENSURE_ARG(jobs_per_thread))
{
    init_threadname(as_mutable(_name), name, "Job batch name") ;
}

job_batch::~job_batch()
{
    if (!jobcount())
        return ;

    stop() ;
    _finished.wait() ;

    PCOMN_VERIFY(!_threads.empty()) ;
    // Join only _threads.front(), it will joins other threads.
    _threads.front().join() ;
}

bool job_batch::run()
{
    PCOMN_SCOPE_W_XLOCK(lock, _pool_mutex) ;

    if (!jobcount())
    {
        _finished.unlock() ;
        return true ;
    }

    if (_pending.load(std::memory_order_relaxed))
        return false ;

    const unsigned threadcount =
        std::min(_max_threadcount, (jobcount() + _jobs_per_thread - 1)/_jobs_per_thread) ;

    NOXCHECK(threadcount) ;

    if (size())
        goto end ;

    _threads.reserve(threadcount) ;

    _jobndx.store(jobcount(), std::memory_order_release) ;
    _pending.store(jobcount(), std::memory_order_release) ;

    for (unsigned ndx = 0 ; ndx < threadcount ; ++ndx)
    {
        _threads.emplace_back(pthread::F_DEFAULT, _name,
                              [this, ndx] { worker_thread_function(ndx) ; }) ;
    }

  end:
    lock.unlock() ;
    return _finished.try_wait() ;
}

void job_batch::stop()
{
    PCOMN_SCOPE_W_LOCK(lock, _pool_mutex) ;

    TRACEPX(PCOMN_ThreadPool, DBGL_ALWAYS, "Stop requested for the"
            << (size() ? " started " : " not started ") << *this) ;

    if (!size())
    {
        if (jobcount())
            _jobs.clear() ;
        _finished.unlock() ;
        return ;
    }

    // Prevent all not-yet-started jobs from starting and get the count of unstarted
    // jobs. Note after this statement _jobndx is <0.
    const ssize_t unstarted_count =
        _jobndx.fetch_sub(_jobndx.load(std::memory_order_relaxed) + 1, std::memory_order_acq_rel) ;

    if (_pending.fetch_sub(std::min((ssize_t)0, unstarted_count + 1), std::memory_order_acq_rel))
        _finished.unlock() ;
}

void job_batch::exec_task(assignment &job) noexcept
{
    try { job.run() ; }

    catch(...) { SUPPRESS_EXCEPTION(job.set_exception(std::current_exception())) ; }
}

// Rather than joining all the workers somewhere in destructor after completing
// all the jobs, delegate (the most of) this work to one of the worker threads,
// namely the thread with index 0.
// Then, only joining with this thread is needed.
void job_batch::worker_thread_function(unsigned threadndx)
{
    ssize_t pending = 1 ;
    ssize_t job_index ;

    // The initial value of _jobndx is jobcount(), the final value is 0.
    // Note it is decremented at each started job, but we start jobs in submission order
    // (i.e. from 0 to `jobcount()-1`).
    // Note the order:
    //  decrement _jobndx ;
    //  make corresponding job done ;
    //  decrement _pending ;
    //
    while ((job_index = _jobndx.fetch_sub(1, std::memory_order_acq_rel)) > 0)
    {
        auto current_job (std::move(_jobs[jobcount() - job_index])) ;
        exec_task(*current_job) ;

        pending = _pending.fetch_sub(1, std::memory_order_acq_rel) - 1 ;
    }

    if (!pending)
        _finished.unlock() ;

    // The 0th worker joins others.
    if (!threadndx)
    {
        PCOMN_SCOPE_R_LOCK(lock, _pool_mutex) ;
        for (size_t ndx = size() ; --ndx ; _threads[ndx].join()) ;
    }
}

void job_batch::wait()
{
    if (!run())
        _finished.wait() ;
}

void job_batch::print(std::ostream &os)
{
    os << "job_batch(" << squote(_name)
       << ", unstarted=" << _jobndx.load(std::memory_order_relaxed)
       << ", pending=" << _pending.load(std::memory_order_relaxed) << ')' ;
}

/*******************************************************************************
 job_batch::assignment
*******************************************************************************/
void job_batch::assignment::set_exception(std::exception_ptr &&xptr)
{
    const char * const kind = str::endswith(PCOMN_TYPENAME(*this), "task") ? "task" : "job" ;

    LOGPXERR(PCOMN_ThreadPool,
             "Exception in a worker thread of a job batch or thread pool. "
             "The " << kind << " is aborted. The exception is " << oexception(xptr)) ;
}

/*******************************************************************************
 threadpool
*******************************************************************************/
static constexpr size_t default_queue_capacity_per_thread = 16 ;
static constexpr threadpool::thread_count threadpool::_stopped ;

threadpool::threadpool() = default ;

threadpool::threadpool(size_t threadcount, const strslice &name, size_t max_capacity) :
    _thread_count(0, std::min(threadcount, max_threadcount())),
    _task_queue(estimate_max_capacity(threadcount, max_capacity))
{
    init_threadname(as_mutable(_name), name, "Thread pool name") ;
    check_launch_new_thread(_thread_count.load(std::memory_order_relaxed)) ;
}

threadpool::~threadpool() { stop() ; }

unsigned threadpool::estimate_max_capacity(size_t threadcount, size_t max_capacity)
{
    static const size_t hardware_threads = sys::hw_threads_count() ;

    const size_t maxthreads = std::max(std::min(threadcount, max_threadcount()), hardware_threads) ;

    return std::max(std::min(max_capacity, 0x1000000), maxthreads*default_queue_capacity_per_thread) ;
}

unsigned threadpool::clear_queue()
{
    return _task_queue.try_pop_some(-1).size() ;
}

void threadpool::resize(size_t threadcount)
{
    const int32_t new_count = std::min(threadcount, max_threadcount()) ;
    {
        PCOMN_SCOPE_LOCK(lock, _pool_mutex) ;

        const int32_t current_count = size() ;
        if (current_count == new_count)
            return ;

        ensure<object_closed>(current_count >= 0, "Thread pool") ;

        const int32_t increment = new_count - current_count ;

        thread_count prev_count =
            atomic_op::fetch_and_F(&_thread_count, [increment](thread_count old_count)
            {
                old_count._diff += increment ;
                return old_count ;
            }) ;
    }

    prev_count._diff += increment ;

    // If we need a new thread, make single attempt to start one.
    check_launch_new_thread(prev_count) ;
}

void threadpool::worker_thread_function(thread_list::iterator self)
{
    do while(check_launch_new_thread(_thread_count.load(std::memory_order_acquire))) ;

    for(fwd::optional task_opt ; handle_pool_resize(self) && (task_opt = _task_queue.pop_opt()) ;)
    {
        if (const task_ptr current_task = std::move(*task_opt))
        {
            job_batch::exec_task(*current_task) ;
        }
    }
}

bool threadpool::handle_pool_resize(thread_list::iterator self) noexcept
{
    // Check if there are too many threads, and if so, dismiss itself.
    auto dismissed = check_dismiss_itself(self) ;

    const thread_count current_count = std::get<thread_count>(dismissed) ;
    NOXCHECK(current_count._diff >= 0) ;

    // If there is not enough threads, attempt to launch a new thread
    SUPPRESS_EXCEPTION(check_launch_new_thread(current_count)) ;

    // If there is a pending dropped thread, join it.
    if (_dropped.load(std::memory_order_relaxed) &&
        _pool_mutex.try_lock())
    {
        thread_list dropped ;
        std::swap(_dropped_thread, dropped) ;

        if (dropped.size())
        {
            NOXCHECK(_dropped.load(std::memory_order_relaxed)) ;

            _dropped.store(false, std::memory_order_release) ;
        }

        _pool_mutex.unlock() ;
    }
}

std::pair<bool, thread_count> threadpool::check_dismiss_itself(thread_list::iterator self) noexcept
{
    auto dismissed =
        check_and_apply(&_thread_count,
                        [](thread_count oldcount) /* Check */
                        {
                            return oldcount._diff < 0 ;
                        },
                        [](thread_count newcount) /* Apply */
                        {
                            PCOMN_VERIFY(newcount.expected_count() >= 0) ;
                            return newcount -= 1 ;
                        }) ;

    if (!std::get<bool>(dismissed))
        return dismissed ;

    // Allow prev_dropped to call destructor out of scoped lock scope, to avoid holding
    // _pool_mutex during (potentially time-consuming) join with the dropped thread.
    thread_list prev_dropped ;

    PCOMN_SCOPE_LOCK(lock, _pool_mutex) ;
    {
        prev_dropped.splice(dropped_self.end(), _threads, self) ;
        // After this swap _dropped_thread holds `self`, while prev_dropped
        // holds previous content of _dropped_thread, which (i.e. prev_dropped)
        // will be joined
        std::swap(_dropped_thread, dropped_self) ;

        NOXCHECK(dropped_self.size() <= 1) ;

        _dropped.store(true, std::memory_order_release) ;
    }

    return dismissed ;
}

bool threadpool::check_launch_new_thread(thread_count current_count)
{
    if (current_count._diff <= 0)
        return false ;

    // Prepare allocated list item before entering critical section:
    // reduce potential exception points.
    thread_list new_thread (1) ;

    thread_count newcount = current_count ;
    newcount += 1 ;

    if (_thread_count.compare_exchange_strong(count, newcount, std::memory_order_acq_rel))
    {
        PCOMN_SCOPE_LOCK(lock, _pool_mutex) ;

        // If pthread constructor has thrown exception, roll back all the already
        // taken actions: decrement thread count and delete new thread placeholder
        // from _threads.
        auto on_launch_thread_failure = make_finalizer([&]
        {
            LOGPXERR(PCOMN_ThreadPool,
                     "Exception attempting to launch a new thread for " << *this << ": " << oexception()) ;

            fetch_and_F(&_thread_count, [](thread_count c) { return c -= 1 ; }) ;

            NOXCHECK(new_thread.empty()) ;
            _threads.erase(_threads.begin()) ;
        }) ;

        // No exceptions here, the new thread will be at _threads.begin()
        _threads.splice(_threads.begin(), std::move(new_thread))  ;

        _threads.front() = pthread(pthread::F_AUTOJOIN, _name,
                                   [this, new_self=_threads.begin()] { worker_thread_function(new_self) ; }) ;

        on_launch_thread_failure.release() ;

        count = newcount ;
    }

    return count._diff > 0 ;
}

void threadpool::stop(bool complete_pending_tasks)
{
    PCOMN_SCOPE_XLOCK(lock, _pool_mutex) ;

    if (!complete_pending_tasks)
    {
        // Close both ends, don't wait
       _task_queue.close() ;
       _thread_count.store({-1, 0}, std::memory_order_release) ;
       return ;
    }

    if (_task_queue.close_push())
    {
        // The queue is empty anyway, nothing to wait for.
        _thread_count.store({-1, 0}, std::memory_order_release) ;
        return ;
    }

    // Have to wait until working threads pick up all the remaining tasks from the queue.
    // It's assumed there _are_ working threads, check it.
    if (!size())
        // No working threads, nobody to pop and handle the outstanding tasks.
        throw_syserror(std::errc::resource_deadlock_would_occur,
                       "Request to stop a thread pool waiting for task completion may result in deadlock: "
                       "there is no running worker threads in the pool.") ;

    lock.unlock() ;

    _task_queue.close_push_wait_empty(100'000h) ;
}

void threadpool::print(std::ostream &os)
{
}

} // end of namespace pcomn
