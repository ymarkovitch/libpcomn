/*-*- tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_hash.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2017. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for hash functions/classes.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   29 Jun 2017
*******************************************************************************/
#include <pcomn_hash.h>
#include <pcomn_string.h>
#include <pcomn_strslice.h>
#include <pcomn_unittest.h>

#include <typeinfo>
#include <memory>
#include <fstream>

#include <stdio.h>

/*******************************************************************************
 HashFnTests
*******************************************************************************/
class HashFnTests : public CppUnit::TestFixture {

   private:
      void Test_Hash_Functions() ;

      CPPUNIT_TEST_SUITE(HashFnTests) ;

      CPPUNIT_TEST(Test_Hash_Functions) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

static const char * const StrArray[] =
{
   "Str1", "Str2", "Str3", "Str4", "Str5", "Str6", "Str7", "Str8", "Str9", "Str10",
   "Str1", "Str2", "Str3", "Str4", "Str5", "Str6", "Str7", "Str8", "Str9", "Str10"
} ;

void HashFnTests::Test_Hash_Functions()
{
   CPPUNIT_LOG_NOT_EQUAL(pcomn::hash_fn<int>()(0), (size_t)0) ;
   CPPUNIT_LOG_NOT_EQUAL(pcomn::hash_fn<int>()(1), (size_t)1) ;
   CPPUNIT_LOG_NOT_EQUAL(pcomn::hash_fn<int>()(1), pcomn::hash_fn<int>()(0)) ;

   CPPUNIT_LOG_EQUAL(pcomn::hash_fn<int>()(13), pcomn::hash_fn<long>()(13)) ;
   CPPUNIT_LOG_EQUAL(pcomn::hash_fn<unsigned short>()(13), pcomn::hash_fn<int>()(13)) ;

   CPPUNIT_LOG_EQUAL(pcomn::hash_fn<unsigned char>()(13), (size_t)13) ;
   CPPUNIT_LOG_EQUAL(pcomn::hash_fn<bool>()(true), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(pcomn::hash_fn<bool>()(false), (size_t)0) ;

   const char * const Hello = "Hello, world!" ;
   CPPUNIT_LOG_ASSERT(pcomn::hasher(Hello) != pcomn::hasher((const void *)Hello)) ;
   CPPUNIT_LOG_EQUAL(pcomn::hasher(Hello), pcomn::hasher("Hello, world!")) ;
   CPPUNIT_LOG_EQUAL(pcomn::hasher(Hello), pcomn::hasher(std::string("Hello, world!"))) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(pcomn::hash_fn_seq<std::string>()(CPPUNIT_CONTAINER(std::vector<const char *>, ("Foo")("Bar"))),
                     pcomn::tuple_hash().append_data("Foo").append_data("Bar").value()) ;

   CPPUNIT_LOG_NOT_EQUAL(pcomn::hash_fn_seq<std::string>()(CPPUNIT_CONTAINER(std::vector<const char *>, ("Foo"))),
                     pcomn::tuple_hash().append_data("Foo").append_data("Bar").value()) ;

   const char * const Foo = "Foo" ;
   const char * const Bar = "Bar" ;
   CPPUNIT_LOG_EQUAL(pcomn::hash_fn_seq<const char *>()(CPPUNIT_CONTAINER(std::vector<const char *>, (Foo)(Bar))),
                     pcomn::tuple_hash().append_data(Foo).append_data(Bar).value()) ;
   CPPUNIT_LOG_NOT_EQUAL(pcomn::hash_fn_seq<const void *>()(CPPUNIT_CONTAINER(std::vector<const char *>, (Foo)(Bar))),
                         pcomn::tuple_hash().append_data(Foo).append_data(Bar).value()) ;

   CPPUNIT_LOG_EQUAL(pcomn::hash_fn_seq<const char *>()(CPPUNIT_CONTAINER(std::vector<const char *>, (Foo)(Bar))),
                     pcomn::tuple_hash().append(pcomn::hasher(Foo)).append_data(Bar).value()) ;
   CPPUNIT_LOG_EQUAL(pcomn::hash_fn_seq<const char *>()(CPPUNIT_CONTAINER(std::vector<const char *>, (Foo)(Bar))),
                     pcomn::tuple_hash(pcomn::hasher(Foo)).append_data(Bar).value()) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(pcomn::hash_fn_seq<int>()(CPPUNIT_CONTAINER(std::vector<int>, (1)(2)(3))),
                     pcomn::tuple_hash().append_data(1).append_data(2).append_data(3).value()) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(HashFnTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv, "unittest.diag.ini", "Closed hashtable tests") ;
}
