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

/*******************************************************************************
 job_batch
*******************************************************************************/
job_batch::job_batch(unsigned max_threadcount, unsigned jobs_per_thread, const strslice &name) :
    _max_threadcount(PCOMN_ENSURE_ARG(max_threadcount)),
    _jobs_per_thread(PCOMN_ENSURE_ARG(jobs_per_thread))
{
    PCOMN_THROW_IF(name.size() >= sizeof(_name), std::out_of_range,
                   "Job batch name " P_STRSLICEQF " is too long, maximum allowed length is %u.",
                   P_STRSLICEV(name), unsigned(sizeof(_name) - 1)) ;

    memslicemove(const_cast<char *>(_name), name) ;
}

job_batch::~job_batch()
{
    if (!jobcount())
        return ;

    stop() ;
    _finished.wait() ;

    PCOMN_VERIFY(!_threads.empty()) ;
    // _threads.front() joins other threads
    _threads.front().join() ;
}

bool job_batch::run()
{
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

    PCOMN_SCOPE_W_XLOCK(lock, _pool_mutex) ;

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
    PCOMN_SCOPE_W_XLOCK(lock, _pool_mutex) ;
    if (!size())
        return ;

    const ssize_t latest_jobndx =
        _jobndx.fetch_sub(_jobndx.load(std::memory_order_relaxed) + 1, std::memory_order_acq_rel) ;

    const ssize_t pending = std::min(ssize_t(), latest_jobndx + 1) ;

    if (_pending.fetch_sub(pending, std::memory_order_acq_rel))
        _finished.unlock() ;
}

// Rather than joining all the workers somewhere in destructor after completing
// all the jobs, delegate (the most of) this work to one of the worker threads,
// namely the thread with index 0.
// Then, only joining with this thread is needed.
void job_batch::worker_thread_function(unsigned threadndx)
{
    ssize_t pending = 1 ;
    ssize_t job_index ;

    while ((job_index = _jobndx.fetch_sub(1, std::memory_order_acq_rel)) >= 0)
    {
        auto job (std::move(_jobs[job_index])) ;

        try { job->run() ; }

        catch(...)
        {
            try { job->set_exception(std::current_exception()) ; } catch(...) {}
        }

        pending = _pending.fetch_sub(1, std::memory_order_acq_rel) - 1 ;
    }

    if (!pending)
        _finished.unlock() ;

    if (!threadndx)
    {
        // The zeroth worker joins others.
        PCOMN_SCOPE_R_LOCK(lock, _pool_mutex) ;
        for (size_t ndx = size() ; --ndx ; _threads[ndx].join()) ;
    }
}

void job_batch::wait()
{
    NOXFAIL("not implemented") ;
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
threadpool::threadpool() = default ;

threadpool::~threadpool() { stop() ; }

unsigned threadpool::clear_queue()
{
    return _task_queue.try_pop_some(std::numeric_limits<int32_t>::max()).size() ;
}

void threadpool::resize(size_t threadcount)
{
}

unsigned threadpool::stop(bool complete_pending_tasks)
{
    return 0 ;
}

} // end of namespace pcomn
