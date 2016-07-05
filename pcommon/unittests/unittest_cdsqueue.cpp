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
#include <pcomn_testcds.h>

#include <thread>
#include <numeric>

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
      void Test_QueueChecker_BadResult6() ;
      void Test_QueueChecker_BadResult7() ;
      void Test_QueueChecker_BadResult8() ;

      template<size_t producers, size_t count>
      void Test_CdsQueue_Nx1() ;
      template<size_t producers, size_t count>
      void Test_DualQueue_Nx1() ;

      template<size_t producers, size_t consumers, size_t count>
      void Test_CdsQueue_NxN() ;
      template<size_t producers, size_t consumers, size_t count>
      void Test_DualQueue_NxN() ;

      CPPUNIT_TEST_SUITE(ConcurrentDynQueueTests) ;

      CPPUNIT_TEST(Test_CdsQueue_SingleThread) ;
      CPPUNIT_TEST(Test_DualQueue_SingleThread) ;

      CPPUNIT_TEST(Test_QueueChecker_GoodResults) ;
      CPPUNIT_TEST_FAIL(Test_QueueChecker_BadResult1) ;
      CPPUNIT_TEST_FAIL(Test_QueueChecker_BadResult2) ;
      CPPUNIT_TEST_FAIL(Test_QueueChecker_BadResult3) ;
      CPPUNIT_TEST_FAIL(Test_QueueChecker_BadResult4) ;
      CPPUNIT_TEST_FAIL(Test_QueueChecker_BadResult5) ;
      CPPUNIT_TEST_FAIL(Test_QueueChecker_BadResult6) ;
      CPPUNIT_TEST_FAIL(Test_QueueChecker_BadResult7) ;
      CPPUNIT_TEST_FAIL(Test_QueueChecker_BadResult8) ;

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

      CPPUNIT_TEST(P_PASS(Test_CdsQueue_NxN<1, 1, 1>)) ;
      CPPUNIT_TEST(P_PASS(Test_CdsQueue_NxN<1, 2, 1>)) ;
      CPPUNIT_TEST(P_PASS(Test_CdsQueue_NxN<2, 4, 1000>)) ;
      CPPUNIT_TEST(P_PASS(Test_CdsQueue_NxN<2, 2, 1000>)) ;
      CPPUNIT_TEST(P_PASS(Test_CdsQueue_NxN<3, 1, 1000>)) ;
      CPPUNIT_TEST(P_PASS(Test_CdsQueue_NxN<7, 3, 1000>)) ;

      CPPUNIT_TEST(P_PASS(Test_DualQueue_NxN<1, 1, 1>)) ;
      CPPUNIT_TEST(P_PASS(Test_DualQueue_NxN<7, 3, 1000>)) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;


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
   CheckQueueResultConsistency(1, 15, &v15, &v15 + 1) ;

   std::vector<size_t> v8 {0, 1, 4, 5, 2, 6, 3, 7} ;
   CheckQueueResultConsistency(2, 4, v8) ;
   CheckQueueResultConsistency(2, 4, &v8, &v8 + 1) ;

   std::vector<size_t> v4[2] = {
      {0, 1, 4, 5},
      {2, 6, 3, 7}
   } ;
   CheckQueueResultConsistency(2, 4, std::begin(v4), std::end(v4)) ;

   std::vector<size_t> v34[2] = {
      {0, 1, 4, 5, 2, 6},
      {7, 3}
   } ;
   CheckQueueResultConsistency(2, 4, std::begin(v34), std::end(v34)) ;
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

void ConcurrentDynQueueTests::Test_QueueChecker_BadResult6()
{
   std::vector<size_t> v4[2] = {
      {0, 1, 4, 6},
      {2, 6, 3, 7}
   } ;
   CheckQueueResultConsistency(2, 4, std::begin(v4), std::end(v4)) ;
}

void ConcurrentDynQueueTests::Test_QueueChecker_BadResult7()
{
   std::vector<size_t> v4[2] = {
      {0, 1, 4, 5},
      {2, 3, 6, 8}
   } ;
   CheckQueueResultConsistency(2, 4, std::begin(v4), std::end(v4)) ;
}

void ConcurrentDynQueueTests::Test_QueueChecker_BadResult8()
{
   std::vector<size_t> v4[2] = {
      {0, 1, 4, 6, 2, 5},
      {3, 7}
   } ;
   CheckQueueResultConsistency(2, 4, std::begin(v4), std::end(v4)) ;
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

   volatile int endprod = 0 ;
   consumer = std::thread
      ([&]
       {
          while(!endprod || !q.empty())
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

   CPPUNIT_LOG_RUN(endprod = 1) ;
   CPPUNIT_LOG_RUN(consumer.join()) ;

   CPPUNIT_LOG_EQ(v.size(), N) ;
   CPPUNIT_LOG_ASSERT(q.empty()) ;

   CheckQueueResultConsistency(P, per_thread, v) ;
}

template<size_t P, size_t K>
void ConcurrentDynQueueTests::Test_DualQueue_Nx1()
{
   const size_t N = 3*5*7*16*K ;
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

template<size_t P, size_t C, size_t K>
void ConcurrentDynQueueTests::Test_CdsQueue_NxN()
{
   const size_t N = 3*5*7*16*K ;
   PCOMN_STATIC_CHECK(N % P == 0) ;
   const size_t per_thread = N/P ;

   CPPUNIT_LOG_LINE("****************** " << P << " producers, " << C << "  consumer(s), "
                    << N << " items, " << per_thread << " per producer thread *******************") ;

   std::thread producers[P] ;
   std::thread consumers[C] ;
   std::vector<size_t> v[C] ;

   concurrent_dynqueue<size_t> q ;

   volatile int endprod = 0 ;
   size_t num = 0 ;
   for (std::thread &cs: consumers)
   {
      cs = std::thread
         ([&,num]
          {
             while(!endprod || !q.empty())
             {
                size_t c = 0 ;
                if (q.pop(c))
                   v[num].push_back(c) ;
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
             for (size_t i = start_from, end_with = start_from + per_thread ; i < end_with ; ++i)
                q.push(i) ;
          }) ;
      start_from += per_thread ;
   }

   for (std::thread &p: producers)
      CPPUNIT_LOG_RUN(p.join()) ;

   CPPUNIT_LOG_RUN(endprod = 1) ;

   for (std::thread &c: consumers)
      CPPUNIT_LOG_RUN(c.join()) ;

   CPPUNIT_LOG_ASSERT(q.empty()) ;

   CheckQueueResultConsistency(P, per_thread, std::begin(v), std::end(v)) ;
}

template<size_t P, size_t C, size_t K>
void ConcurrentDynQueueTests::Test_DualQueue_NxN()
{
   const size_t N = 3*5*7*16*K ;
   PCOMN_STATIC_CHECK(N % P == 0) ;
   const size_t per_thread = N/P ;

   CPPUNIT_LOG_LINE("****************** " << P << " producers, " << C << "  consumer(s), "
                    << N << " items, " << per_thread << " per producer thread *******************") ;

   std::thread producers[P] ;
   std::thread consumers[C] ;
   std::vector<size_t> v[C] ;

   concurrent_dualqueue<size_t> q ;

   size_t num = 0 ;
   for (std::thread &cs: consumers)
   {
      cs = std::thread
         ([&,num]
          {
             for (size_t c ; (c = q.pop()) != (size_t)-1 ; v[num].push_back(c)) ;
          }) ;
      ++num ;
   }

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

   CPPUNIT_LOG_RUN(for(int c = C ; c-- ; q.push_back(-1))) ;

   for (std::thread &c: consumers)
      CPPUNIT_LOG_RUN(c.join()) ;

   CPPUNIT_LOG_ASSERT(q.empty()) ;

   CheckQueueResultConsistency(P, per_thread, std::begin(v), std::end(v)) ;
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
