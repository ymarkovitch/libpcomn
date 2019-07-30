/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_bitarray.cpp
 COPYRIGHT    :   Yakov Markovitch, 2006-2019. All rights reserved.
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
 class BitVectorTests
*******************************************************************************/
class BitVectorTests : public CppUnit::TestFixture {

      void Test_Constructors() ;
      void Test_Set_Reset_Bits() ;
      void Test_Bit_Count() ;
      void Test_Bit_Search() ;
      template<typename I>
      void Test_Positional_Iterator() ;
      void Test_Atomic_Set_Reset_Bits() ;

      CPPUNIT_TEST_SUITE(BitVectorTests) ;

      CPPUNIT_TEST(Test_Constructors) ;
      CPPUNIT_TEST(Test_Set_Reset_Bits) ;
      CPPUNIT_TEST(Test_Bit_Count) ;
      CPPUNIT_TEST(Test_Bit_Search) ;
      CPPUNIT_TEST(Test_Positional_Iterator<uint32_t>) ;
      CPPUNIT_TEST(Test_Positional_Iterator<uint64_t>) ;
      CPPUNIT_TEST(Test_Atomic_Set_Reset_Bits) ;

      CPPUNIT_TEST_SUITE_END() ;
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
 BitVectorTests
*******************************************************************************/
void BitVectorTests::Test_Constructors()
{
   basic_bitvector<uint64_t> empty_64 ;
   basic_bitvector<uint32_t> empty_32 ;

   CPPUNIT_LOG_IS_NULL(empty_64.data()) ;
   CPPUNIT_LOG_IS_NULL(empty_32.data()) ;
   CPPUNIT_LOG_EQ(empty_64.size(), 0) ;
   CPPUNIT_LOG_EQ(empty_32.size(), 0) ;
   CPPUNIT_LOG_EQ(empty_64.nelements(), 0) ;
   CPPUNIT_LOG_EQ(empty_32.nelements(), 0) ;

   CPPUNIT_LOG_EQ(string_cast(empty_64), "") ;
   CPPUNIT_LOG_EQ(string_cast(empty_32), "") ;

   uint32_t v1[] = { 0, 4 } ;
   uint64_t v2[] = { 0, 0, 0x8000000000000002ULL } ;
   const unsigned long long v3[] = { 0x0800000000000055LL } ;

   auto bv1 = make_bitvector(v1) ;
   auto bv2 = make_bitvector(v2 + 0, 2) ;

   auto bv3 = make_bitvector(v3) ;

   CPPUNIT_LOG_EQ(bv1.size(), 64) ;
   CPPUNIT_LOG_EQ(bv1.nelements(), 2) ;

   CPPUNIT_LOG_EQ(bv2.size(), 128) ;
   CPPUNIT_LOG_EQ(bv2.nelements(), 2) ;

   CPPUNIT_LOG_EQ(bv3.size(), 64) ;
   CPPUNIT_LOG_EQ(bv3.nelements(), 1) ;

   CPPUNIT_LOG_EQ(string_cast(bv1), "0000000000000000000000000000000000100000000000000000000000000000") ;
   CPPUNIT_LOG_EQ(string_cast(bv2), std::string(128, '0')) ;
   CPPUNIT_LOG_EQ(string_cast(bv3), "1010101000000000000000000000000000000000000000000000000000010000") ;

   CPPUNIT_LOG_ASSERT(bv3.test(0)) ;
   CPPUNIT_LOG_IS_FALSE(bv3.test(1)) ;
   CPPUNIT_LOG_ASSERT(bv3.test(2)) ;
   CPPUNIT_LOG_IS_FALSE(bv3.test(3)) ;

   CPPUNIT_LOG_IS_FALSE(bv3.test(58)) ;
   CPPUNIT_LOG_ASSERT(bv3.test(59)) ;

   CPPUNIT_LOG_IS_FALSE(bv1.test(31)) ;
   CPPUNIT_LOG_IS_FALSE(bv1.test(32)) ;
   CPPUNIT_LOG_IS_FALSE(bv1.test(33)) ;
   CPPUNIT_LOG_ASSERT(bv1.test(34)) ;
}

void BitVectorTests::Test_Set_Reset_Bits()
{
   uint32_t v1[] = { 0, 4 } ;
   uint64_t v2[] = { 0, 0, 0x8000000000000002ULL } ;

   auto bv1 = make_bitvector(v1) ;
   auto bv2 = make_bitvector(v2 + 0, 2) ;
   auto bv3 = make_bitvector(v2) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(bv1.flip(1)) ;
   CPPUNIT_LOG_ASSERT(bv1.test(1)) ;
   CPPUNIT_LOG_EQ(string_cast(bv1), "0100000000000000000000000000000000100000000000000000000000000000") ;

   CPPUNIT_LOG_IS_FALSE(bv1.set(4)) ;
   CPPUNIT_LOG_IS_FALSE(bv1.set(63)) ;
   CPPUNIT_LOG_ASSERT(bv1.set(1, false)) ;
   CPPUNIT_LOG_EQ(string_cast(bv1), "0000100000000000000000000000000000100000000000000000000000000001") ;

   CPPUNIT_LOG_EQ(string_cast(bv2),
                  "0000000000000000000000000000000000000000000000000000000000000000"
                  "0000000000000000000000000000000000000000000000000000000000000000"
      ) ;

   CPPUNIT_LOG_EQ(string_cast(bv3),
                  "0000000000000000000000000000000000000000000000000000000000000000"
                  "0000000000000000000000000000000000000000000000000000000000000000"
                  "0100000000000000000000000000000000000000000000000000000000000001"
      ) ;

   CPPUNIT_LOG_IS_FALSE(bv2.set(65)) ;
   CPPUNIT_LOG_IS_FALSE(bv2.set(66)) ;

   CPPUNIT_LOG_EQ(string_cast(bv2),
                  "0000000000000000000000000000000000000000000000000000000000000000"
                  "0110000000000000000000000000000000000000000000000000000000000000"
      ) ;

   CPPUNIT_LOG_EQ(string_cast(bv3),
                  "0000000000000000000000000000000000000000000000000000000000000000"
                  "0110000000000000000000000000000000000000000000000000000000000000"
                  "0100000000000000000000000000000000000000000000000000000000000001"
      ) ;
}

void BitVectorTests::Test_Bit_Count()
{
   basic_bitvector<uint64_t> empty_64 ;
   basic_bitvector<uint32_t> empty_32 ;

   CPPUNIT_LOG_EQ(empty_64.count(true), 0) ;
   CPPUNIT_LOG_EQ(empty_64.count(false), 0) ;
   CPPUNIT_LOG_EQ(empty_64.count(), 0) ;

   CPPUNIT_LOG_EQ(empty_32.count(true), 0) ;
   CPPUNIT_LOG_EQ(empty_32.count(false), 0) ;
   CPPUNIT_LOG_EQ(empty_32.count(), 0) ;

   uint32_t v1[] = { 0, 4 } ;
   uint64_t v2[] = { 0, 0, 0x8000000000000002ULL } ;
   const unsigned long long v3[] = { 0x0800000000000055LL } ;

   auto bv1 = make_bitvector(v1) ;
   auto bv2 = make_bitvector(v2) ;
   auto bv3 = make_bitvector(v3) ;

   CPPUNIT_LOG_EQ(bv1.count(true), 1) ;
   CPPUNIT_LOG_EQ(bv1.count(false), 63) ;
   CPPUNIT_LOG_EQ(bv1.count(), 1) ;

   CPPUNIT_LOG_EQ(bv2.count(true), 2) ;
   CPPUNIT_LOG_EQ(bv2.count(false), 190) ;
   CPPUNIT_LOG_EQ(bv2.count(), 2) ;

   CPPUNIT_LOG_EQ(bv3.count(true), 5) ;
   CPPUNIT_LOG_EQ(bv3.count(false), 59) ;
   CPPUNIT_LOG_EQ(bv3.count(), 5) ;
}

void BitVectorTests::Test_Bit_Search()
{
   basic_bitvector<uint64_t> empty_64 ;
   basic_bitvector<uint32_t> empty_32 ;

   uint64_t v0_64[] = { 2 } ;
   uint32_t v0_32[] = { 2 } ;

   uint32_t v1[] = { 0, 4 } ;
   uint64_t v2[] = { 0, 0, 0x8000000000000002ULL } ;
   const unsigned long long v3[] = { 0x0800000000000055LL } ;

   auto bv1 = make_bitvector(v1) ;
   auto bv2 = make_bitvector(v2 + 0, 2) ;
   auto bv2_full = make_bitvector(v2) ;

   auto bv3 = make_bitvector(v3) ;

   auto bv0_64 = make_bitvector(v0_64) ;
   auto bv0_32 = make_bitvector(v0_32) ;

   CPPUNIT_LOG_EQ(empty_64.find_first_bit<1>(0), 0) ;
   CPPUNIT_LOG_EQ(empty_64.find_first_bit<1>(0, 0), 0) ;
   CPPUNIT_LOG_EQ(empty_64.find_first_bit<1>(2, 1), 0) ;
   CPPUNIT_LOG_EQ(empty_32.find_first_bit<1>(0), 0) ;
   CPPUNIT_LOG_EQ(empty_32.find_first_bit<1>(0, 0), 0) ;
   CPPUNIT_LOG_EQ(empty_32.find_first_bit<1>(2, 1), 0) ;

   CPPUNIT_LOG_EQ(empty_64.find_first_bit<0>(0), 0) ;
   CPPUNIT_LOG_EQ(empty_64.find_first_bit<0>(0, 0), 0) ;
   CPPUNIT_LOG_EQ(empty_64.find_first_bit<0>(2, 1), 0) ;
   CPPUNIT_LOG_EQ(empty_32.find_first_bit<0>(0), 0) ;
   CPPUNIT_LOG_EQ(empty_32.find_first_bit<0>(0, 0), 0) ;
   CPPUNIT_LOG_EQ(empty_32.find_first_bit<0>(2, 1), 0) ;
   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_EQ(bv0_64.find_first_bit<1>(0), 1) ;
   CPPUNIT_LOG_EQ(bv0_64.find_first_bit<1>(0, 0), 0) ;
   CPPUNIT_LOG_EQ(bv0_64.find_first_bit<1>(2, 1), 1) ;
   CPPUNIT_LOG_EQ(bv0_64.find_first_bit<1>(2), 64) ;

   CPPUNIT_LOG_EQ(bv0_64.find_first_bit<0>(0), 0) ;
   CPPUNIT_LOG_EQ(bv0_64.find_first_bit<0>(0, 0), 0) ;
   CPPUNIT_LOG_EQ(bv0_64.find_first_bit<0>(2, 1), 1) ;
   CPPUNIT_LOG_EQ(bv0_64.find_first_bit<0>(1), 2) ;
   CPPUNIT_LOG_EQ(bv0_64.find_first_bit<0>(2), 2) ;

   CPPUNIT_LOG_EQ(bv0_32.find_first_bit<1>(0), 1) ;
   CPPUNIT_LOG_EQ(bv0_32.find_first_bit<1>(0, 0), 0) ;
   CPPUNIT_LOG_EQ(bv0_32.find_first_bit<1>(2, 1), 1) ;
   CPPUNIT_LOG_EQ(bv0_32.find_first_bit<1>(2), 32) ;

   CPPUNIT_LOG_EQ(bv0_32.find_first_bit<0>(0), 0) ;
   CPPUNIT_LOG_EQ(bv0_32.find_first_bit<0>(0, 0), 0) ;
   CPPUNIT_LOG_EQ(bv0_32.find_first_bit<0>(2, 1), 1) ;
   CPPUNIT_LOG_EQ(bv0_32.find_first_bit<0>(1), 2) ;
   CPPUNIT_LOG_EQ(bv0_32.find_first_bit<0>(2), 2) ;

   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_EQ(bv1.find_first_bit<1>(), 34) ;
   CPPUNIT_LOG_EQ(bv1.find_first_bit<0>(), 0) ;
   CPPUNIT_LOG_EQ(bv1.find_first_bit<1>(34), 34) ;
   CPPUNIT_LOG_EQ(bv1.find_first_bit<1>(35), 64) ;

   CPPUNIT_LOG_EQ(bv3.find_first_bit<1>(), 0) ;
   CPPUNIT_LOG_EQ(bv3.find_first_bit<1>(1), 2) ;
   CPPUNIT_LOG_EQ(bv3.find_first_bit<1>(3), 4) ;
   CPPUNIT_LOG_EQ(bv3.find_first_bit<1>(5), 6) ;
   CPPUNIT_LOG_EQ(bv3.find_first_bit<1>(7), 59) ;
   CPPUNIT_LOG_EQ(bv3.find_first_bit<1>(60), 64) ;

   CPPUNIT_LOG_EQ(bv2.size(), 128) ;
   CPPUNIT_LOG_EQ(bv2.find_first_bit<1>(), 128) ;
   CPPUNIT_LOG_EQ(bv2.find_first_bit<0>(), 0) ;

   CPPUNIT_LOG_EQ(bv2_full.size(), 192) ;
   CPPUNIT_LOG_EQ(bv2_full.find_first_bit<1>(), 129) ;
   CPPUNIT_LOG_EQ(bv2_full.find_first_bit<0>(129), 130) ;
   CPPUNIT_LOG_EQ(bv2_full.find_first_bit<1>(130), 191) ;
}

template<typename I>
void BitVectorTests::Test_Positional_Iterator()
{
   typedef typename basic_bitvector<I>::template positional_iterator<true> positional_iterator ;

   basic_bitvector<I> bv_empty ;
   CPPUNIT_LOG_ASSERT(bv_empty.begin_positional() == bv_empty.end_positional()) ;

   I vdata[4096/bitsizeof(I)]  = {} ;

   auto bv = make_bitvector(vdata) ;

   set_bits(bv, {36, 44, 48, 52, 64, 70, 72, 76, 100, 208}) ;

   auto bp = bv.begin_positional() ;
   const auto ep = bv.end_positional() ;

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

   bp = positional_iterator(bv, 36) ;
   CPPUNIT_LOG_ASSERT(bp != ep) ;
   CPPUNIT_LOG_EQ(*bp, 36) ;
   CPPUNIT_LOG_ASSERT(++bp != ep) ;
   CPPUNIT_LOG_EQ(*bp, 44) ;

   bp = positional_iterator(bv, 127) ;
   CPPUNIT_LOG_ASSERT(bp != ep) ;
   CPPUNIT_LOG_EQ(*bp, 208) ;
   CPPUNIT_LOG_ASSERT(++bp == ep) ;

   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_RUN(std::fill_n(bv.data(), bv.nelements(), 0)) ;
   CPPUNIT_LOG_RUN(bp = bv.begin_positional()) ;
   CPPUNIT_LOG_ASSERT(bp == ep) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(bv.set(4095)) ;
   CPPUNIT_LOG_RUN(bp = bv.begin_positional()) ;
   CPPUNIT_LOG_ASSERT(bp != ep) ;
   CPPUNIT_LOG_EQ(*bp, 4095) ;
   CPPUNIT_LOG_ASSERT(++bp == ep) ;
}

void BitVectorTests::Test_Atomic_Set_Reset_Bits()
{
   uint32_t v1[] = { 0, 4 } ;
   uint64_t v2[] = { 0, 0, 0x8000000000000002ULL } ;

   auto bv1 = make_bitvector(v1) ;
   auto bv2 = make_bitvector(v2 + 0, 2) ;
   auto bv3 = make_bitvector(v2) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(bv1.flip(1, std::memory_order_acq_rel)) ;
   CPPUNIT_LOG_ASSERT(bv1.test(1, std::memory_order_acq_rel)) ;
   CPPUNIT_LOG_EQ(string_cast(bv1), "0100000000000000000000000000000000100000000000000000000000000000") ;

   CPPUNIT_LOG_IS_FALSE(bv1.set(4, std::memory_order_acq_rel)) ;
   CPPUNIT_LOG_IS_FALSE(bv1.set(63, std::memory_order_acq_rel)) ;
   CPPUNIT_LOG_ASSERT(bv1.set(1, false, std::memory_order_acq_rel)) ;
   CPPUNIT_LOG_EQ(string_cast(bv1), "0000100000000000000000000000000000100000000000000000000000000001") ;

   CPPUNIT_LOG_EQ(string_cast(bv2),
                  "0000000000000000000000000000000000000000000000000000000000000000"
                  "0000000000000000000000000000000000000000000000000000000000000000"
      ) ;

   CPPUNIT_LOG_EQ(string_cast(bv3),
                  "0000000000000000000000000000000000000000000000000000000000000000"
                  "0000000000000000000000000000000000000000000000000000000000000000"
                  "0100000000000000000000000000000000000000000000000000000000000001"
      ) ;

   CPPUNIT_LOG_IS_FALSE(bv2.set(65, std::memory_order_acq_rel)) ;
   CPPUNIT_LOG_IS_FALSE(bv2.set(66, std::memory_order_acq_rel)) ;

   CPPUNIT_LOG_EQ(string_cast(bv2),
                  "0000000000000000000000000000000000000000000000000000000000000000"
                  "0110000000000000000000000000000000000000000000000000000000000000"
      ) ;

   CPPUNIT_LOG_EQ(string_cast(bv3),
                  "0000000000000000000000000000000000000000000000000000000000000000"
                  "0110000000000000000000000000000000000000000000000000000000000000"
                  "0100000000000000000000000000000000000000000000000000000000000001"
      ) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(bv2.flip(1, std::memory_order_relaxed)) ;
   CPPUNIT_LOG_EQ(string_cast(bv2),
                  "0100000000000000000000000000000000000000000000000000000000000000"
                  "0110000000000000000000000000000000000000000000000000000000000000"
      ) ;
   CPPUNIT_LOG_IS_FALSE(bv2.flip(65, std::memory_order_relaxed)) ;
   CPPUNIT_LOG_EQ(string_cast(bv2),
                  "0100000000000000000000000000000000000000000000000000000000000000"
                  "0010000000000000000000000000000000000000000000000000000000000000"
      ) ;
   CPPUNIT_LOG_ASSERT(bv2.flip(65, std::memory_order_relaxed)) ;
   CPPUNIT_LOG_EQ(string_cast(bv2),
                  "0100000000000000000000000000000000000000000000000000000000000000"
                  "0110000000000000000000000000000000000000000000000000000000000000"
      ) ;

   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_ASSERT(bv2.cas(68, false, true)) ;
   CPPUNIT_LOG_EQ(string_cast(bv2),
                  "0100000000000000000000000000000000000000000000000000000000000000"
                  "0110100000000000000000000000000000000000000000000000000000000000"
      ) ;

   CPPUNIT_LOG_IS_FALSE(bv2.cas(3, true, true)) ;
   CPPUNIT_LOG_EQ(string_cast(bv2),
                  "0100000000000000000000000000000000000000000000000000000000000000"
                  "0110100000000000000000000000000000000000000000000000000000000000"
      ) ;

   CPPUNIT_LOG_ASSERT(bv2.cas(3, false, false)) ;
   CPPUNIT_LOG_EQ(string_cast(bv2),
                  "0100000000000000000000000000000000000000000000000000000000000000"
                  "0110100000000000000000000000000000000000000000000000000000000000"
      ) ;

   CPPUNIT_LOG_ASSERT(bv2.cas(3, false, true, std::memory_order_relaxed)) ;
   CPPUNIT_LOG_EQ(string_cast(bv2),
                  "0101000000000000000000000000000000000000000000000000000000000000"
                  "0110100000000000000000000000000000000000000000000000000000000000"
      ) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(BitArrayTests::suite()) ;
   runner.addTest(BitVectorTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "unittest.diag.ini", "Bitarray and bitvector unittests") ;
}
