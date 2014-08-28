/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_atomic.cpp
 COPYRIGHT    :   Yakov Markovitch, 2009-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Test atomic operations for different word sizes

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   17 Feb 2009
*******************************************************************************/
#include <pcomn_atomic.h>

#include <pcomn_unittest.h>

using namespace pcomn ;

class AtomicTests : public CppUnit::TestFixture {

      void Test_AtomicIncDec() ;
      void Test_AtomicXchg() ;
      void Test_CAS() ;

      CPPUNIT_TEST_SUITE(AtomicTests) ;

      CPPUNIT_TEST(Test_AtomicXchg) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

void AtomicTests::Test_AtomicXchg()
{
   int64_t i64_1, i64_2 ;
   uint64_t ui64_1, ui64_2 ;
   int32_t i32_1, i32_2 ;
   uint64_t ui32_1, ui32_2 ;

   CPPUNIT_LOG_RUN(i64_1 = i64_2 = 1) ;
   CPPUNIT_LOG_EQUAL(atomic_op::xchg(&i64_1, i64_2), (int64_t)1) ;
   CPPUNIT_LOG_EQUAL(i64_1, (int64_t)1) ;
   CPPUNIT_LOG_EQUAL(i64_1, i64_2) ;

   CPPUNIT_LOG_RUN(i64_2 = 2) ;
   CPPUNIT_LOG_EQUAL(atomic_op::xchg(&i64_1, i64_2), (int64_t)1) ;
   CPPUNIT_LOG_EQUAL(i64_1, (int64_t)2) ;
   CPPUNIT_LOG_EQUAL(i64_1, i64_2) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(ui64_1 = ui64_2 = 1) ;
   CPPUNIT_LOG_EQUAL(atomic_op::xchg(&ui64_1, ui64_2), (uint64_t)1) ;
   CPPUNIT_LOG_EQUAL(ui64_1, (uint64_t)1) ;
   CPPUNIT_LOG_EQUAL(ui64_1, ui64_2) ;

   CPPUNIT_LOG_RUN(ui64_2 = 2) ;
   CPPUNIT_LOG_EQUAL(atomic_op::xchg(&ui64_1, ui64_2), (uint64_t)1) ;
   CPPUNIT_LOG_EQUAL(ui64_1, (uint64_t)2) ;
   CPPUNIT_LOG_EQUAL(ui64_1, ui64_2) ;

}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(AtomicTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv, "unittest.ini", "Test atomic operations") ;
}
