/*-*- tab-width:4;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
/*******************************************************************************
 FILE         :   unittest_network_address.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Internet address classes unit tests.

 CREATION DATE:   27 Jan 2008
*******************************************************************************/
#include <pcomn_netaddr.h>
#include <pcomn_unittest.h>
#include <pcomn_string.h>

using namespace pcomn ;

/*******************************************************************************
                            class IPAddressTests
*******************************************************************************/
class IPAddressTests : public CppUnit::TestFixture {
private:
    void Test_IPv4_Address() ;
    void Test_IPv4_Subnet_Address() ;
    void Test_IPv6_Address() ;
    void Test_IPv6_Address_Parser() ;
    void Test_IPv6_Subnet_Address() ;
    void Test_Subnet_Match() ;

    CPPUNIT_TEST_SUITE(IPAddressTests) ;

    CPPUNIT_TEST(Test_IPv4_Address) ;
    CPPUNIT_TEST(Test_IPv4_Subnet_Address) ;

    CPPUNIT_TEST(Test_IPv6_Address) ;
    CPPUNIT_TEST(Test_IPv6_Address_Parser) ;
    CPPUNIT_TEST(Test_IPv6_Subnet_Address) ;

    CPPUNIT_TEST(Test_Subnet_Match) ;

    CPPUNIT_TEST_SUITE_END() ;
} ;

static constexpr union {
    uint16_t n ;
    char s[2] ;
    bool is_little() const { return s[0] == 2 ; }
} endianness = { 0x0102 } ;

void IPAddressTests::Test_IPv4_Address()
{
    CPPUNIT_LOG_EQUAL(ipv4_addr(), ipv4_addr()) ;
    CPPUNIT_LOG_IS_TRUE(ipv4_addr() == ipv4_addr()) ;
    CPPUNIT_LOG_IS_FALSE(ipv4_addr() != ipv4_addr()) ;
    CPPUNIT_LOG_IS_FALSE(ipv4_addr() < ipv4_addr()) ;
    CPPUNIT_LOG_EQ(ipv4_addr().ipaddr(), 0) ;
    CPPUNIT_LOG_EQ(ipv4_addr().inaddr().s_addr, 0) ;
    CPPUNIT_LOG_EQUAL(ipv4_addr().str(), std::string("0.0.0.0")) ;

    CPPUNIT_LOG_EQUAL(ipv4_addr("", ipv4_addr::ALLOW_EMPTY).ipaddr(), (uint32_t)0) ;
    CPPUNIT_LOG_EXCEPTION_MSG(ipv4_addr(""), invalid_str_repr, "mpty") ;
    CPPUNIT_LOG_EXCEPTION_MSG(ipv4_addr("  65.66.67.68  ", ipv4_addr::ONLY_DOTDEC).ipaddr(), invalid_str_repr, "decimal") ;
    CPPUNIT_LOG_EXCEPTION_MSG(ipv4_addr("a5.66.67.68", ipv4_addr::ONLY_DOTDEC).ipaddr(), invalid_str_repr, "decimal") ;
    CPPUNIT_LOG_EXCEPTION_MSG(ipv4_addr("abc", ipv4_addr::ONLY_DOTDEC).ipaddr(), invalid_str_repr, "decimal") ;
    CPPUNIT_LOG_EXCEPTION_MSG(ipv4_addr("65..66.67", ipv4_addr::ONLY_DOTDEC).ipaddr(), invalid_str_repr, "decimal") ;
    CPPUNIT_LOG_EXCEPTION_MSG(ipv4_addr("1.1.1.555", ipv4_addr::ONLY_DOTDEC), invalid_str_repr, "decimal") ;
    CPPUNIT_LOG_EXCEPTION_MSG(ipv4_addr("1.1.555", ipv4_addr::ONLY_DOTDEC), invalid_str_repr, "decimal") ;
    CPPUNIT_LOG_EXCEPTION_MSG(ipv4_addr("1.555", ipv4_addr::ONLY_DOTDEC), invalid_str_repr, "decimal") ;
    CPPUNIT_LOG_EXCEPTION_MSG(ipv4_addr("555", ipv4_addr::ONLY_DOTDEC), invalid_str_repr, "decimal") ;
    CPPUNIT_LOG_EXCEPTION_MSG(ipv4_addr("-0.1.2.3", ipv4_addr::ONLY_DOTDEC), invalid_str_repr, "decimal") ;
    CPPUNIT_LOG_EXCEPTION_MSG(ipv4_addr("127.0.0.", ipv4_addr::ONLY_DOTDEC), invalid_str_repr, "decimal") ;
    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG_EQUAL(ipv4_addr(65, 66, 67, 68).ipaddr(), (uint32_t)0x41424344) ;
    CPPUNIT_LOG_EQUAL((uint32_t)ipv4_addr(65, 66, 67, 68).ipaddr(), (uint32_t)0x41424344) ;
    CPPUNIT_LOG_EQUAL(ipv4_addr((uint32_t)ipv4_addr(65, 66, 67, 68)), ipv4_addr(65, 66, 67, 68)) ;
    CPPUNIT_LOG_EQ(ipv4_addr(65, 66, 67, 68).octet(0), 65) ;
    CPPUNIT_LOG_EQ(ipv4_addr(65, 66, 67, 68).octet(1), 66) ;
    CPPUNIT_LOG_EQ(ipv4_addr(65, 66, 67, 68).octet(2), 67) ;
    CPPUNIT_LOG_EQ(ipv4_addr(65, 66, 67, 68).octet(3), 68) ;
    CPPUNIT_LOG_EQUAL(ipv4_addr(65, 66, 67, 68).str(), std::string("65.66.67.68")) ;
    CPPUNIT_LOG_EQUAL(ipv4_addr("65.66.67.68").ipaddr(), (uint32_t)0x41424344) ;
    CPPUNIT_LOG_EQ(((in_addr)ipv4_addr(65, 66, 67, 68)).s_addr, (uint32_t)(endianness.is_little() ? 0x44434241 : 0x41424344)) ;
    CPPUNIT_LOG_EQUAL(ipv4_addr(0x41424344).ipaddr(), (uint32_t)0x41424344) ;
    in_addr InAddr ;
    CPPUNIT_LOG_RUN(InAddr.s_addr = htonl(0x41424344)) ;
    CPPUNIT_LOG_EQUAL(ipv4_addr(InAddr).ipaddr(), (uint32_t)0x41424344) ;
    CPPUNIT_LOG_EQUAL(ipv4_addr(InAddr).str(), std::string("65.66.67.68")) ;

    CPPUNIT_LOG_EQUAL(ipv4_addr(1, 2, 3, 4).str(), std::string("1.2.3.4")) ;
    CPPUNIT_LOG_EQUAL(ipv4_addr(1, 2, 3, 4).octet(0), (uint8_t)1) ;
    CPPUNIT_LOG_EQUAL(ipv4_addr(1, 2, 3, 4).octet(2), (uint8_t)3) ;
    CPPUNIT_LOG_EQUAL(ipv4_addr("127.0.0.2"), ipv4_addr(127, 0, 0, 2)) ;
    CPPUNIT_LOG_EQUAL(ipv4_addr("127.0.0.2").octets()[0], (uint8_t)127) ;
    CPPUNIT_LOG_EQUAL(ipv4_addr("127.0.0.2").octets()[3], (uint8_t)2) ;
    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG_EQUAL(ipv4_addr::localhost(), ipv4_addr(127, 0, 0, 1)) ;
    CPPUNIT_LOG(std::endl) ;

    // Use root nameserver address for testing: I _dearly_ hope it will not change!
    CPPUNIT_LOG_EQUAL(ipv4_addr("j.root-servers.net", ipv4_addr::USE_HOSTNAME),
                      ipv4_addr(192, 58, 128, 30)) ;
    CPPUNIT_LOG_EQUAL(ipv4_addr(192, 58, 128, 30).hostname(),
                      std::string("j.root-servers.net")) ;

    CPPUNIT_LOG_EQUAL(ipv4_addr("localhost", ipv4_addr::USE_HOSTNAME).str(), std::string("127.0.0.1")) ;
    CPPUNIT_LOG_EQUAL(inaddr_loopback(), ipv4_addr("localhost", ipv4_addr::USE_HOSTNAME)) ;
    CPPUNIT_LOG_EQUAL(inaddr_broadcast(), ipv4_addr(255, 255, 255, 255)) ;
    CPPUNIT_LOG_EXCEPTION(ipv4_addr("Hello, world!", ipv4_addr::USE_HOSTNAME), system_error) ;
    //CPPUNIT_LOG_EQUAL(ipv4_addr(1, 2, 3, 4).hostname(), std::string("1.2.3.4")) ;
    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG(std::endl) ;

    std::errc errcode ;

    CPPUNIT_LOG_EQUAL(ipv4_addr("", errcode).ipaddr(), (uint32_t)0) ;
    CPPUNIT_LOG_EQUAL(errcode, std::errc::invalid_argument) ;

    CPPUNIT_LOG_EQUAL(ipv4_addr("127.0.0.2", errcode), ipv4_addr(127, 0, 0, 2)) ;
    CPPUNIT_LOG_EQUAL(errcode, std::errc()) ;

    CPPUNIT_LOG_EQUAL(ipv4_addr("65..66.67", errcode, ipv4_addr::ONLY_DOTDEC), ipv4_addr()) ;
    CPPUNIT_LOG_EQUAL(errcode, std::errc::invalid_argument) ;

    CPPUNIT_LOG_EQUAL(ipv4_addr("Hello, world!", errcode={}, ipv4_addr::USE_HOSTNAME), ipv4_addr()) ;
    CPPUNIT_LOG_EQUAL(errcode, std::errc::invalid_argument) ;

    CPPUNIT_LOG_EQUAL(ipv4_addr("localhost", errcode, ipv4_addr::USE_HOSTNAME), ipv4_addr(127, 0, 0, 1)) ;
    CPPUNIT_LOG_EQUAL(errcode, std::errc()) ;
}

void IPAddressTests::Test_IPv4_Subnet_Address()
{
    CPPUNIT_LOG_EQUAL(ipv4_subnet(), ipv4_subnet()) ;
    CPPUNIT_LOG_IS_TRUE(ipv4_subnet() == ipv4_subnet()) ;
    CPPUNIT_LOG_IS_FALSE(ipv4_subnet() != ipv4_subnet()) ;

    CPPUNIT_LOG_EQ(ipv4_subnet(65, 66, 67, 68, 32).str(), "65.66.67.68/32") ;
    CPPUNIT_LOG_EQ(ipv4_subnet(65, 66, 67, 68, 24).str(), "65.66.67.68/24") ;
    CPPUNIT_LOG_EQ(ipv4_subnet(65, 66, 67, 68, 24).str(), "65.66.67.68/24") ;
    CPPUNIT_LOG_EQ(ipv4_subnet(65, 66, 67, 68, 24).subnet().str(), "65.66.67.0/24") ;
    CPPUNIT_LOG_ASSERT(ipv4_subnet(65, 66, 67, 0, 24) < ipv4_subnet(65, 66, 68, 0, 24)) ;
    CPPUNIT_LOG_ASSERT(ipv4_subnet(65, 66, 67, 0, 24) < ipv4_subnet(65, 66, 67, 0, 25)) ;

    CPPUNIT_LOG_EQUAL(ipv4_subnet(65, 66, 67, 68, 24).addr(), ipv4_addr(65, 66, 67, 68)) ;
    CPPUNIT_LOG_EQUAL(ipv4_subnet(65, 66, 67, 68, 24).subnet().addr(), ipv4_addr(65, 66, 67, 0)) ;
    CPPUNIT_LOG_EQ(ipv4_subnet(65, 66, 67, 68, 24).pfxlen(), 24) ;
    CPPUNIT_LOG_EQ(ipv4_subnet(65, 66, 67, 68, 24).netmask(), 0xffffff00) ;

    CPPUNIT_LOG(std::endl) ;
    CPPUNIT_LOG_EQ(ipv4_subnet(65, 66, 67, 68, 24).pfxlen(), 24) ;
    CPPUNIT_LOG_EQ(ipv4_subnet(65, 66, 67, 68, 0).pfxlen(), 0) ;
    CPPUNIT_LOG_EQ(ipv4_subnet(65, 66, 67, 68, 1).pfxlen(), 1) ;
    CPPUNIT_LOG_EQ(ipv4_subnet(65, 66, 67, 68, 31).pfxlen(), 31) ;
    CPPUNIT_LOG_EQ(ipv4_subnet(65, 66, 67, 68, 32).pfxlen(), 32) ;
    CPPUNIT_LOG_EXCEPTION(ipv4_subnet(65, 66, 67, 68, 33), std::invalid_argument) ;

    CPPUNIT_LOG_EQ(ipv4_subnet(65, 66, 67, 68, 24).netmask(), 0xffffff00) ;
    CPPUNIT_LOG_EQ(ipv4_subnet(65, 66, 67, 68, 16).netmask(), 0xffff0000) ;
    CPPUNIT_LOG_EQ(ipv4_subnet(65, 66, 67, 68, 1).netmask(), 0x80000000) ;
    CPPUNIT_LOG_EQ(ipv4_subnet(65, 66, 67, 68, 31).netmask(), 0xfffffffe) ;
    CPPUNIT_LOG_EQ(ipv4_subnet(65, 66, 67, 68, 32).netmask(), 0xffffffff) ;
    CPPUNIT_LOG_EQ(ipv4_subnet(65, 66, 67, 68, 0).netmask(), 0) ;

    CPPUNIT_LOG(std::endl) ;
    CPPUNIT_LOG_EQ(ipv4_subnet(65, 66, 67, 68, 24).addr_range(),
                   std::make_pair(ipv4_addr(65, 66, 67, 0), ipv4_addr(65, 66, 67, 255))) ;
    CPPUNIT_LOG_EQ(ipv4_subnet(65, 66, 67, 3, 31).addr_range(),
                   std::make_pair(ipv4_addr(65, 66, 67, 2), ipv4_addr(65, 66, 67, 3))) ;
    CPPUNIT_LOG_EQ(ipv4_subnet(65, 66, 67, 3, 32).addr_range(),
                   std::make_pair(ipv4_addr(65, 66, 67, 3), ipv4_addr(65, 66, 67, 3))) ;
    CPPUNIT_LOG_EQ(ipv4_subnet(65, 66, 67, 3, 0).addr_range(),
                   std::make_pair(ipv4_addr(0, 0, 0, 0), ipv4_addr(255, 255, 255, 255))) ;

    CPPUNIT_LOG(std::endl) ;
    CPPUNIT_LOG_EQUAL(ipv4_subnet("10.0.61.5/24"), ipv4_subnet(10, 0, 61, 5, 24)) ;
    CPPUNIT_LOG_EQUAL(ipv4_subnet("10.0.61.5/24").addr(), ipv4_addr(10, 0, 61, 5)) ;
    CPPUNIT_LOG_EQ(ipv4_subnet("10.0.61.5/24").pfxlen(), 24) ;

    CPPUNIT_LOG_EQUAL(ipv4_subnet("0.0.0.0/0"), ipv4_subnet()) ;
    CPPUNIT_LOG_NOT_EQUAL(ipv4_subnet("0.0.0.0/1"), ipv4_subnet()) ;

    CPPUNIT_LOG(std::endl) ;
    CPPUNIT_LOG_EXCEPTION(ipv4_subnet("10.0.61.5/-1"), invalid_str_repr) ;
    CPPUNIT_LOG_EXCEPTION(ipv4_subnet("10.0.61.5"), invalid_str_repr) ;
    CPPUNIT_LOG_EXCEPTION_MSG(ipv4_subnet("10.0.61.5/0x1"), invalid_str_repr, "network prefix specification") ;
}

void IPAddressTests::Test_IPv6_Address()
{
    CPPUNIT_LOG_EQUAL(ipv6_addr(), ipv6_addr()) ;
    CPPUNIT_LOG_IS_TRUE(ipv6_addr() == ipv6_addr()) ;
    CPPUNIT_LOG_IS_FALSE(ipv6_addr() != ipv6_addr()) ;
    CPPUNIT_LOG_IS_FALSE(ipv6_addr() < ipv6_addr()) ;
    CPPUNIT_LOG_EQ(ipv6_addr().str(), "::") ;
    CPPUNIT_LOG_EXCEPTION_MSG(ipv6_addr(""), invalid_str_repr, "mpty") ;
    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG_EQUAL(ipv6_addr("", ipv6_addr::ALLOW_EMPTY), ipv6_addr()) ;
    CPPUNIT_LOG_EQUAL((binary128_t)ipv6_addr("", ipv6_addr::ALLOW_EMPTY), binary128_t()) ;
    CPPUNIT_LOG_EQUAL((binary128_t)ipv6_addr(), binary128_t()) ;
    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG_EQUAL(ipv6_addr::localhost(), ipv6_addr(0, 0, 0, 0, 0, 0, 0, 1)) ;
    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG_EQ(ipv6_addr(0x2001, 0x0DB8, 0xAC10, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00D).str(),
                   "2001:db8:ac10:fe01:feed:babe:cafe:f00d") ;

    CPPUNIT_LOG_EQ(ipv6_addr(0, 0x0DB8, 0xAC10, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00D).str(),
                   "0:db8:ac10:fe01:feed:babe:cafe:f00d") ;

    CPPUNIT_LOG_EQ(ipv6_addr(0, 0, 0xAC10, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00D).str(),
                   "::ac10:fe01:feed:babe:cafe:f00d") ;

    CPPUNIT_LOG_EQ(ipv6_addr(0, 0, 0xAC10, 0xFE01, 0, 0, 0, 0).str(), "0:0:ac10:fe01::") ;

    CPPUNIT_LOG_EQ(ipv6_addr(1, 0, 0xAC10, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00D).str(),
                   "1:0:ac10:fe01:feed:babe:cafe:f00d") ;

    CPPUNIT_LOG_EQ(ipv6_addr(1, 0, 0, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00D).str(),
                   "1::fe01:feed:babe:cafe:f00d") ;

    CPPUNIT_LOG_EQ(ipv6_addr(1, 0, 0, 0xFE01, 0, 0, 0xCAFE, 0xF00D).str(),
                   "1::fe01:0:0:cafe:f00d") ;

    CPPUNIT_LOG_EQ(ipv6_addr(1, 0, 0, 0xFE01, 0, 0, 0, 0xF00D).str(),
                   "1:0:0:fe01::f00d") ;

    CPPUNIT_LOG_EQ(ipv6_addr(1, 0, 0, 0xFE01, 0, 0, 0, 0).str(),
                   "1:0:0:fe01::") ;

    CPPUNIT_LOG_EQ(ipv6_addr(0, 0, 0, 0, 0, 0, 0, 0xF00D).str(),
                   "::f00d") ;
    CPPUNIT_LOG_EQ(ipv6_addr(1, 0, 0, 0, 0, 0, 0, 0).str(),
                   "1::") ;

    CPPUNIT_LOG_EQ(ipv6_addr(0, 0, 0, 0, 0, 0, 0, 1).str(),
                   "::1") ;

    CPPUNIT_LOG_EQ(ipv6_addr().str(), "::") ;

    CPPUNIT_LOG(std::endl) ;

    // IPv6-mapped IPv4
    CPPUNIT_LOG_EQUAL((binary128_t)ipv6_addr(ipv4_addr(127, 0, 0, 1)), binary128_t(0, 0, 0, 0, 0, 0xffff, 0x7f00, 0x0001)) ;
    CPPUNIT_LOG_ASSERT(ipv6_addr(ipv4_addr(127, 0, 0, 1)).is_ipv4_mapped()) ;

    CPPUNIT_LOG_IS_FALSE(ipv6_addr(1, 0, 0, 0xFE01, 0, 0, 0, 0xF00D).is_ipv4_mapped()) ;
    CPPUNIT_LOG_IS_FALSE(ipv6_addr(0, 0, 0, 0, 0, 0, 0, 0xF00D).is_ipv4_mapped()) ;

    CPPUNIT_LOG_EQ(ipv6_addr(ipv4_addr(127, 0, 0, 1)).str(), "127.0.0.1") ;

    CPPUNIT_LOG_EQUAL((ipv4_addr)ipv6_addr(ipv4_addr(127, 0, 0, 1)), ipv4_addr(127, 0, 0, 1)) ;
    CPPUNIT_LOG_EQUAL((ipv4_addr)ipv6_addr(0, 0, 0, 0, 0, 0, 0, 0xF00D), ipv4_addr()) ;
    CPPUNIT_LOG_EQUAL((ipv4_addr)ipv6_addr(1, 0, 0, 0, 0, 0xffff, 0, 0xF00D), ipv4_addr()) ;

    // Make a distinction between
    //  - "universal unspecified address", AKA DENIL, which is equal by its binary
    //    representation for _both_ the ipv4 and v6 and is all-zeros 128-bit binary,
    //  - and ipv4 unspecified address, which is ::ffff:0.0.0.0
    //
    CPPUNIT_LOG_EQ(ipv6_addr(ipv4_addr()).str(), "::ffff:0.0.0.0") ;

    CPPUNIT_LOG_ASSERT(ipv6_addr(ipv4_addr())) ;
    CPPUNIT_LOG_IS_FALSE(ipv6_addr()) ;
}

void IPAddressTests::Test_IPv6_Address_Parser()
{
    CPPUNIT_LOG_EQUAL(ipv6_addr("::"), ipv6_addr()) ;

    CPPUNIT_LOG_EQUAL(ipv6_addr("2001:db8:ac10:fe01:feed:babe:cafe:f00d"),
                      ipv6_addr(0x2001, 0x0DB8, 0xAC10, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00D)) ;

    CPPUNIT_LOG_EQUAL(ipv6_addr("0:db8:ac10:fe01:feed:babe:cafe:f00d"),
                      ipv6_addr(0, 0x0DB8, 0xAC10, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00D)) ;

    CPPUNIT_LOG_EQUAL(ipv6_addr("::ac10:fe01:feed:babe:cafe:f00d"),
                      ipv6_addr(0, 0, 0xAC10, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00D)) ;

    CPPUNIT_LOG_EQUAL(ipv6_addr("1:0:ac10:fe01:feed:babe:cafe:f00d"),
                      ipv6_addr(1, 0, 0xAC10, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00D)) ;

    CPPUNIT_LOG_EQUAL(ipv6_addr("1::fe01:feed:babe:cafe:f00d"),
                      ipv6_addr(1, 0, 0, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00D)) ;

    CPPUNIT_LOG_EQUAL(ipv6_addr("1::fe01:0:0:cafe:f00d"),
                      ipv6_addr(1, 0, 0, 0xFE01, 0, 0, 0xCAFE, 0xF00D)) ;

    CPPUNIT_LOG_EQUAL(ipv6_addr("1:0:0:fe01::f00d"),
                      ipv6_addr(1, 0, 0, 0xFE01, 0, 0, 0, 0xF00D)) ;

    CPPUNIT_LOG_EQUAL(ipv6_addr("1:0:0:fe01::"),
                      ipv6_addr(1, 0, 0, 0xFE01, 0, 0, 0, 0)) ;

    CPPUNIT_LOG_EQUAL(ipv6_addr("::f00d"),
                      ipv6_addr(0, 0, 0, 0, 0, 0, 0, 0xF00D)) ;

    CPPUNIT_LOG_EQUAL(ipv6_addr("1::"),
                      ipv6_addr(1, 0, 0, 0, 0, 0, 0, 0)) ;

    CPPUNIT_LOG_EQUAL(ipv6_addr("::1"),
                      ipv6_addr(0, 0, 0, 0, 0, 0, 0, 1)) ;

    CPPUNIT_LOG_EQUAL(ipv6_addr("::ffff:0.0.0.0"),
                      ipv6_addr(0, 0, 0, 0, 0, 0xffff, 0, 0)) ;

    CPPUNIT_LOG_EQUAL(ipv6_addr("::ffff:127.0.0.1"),
                      ipv6_addr(0, 0, 0, 0, 0, 0xffff, 0x7f00, 1)) ;

    CPPUNIT_LOG_EQUAL(ipv6_addr("127.0.0.1"),
                      ipv6_addr(0, 0, 0, 0, 0, 0xffff, 0x7f00, 1)) ;

    CPPUNIT_LOG_EQUAL(ipv6_addr("255.255.255.255"),
                      ipv6_addr(0, 0, 0, 0, 0, 0xffff, 0xffff, 0xffff)) ;

    CPPUNIT_LOG_EQUAL(ipv6_addr("172.16.9.100"),
                      ipv6_addr(0, 0, 0, 0, 0, 0xffff, 0xac10, 0x964)) ;

    CPPUNIT_LOG_EQUAL(ipv6_addr("0.0.0.0"), ipv6_addr()) ;
    CPPUNIT_LOG_EQUAL((ipv4_addr)ipv6_addr("0.0.0.0"), ipv4_addr()) ;

    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG_EQUAL(ipv6_addr("1::fe01:feed:babe:cafe:f00d", ipv6_addr::IGNORE_DOTDEC),
                      ipv6_addr(1, 0, 0, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00D)) ;

    CPPUNIT_LOG_EXCEPTION_MSG(ipv6_addr("::ffff:127.0.0.1", ipv6_addr::IGNORE_DOTDEC), invalid_str_repr, "address") ;

    CPPUNIT_LOG_EXCEPTION_MSG(ipv6_addr("172.16.9.100", ipv6_addr::IGNORE_DOTDEC), invalid_str_repr, "address") ;


    CPPUNIT_LOG(std::endl) ;
    CPPUNIT_LOG(std::endl) ;

    std::errc errcode = {} ;

    CPPUNIT_LOG_EQUAL(ipv6_addr("::ffff:127.0.0.1", errcode, ipv6_addr::IGNORE_DOTDEC), ipv6_addr()) ;
    CPPUNIT_LOG_EQUAL(errcode, std::errc::invalid_argument) ;

    CPPUNIT_LOG_EQUAL(ipv6_addr("1::fe01:feed:babe:cafe:f00d", errcode, ipv6_addr::IGNORE_DOTDEC),
                      ipv6_addr(1, 0, 0, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00D)) ;
    CPPUNIT_LOG_EQUAL(errcode, std::errc()) ;

    CPPUNIT_LOG_EQUAL(ipv6_addr("", errcode), ipv6_addr()) ;
    CPPUNIT_LOG_EQUAL(errcode, std::errc::invalid_argument) ;

    CPPUNIT_LOG_EQUAL(ipv6_addr("", errcode, ipv6_addr::ALLOW_EMPTY), ipv6_addr()) ;
    CPPUNIT_LOG_EQUAL(errcode, std::errc()) ;
}

void IPAddressTests::Test_IPv6_Subnet_Address()
{
    CPPUNIT_LOG_EQUAL(ipv6_subnet(), ipv6_subnet()) ;
    CPPUNIT_LOG_IS_TRUE(ipv6_subnet() == ipv6_subnet()) ;
    CPPUNIT_LOG_IS_FALSE(ipv6_subnet() != ipv6_subnet()) ;

    CPPUNIT_LOG(std::endl) ;

    const ipv6_addr addr_2001_food (0x2001, 0x0DB8, 0xAC10, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00D) ;
    const ipv6_addr addr_00_food   (0,      0,      0xAC10, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00D) ;

    CPPUNIT_LOG_EQUAL(ipv6_subnet(addr_2001_food, 64), ipv6_subnet(addr_2001_food, 64)) ;

    CPPUNIT_LOG_NOT_EQUAL(ipv6_subnet(addr_2001_food, 65), ipv6_subnet(addr_2001_food, 64)) ;
    CPPUNIT_LOG_NOT_EQUAL(ipv6_subnet(addr_2001_food, 63), ipv6_subnet(addr_2001_food, 64)) ;
    CPPUNIT_LOG_NOT_EQUAL(ipv6_subnet(addr_2001_food, 0), ipv6_subnet(addr_2001_food, 64)) ;
    CPPUNIT_LOG_NOT_EQUAL(ipv6_subnet(addr_2001_food, 128), ipv6_subnet(addr_2001_food, 64)) ;

    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG_EQ(ipv6_subnet(0x2001, 0x0DB8, 0xAC10, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00D, 0).str(),
                   "2001:db8:ac10:fe01:feed:babe:cafe:f00d/0") ;

    CPPUNIT_LOG_EQ(ipv6_subnet(0x2001, 0x0DB8, 0xAC10, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00D, 64).str(),
                   "2001:db8:ac10:fe01:feed:babe:cafe:f00d/64") ;

    CPPUNIT_LOG_EQ(ipv6_subnet(addr_2001_food, 64).str(), "2001:db8:ac10:fe01:feed:babe:cafe:f00d/64") ;
    CPPUNIT_LOG_EQ(ipv6_subnet(addr_00_food, 64).str(), "::ac10:fe01:feed:babe:cafe:f00d/64") ;

    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG_EQUAL(ipv6_subnet(addr_00_food, 64).subnet_addr(),
                      ipv6_addr(0, 0, 0xAC10, 0xFE01, 0, 0, 0, 0)) ;

    CPPUNIT_LOG_EQUAL(ipv6_subnet(addr_2001_food, 64).subnet_addr(),
                      ipv6_addr(0x2001, 0x0DB8, 0xAC10, 0xFE01, 0, 0, 0, 0)) ;

    CPPUNIT_LOG_EQUAL(ipv6_subnet(addr_2001_food, 128).subnet_addr(), addr_2001_food) ;
    CPPUNIT_LOG_EQUAL(ipv6_subnet(addr_2001_food, 0).subnet_addr(), ipv6_addr()) ;

    CPPUNIT_LOG_EQUAL(ipv6_subnet(addr_00_food, 63).subnet_addr(),
                      ipv6_addr(0, 0, 0xAC10, 0xFE00, 0, 0, 0, 0)) ;

    CPPUNIT_LOG_EQUAL(ipv6_subnet(addr_00_food, 48).subnet_addr(),
                      ipv6_addr(0, 0, 0xAC10, 0, 0, 0, 0, 0)) ;
    CPPUNIT_LOG_EQUAL(ipv6_subnet(addr_00_food, 48).subnet_addr(),
                      ipv6_addr(0, 0, 0xAC10, 0, 0, 0, 0, 0)) ;

    CPPUNIT_LOG_EQUAL(ipv6_subnet(addr_2001_food, 1).subnet_addr(), ipv6_addr()) ;
    CPPUNIT_LOG_EQUAL(ipv6_subnet(addr_2001_food, 3).subnet_addr(), ipv6_addr(0x2000, 0, 0, 0, 0, 0, 0, 0)) ;

    CPPUNIT_LOG_EQUAL(ipv6_subnet(addr_2001_food, 127).subnet_addr(),
                      ipv6_addr(0x2001, 0x0DB8, 0xAC10, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00C)) ;

    CPPUNIT_LOG_EQUAL(ipv6_subnet(addr_2001_food, 126).subnet_addr(),
                      ipv6_addr(0x2001, 0x0DB8, 0xAC10, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00C)) ;

    CPPUNIT_LOG_EQUAL(ipv6_subnet(addr_2001_food, 125).subnet_addr(),
                      ipv6_addr(0x2001, 0x0DB8, 0xAC10, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF008)) ;

    CPPUNIT_LOG_EQUAL(ipv6_subnet(addr_2001_food, 65).subnet_addr(),
                      ipv6_addr(0x2001, 0x0DB8, 0xAC10, 0xFE01, 0x8000, 0, 0, 0)) ;

    CPPUNIT_LOG_EQUAL(ipv6_subnet(addr_2001_food, 65).subnet(),
                      ipv6_subnet(0x2001, 0x0DB8, 0xAC10, 0xFE01, 0x8000, 0, 0, 0, 65)) ;

    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG_EQUAL(ipv6_subnet("::/0"),  ipv6_subnet()) ;
    CPPUNIT_LOG_EQUAL(ipv6_subnet("::/64"), ipv6_subnet(ipv6_addr(), 64)) ;
    CPPUNIT_LOG_EQ(ipv6_subnet("::/64").pfxlen(), 64) ;

    CPPUNIT_LOG_EQUAL(ipv6_subnet("2001:db8:ac10:fe01:feed:babe:cafe:f00d/0"),
                      ipv6_subnet(0x2001, 0x0DB8, 0xAC10, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00D, 0)) ;

    CPPUNIT_LOG_EQUAL(ipv6_subnet("2001:db8:ac10:fe01:feed:babe:cafe:f00d/128"),
                      ipv6_subnet(0x2001, 0x0DB8, 0xAC10, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00D, 128)) ;

    CPPUNIT_LOG_EQUAL(ipv6_subnet("2001:db8:ac10:fe01:feed:babe:cafe:f00d/64"),
                      ipv6_subnet(0x2001, 0x0DB8, 0xAC10, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00D, 64)) ;

    CPPUNIT_LOG_EQUAL(ipv6_subnet("::ac10:fe01:feed:babe:cafe:f00d/32"),
                      ipv6_subnet(0, 0, 0xAC10, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00D, 32)) ;

    CPPUNIT_LOG_EQUAL(ipv6_subnet("1:0:0:fe01::/16"),
                      ipv6_subnet(1, 0, 0, 0xFE01, 0, 0, 0, 0, 16)) ;

    CPPUNIT_LOG_EQUAL(ipv6_subnet("::f00d/112"),
                      ipv6_subnet(0, 0, 0, 0, 0, 0, 0, 0xF00D, 112)) ;

    CPPUNIT_LOG_EQUAL(ipv6_subnet("1::/16"),
                      ipv6_subnet(1, 0, 0, 0, 0, 0, 0, 0, 16)) ;

    CPPUNIT_LOG_EQUAL(ipv6_subnet("::1/128"),
                      ipv6_subnet(0, 0, 0, 0, 0, 0, 0, 1, 128)) ;

    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG_EXCEPTION_MSG(ipv6_subnet(addr_2001_food, 129), std::invalid_argument, "prefix length") ;
    CPPUNIT_LOG_EXCEPTION(ipv6_subnet("1::/-1"), invalid_str_repr) ;
    CPPUNIT_LOG_EXCEPTION(ipv6_subnet("1::"), invalid_str_repr) ;
    CPPUNIT_LOG_EXCEPTION(ipv6_subnet("1::/129"), invalid_str_repr) ;
    CPPUNIT_LOG_EXCEPTION(ipv6_subnet("1::/0x1"), invalid_str_repr) ;

    CPPUNIT_LOG_EXCEPTION_MSG(ipv6_subnet("1::/0x10"), invalid_str_repr, "IPv6 network prefix specification") ;
    CPPUNIT_LOG_EXCEPTION_MSG(ipv6_subnet("172.16.1.1/12"), invalid_str_repr, "IPv6 network prefix specification") ;
    CPPUNIT_LOG_EXCEPTION_MSG(ipv6_subnet("::ffff:172.16.1.1/12"), invalid_str_repr, "IPv6 network prefix specification") ;
    CPPUNIT_LOG_EXCEPTION_MSG(ipv6_subnet("0.0.0.0/0"), invalid_str_repr, "IPv6 network prefix specification") ;
}

void IPAddressTests::Test_Subnet_Match()
{
    const ipv6_addr addr_2001_food (0x2001, 0x0DB8, 0xAC10, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00D) ;
    const ipv6_addr addr_00_food   (0,      0,      0xAC10, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00D) ;

    const ipv6_addr addr_pre_1 (1, 0, 0, 0, 0, 0, 0, 0) ;
    const ipv6_addr addr_post_1 (0, 0, 0, 0, 0, 0, 0, 1) ;

    CPPUNIT_LOG_ASSERT(ipv4_subnet("172.16.1.1/12").match(ipv4_addr("172.16.1.20"))) ;
    CPPUNIT_LOG_IS_FALSE(ipv4_subnet("172.16.1.1/12").match(ipv4_addr("172.48.1.1"))) ;

    CPPUNIT_LOG_ASSERT(ipv4_subnet("1.1.1.1/0").match(ipv4_addr("172.16.1.20"))) ;
    CPPUNIT_LOG_ASSERT(ipv4_subnet("1.1.1.1/0").match(ipv4_addr("1.0.0.1"))) ;
    CPPUNIT_LOG_ASSERT(ipv4_subnet("1.1.1.1/0").match(ipv4_addr("103.15.17.1"))) ;

    CPPUNIT_LOG_ASSERT(ipv4_subnet("1.1.1.1/32").match(ipv4_addr("1.1.1.1"))) ;
    CPPUNIT_LOG_IS_FALSE(ipv4_subnet("1.1.1.1/32").match(ipv4_addr("1.1.1.0"))) ;

    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG_ASSERT(ipv6_subnet("2001:db8:ac10:fe01:feed:babe:cafe:f00d/128").match(addr_2001_food)) ;

    CPPUNIT_LOG_ASSERT(ipv6_subnet("2001:db8:ac10:fe01:feed:babe:cafe:f00d/128")
                       .match({0x2001, 0x0DB8, 0xAC10, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00D})) ;

    CPPUNIT_LOG_IS_FALSE(ipv6_subnet("2001:db8:ac10:fe01:feed:babe:cafe:f00d/128")
                         .match({0x2001, 0x0DB8, 0xAC10, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00F})) ;

    CPPUNIT_LOG_ASSERT(ipv6_subnet("2001:db8:ac10:fe01:feed:babe:cafe:f00d/125")
                       .match({0x2001, 0x0DB8, 0xAC10, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00F})) ;

    CPPUNIT_LOG_ASSERT(ipv6_subnet("2001:db8:ac10:fe01:feed:babe:cafe:f00d/64")
                       .match({0x2001, 0x0DB8, 0xAC10, 0xFE01,
                               0x1111, 0x2222, 0x3333, 0x4444})) ;

    CPPUNIT_LOG_IS_FALSE(ipv6_subnet("2001:db8:ac10:fe01:feed:babe:cafe:f00d/65")
                         .match({0x2001, 0x0DB8, 0xAC10, 0xFE01,
                                 0x1111, 0x2222, 0x3333, 0x4444})) ;

    CPPUNIT_LOG_ASSERT(ipv6_subnet("2001:db8:ac10:fe01:feed:babe:cafe:f00d/65")
                       .match({0x2001, 0x0DB8, 0xAC10, 0xFE01,
                               0x8111, 0x2222, 0x3333, 0x4444})) ;

    CPPUNIT_LOG_ASSERT(ipv6_subnet("8001::/0").match(addr_00_food)) ;
    CPPUNIT_LOG_ASSERT(ipv6_subnet("2001:db8:ac10:fe01:feed:babe:cafe:f00d/0").match(addr_pre_1)) ;
    CPPUNIT_LOG_ASSERT(ipv6_subnet("2001:db8:ac10:fe01:feed:babe:cafe:f00d/0").match(addr_post_1)) ;

    CPPUNIT_LOG(std::endl) ;

    // IPv4-mapped IPv6 vs. ipv4_subnet
    CPPUNIT_LOG_ASSERT(ipv4_subnet("172.16.1.1/12").match(ipv6_addr("172.16.1.20"))) ;
    CPPUNIT_LOG_IS_FALSE(ipv4_subnet("172.16.1.1/12").match(ipv6_addr("172.48.1.1"))) ;

    CPPUNIT_LOG_ASSERT(ipv4_subnet("1.1.1.1/0").match(ipv6_addr("172.16.1.20"))) ;
    CPPUNIT_LOG_ASSERT(ipv4_subnet("1.1.1.1/0").match(ipv6_addr("1.0.0.1"))) ;
    CPPUNIT_LOG_ASSERT(ipv4_subnet("1.1.1.1/0").match(ipv6_addr("103.15.17.1"))) ;

    CPPUNIT_LOG_ASSERT(ipv4_subnet("1.2.3.4/32").match(ipv6_addr("1.2.3.4"))) ;
    CPPUNIT_LOG_IS_FALSE(ipv4_subnet("1.2.3.4/32").match(ipv6_addr("1.2.3.2"))) ;
    CPPUNIT_LOG_IS_FALSE(ipv4_subnet("1.2.3.4/32").match(ipv6_addr("4.3.2.1"))) ;

    CPPUNIT_LOG_ASSERT(ipv4_subnet("1.2.3.4/32").match(ipv6_addr("::ffff:0102:0304"))) ;
    CPPUNIT_LOG_IS_FALSE(ipv4_subnet("1.2.3.4/32").match(ipv6_addr("::0102:0304"))) ;
}

/*******************************************************************************
 main
*******************************************************************************/
int main(int argc, char *argv[])
{
    return pcomn::unit::run_tests
        <
            IPAddressTests
        >
        (argc, argv) ;
}
