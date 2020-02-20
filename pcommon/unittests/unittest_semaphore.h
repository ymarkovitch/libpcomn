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
 ProducerConsumerFixture
*******************************************************************************/
class ProducerConsumerFixture : public CppUnit::TestFixture {
public:
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
    class tester_thread {
    public:
        virtual ~tester_thread() = default ;
        virtual std::thread &self() = 0 ;

        geometric_distributed_range _generate ;
        std::mt19937                _random_engine {random_seed()} ;
        std::uniform_int_distribution<uint64_t> _generate_pause ;

        nanoseconds generate_pause()
        {
            return nanoseconds(_generate_pause(_random_engine)) ;
        }

    protected:
        tester_thread(double p, unsigned max_count, nanoseconds max_pause) :
            _generate(1, max_count, p),
            _generate_pause(0, max_pause.count())
        {}

        void init(TesterMode mode)
        {
            std::thread &tester_self_thread = self() ;
            PCOMN_VERIFY(!tester_self_thread.joinable()) ;
            tester_self_thread = std::thread([this, mode] { mode == Consumer ? consume() : produce() ; }) ;
        }

    private:
        virtual void produce() = 0 ;
        virtual void consume() = 0 ;
    } ;

    typedef std::unique_ptr<tester_thread> tester_thread_ptr ;

private:
    unit::watchdog watchdog ;

protected:
    std::vector<tester_thread_ptr> _producers ;
    std::vector<tester_thread_ptr> _consumers ;

protected:
    explicit ProducerConsumerFixture(milliseconds watchdog_timeout) :
        watchdog(watchdog_timeout)
    {}

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
 ProducerConsumerFixture
*******************************************************************************/
void ProducerConsumerFixture::join_tester_threads(const simple_slice<tester_thread_ptr> &testers,
                                                  const strslice &what)
{
    const auto is_joinable = [](const auto &t) { return t && t->self().joinable() ; } ;

    const unsigned joinable_count = std::count_if(testers.begin(), testers.end(), is_joinable) ;
    if (!joinable_count)
        return ;

    CPPUNIT_LOG_LINE("Join " << joinable_count << " " << what << " of " << testers.size()) ;

    for (const auto &t: testers)
        if (is_joinable(t))
            t->self().join() ;

    CPPUNIT_LOG_LINE("Joined " << joinable_count << " " << what) ;
}
