/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_bitarray.cpp
 COPYRIGHT    :   Yakov Markovitch, 2006-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unit tests of pcomn::bitarray container

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   23 Nov 2006
*******************************************************************************/
#include <pcomn_bitarray.h>
#include <pcomn_unittest.h>

using namespace pcomn ;

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
   bitarray empty ;
   CPPUNIT_LOG_EQ(string_cast(empty), "") ;
   CPPUNIT_LOG_IS_FALSE(empty.any()) ;
   CPPUNIT_LOG_ASSERT(empty.none()) ;
   CPPUNIT_LOG_ASSERT(empty.all()) ;

   bitarray b1_01 (1) ;
   CPPUNIT_LOG_EQ(b1_01.size(), 1) ;

   CPPUNIT_LOG_EQ(string_cast(b1_01), "0") ;
   CPPUNIT_LOG_IS_FALSE(b1_01.any()) ;
   CPPUNIT_LOG_IS_FALSE(b1_01.all()) ;
   CPPUNIT_LOG_ASSERT(b1_01.none()) ;
   CPPUNIT_LOG_EQ(b1_01.count(), 0) ;

   CPPUNIT_LOG_EQ(string_cast(b1_01.set(0)), "1") ;
   CPPUNIT_LOG_ASSERT(b1_01.any()) ;
   CPPUNIT_LOG_IS_FALSE(b1_01.none()) ;
   CPPUNIT_LOG_EQ(b1_01.count(), 1) ;

   CPPUNIT_LOG_EQ(string_cast(b1_01.set()), "1") ;
   CPPUNIT_LOG_ASSERT(b1_01.any()) ;
   CPPUNIT_LOG_ASSERT(b1_01.all()) ;
   CPPUNIT_LOG_IS_FALSE(b1_01.none()) ;
   CPPUNIT_LOG_EQ(b1_01.count(), 1) ;

   CPPUNIT_LOG_EQ(string_cast(b1_01.reset()), "0") ;
   CPPUNIT_LOG_IS_FALSE(b1_01.any()) ;
   CPPUNIT_LOG_ASSERT(b1_01.none()) ;
   CPPUNIT_LOG_EQ(b1_01.count(), 0) ;

   bitarray b65_01 (65, true) ;
   CPPUNIT_LOG_EQ(b65_01.size(), 65) ;

   CPPUNIT_LOG_EQ(string_cast(b65_01), std::string(65, '1')) ;
   CPPUNIT_LOG_ASSERT(b65_01.any()) ;
   CPPUNIT_LOG_ASSERT(b65_01.all()) ;
   CPPUNIT_LOG_IS_FALSE(b65_01.none()) ;
   CPPUNIT_LOG_EQ(b65_01.count(), 65) ;

   CPPUNIT_LOG_EQUAL(b65_01, ~~b65_01) ;
   CPPUNIT_LOG_EQ(string_cast(~b65_01), std::string(65, '0')) ;

   bitarray b65_02 (b65_01) ;
   CPPUNIT_LOG_EQUAL(b65_01, b65_02) ;
   CPPUNIT_LOG_NOT_EQUAL(b65_01, b65_02.flip()) ;
   CPPUNIT_LOG_NOT_EQUAL(b65_01, b65_02) ;
   CPPUNIT_LOG_EQ(b65_01.count(), 65) ;
   CPPUNIT_LOG_EQ(b65_02.count(), 0) ;

   CPPUNIT_LOG_EQ(b65_02.flip(64).count(), 1) ;
   CPPUNIT_LOG_ASSERT(b65_02.any()) ;
   CPPUNIT_LOG_IS_FALSE(b65_02.all()) ;
   CPPUNIT_LOG_IS_FALSE(b65_02.none()) ;

   CPPUNIT_LOG_EQ(string_cast(b65_02), std::string(64, '0') + "1") ;
   CPPUNIT_LOG_EQ(b65_02.set(63).count(), 2) ;
   CPPUNIT_LOG_EQ(string_cast(b65_02), std::string(63, '0') + "11") ;

   bitarray b127_01 (127) ;
   CPPUNIT_LOG_EQ(b127_01.size(), 127) ;

   CPPUNIT_LOG_EQ(string_cast(b127_01), std::string(127, '0')) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(BitArrayTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "unittest.diag.ini", "FooTest") ;
}
