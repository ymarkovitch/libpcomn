/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_utils.cpp
 COPYRIGHT    :   Yakov Markovitch, 2011-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests of various stuff from pcomn_utils.h

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   13 Apr 2011
*******************************************************************************/
#include <pcomn_utils.h>
#include <pcomn_meta.h>
#include <pcomn_tuple.h>
#include <pcomn_unittest.h>

#include <utility>

/*******************************************************************************
 Utility tests: pointer hacks, tuples, etc
*******************************************************************************/
class UtilityTests : public CppUnit::TestFixture {

      void Test_TaggedPtr() ;
      void Test_TypeTraits() ;
      void Test_TupleUtils() ;

      CPPUNIT_TEST_SUITE(UtilityTests) ;

      CPPUNIT_TEST(Test_TaggedPtr) ;
      CPPUNIT_TEST(Test_TypeTraits) ;
      CPPUNIT_TEST(Test_TupleUtils) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

using namespace pcomn ;

/*******************************************************************************
 UtilityTests
*******************************************************************************/
void UtilityTests::Test_TaggedPtr()
{
   int dummy ;
   int *ptr = &dummy ;

   CPPUNIT_LOG_EQUAL(untag_ptr(ptr), ptr) ;
   CPPUNIT_LOG_NOT_EQUAL(tag_ptr(ptr), ptr) ;
   CPPUNIT_LOG_EQUAL(untag_ptr(tag_ptr(ptr)), ptr) ;
   CPPUNIT_LOG_EQUAL(fliptag_ptr(ptr), tag_ptr(ptr)) ;
   CPPUNIT_LOG_EQUAL(fliptag_ptr(fliptag_ptr(ptr)), ptr) ;

   CPPUNIT_LOG_IS_NULL(null_if_tagged_or_null(tag_ptr(ptr))) ;
   CPPUNIT_LOG_IS_NULL(null_if_tagged_or_null((int *)NULL)) ;
   CPPUNIT_LOG_EQUAL(null_if_tagged_or_null(ptr), ptr) ;

   CPPUNIT_LOG_IS_NULL(null_if_untagged_or_null(ptr)) ;
   CPPUNIT_LOG_IS_NULL(null_if_tagged_or_null((int *)NULL)) ;
   CPPUNIT_LOG_EQUAL(null_if_untagged_or_null(tag_ptr(ptr)), ptr) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_IS_TRUE(is_ptr_tagged(tag_ptr(ptr))) ;
   CPPUNIT_LOG_IS_FALSE(is_ptr_tagged((int *)NULL)) ;
   CPPUNIT_LOG_IS_FALSE(is_ptr_tagged(ptr)) ;

   CPPUNIT_LOG_IS_TRUE(is_ptr_tagged_or_null(tag_ptr(ptr))) ;
   CPPUNIT_LOG_IS_TRUE(is_ptr_tagged_or_null((int *)NULL)) ;
   CPPUNIT_LOG_IS_FALSE(is_ptr_tagged_or_null(ptr)) ;
}

void UtilityTests::Test_TypeTraits()
{
   CPPUNIT_LOG_IS_TRUE((std::is_trivially_copyable<std::pair<int, char *> >::value)) ;
   CPPUNIT_LOG_IS_TRUE((std::has_trivial_copy_assign<std::pair<int, char *> >::value)) ;
   CPPUNIT_LOG_IS_FALSE((std::is_trivially_copyable<std::pair<int, std::string> >::value)) ;
}

void UtilityTests::Test_TupleUtils()
{
   const std::tuple<> empty_tuple ;
   const std::tuple<std::string, int, const char *> t3 {"Hello", 3, "world"} ;

   CPPUNIT_LOG_EQUAL(CPPUNIT_STRING(empty_tuple), std::string("()")) ;
   CPPUNIT_LOG_EQUAL(CPPUNIT_STRING(t3), std::string(R"(("Hello" 3 world))")) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(UtilityTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "unittest.diag.ini", "Tests of various stuff from pcomn_utils.h") ;
}
