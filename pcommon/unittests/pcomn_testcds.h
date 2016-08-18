/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_TESTCDS_H
#define __PCOMN_TESTCDS_H
/*******************************************************************************
 FILE         :   pcomn_testcds.h
 COPYRIGHT    :   Yakov Markovitch, 2016

 DESCRIPTION  :   Helpers for concurrent data structures testing

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   5 Jul 2016
*******************************************************************************/
/** @file
 Helper classes and functions for concurrent data structures testing.
*******************************************************************************/
#include <pcomn_unittest.h>
#include <pcomn_calgorithm.h>
#include <pcomn_stopwatch.h>
#include <pcomn_syncobj.h>
#include <pcomn_simplematrix.h>

#include <thread>
#include <vector>
#include <numeric>
#include <random>
#include <algorithm>

namespace pcomn {
namespace unit {

const size_t CDSTEST_COUNT_QUOTIENT = 3*5*7*9*11*16 ;

enum CdsTestFlags : unsigned {
   CDSTST_NOCHECK = 0x0001  /**< Don't check consistency */
} ;

template<typename T, size_t align, bool derive = std::is_class<T>::value>
struct alignas(align) aligned_type ;

template<class T, size_t align>
struct alignas(align) aligned_type<T, align, true> : T { using T::T ; } ;

template<typename T, size_t align>
struct alignas(align) aligned_type<T, align, false> {

   constexpr aligned_type() : value() {}
   constexpr aligned_type(T v) : value(v) {}

   constexpr operator T() const { return value ; }
   operator T&() { return value ; }

   T value ;
} ;

template<typename T>
using cache_aligned = aligned_type<T, PCOMN_CACHELINE_SIZE> ;

struct pause_distribution {
      pause_distribution(unsigned from, unsigned to) :
         _start(from),
         _multiplier((to - from + 7)/8),
         _distrib({10, 15, 25, 40, 70, 40, 25, 15, 10})
      {}

      unsigned operator()() { return _start + _distrib(_device)*_multiplier ; }

   private:
      const unsigned     _start ;
      const unsigned     _multiplier ;
      std::discrete_distribution<unsigned> _distrib ;
      std::random_device _device ;
} ;

/*******************************************************************************
 Check consistency of test results
*******************************************************************************/
template<typename T>
std::vector<size_t> CheckQueueResultConsistency(size_t producers_count, size_t items_per_producer, size_t enqueued_items,
                                                const std::vector<T> *bresults, const std::vector<T> *eresults)
{
   const size_t nmax_produced = items_per_producer*producers_count ;
   const size_t consumers = eresults - bresults ;

   CPPUNIT_LOG_LINE(("\nCHECK QUEUE RESULTS CONSISTENCY: ")
                    << producers_count << " producers, " << consumers << " consumer(s), " << enqueued_items
                    << " enqueued items, " << items_per_producer << " max per producer") ;

   CPPUNIT_ASSERT(consumers != 0) ;
   CPPUNIT_ASSERT(items_per_producer) ;
   CPPUNIT_ASSERT(enqueued_items) ;
   CPPUNIT_ASSERT(producers_count) ;
   CPPUNIT_LOG_ASSERT(enqueued_items <= nmax_produced) ;

   auto consnum = [=](const std::vector<T> *result) { return result - bresults + 1 ; } ;

   const size_t nconsumed =
      std::accumulate(bresults, eresults, 0UL, [](size_t a, const std::vector<T> &v)
      {
         CPPUNIT_LOG_LINE("Consumed " << v.size() << " items") ;
         return a + v.size() ;
      }) ;

   CPPUNIT_LOG_EQ(nconsumed, enqueued_items) ;

   typedef std::pair<const std::vector<T> *, ptrdiff_t> indicator_pair ;

   std::vector<indicator_pair> indicator (nmax_produced, {nullptr, -1}) ;
   std::vector<size_t> produced (producers_count) ;

   CPPUNIT_LOG_LINE("Checking every produced item is present in the result exactly once:") ;

   for (auto result = bresults ; result != eresults ; ++result)
   {
      CPPUNIT_LOG("Checking consumer" << consnum(result) << " ...") ;

      for (const auto &v: *result)
      {
         const ptrdiff_t result_ndx = &v - result->data() ;

         if ((size_t)v < nmax_produced && !indicator[v].first)
         {
            indicator[v] = {result, result_ndx} ;
            ++produced[(size_t)v/items_per_producer] ;
         }
         else
         {
            CPPUNIT_LOG(" ERROR consumer" << consnum(result) << " item #" << result_ndx << "=" << v) ;
            if ((size_t)v >= nmax_produced)
               CPPUNIT_LOG_LINE(": item value is too big") ;
            else
               CPPUNIT_LOG_LINE(": duplicate item, first appeared in consumer"
                                << consnum(indicator[v].first) << " at #" << indicator[v].second) ;
            CPPUNIT_FAIL("Inconsistent concurrent queue results") ;
         }
      }
      CPPUNIT_LOG_LINE(" OK") ;
   }
   CPPUNIT_LOG_LINE("Checked OK") ;

   CPPUNIT_LOG_LINE("Checking consumer results for sequential consistency with producers:") ;

   size_t prodnum = 0 ;
   // Scan per-producer
   for (auto prng = make_simple_slice(pbegin(indicator), pbegin(indicator) + items_per_producer) ;
        prng.begin() != pend(indicator) ;
        prng = make_simple_slice(prng.end(), prng.end() + items_per_producer))
   {
      CPPUNIT_LOG("Checking producer" << ++prodnum << " ...") ;

      // Check if some items were skipped
      auto pskipped = std::adjacent_find(prng.begin(), prng.end(), [](const indicator_pair &x, const indicator_pair &y)
      {
         return !x.first && y.first ;
      }) ;
      if (pskipped != prng.end())
      {
         const size_t item = pskipped - pbegin(indicator) ;
         CPPUNIT_LOG_LINE("ERROR skipped (not consumed) item #" << item << " (item" << item%items_per_producer
                          << " of producer" << prodnum << ")") ;
         CPPUNIT_FAIL("Inconsistent concurrent queue results") ;
      }

      std::sort(prng.begin(), prng.end(), [](const indicator_pair &x, const indicator_pair &y)
      {
         return y.first < x.first || y.first == x.first && y.first && x.second < y.second ;
      }) ;

      auto porder_violated = std::adjacent_find(prng.begin(), prng.end(), [](const indicator_pair &x, const indicator_pair &y)
      {
         return x.first && x.first == y.first && x.first->at(x.second) >= y.first->at(y.second) ;
      }) ;

      if (porder_violated != prng.end())
      {
         const size_t item = porder_violated->first->at(porder_violated->second) ;

         CPPUNIT_LOG_LINE("ERROR out-of-order item #" << item << " (item" << item%items_per_producer
                          << " of producer" << prodnum << ") is before item #"
                          << (porder_violated+1)->first->at((porder_violated+1)->second) << " at consumer"
                          << consnum(porder_violated->first)) ;

         CPPUNIT_FAIL("Inconsistent concurrent queue results") ;
      }
      CPPUNIT_LOG_LINE(" OK") ;
   }
   CPPUNIT_LOG_LINE("Checked OK") ;

   return std::move(produced) ;
}

template<typename T>
void CheckQueueResultConsistency(size_t producers_count, size_t items_per_producer, const std::vector<T> &result)
{
   CheckQueueResultConsistency(producers_count, items_per_producer, producers_count*items_per_producer,
                               &result, &result + 1) ;
}

template<typename T>
void CheckQueueResultConsistency(size_t producers_count, size_t items_per_producer,
                                 const std::vector<T> *bresults,
                                 const std::vector<T> *eresults)
{
   CheckQueueResultConsistency(producers_count, items_per_producer, producers_count*items_per_producer,
                               bresults, eresults) ;
}

/*******************************************************************************
 Various queue tests
*******************************************************************************/
template<typename S, typename P, typename T>
static void FinalizeQueueTestNx1(S stop, P &producers, std::thread &consumer, size_t per_thread,
                                 const std::vector<T> &result)
{
   for (std::thread &p: producers)
      CPPUNIT_LOG_RUN(p.join()) ;

   CPPUNIT_LOG_RUN(stop()) ;
   CPPUNIT_LOG_RUN(consumer.join()) ;

   CheckQueueResultConsistency(std::size(producers), per_thread, result) ;
}

template<typename S, typename V, typename T>
static void FinalizeQueueTestNxN(S stop, V &producers, V &consumers, size_t per_thread,
                                 const std::vector<T> *bresults,
                                 const std::vector<T> *eresults)
{
   for (std::thread &p: producers)
      CPPUNIT_LOG_RUN(p.join()) ;

   CPPUNIT_LOG_RUN(stop()) ;

   for (std::thread &c: consumers)
      CPPUNIT_LOG_RUN(c.join()) ;

   CheckQueueResultConsistency(std::size(producers), per_thread, bresults, eresults) ;
}

template<typename Queue>
void CdsQueueTest_Nx1(Queue &q, size_t producers_count, size_t repeat_count)
{
   CPPUNIT_LOG_ASSERT(producers_count > 0) ;
   CPPUNIT_LOG_ASSERT(repeat_count > 0) ;

   const size_t N = CDSTEST_COUNT_QUOTIENT*repeat_count ;
   const size_t per_thread = N/producers_count ;

   CPPUNIT_LOG_LINE("****************** " << producers_count << " producers, 1 consumer, "
                    << N << " items, " << per_thread << " per producer thread *******************") ;

   CPPUNIT_LOG_ASSERT(N % producers_count == 0) ;

   std::vector<std::thread> producers (producers_count) ;
   std::thread consumer ;

   std::vector<size_t> v ;
   v.reserve(N) ;

   volatile int endprod = 0 ;
   consumer = std::thread
      ([&]
       {
          while(!endprod || !q.empty())
          {
             size_t c = 0 ;
             if (q.try_pop(c))
                v.push_back(c) ;
          }
       }) ;

   size_t start_from = 0 ;
   for (std::thread &p: producers)
   {
      p = std::thread
         ([=,&q]() mutable
          {
             std::random_device rd1 ;
             std::uniform_int_distribution<int> distrib (0, 200) ;
             for (size_t i = start_from, end_with = start_from + per_thread ; i < end_with ; ++i)
             {
                sys::pause_cpu(distrib(rd1)) ;
                q.push(i) ;
             }
          }) ;
      start_from += per_thread ;
   }

   FinalizeQueueTestNx1([&]()mutable { endprod = 1 ; }, producers, consumer, per_thread, v) ;
   CPPUNIT_LOG_EQ(v.size(), N) ;
}

template<typename Queue>
void CdsQueueTest_NxN(Queue &q, size_t producers_count, size_t consumers_count, size_t repeat_count)
{
   CPPUNIT_LOG_ASSERT(producers_count > 0) ;
   CPPUNIT_LOG_ASSERT(consumers_count > 0) ;
   CPPUNIT_LOG_ASSERT(repeat_count > 0) ;

   const size_t N = CDSTEST_COUNT_QUOTIENT*repeat_count ;
   const size_t per_thread = N/producers_count ;

   CPPUNIT_LOG_LINE("****************** " << producers_count << " producers, " << consumers_count << "  consumer(s), "
                    << N << " items, " << per_thread << " per producer thread *******************") ;

   CPPUNIT_LOG_ASSERT(N % producers_count == 0) ;

   std::vector<std::thread> producers (producers_count) ;
   std::vector<std::thread> consumers (consumers_count) ;
   std::vector<std::vector<size_t>> v (consumers_count) ;

   volatile int endprod = 0 ;
   size_t num = 0 ;
   for (std::thread &cs: consumers)
   {
      cs = std::thread
         ([&,num]
          {
             auto &r = v[num] ;
             while(!endprod || !q.empty())
             {
                size_t c = 0 ;
                if (q.try_pop(c))
                   r.push_back(c) ;
             }
          }) ;
      ++num ;
   }

   size_t start_from = 0 ;
   for (std::thread &p: producers)
   {
      p = std::thread
         ([=,&q]() mutable
          {
             std::random_device rd1 ;
             std::uniform_int_distribution<int> distrib (0, 200) ;
             for (size_t i = start_from, end_with = start_from + per_thread ; i < end_with ; ++i)
             {
                sys::pause_cpu(distrib(rd1)) ;
                q.push(i) ;
             }
          }) ;
      start_from += per_thread ;
   }

   FinalizeQueueTestNxN([&]()mutable { endprod = 1 ; }, producers, consumers, per_thread, pbegin(v), pend(v)) ;
   CPPUNIT_LOG_ASSERT(q.empty()) ;
}

template<typename Queue>
void DualQueueTest_Nx1(Queue &q, size_t producers_count, size_t repeat_count)
{
   CPPUNIT_LOG_ASSERT(producers_count > 0) ;
   CPPUNIT_LOG_ASSERT(repeat_count > 0) ;

   const size_t N = CDSTEST_COUNT_QUOTIENT*repeat_count ;
   const size_t per_thread = N/producers_count ;

   CPPUNIT_LOG_LINE("****************** " << producers_count << " producers, 1 consumer, "
                    << N << " items, " << per_thread << " per producer thread *******************") ;

   CPPUNIT_LOG_ASSERT(N % producers_count == 0) ;

   std::vector<std::thread> producers (producers_count) ;
   std::thread consumer ;

   std::vector<size_t> v ;
   v.reserve(N) ;

   consumer = std::thread
      ([&]
       {
          while(v.size() < N)
             v.push_back(q.pop()) ;
       }) ;

   size_t start_from = 0 ;
   for (std::thread &p: producers)
   {
      p = std::thread
         ([=,&q]() mutable
          {
             for (size_t i = start_from, end_with = start_from + per_thread ; i < end_with ; ++i)
                q.push(i) ;
          }) ;
      start_from += per_thread ;
   }

   FinalizeQueueTestNx1([]{}, producers, consumer, per_thread, v) ;
   CPPUNIT_LOG_ASSERT(q.empty()) ;
}

template<typename Queue>
void DualQueueTest_NxN(Queue &q, size_t producers_count, size_t consumers_count, size_t repeat_count)
{
   using namespace std ;

   CPPUNIT_LOG_ASSERT(producers_count > 0) ;
   CPPUNIT_LOG_ASSERT(consumers_count > 0) ;
   CPPUNIT_LOG_ASSERT(repeat_count > 0) ;

   const size_t N = CDSTEST_COUNT_QUOTIENT*repeat_count ;
   const size_t per_thread = N/producers_count ;

   CPPUNIT_LOG_LINE("****************** " << producers_count << " producers, " << consumers_count << "  consumer(s), "
                    << N << " items, " << per_thread << " per producer thread *******************") ;

   CPPUNIT_LOG_ASSERT(N % producers_count == 0) ;

   vector<thread> producers (producers_count) ;
   vector<thread> consumers (consumers_count) ;
   vector<vector<size_t>> v (consumers_count) ;

   size_t num = 0 ;
   for (thread &cs: consumers)
   {
      cs = thread
         ([&,num]
          {
             for (size_t c ; (c = q.pop()) != (size_t)-1 ; v[num].push_back(c)) ;
          }) ;
      ++num ;
   }

   size_t start_from = 0 ;
   for (thread &p: producers)
   {
      p = thread
         ([=,&q]() mutable
          {
             for (size_t i = start_from, end_with = start_from + per_thread ; i < end_with ; ++i)
                q.push(i) ;
          }) ;
      start_from += per_thread ;
   }

   FinalizeQueueTestNxN([&]()mutable{ fill_n(back_inserter(q), consumers.size(), -1) ; },
                        producers, consumers, per_thread, pbegin(v), pend(v)) ;
   CPPUNIT_LOG_ASSERT(q.empty()) ;
}

/*******************************************************************************
 Tantrum queues tests.
 A tantrum queue is a queue in which an enqueue can nondeterministically refuse
 to enqueue its item, returning CLOSED instead and moving the queue to a CLOSED state.
*******************************************************************************/
template<typename Queue>
void TantrumQueueTest(Queue &q, size_t producers_count, size_t consumers_count, size_t per_producer_count,
                      unipair<unsigned> enqueue_pause = {}, unipair<unsigned> dequeue_pause = {},
                      unsigned flags = 0)
{
   typedef cache_aligned<size_t> cache_aligned_counter ;

   CPPUNIT_ASSERT(producers_count > 0) ;
   CPPUNIT_ASSERT(consumers_count > 0) ;
   CPPUNIT_ASSERT(per_producer_count > 0) ;
   CPPUNIT_ASSERT(enqueue_pause.first <= enqueue_pause.second) ;
   CPPUNIT_ASSERT(dequeue_pause.first <= dequeue_pause.second) ;

   const size_t total = per_producer_count*producers_count ;

   CPPUNIT_LOG_LINE("****************** " << producers_count << " producers, " << consumers_count << "  consumer(s), "
                    << total << " items, " << per_producer_count << " per producer thread *******************") ;

   if (enqueue_pause.first != enqueue_pause.second || dequeue_pause.first != dequeue_pause.second)
      CPPUNIT_LOG_LINE("****************** enqueue pause: " << enqueue_pause.first << ".." << enqueue_pause.second << " clocks, "
                       << "dequeue pause: " << dequeue_pause.first << ".." << dequeue_pause.second << " clocks") ;


   std::vector<std::thread> producers (producers_count) ;
   std::vector<cache_aligned_counter> produced_counts (producers_count) ;
   std::vector<cache_aligned_counter> consumed_counts (consumers_count) ;

   std::vector<std::thread> consumers (consumers_count) ;
   std::vector<cache_aligned<std::vector<size_t>>> v (consumers_count) ;
   const bool check_consistency = !(flags & CDSTST_NOCHECK) ;

   PCpuStopwatch  cpu_stopwatch ;
   PRealStopwatch wallclock_stopwatch ;

   cpu_stopwatch.start() ;
   wallclock_stopwatch.start() ;

   std::atomic<int> endprod = {} ;
   size_t num = 0 ;
   for (std::thread &cs: consumers)
   {
      cs = std::thread
         ([&,num]
          {
             pause_distribution distrib (dequeue_pause.first, dequeue_pause.second) ;
             auto &r = v[num] ;
             size_t &count = consumed_counts[num] ;

             while(!endprod.load(std::memory_order_relaxed) || !q.empty())
             {
                sys::pause_cpu(distrib()) ;

                auto dv = q.dequeue() ;
                if (dv.second)
                {
                   ++count ;
                   if (check_consistency)
                      r.push_back(dv.first) ;
                }
             }
          }) ;
      ++num ;
   }

   size_t start_from = 0 ;
   num = 0 ;
   for (std::thread &p: producers)
   {
      p = std::thread
         ([=,&q,&produced_counts]() mutable
          {
             pause_distribution distrib (enqueue_pause.first, enqueue_pause.second) ;
             size_t &count = produced_counts[num] ;

             for (size_t i = start_from, end_with = start_from + per_producer_count ; i < end_with ; ++i)
             {
                sys::pause_cpu(distrib()) ;

                if (q.enqueue(i + 0))
                   ++count ;
                else
                   break ;
             }
          }) ;
      start_from += per_producer_count ;
      ++num ;
   }

   for (std::thread &p: producers)
      CPPUNIT_LOG_RUN(p.join()) ;

   endprod = 1 ;

   for (std::thread &c: consumers)
      CPPUNIT_LOG_RUN(c.join()) ;

   if (check_consistency)
   {
      std::vector<std::vector<size_t>> consumed_data
         (std::make_move_iterator(v.begin()), std::make_move_iterator(v.end())) ;

      CheckQueueResultConsistency(producers_count, per_producer_count,
                                  std::accumulate(produced_counts.begin(), produced_counts.end(), 0),
                                  pbegin(consumed_data), pend(consumed_data)) ;

      CPPUNIT_LOG_ASSERT(q.empty()) ;
   }
   else
   {
      const double elapsed_time = wallclock_stopwatch.stop() ;
      const double elapsed_cputime = cpu_stopwatch.stop() ;

      const size_t produced = std::accumulate(std::begin(produced_counts), std::end(produced_counts), 0) ;
      const size_t consumed = std::accumulate(std::begin(consumed_counts), std::end(consumed_counts), 0) ;

      CPPUNIT_LOG_LINE('\n' << producers_count << " producer(s), " << consumers_count << " consumer(s), " << produced
                        << " enqueued items, " << consumed << " dequeued items\n") ;
      CPPUNIT_LOG_LINE("time real: " << elapsed_time << '\n' <<
                       "time cpu : " << elapsed_cputime) ;
   }
}

} // end of namespace pcomn::unit
} // end of namespace pcomn

#endif /* __PCOMN_TESTCDS_H */
