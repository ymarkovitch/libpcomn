/*-*- tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_cdsqueue.cpp
 COPYRIGHT    :   Yakov Markovitch, 2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for conqurrent queues.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   22 Jun 2016
*******************************************************************************/
#include <pcomn_cdsqueue.h>
#include <pcomn_iterator.h>
#include <pcomn_unittest.h>

#include <thread>

using namespace pcomn ;

class ConcurrentDynQueueTests : public CppUnit::TestFixture {

      void Test_CdsQueue_SingleThread() ;
      void Test_DualQueue_SingleThread() ;
      void Test_CdsQueues_Of_Movable() ;

      void Test_QueueChecker_GoodResults() ;
      void Test_QueueChecker_BadResult1() ;
      void Test_QueueChecker_BadResult2() ;
      void Test_QueueChecker_BadResult3() ;
      void Test_QueueChecker_BadResult4() ;
      void Test_QueueChecker_BadResult5() ;

      template<size_t producers, size_t count = 1000>
      void Test_CdsQueue_Nx1() ;
      template<size_t producers, size_t count = 1000>
      void Test_DualQueue_Nx1() ;

      CPPUNIT_TEST_SUITE(ConcurrentDynQueueTests) ;

      CPPUNIT_TEST(Test_CdsQueue_SingleThread) ;
      CPPUNIT_TEST(Test_DualQueue_SingleThread) ;

      CPPUNIT_TEST(Test_QueueChecker_GoodResults) ;
      CPPUNIT_TEST_FAIL(Test_QueueChecker_BadResult1) ;
      CPPUNIT_TEST_FAIL(Test_QueueChecker_BadResult2) ;
      CPPUNIT_TEST_FAIL(Test_QueueChecker_BadResult3) ;
      CPPUNIT_TEST_FAIL(Test_QueueChecker_BadResult4) ;
      CPPUNIT_TEST_FAIL(Test_QueueChecker_BadResult5) ;

      CPPUNIT_TEST(P_PASS(Test_CdsQueue_Nx1<1, 1>)) ;
      CPPUNIT_TEST(P_PASS(Test_CdsQueue_Nx1<1, 1000>)) ;
      CPPUNIT_TEST(P_PASS(Test_CdsQueue_Nx1<2, 1>)) ;
      CPPUNIT_TEST(P_PASS(Test_CdsQueue_Nx1<2, 1000>)) ;
      CPPUNIT_TEST(P_PASS(Test_CdsQueue_Nx1<3, 1>)) ;
      CPPUNIT_TEST(P_PASS(Test_CdsQueue_Nx1<3, 1000>)) ;
      CPPUNIT_TEST(P_PASS(Test_CdsQueue_Nx1<16, 1>)) ;
      CPPUNIT_TEST(P_PASS(Test_CdsQueue_Nx1<16, 1000>)) ;

      CPPUNIT_TEST(P_PASS(Test_DualQueue_Nx1<1, 1>)) ;
      CPPUNIT_TEST(P_PASS(Test_DualQueue_Nx1<1, 1000>)) ;
      CPPUNIT_TEST(P_PASS(Test_DualQueue_Nx1<2, 1>)) ;
      CPPUNIT_TEST(P_PASS(Test_DualQueue_Nx1<2, 1000>)) ;
      CPPUNIT_TEST(P_PASS(Test_DualQueue_Nx1<3, 1>)) ;
      CPPUNIT_TEST(P_PASS(Test_DualQueue_Nx1<3, 1000>)) ;
      CPPUNIT_TEST(P_PASS(Test_DualQueue_Nx1<16, 1>)) ;
      CPPUNIT_TEST(P_PASS(Test_DualQueue_Nx1<16, 1000>)) ;

      CPPUNIT_TEST_SUITE_END() ;

   public:
      template<typename T>
      static void CheckQueueResultConsistency(size_t items_per_thread, size_t producers_count, const std::vector<T> &result) ;
} ;

template<typename T>
void ConcurrentDynQueueTests::CheckQueueResultConsistency(size_t producers_count, size_t items_per_thread,
                                                          const std::vector<T> &result)
{
   const size_t n = items_per_thread*producers_count ;
   CPPUNIT_LOG_LINE("CHECK RESULTS CONSISTENCY: " << producers_count << " producers, 1 consumer, "
                    << n << " items, " << items_per_thread << " per producer") ;

   CPPUNIT_ASSERT(items_per_thread) ;
   CPPUNIT_ASSERT(producers_count) ;

   CPPUNIT_LOG_EQ(result.size(), n) ;
   std::vector<ptrdiff_t> indicator (n, -1) ;

   CPPUNIT_LOG("Checking every source item is present in the result exactly once...") ;
   for (const auto &v: result)
      if ((size_t)v < n && indicator[v] == -1)
         indicator[v] = &v - result.data() ;
      else
      {
         CPPUNIT_LOG(" ERROR at item #" << &v - result.data() << "=" << v) ;
         if ((size_t)v >= n)
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

      CPPUNIT_LOG("Checking result sequential consistency with producer #" << p+1
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

/*******************************************************************************
 ConcurrentDynQueueTests
*******************************************************************************/
void ConcurrentDynQueueTests::Test_CdsQueue_SingleThread()
{
   typedef concurrent_dynqueue<std::string> strcdsq ;

   strcdsq empty0 ;
   std::string val0 = "Hello, world!" ;

   CPPUNIT_LOG_ASSERT(empty0.empty()) ;
   CPPUNIT_LOG_IS_FALSE(empty0.pop(val0)) ;
   CPPUNIT_LOG_ASSERT(empty0.empty()) ;
   CPPUNIT_LOG_EQ(val0, "Hello, world!") ;
   CPPUNIT_LOG_EQ(empty0.pop_default("Foo"), std::make_pair("Foo", false)) ;
   CPPUNIT_LOG_ASSERT(empty0.empty()) ;

   CPPUNIT_LOG(std::endl) ;

   strcdsq q1 ;
   std::string val1 = "Bye, baby!" ;
   std::string val ;

   CPPUNIT_LOG_ASSERT(q1.empty()) ;
   CPPUNIT_LOG_IS_FALSE(q1.pop(val1)) ;
   CPPUNIT_LOG_EQ(val1, "Bye, baby!") ;
   CPPUNIT_LOG_RUN(q1.push(val0)) ;
   CPPUNIT_LOG_IS_FALSE(q1.empty()) ;

   CPPUNIT_LOG_ASSERT(q1.pop(val)) ;
   CPPUNIT_LOG_EQ(val, "Hello, world!") ;
   CPPUNIT_LOG_ASSERT(q1.empty()) ;
   CPPUNIT_LOG_EQ(q1.pop_default("Foo"), std::make_pair("Foo", false)) ;
   CPPUNIT_LOG_ASSERT(q1.empty()) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(q1.push("Foo")) ;
   CPPUNIT_LOG_IS_FALSE(q1.empty()) ;
   CPPUNIT_LOG_RUN(q1.push_back("Quux")) ;
   CPPUNIT_LOG_IS_FALSE(q1.empty()) ;
   CPPUNIT_LOG_RUN(q1.push(std::move(val0))) ;
   CPPUNIT_LOG_IS_FALSE(q1.empty()) ;
   CPPUNIT_LOG_EQ(val0, "") ;
   CPPUNIT_LOG_RUN(q1.emplace(16, '@')) ;
   CPPUNIT_LOG_IS_FALSE(q1.empty()) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(q1.pop(val)) ;
   CPPUNIT_LOG_EQ(val, "Foo") ;
   CPPUNIT_LOG_IS_FALSE(q1.empty()) ;
   CPPUNIT_LOG_EQ(q1.pop_default(), std::make_pair("Quux", true)) ;
   CPPUNIT_LOG_IS_FALSE(q1.empty()) ;
   CPPUNIT_LOG_EQ(q1.pop_default(), std::make_pair("Hello, world!", true)) ;
   CPPUNIT_LOG_IS_FALSE(q1.empty()) ;
   CPPUNIT_LOG_EQ(q1.pop_default(), std::make_pair("@@@@@@@@@@@@@@@@", true)) ;
   CPPUNIT_LOG_ASSERT(q1.empty()) ;
   CPPUNIT_LOG_EQ(q1.pop_default(), std::make_pair("", false)) ;

   CPPUNIT_LOG(std::endl) ;
   {
      strcdsq q2 ;
      CPPUNIT_LOG_RUN(q2.push("Quux")) ;
      CPPUNIT_LOG_RUN(q2.push_back("Bar")) ;
      CPPUNIT_LOG_RUN(q2.emplace(16, '+')) ;
   }

   CPPUNIT_LOG(std::endl) ;
   strcdsq q3 ;
   CPPUNIT_LOG_RUN(q3.push("Quux")) ;
   CPPUNIT_LOG_RUN(q3.push_back("Bar")) ;
   CPPUNIT_LOG_RUN(q3.emplace(16, '+')) ;
}

void ConcurrentDynQueueTests::Test_DualQueue_SingleThread()
{
   typedef concurrent_dualqueue<std::string> strcdsq ;

   strcdsq empty0 ;
   std::string val0 = "Hello, world!" ;

   CPPUNIT_LOG_ASSERT(empty0.empty()) ;
   CPPUNIT_LOG_IS_FALSE(empty0.try_pop(val0)) ;
   CPPUNIT_LOG_ASSERT(empty0.empty()) ;
   CPPUNIT_LOG_EQ(val0, "Hello, world!") ;
   CPPUNIT_LOG_EQ(empty0.pop_default("Foo"), std::make_pair("Foo", false)) ;
   CPPUNIT_LOG_ASSERT(empty0.empty()) ;

   CPPUNIT_LOG(std::endl) ;

   strcdsq q1 ;
   std::string val1 = "Bye, baby!" ;
   std::string val ;

   CPPUNIT_LOG_ASSERT(q1.empty()) ;
   CPPUNIT_LOG_IS_FALSE(q1.try_pop(val1)) ;
   CPPUNIT_LOG_EQ(val1, "Bye, baby!") ;
   CPPUNIT_LOG_RUN(q1.push(val0)) ;
   CPPUNIT_LOG_IS_FALSE(q1.empty()) ;

   CPPUNIT_LOG_ASSERT(q1.try_pop(val)) ;
   CPPUNIT_LOG_EQ(val, "Hello, world!") ;
   CPPUNIT_LOG_ASSERT(q1.empty()) ;
   CPPUNIT_LOG_EQ(q1.pop_default("Foo"), std::make_pair("Foo", false)) ;
   CPPUNIT_LOG_ASSERT(q1.empty()) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(q1.push("Foo")) ;
   CPPUNIT_LOG_IS_FALSE(q1.empty()) ;
   CPPUNIT_LOG_RUN(q1.push_back("Quux")) ;
   CPPUNIT_LOG_IS_FALSE(q1.empty()) ;
   CPPUNIT_LOG_RUN(q1.push(std::move(val0))) ;
   CPPUNIT_LOG_IS_FALSE(q1.empty()) ;
   CPPUNIT_LOG_EQ(val0, "") ;
   CPPUNIT_LOG_RUN(q1.emplace(16, '@')) ;
   CPPUNIT_LOG_IS_FALSE(q1.empty()) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(q1.try_pop(val)) ;
   CPPUNIT_LOG_EQ(val, "Foo") ;
   CPPUNIT_LOG_IS_FALSE(q1.empty()) ;
   CPPUNIT_LOG_EQ(q1.pop_default(), std::make_pair("Quux", true)) ;
   CPPUNIT_LOG_IS_FALSE(q1.empty()) ;
   CPPUNIT_LOG_EQ(q1.pop_default(), std::make_pair("Hello, world!", true)) ;
   CPPUNIT_LOG_IS_FALSE(q1.empty()) ;
   CPPUNIT_LOG_EQ(q1.pop_default(), std::make_pair("@@@@@@@@@@@@@@@@", true)) ;
   CPPUNIT_LOG_ASSERT(q1.empty()) ;
   CPPUNIT_LOG_EQ(q1.pop_default(), std::make_pair("", false)) ;

   CPPUNIT_LOG(std::endl) ;
   {
      strcdsq q2 ;
      CPPUNIT_LOG_RUN(q2.push("Quux")) ;
      CPPUNIT_LOG_RUN(q2.push_back("Bar")) ;
      CPPUNIT_LOG_RUN(q2.emplace(16, '+')) ;
   }

   CPPUNIT_LOG(std::endl) ;
   strcdsq q3 ;
   CPPUNIT_LOG_RUN(q3.push("Quux")) ;
   CPPUNIT_LOG_RUN(q3.push_back("Bar")) ;
   CPPUNIT_LOG_RUN(q3.emplace(16, '+')) ;
}

void ConcurrentDynQueueTests::Test_CdsQueues_Of_Movable()
{
   typedef std::unique_ptr<std::string> struptr ;
   typedef concurrent_dynqueue<struptr> str_cdsq ;
   typedef concurrent_dualqueue<struptr> str_dualq ;

   str_cdsq cdsq ;
   str_dualq dualq ;

   std::string *sp1 = new std::string("Hello, world!") ;
   struptr sup1 {sp1} ;

   CPPUNIT_LOG_IS_FALSE(cdsq.pop(sup1)) ;
   CPPUNIT_LOG_EQ(sup1.get(), sp1) ;
   CPPUNIT_LOG_IS_FALSE(dualq.try_pop(sup1)) ;
   CPPUNIT_LOG_EQ(sup1.get(), sp1) ;
}

/*******************************************************************************
 ConcurrentDynQueueTests::Test_QueueChecker
*******************************************************************************/
void ConcurrentDynQueueTests::Test_QueueChecker_GoodResults()
{
   std::vector<size_t> v15 (count_iter(0), count_iter(15)) ;
   CheckQueueResultConsistency(1, 15, v15) ;
   std::vector<size_t> v8 {0, 1, 4, 5, 2, 6, 3, 7} ;
   CheckQueueResultConsistency(2, 4, v8) ;
}

void ConcurrentDynQueueTests::Test_QueueChecker_BadResult1()
{
   std::vector<size_t> v15 (count_iter(0), count_iter(15)) ;
   CheckQueueResultConsistency(1, 14, v15) ;
}

void ConcurrentDynQueueTests::Test_QueueChecker_BadResult2()
{
   std::vector<size_t> v15 (count_iter(0), count_iter(15)) ;
   CPPUNIT_LOG_RUN(v15.back() = 15) ;
   CheckQueueResultConsistency(1, 15, v15) ;
}

void ConcurrentDynQueueTests::Test_QueueChecker_BadResult3()
{
   std::vector<size_t> v16 (count_iter(0), count_iter(16)) ;
   CPPUNIT_LOG_RUN(v16[7] = 10) ;
   CheckQueueResultConsistency(2, 8, v16) ;
}

void ConcurrentDynQueueTests::Test_QueueChecker_BadResult4()
{
   std::vector<size_t> v15 (count_iter(0), count_iter(15)) ;
   CPPUNIT_LOG_RUN(std::swap(v15[5], v15[10])) ;
   CheckQueueResultConsistency(1, 15, v15) ;
}

void ConcurrentDynQueueTests::Test_QueueChecker_BadResult5()
{
   std::vector<size_t> v8 {0, 1, 4, 5, 2, 7, 3, 6} ;
   CheckQueueResultConsistency(2, 4, v8) ;
}

/*******************************************************************************
 ConcurrentDynQueueTests::Test_Queue
*******************************************************************************/
template<size_t P, size_t C>
void ConcurrentDynQueueTests::Test_CdsQueue_Nx1()
{
   const size_t N = 3*5*7*16*C ;
   PCOMN_STATIC_CHECK(N % P == 0) ;
   const size_t per_thread = N/P ;

   CPPUNIT_LOG_LINE("****************** " << P << " producers, 1 consumer, "
                    << N << " items, " << per_thread << " per producer thread *******************") ;

   std::thread producers[P] ;
   std::thread consumer ;

   concurrent_dynqueue<size_t> q ;
   std::vector<size_t> v ;
   v.reserve(N) ;

   consumer = std::thread
      ([&]
       {
          while(v.size() < N)
          {
             size_t c = 0 ;
             if (q.pop(c))
                v.push_back(c) ;
          }
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

   for (std::thread &p: producers)
      CPPUNIT_LOG_RUN(p.join()) ;

   CPPUNIT_LOG_RUN(consumer.join()) ;

   CPPUNIT_LOG_EQ(v.size(), N) ;
   CPPUNIT_LOG_ASSERT(q.empty()) ;

   CheckQueueResultConsistency(P, per_thread, v) ;
}

template<size_t P, size_t C>
void ConcurrentDynQueueTests::Test_DualQueue_Nx1()
{
   const size_t N = 3*5*7*16*C ;
   PCOMN_STATIC_CHECK(N % P == 0) ;
   const size_t per_thread = N/P ;

   CPPUNIT_LOG_LINE("****************** " << P << " producers, 1 consumer, "
                    << N << " items, " << per_thread << " per producer thread *******************") ;

   std::thread producers[P] ;
   std::thread consumer ;

   concurrent_dualqueue<size_t> q ;
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

   for (std::thread &p: producers)
      CPPUNIT_LOG_RUN(p.join()) ;

   CPPUNIT_LOG_RUN(consumer.join()) ;

   CPPUNIT_LOG_EQ(v.size(), N) ;
   CPPUNIT_LOG_ASSERT(q.empty()) ;

   CheckQueueResultConsistency(P, per_thread, v) ;
}

/*******************************************************************************

*******************************************************************************/
int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(ConcurrentDynQueueTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv, nullptr, "Tests of concurrent queues") ;
}
