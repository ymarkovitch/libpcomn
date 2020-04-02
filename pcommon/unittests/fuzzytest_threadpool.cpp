/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   fuzzytest_threadpool.cpp
 COPYRIGHT    :   Yakov Markovitch, 2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   2 Apr 2020
*******************************************************************************/
#include "unittest_semaphore.h"

#include <pcomn_threadpool.h>
#include <pcomn_stopwatch.h>
#include <pcomn_calgorithm.h>

#include <thread>
#include <map>
#include <random>
#include <numeric>

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

using namespace pcomn ;
using namespace std::chrono ;

static size_t INIT_THREADCOUNT ;

/*******************************************************************************
                            class ThreadPoolFuzzyTests
*******************************************************************************/
class ThreadPoolFuzzyTests : public CppUnit::TestFixture {

    void Test_ThreadPool_MultiDynamicResize() ;

    CPPUNIT_TEST_SUITE(ThreadPoolFuzzyTests) ;

    CPPUNIT_TEST(Test_ThreadPool_MultiDynamicResize) ;

    CPPUNIT_TEST_SUITE_END() ;

private:
    unit::watchdog watchdog {5s} ;

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

    void MultiDynamicResize(milliseconds test_duration,
                            size_t max_workers, size_t submitters_count,
                            size_t avg_vector_size, size_t result_queue_capacity) ;
} ;

/*******************************************************************************
 ThreadPoolFuzzyTests
*******************************************************************************/
struct test_result {
    size_t ndx = 0 ;
    std::vector<long> data ;
} ;

struct test_plan {
    size_t ndx ;
    long start ;
    unsigned length ;

    static test_result test_job(size_t ndx, std::vector<long> &data)
    {
        return test_result{ndx, std::move(pcomn::sort(data))} ;
    }
} ;

typedef threadpool::result_ptr<test_result>   test_result_ptr ;
typedef threadpool::result_queue<test_result> test_result_queue ;

template<typename T>
struct uniform_distributed_range : std::uniform_int_distribution<T> {

    using std::uniform_int_distribution<T>::uniform_int_distribution ;
    using std::uniform_int_distribution<T>::operator() ;

    T operator()() { return (*this)(_random_engine) ; }

    static thread_local std::random_device seed_device ;

private:
    std::mt19937 _random_engine {seed_device()} ;
} ;

template<typename T>
thread_local std::random_device uniform_distributed_range<T>::seed_device ;

void ThreadPoolFuzzyTests::MultiDynamicResize(milliseconds test_duration,
                                              size_t max_workers, size_t submitters_count,
                                              size_t max_input_capacity, size_t result_queue_capacity)
{
    CPPUNIT_LOG_LINE('\n'
                     << "########################################################################################\n"
                     << "# " << test_duration.count() << "ms, "
                     << max_workers << " workers, "
                     << submitters_count << " submitters, "
                     << max_input_capacity << " max task queue capacity, "
                     << result_queue_capacity << " result queue capacity\n"
                     << "########################################################################################") ;

    CPPUNIT_LOG_ASSERT(test_duration > 0ms) ;
    CPPUNIT_LOG_ASSERT(test_duration <= watchdog.timeout()/2) ;
    CPPUNIT_LOG_ASSERT(max_input_capacity) ;
    CPPUNIT_LOG_ASSERT(submitters_count) ;
    CPPUNIT_LOG_ASSERT(max_workers) ;
    CPPUNIT_LOG_ASSERT(result_queue_capacity) ;

    /***************************************************************************
     Pool
    ***************************************************************************/
    threadpool pool (max_workers, "FuzzyPool", max_input_capacity) ;

    CPPUNIT_LOG_RUN(pool.set_queue_capacity(max_input_capacity)) ;
    CPPUNIT_LOG_EQ(pool.size(), max_workers) ;
    CPPUNIT_LOG_EQ(pool.queue_capacity(), max_input_capacity) ;

    /***************************************************************************
     Fuzzer thread
    ***************************************************************************/
    std::atomic<bool> stop_test {false} ;
    const steady_clock::time_point h_hour = steady_clock::now() + test_duration ;

    pthread fuzzer (pthread::F_AUTOJOIN, [&]
    {
        CPPUNIT_LOG_LINE("Fuzzer started") ;

        auto select_wait_interval = [dist=uniform_distributed_range<int>(-1000, 10000)]() mutable
        {
            return microseconds(std::clamp(dist(), 0, 10000)) ;
        } ;

        uniform_distributed_range<unsigned> select_change (1, 3) ;
        uniform_distributed_range<unsigned> select_threadcount (0, max_workers) ;
        uniform_distributed_range<unsigned> select_queue_capacity (1, max_input_capacity) ;

        for (std::this_thread::sleep_for(select_wait_interval()) ;
             steady_clock::now() < h_hour ;
             std::this_thread::sleep_for(select_wait_interval()))
        {
            const unsigned mask = select_change() ;
            if (mask & 1)
                pool.resize(select_threadcount()) ;

            if (mask & 2)
                pool.set_queue_capacity(select_queue_capacity()) ;
        }
        CPPUNIT_LOG_RUN(pool.stop(true)) ;
        stop_test = true ;

        CPPUNIT_LOG_LINE("Fuzzer finished") ;
    }) ;

    /***************************************************************************
     Submitters
    ***************************************************************************/
    std::mutex plans_mutex ;
    std::atomic<size_t> next_ndx {1} ;
    std::vector<test_plan> plans ;

    std::vector<std::shared_ptr<test_result_queue>> outputs ;
    std::multimap<size_t, std::vector<long>> results ;

    // Create output queues
    for (size_t i = 0 ; i < submitters_count ; ++i)
        outputs.push_back(std::make_shared<test_result_queue>(result_queue_capacity)) ;

    // Task submitter threads
    job_batch submitters (submitters_count, "Submitter") ;

    for (unsigned i = 0 ; i < submitters_count ; ++i)
    {
        submitters.add_job([&,i,out=outputs[i]]
        {
            CPPUNIT_LOG_LINE("Submitter " << i << " started") ;

            auto select_wait_interval = [dist=uniform_distributed_range<int>(-10, 100)]() mutable
            {
                return microseconds(std::clamp(dist(), 0, 100)) ;
            } ;

            uniform_distributed_range<long> select_start (-1000'000, 999'999) ;
            uniform_distributed_range<unsigned> select_count (1, 256*1024) ;

            try {
                for(;;)
                {
                    test_plan test {next_ndx++, select_start(), select_count()} ;
                    std::vector<long> source (test.length) ;

                    std::iota(source.begin(), source.end(), test.start) ;
                    std::random_shuffle(source.begin(), source.end()) ;

                    pool.enqueue_job(&test_plan::test_job, test.ndx, std::move(source)) ;

                    PCOMN_SCOPE_LOCK(lock, plans_mutex) ;
                    plans.push_back(test) ;

                    std::this_thread::sleep_for(select_wait_interval()) ;
                }
            }
            catch (const object_closed &)
            {
                CPPUNIT_LOG_LINE("Input queue closed in submitter " << i) ;
            }

            CPPUNIT_LOG_LINE("Submitter " << i << " finsished") ;
        }) ;
    }

    auto get_results = [&]
    {
    } ;

    do {
        std::this_thread::sleep_for(50ms) ;
        get_results() ;
    }
    while (!stop_test) ;

    CPPUNIT_LOG_LINE("Stop requested") ;
    CPPUNIT_LOG_EXPRESSION(pool) ;

    std::this_thread::sleep_for(50ms) ;
    get_results() ;

    CPPUNIT_LOG_LINE("Stop the pool, wait for completion") ;
    CPPUNIT_LOG_EXPRESSION(pool) ;

    CPPUNIT_LOG_RUN(pool.stop(true)) ;

    CPPUNIT_LOG_RUN(fuzzer.join()) ;
}

void ThreadPoolFuzzyTests::Test_ThreadPool_MultiDynamicResize()
{
    MultiDynamicResize(300ms, 2, 1, 1024, 1'000'000) ;
}


/*******************************************************************************
 main
*******************************************************************************/
int main(int argc, char *argv[])
{
    return pcomn::unit::run_tests
        <
            ThreadPoolFuzzyTests
        >
        (argc, argv, "unittest.diag.ini") ;
}
