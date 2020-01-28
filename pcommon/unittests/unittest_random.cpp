/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_random.cpp
 COPYRIGHT    :   Yakov Markovitch, 2016-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittest for pseudorandom generators

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   23 Aug 2016
*******************************************************************************/
#include <pcomn_random.h>
#include <pcomn_atomic.h>
#include <pcomn_unittest.h>

class PRNGTests : public CppUnit::TestFixture {

      void Test_SplitMix64() ;
      void Test_Xoroshiro() ;
      void Test_Xoroshiro_Atomic() ;

      CPPUNIT_TEST_SUITE(PRNGTests) ;

      CPPUNIT_TEST(Test_SplitMix64) ;
      CPPUNIT_TEST(Test_Xoroshiro) ;
      CPPUNIT_TEST(Test_Xoroshiro_Atomic) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

using namespace pcomn ;

/*******************************************************************************
 PRNGTests
*******************************************************************************/
void PRNGTests::Test_SplitMix64()
{
   splitmix64 m64_0 ;
   splitmix64 m64_01(0) ;

   CPPUNIT_LOG_ASSERT(is_atomic<splitmix64>::value) ;

   CPPUNIT_LOG_EQ(m64_0(), 0xe220a8397b1dcdaf) ;
   CPPUNIT_LOG_EQ(m64_0(), 0x6e789e6aa1b965f4) ;
   CPPUNIT_LOG_EQ(m64_01(), 0xe220a8397b1dcdaf) ;
   CPPUNIT_LOG_EQ(m64_01(), 0x6e789e6aa1b965f4) ;

   splitmix64 m64_1024(1024) ;

   CPPUNIT_LOG_EQ(m64_1024(), 0x4426acba529f17cc) ;
   CPPUNIT_LOG_EQ(m64_1024(), 0xf2a46c019abe148a) ;
}

void PRNGTests::Test_Xoroshiro()
{
   xoroshiro_prng<uint64_t> x64_0 ;
   xoroshiro_prng<uint64_t> x64_01(0) ;

   struct alignas(16) xp : xoroshiro_prng<uint64_t> {} ;

   CPPUNIT_LOG_ASSERT(is_atomic2<xp>::value) ;

   CPPUNIT_LOG_EQ(x64_0(),  0x509946a41cd733a3) ;
   CPPUNIT_LOG_EQ(x64_0(),  0x885667b1934bfa) ;
   CPPUNIT_LOG_EQ(x64_01(), 0x509946a41cd733a3) ;
   CPPUNIT_LOG_EQ(x64_01(), 0x885667b1934bfa) ;

   xoroshiro_prng<uint64_t> x64_1024(1024) ;

   CPPUNIT_LOG_EQ(x64_1024(), 0x36cb18bbed5d2c56) ;
   CPPUNIT_LOG_EQ(x64_1024(), 0x629e56513e05d889) ;

   CPPUNIT_LOG_RUN(x64_1024 = xoroshiro_prng<uint64_t>{1024}) ;
   CPPUNIT_LOG_RUN(x64_1024.jump()) ;

   CPPUNIT_LOG_EQ(x64_1024(), 0x95ef4131aac51b3) ;
   CPPUNIT_LOG_EQ(x64_1024(), 0xd6da96741416be7c) ;

   CPPUNIT_LOG_RUN(x64_1024 = xoroshiro_prng<uint64_t>{1024}) ;
   CPPUNIT_LOG_RUN(x64_1024.jump().jump()) ;

   CPPUNIT_LOG_EQ(x64_1024(), 0x1ccca0b6e0111680) ;
   CPPUNIT_LOG_EQ(x64_1024(), 0x180d58fd5aef78a) ;
}


void PRNGTests::Test_Xoroshiro_Atomic()
{
   xoroshiro_prng<std::atomic<uint64_t>> x64_0 ;
   xoroshiro_prng<std::atomic<uint64_t>> x64_01(0) ;

   CPPUNIT_LOG_EQ(x64_0(),  0x509946a41cd733a3) ;
   CPPUNIT_LOG_EQ(x64_0(),  0x885667b1934bfa) ;
   CPPUNIT_LOG_EQ(x64_01(), 0x509946a41cd733a3) ;
   CPPUNIT_LOG_EQ(x64_01(), 0x885667b1934bfa) ;

   xoroshiro_prng<std::atomic<uint64_t>> x64_1024 {xoroshiro_prng<uint64_t>(1024)} ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQ(x64_1024(), 0x36cb18bbed5d2c56) ;
   CPPUNIT_LOG_EQ(x64_1024(), 0x629e56513e05d889) ;

   CPPUNIT_LOG_RUN(x64_1024 = xoroshiro_prng<uint64_t>{1024}) ;

   CPPUNIT_LOG_EQ(x64_1024(), 0x36cb18bbed5d2c56) ;
   CPPUNIT_LOG_EQ(x64_1024(), 0x629e56513e05d889) ;

   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_RUN(x64_1024 = xoroshiro_prng<uint64_t>{1024}) ;

   const xoroshiro_prng<uint64_t> &c_other = x64_1024.jump() ;
   xoroshiro_prng<uint64_t> &other = const_cast<xoroshiro_prng<uint64_t> &>(c_other) ;

   CPPUNIT_LOG_EQ(x64_1024(), 0x95ef4131aac51b3) ;
   CPPUNIT_LOG_EQ(x64_1024(), 0xd6da96741416be7c) ;

   CPPUNIT_LOG_EQ(other(), 0x95ef4131aac51b3) ;
   CPPUNIT_LOG_EQ(other(), 0xd6da96741416be7c) ;
}


int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(PRNGTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv, "unittest.diag.ini",
                             "Pseudorandom generators tests") ;
}
