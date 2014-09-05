/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_bitarray.cpp
 COPYRIGHT    :   Yakov Markovitch, 2006-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unit tests of pcomn::bitarray container

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   23 Nov 2006
*******************************************************************************/
#include <pcomn_bitarray.h>
#include <pcomn_unittest.h>

class BitArrayTests : public CppUnit::TestFixture {

      void Test_Constructors() ;

      CPPUNIT_TEST_SUITE(BitArrayTests) ;

      CPPUNIT_TEST(Test_Constructors) ;

      CPPUNIT_TEST_SUITE_END() ;

   public:
      void setUp()
      {}

      void tearDown()
      {}
} ;

void BitArrayTests::Test_Constructors()
{
}


int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(BitArrayTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "unittest.diag.ini", "FooTest") ;
}
