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
#include <pcomn_unittest.h>

#include <thread>

using namespace pcomn ;

class ConcurrentDynQueueTests : public CppUnit::TestFixture {

      void Test_CdsQueue_SingleThread() ;
      void Test_DualQueue_SingleThread() ;
      void Test_CdsQueues_Of_Movable() ;

      template<size_t producers, size_t count = 10000>
      void Test_CdsQueue_Nx1() ;

      CPPUNIT_TEST_SUITE(ConcurrentDynQueueTests) ;

      CPPUNIT_TEST(Test_CdsQueue_SingleThread) ;
      CPPUNIT_TEST(Test_DualQueue_SingleThread) ;
      CPPUNIT_TEST(P_PASS(Test_CdsQueue_Nx1<1, 1>)) ;
      CPPUNIT_TEST(P_PASS(Test_CdsQueue_Nx1<1, 10000>)) ;
      CPPUNIT_TEST(P_PASS(Test_CdsQueue_Nx1<2, 1>)) ;
      CPPUNIT_TEST(P_PASS(Test_CdsQueue_Nx1<2, 10000>)) ;
      CPPUNIT_TEST(P_PASS(Test_CdsQueue_Nx1<3, 1>)) ;
      CPPUNIT_TEST(P_PASS(Test_CdsQueue_Nx1<3, 10000>)) ;
      CPPUNIT_TEST(P_PASS(Test_CdsQueue_Nx1<16, 1>)) ;
      CPPUNIT_TEST(P_PASS(Test_CdsQueue_Nx1<16, 10000>)) ;

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
