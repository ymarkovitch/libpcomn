/*-*- tab-width:4;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
/*******************************************************************************
 FILE         :   unittest_network_address.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Internet address classes unit tests.

 CREATION DATE:   27 Jan 2008
*******************************************************************************/
#include <pcomn_netaddr.h>
#include <pcomn_unittest.h>

#include <pcomn_string.h>
#include <pcomn_handle.h>
#include <pcomn_exec.h>

#include <stdexcept>

#include <stdlib.h>
#include <stdio.h>

using namespace pcomn ;

/*******************************************************************************
                            class InetAddressTests
*******************************************************************************/
class InetAddressTests : public CppUnit::TestFixture {
private:
    void Test_IPv4_Address() ;
    void Test_IPv6_Address() ;
    void Test_IPv6_Address_Parser() ;
    void Test_Sock_Address() ;
    void Test_Iface_Address() ;
    void Test_Subnet_Address() ;

    CPPUNIT_TEST_SUITE(InetAddressTests) ;

    CPPUNIT_TEST(Test_IPv4_Address) ;
    CPPUNIT_TEST(Test_IPv6_Address) ;
    CPPUNIT_TEST(Test_IPv6_Address_Parser) ;
    CPPUNIT_TEST(Test_Sock_Address) ;
    CPPUNIT_TEST(Test_Iface_Address) ;
    CPPUNIT_TEST(Test_Subnet_Address) ;

    CPPUNIT_TEST_SUITE_END() ;
} ;

static constexpr union {
    uint16_t n ;
    char s[2] ;
    bool is_little() const { return s[0] == 2 ; }
} endianness = { 0x0102 } ;

void InetAddressTests::Test_IPv4_Address()
{
    CPPUNIT_LOG_EQUAL(ipv4_addr(), ipv4_addr()) ;
    CPPUNIT_LOG_IS_TRUE(ipv4_addr() == ipv4_addr()) ;
    CPPUNIT_LOG_IS_FALSE(ipv4_addr() != ipv4_addr()) ;
    CPPUNIT_LOG_IS_FALSE(ipv4_addr() < ipv4_addr()) ;
    CPPUNIT_LOG_EQ(ipv4_addr().ipaddr(), 0) ;
    CPPUNIT_LOG_EQ(ipv4_addr().inaddr().s_addr, 0) ;
    CPPUNIT_LOG_EQUAL(ipv4_addr().str(), std::string("0.0.0.0")) ;
    CPPUNIT_LOG_EXCEPTION(ipv4_addr(""), std::invalid_argument) ;

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

    // Use root nameserver address for testing: I _dearly_ hope it will not change!
    CPPUNIT_LOG_EQUAL(ipv4_addr("j.root-servers.net", ipv4_addr::USE_HOSTNAME),
                      ipv4_addr(192, 58, 128, 30)) ;
    CPPUNIT_LOG_EQUAL(ipv4_addr(192, 58, 128, 30).hostname(),
                      std::string("j.root-servers.net")) ;

    CPPUNIT_LOG_EQUAL(ipv4_addr("localhost", ipv4_addr::USE_HOSTNAME).str(), std::string("127.0.0.1")) ;
    CPPUNIT_LOG_EQUAL(inaddr_loopback(), ipv4_addr("localhost", ipv4_addr::USE_HOSTNAME)) ;
    CPPUNIT_LOG_EQUAL(inaddr_broadcast(), ipv4_addr(255, 255, 255, 255)) ;
    CPPUNIT_LOG_EXCEPTION(ipv4_addr("Hello, world!", ipv4_addr::USE_HOSTNAME), system_error) ;
    CPPUNIT_LOG_EQUAL(ipv4_addr(1, 2, 3, 4).hostname(), std::string("1.2.3.4")) ;
}

void InetAddressTests::Test_IPv6_Address()
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


    CPPUNIT_LOG_EQ(ipv6_addr(0x2001, 0x0DB8, 0xAC10, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00D).str(),
                   "2001:db8:ac10:fe01:feed:babe:cafe:f00d") ;

    CPPUNIT_LOG_EQ(ipv6_addr(0, 0x0DB8, 0xAC10, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00D).str(),
                   "0:db8:ac10:fe01:feed:babe:cafe:f00d") ;

    CPPUNIT_LOG_EQ(ipv6_addr(0, 0, 0xAC10, 0xFE01, 0xFEED, 0xBABE, 0xCAFE, 0xF00D).str(),
                   "::ac10:fe01:feed:babe:cafe:f00d") ;

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
    CPPUNIT_LOG_ASSERT(ipv6_addr(ipv4_addr(127, 0, 0, 1)).is_mapped_ipv4()) ;

    CPPUNIT_LOG_EQ(ipv6_addr(ipv4_addr(127, 0, 0, 1)).str(), "127.0.0.1") ;

    // Make a distinction between
    //  - "universal unspecified address", AKA DENIL, which is equal by its binary
    //    representation for _both_ the ipv4 and v6 and is all-zeros 128-bit binary,
    //  - and ipv4 unspecified address, which is ::ffff:0.0.0.0
    //
    CPPUNIT_LOG_EQ(ipv6_addr(ipv4_addr()).str(), "::ffff:0.0.0.0") ;

    CPPUNIT_LOG_ASSERT(ipv6_addr(ipv4_addr())) ;
    CPPUNIT_LOG_IS_FALSE(ipv6_addr()) ;
}

void InetAddressTests::Test_IPv6_Address_Parser()
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
}

void InetAddressTests::Test_Sock_Address()
{
    CPPUNIT_LOG_EQUAL(sock_address(), sock_address()) ;
    CPPUNIT_LOG_IS_TRUE(sock_address().is_null()) ;
    CPPUNIT_LOG_IS_TRUE(sock_address() == sock_address()) ;
    CPPUNIT_LOG_IS_FALSE(sock_address() != sock_address()) ;
    CPPUNIT_LOG_IS_FALSE(sock_address() < sock_address()) ;
    CPPUNIT_LOG_IS_TRUE(!sock_address().addr().ipaddr()) ;
    CPPUNIT_LOG_EQUAL(sock_address().port(), (uint16_t)0) ;
    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG_EQUAL(sock_address(50000).str(), std::string("127.0.0.1:50000")) ;
    CPPUNIT_LOG_EQUAL(sock_address("localhost", 50000).port(), (uint16_t)50000) ;
    CPPUNIT_LOG_EQUAL(sock_address("localhost", 50000).addr(), inaddr_loopback()) ;
    CPPUNIT_LOG_EQUAL(sock_address(50001).addr(), inaddr_loopback()) ;
    CPPUNIT_LOG_EQUAL(sock_address(50001).port(), (uint16_t)50001) ;
    CPPUNIT_LOG_EQUAL(sock_address(50000), sock_address(50000)) ;
    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG_IS_TRUE(sock_address(50000) != sock_address(50001)) ;
    CPPUNIT_LOG_IS_TRUE(sock_address(50000) < sock_address(50001)) ;
    CPPUNIT_LOG_IS_TRUE(sock_address(50001) > sock_address(50000)) ;
    CPPUNIT_LOG_IS_TRUE(sock_address(50001) >= sock_address(50000)) ;
    CPPUNIT_LOG_IS_TRUE(sock_address(50000) >= sock_address(50000)) ;
    CPPUNIT_LOG_IS_FALSE(sock_address(49999) >= sock_address(50000)) ;
    CPPUNIT_LOG_IS_TRUE(sock_address(49999) <= sock_address(50000)) ;
    CPPUNIT_LOG_IS_TRUE(sock_address(50000) <= sock_address(50000)) ;
    CPPUNIT_LOG_IS_FALSE(sock_address(50000) <= sock_address(49999)) ;
    CPPUNIT_LOG_EQUAL(sock_address(ipv4_addr(1, 2, 3, 4), 50000),
                      sock_address(ipv4_addr(1, 2, 3, 4), 50000)) ;
    CPPUNIT_LOG_IS_TRUE(sock_address(ipv4_addr(2, 2, 3, 4), 50000) > sock_address(ipv4_addr(1, 2, 3, 4), 50000)) ;
    CPPUNIT_LOG_IS_TRUE(sock_address(ipv4_addr(1, 2, 3, 3), 50000) < sock_address(ipv4_addr(1, 2, 3, 4), 50000)) ;
    CPPUNIT_LOG_IS_TRUE(sock_address(ipv4_addr(1, 2, 3, 3), 50001) < sock_address(ipv4_addr(1, 2, 3, 4), 50000)) ;
    CPPUNIT_LOG(std::endl) ;

    sockaddr_in sa ;

    CPPUNIT_LOG_RUN(memset(&sa, 0, sizeof sa)) ;
    CPPUNIT_LOG_RUN((sa.sin_family = AF_INET, sa.sin_port = htons(50002), sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK))) ;
    CPPUNIT_LOG_EQUAL(sock_address(sa), sock_address(ipv4_addr(127, 0, 0, 1), 50002)) ;
    CPPUNIT_LOG_RUN(memset(&sa, 0, sizeof sa)) ;

    sock_address SockAddr ;
    CPPUNIT_LOG_RUN(SockAddr = sock_address(ipv4_addr(127, 0, 0, 2), 49999)) ;
    CPPUNIT_LOG_EQUAL((int)SockAddr.as_sockaddr_in()->sin_family, (int)AF_INET) ;
    CPPUNIT_LOG_EQUAL(SockAddr.as_sockaddr_in()->sin_port, htons(49999)) ;
    CPPUNIT_LOG_EQUAL(SockAddr.as_sockaddr_in()->sin_addr.s_addr, htonl(0x7f000002)) ;
}

void InetAddressTests::Test_Iface_Address()
{
#ifdef PCOMN_PL_LINUX
    std::string ifaddr ;
    std::string ifname ;
    CPPUNIT_LOG_RUN(ifaddr = str::strip(sys::shellcmd("ifconfig eth0 | grep -Eoe 'inet addr:[0-9]+[.][0-9]+[.][0-9]+[.][0-9]+'",
                                                      DONT_RAISE_ERROR).second).stdstring()) ;
    if (!ifaddr.empty())
        ifname = "eth0" ;
    else
    {
        CPPUNIT_LOG_RUN(ifaddr = str::strip(sys::shellcmd("ifconfig eth1 | grep -Eoe 'inet addr:[0-9]+[.][0-9]+[.][0-9]+[.][0-9]+'",
                                                          DONT_RAISE_ERROR).second).stdstring()) ;
        ifname = "eth1" ;
    }
    if (ifaddr.empty() || !str::startswith(ifaddr, "inet addr:") || ifaddr.erase(0, strlen("inet addr:")).empty())
        CPPUNIT_LOG("Cannot find out ethernet address. Skipping iface_addr test." << std::endl) ;
    else
    {
        CPPUNIT_LOG("ifname: " << ifname << ", ifaddr: " << ifaddr << std::endl) ;
        CPPUNIT_LOG_EQUAL(ipv4_addr(ifname, ipv4_addr::ONLY_IFACE), ipv4_addr(ifaddr)) ;
    }

    CPPUNIT_LOG_EQUAL(iface_addr("lo"), inaddr_loopback()) ;
    CPPUNIT_LOG_EQUAL(ipv4_addr("lo", ipv4_addr::ONLY_IFACE), inaddr_loopback()) ;
#endif

    // There is no network interface with such _name_: "65.66.67.68"
    CPPUNIT_LOG_EQUAL(iface_addr("65.66.67.68"), ipv4_addr()) ;

    CPPUNIT_LOG_EQUAL(ipv4_addr("65.66.67.68", ipv4_addr::USE_IFACE), ipv4_addr(65, 66, 67, 68)) ;
    CPPUNIT_LOG_EQUAL(ipv4_addr("localhost", ipv4_addr::USE_IFACE|ipv4_addr::USE_HOSTNAME), inaddr_loopback()) ;
    CPPUNIT_LOG_EXCEPTION(ipv4_addr("lo", ipv4_addr::USE_HOSTNAME), system_error) ;

    CPPUNIT_LOG_ASSERT(iface_addr("NoSuch").ipaddr() == 0) ;
    CPPUNIT_LOG_EXCEPTION(ipv4_addr("NoSuch", ipv4_addr::ONLY_IFACE), system_error) ;
}

void InetAddressTests::Test_Subnet_Address()
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
}

int main(int argc, char *argv[])
{
    pcomn::unit::TestRunner runner ;
    runner.addTest(InetAddressTests::suite()) ;

    #ifdef PCOMN_PL_MS
    WSAStartup(MAKEWORD(2, 0), &std::move<WSADATA>(WSADATA{})) ;
    #endif

    return
        pcomn::unit::run_tests(runner, argc, argv, "unittest.trace.ini",
                               "Testing internet address classes.") ;
}
