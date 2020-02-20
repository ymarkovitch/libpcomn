/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
#pragma once
/*******************************************************************************
 FILE         :   unittest_semaphore.h
 COPYRIGHT    :   Yakov Markovitch, 2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for counting and binary semaphores.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   17 Feb 2020
*******************************************************************************/
#include <pcomn_unittest_mt.h>
#include <pcomn_semaphore.h>

#include <thread>
#include <chrono>
#include <random>
#include <numeric>

#include <stdlib.h>
#include <pthread.h>
#include <math.h>

using namespace pcomn ;
using namespace std::chrono ;

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
    unit::watchdog watchdog {10s} ;

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
