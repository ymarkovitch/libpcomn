/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
/*******************************************************************************
 FILE         :   unittest_netstreams.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Streams-over-sockets tests.

 CREATION DATE:   14 Jun 2008
*******************************************************************************/
#include <pcomn_net/netstreams.h>

#include <stdlib.h>
#include <stdio.h>

using namespace pcomn ;

/*******************************************************************************
                            class SocketStreamTests
*******************************************************************************/
class SocketStreamTests : public CppUnit::TestFixture {
private:
    void Test_Socket_Stream() ;
    void Test_Socket_Bufstream() ;

    CPPUNIT_TEST_SUITE(SocketStreamTests) ;

    CPPUNIT_TEST(Test_Socket_Stream) ;
    CPPUNIT_TEST(Test_Socket_Bufstream) ;

    CPPUNIT_TEST_SUITE_END() ;

    std::string EchoServerName ;

public:
    void setUp()
    {
        EchoServerName = CPPUNIT_AT_TESTDIR("echoserver-stream.py ") ;
    }
} ;

#define SPAWN_ECHOSERVER(sleep_after)                                   \
    pcomn::unit::spawncmd echoserver (EchoServerName + "'run(port=49999)'", false) ; \
    CPPUNIT_LOG("Spawned echo server listening at port 49999" << std::endl) ; \
    sleep(sleep_after)

void SocketStreamTests::Test_Socket_Stream()
{
    CPPUNIT_LOG_EXCEPTION(net::socket_istream(net::stream_socket_ptr(NULL)), std::invalid_argument) ;
    CPPUNIT_LOG_EXCEPTION(net::socket_ostream(net::stream_socket_ptr(NULL)), std::invalid_argument) ;

    char buf[8096] ;

    SPAWN_ECHOSERVER(2) ;
    {
        net::stream_socket_ptr sock (new net::client_socket(net::sock_address(49999))) ;
        net::socket_istream is (sock) ;
        net::socket_ostream os (sock) ;

        CPPUNIT_LOG_EQUAL(is.timeout(), -1) ;
        CPPUNIT_LOG_EQUAL(os.timeout(), -1) ;
        CPPUNIT_LOG_RUN(is.set_timeout(1000)) ;
        CPPUNIT_LOG_RUN(os.set_timeout(1000)) ;
        CPPUNIT_LOG_EQUAL(is.timeout(), 1000) ;
        CPPUNIT_LOG_EQUAL(os.timeout(), 1000) ;
        CPPUNIT_LOG_RUN(is.set_timeout(-1)) ;
        CPPUNIT_LOG_RUN(os.set_timeout(-1)) ;
        CPPUNIT_LOG_EQUAL(is.timeout(), -1) ;
        CPPUNIT_LOG_EQUAL(os.timeout(), -1) ;

        CPPUNIT_LOG(std::endl) ;
        CPPUNIT_LOG_EQUAL(os.write("Hello, world!"), (size_t)13) ;
        CPPUNIT_LOG_EQUAL(is.read(pcomn::unit::fillstrbuf(buf), sizeof buf - 1), (size_t)13) ;
        CPPUNIT_LOG_EQUAL(std::string(buf), std::string("Hello, world!")) ;
    }

    CPPUNIT_LOG(std::endl) ;
    {
        net::stream_socket_ptr sock (new net::client_socket(net::sock_address(49999))) ;
        net::socket_istream is (sock) ; net::socket_ostream os (sock) ;

        CPPUNIT_LOG_EQUAL(os.write("Bye, baby!"), (size_t)10) ;
        CPPUNIT_LOG_EQUAL((char)is.get(), 'B') ;
        CPPUNIT_LOG_EQUAL((char)is.get(), 'y') ;
        CPPUNIT_LOG_EQUAL(is.read(pcomn::unit::fillstrbuf(buf), sizeof buf - 1), (size_t)8) ;
        CPPUNIT_LOG_EQUAL(std::string(buf), std::string("e, baby!")) ;
    }
}

void SocketStreamTests::Test_Socket_Bufstream()
{
    char buf[8096] ;

    SPAWN_ECHOSERVER(1) ;
    {
        net::stream_socket_ptr sock (new net::client_socket(net::sock_address(49999))) ;
        pcomn::binary_ibufstream is (new net::socket_istream(sock), 2048) ;
        pcomn::binary_obufstream os (new net::socket_ostream(sock), 2048) ;

        CPPUNIT_LOG(std::endl) ;
        CPPUNIT_LOG_EQUAL(os.write("Hello, world!"), (size_t)13) ;
        CPPUNIT_LOG_RUN(os.flush()) ;
        CPPUNIT_LOG_EQUAL(is.read(pcomn::unit::fillstrbuf(buf), sizeof buf - 1), (size_t)13) ;
        CPPUNIT_LOG_EQUAL(std::string(buf), std::string("Hello, world!")) ;
    }

    CPPUNIT_LOG(std::endl) ;
    {
        net::stream_socket_ptr sock (new net::client_socket(net::sock_address(49999))) ;
        pcomn::binary_ibufstream is (new net::socket_istream(sock), 2048) ;
        pcomn::binary_obufstream os (new net::socket_ostream(sock), 2048) ;

        CPPUNIT_LOG_EQUAL(os.write("Bye, "), (size_t)5) ;
        CPPUNIT_LOG_RUN(os.put('b').put('a').put('b').put('y').put('!')) ;
        CPPUNIT_LOG_RUN(os.flush()) ;

        CPPUNIT_LOG_EQUAL((char)is.get(), 'B') ;
        CPPUNIT_LOG_EQUAL((char)is.get(), 'y') ;
        CPPUNIT_LOG_EQUAL(is.read(pcomn::unit::fillstrbuf(buf), sizeof buf - 1), (size_t)8) ;
        CPPUNIT_LOG_EQUAL(std::string(buf), std::string("e, baby!")) ;
    }
}

int main(int argc, char *argv[])
{
    net::stream_socket_ptr sock (new net::client_socket(net::sock_address(49999))) ;
    net::socket_istream is (sock) ;
    net::socket_ostream os (sock) ;

    pcomn::unit::TestRunner runner ;
    runner.addTest(SocketStreamTests::suite()) ;

    return
        pcomn::unit::run_tests(runner, argc, argv, "unittest.trace.ini",
                               "Testing socket streams.") ;
}
