/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   unittest_threadpool.cpp
 COPYRIGHT    :   Yakov Markovitch, 2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for job_batch and threadpool.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   13 Mar 2020
*******************************************************************************/
#include "unittest_semaphore.h"

#include <pcomn_threadpool.h>
#include <pcomn_stopwatch.h>

#include <thread>

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

using namespace pcomn ;
using namespace std::chrono ;

/*******************************************************************************
                            class JobBatchTests
*******************************************************************************/
class JobBatchTests : public CppUnit::TestFixture {

    void Test_JobBatch_Init() ;
    void Test_JobBatch_Run() ;

    CPPUNIT_TEST_SUITE(JobBatchTests) ;

    CPPUNIT_TEST(Test_JobBatch_Init) ;
    CPPUNIT_TEST(Test_JobBatch_Run) ;

    CPPUNIT_TEST_SUITE_END() ;

private:
    unit::watchdog watchdog {3s} ;

public:
    void setUp()
    {
        watchdog.arm() ;
    }
    void tearDown()
    {
        watchdog.disarm() ;
    }
} ;

/*******************************************************************************
                            class ThreadPoolTests
*******************************************************************************/
class ThreadPoolTests : public CppUnit::TestFixture {

    void Test_JobBatch_Init() ;
    void Test_JobBatch_Run() ;

    void Test_ThreadPool_Init() ;
    void Test_ThreadPool_SingleThreaded() ;
    void Test_ThreadPool_MultiThreaded() ;

    CPPUNIT_TEST_SUITE(ThreadPoolTests) ;

    CPPUNIT_TEST(Test_ThreadPool_Init) ;
    CPPUNIT_TEST(Test_ThreadPool_SingleThreaded) ;

    CPPUNIT_TEST(Test_ThreadPool_Init) ;
    CPPUNIT_TEST(Test_ThreadPool_SingleThreaded) ;
    CPPUNIT_TEST(Test_ThreadPool_MultiThreaded) ;

    CPPUNIT_TEST_SUITE_END() ;

private:
    unit::watchdog watchdog {3s} ;

public:
    void setUp()
    {
        watchdog.arm() ;
    }
    void tearDown()
    {
        watchdog.disarm() ;
    }
} ;

/*******************************************************************************
 JobBatchTests
*******************************************************************************/
void JobBatchTests::Test_JobBatch_Init()
{
    job_batch b1 (1) ;
    job_batch b2 (5, 3) ;
}

void JobBatchTests::Test_JobBatch_Run()
{
}

/*******************************************************************************
 ThreadPoolTests
*******************************************************************************/
void ThreadPoolTests::Test_ThreadPool_Init()
{
    threadpool p0 ;
    threadpool p1 (1) ;
    threadpool p4 (4) ;
}

void ThreadPoolTests::Test_ThreadPool_SingleThreaded()
{
}

void ThreadPoolTests::Test_ThreadPool_MultiThreaded()
{
}

/*******************************************************************************
 main
*******************************************************************************/
int main(int argc, char *argv[])
{
    return pcomn::unit::run_tests
        <
            JobBatchTests,
            ThreadPoolTests
        >
        (argc, argv) ;
}
