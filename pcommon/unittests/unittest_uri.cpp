/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_uri.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unit test fixture for pcomn::uri

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   19 Mar 2008
*******************************************************************************/
#include <pcomn_uri.h>
#include <pcomn_unittest.h>

#include <iostream>
#include <utility>

using namespace pcomn ;
using namespace pcomn::uri ;

class UriTests : public CppUnit::TestFixture {

      template<typename S>
      void Test_Url_Parse() ;
      void Test_Url_Unparse() ;
      void Test_Url_Query() ;

      CPPUNIT_TEST_SUITE(UriTests) ;

      CPPUNIT_TEST(Test_Url_Parse<std::string>) ;
      CPPUNIT_TEST(Test_Url_Parse<const char *>) ;
      CPPUNIT_TEST(Test_Url_Parse<pcomn::strslice>) ;
      CPPUNIT_TEST(Test_Url_Unparse) ;
      CPPUNIT_TEST(Test_Url_Query) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

template<typename S>
void UriTests::Test_Url_Parse()
{
   typedef pcomn::uri::URL<S> Url ;

   CPPUNIT_LOG_EQUAL(Url("http://localhost/hello").path(), pcomn::strslice("/hello")) ;
   CPPUNIT_LOG_EQUAL(Url("http://localhost/hello").host(), pcomn::strslice("localhost")) ;
   CPPUNIT_LOG_EQUAL(Url("http://localhost/hello").scheme(), pcomn::strslice("http")) ;
   CPPUNIT_LOG_EQUAL(Url("http://localhost/hello").query(), pcomn::strslice()) ;
   CPPUNIT_LOG_EQUAL(Url("http://localhost:5080/hello").port(), (unsigned)5080) ;

   CPPUNIT_LOG_ASSERT(Url("http://localhost")) ;
   CPPUNIT_LOG_ASSERT(Url("http://localhost/hello.world")) ;
   CPPUNIT_LOG_ASSERT(Url("/hello.world")) ;
   CPPUNIT_LOG_IS_FALSE(Url("http://")) ;
}

void UriTests::Test_Url_Unparse()
{
   typedef pcomn::uri::URL<std::string> Url ;

   CPPUNIT_LOG_EQUAL(Url("http", "localhost", "/hello").str(), std::string("http://localhost/hello")) ;
   CPPUNIT_LOG_EQUAL(Url("http", "localhost", 8080, "/hello").str(), std::string("http://localhost:8080/hello")) ;
}

void UriTests::Test_Url_Query()
{
   query_dictionary dict ;
   CPPUNIT_LOG_EQUAL(query_decode("foo=bar+foobar&quux=", dict), std::string()) ;
   CPPUNIT_LOG_EQUAL(dict.size(), (size_t)2) ;
   CPPUNIT_LOG_ASSERT(dict.has_key("foo")) ;
   CPPUNIT_LOG_ASSERT(dict.has_key("quux")) ;
   CPPUNIT_LOG_EQUAL(dict.get("foo"), std::string("bar foobar")) ;
   CPPUNIT_LOG_EQUAL(dict.get("quux"), std::string()) ;

   CPPUNIT_LOG_EQUAL(URI("http://localhost/hello?foo=bar+foobar&quux=").query(), pcomn::strslice("foo=bar+foobar&quux=")) ;
   CPPUNIT_LOG_EQUAL(URI("http://localhost/hello?foo=bar+foobar&quux=").query(dict).size(), (size_t)2) ;
}


int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(UriTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "unittest.diag.ini", "URI tests") ;
}
