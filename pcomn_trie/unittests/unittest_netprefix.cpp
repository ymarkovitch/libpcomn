/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   unittest_netprefix.cpp
 COPYRIGHT    :   Yakov Markovitch, 2019-2020

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
    void Test_ShortestNetPrefixSet_IPv4_Build() ;
    void Test_ShortestNetPrefixSet_IPv6_Build() ;
    void Test_ShortestNetPrefixSet_IPv4_MemberTest() ;
    void Test_ShortestNetPrefixSet_IPv6_MemberTest() ;

    CPPUNIT_TEST_SUITE(ShortestNetPrefixSetTests) ;

    CPPUNIT_TEST(Test_BitTupleSelect) ;
    CPPUNIT_TEST(Test_ShortestNetPrefixSet_IPv4_Build) ;
    CPPUNIT_TEST(Test_ShortestNetPrefixSet_IPv6_Build) ;
    CPPUNIT_TEST(Test_ShortestNetPrefixSet_IPv4_MemberTest) ;
    CPPUNIT_TEST(Test_ShortestNetPrefixSet_IPv6_MemberTest) ;

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
    CPPUNIT_LOG_EQ(bittuple<6>(ipv4_addr(v32_1), 0, 28), 0b1010'00) ;
    CPPUNIT_LOG_EQ(bittuple<6>(ipv4_addr(v32_1), 0, 5),  0b1'01000) ;

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

    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG_EQ(bittuple<6>(ipv6_addr(v128_1), 0, 3), 0b101010) ;
    CPPUNIT_LOG_EQ(bittuple<6>(ipv6_addr(v128_1), 1, 5), 0b011111) ;

    CPPUNIT_LOG_EQ(bittuple<6>(ipv6_addr(v128_1), 12), 0b111101) ;
    CPPUNIT_LOG_EQ(bittuple<6>(ipv6_addr(v128_1), 13), 0b000010) ;
}

void ShortestNetPrefixSetTests::Test_ShortestNetPrefixSet_IPv4_Build()
{
    typedef ipaddr_prefix_set<ipv4_addr> netprefix_set ;

    netprefix_set empty_set ;
    CPPUNIT_LOG_EQ(empty_set.depth(), 0) ;
    CPPUNIT_LOG_EQ(empty_set.nodes_count(), 0) ;

    netprefix_set any_set ({{ipv4_addr::localhost(), 0}}) ;
    CPPUNIT_LOG_EQ(any_set.depth(), 1) ;
    CPPUNIT_LOG_EQ(any_set.nodes_count(), 1) ;

    netprefix_set one_set ({{"8.0.0.1/6"}}) ;
    CPPUNIT_LOG_EQ(one_set.depth(), 1) ;
    CPPUNIT_LOG_EQ(one_set.nodes_count(), 1) ;

    netprefix_set two_set ({{"128.0.0.1/4"}, {"8.0.0.1/6"}}) ;
    CPPUNIT_LOG_EQ(two_set.depth(), 1) ;
    CPPUNIT_LOG_EQ(two_set.nodes_count(), 1) ;

    netprefix_set three_set ({{"128.0.0.1/4"}, {"12.0.0.1/6"}, {"160.0.0.1/5"}}) ;
    CPPUNIT_LOG_EQ(three_set.depth(), 1) ;
    CPPUNIT_LOG_EQ(three_set.nodes_count(), 1) ;

    netprefix_set localhost_set ({{ipv4_addr::localhost(), 24}}) ;
    CPPUNIT_LOG_EQ(localhost_set.depth(), 4) ;
    CPPUNIT_LOG_EQ(localhost_set.nodes_count(), 4) ;

    netprefix_set one_child_set ({{"128.0.0.1/4"}, {"10.0.0.1/8"}}) ;
    CPPUNIT_LOG_EQ(one_child_set.depth(), 2) ;
    CPPUNIT_LOG_EQ(one_child_set.nodes_count(), 2) ;

    netprefix_set private_set({{"127.0.0.1/24"},
                               {"10.0.0.1/8"},
                               {"172.16.0.1/12"},
                               {"192.168.0.0/16"}}) ;

    CPPUNIT_LOG_EQ(private_set.depth(), 4) ;
    CPPUNIT_LOG_EQ(private_set.nodes_count(), 8) ;
}

void ShortestNetPrefixSetTests::Test_ShortestNetPrefixSet_IPv6_Build()
{
    typedef ipaddr_prefix_set<ipv6_addr> netprefix_set ;

    netprefix_set empty_set ;
    CPPUNIT_LOG_EQ(empty_set.depth(), 0) ;
    CPPUNIT_LOG_EQ(empty_set.nodes_count(), 0) ;

    netprefix_set any_set ({{ipv6_addr::localhost(), 0}}) ;
    CPPUNIT_LOG_EQ(any_set.depth(), 1) ;
    CPPUNIT_LOG_EQ(any_set.nodes_count(), 1) ;
}

void ShortestNetPrefixSetTests::Test_ShortestNetPrefixSet_IPv4_MemberTest()
{
    typedef ipaddr_prefix_set<ipv4_addr> netprefix_set ;

    netprefix_set empty_set ;
    netprefix_set any_set ({{ipv4_addr(1, 0, 0, 0), 0}}) ;
    netprefix_set one_set ({{"8.0.0.1/6"}}) ;

    CPPUNIT_LOG_IS_FALSE(empty_set.is_member({127, 0, 0, 1})) ;
    CPPUNIT_LOG_ASSERT(any_set.is_member({127, 0, 0, 1})) ;

    CPPUNIT_LOG_IS_FALSE(one_set.is_member({127, 0, 0, 1})) ;
    CPPUNIT_LOG_ASSERT(one_set.is_member({8, 0, 0, 1})) ;
    CPPUNIT_LOG_ASSERT(one_set.is_member({8, 0, 155, 1})) ;
    CPPUNIT_LOG_ASSERT(one_set.is_member({9, 0, 155, 1})) ;
    CPPUNIT_LOG_IS_FALSE(one_set.is_member({12, 0, 0, 1})) ;

    CPPUNIT_LOG(std::endl) ;

    netprefix_set private_set({{"127.0.0.1/24"},
                               {"10.0.0.1/8"},
                               {"172.16.0.1/12"},
                               {"192.168.0.0/16"}}) ;

    CPPUNIT_LOG_ASSERT(private_set.is_member({127, 0, 0, 255})) ;
    CPPUNIT_LOG_IS_FALSE(private_set.is_member({127, 1, 0, 255})) ;

    CPPUNIT_LOG_ASSERT(private_set.is_member({172, 20, 0, 255})) ;
    CPPUNIT_LOG_IS_FALSE(private_set.is_member({172, 15, 0, 1})) ;

    CPPUNIT_LOG_IS_FALSE(private_set.is_member({8, 8, 8, 8})) ;
}

void ShortestNetPrefixSetTests::Test_ShortestNetPrefixSet_IPv6_MemberTest()
{
    typedef ipaddr_prefix_set<ipv6_addr> netprefix_set ;
    netprefix_set empty_set ;
    netprefix_set any_set ({{ipv6_addr(1, 0, 0, 0, 0, 0, 0, 0), 0}}) ;
    netprefix_set one_set ({{"8::1/6"}}) ;

    CPPUNIT_LOG_IS_FALSE(empty_set.is_member(ipv6_addr::localhost())) ;
    CPPUNIT_LOG_ASSERT(any_set.is_member(ipv6_addr::localhost())) ;

    CPPUNIT_LOG_IS_FALSE(one_set.is_member(ipv6_addr::localhost())) ;
    CPPUNIT_LOG_ASSERT(one_set.is_member({0x800, 0, 0, 0, 0, 0, 0, 1})) ;
    CPPUNIT_LOG_ASSERT(one_set.is_member({0x800, 0, 0xfeed, 0, 0, 0, 0, 1})) ;
    CPPUNIT_LOG_ASSERT(one_set.is_member({0x900, 0, 0xfeed, 0, 0, 0, 0, 1})) ;
    CPPUNIT_LOG_IS_FALSE(one_set.is_member({0xc00, 0, 0, 0, 0, 0, 0, 0xf00d})) ;

    CPPUNIT_LOG(std::endl) ;

    /*
    netprefix_set private_set({{"127.0.0.1/24"},
                               {"10.0.0.1/8"},
                               {"172.16.0.1/12"},
                               {"192.168.0.0/16"}}) ;

    CPPUNIT_LOG_ASSERT(private_set.is_member({127, 0, 0, 255})) ;
    CPPUNIT_LOG_IS_FALSE(private_set.is_member({127, 1, 0, 255})) ;

    CPPUNIT_LOG_ASSERT(private_set.is_member({172, 20, 0, 255})) ;
    CPPUNIT_LOG_IS_FALSE(private_set.is_member({172, 15, 0, 1})) ;

    CPPUNIT_LOG_IS_FALSE(private_set.is_member({8, 8, 8, 8})) ;
    */
}

int main(int argc, char *argv[])
{
   return pcomn::unit::run_tests
      <
         ShortestNetPrefixSetTests
      >
      (argc, argv) ;
}
