/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   unittest_semaphore.cpp
 COPYRIGHT    :   Yakov Markovitch, 2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for counting and binary semaphores.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   17 Feb 2020
*******************************************************************************/
#define CPPUNIT_USE_SYNC_LOGSTREAM

#include <pcomn_unittest.h>
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
 Watchdog
*******************************************************************************/
class Watchdog {
public:
    explicit Watchdog(milliseconds timeout) :
        _timeout(timeout)
    {}

    void arm() ;
    void disarm() ;

private:
    milliseconds     _timeout ;
    std::timed_mutex _armed ;
    std::thread      _watchdog ;
} ;

void Watchdog::arm()
{
    CPPUNIT_ASSERT(!_watchdog.joinable()) ;

    _armed.lock() ;

    _watchdog = std::thread([&]{
        if (!_armed.try_lock_for(_timeout))
        {
            CPPUNIT_LOG_LINE("ERROR: THE TEST DEADLOCKED") ;
            exit(3) ;
        }
    }) ;
}

void Watchdog::disarm()
{
    CPPUNIT_ASSERT(_watchdog.joinable()) ;

    _armed.unlock() ;
    _watchdog.join() ;
    _watchdog = {} ;
}

struct geometric_distributed_range {
    geometric_distributed_range(unsigned lo, unsigned hi, double p) :
        _generator(p),
        _offset((PCOMN_VERIFY(lo <= hi), lo)),
        _hibound(hi - lo)
    {
        PCOMN_VERIFY(hi != 0) ;
    }

    unsigned operator()()
    {
        unsigned v ;
        // Chop off the tail
        while((v = _generator(_random_engine)) > _hibound) ;
        return v + _offset ;
    }

    static std::random_device seed_device ;

private:
    std::mt19937 _random_engine {seed_device()} ;
    std::geometric_distribution<unsigned> _generator ;
    unsigned _offset ;
    unsigned _hibound ;
} ;

std::random_device geometric_distributed_range::seed_device ;

/*******************************************************************************
 class MutexTests
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
    Watchdog watchdog {3s} ;

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

const unsigned SemaphoreTests::maxcount ;

/*******************************************************************************
 SemaphoreMTFixture
*******************************************************************************/
class SemaphoreMTFixture : public CppUnit::TestFixture {
public:
    class tester_thread ;
    typedef std::unique_ptr<tester_thread> tester_thread_ptr ;

    enum TesterMode : bool {
        Producer,
        Consumer
    } ;

    static std::random_device &seed_random_device()
    {
        return geometric_distributed_range::seed_device ;
    }

    static unsigned random_seed() { return seed_random_device()() ; }

    void setUp()
    {
        watchdog.arm() ;
    }

    void tearDown()
    {
        join_producers() ;
        join_consumers() ;

        watchdog.disarm() ;

        _producers.clear() ;
        _consumers.clear() ;
    }

    /***************************************************************************
     tester_thread
    ***************************************************************************/
    class tester_thread final {
    public:
        geometric_distributed_range _generate ;
        std::mt19937                _random_engine {random_seed()} ;
        std::uniform_int_distribution<uint64_t> _generate_pause ;

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

        nanoseconds generate_pause()
        {
            return nanoseconds(_generate_pause(_random_engine)) ;
        }
    private:
        void produce() ;
        void consume() ;
    } ;

private:
    Watchdog watchdog {10s} ;

protected:
    std::vector<tester_thread_ptr> _producers ;
    std::vector<tester_thread_ptr> _consumers ;

protected:
    void join_producers()
    {
        join_tester_threads(_producers, "producers") ;
    }
    void join_consumers()
    {
        join_tester_threads(_consumers, "consumers") ;
    }


private:
    void join_tester_threads(const simple_slice<tester_thread_ptr> &testers,
                             const strslice &what) ;
} ;

/*******************************************************************************
 SemaphoreMTFixture
*******************************************************************************/
void SemaphoreMTFixture::join_tester_threads(const simple_slice<tester_thread_ptr> &testers,
                                             const strslice &what)
{
    const auto is_joinable = [](const auto &t) { return t && t->_thread.joinable() ; } ;

    const unsigned joinable_count = std::count_if(testers.begin(), testers.end(), is_joinable) ;
    if (!joinable_count)
        return ;

    CPPUNIT_LOG_LINE("Join " << joinable_count << " " << what << " of " << testers.size()) ;

    for (const auto &t: testers)
        if (is_joinable(t))
            t->_thread.join() ;

    CPPUNIT_LOG_LINE("Joined " << joinable_count << " " << what) ;
}

/*******************************************************************************
 SemaphoreMTFixture::tester_thread
*******************************************************************************/
SemaphoreMTFixture::tester_thread::
tester_thread(TesterMode mode, counting_semaphore &semaphore,
              unsigned volume, double p, nanoseconds max_pause) :

    _generate(1, volume, p),
    _generate_pause(0, max_pause.count()),
    _volume(volume),
    _semaphore(semaphore),
    _thread([this, mode] { mode == Consumer ? consume() : produce() ; })
{}

void SemaphoreMTFixture::tester_thread::produce()
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

void SemaphoreMTFixture::tester_thread::consume()
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
                            class SemaphoreFuzzyTests
*******************************************************************************/
class SemaphoreFuzzyTests : public SemaphoreMTFixture {

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
} ;

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
        consumer->_stop = true ;

    for (unsigned i = consumers ; i-- ;)
    {
        std::this_thread::sleep_for(consumers_timeout) ;
        semaphore.release(maxcount - semaphore.borrow(0)) ;
    }
    join_consumers() ;

    const auto eval_total = [](auto &testers, size_t init)
    {
        return std::accumulate(testers.begin(), testers.end(), init,
                               [](size_t total, const auto &p) { return total + p->_total ; }) ;
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
 SemaphoreTests
*******************************************************************************/
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
