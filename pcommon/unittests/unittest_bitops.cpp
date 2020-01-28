/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_bitops.cpp
 COPYRIGHT    :   Yakov Markovitch, 2006-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittest for advanced bit operations (pcomn::bitops namespace)

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   28 Nov 2006
*******************************************************************************/
#include <pcomn_bitops.h>
#include <pcomn_primenum.h>
#include <pcomn_atomic.h>
#include <pcomn_unittest.h>

using namespace pcomn ;

/*******************************************************************************
 BitOperationsTests
*******************************************************************************/
class BitOperationsTests : public CppUnit::TestFixture {

      void Test_Bitsize() ;
      void Test_SignTraits() ;
      void Test_Bitcount() ;
      void Test_Bitcount_CompileTime() ;
      void Test_Broadcast() ;
      void Test_Clrrnzb() ;
      void Test_Getrnzb() ;
      void Test_NzbitIterator() ;
      void Test_NzbitPosIterator() ;
      void Test_BitRangeBoundary() ;
      void Test_OneOf() ;
      void Test_Log2() ;

      CPPUNIT_TEST_SUITE(BitOperationsTests) ;

      CPPUNIT_TEST(Test_Bitsize) ;
      CPPUNIT_TEST(Test_SignTraits) ;
      CPPUNIT_TEST(Test_Bitcount) ;
      CPPUNIT_TEST(Test_Bitcount_CompileTime) ;
      CPPUNIT_TEST(Test_Broadcast) ;
      CPPUNIT_TEST(Test_Clrrnzb) ;
      CPPUNIT_TEST(Test_Getrnzb) ;
      CPPUNIT_TEST(Test_NzbitIterator) ;
      CPPUNIT_TEST(Test_NzbitPosIterator) ;
      CPPUNIT_TEST(Test_BitRangeBoundary) ;
      CPPUNIT_TEST(Test_OneOf) ;
      CPPUNIT_TEST(Test_Log2) ;

      CPPUNIT_TEST_SUITE_END() ;

      template<typename T> bool isSigned(T) { return std::numeric_limits<T>::is_signed ; }
} ;

/*******************************************************************************
 BitOperationsTests
*******************************************************************************/
void BitOperationsTests::Test_Bitsize()
{
   CPPUNIT_LOG_EQUAL(int_traits<int8_t>::bitsize, 8U) ;
   CPPUNIT_LOG_EQUAL(int_traits<uint8_t>::bitsize, 8U) ;
   CPPUNIT_LOG_EQUAL(int_traits<int16_t>::bitsize, 16U) ;
   CPPUNIT_LOG_EQUAL(int_traits<uint16_t>::bitsize, 16U) ;
   CPPUNIT_LOG_EQUAL(int_traits<int32_t>::bitsize, 32U) ;
   CPPUNIT_LOG_EQUAL(int_traits<uint32_t>::bitsize, 32U) ;
   CPPUNIT_LOG_EQUAL(int_traits<int64_t>::bitsize, 64U) ;
   CPPUNIT_LOG_EQUAL(int_traits<uint64_t>::bitsize, 64U) ;
}

void BitOperationsTests::Test_SignTraits()
{
#define TEST_SIGNED_TRAITS(signed_t, unsigned_t)                        \
   CPPUNIT_LOG_IS_TRUE((std::is_same<signed_t, int_traits<signed_t>::stype>::value)) ; \
   CPPUNIT_LOG_IS_TRUE((std::is_same<signed_t, int_traits<unsigned_t>::stype>::value)) ; \
   CPPUNIT_LOG_IS_TRUE((std::is_same<unsigned_t, int_traits<signed_t>::utype>::value)) ; \
   CPPUNIT_LOG_IS_TRUE((std::is_same<unsigned_t, int_traits<unsigned_t>::utype>::value))

   TEST_SIGNED_TRAITS(signed char, unsigned char) ;
   TEST_SIGNED_TRAITS(short, unsigned short) ;
   TEST_SIGNED_TRAITS(int, unsigned) ;
   TEST_SIGNED_TRAITS(long, unsigned long) ;
   TEST_SIGNED_TRAITS(int64_t, uint64_t) ;
   TEST_SIGNED_TRAITS(long long, unsigned long long) ;

   CPPUNIT_LOG_IS_TRUE((std::is_same<signed char, int_traits<char>::stype>::value)) ;
   CPPUNIT_LOG_IS_TRUE((std::is_same<unsigned char, int_traits<char>::utype>::value)) ;

#undef TEST_SIGNED_TRAITS
}

void BitOperationsTests::Test_Bitcount()
{
   CPPUNIT_LOG_EQUAL(bitop::popcount(static_cast<int8_t>(0)), 0U) ;
   CPPUNIT_LOG_EQUAL(bitop::popcount(static_cast<uint8_t>(0)), 0U) ;
   CPPUNIT_LOG_EQUAL(bitop::popcount(static_cast<int16_t>(0)), 0U) ;
   CPPUNIT_LOG_EQUAL(bitop::popcount(static_cast<uint16_t>(0)), 0U) ;
   CPPUNIT_LOG_EQUAL(bitop::popcount(static_cast<int32_t>(0)), 0U) ;
   CPPUNIT_LOG_EQUAL(bitop::popcount(static_cast<uint32_t>(0)), 0U) ;
   CPPUNIT_LOG_EQUAL(bitop::popcount(static_cast<int64_t>(0)), 0U) ;
   CPPUNIT_LOG_EQUAL(bitop::popcount(static_cast<uint64_t>(0)), 0U) ;

   CPPUNIT_LOG_EQUAL(bitop::popcount(static_cast<int8_t>(-1)), 8U) ;
   CPPUNIT_LOG_EQUAL(bitop::popcount(static_cast<uint8_t>(-1)), 8U) ;
   CPPUNIT_LOG_EQUAL(bitop::popcount(static_cast<int16_t>(-1)), 16U) ;
   CPPUNIT_LOG_EQUAL(bitop::popcount(static_cast<uint16_t>(-1)), 16U) ;
   CPPUNIT_LOG_EQUAL(bitop::popcount(static_cast<int32_t>(-1)), 32U) ;
   CPPUNIT_LOG_EQUAL(bitop::popcount(static_cast<uint32_t>(-1)), 32U) ;
   CPPUNIT_LOG_EQUAL(bitop::popcount(static_cast<int64_t>(-1)), 64U) ;
   CPPUNIT_LOG_EQUAL(bitop::popcount(static_cast<uint64_t>((int64_t)-1)), 64U) ;

   CPPUNIT_LOG_EQUAL(bitop::popcount(static_cast<int8_t>(0x41)), 2U) ;
   CPPUNIT_LOG_EQUAL(bitop::popcount(static_cast<int8_t>(-1)), 8U) ;
   CPPUNIT_LOG_EQUAL(bitop::popcount(static_cast<uint8_t>(0x41)), 2U) ;
   CPPUNIT_LOG_EQUAL(bitop::popcount(static_cast<uint8_t>(0x43)), 3U) ;
   CPPUNIT_LOG_EQUAL(bitop::popcount(static_cast<uint8_t>(0x80)), 1U) ;
   CPPUNIT_LOG_EQUAL(bitop::popcount(static_cast<int32_t>(0xF1)), 5U) ;
   CPPUNIT_LOG_EQUAL(bitop::popcount(static_cast<int64_t>(0xF1)), 5U) ;
   CPPUNIT_LOG_EQUAL(bitop::popcount(static_cast<int32_t>(0x10000001)), 2U) ;

   // Test backward compatibility
   CPPUNIT_LOG_EQUAL(bitop::bitcount(static_cast<int32_t>(0x10000001)), 2U) ;
   CPPUNIT_LOG_EQUAL(bitop::bitcount(static_cast<int8_t>(0x41)), 2U) ;
}

void BitOperationsTests::Test_Bitcount_CompileTime()
{
   CPPUNIT_LOG_EQUAL(bitop::ct_popcount<0>::value, 0U) ;
   CPPUNIT_LOG_EQUAL(uint_constant<bitop::popcount(0)>(), uint_constant<0>()) ;

   CPPUNIT_LOG_EQUAL(bitop::ct_popcount<0x55>::value, 4U) ;
   CPPUNIT_LOG_EQUAL(bitop::ct_popcount<(unsigned)-1>::value, int_traits<unsigned>::bitsize) ;
   CPPUNIT_LOG_EQUAL(bitop::ct_popcount<0x20030055>::value, 7U) ;

   // Test backward compatibility
   CPPUNIT_LOG_EQUAL(bitop::ct_bitcount<0x20030055>::value, 7U) ;
}

void BitOperationsTests::Test_Broadcast()
{
   CPPUNIT_LOG_EQUAL(bitop::broadcasti<uint16_t>((uint8_t)0x50), (uint16_t)0x5050) ;
   CPPUNIT_LOG_EQUAL(bitop::broadcasti<uint8_t>((int8_t)0x50), (uint8_t)0x50) ;
   CPPUNIT_LOG_EQUAL(bitop::broadcasti<uint32_t>((uint8_t)0x50), (uint32_t)0x50505050) ;
   CPPUNIT_LOG_EQUAL(bitop::broadcasti<int32_t>((uint8_t)0x50), (int32_t)0x50505050) ;
   CPPUNIT_LOG_EQUAL(bitop::broadcasti<int64_t>(0x65432100), (int64_t)0x6543210065432100LL) ;
   CPPUNIT_LOG_EQUAL((bitop::broadcasti<uint64_t, int8_t>(0x65)), (uint64_t)0x65'65'65'65'65'65'65'65ULL) ;
}

void BitOperationsTests::Test_Clrrnzb()
{
   CPPUNIT_LOG_EQUAL(bitop::clrrnzb(0xF0), 0xE0) ;
   CPPUNIT_LOG_EQUAL(bitop::clrrnzb((uint32_t)0x80000000), (uint32_t)0) ;
   CPPUNIT_LOG_EQUAL(bitop::clrrnzb(0), 0) ;
   CPPUNIT_LOG_EQUAL(bitop::clrrnzb(1), 0) ;
   CPPUNIT_LOG_EQUAL(bitop::clrrnzb((signed char)3), (signed char)2) ;
}

void BitOperationsTests::Test_Getrnzb()
{
   CPPUNIT_LOG_EQUAL(bitop::getrnzb(0xF0), 0x10) ;
   CPPUNIT_LOG_EQ(bitop::rzcnt(0xF0), 4) ;
   CPPUNIT_LOG_EQUAL(bitop::getrnzb(0xF0ULL), 0x10ULL) ;
   CPPUNIT_LOG_EQ(bitop::rzcnt(0xF0ULL), 4) ;

   CPPUNIT_LOG_EQUAL(bitop::getrnzb(1), 1) ;
   CPPUNIT_LOG_EQUAL(bitop::getrnzb(-1), 1) ;
   CPPUNIT_LOG_EQUAL(bitop::getrnzb(6), 2) ;
   CPPUNIT_LOG_EQ(bitop::rzcnt(6), 1) ;

   CPPUNIT_LOG_EQ(bitop::rzcnt((uint64_t)0), 64) ;
   CPPUNIT_LOG_EQ(bitop::rzcnt((uint32_t)0), 32) ;
   CPPUNIT_LOG_EQ(bitop::rzcnt((uint16_t)0), 16) ;
   CPPUNIT_LOG_EQ(bitop::rzcnt((uint8_t)0),   8) ;
   CPPUNIT_LOG_EQ(bitop::rzcnt(1), 0) ;

   CPPUNIT_LOG_EQUAL(bitop::getrnzb((char)0x50), (char)0x10) ;
   CPPUNIT_LOG_EQUAL(bitop::getrnzb(0x5500000000000000ll), 0x100000000000000ll) ;
}

void BitOperationsTests::Test_NzbitIterator()
{
   CPPUNIT_LOG_IS_TRUE(bitop::nzbit_iterator<int32_t>() == bitop::nzbit_iterator<int32_t>()) ;
   CPPUNIT_LOG_IS_FALSE(bitop::nzbit_iterator<int32_t>() != bitop::nzbit_iterator<int32_t>()) ;
   CPPUNIT_LOG_IS_TRUE(bitop::nzbit_iterator<int32_t>(0x20005) ==
                       bitop::nzbit_iterator<int32_t>(0x20005)) ;
   CPPUNIT_LOG_IS_FALSE(bitop::nzbit_iterator<int32_t>(0x20005) !=
                        bitop::nzbit_iterator<int32_t>(0x20005)) ;
   CPPUNIT_LOG_IS_TRUE(bitop::nzbit_iterator<int32_t>(0x20005) !=
                       bitop::nzbit_iterator<int32_t>()) ;
   CPPUNIT_LOG_IS_TRUE(bitop::nzbit_iterator<int32_t>(0x20005) ==
                       bitop::make_nzbit_iterator((int32_t)0x20005)) ;

   bitop::nzbit_iterator<int32_t> iter(0x20005) ;
   bitop::nzbit_iterator<int32_t> end ;
   CPPUNIT_LOG_IS_FALSE(iter == end) ;
   CPPUNIT_LOG_EQUAL(*iter, (int32_t)1) ;
   CPPUNIT_LOG_EQUAL(*++iter, (int32_t)4) ;
   CPPUNIT_LOG_IS_FALSE(iter == end) ;
   CPPUNIT_LOG_ASSERT(*iter++ == (int32_t)4) ;
   CPPUNIT_LOG_IS_FALSE(iter == end) ;
   CPPUNIT_LOG_ASSERT(*iter == (int32_t)0x20000) ;
   CPPUNIT_LOG_IS_TRUE(++iter == end) ;
   CPPUNIT_LOG_IS_TRUE(iter == end) ;
}

enum TestEnum : uint8_t {
   TE_0,
   TE_1,
   TE_2,
   TE_3
} ;

void BitOperationsTests::Test_NzbitPosIterator()
{
   CPPUNIT_LOG_IS_TRUE(bitop::nzbitpos_iterator<int32_t>() == bitop::nzbitpos_iterator<int32_t>()) ;
   CPPUNIT_LOG_IS_FALSE(bitop::nzbitpos_iterator<int32_t>() != bitop::nzbitpos_iterator<int32_t>()) ;
   CPPUNIT_LOG_IS_TRUE(bitop::nzbitpos_iterator<int32_t>() == bitop::nzbitpos_iterator<int32_t>(0)) ;
   CPPUNIT_LOG_IS_TRUE(bitop::nzbitpos_iterator<int32_t>(0x20005) ==
                       bitop::nzbitpos_iterator<int32_t>(0x20005)) ;
   CPPUNIT_LOG_IS_FALSE(bitop::nzbitpos_iterator<int32_t>(0x20005) !=
                        bitop::nzbitpos_iterator<int32_t>(0x20005)) ;
   CPPUNIT_LOG_IS_TRUE(bitop::nzbitpos_iterator<int32_t>(0x20005) !=
                       bitop::nzbitpos_iterator<int32_t>()) ;

   bitop::nzbitpos_iterator<int32_t> iter(0x20005) ;
   bitop::nzbitpos_iterator<int32_t> end ;
   CPPUNIT_LOG_IS_FALSE(iter == end) ;
   CPPUNIT_LOG_EQUAL(*iter, 0) ;
   CPPUNIT_LOG_EQUAL(*++iter, 2) ;
   CPPUNIT_LOG_IS_FALSE(iter == end) ;
   CPPUNIT_LOG_ASSERT(*iter++ == 2) ;
   CPPUNIT_LOG_IS_FALSE(iter == end) ;
   CPPUNIT_LOG_ASSERT(*iter == 17) ;
   CPPUNIT_LOG_IS_TRUE(++iter == end) ;
   CPPUNIT_LOG_IS_TRUE(iter == end) ;

   bitop::nzbitpos_iterator<int64_t> iter64((int64_t)0x8000000000000000ll) ;
   bitop::nzbitpos_iterator<int64_t> end64 ;
   CPPUNIT_LOG_IS_FALSE(iter64 == end64) ;
   CPPUNIT_LOG_EQUAL(*iter64, 63) ;
   CPPUNIT_LOG_EQUAL(*iter64++, 63) ;
   CPPUNIT_LOG_IS_TRUE(iter64 == end64) ;

   typedef bitop::nzbitpos_iterator<unsigned, TestEnum> te_iter ;
   te_iter iter_te((1 << TE_1) | (1 << TE_3)) ;
   CPPUNIT_LOG_EQUAL(std::vector<TestEnum>(iter_te, te_iter()), (std::vector<TestEnum>{TE_1, TE_3})) ;
}

void BitOperationsTests::Test_BitRangeBoundary()
{
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint8_t)0, 0), 8) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int8_t)0, 0), 8) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint16_t)0, 0), 16) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int16_t)0, 0), 16) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint32_t)0, 0), 32) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int32_t)0, 0), 32) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint64_t)0, 0), 64) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int64_t)0, 0), 64) ;

   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint8_t)~0ULL, 0), 8) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int8_t)~0ULL, 0), 8) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint16_t)~0ULL, 0), 16) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int16_t)~0ULL, 0), 16) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint32_t)~0ULL, 0), 32) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int32_t)~0ULL, 0), 32) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint64_t)~0ULL, 0), 64) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int64_t)~0ULL, 0), 64) ;

   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint8_t)1, 0), 1) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int8_t)1, 0), 1) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint16_t)1, 0), 1) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int16_t)1, 0), 1) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint32_t)1, 0), 1) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int32_t)1, 0), 1) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint64_t)1, 0), 1) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int64_t)1, 0), 1) ;

   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint8_t)0b10, 0), 1) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int8_t)0b10, 0), 1) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint16_t)0b10, 0), 1) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int16_t)0b10, 0), 1) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint32_t)0b10, 0), 1) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int32_t)0b10, 0), 1) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint64_t)0b10, 0), 1) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int64_t)0b10, 0), 1) ;

   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint8_t)0b10, 1), 2) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int8_t)0b10, 1), 2) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint16_t)0b10, 1), 2) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int16_t)0b10, 1), 2) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint32_t)0b10, 1), 2) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int32_t)0b10, 1), 2) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint64_t)0b10, 1), 2) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int64_t)0b10, 1), 2) ;

   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint8_t)0b11111110, 1), 8) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int8_t)0b11111110, 1), 8) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint16_t)0b11111110, 1), 8) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int16_t)0b11111110, 1), 8) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint32_t)0b11111110, 1), 8) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int32_t)0b11111110, 1), 8) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint64_t)0b11111110, 1), 8) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int64_t)0b11111110, 1), 8) ;

   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint8_t)0b11111110, 7), 8) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int8_t)0b11111110, 7), 8) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint16_t)0b11111110, 7), 8) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int16_t)0b11111110, 7), 8) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint32_t)0b11111110, 7), 8) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int32_t)0b11111110, 7), 8) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint64_t)0b11111110, 7), 8) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int64_t)0b11111110, 7), 8) ;

   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint64_t)0x80'000000'00000000ULL, 63), 64) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int64_t) 0x80'000000'00000000ULL, 63), 64) ;

   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint64_t)0x80'000000'00000000ULL, 62), 63) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int64_t) 0x80'000000'00000000ULL, 62), 63) ;

   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint64_t)0x80'000000'00000000ULL, 61), 63) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int64_t) 0x80'000000'00000000ULL, 61), 63) ;

   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint64_t)0x80'000000'00000000ULL, 0), 63) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int64_t) 0x80'000000'00000000ULL, 0), 63) ;

   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint64_t)0xf0'000000'00000000ULL, 63), 64) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int64_t) 0xf0'000000'00000000ULL, 63), 64) ;

   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint64_t)0xf0'000000'00000000ULL, 62), 64) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int64_t) 0xf0'000000'00000000ULL, 62), 64) ;

   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint64_t)0xf0'000000'00000000ULL, 60), 64) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int64_t) 0xf0'000000'00000000ULL, 60), 64) ;

   CPPUNIT_LOG_EQ(bitop::find_range_boundary((uint64_t)0xf0'000000'00000000ULL, 59), 60) ;
   CPPUNIT_LOG_EQ(bitop::find_range_boundary((int64_t) 0xf0'000000'00000000ULL, 59), 60) ;

   CPPUNIT_LOG(std::endl) ;
}

void BitOperationsTests::Test_OneOf()
{
   CPPUNIT_LOG_IS_TRUE((one_of<1, 4>::is(4))) ;
   CPPUNIT_LOG_IS_FALSE((one_of<1, 4>::is(5))) ;
   CPPUNIT_LOG_IS_FALSE((one_of<1, 4>::is(1000))) ;
   CPPUNIT_LOG_IS_FALSE((one_of<1, 4>::is(std::numeric_limits<long long>::max()))) ;

   CPPUNIT_LOG_ASSERT((one_of<63, 0, 32, 8>::is(0))) ;
   CPPUNIT_LOG_ASSERT((one_of<63, 0, 32, 8>::is(32))) ;
   CPPUNIT_LOG_ASSERT((one_of<63, 0, 32, 8>::is(63))) ;
   CPPUNIT_LOG_IS_FALSE((one_of<63, 0, 32, 8>::is(64))) ;

   CPPUNIT_LOG_ASSERT((one_of<1, 0>::is(0))) ;
   CPPUNIT_LOG_IS_FALSE((one_of<1>::is(0))) ;
}

void BitOperationsTests::Test_Log2()
{
   CPPUNIT_LOG("\n**** Testing compile-time log2 ****\n" << std::endl) ;
   CPPUNIT_LOG_EQUAL(bitop::ct_lnzbpos<0x80>::value, 7) ;
   CPPUNIT_LOG_EQUAL(bitop::ct_lnzbpos<0xff>::value, 7) ;
   CPPUNIT_LOG_EQUAL(bitop::ct_lnzbpos<0x40>::value, 6) ;

   CPPUNIT_LOG_EQUAL(bitop::ct_lnzbpos<0x800>::value, 11) ;
   CPPUNIT_LOG_EQUAL(bitop::ct_lnzbpos<0xfff>::value, 11) ;
   CPPUNIT_LOG_EQUAL(bitop::ct_lnzbpos<0x400>::value, 10) ;

   CPPUNIT_LOG_EQUAL(bitop::ct_lnzbpos<0x8000>::value, 15) ;
   CPPUNIT_LOG_EQUAL(bitop::ct_lnzbpos<0x4000>::value, 14) ;

   CPPUNIT_LOG_EQUAL(bitop::ct_lnzbpos<0x80000000>::value, 31) ;
   CPPUNIT_LOG_EQUAL(bitop::ct_lnzbpos<0x40000000>::value, 30) ;
   CPPUNIT_LOG_EQUAL(bitop::ct_lnzbpos<0x1>::value, 0) ;
   CPPUNIT_LOG_EQUAL(bitop::ct_lnzbpos<0>::value, -1) ;

   CPPUNIT_LOG_EQUAL(bitop::ct_log2ceil<0x80000000>::value, 31) ;
   CPPUNIT_LOG_EQUAL(bitop::ct_log2floor<0x80000000>::value, 31) ;

   CPPUNIT_LOG_EQUAL(bitop::ct_log2ceil<0x40000000>::value, 30) ;
   CPPUNIT_LOG_EQUAL(bitop::ct_log2floor<0x40000000>::value, 30) ;

   CPPUNIT_LOG_EQUAL(bitop::ct_log2ceil<0x40000001>::value, 31) ;
   CPPUNIT_LOG_EQUAL(bitop::ct_log2floor<0x40000001>::value, 30)
      ;
   CPPUNIT_LOG_EQUAL(bitop::ct_log2ceil<0x80000001>::value, 32) ;
   CPPUNIT_LOG_EQUAL(bitop::ct_log2floor<0x80000001>::value, 31) ;

   CPPUNIT_LOG_EQUAL(bitop::ct_log2ceil<0>::value, -1) ;
   CPPUNIT_LOG_EQUAL(bitop::ct_log2floor<0>::value, -1) ;

   CPPUNIT_LOG_EQUAL(bitop::ct_log2ceil<1>::value, 0) ;
   CPPUNIT_LOG_EQUAL(bitop::ct_log2ceil<2>::value, 1) ;
   CPPUNIT_LOG_EQUAL(bitop::ct_log2ceil<3>::value, 2) ;
   CPPUNIT_LOG_EQUAL(bitop::ct_log2ceil<4>::value, 2) ;
   CPPUNIT_LOG_EQUAL(bitop::ct_log2ceil<10>::value, 4) ;

   CPPUNIT_LOG("\n**** Testing run-time log2 ****\n" << std::endl) ;
   CPPUNIT_LOG_EQUAL(bitop::log2floor(0), -1) ;
   CPPUNIT_LOG_EQUAL(bitop::log2ceil(0), -1) ;
   CPPUNIT_LOG_EQUAL(bitop::log2floor(1), 0) ;
   CPPUNIT_LOG_EQUAL(bitop::log2ceil(1), 0) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(bitop::log2floor(0x80000000), 31) ;
   CPPUNIT_LOG_EQUAL(bitop::log2ceil(0x80000000), 31) ;
   CPPUNIT_LOG_EQUAL(bitop::log2floor(0x40000000), 30) ;
   CPPUNIT_LOG_EQUAL(bitop::log2ceil(0x40000000), 30) ;
   CPPUNIT_LOG_EQUAL(bitop::log2floor(0x40000001), 30) ;
   CPPUNIT_LOG_EQUAL(bitop::log2ceil(0x40000001), 31) ;
   CPPUNIT_LOG_EQUAL(bitop::log2floor(0x80000001), 31) ;
   CPPUNIT_LOG_EQUAL(bitop::log2ceil(0x80000001), 32) ;
   CPPUNIT_LOG_EQUAL(bitop::log2ceil(0), -1) ;
   CPPUNIT_LOG_EQUAL(bitop::log2ceil(1), 0) ;
   CPPUNIT_LOG_EQUAL(bitop::log2ceil(2), 1) ;
   CPPUNIT_LOG_EQUAL(bitop::log2ceil(3), 2) ;
   CPPUNIT_LOG_EQUAL(bitop::log2ceil(4), 2) ;
   CPPUNIT_LOG_EQUAL(bitop::log2ceil(10), 4) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(bitop::log2ceil((uint8_t)10), 4) ;
   CPPUNIT_LOG_EQUAL(bitop::log2floor((int16_t)-1), 15) ;
   CPPUNIT_LOG_EQUAL(bitop::log2ceil((int16_t)-1), 16) ;
   CPPUNIT_LOG_EQUAL(bitop::log2floor(0x800000001LLU), 35) ;
   CPPUNIT_LOG_EQUAL(bitop::log2ceil(0x800000001LLU), 36) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(bitop::tstpow2(0x8000)) ;
   CPPUNIT_LOG_ASSERT(bitop::tstpow2(0x1)) ;
   CPPUNIT_LOG_IS_FALSE(bitop::tstpow2(0x6)) ;
   CPPUNIT_LOG_IS_FALSE(bitop::tstpow2(0)) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(bitop::round2z(0U), 0U) ;
   CPPUNIT_LOG_EQUAL(bitop::round2z(1U), 1U) ;
   CPPUNIT_LOG_EQUAL(bitop::round2z(2U), 2U) ;
   CPPUNIT_LOG_EQUAL(bitop::round2z(3U), 4U) ;
   CPPUNIT_LOG_EQUAL(bitop::round2z(4U), 4U) ;
   CPPUNIT_LOG_EQUAL(bitop::round2z(5U), 8U) ;

   CPPUNIT_LOG_EQUAL(bitop::round2z((uint8_t)0), uint8_t(0)) ;
   CPPUNIT_LOG_EQUAL(bitop::round2z((uint8_t)9), uint8_t(16)) ;
}

/*******************************************************************************
 main
*******************************************************************************/
int main(int argc, char *argv[])
{
   return unit::run_tests
       <
          BitOperationsTests
       >
       (argc, argv) ;
}
