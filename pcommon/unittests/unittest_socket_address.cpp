/*-*- tab-width:4;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
/*******************************************************************************
 FILE         :   unittest_socket_address.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Socket and interface address tests.

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
                            class SocketAddressTests
*******************************************************************************/
class SocketAddressTests : public CppUnit::TestFixture {
private:
    void Test_Sock_Address() ;
    void Test_Iface_Address() ;

    CPPUNIT_TEST_SUITE(SocketAddressTests) ;

    CPPUNIT_TEST(Test_Sock_Address) ;
    CPPUNIT_TEST(Test_Iface_Address) ;

    CPPUNIT_TEST_SUITE_END() ;
} ;

/*******************************************************************************
 SocketAddressTests
*******************************************************************************/
void SocketAddressTests::Test_Sock_Address()
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

void SocketAddressTests::Test_Iface_Address()
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

int main(int argc, char *argv[])
{
    #ifdef PCOMN_PL_MS
    WSAStartup(MAKEWORD(2, 0), &std::move<WSADATA>(WSADATA{})) ;
    #endif

    return pcomn::unit::run_tests
        <
            SocketAddressTests
        >
        (argc, argv) ;
}
