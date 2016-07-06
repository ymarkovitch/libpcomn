/*-*- tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   perftest_cdsqueue.cpp
 COPYRIGHT    :   Yakov Markovitch, 2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Performance test for conqurrent queues.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   22 Jun 2016
*******************************************************************************/
#include <pcomn_cdsqueue.h>
#include <unittests/pcomn_testcds.h>

using namespace pcomn ;

/*******************************************************************************
 class DynQueuePerfTest
*******************************************************************************/
class DynQueuePerfTest : public CppUnit::TestFixture {

      void Test_Queue_Performace()
      {
         concurrent_dynqueue<size_t> q ;
         if (consumers == 1)
            unit::CdsQueueTest_Nx1(q, producers, count) ;
         else
            unit::CdsQueueTest_NxN(q, producers, consumers, count) ;
      }

      CPPUNIT_TEST_SUITE(DynQueuePerfTest) ;

      CPPUNIT_TEST(Test_Queue_Performace) ;

      CPPUNIT_TEST_SUITE_END() ;

   public:
      static size_t producers ;
      static size_t consumers ;
      static size_t count ;
} ;

size_t DynQueuePerfTest::producers ;
size_t DynQueuePerfTest::consumers ;
size_t DynQueuePerfTest::count ;

/*******************************************************************************

*******************************************************************************/
int main(int argc, char *argv[])
{
   typedef DynQueuePerfTest test_type ;

   if (argc != 4)
      return 1 ;

   test_type::producers = atoll(argv[1]) ;
   test_type::consumers = atoll(argv[2]) ;
   test_type::count =     atoll(argv[3]) ;

   pcomn::unit::TestRunner runner ;
   runner.addTest(DynQueuePerfTest::suite()) ;

   return
      pcomn::unit::run_tests(runner, 1, argv, nullptr, "Lock-free queue performance test") ;
}
