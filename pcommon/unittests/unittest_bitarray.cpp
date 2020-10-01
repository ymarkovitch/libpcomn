/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_bitarray.cpp
 COPYRIGHT    :   Yakov Markovitch, 2006-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unit tests of pcomn::bitarray container

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   23 Nov 2006
*******************************************************************************/
#include <pcomn_bitarray.h>
#include <pcomn_bitvector.h>
#include <pcomn_unittest.h>

#include <initializer_list>

using namespace pcomn ;

template<typename T>
static T &set_bits(T &a, std::initializer_list<unsigned> bits, bool value = true)
{
   for (unsigned pos: bits)
      a.set(pos, value) ;
   return a ;
}

/*******************************************************************************
 class BitArrayTests
*******************************************************************************/
class BitArrayTests : public CppUnit::TestFixture {

      void Test_Constructors() ;
      void Test_Set_Reset_Bits() ;
      void Test_Bit_Count() ;
      void Test_Bit_Search() ;
      void Test_Positional_Iterator() ;

      CPPUNIT_TEST_SUITE(BitArrayTests) ;

      CPPUNIT_TEST(Test_Constructors) ;
      CPPUNIT_TEST(Test_Set_Reset_Bits) ;
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

/*******************************************************************************
 BitArrayTests
*******************************************************************************/
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

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQ(string_cast(1_bit), "1") ;
   CPPUNIT_LOG_EQ(string_cast(0_bit), "0") ;
   CPPUNIT_LOG_EXCEPTION(2_bit, std::invalid_argument) ;
   CPPUNIT_LOG_EQ(string_cast(0001_bit), "0001") ;
   CPPUNIT_LOG_EQ(string_cast(01111111100000000111111110000000011111111000000001111111100000000_bit),
                  "01111111100000000111111110000000011111111000000001111111100000000") ;
}

void BitArrayTests::Test_Set_Reset_Bits()
{
   bitarray b1_00 (1) ;
   bitarray b1_01 (1, true) ;

   CPPUNIT_LOG_NOT_EQUAL(b1_00, b1_01) ;
   CPPUNIT_LOG_IS_FALSE(b1_00 == b1_01) ;
   CPPUNIT_LOG_ASSERT(b1_00.none()) ;
   CPPUNIT_LOG_IS_FALSE(b1_00.any()) ;
   CPPUNIT_LOG_IS_FALSE(b1_00.all()) ;

   CPPUNIT_LOG_IS_FALSE(b1_01.none()) ;
   CPPUNIT_LOG_ASSERT(b1_01.any()) ;
   CPPUNIT_LOG_ASSERT(b1_01.all()) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(b1_00.set()) ;
   CPPUNIT_LOG_EQUAL(b1_00, b1_01) ;
   CPPUNIT_LOG_IS_FALSE(b1_00 != b1_01) ;

   CPPUNIT_LOG_IS_FALSE(b1_00.none()) ;
   CPPUNIT_LOG_ASSERT(b1_00.any()) ;
   CPPUNIT_LOG_ASSERT(b1_00.all()) ;

   CPPUNIT_LOG(std::endl) ;
   bitarray b2_00 (2) ;
   bitarray b2_01 (2, true) ;
   CPPUNIT_LOG_EQ(b2_00.size(), 2) ;

   CPPUNIT_LOG_IS_FALSE(b2_00 == b2_01) ;
   CPPUNIT_LOG_ASSERT(b2_00.none()) ;
   CPPUNIT_LOG_IS_FALSE(b2_00.any()) ;
   CPPUNIT_LOG_IS_FALSE(b2_00.all()) ;

   CPPUNIT_LOG_IS_FALSE(b2_01.none()) ;
   CPPUNIT_LOG_ASSERT(b2_01.any()) ;
   CPPUNIT_LOG_ASSERT(b2_01.all()) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(b2_01.flip(1)) ;
   CPPUNIT_LOG_EQ(string_cast(b2_01), "10") ;
   CPPUNIT_LOG_NOT_EQUAL(b2_00, b2_01) ;
   CPPUNIT_LOG_RUN(b2_00.flip(0)) ;
   CPPUNIT_LOG_EQUAL(b2_00, b2_01) ;

   CPPUNIT_LOG(std::endl) ;
   bitarray b65_00 (65) ;
   bitarray b65_01 (65, true) ;

   CPPUNIT_LOG_IS_FALSE(b65_00 == b65_01) ;
   CPPUNIT_LOG_ASSERT(b65_00.none()) ;
   CPPUNIT_LOG_IS_FALSE(b65_00.any()) ;
   CPPUNIT_LOG_IS_FALSE(b65_00.all()) ;

   CPPUNIT_LOG_IS_FALSE(b65_01.none()) ;
   CPPUNIT_LOG_ASSERT(b65_01.any()) ;
   CPPUNIT_LOG_ASSERT(b65_01.all()) ;

   CPPUNIT_LOG_RUN(for(int i = 0 ; i < 65 ; b65_00.set(i++));) ;
   CPPUNIT_LOG_EQUAL(b65_00, b65_01) ;
   CPPUNIT_LOG_RUN(b65_00.reset(64)) ;
   CPPUNIT_LOG_IS_FALSE(b65_01.flip(64)) ;
   CPPUNIT_LOG_EQUAL(b65_00, b65_01) ;

   CPPUNIT_LOG_RUN(b65_00.reset()) ;
   CPPUNIT_LOG_NOT_EQUAL(b65_00, b65_01) ;
   CPPUNIT_LOG_RUN(b65_00.set(64)) ;
   CPPUNIT_LOG_RUN(b65_01.flip()) ;
   CPPUNIT_LOG_EQUAL(b65_00, b65_01) ;
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

   bitarray b1_67 (67, true) ;
   CPPUNIT_LOG_EQ(b1_67.count(true), 67) ;
   CPPUNIT_LOG_EQ(b1_67.count(false), 0) ;

   CPPUNIT_LOG_RUN(b1_67.set(2, false)) ;
   CPPUNIT_LOG_IS_FALSE(b1_67.flip(65)) ;

   CPPUNIT_LOG_EQ(b1_67.count(true), 65) ;
   CPPUNIT_LOG_EQ(b1_67.count(false), 2) ;

   CPPUNIT_LOG_IS_FALSE(b1_67.flip(63)) ;

   CPPUNIT_LOG_EQ(b1_67.count(true), 64) ;
   CPPUNIT_LOG_EQ(b1_67.count(false), 3) ;

   CPPUNIT_LOG_RUN(b1_67.flip()) ;
   CPPUNIT_LOG_EQ(b1_67.count(true), 3) ;
   CPPUNIT_LOG_EQ(b1_67.count(false), 64) ;

   CPPUNIT_LOG_RUN(b1_67.reset()) ;
   CPPUNIT_LOG_EQ(b1_67.count(true), 0) ;
   CPPUNIT_LOG_EQ(b1_67.count(false), 67) ;

   CPPUNIT_LOG_RUN(b1_67.set()) ;
   CPPUNIT_LOG_EQ(b1_67.count(true), 67) ;
   CPPUNIT_LOG_EQ(b1_67.count(false), 0) ;
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
   auto bp = b1.begin_positional() ;
   const auto ep = b1.end_positional() ;
   CPPUNIT_LOG_ASSERT(bp != ep) ;
   CPPUNIT_LOG_EQ(*bp, 36) ;
   CPPUNIT_LOG_ASSERT(++bp != ep) ;
   CPPUNIT_LOG_EQ(*bp, 44) ;
   CPPUNIT_LOG_ASSERT(++bp != ep) ;
   CPPUNIT_LOG_EQ(*bp, 48) ;
   CPPUNIT_LOG_ASSERT(++bp != ep) ;
   CPPUNIT_LOG_EQ(*bp, 52) ;
   CPPUNIT_LOG_ASSERT(++bp != ep) ;
   CPPUNIT_LOG_EQ(*bp, 64) ;
   CPPUNIT_LOG_ASSERT(++bp != ep) ;
   CPPUNIT_LOG_EQ(*bp, 70) ;
   CPPUNIT_LOG_EQ(*bp, 70) ;
   CPPUNIT_LOG_ASSERT(++bp != ep) ;
   CPPUNIT_LOG_EQ(*bp, 72) ;
   CPPUNIT_LOG_ASSERT(++bp != ep) ;
   CPPUNIT_LOG_EQ(*bp, 76) ;
   CPPUNIT_LOG_ASSERT(++bp != ep) ;
   CPPUNIT_LOG_EQ(*bp, 100) ;
   CPPUNIT_LOG_ASSERT(++bp != ep) ;
   CPPUNIT_LOG_EQ(*bp, 208) ;
   CPPUNIT_LOG_ASSERT(++bp == ep) ;
   CPPUNIT_LOG_ASSERT(bp == ep) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(b1.reset()) ;
   CPPUNIT_LOG_RUN(bp = b1.begin_positional()) ;
   CPPUNIT_LOG_ASSERT(bp == ep) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(b1.set(4095)) ;
   CPPUNIT_LOG_RUN(bp = b1.begin_positional()) ;
   CPPUNIT_LOG_ASSERT(bp != ep) ;
   CPPUNIT_LOG_EQ(*bp, 4095) ;
   CPPUNIT_LOG_ASSERT(++bp == ep) ;
}

/*******************************************************************************
 main
*******************************************************************************/
int main(int argc, char *argv[])
{
   return unit::run_tests
       <
          BitArrayTests
       >
       (argc, argv) ;
}
