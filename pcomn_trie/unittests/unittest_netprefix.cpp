/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   unittest_netprefix.cpp
 COPYRIGHT    :   Yakov Markovitch, 2019

 DESCRIPTION  :

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   23 Oct 2019
*******************************************************************************/
#include <pcomn_netprefix.h>
#include <pcomn_unittest.h>

using namespace pcomn ;

/*******************************************************************************
                            class ShortestNetPrefixSetTests
*******************************************************************************/
class ShortestNetPrefixSetTests : public CppUnit::TestFixture {

      void Test_BitTupleSelect() ;
      void Test_ShortestNetPrefixSet_Build() ;

      CPPUNIT_TEST_SUITE(ShortestNetPrefixSetTests) ;

      CPPUNIT_TEST(Test_BitTupleSelect) ;
      CPPUNIT_TEST(Test_ShortestNetPrefixSet_Build) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

/*******************************************************************************
 ShortestNetPrefixSetTests
*******************************************************************************/
void ShortestNetPrefixSetTests::Test_BitTupleSelect()
{
    constexpr uint32_t v32_1 = 0b111101'010000'111111'000000'011010'10U ;

    CPPUNIT_LOG_EQ(bittuple<6>(v32_1, 0), 0b111101) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v32_1, 1), 0b010000) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v32_1, 2), 0b111111) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v32_1, 3), 0b000000) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v32_1, 4), 0b011010) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v32_1, 5), 0b10'0000) ;

    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG_EQ(bittuple<6>(v32_1, 0, 31), 0) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v32_1, 0, 30), 0b10'0000) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v32_1, 0, 29), 0b010'000) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v32_1, 0, 28), 0b1010'00) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v32_1, 0, 5),  0b1'01000) ;

    CPPUNIT_LOG(std::endl) ;

    constexpr uint64_t v64_1 = 0b111101'010000'111111'000000'011010'101010'010101'001100'110011'101101'0101ULL ;

    CPPUNIT_LOG_EQ(bittuple<6>(v64_1, 0), 0b111101) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v64_1, 1), 0b010000) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v64_1, 2), 0b111111) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v64_1, 3), 0b000000) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v64_1, 4), 0b011010) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v64_1, 5), 0b101010) ;

    CPPUNIT_LOG_EQ(bittuple<6>(v64_1, 6), 0b010101) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v64_1, 7), 0b001100) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v64_1, 8), 0b110011) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v64_1, 9), 0b101101) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v64_1, 10), 0b010100) ;

    CPPUNIT_LOG(std::endl) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v64_1, 10, 3), 0b100000) ;

    CPPUNIT_LOG(std::endl) ;
    CPPUNIT_LOG_EQ(bittuple<6>(-3, 5),    0b010000) ;
    CPPUNIT_LOG_EQ(bittuple<6>(-3, 4, 2), 0b111101) ;

    CPPUNIT_LOG(std::endl) ;
    CPPUNIT_LOG_EQ(bittuple<3>(v32_1, 1),    0b101) ;
    CPPUNIT_LOG_EQ(bittuple<3>(v32_1, 0, 2), 0b110) ;

    CPPUNIT_LOG(std::endl) ;

    constexpr binary128_t v128_1
        (   0b111101'010000'111111'000000'011010'101010'010101'001100'110011'101101'0101ULL,
         0b11'110111'111101'000010'101000'000001'101001'010100'110011'001110'110101'01ULL) ;

    CPPUNIT_LOG_EQ(bittuple<6>(v128_1, 0), 0b111101) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v128_1, 1), 0b010000) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v128_1, 2), 0b111111) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v128_1, 3), 0b000000) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v128_1, 4), 0b011010) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v128_1, 5), 0b101010) ;

    CPPUNIT_LOG_EQ(bittuple<6>(v128_1, 6), 0b010101) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v128_1, 7), 0b001100) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v128_1, 8), 0b110011) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v128_1, 9), 0b101101) ;

    CPPUNIT_LOG_EQ(bittuple<6>(v128_1, 10), 0b010111) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v128_1, 11), 0b110111) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v128_1, 12), 0b111101) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v128_1, 13), 0b000010) ;

    CPPUNIT_LOG_EQ(bittuple<6>(v128_1, 21),    0b010000) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v128_1, 21, 1), 0b100000) ;

    CPPUNIT_LOG_EQ(bittuple<6>(v128_1, 0, 3), 0b101010) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v128_1, 1, 5), 0b011111) ;

    CPPUNIT_LOG_EQ(bittuple<6>(v128_1, 12), 0b111101) ;
    CPPUNIT_LOG_EQ(bittuple<6>(v128_1, 13), 0b000010) ;
}

void ShortestNetPrefixSetTests::Test_ShortestNetPrefixSet_Build()
{
}

int main(int argc, char *argv[])
{
   return pcomn::unit::run_tests
      <
         ShortestNetPrefixSetTests
      >
      (argc, argv) ;
}
