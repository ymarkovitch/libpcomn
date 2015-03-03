/*-*- tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)) -*-*/
/*******************************************************************************
 FILE         :   perftest_locks.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2015. All rights reserved.

 DESCRIPTION  :   Performance and sizes of various synchronization primitives.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   13 Nov 2008
*******************************************************************************/
#include <pcomn_except.h>
#include <pcomn_synccomplex.h>
#include <pcomn_stopwatch.h>
#include <pcomn_meta.h>
#include <pcomn_thread.h>

#include <iostream>
#include <new>
#include <functional>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static void usage()
{
    fprintf(stderr,
            "Usage: %s passcount [threads]\n"
            "Test performance of various PCOMMON synchronization primitives.\n",
            PCOMN_PROGRAM_SHORTNAME) ;
    exit(1) ;
}

using namespace pcomn ;

static const unsigned P_threads = argc < 3 ? 2 : atoi(argv[1]) ;


            PTSafePtr<TaskThread> spawned_thread
                (new TaskThread(make_job(std::bind(&infinitely_run_client, foo_client)), true)) ;

int main(int argc, char *argv[])
{
    if (!pcomn::inrange(argc, 2, 3))
        usage() ;

    const long count = atol(argv[1]) ;
    const int threads = argc < 3 ? 2 : atoi(argv[1]) ;

    if (count <= 0 || threads <= 0)
        usage() ;

    std::cout << "Running tests on " << PCOMN_PL_CPUBITS << "-bit platform" << std::endl ;

    try {
        typedef pcomn::shared_mutex LockType ;
        PCpuStopwatch stopwatch ;
        std::cout << PCOMN_TYPENAME(LockType) << ' ' << sizeof(LockType) << ' '
                  << std::alignment_of<LockType>::value << std::endl ;

        std::cout << "Aquiring/releasing the read half of a read/write lock " << count << " times... " << std::flush ;

        stopwatch.start() ;
        for (unsigned c = count ; c ; --c)
        {
            //LockType lock ;
            void *buf[sizeof(LockType)/sizeof(void *) + 1] ;
            new (buf) LockType ;
            /*
            lock.lock() ;
            lock.unlock() ;
            lock.lock_shared() ;
            lock.unlock_shared() ;
            */
            /*
            lock.lock() ;
            lock.unlock() ;
            */
        }
        stopwatch.stop() ;
        for (unsigned c = count ; c ; --c)
        {
            /*
            lock.unlock() ;
            */
            //lock.unlock_shared() ;
            /*
            lock.lock() ;
            lock.unlock() ;
            */
        }
        std::cout << "OK\nElapsed time: " << stopwatch << " seconds" << std::endl ;
    }
    catch (const std::exception &x)
    {
        std::cerr << STDEXCEPTOUT(x) << std::endl ;
        return 1 ;
    }
    return 0 ;
}
