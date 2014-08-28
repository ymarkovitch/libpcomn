/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_integer.cpp
 COPYRIGHT    :   Yakov Markovitch, 2006-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittest for pcomn_integer classes/functions

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   28 Nov 2006
*******************************************************************************/
#include <pcomn_integer.h>
#include <pcomn_primenum.h>
#include <pcomn_unittest.h>

class IntegerTests : public CppUnit::TestFixture {

      void Test_Bitsize() ;
      void Test_SignTraits() ;
      void Test_Bitcount() ;
      void Test_Bitcount_CompileTime() ;
      void Test_Clrrnzb() ;
      void Test_Getrnzb() ;
      void Test_NzbitIterator() ;
      void Test_NzbitPosIterator() ;
      void Test_OneOf() ;
      void Test_Log2() ;

      CPPUNIT_TEST_SUITE(IntegerTests) ;

      CPPUNIT_TEST(Test_Bitsize) ;
      CPPUNIT_TEST(Test_SignTraits) ;
      CPPUNIT_TEST(Test_Bitcount) ;
      CPPUNIT_TEST(Test_Bitcount_CompileTime) ;
      CPPUNIT_TEST(Test_Clrrnzb) ;
      CPPUNIT_TEST(Test_Getrnzb) ;
      CPPUNIT_TEST(Test_NzbitIterator) ;
      CPPUNIT_TEST(Test_NzbitPosIterator) ;
      CPPUNIT_TEST(Test_OneOf) ;
      CPPUNIT_TEST(Test_Log2) ;

      CPPUNIT_TEST_SUITE_END() ;

      template<typename T> bool isSigned(T) { return std::numeric_limits<T>::is_signed ; }
} ;

/*******************************************************************************
 IntegerTests
*******************************************************************************/
void IntegerTests::Test_Bitsize()
{
   CPPUNIT_LOG_EQUAL(pcomn::int_traits<pcomn::int8>::bitsize, 8U) ;
   CPPUNIT_LOG_EQUAL(pcomn::int_traits<pcomn::uint8>::bitsize, 8U) ;
   CPPUNIT_LOG_EQUAL(pcomn::int_traits<pcomn::int16>::bitsize, 16U) ;
   CPPUNIT_LOG_EQUAL(pcomn::int_traits<pcomn::uint16>::bitsize, 16U) ;
   CPPUNIT_LOG_EQUAL(pcomn::int_traits<pcomn::int32>::bitsize, 32U) ;
   CPPUNIT_LOG_EQUAL(pcomn::int_traits<pcomn::uint32>::bitsize, 32U) ;
   CPPUNIT_LOG_EQUAL(pcomn::int_traits<pcomn::int64>::bitsize, 64U) ;
   CPPUNIT_LOG_EQUAL(pcomn::int_traits<pcomn::uint64>::bitsize, 64U) ;
}

void IntegerTests::Test_SignTraits()
{
#define TEST_SIGNED_TRAITS(signed_t, unsigned_t)                        \
   CPPUNIT_LOG_IS_TRUE((std::is_same<signed_t, pcomn::int_traits<signed_t>::stype>::value)) ; \
   CPPUNIT_LOG_IS_TRUE((std::is_same<signed_t, pcomn::int_traits<unsigned_t>::stype>::value)) ; \
   CPPUNIT_LOG_IS_TRUE((std::is_same<unsigned_t, pcomn::int_traits<signed_t>::utype>::value)) ; \
   CPPUNIT_LOG_IS_TRUE((std::is_same<unsigned_t, pcomn::int_traits<unsigned_t>::utype>::value))

   TEST_SIGNED_TRAITS(signed char, unsigned char) ;
   TEST_SIGNED_TRAITS(short, unsigned short) ;
   TEST_SIGNED_TRAITS(int, unsigned) ;
   TEST_SIGNED_TRAITS(long, unsigned long) ;
   TEST_SIGNED_TRAITS(pcomn::int64, pcomn::uint64) ;
   TEST_SIGNED_TRAITS(long long, unsigned long long) ;

   CPPUNIT_LOG_IS_TRUE((std::is_same<signed char, pcomn::int_traits<char>::stype>::value)) ;
   CPPUNIT_LOG_IS_TRUE((std::is_same<unsigned char, pcomn::int_traits<char>::utype>::value)) ;

#undef TEST_SIGNED_TRAITS
}

void IntegerTests::Test_Bitcount()
{
   CPPUNIT_LOG_EQUAL(pcomn::bitop::bitcount(static_cast<pcomn::int8>(0)), 0U) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::bitcount(static_cast<pcomn::uint8>(0)), 0U) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::bitcount(static_cast<pcomn::int16>(0)), 0U) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::bitcount(static_cast<pcomn::uint16>(0)), 0U) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::bitcount(static_cast<pcomn::int32>(0)), 0U) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::bitcount(static_cast<pcomn::uint32>(0)), 0U) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::bitcount(static_cast<pcomn::int64>(0)), 0U) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::bitcount(static_cast<pcomn::uint64>(0)), 0U) ;

   CPPUNIT_LOG_EQUAL(pcomn::bitop::bitcount(static_cast<pcomn::int8>(-1)), 8U) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::bitcount(static_cast<pcomn::uint8>(-1)), 8U) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::bitcount(static_cast<pcomn::int16>(-1)), 16U) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::bitcount(static_cast<pcomn::uint16>(-1)), 16U) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::bitcount(static_cast<pcomn::int32>(-1)), 32U) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::bitcount(static_cast<pcomn::uint32>(-1)), 32U) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::bitcount(static_cast<pcomn::int64>(-1)), 64U) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::bitcount(static_cast<pcomn::uint64>((pcomn::int64)-1)), 64U) ;

   CPPUNIT_LOG_EQUAL(pcomn::bitop::bitcount(static_cast<pcomn::int8>(0x41)), 2U) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::bitcount(static_cast<pcomn::int8>(-1)), 8U) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::bitcount(static_cast<pcomn::uint8>(0x41)), 2U) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::bitcount(static_cast<pcomn::uint8>(0x43)), 3U) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::bitcount(static_cast<pcomn::uint8>(0x80)), 1U) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::bitcount(static_cast<pcomn::int32>(0xF1)), 5U) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::bitcount(static_cast<pcomn::int64>(0xF1)), 5U) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::bitcount(static_cast<pcomn::int32>(0x10000001)), 2U) ;
}

void IntegerTests::Test_Bitcount_CompileTime()
{
   CPPUNIT_LOG_EQUAL(pcomn::bitop::ct_bitcount<0>::value, 0U) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::ct_bitcount<0x55>::value, 4U) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::ct_bitcount<(unsigned)-1>::value, pcomn::int_traits<unsigned>::bitsize) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::ct_bitcount<0x20030055>::value, 7U) ;
}

void IntegerTests::Test_Clrrnzb()
{
   CPPUNIT_LOG_EQUAL(pcomn::bitop::clrrnzb(0xF0), 0xE0) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::clrrnzb((pcomn::uint32)0x80000000), (pcomn::uint32)0) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::clrrnzb(0), 0) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::clrrnzb(1), 0) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::clrrnzb((signed char)3), (signed char)2) ;
}

void IntegerTests::Test_Getrnzb()
{
   CPPUNIT_LOG_EQUAL(pcomn::bitop::getrnzb(0xF0), 0x10) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::getrnzb(1), 1) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::getrnzb(-1), 1) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::getrnzb(6), 2) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::getrnzb((char)0x50), (char)0x10) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::getrnzb(0x5500000000000000ll), 0x100000000000000ll) ;
}

void IntegerTests::Test_NzbitIterator()
{
   using namespace pcomn ;
   CPPUNIT_LOG_IS_TRUE(bitop::nzbit_iterator<int32>() == bitop::nzbit_iterator<int32>()) ;
   CPPUNIT_LOG_IS_FALSE(bitop::nzbit_iterator<int32>() != bitop::nzbit_iterator<int32>()) ;
   CPPUNIT_LOG_IS_TRUE(bitop::nzbit_iterator<int32>(0x20005) ==
                       bitop::nzbit_iterator<int32>(0x20005)) ;
   CPPUNIT_LOG_IS_FALSE(bitop::nzbit_iterator<int32>(0x20005) !=
                        bitop::nzbit_iterator<int32>(0x20005)) ;
   CPPUNIT_LOG_IS_TRUE(bitop::nzbit_iterator<int32>(0x20005) !=
                       bitop::nzbit_iterator<int32>()) ;
   CPPUNIT_LOG_IS_TRUE(bitop::nzbit_iterator<int32>(0x20005) ==
                       bitop::make_nzbit_iterator((int32)0x20005)) ;

   bitop::nzbit_iterator<int32> iter(0x20005) ;
   bitop::nzbit_iterator<int32> end ;
   CPPUNIT_LOG_IS_FALSE(iter == end) ;
   CPPUNIT_LOG_EQUAL(*iter, (int32)1) ;
   CPPUNIT_LOG_EQUAL(*++iter, (int32)4) ;
   CPPUNIT_LOG_IS_FALSE(iter == end) ;
   CPPUNIT_LOG_ASSERT(*iter++ == (int32)4) ;
   CPPUNIT_LOG_IS_FALSE(iter == end) ;
   CPPUNIT_LOG_ASSERT(*iter == (int32)0x20000) ;
   CPPUNIT_LOG_IS_TRUE(++iter == end) ;
   CPPUNIT_LOG_IS_TRUE(iter == end) ;
}

void IntegerTests::Test_NzbitPosIterator()
{
   using namespace pcomn ;
   CPPUNIT_LOG_IS_TRUE(bitop::nzbitpos_iterator<int32>() == bitop::nzbitpos_iterator<int32>()) ;
   CPPUNIT_LOG_IS_FALSE(bitop::nzbitpos_iterator<int32>() != bitop::nzbitpos_iterator<int32>()) ;
   CPPUNIT_LOG_IS_TRUE(bitop::nzbitpos_iterator<int32>() == bitop::nzbitpos_iterator<int32>(0)) ;
   CPPUNIT_LOG_IS_TRUE(bitop::nzbitpos_iterator<int32>(0x20005) ==
                       bitop::nzbitpos_iterator<int32>(0x20005)) ;
   CPPUNIT_LOG_IS_FALSE(bitop::nzbitpos_iterator<int32>(0x20005) !=
                        bitop::nzbitpos_iterator<int32>(0x20005)) ;
   CPPUNIT_LOG_IS_TRUE(bitop::nzbitpos_iterator<int32>(0x20005) !=
                       bitop::nzbitpos_iterator<int32>()) ;

   bitop::nzbitpos_iterator<int32> iter(0x20005) ;
   bitop::nzbitpos_iterator<int32> end ;
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
}

void IntegerTests::Test_OneOf()
{
   CPPUNIT_LOG_IS_TRUE((pcomn::one_of<1, 4>::is(4))) ;
   CPPUNIT_LOG_IS_FALSE((pcomn::one_of<1, 4>::is(5))) ;
   CPPUNIT_LOG_IS_FALSE((pcomn::one_of<1, 4>::is(1000))) ;
   CPPUNIT_LOG_IS_TRUE((pcomn::one_of<1, 0>::is(0))) ;
   CPPUNIT_LOG_IS_FALSE((pcomn::one_of<1>::is(0))) ;
}

void IntegerTests::Test_Log2()
{
   CPPUNIT_LOG("\n**** Testing compile-time log2 ****\n" << std::endl) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::ct_lnzbpos<0x80000000>::value, 31) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::ct_lnzbpos<0x40000000>::value, 30) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::ct_lnzbpos<0x1>::value, 0) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::ct_lnzbpos<0>::value, -1) ;

   CPPUNIT_LOG_EQUAL(pcomn::bitop::ct_log2ceil<0x80000000>::value, 31) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::ct_log2ceil<0x40000000>::value, 30) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::ct_log2ceil<0x40000001>::value, 31) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::ct_log2ceil<0x80000001>::value, 32) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::ct_log2ceil<0>::value, -1) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::ct_log2ceil<1>::value, 0) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::ct_log2ceil<2>::value, 1) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::ct_log2ceil<3>::value, 2) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::ct_log2ceil<4>::value, 2) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::ct_log2ceil<10>::value, 4) ;

   CPPUNIT_LOG("\n**** Testing run-time log2 ****\n" << std::endl) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::log2floor(0), -1) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::log2ceil(0), -1) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::log2floor(1), 0) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::log2ceil(1), 0) ;

   CPPUNIT_LOG_EQUAL(pcomn::bitop::log2floor(0x80000000), 31) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::log2ceil(0x80000000), 31) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::log2floor(0x40000000), 30) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::log2ceil(0x40000000), 30) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::log2floor(0x40000001), 30) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::log2ceil(0x40000001), 31) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::log2floor(0x80000001), 31) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::log2ceil(0x80000001), 32) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::log2ceil(0), -1) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::log2ceil(1), 0) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::log2ceil(2), 1) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::log2ceil(3), 2) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::log2ceil(4), 2) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::log2ceil(10), 4) ;

   CPPUNIT_LOG_EQUAL(pcomn::bitop::log2ceil((uint8_t)10), 4) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::log2floor((int16_t)-1), 15) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::log2ceil((int16_t)-1), 16) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::log2floor(0x800000001LLU), 35) ;
   CPPUNIT_LOG_EQUAL(pcomn::bitop::log2ceil(0x800000001LLU), 36) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(IntegerTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv, "unittest.diag.ini", "pcomn_integer tests") ;
}
