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

#include <thread>
#include <vector>
#include <numeric>
#include <random>
#include <algorithm>

namespace pcomn {
namespace unit {

const size_t CDSTEST_COUNT_QUOTIENT = 3*5*7*9*11*16 ;

/*******************************************************************************
 Check consistency of test results
*******************************************************************************/
template<typename T>
void CheckQueueResultConsistency(size_t producers_count, size_t items_per_thread, const std::vector<T> &result)
{
   const size_t nproduced = items_per_thread*producers_count ;
   CPPUNIT_LOG_LINE("\nCHECK RESULTS CONSISTENCY: " << producers_count << " producers, 1 consumer, "
                    << nproduced << " items, " << items_per_thread << " per producer") ;

   CPPUNIT_ASSERT(items_per_thread) ;
   CPPUNIT_ASSERT(producers_count) ;

   CPPUNIT_LOG_EQ(result.size(), nproduced) ;
   std::vector<ptrdiff_t> indicator (nproduced, -1) ;

   CPPUNIT_LOG("Checking every source item is present in the result exactly once...") ;
   for (const auto &v: result)
      if ((size_t)v < nproduced && indicator[v] == -1)
         indicator[v] = &v - result.data() ;
      else
      {
         CPPUNIT_LOG(" ERROR at item #" << &v - result.data() << "=" << v) ;
         if ((size_t)v >= nproduced)
            CPPUNIT_LOG_LINE(": item value is too big") ;
         else
            CPPUNIT_LOG_LINE(": duplicate item, first appears at #" << indicator[v]) ;
         CPPUNIT_FAIL("Inconsistent concurrent queue results") ;
      }

   CPPUNIT_LOG_LINE(" OK") ;

   for (size_t p = 0 ; p < producers_count ; ++p)
   {
      size_t start = p*items_per_thread ;
      const size_t finish = start + items_per_thread ;

      CPPUNIT_LOG("Checking result sequential consistency with producer" << p+1
                  << " items " << start << ".." << finish-1 << " ...") ;

      const auto bad = std::find_if(result.begin(), result.end(), [&](T v) mutable
      {
         return xinrange((size_t)v, start, finish) && (size_t)v != start++ ;
      }) ;
      if (bad == result.end())
         CPPUNIT_LOG_LINE(" OK") ;
      else
      {
         CPPUNIT_LOG_LINE(" ERROR at item #" << bad - result.begin()) ;
         CPPUNIT_LOG_EQ(*bad, start - 1) ;
      }
   }
}

template<typename T>
void CheckQueueResultConsistency(size_t producers_count, size_t items_per_thread,
                                 const std::vector<T> *bresults,
                                 const std::vector<T> *eresults)
{
   const size_t nproduced = items_per_thread*producers_count ;
   const size_t consumers = eresults - bresults ;

   CPPUNIT_LOG_LINE(("\nCHECK RESULTS CONSISTENCY: ")
                    << producers_count << " producers, " << consumers << " consumer(s), " << nproduced
                    << " items, " << items_per_thread << " per producer") ;

   CPPUNIT_ASSERT(consumers != 0) ;
   CPPUNIT_ASSERT(items_per_thread) ;
   CPPUNIT_ASSERT(producers_count) ;

   auto consnum = [=](const std::vector<T> *result) { return result - bresults + 1 ; } ;

   const size_t nconsumed =
      std::accumulate(bresults, eresults, 0UL, [](size_t a, const std::vector<T> &v)
      {
         CPPUNIT_LOG_LINE("Consumed " << v.size() << " items") ;
         return a + v.size() ;
      }) ;

   CPPUNIT_LOG_EQ(nconsumed, nproduced) ;

   std::vector<std::pair<const std::vector<T> *, ptrdiff_t>> indicator (nproduced, {nullptr, -1}) ;

   CPPUNIT_LOG_LINE("Checking every source item is present in the result exactly once") ;

   for (auto result = bresults ; result != eresults ; ++result)
   {
      CPPUNIT_LOG("Checking consumer" << consnum(result) << " ...") ;

      for (const auto &v: *result)
      {
         const ptrdiff_t nitem = &v - result->data() ;

         if ((size_t)v < nproduced && !indicator[v].first)
            indicator[v] = {result, nitem} ;
         else
         {
            CPPUNIT_LOG(" ERROR consumer" << consnum(result) << " item #" << nitem << "=" << v) ;
            if ((size_t)v >= nproduced)
               CPPUNIT_LOG_LINE(": item value is too big") ;
            else
               CPPUNIT_LOG_LINE(": duplicate item, first appeared in consumer "
                                << consnum(indicator[v].first) << " at #" << indicator[v].second) ;
            CPPUNIT_FAIL("Inconsistent concurrent queue results") ;
         }
      }
      CPPUNIT_LOG_LINE(" OK") ;
   }

   CPPUNIT_LOG_LINE("Checking consumer results for sequential consistency with producers") ;

   for (auto result = bresults ; result != eresults ; ++result)
      for (size_t p = 0 ; p < producers_count ; ++p)
      {
         const size_t start = p*items_per_thread ;
         const size_t finish = start + items_per_thread ;
         size_t last = start ;

         CPPUNIT_LOG("Checking consumer" << consnum(result) << " sequential consistency with producer" << p+1
                     << " items " << start << ".." << finish-1 << " ...") ;

         const auto bad = std::find_if(result->begin(), result->end(), [&](T v) mutable
         {
            if (xinrange((size_t)v, start, finish))
            {
               if ((size_t)v < last)
                  return true ;
               last = v ;
            }
            return false ;
         }) ;
         if (bad == result->end())
            CPPUNIT_LOG_LINE(" OK") ;
         else
         {
            CPPUNIT_LOG_LINE(" ERROR at item #" << bad - result->begin() << ": " << last << " precedes " << *bad) ;
            CPPUNIT_FAIL("Order of consumed results is not consistent with producer") ;
         }
      }
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

   //CheckQueueResultConsistency(std::size(producers), per_thread, result) ;
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

   //CheckQueueResultConsistency(std::size(producers), per_thread, bresults, eresults) ;
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
                for (int pause = distrib(rd1) ; pause-- ; sys::pause_cpu()) ;
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
                for (int pause = distrib(rd1) ; pause-- ; sys::pause_cpu()) ;
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

} // end of namespace pcomn::unit
} // end of namespace pcomn

#endif /* __PCOMN_TESTCDS_H */
