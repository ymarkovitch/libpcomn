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

    catch(...)
    {
        try { job.set_exception(std::current_exception()) ; } catch(...) {}
    }
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

threadpool::threadpool() = default ;

threadpool::threadpool(size_t threadcount, const strslice &name, size_t max_capacity) :
    _task_queue(estimate_max_capacity(threadcount, max_capacity))
{
    init_threadname(as_mutable(_name), name, "Thread pool name") ;
    resize(threadcount) ;
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

    PCOMN_SCOPE_W_LOCK(lock, _pool_mutex) ;

    const int32_t current_count = size() ;
    if (current_count == new_count)
        return ;

    const int32_t increment = new_count - current_count ;

    thread_count prev_count =
        atomic_op::fetch_and_F(&_thread_count, [increment](thread_count old_count)
        {
            old_count._diff += increment ;
            return old_count ;
        }) ;

    prev_count._diff += increment ;

    // If we need a new thread, make single attempt to start one.
    // This
    if (prev_count._diff > 0)
    {
        thread_count next_count = prev_count ;
        --next_count._diff ;
        ++next_count._running ;

         if (_thread_count.compare_exchange_strong(prev_count, next_count, std::memory_order_acq_rel)) ;
    }
}

void threadpool::worker_thread_function(std::list<pthread>::iterator self)
{
    for(fwd::optional task_opt ; handle_pool_resize() && (task_opt = _task_queue.pop_opt()) ;)
    {
        if (const task_ptr current_task = std::move(*task_opt))
        {
            job_batch::exec_task(*current_task) ;
        }
    }

    PCOMN_SCOPE_W_LOCK(lock, _pool_mutex) ;
    _threads.erase(self) ;
}

unsigned threadpool::stop(bool complete_pending_tasks)
{
    return 0 ;
}

void threadpool::print(std::ostream &os)
{
}

} // end of namespace pcomn
