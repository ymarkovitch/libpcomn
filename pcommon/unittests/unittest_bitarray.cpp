/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_bitarray.cpp
 COPYRIGHT    :   Yakov Markovitch, 2006-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unit tests of pcomn::bitarray container

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   23 Nov 2006
*******************************************************************************/
#include <pcomn_bitarray.h>
#include <pcomn_unittest.h>

#include <initializer_list>

using namespace pcomn ;

static bitarray &set_bits(bitarray &a, std::initializer_list<unsigned> bits, bool value = true)
{
   for (unsigned pos: bits)
      a.set(pos, value) ;
   return a ;
}

class BitArrayTests : public CppUnit::TestFixture {

      void Test_Constructors() ;
      void Test_Bit_Count() ;
      void Test_Bit_Search() ;
      void Test_Positional_Iterator() ;

      CPPUNIT_TEST_SUITE(BitArrayTests) ;

      CPPUNIT_TEST(Test_Constructors) ;
      CPPUNIT_TEST(Test_Bit_Count) ;
      CPPUNIT_TEST(Test_Bit_Search) ;
      CPPUNIT_TEST(Test_Positional_Iterator) ;

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

   CPPUNIT_LOG_ASSERT(b65_02.flip(64)) ;
   CPPUNIT_LOG_EQ(b65_02.count(), 1) ;
   CPPUNIT_LOG_ASSERT(b65_02.any()) ;
   CPPUNIT_LOG_IS_FALSE(b65_02.all()) ;
   CPPUNIT_LOG_IS_FALSE(b65_02.none()) ;

   CPPUNIT_LOG_EQ(string_cast(b65_02), std::string(64, '0') + "1") ;
   CPPUNIT_LOG_EQ(b65_02.set(63).count(), 2) ;
   CPPUNIT_LOG_EQ(string_cast(b65_02), std::string(63, '0') + "11") ;

   bitarray b65_03 (std::move(b65_02)) ;
   CPPUNIT_LOG_EQ(string_cast(b65_03), std::string(63, '0') + "11") ;
   CPPUNIT_LOG_EQ(string_cast(b65_02), "") ;

   bitarray b127_01 (127) ;
   CPPUNIT_LOG_EQ(b127_01.size(), 127) ;

   CPPUNIT_LOG_EQ(string_cast(b127_01), std::string(127, '0')) ;
   CPPUNIT_LOG_IS_FALSE(b127_01.any()) ;
   CPPUNIT_LOG_IS_FALSE(b127_01.all()) ;
   CPPUNIT_LOG_ASSERT(b127_01.none()) ;

   bitarray b129_01 (129, true) ;
   CPPUNIT_LOG_EQ(b129_01.size(), 129) ;

   CPPUNIT_LOG_EQ(string_cast(b129_01), std::string(129, '1')) ;
   CPPUNIT_LOG_ASSERT(b129_01.all()) ;

   bitarray b128_01 (128, true) ;
   CPPUNIT_LOG_EQ(b128_01.size(), 128) ;

   CPPUNIT_LOG_EQ(string_cast(b128_01), std::string(128, '1')) ;
   CPPUNIT_LOG_ASSERT(b128_01.all()) ;
}

void BitArrayTests::Test_Bit_Count()
{
   bitarray empty ;
   CPPUNIT_LOG_EQ(empty.count(true), 0) ;
   CPPUNIT_LOG_EQ(empty.count(false), 0) ;
   CPPUNIT_LOG_EQ(empty.count(), 0) ;

   bitarray b1_01 (1) ;
   CPPUNIT_LOG_EQ(b1_01.count(true), 0) ;
   CPPUNIT_LOG_EQ(b1_01.count(false), 1) ;
   CPPUNIT_LOG_EQ(b1_01.count(), 0) ;

   bitarray b1_02 (1, true) ;
   CPPUNIT_LOG_EQ(b1_02.count(true), 1) ;
   CPPUNIT_LOG_EQ(b1_02.count(false), 0) ;
   CPPUNIT_LOG_EQ(b1_02.count(), 1) ;
}

void BitArrayTests::Test_Bit_Search()
{
   bitarray empty ;
   CPPUNIT_LOG_EQ(empty.find_first_bit(0), 0) ;
   CPPUNIT_LOG_EQ(empty.find_first_bit(0, 0), 0) ;
   CPPUNIT_LOG_EQ(empty.find_first_bit(2, 1), 0) ;

   bitarray b1_01 (1) ;
   CPPUNIT_LOG_EQ(b1_01.find_first_bit(0), 1) ;
   CPPUNIT_LOG_EQ(b1_01.find_first_bit(0, 0), 0) ;
   CPPUNIT_LOG_EQ(b1_01.find_first_bit(2, 1), 1) ;

   bitarray b1_02 (1, true) ;
   CPPUNIT_LOG_EQ(b1_02.find_first_bit(0), 0) ;
   CPPUNIT_LOG_EQ(b1_02.find_first_bit(1), 1) ;
   CPPUNIT_LOG_EQ(b1_02.find_first_bit(0, 0), 0) ;
   CPPUNIT_LOG_EQ(b1_02.find_first_bit(2, 1), 1) ;

   bitarray b127_01 (127) ;
   CPPUNIT_LOG_RUN(b127_01.set(126)) ;
   CPPUNIT_LOG_EQ(b127_01.find_first_bit(0), 126) ;
   CPPUNIT_LOG_EQ(b127_01.find_first_bit(126), 126) ;
   CPPUNIT_LOG_EQ(b127_01.count(), 1) ;

   CPPUNIT_LOG_RUN(b127_01.set(124)) ;
   CPPUNIT_LOG_EQ(b127_01.find_first_bit(), 124) ;
   CPPUNIT_LOG_EQ(b127_01.find_first_bit(0, 120), 120) ;
   CPPUNIT_LOG_EQ(b127_01.find_first_bit(124), 124) ;
   CPPUNIT_LOG_EQ(b127_01.find_first_bit(125), 126) ;


   CPPUNIT_LOG_RUN(b127_01.set(63)) ;
   CPPUNIT_LOG_EQ(b127_01.count(), 3) ;
   CPPUNIT_LOG_EQ(b127_01.find_first_bit(), 63) ;
   CPPUNIT_LOG_EQ(b127_01.find_first_bit(64), 124) ;

   CPPUNIT_LOG_RUN(b127_01.set(64)) ;
   CPPUNIT_LOG_EQ(b127_01.count(), 4) ;
   CPPUNIT_LOG_EQ(b127_01.find_first_bit(), 63) ;
   CPPUNIT_LOG_EQ(b127_01.find_first_bit(64), 64) ;

   CPPUNIT_LOG_RUN(b127_01.set(63, false)) ;
   CPPUNIT_LOG_EQ(b127_01.count(), 3) ;
   CPPUNIT_LOG_EQ(b127_01.find_first_bit(), 64) ;
}

void BitArrayTests::Test_Positional_Iterator()
{
   bitarray b1 (4096) ;
   set_bits(b1, {36, 44, 48, 52, 64, 70, 72, 76, 100, 208}) ;
   CPPUNIT_LOG_EQ(std::vector<unsigned>(b1.begin_positional(), b1.end_positional()),
                  (std::vector<unsigned>{36, 44, 48, 52, 64, 70, 72, 76, 100, 208})) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(BitArrayTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "unittest.diag.ini", "FooTest") ;
}
