/*-*- tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_cdscrq.cpp
 COPYRIGHT    :   Yakov Markovitch, 2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for CRQ and LCRQ queues.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   7 Aug 2016
*******************************************************************************/
#include <pcomn_cdscrq.h>
#include <pcomn_iterator.h>
#include <pcomn_unittest.h>

#include "pcomn_testcds.h"

#include <thread>
#include <numeric>

using namespace pcomn ;

#if defined(_NDEBUG) || defined(__OPTIMIZE__)
static const size_t REPCOUNT = 50 ;
#else
static const size_t REPCOUNT = 5 ;
#endif

/*******************************************************************************
 class CRQTests
*******************************************************************************/
class CRQTests : public CppUnit::TestFixture {

      void Test_CRQ_Data() ;
      void Test_CRQ_SingleThread() ;

      CPPUNIT_TEST_SUITE(CRQTests) ;

      CPPUNIT_TEST(Test_CRQ_Data) ;
      CPPUNIT_TEST(Test_CRQ_SingleThread) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;


/*******************************************************************************
 CRQTests
*******************************************************************************/
void CRQTests::Test_CRQ_Data()
{
}

void CRQTests::Test_CRQ_SingleThread()
{
   typedef std::unique_ptr<std::string> string_ptr ;

   typedef crq<int>         int_crq ;
   typedef crq<string_ptr>  string_crq ;

   std::unique_ptr<int_crq>    i_crq0 ;
   std::unique_ptr<string_crq> s_crq0 ;

   CPPUNIT_LOG_RUN(i_crq0.reset(int_crq::make_crq(1, 0))) ;
   CPPUNIT_LOG_RUN(s_crq0.reset(string_crq::make_crq(1, 0))) ;

   CPPUNIT_LOG_ASSERT(i_crq0->empty()) ;
   CPPUNIT_LOG_ASSERT(s_crq0->empty()) ;

   /*
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
   */
}

/*******************************************************************************

*******************************************************************************/
int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(CRQTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv, nullptr, "Tests of CRQ and LCRQ") ;
}
