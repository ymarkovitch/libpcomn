/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_bitops.cpp
 COPYRIGHT    :   Yakov Markovitch, 2016-2017. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittest for pcomn_integer classes/functions

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   22 Mar 2016
*******************************************************************************/
#include <pcomn_bitops.h>
#include <pcomn_unittest.h>

using namespace pcomn ;

class BitopsTests : public CppUnit::TestFixture {

      template<typename T>
      void Test_Native_Bitcount() ;

      CPPUNIT_TEST_SUITE(BitopsTests) ;

      CPPUNIT_TEST(Test_Native_Bitcount<generic_isa_tag>) ;
      CPPUNIT_TEST(Test_Native_Bitcount<native_isa_tag>) ;

      #ifdef PCOMN_PL_SIMD_AVX2
      CPPUNIT_TEST(Test_Native_Bitcount<avx2_isa_tag>) ;
      #endif
      #ifdef PCOMN_PL_SIMD_AVX
      CPPUNIT_TEST(Test_Native_Bitcount<avx_isa_tag>) ;
      #endif
      #ifdef PCOMN_PL_SIMD_SSE42
      CPPUNIT_TEST(Test_Native_Bitcount<sse42_isa_tag>) ;
      #endif

      CPPUNIT_TEST_SUITE_END() ;
} ;

/*******************************************************************************
 BitopsTests
*******************************************************************************/
template<typename T>
void BitopsTests::Test_Native_Bitcount()
{
   typedef T isa_tag ;

   CPPUNIT_LOG_LINE("**** " << PCOMN_CLASSNAME(isa_tag) << '\n') ;

   CPPUNIT_LOG_EQ(native_bitcount(int8_t(), isa_tag()), 0U) ;
   CPPUNIT_LOG_EQ(native_bitcount(uint8_t(), isa_tag()), 0U) ;
   CPPUNIT_LOG_EQ(native_bitcount(int16_t(), isa_tag()), 0U) ;
   CPPUNIT_LOG_EQ(native_bitcount(uint16_t(), isa_tag()), 0U) ;
   CPPUNIT_LOG_EQ(native_bitcount(int32_t(), isa_tag()), 0U) ;
   CPPUNIT_LOG_EQ(native_bitcount(uint32_t(), isa_tag()), 0U) ;
   CPPUNIT_LOG_EQ(native_bitcount(int64_t(), isa_tag()), 0U) ;
   CPPUNIT_LOG_EQ(native_bitcount(uint64_t(), isa_tag()), 0U) ;

   CPPUNIT_LOG_EQ(native_bitcount((int8_t)-1, isa_tag()), 8U) ;
   CPPUNIT_LOG_EQ(native_bitcount((uint8_t)-1, isa_tag()), 8U) ;
   CPPUNIT_LOG_EQ(native_bitcount((int16_t)-1, isa_tag()), 16U) ;
   CPPUNIT_LOG_EQ(native_bitcount((uint16_t)-1, isa_tag()), 16U) ;
   CPPUNIT_LOG_EQ(native_bitcount((int32_t)-1, isa_tag()), 32U) ;
   CPPUNIT_LOG_EQ(native_bitcount((uint32_t)-1, isa_tag()), 32U) ;
   CPPUNIT_LOG_EQ(native_bitcount((int64_t)-1, isa_tag()), 64U) ;
   CPPUNIT_LOG_EQ(native_bitcount((uint64_t)(-1), isa_tag()), 64U) ;

   CPPUNIT_LOG_EQ(native_bitcount((int8_t)0x41, isa_tag()), 2U) ;
   CPPUNIT_LOG_EQ(native_bitcount((int8_t)-1, isa_tag()), 8U) ;
   CPPUNIT_LOG_EQ(native_bitcount((uint8_t)0x41, isa_tag()), 2U) ;
   CPPUNIT_LOG_EQ(native_bitcount((uint8_t)0x43, isa_tag()), 3U) ;
   CPPUNIT_LOG_EQ(native_bitcount((uint8_t)0x80, isa_tag()), 1U) ;
   CPPUNIT_LOG_EQ(native_bitcount((int32_t)0xF1, isa_tag()), 5U) ;
   CPPUNIT_LOG_EQ(native_bitcount((int64_t)0xF1, isa_tag()), 5U) ;
   CPPUNIT_LOG_EQ(native_bitcount((int32_t)0x10000001, isa_tag()), 2U) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(BitopsTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv, "unittest.diag.ini", "pcommon bit operations tests") ;
}
