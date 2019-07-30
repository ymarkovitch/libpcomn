/*-*- tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   perftest_cdscrq.cpp
 COPYRIGHT    :   Yakov Markovitch, 2016-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Performance test for CRQ.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   18 Aug 2016
*******************************************************************************/
#include <pcomn_cdscrq.h>
#include <unittests/pcomn_testcds.h>

using namespace pcomn ;

/*******************************************************************************
 class CRQPerfTest
*******************************************************************************/
class CRQPerfTest : public CppUnit::TestFixture {

      void Test_Queue_Performance()
      {
         std::unique_ptr<crq<size_t>> q {crq<size_t>::make_crq(0)} ;
         unit::TantrumQueueTest(*q, producers, consumers, count, {0, 400}, {}, unit::CDSTST_NOCHECK) ;
      }

      CPPUNIT_TEST_SUITE(CRQPerfTest) ;

      CPPUNIT_TEST(Test_Queue_Performance) ;

      CPPUNIT_TEST_SUITE_END() ;

   public:
      static size_t producers ;
      static size_t consumers ;
      static size_t count ;
} ;

size_t CRQPerfTest::producers ;
size_t CRQPerfTest::consumers ;
size_t CRQPerfTest::count ;

/*******************************************************************************

*******************************************************************************/
int main(int argc, char *argv[])
{
   typedef CRQPerfTest test_type ;

   if (argc != 4)
      return 1 ;

   test_type::producers = atoll(argv[1]) ;
   test_type::consumers = atoll(argv[2]) ;
   test_type::count =     atoll(argv[3]) ;

   pcomn::unit::TestRunner runner ;
   runner.addTest(CRQPerfTest::suite()) ;

   return
      pcomn::unit::run_tests(runner, 1, argv, nullptr, "Concurrent Ring Queue (CRQ) performance test") ;
}
