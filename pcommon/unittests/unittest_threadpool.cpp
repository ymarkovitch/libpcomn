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

static size_t INIT_THREADCOUNT ;

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
        INIT_THREADCOUNT = get_threadcount() ;
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

    void Test_ThreadPool_Init() ;
    void Test_ThreadPool_SingleThreaded() ;
    void Test_ThreadPool_MultiThreaded() ;

    CPPUNIT_TEST_SUITE(ThreadPoolTests) ;

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
        INIT_THREADCOUNT = get_threadcount() ;
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
    CPPUNIT_LOG_EXCEPTION(job_batch(0), std::invalid_argument) ;
    CPPUNIT_LOG_EXCEPTION(job_batch(5, 0), std::invalid_argument) ;
    CPPUNIT_LOG_EXCEPTION(job_batch(0, 0), std::invalid_argument) ;
    CPPUNIT_LOG_EXCEPTION(job_batch(0, 5), std::invalid_argument) ;

    CPPUNIT_LOG_EXCEPTION(job_batch(1, "TooLongBatchName"), std::out_of_range) ;

    job_batch b1 (1) ;
    job_batch b2 (5, 3) ;
    job_batch b3 (20, "Name3") ;
    job_batch b4 (8, 2, "NameOfMaxLength") ;

    CPPUNIT_LOG_EQ(strslice(b1.name()), "") ;
    CPPUNIT_LOG_EQ(strslice(b3.name()), "Name3") ;
    CPPUNIT_LOG_EQ(strslice(b4.name()), "NameOfMaxLength") ;

    CPPUNIT_LOG(std::endl) ;
    unsigned counter = 2 ;
    {
        job_batch b5 (1) ;
        CPPUNIT_LOG_EQ(b5.size(), 0) ;

        // Add a job but do not run
        CPPUNIT_LOG_RUN(b5.add_job([&counter](unsigned inc) { counter += inc ; }, 9)) ;
        CPPUNIT_LOG_EQ(b5.size(), 0) ;
    }
    CPPUNIT_LOG_EQUAL(counter, 2U) ;
}

struct test_error : std::runtime_error {
    using std::runtime_error::runtime_error ;
} ;

void JobBatchTests::Test_JobBatch_Run()
{
    CPPUNIT_LOG_EXPRESSION(INIT_THREADCOUNT) ;
    CPPUNIT_LOG_EQUAL(get_threadcount(), INIT_THREADCOUNT) ;

    {
        // "Run" the empty batch
        job_batch b1 (1) ;
        CPPUNIT_LOG_EQ(b1.size(), 0) ;
        CPPUNIT_LOG_ASSERT(b1.run()) ;
        CPPUNIT_LOG_EQ(b1.size(), 0) ;
        CPPUNIT_LOG_ASSERT(b1.try_wait()) ;
        CPPUNIT_LOG_RUN(b1.wait()) ;
    }

    CPPUNIT_LOG(std::endl) ;
    {
        job_batch b2 (1) ;
        CPPUNIT_LOG_EQ(b2.size(), 0) ;

        unsigned counter = 2 ;

        CPPUNIT_LOG_RUN(b2.add_job([&counter](unsigned inc) { counter += inc ; }, 9)) ;

        CPPUNIT_LOG_EQ(b2.size(), 0) ;
        CPPUNIT_LOG_RUN(b2.run()) ;
        CPPUNIT_LOG_EQ(b2.size(), 1) ;
        CPPUNIT_LOG_RUN(b2.wait()) ;
        CPPUNIT_LOG_EQ(b2.size(), 1) ;
        CPPUNIT_LOG_ASSERT(b2.try_wait()) ;

        CPPUNIT_LOG_EQUAL(counter, 11U) ;
    }

    CPPUNIT_LOG(std::endl) ;
    {
        job_batch b3 (2, "Hello") ;
        CPPUNIT_LOG_EQ(b3.size(), 0) ;

        pthread::id t1, t2 ;
        CPPUNIT_LOG_EQUAL(t1, t2) ;

        for (pthread::id *tid: {&t1, &t2})
        {
            auto test_job = [&tid=*tid]
            {
                std::this_thread::sleep_for(100ms) ;

                CPPUNIT_LOG_EXPRESSION(get_threadcount()) ;
                PCOMN_VERIFY(get_threadcount() == INIT_THREADCOUNT + 2) ;

                tid = pthread::id::this_thread() ;

                CPPUNIT_LOG_EXPRESSION(get_thread_name()) ;
                PCOMN_VERIFY(get_thread_name() == "Hello") ;
            } ;

            b3.add_job(test_job) ;
        }

        CPPUNIT_LOG_RUN(b3.wait()) ;

        CPPUNIT_LOG_EQ(b3.size(), 2) ;

        CPPUNIT_LOG_ASSERT(t1) ;
        CPPUNIT_LOG_ASSERT(t2) ;
        CPPUNIT_LOG_NOT_EQUAL(t1, t2) ;

        CPPUNIT_LOG_RUN(b3.wait()) ;
    }

    CPPUNIT_LOG(std::endl) ;
    {
        job_batch b4 (6, 3, "Multi") ;

        counting_semaphore sem ;
        std::vector<std::future<std::string>> results ;

        auto test_job = [&sem](const char *name)
        {
            CPPUNIT_LOG_LINE("    Task " << squote(name) << " has started") ;

            if (*name == '0')
            {
                std::this_thread::sleep_for(15ms) ;
                CPPUNIT_LOG_EXPRESSION(get_threadcount()) ;
                sem.release() ;
                CPPUNIT_LOG_LINE("    Task " << squote(name) << " has thrown exception") ;

                throw test_error(name) ;
            }

            std::this_thread::sleep_for(20ms) ;
            CPPUNIT_LOG_EXPRESSION(get_threadcount()) ;
            sem.release() ;
            CPPUNIT_LOG_LINE("    Task " << squote(name) << " has finished") ;

            return std::string(name) ;
        } ;

        for (const char *s: {"001", "two", "003", "four"})
            CPPUNIT_LOG_RUN(b4.add_job(test_job, s)) ;

        for (const char *s: {"005", "six", "007"})
            CPPUNIT_LOG_RUN(results.push_back(b4.add_task(test_job, s))) ;

        for (const char *s: {"eight", "009"})
            CPPUNIT_LOG_RUN(b4.add_job(test_job, s)) ;

        for (const char *s: {"ten", "011"})
            CPPUNIT_LOG_RUN(results.push_back(b4.add_task(test_job, s))) ;

        CPPUNIT_IS_FALSE(b4.run()) ;

        CPPUNIT_LOG_RUN(sem.acquire(5)) ;
        CPPUNIT_LOG_EQ(b4.size(), 4) ;
        CPPUNIT_LOG_RUN(b4.stop()) ;
        CPPUNIT_LOG_RUN(b4.wait()) ;

        const size_t finished = std::count_if(results.begin(), results.end(), [](auto &r)
        {
            return r.wait_for(0ns) == std::future_status::ready ;
        }) ;

        CPPUNIT_LOG_EXPRESSION(finished) ;
        CPPUNIT_LOG_ASSERT(finished) ;
        CPPUNIT_LOG_ASSERT(finished < results.size()) ;

        CPPUNIT_LOG_EXCEPTION_MSG(results[0].get(), test_error, "005") ;
        if (finished > 1)
            CPPUNIT_LOG_EQ(results[1].get(), "six") ;
        if (finished > 2)
            CPPUNIT_LOG_EXCEPTION_MSG(results[2].get(), test_error, "007") ;
        if (finished > 3)
            CPPUNIT_LOG_EQ(results[3].get(), "ten") ;
        if (finished > 4)
            CPPUNIT_LOG_EXCEPTION_MSG(results[4].get(), test_error, "011") ;
    }

    // Check for movable-only result
    CPPUNIT_LOG(std::endl) ;
    {
        job_batch b5 (2, "Movable") ;

        std::future<std::unique_ptr<std::string>> result ;

        auto test_job = [](const char *name)
        {
            CPPUNIT_LOG_LINE("    Task " << squote(name) << " has started") ;

            std::this_thread::sleep_for(20ms) ;
            CPPUNIT_LOG_EXPRESSION(get_threadcount()) ;
            CPPUNIT_LOG_LINE("    Task " << squote(name) << " has finished") ;

            return std::make_unique<std::string>(name) ;
        } ;

        CPPUNIT_LOG_RUN(result = b5.add_task(test_job, "Hello!")) ;
        CPPUNIT_LOG_RUN(b5.wait()) ;
        CPPUNIT_LOG_ASSERT(result.wait_for(0ns) == std::future_status::ready) ;
        CPPUNIT_LOG_EQ(*result.get(), "Hello!") ;
    }
}

/*******************************************************************************
 ThreadPoolTests
*******************************************************************************/
void ThreadPoolTests::Test_ThreadPool_Init()
{
    CPPUNIT_LOG_EQUAL(get_threadcount(), INIT_THREADCOUNT) ;
    CPPUNIT_LOG_EXCEPTION(threadpool(1, "TooLongTPoolName"), std::out_of_range) ;
    CPPUNIT_LOG_EQUAL(get_threadcount(), INIT_THREADCOUNT) ;

    {
        threadpool p1 (1) ;

        CPPUNIT_LOG_EXPRESSION(p1) ;
        CPPUNIT_LOG_EQ(p1.size(), 1) ;
        CPPUNIT_LOG_EQUAL(strslice(p1.name()), strslice()) ;
        // At least the first thread of the pool is started by the constructor
        CPPUNIT_LOG_EQUAL(get_threadcount(), INIT_THREADCOUNT + 1U) ;
    }

    CPPUNIT_LOG(std::endl) ;
    {
        threadpool p4 (4, "Pool4", 8192) ;

        CPPUNIT_LOG_EXPRESSION(p4) ;

        CPPUNIT_LOG_EQ(p4.size(), 4) ;
        CPPUNIT_LOG_EQ(strslice(p4.name()), "Pool4") ;

        CPPUNIT_LOG_EQ(p4.max_queue_capacity(), 8192) ;
        CPPUNIT_LOG_EQ(p4.queue_capacity(), 8192) ;
        CPPUNIT_LOG_EQ(p4.capacity(), 8196) ;

        CPPUNIT_LOG_EXCEPTION(p4.set_queue_capacity(0), std::out_of_range) ;
        CPPUNIT_LOG_EQ(p4.queue_capacity(), 8192) ;
        CPPUNIT_LOG_EXCEPTION(p4.set_queue_capacity(8193), std::out_of_range) ;

        CPPUNIT_LOG_RUN(std::this_thread::sleep_for(20ms)) ;
        CPPUNIT_LOG_EXPRESSION(p4) ;
        CPPUNIT_LOG_EQUAL(get_threadcount(), INIT_THREADCOUNT + 4U) ;

        CPPUNIT_LOG(std::endl) ;
        CPPUNIT_LOG_RUN(p4.resize(20)) ;
        CPPUNIT_LOG_EQ(p4.size(), 20) ;
        CPPUNIT_LOG_EXPRESSION(p4) ;
        CPPUNIT_LOG_RUN(std::this_thread::sleep_for(20ms)) ;
        CPPUNIT_LOG_EXPRESSION(p4) ;
        CPPUNIT_LOG_EQUAL(get_threadcount(), INIT_THREADCOUNT + 20U) ;

        CPPUNIT_LOG(std::endl) ;
        CPPUNIT_LOG_RUN(p4.resize(1)) ;
        CPPUNIT_LOG_EQ(p4.size(), 1) ;
        CPPUNIT_LOG_EXPRESSION(p4) ;
        CPPUNIT_LOG_RUN(std::this_thread::sleep_for(20ms)) ;
        CPPUNIT_LOG_EXPRESSION(p4) ;
    }
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
        (argc, argv, "unittest.diag.ini") ;
}
