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
job_batch::job_batch(unsigned max_threadcount, unsigned jobs_per_thread) :
    _max_threadcount(PCOMN_ENSURE_ARG(max_threadcount)),
    _jobs_per_thread(PCOMN_ENSURE_ARG(jobs_per_thread))
{}

job_batch::~job_batch()
{
    if (!jobcount())
        return ;

    _pending.store(0, std::memory_order_release) ;
    _jobndx.store(-1, std::memory_order_release) ;

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
        // The first worker thread will join all other worker threads.
        const bool join_others = !ndx ;
        _threads.emplace_back([this, join_others] { worker_thread_function(join_others) ; }) ;
    }

  end:
    lock.unlock() ;
    return _finished.try_wait() ;
}

void job_batch::worker_thread_function(bool join_others)
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

    if (join_others)
    {
        PCOMN_SCOPE_R_LOCK(lock, _pool_mutex) ;
        for (size_t i = size() ; --i ; _threads[i].join()) ;
    }
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

} // end of namespace pcomn
