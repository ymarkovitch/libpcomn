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
#include <pcomn_cstrptr.h>
#include <pcomn_unittest.h>

#include <typeinfo>
#include <memory>
#include <fstream>

#include <stdio.h>

/*******************************************************************************
 HashFnTests
*******************************************************************************/
class HashFnTests : public CppUnit::TestFixture {

      void Test_Hash_Functions() ;
      void Test_String_Hash() ;
      void Test_Tuple_Hash() ;

      CPPUNIT_TEST_SUITE(HashFnTests) ;

      CPPUNIT_TEST(Test_Hash_Functions) ;
      CPPUNIT_TEST(Test_String_Hash) ;
      CPPUNIT_TEST(Test_Tuple_Hash) ;

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

   CPPUNIT_LOG_EQUAL(pcomn::hash_fn<int>()(13), size_t(12198420960622345777ULL)) ;

   CPPUNIT_LOG_EQUAL(pcomn::hash_fn<int>()(13), pcomn::hash_fn<long>()(13)) ;
   CPPUNIT_LOG_EQUAL(pcomn::hash_fn<unsigned short>()(13), pcomn::hash_fn<size_t>()(13)) ;

   CPPUNIT_LOG_EQUAL(pcomn::hash_fn<bool>()(true), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(pcomn::hash_fn<bool>()(false), (size_t)0) ;

   const char * const Hello = "Hello, world!" ;
   CPPUNIT_LOG_ASSERT(pcomn::valhash(Hello) != pcomn::valhash((const void *)Hello)) ;
   CPPUNIT_LOG_EQUAL(pcomn::valhash(Hello), pcomn::valhash("Hello, world!")) ;
   CPPUNIT_LOG_EQUAL(pcomn::valhash(Hello), pcomn::valhash(std::string("Hello, world!"))) ;

   CPPUNIT_LOG(std::endl) ;
   typedef std::vector<const char *> cstr_vector ;

   CPPUNIT_LOG_EQUAL(pcomn::hash_fn_sequence<cstr_vector>()(cstr_vector{"Foo", "Bar"}),
                     pcomn::hash_combinator().append_data("Foo").append_data("Bar").value()) ;

   CPPUNIT_LOG_NOT_EQUAL(pcomn::hash_fn_sequence<cstr_vector>()(cstr_vector{"Foo"}),
                         pcomn::hash_combinator().append_data("Foo").append_data("Bar").value()) ;

   const char * const Foo = "Foo" ;
   const char * const Bar = "Bar" ;
   CPPUNIT_LOG_EQUAL(pcomn::hash_sequence(cstr_vector{Foo, Bar}),
                     pcomn::hash_combinator().append_data(Foo).append_data(Bar).value()) ;

   CPPUNIT_LOG_NOT_EQUAL((pcomn::hash_fn_sequence<cstr_vector, pcomn::hash_fn<void *>>()(cstr_vector{Foo, Bar})),
                         pcomn::hash_combinator().append_data(Foo).append_data(Bar).value()) ;

   CPPUNIT_LOG_EQUAL(pcomn::hash_fn_sequence<cstr_vector>()(cstr_vector{Foo, Bar}),
                     pcomn::hash_combinator().append(pcomn::valhash(Foo)).append_data(Bar).value()) ;
   CPPUNIT_LOG_EQUAL(pcomn::hash_fn_sequence<cstr_vector>()(cstr_vector{Foo, Bar}),
                     pcomn::hash_combinator(pcomn::valhash(Foo)).append_data(Bar).value()) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(pcomn::hash_sequence({1, 2, 3}), pcomn::hash_combinator().append_data(1).append_data(2).append_data(3).value()) ;
}

void HashFnTests::Test_String_Hash()
{
   const pcomn::hash_fn<char *> cstr_hash ;
   const pcomn::hash_fn<std::string> str_hash ;
   const pcomn::hash_fn<pcomn::strslice> sslice_hash ;
   const pcomn::hash_fn<pcomn::cstrptr> cstrptr_hash ;

   using pcomn::strslice ;
   using pcomn::cstrptr ;

   CPPUNIT_LOG_EQUAL(cstr_hash(nullptr), cstr_hash("")) ;
   CPPUNIT_LOG_NOT_EQUAL(cstr_hash(" "), cstr_hash("")) ;

   CPPUNIT_LOG_EQUAL(cstr_hash(""), str_hash(std::string())) ;
   CPPUNIT_LOG_EQUAL(pcomn::valhash(cstrptr("")), cstr_hash("")) ;
   CPPUNIT_LOG_EQUAL(cstrptr_hash(cstrptr()), cstr_hash("")) ;
   CPPUNIT_LOG_EQUAL(sslice_hash(strslice()), cstr_hash("")) ;
   CPPUNIT_LOG_EQUAL(sslice_hash(strslice("")), cstr_hash("")) ;

   CPPUNIT_LOG_EQUAL(cstr_hash("Hello"), str_hash(std::string("Hello"))) ;
   CPPUNIT_LOG_NOT_EQUAL(cstr_hash("Hello"), cstr_hash("")) ;
   CPPUNIT_LOG_NOT_EQUAL(cstr_hash("Hello"), cstr_hash("hello")) ;
   CPPUNIT_LOG_EQUAL(pcomn::valhash(cstrptr("Hello")), cstr_hash("Hello")) ;
   CPPUNIT_LOG_EQUAL(cstrptr_hash(cstrptr("Hello")), cstr_hash("Hello")) ;
}

void HashFnTests::Test_Tuple_Hash()
{
   using pcomn::strslice ;
   using std::tuple ;
   using std::pair ;
   using std::string ;

   CPPUNIT_LOG_EQUAL(pcomn::hash_fn<tuple<>>()(tuple<>()), pcomn::tuplehash()) ;
   CPPUNIT_LOG_EQUAL((pcomn::hash_fn<pair<int, string>>()({10, "Foo"})),  pcomn::tuplehash(10, "Foo")) ;
   CPPUNIT_LOG_EQUAL((pcomn::hash_fn<pair<int, string>>()({10, "Foo"})),  pcomn::valhash(pair<int, string>(10, "Foo"))) ;
   CPPUNIT_LOG_EQUAL((pcomn::hash_fn<tuple<int, string>>()({10, "Foo"})), pcomn::valhash(pair<int, string>(10, "Foo"))) ;

   CPPUNIT_LOG_NOT_EQUAL(pcomn::tuplehash(10), pcomn::tuplehash(10, "Foo")) ;
   CPPUNIT_LOG_NOT_EQUAL(pcomn::tuplehash("Foo"), pcomn::tuplehash(10, "Foo")) ;
   CPPUNIT_LOG_NOT_EQUAL(pcomn::tuplehash("Foo", 10), pcomn::tuplehash(10, "Foo")) ;

   CPPUNIT_LOG_NOT_EQUAL(pcomn::tuplehash(10), pcomn::valhash(10)) ;
   CPPUNIT_LOG_EQUAL((pcomn::hash_fn<tuple<int>>()({10})), pcomn::tuplehash(10)) ;

   CPPUNIT_LOG_EQUAL((pcomn::hash_fn<tuple<int, double, strslice>>()({10, 0.25, "Bar"})),
                     pcomn::tuplehash((char)10, 0.25, string("Bar"))) ;

   CPPUNIT_LOG_EQUAL((pcomn::hash_fn<tuple<int, float, strslice, size_t, char>>()({10, 0.25, "Bar", 1024*1024*8192ULL, 'A'})),
                     pcomn::tuplehash(10, 0.25, "Bar", 1024*1024*8192LL, 'A')) ;
}

int main(int argc, char *argv[])
{
   return pcomn::unit::run_tests
      <
         HashFnTests
      >
      (argc, argv) ;
}
