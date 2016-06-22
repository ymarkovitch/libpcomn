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

using namespace pcomn ;

class ConcurrentDynQueueTests : public CppUnit::TestFixture {

      void Test_CdsQueue_SingleThread() ;

      CPPUNIT_TEST_SUITE(ConcurrentDynQueueTests) ;

      CPPUNIT_TEST(Test_CdsQueue_SingleThread) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

/*******************************************************************************
 ConcurrentDynQueueTests
*******************************************************************************/
void ConcurrentDynQueueTests::Test_CdsQueue_SingleThread()
{
   typedef concurrent_dynqueue<std::string> cdsq_string ;

   cdsq_string empty0 ;
   std::string val0 = "Hello, world!" ;

   CPPUNIT_LOG_ASSERT(empty0.empty()) ;
   CPPUNIT_LOG_IS_FALSE(empty0.pop(val0)) ;
   CPPUNIT_LOG_ASSERT(empty0.empty()) ;
   CPPUNIT_LOG_EQ(val0, "Hello, world!") ;
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
