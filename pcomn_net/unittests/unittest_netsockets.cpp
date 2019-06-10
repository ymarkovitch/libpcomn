/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
/*******************************************************************************
 FILE         :   unittest_netsockets.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2017. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Network sockets tests.

 CREATION DATE:   31 Jan 2008
*******************************************************************************/
#include <pcomn_net/netsockets.h>
#include <pcomn_unittest.h>
#include <pcomn_sys.h>
#include <pcomn_exec.h>

#include <stdlib.h>
#include <stdio.h>

using namespace pcomn ;
using namespace pcomn::sys ;
using namespace pcomn::unit ;

/*******************************************************************************
                            class StreamSocketTests
*******************************************************************************/
class StreamSocketTests : public CppUnit::TestFixture {
private:
    void Test_Client_Socket_Read_Write() ;
    void Test_Server_Socket() ;

    CPPUNIT_TEST_SUITE(StreamSocketTests) ;

    CPPUNIT_TEST(Test_Client_Socket_Read_Write) ;
    //CPPUNIT_TEST(Test_Server_Socket) ;

    CPPUNIT_TEST_SUITE_END() ;

    std::string EchoServerName ;
} ;

void StreamSocketTests::Test_Client_Socket_Read_Write()
{
    EchoServerName = CPPUNIT_AT_TESTDIR("echoserver-stream.py ") ;

    // Attempt to connect to a nonexistent host fails with a timeout.
    CPPUNIT_LOG_EXCEPTION(net::client_socket(net::sock_address(ipv4_addr(1, 2, 3, 4), 777), 100), net::network_error) ;
    // Host exists but theree is no service on the port; in _no_ case there
    // should be timeout_error, only connection_error!
    CPPUNIT_LOG_EXCEPTION(net::client_socket(net::sock_address(777)), net::connection_error) ;

    char buf[8096] ;
    {
        spawncmd echoserver (EchoServerName + "'run(port=49999)'", false) ;
        CPPUNIT_LOG("Spawned echo server listening at port 49999" << std::endl) ;
        sleep(2) ;
        net::client_socket sock (net::sock_address(49999)) ;
        CPPUNIT_LOG_ASSERT(sock.handle() > 2) ;
        CPPUNIT_LOG_EQUAL(sock.sock_addr().addr(), net::inaddr_loopback()) ;
        CPPUNIT_LOG_ASSERT(sock.sock_addr().port() > 1024) ;
        CPPUNIT_LOG_EQUAL(sock.peer_addr(), net::sock_address(49999)) ;
        CPPUNIT_LOG_EQUAL(sock.transmit("Hello, world!"), (size_t)13) ;
        CPPUNIT_LOG_EQUAL(sock.receive(fillstrbuf(buf), sizeof buf - 1), (size_t)13) ;
        CPPUNIT_LOG_EQUAL(std::string(buf), std::string("Hello, world!")) ;
    }
}

void StreamSocketTests::Test_Server_Socket()
{
    struct local {
        static void run_client(forkcmd &process, int port)
        {
            if (process.is_child())
                try
                {
                    fprintf(stderr, "%u is a connecting client, connecting to a port %d\n",
                            (unsigned)getpid(), port) ;

                    const net::sock_address svraddr (port) ;
                    net::stream_socket_ptr clnt (new net::client_socket(svraddr)) ;
                    std::cerr << "\nConnected " << *clnt << std::endl ;
                    clnt.reset() ;

                    sleep(2) ;

                    clnt = new net::client_socket(svraddr) ;
                    std::cerr << "\nConnected " << *clnt << std::endl ;

                    fprintf(stderr, "\n%u Connecting client is exiting\n", (unsigned)getpid()) ;
                    exit(0) ;
                }
                catch (const std::exception &x)
                {
                    std::cerr << getpid() << " " << STDEXCEPTOUT(x) << std::endl ;
                    exit(3) ;
                }
        }
    } ;

    const net::sock_address addr (61001) ;
    net::server_socket acceptor (addr) ;

    CPPUNIT_LOG_ASSERT(!(pcomn::sys::set_fflags(acceptor.handle(), -1, O_NONBLOCK) & O_NONBLOCK)) ;
    CPPUNIT_LOG_ASSERT(pcomn::sys::fflags(acceptor.handle()) & O_NONBLOCK) ;
    CPPUNIT_LOG_RUN(acceptor.listen()) ;
    CPPUNIT_LOG_IS_NULL(acceptor.accept()) ;

    net::sock_address accepted_addr ;
    CPPUNIT_LOG_EXCEPTION(net::stream_socket(acceptor, &accepted_addr), net::socket_error) ;

    forkcmd connecting_client ;
    local::run_client(connecting_client, addr.port()) ;
    sleep(1) ;

    net::stream_socket_ptr sock ;
    CPPUNIT_LOG_ASSERT(sock = acceptor.accept()) ;
    CPPUNIT_LOG_EXPRESSION(*sock) ;
    CPPUNIT_LOG_ASSERT(pcomn::sys::set_fflags(acceptor.handle(), 0, O_NONBLOCK) & O_NONBLOCK) ;
    CPPUNIT_LOG_ASSERT(!(pcomn::sys::fflags(acceptor.handle()) & O_NONBLOCK)) ;

    CPPUNIT_LOG_ASSERT((sock = new net::stream_socket(acceptor, &accepted_addr))->handle() >= 0) ;
    CPPUNIT_LOG_EXPRESSION(*sock) ;
}

int main(int argc, char *argv[])
{
    pcomn::unit::TestRunner runner ;
    runner.addTest(StreamSocketTests::suite()) ;

    return
        pcomn::unit::run_tests(runner, argc, argv, "unittest.trace.ini",
                               "Testing sockets.") ;
}
