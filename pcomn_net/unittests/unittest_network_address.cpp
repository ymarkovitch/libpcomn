/*-*- tab-width:4;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0) (inline-open . 0) (case-label . +)) -*-*/
/*******************************************************************************
 FILE         :   unittest_network_address.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Internet address classes unit tests.

 CREATION DATE:   27 Jan 2008
*******************************************************************************/
#include <pcomn_net/netaddr.h>
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
    void Test_IP_Address() ;
    void Test_Sock_Address() ;
    void Test_Iface_Address() ;
    void Test_Subnet_Address() ;

    CPPUNIT_TEST_SUITE(InetAddressTests) ;

    CPPUNIT_TEST(Test_IP_Address) ;
    CPPUNIT_TEST(Test_Sock_Address) ;
    CPPUNIT_TEST(Test_Iface_Address) ;
    CPPUNIT_TEST(Test_Subnet_Address) ;

    CPPUNIT_TEST_SUITE_END() ;
} ;

static union {
    uint16_t n ;
    char s[2] ;
    bool is_little() const { return s[0] == 2 ; }
} endianness = { 0x0102 } ;

void InetAddressTests::Test_IP_Address()
{
    CPPUNIT_LOG_EQUAL(net::inet_address(), net::inet_address()) ;
    CPPUNIT_LOG_IS_TRUE(net::inet_address() == net::inet_address()) ;
    CPPUNIT_LOG_IS_FALSE(net::inet_address() != net::inet_address()) ;
    CPPUNIT_LOG_IS_FALSE(net::inet_address() < net::inet_address()) ;
    CPPUNIT_LOG_EQUAL(net::inet_address().ipaddr(), (uint32_t)0) ;
    CPPUNIT_LOG_EQUAL(net::inet_address().inaddr().s_addr, (uint32_t)0) ;
    CPPUNIT_LOG_EQUAL(net::inet_address().str(), std::string("0.0.0.0")) ;
    CPPUNIT_LOG_EXCEPTION(net::inet_address(""), std::invalid_argument) ;

    CPPUNIT_LOG_EQUAL(net::inet_address("", net::inet_address::ALLOW_EMPTY).ipaddr(), (uint32_t)0) ;
    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG_EQUAL(net::inet_address(65, 66, 67, 68).str(), std::string("65.66.67.68")) ;
    CPPUNIT_LOG_EQUAL(net::inet_address(65, 66, 67, 68).ipaddr(), (uint32_t)0x41424344) ;
    CPPUNIT_LOG_EQUAL(net::inet_address("65.66.67.68").ipaddr(), (uint32_t)0x41424344) ;
    CPPUNIT_LOG_EQUAL(((in_addr)net::inet_address(65, 66, 67, 68)).s_addr, (uint32_t)(endianness.is_little() ? 0x44434241 : 0x41424344)) ;
    CPPUNIT_LOG_EQUAL(net::inet_address(0x41424344).ipaddr(), (uint32_t)0x41424344) ;
    in_addr InAddr ;
    CPPUNIT_LOG_RUN(InAddr.s_addr = htonl(0x41424344)) ;
    CPPUNIT_LOG_EQUAL(net::inet_address(InAddr).ipaddr(), (uint32_t)0x41424344) ;
    CPPUNIT_LOG_EQUAL(net::inet_address(InAddr).str(), std::string("65.66.67.68")) ;

    CPPUNIT_LOG_EQUAL(net::inet_address(1, 2, 3, 4).str(), std::string("1.2.3.4")) ;
    CPPUNIT_LOG_EQUAL(net::inet_address(1, 2, 3, 4).octet(0), (uint8_t)1) ;
    CPPUNIT_LOG_EQUAL(net::inet_address(1, 2, 3, 4).octet(2), (uint8_t)3) ;
    CPPUNIT_LOG_EQUAL(net::inet_address("127.0.0.2"), net::inet_address(127, 0, 0, 2)) ;
    CPPUNIT_LOG_EQUAL(net::inet_address("127.0.0.2").octets()[0], (uint8_t)127) ;
    CPPUNIT_LOG_EQUAL(net::inet_address("127.0.0.2").octets()[3], (uint8_t)2) ;
    CPPUNIT_LOG(std::endl) ;

    // Use root nameserver address for testing: I _dearly_ hope it will not change!
    CPPUNIT_LOG_EQUAL(net::inet_address("j.root-servers.net"), net::inet_address(192, 58, 128, 30)) ;
    CPPUNIT_LOG_EQUAL(net::inet_address(192, 58, 128, 30).hostname(), std::string("j.root-servers.net")) ;

    CPPUNIT_LOG_EQUAL(net::inet_address("localhost").str(), std::string("127.0.0.1")) ;
    CPPUNIT_LOG_EQUAL(net::inaddr_loopback(), net::inet_address("localhost")) ;
    CPPUNIT_LOG_EQUAL(net::inaddr_broadcast(), net::inet_address(255, 255, 255, 255)) ;
    CPPUNIT_LOG_EXCEPTION(net::inet_address("Hello, world!"), net::inaddr_error) ;
    CPPUNIT_LOG_EQUAL(net::inet_address(1, 2, 3, 4).hostname(), std::string("1.2.3.4")) ;
}

void InetAddressTests::Test_Sock_Address()
{
    CPPUNIT_LOG_EQUAL(net::sock_address(), net::sock_address()) ;
    CPPUNIT_LOG_IS_TRUE(net::sock_address().is_null()) ;
    CPPUNIT_LOG_IS_TRUE(net::sock_address() == net::sock_address()) ;
    CPPUNIT_LOG_IS_FALSE(net::sock_address() != net::sock_address()) ;
    CPPUNIT_LOG_IS_FALSE(net::sock_address() < net::sock_address()) ;
    CPPUNIT_LOG_IS_TRUE(!net::sock_address().addr().ipaddr()) ;
    CPPUNIT_LOG_EQUAL(net::sock_address().port(), (uint16_t)0) ;
    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG_EQUAL(net::sock_address(50000).str(), std::string("127.0.0.1:50000")) ;
    CPPUNIT_LOG_EQUAL(net::sock_address("localhost", 50000).port(), (uint16_t)50000) ;
    CPPUNIT_LOG_EQUAL(net::sock_address("localhost", 50000).addr(), net::inaddr_loopback()) ;
    CPPUNIT_LOG_EQUAL(net::sock_address(50001).addr(), net::inaddr_loopback()) ;
    CPPUNIT_LOG_EQUAL(net::sock_address(50001).port(), (uint16_t)50001) ;
    CPPUNIT_LOG_EQUAL(net::sock_address(50000), net::sock_address(50000)) ;
    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG_IS_TRUE(net::sock_address(50000) != net::sock_address(50001)) ;
    CPPUNIT_LOG_IS_TRUE(net::sock_address(50000) < net::sock_address(50001)) ;
    CPPUNIT_LOG_IS_TRUE(net::sock_address(50001) > net::sock_address(50000)) ;
    CPPUNIT_LOG_IS_TRUE(net::sock_address(50001) >= net::sock_address(50000)) ;
    CPPUNIT_LOG_IS_TRUE(net::sock_address(50000) >= net::sock_address(50000)) ;
    CPPUNIT_LOG_IS_FALSE(net::sock_address(49999) >= net::sock_address(50000)) ;
    CPPUNIT_LOG_IS_TRUE(net::sock_address(49999) <= net::sock_address(50000)) ;
    CPPUNIT_LOG_IS_TRUE(net::sock_address(50000) <= net::sock_address(50000)) ;
    CPPUNIT_LOG_IS_FALSE(net::sock_address(50000) <= net::sock_address(49999)) ;
    CPPUNIT_LOG_EQUAL(net::sock_address(net::inet_address(1, 2, 3, 4), 50000),
                      net::sock_address(net::inet_address(1, 2, 3, 4), 50000)) ;
    CPPUNIT_LOG_IS_TRUE(net::sock_address(net::inet_address(2, 2, 3, 4), 50000) > net::sock_address(net::inet_address(1, 2, 3, 4), 50000)) ;
    CPPUNIT_LOG_IS_TRUE(net::sock_address(net::inet_address(1, 2, 3, 3), 50000) < net::sock_address(net::inet_address(1, 2, 3, 4), 50000)) ;
    CPPUNIT_LOG_IS_TRUE(net::sock_address(net::inet_address(1, 2, 3, 3), 50001) < net::sock_address(net::inet_address(1, 2, 3, 4), 50000)) ;
    CPPUNIT_LOG(std::endl) ;

    sockaddr_in sa ;

    CPPUNIT_LOG_RUN(memset(&sa, 0, sizeof sa)) ;
    CPPUNIT_LOG_RUN((sa.sin_family = AF_INET, sa.sin_port = htons(50002), sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK))) ;
    CPPUNIT_LOG_EQUAL(net::sock_address(sa), net::sock_address(net::inet_address(127, 0, 0, 1), 50002)) ;
    CPPUNIT_LOG_RUN(memset(&sa, 0, sizeof sa)) ;

    net::sock_address SockAddr ;
    CPPUNIT_LOG_RUN(SockAddr = net::sock_address(net::inet_address(127, 0, 0, 2), 49999)) ;
    CPPUNIT_LOG_EQUAL((int)SockAddr.as_sockaddr_in()->sin_family, (int)AF_INET) ;
    CPPUNIT_LOG_EQUAL(SockAddr.as_sockaddr_in()->sin_port, htons(49999)) ;
    CPPUNIT_LOG_EQUAL(SockAddr.as_sockaddr_in()->sin_addr.s_addr, htonl(0x7f000002)) ;
}

void InetAddressTests::Test_Iface_Address()
{
#ifdef __linux__
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
        CPPUNIT_LOG_EQUAL(net::inet_address(ifname, net::inet_address::FROM_IFACE), net::inet_address(ifaddr)) ;
    }
#endif

    CPPUNIT_LOG_EQUAL(net::iface_addr("lo"), net::inaddr_loopback()) ;
    CPPUNIT_LOG_EQUAL(net::iface_addr("65.66.67.68"), net::inet_address(65, 66, 67, 68)) ;
    CPPUNIT_LOG_EQUAL(net::inet_address("lo", net::inet_address::USE_IFACE), net::inaddr_loopback()) ;
    CPPUNIT_LOG_EQUAL(net::inet_address("65.66.67.68", net::inet_address::USE_IFACE), net::inet_address(65, 66, 67, 68)) ;
    CPPUNIT_LOG_EQUAL(net::inet_address("localhost", net::inet_address::USE_IFACE), net::inaddr_loopback()) ;
    CPPUNIT_LOG_EXCEPTION(net::inet_address("lo"), net::inaddr_error) ;

    fd_safehandle ssock(::socket (PF_INET, SOCK_STREAM, 0)) ;
    CPPUNIT_LOG_ASSERT(ssock.handle() >= 0) ;
    CPPUNIT_LOG_ASSERT(net::iface_addr("NoSuch").ipaddr() == 0) ;
    CPPUNIT_LOG_EXCEPTION(net::inet_address("NoSuch", net::inet_address::FROM_IFACE), net::inaddr_error) ;
}

void InetAddressTests::Test_Subnet_Address()
{
    using namespace net ;

    CPPUNIT_LOG_EQUAL(subnet_address(), subnet_address()) ;
    CPPUNIT_LOG_IS_TRUE(subnet_address() == subnet_address()) ;
    CPPUNIT_LOG_IS_FALSE(subnet_address() != subnet_address()) ;

    CPPUNIT_LOG_EQ(subnet_address(65, 66, 67, 68, 32).str(), "65.66.67.68/32") ;
    CPPUNIT_LOG_EQ(subnet_address(65, 66, 67, 68, 24).str(), "65.66.67.68/24") ;
    CPPUNIT_LOG_EQ(subnet_address(65, 66, 67, 68, 24).str(), "65.66.67.68/24") ;
    CPPUNIT_LOG_EQ(subnet_address(65, 66, 67, 68, 24).subnet().str(), "65.66.67.0/24") ;
    CPPUNIT_LOG_ASSERT(subnet_address(65, 66, 67, 0, 24) < subnet_address(65, 66, 68, 0, 24)) ;
    CPPUNIT_LOG_ASSERT(subnet_address(65, 66, 67, 0, 24) < subnet_address(65, 66, 67, 0, 25)) ;

    CPPUNIT_LOG_EQUAL(subnet_address(65, 66, 67, 68, 24).addr(), inet_address(65, 66, 67, 68)) ;
    CPPUNIT_LOG_EQUAL(subnet_address(65, 66, 67, 68, 24).subnet().addr(), inet_address(65, 66, 67, 0)) ;
    CPPUNIT_LOG_EQ(subnet_address(65, 66, 67, 68, 24).pfxlen(), 24) ;
    CPPUNIT_LOG_EQ(subnet_address(65, 66, 67, 68, 24).netmask(), 0xffffff00) ;
}

int main(int argc, char *argv[])
{
    pcomn::unit::TestRunner runner ;
    runner.addTest(InetAddressTests::suite()) ;

    return
        pcomn::unit::run_tests(runner, argc, argv, "unittest.trace.ini",
                               "Testing internet address classes.") ;
}
