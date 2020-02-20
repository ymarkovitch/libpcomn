/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   unittest_semaphore.cpp
 COPYRIGHT    :   Yakov Markovitch, 2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for counting and binary semaphores.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   17 Feb 2020
*******************************************************************************/
#include "unittest_semaphore.h"

#include <pcomn_semaphore.h>
#include <pcomn_stopwatch.h>

#include <thread>
#include <chrono>
#include <random>
#include <numeric>

#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

using namespace pcomn ;
using namespace std::chrono ;

std::atomic<pthread_t> signaled_id ;

static void sigusr2_handler(int signo)
{
    static constexpr char msg[] = "\nReceived SIGUSR2\n" ;

    if (signo == SIGUSR2)
    {
        signaled_id.store(pthread_self(), std::memory_order_release) ;
        write(STDOUT_FILENO, msg, sizeof(msg) - 1) ;
    }
}

/*******************************************************************************
                            class SemaphoreTests
*******************************************************************************/
class SemaphoreTests : public CppUnit::TestFixture {

    void Test_Semaphore_Limits() ;
    void Test_Semaphore_EINTR() ;
    void Test_Semaphore_SingleThreaded() ;

    CPPUNIT_TEST_SUITE(SemaphoreTests) ;

    CPPUNIT_TEST(Test_Semaphore_Limits) ;
    CPPUNIT_TEST(Test_Semaphore_SingleThreaded) ;
    CPPUNIT_TEST(Test_Semaphore_EINTR) ;

    CPPUNIT_TEST_SUITE_END() ;

private:
    unit::watchdog watchdog {3s} ;

    static const unsigned maxcount = counting_semaphore::max_count() ;

public:
    void setUp()
    {
        signaled_id = {} ;
        watchdog.arm() ;
    }
    void tearDown()
    {
        watchdog.disarm() ;
        reset_sighandler() ;
    }

    static void set_sighandler()
    {
        CPPUNIT_ASSERT(signal(SIGUSR2, sigusr2_handler) != SIG_ERR) ;
    }

    static void reset_sighandler() { signal(SIGUSR2, SIG_DFL) ; }

    static void send_sigusr2(pthread_t tid)
    {
        PCOMN_ENSURE_ENOERR(pthread_kill(tid, SIGUSR2), "pthread_kill") ;
    }
} ;

/*******************************************************************************
                            class SemaphoreFuzzyTests
*******************************************************************************/
class SemaphoreFuzzyTests : public ProducerConsumerFixture {
    typedef ProducerConsumerFixture ancestor ;

    template<unsigned producers, unsigned consumers,
             unsigned pcount, unsigned max_pause_nano>
    void RunTest()
    {
        run(producers, consumers, pcount, max_pause_nano) ;
    }

    CPPUNIT_TEST_SUITE(SemaphoreFuzzyTests) ;

    CPPUNIT_TEST(P_PASS(RunTest<1,1,1,0>)) ;
    CPPUNIT_TEST(P_PASS(RunTest<1,1,1000,0>)) ;
    CPPUNIT_TEST(P_PASS(RunTest<1,1,2'000'000,0>)) ;
    CPPUNIT_TEST(P_PASS(RunTest<2,2,2'000'000,0>)) ;
    CPPUNIT_TEST(P_PASS(RunTest<2,1,1'000'000,1000>)) ;
    CPPUNIT_TEST(P_PASS(RunTest<2,2,2'000'000,1000>)) ;
    CPPUNIT_TEST(P_PASS(RunTest<2,5,10'000'000,0>)) ;
    CPPUNIT_TEST(P_PASS(RunTest<10,10,1'000'000,0>)) ;
    CPPUNIT_TEST(P_PASS(RunTest<10,10,1'000'000,100>)) ;

    CPPUNIT_TEST_SUITE_END() ;

private:
    static const unsigned maxcount = counting_semaphore::max_count() ;

    void run(unsigned producers, unsigned consumers, unsigned pcount, unsigned max_pause_nano) ;

public:
    SemaphoreFuzzyTests() : ancestor(10s) {}

    /***************************************************************************
     tester_thread
    ***************************************************************************/
    class tester_thread final : public ancestor::tester_thread {
        typedef ancestor::tester_thread base ;
    public:
        std::vector<unsigned>   _produced ; /* <0 means attempt with overflow */
        std::vector<unsigned>   _consumed ;  /* <0 means borrow */
        uint64_t                _volume = 0 ;
        uint64_t                _remains = _volume ;
        uint64_t                _total = 0 ;
        counting_semaphore &    _semaphore ;
        std::atomic_bool        _stop {false} ;
        std::thread             _thread ;

        tester_thread(TesterMode mode, counting_semaphore &semaphore, unsigned volume, double p,
                      nanoseconds max_pause = {}) ;

        std::thread &self() final { return _thread ; }

    private:
        void produce() final ;
        void consume() final ;
    } ;

    static tester_thread &tester(const tester_thread_ptr &pt)
    {
        PCOMN_VERIFY(pt) ;
        return *static_cast<tester_thread *>(pt.get()) ;
    }
} ;

/*******************************************************************************
 SemaphoreFuzzyTests::tester_thread
*******************************************************************************/
SemaphoreFuzzyTests::tester_thread::tester_thread(TesterMode mode, counting_semaphore &semaphore,
                                                  unsigned volume, double p, nanoseconds max_pause) :
    base(p, volume, max_pause),
    _volume(volume),
    _semaphore(semaphore)
{
    init(mode) ;
}

void SemaphoreFuzzyTests::tester_thread::produce()
{
    CPPUNIT_LOG_LINE("Start producer " << HEXOUT(_thread.get_id()) << ", must produce " << _remains << " items.") ;

    while (_remains && !_stop.load(std::memory_order_acquire))
    {
        const nanoseconds pause = generate_pause() ;
        if (pause != nanoseconds())
            std::this_thread::sleep_for(pause) ;

        const unsigned count = std::min<uint64_t>(_remains, _generate()) ;
        PCOMN_VERIFY(count > 0) ;

        _semaphore.release(count) ;
        _produced.push_back(count) ;
        _total += count ;
        _remains -= count ;
    }

    CPPUNIT_LOG_LINE("Finish producer " << HEXOUT(_thread.get_id())
                     << ", produced " << _total << " items in " << _produced.size() << " slots, "
                     << _remains << " remains.") ;
}

void SemaphoreFuzzyTests::tester_thread::consume()
{
    CPPUNIT_LOG_LINE("Start consumer " << HEXOUT(_thread.get_id())) ;

    while (!_stop.load(std::memory_order_acquire))
    {
        const unsigned count = std::min<uint64_t>(_volume, _generate()) ;
        const unsigned consumed = _semaphore.acquire(count) ;

        PCOMN_VERIFY(consumed == count) ;

        if (_stop.load(std::memory_order_acquire))
            break ;

        _consumed.push_back(consumed) ;
        _total += consumed ;

        const nanoseconds pause = generate_pause() ;
        if (pause != nanoseconds())
            std::this_thread::sleep_for(pause) ;
    }

    CPPUNIT_LOG_LINE("Finish consumer " << HEXOUT(_thread.get_id())
                     << ", consumed " << _total << " items in " << _consumed.size() << " slots.") ;
}

/*******************************************************************************
 SemaphoreTests
*******************************************************************************/
const unsigned SemaphoreTests::maxcount ;

void SemaphoreTests::Test_Semaphore_Limits()
{
    // Costructing with invalid count
    CPPUNIT_LOG_EXCEPTION(counting_semaphore(maxcount + 1), std::system_error) ;
    CPPUNIT_LOG_EXCEPTION(counting_semaphore(-1), std::system_error) ;

    // Costructing with valid count (borrow(0) returns current count)
    CPPUNIT_LOG_EQ(counting_semaphore().borrow(0), 0) ;
    CPPUNIT_LOG_EQ(counting_semaphore(1).borrow(0), 1) ;
    CPPUNIT_LOG_EQ(counting_semaphore(maxcount).borrow(0), counting_semaphore::max_count()) ;

    CPPUNIT_LOG(std::endl) ;

    counting_semaphore cs0 ;
    counting_semaphore cs1 ;

    CPPUNIT_LOG_RUN(cs1.release(maxcount)) ;
    CPPUNIT_LOG_EQ(cs1.borrow(0), maxcount) ;

    CPPUNIT_LOG_EXCEPTION(cs1.release(1), std::system_error) ;
    CPPUNIT_LOG_RUN(cs1.release(0)) ;
    CPPUNIT_LOG_EXCEPTION(cs1.release(), std::system_error) ;

    CPPUNIT_LOG_EQ(cs1.borrow(0), maxcount) ;

    CPPUNIT_LOG_EXCEPTION(cs0.acquire(maxcount + 1), std::system_error) ;
    CPPUNIT_LOG_EQ(cs0.borrow(0), 0) ;
    CPPUNIT_LOG_EXCEPTION(cs0.try_acquire(maxcount + 1), std::system_error) ;
    CPPUNIT_LOG_EQ(cs0.borrow(0), 0) ;

    CPPUNIT_LOG_IS_FALSE(cs0.try_acquire_some(maxcount + 1)) ;
    CPPUNIT_LOG_EQ(cs0.borrow(0), 0) ;

    CPPUNIT_LOG_EQ(cs1.borrow(0), maxcount) ;
    CPPUNIT_LOG_EQ(cs1.acquire_some(maxcount + 10), maxcount) ;
    CPPUNIT_LOG_EQ(cs1.borrow(0), 0) ;

    CPPUNIT_LOG_EXCEPTION(cs1.borrow(maxcount + 1), std::system_error) ;
    CPPUNIT_LOG_EQ(cs1.borrow(0), 0) ;
    CPPUNIT_LOG_RUN(cs1.release(maxcount)) ;
    CPPUNIT_LOG_EQ(cs1.borrow(maxcount + 1), maxcount) ;
    CPPUNIT_LOG_EQ(cs1.borrow(1), -1) ;
    CPPUNIT_LOG_RUN(cs1.release(2)) ;

    CPPUNIT_LOG_EXCEPTION(cs0.try_acquire(maxcount + 1), std::system_error) ;
    CPPUNIT_LOG_EQ(cs0.borrow(0), 0) ;

    CPPUNIT_LOG(std::endl) ;

    counting_semaphore cs2 ;
    CPPUNIT_LOG_EQ(cs1.borrow(0), 0) ;
    CPPUNIT_LOG_EQ(cs2.borrow(0), 0) ;

    CPPUNIT_LOG_IS_FALSE(cs1.try_acquire(20)) ;
    CPPUNIT_LOG_EQ(cs1.borrow(20), 0) ;
    CPPUNIT_LOG_EQ(cs1.borrow(1), -20) ;

    CPPUNIT_LOG_IS_FALSE(cs1.try_acquire()) ;
    CPPUNIT_LOG_RUN(cs1.release(9)) ;
    CPPUNIT_LOG_IS_FALSE(cs1.try_acquire()) ;
    CPPUNIT_LOG_RUN(cs1.release(11)) ;
    CPPUNIT_LOG_IS_FALSE(cs1.try_acquire()) ;
    CPPUNIT_LOG_RUN(cs1.release()) ;
    CPPUNIT_LOG_IS_FALSE(cs1.try_acquire()) ;

    CPPUNIT_LOG_RUN(cs1.release()) ;
    CPPUNIT_LOG_ASSERT(cs1.try_acquire()) ;
    CPPUNIT_LOG_IS_FALSE(cs1.try_acquire()) ;

    CPPUNIT_LOG_RUN(cs1.release(20)) ;
    CPPUNIT_LOG_EQ(cs1.borrow(5), 20) ;
    CPPUNIT_LOG_EQ(cs1.acquire_some(-1), 15) ;
    CPPUNIT_LOG_IS_FALSE(cs1.try_acquire()) ;
}

void SemaphoreTests::Test_Semaphore_SingleThreaded()
{
    counting_semaphore cs0 ;
    CPPUNIT_LOG_IS_FALSE(cs0.try_acquire_for(50ms)) ;
}

void SemaphoreTests::Test_Semaphore_EINTR()
{
    const pthread_t self = pthread_self() ;

    // Check functioning of the fixture itself
    CPPUNIT_LOG_ASSERT(self) ;

    set_sighandler() ;

    CPPUNIT_LOG_ASSERT(!signaled_id.load(std::memory_order_acquire)) ;
    CPPUNIT_LOG_RUN(send_sigusr2(pthread_self())) ;

    CPPUNIT_LOG_EQUAL(signaled_id.load(std::memory_order_acquire), self) ;

    CPPUNIT_LOG(std::endl) ;
    set_sighandler() ;
}

/*******************************************************************************
 SemaphoreFuzzyTests
*******************************************************************************/
void SemaphoreFuzzyTests::run(unsigned producers, unsigned consumers,
                              unsigned pcount, unsigned max_pause_nano)
{
    const uint64_t total_volume = pcount*producers ;
    const nanoseconds max_pause (max_pause_nano) ;

    const nanoseconds consumers_timeout (std::max(consumers*100*max_pause, nanoseconds(50ms))) ;

    CPPUNIT_LOG_LINE("Running " << producers << " producers, " << consumers << " consumers, "
                     << total_volume << " total items (" << pcount << " per producer), "
                     << "max pause "<< (duration<double,std::micro>(max_pause).count()) << "ms") ;

    PRealStopwatch wall_time ;
    PCpuStopwatch  cpu_time ;

    counting_semaphore semaphore ;

    wall_time.start() ;
    cpu_time.start() ;

    const auto make_testers = [&](TesterMode mode, auto &testers, size_t count)
    {
        CPPUNIT_ASSERT(testers.empty()) ;
        for (testers.reserve(count) ; count ; --count)
        {
            testers.emplace_back(new tester_thread(mode, semaphore, pcount, 0.01, max_pause)) ;
        }
    } ;

    make_testers(Consumer, _consumers, consumers) ;
    make_testers(Producer, _producers, producers) ;

    join_producers() ;

    // Wait until the quiescent state
    unsigned pending = 0 ;
    for (unsigned p ; (p = semaphore.borrow(0)) != pending ; std::this_thread::sleep_for(consumers_timeout))
        pending = p ;

    CPPUNIT_LOG_LINE("Stopping consumers, " << pending << " items pending.") ;

    for (auto &consumer: _consumers)
        tester(consumer)._stop = true ;

    for (unsigned i = consumers ; i-- ;)
    {
        std::this_thread::sleep_for(consumers_timeout) ;
        semaphore.release(maxcount - semaphore.borrow(0)) ;
    }
    join_consumers() ;

    const auto eval_total = [](auto &testers, size_t init)
    {
        return std::accumulate(testers.begin(), testers.end(), init,
                               [](size_t total, const auto &p) { return total + tester(p)._total ; }) ;
    } ;

    cpu_time.stop() ;
    wall_time.stop() ;

    const size_t total_produced = eval_total(_producers, 0) ;
    const size_t total_consumed = eval_total(_consumers, pending) ;

    CPPUNIT_LOG_LINE("Finished in " << wall_time << " real time, " << cpu_time << " CPU time.") ;
    CPPUNIT_LOG_LINE(total_produced << " produced, " <<  total_consumed << " consumed, "
                     << '(' << pending << " pending), " << total_volume << " expected.") ;

    CPPUNIT_LOG_EQUAL(total_produced, total_volume) ;
    CPPUNIT_LOG_EQUAL(total_consumed, total_produced) ;
}

/*******************************************************************************
 main
*******************************************************************************/
int main(int argc, char *argv[])
{
    return pcomn::unit::run_tests
        <
            SemaphoreTests,
            SemaphoreFuzzyTests
        >
        (argc, argv) ;
}
