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

      void Test_CompileTime_Utils() ;
      void Test_PtrTag() ;
      void Test_TaggedPtrUnion() ;
      void Test_TypeTraits() ;
      void Test_TupleUtils() ;

      CPPUNIT_TEST_SUITE(UtilityTests) ;

      CPPUNIT_TEST(Test_PtrTag) ;
      CPPUNIT_TEST(Test_TaggedPtrUnion) ;
      CPPUNIT_TEST(Test_TypeTraits) ;
      CPPUNIT_TEST(Test_TupleUtils) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

using namespace pcomn ;

/*******************************************************************************
 UtilityTests
*******************************************************************************/
void UtilityTests::Test_CompileTime_Utils()
{
   PCOMN_STATIC_CHECK(ct_min<int, 0>::value == 0) ;
   PCOMN_STATIC_CHECK(ct_min<int, 1, -1>::value == -1) ;
   PCOMN_STATIC_CHECK(ct_min<int, 200, 100, 300>::value == 100) ;
   PCOMN_STATIC_CHECK(ct_min<int, 200, 300, 5>::value == 5) ;

   PCOMN_STATIC_CHECK(ct_max<unsigned, 1>::value == 1) ;
   PCOMN_STATIC_CHECK(ct_max<unsigned, 1, 5>::value == 5) ;
   PCOMN_STATIC_CHECK(ct_max<unsigned, 200, 300, 100>::value == 300) ;
   PCOMN_STATIC_CHECK(ct_max<int, 200, 100, 0, 300>::value == 300) ;
}

void UtilityTests::Test_PtrTag()
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

void UtilityTests::Test_TaggedPtrUnion()
{
   typedef tagged_ptr_union<double, int> tagged_ptr_di ;
   tagged_ptr_di di ;
   CPPUNIT_IS_FALSE(di) ;

   PCOMN_STATIC_CHECK(std::is_same<tagged_ptr_di::first_type, double *>::value) ;
   PCOMN_STATIC_CHECK(std::is_same<tagged_ptr_di::second_type, int *>::value) ;

   PCOMN_STATIC_CHECK(std::is_same<tagged_ptr_di::element_type<0>, double *>::value) ;
   PCOMN_STATIC_CHECK(std::is_same<tagged_ptr_di::element_type<1>, int *>::value) ;

   PCOMN_STATIC_CHECK(is_ptr_exact_assignable<double, double>::value) ;
   PCOMN_STATIC_CHECK(is_ptr_exact_assignable<const double, double>::value) ;
   PCOMN_STATIC_CHECK(is_ptr_exact_assignable<const double, const double>::value) ;
   PCOMN_STATIC_CHECK(!is_ptr_exact_assignable<double, const double>::value) ;

   struct A { void *a ; } ;
   struct B : A {} ;
   PCOMN_STATIC_CHECK(is_ptr_exact_assignable<A, A>::value) ;
   PCOMN_STATIC_CHECK(is_ptr_exact_assignable<B, B>::value) ;
   PCOMN_STATIC_CHECK(!is_ptr_exact_assignable<B, A>::value) ;

   CPPUNIT_LOG_EQ((detail::count_exact_copyable<true, double, double>::value), 1) ;
   CPPUNIT_LOG_EQ((detail::count_exact_copyable<false, double, double>::value), 1) ;
   CPPUNIT_LOG_EQ((detail::count_exact_copyable<true, double>::value), 0) ;
   CPPUNIT_LOG_EQ((detail::count_exact_copyable<false, double>::value), 0) ;
   CPPUNIT_LOG_EQ((detail::count_exact_copyable<true, double, int>::value), 0) ;
   CPPUNIT_LOG_EQ((detail::count_exact_copyable<false, double, int>::value), 0) ;
   CPPUNIT_LOG_EQ((detail::count_exact_copyable<true, double, int, double>::value), 1) ;
   CPPUNIT_LOG_EQ((detail::count_exact_copyable<false, double, int, double>::value), 1) ;
   CPPUNIT_LOG_EQ((detail::count_exact_copyable<true, double, int, double, void, double>::value), 2) ;
   CPPUNIT_LOG_EQ((detail::count_exact_copyable<false, double, int, double, void, double>::value), 2) ;

   CPPUNIT_LOG_EQ((detail::find_exact_copyable<true, double, double>::value), 0) ;
   CPPUNIT_LOG_EQ((detail::find_exact_copyable<false, double, double>::value), 0) ;
   CPPUNIT_LOG_EQ((detail::find_exact_copyable<true, double>::value), -1) ;
   CPPUNIT_LOG_EQ((detail::find_exact_copyable<false, double>::value), -1) ;
   CPPUNIT_LOG_EQ((detail::find_exact_copyable<true, double, int>::value), -1) ;
   CPPUNIT_LOG_EQ((detail::find_exact_copyable<false, double, int>::value), -1) ;
   CPPUNIT_LOG_EQ((detail::find_exact_copyable<true, double, int, double>::value), 1) ;
   CPPUNIT_LOG_EQ((detail::find_exact_copyable<true, double, int, double, void, double>::value), 1) ;
   CPPUNIT_LOG_EQ((detail::find_exact_copyable<false, double, int, double, void, double>::value), 1) ;

   CPPUNIT_LOG_EQ((detail::find_exact_copyable<true, const double, bool, int, double, void>::value), -1) ;
   CPPUNIT_LOG_EQ((detail::find_exact_copyable<false, const double, bool, int, double, void>::value), 2) ;
   CPPUNIT_LOG_EQ((detail::find_exact_copyable<false, const double, bool, int, const double, void>::value), 2) ;

   CPPUNIT_LOG(std::endl) ;

   typedef tagged_ptr_union<double, int, A> tagged_ptr_dia ;
   A a1 { nullptr } ;
   int i1 = 7 ;
   double d1 = 0.25 ;

   tagged_ptr_dia pa1 {&a1} ;
   tagged_ptr_dia pi1 {&i1} ;
   tagged_ptr_dia pd1 {&d1} ;

   CPPUNIT_LOG_ASSERT(tagged_ptr_dia(nullptr) == (void *)nullptr) ;
   CPPUNIT_LOG_EQ(tagged_ptr_dia(nullptr).type_ndx(), 0) ;
   CPPUNIT_LOG_EQ(tagged_ptr_dia(&d1).type_ndx(), 0) ;
   CPPUNIT_LOG_EQ(tagged_ptr_dia(&i1).type_ndx(), 1) ;
   CPPUNIT_LOG_EQ(tagged_ptr_dia(&a1).type_ndx(), 2) ;

   CPPUNIT_LOG_IS_NULL(pa1.get<0>()) ;
   CPPUNIT_LOG_IS_NULL(pa1.get<1>()) ;
   CPPUNIT_LOG_EQUAL(pa1.get<2>(), &a1) ;

   CPPUNIT_LOG_IS_NULL(pa1.get<int>()) ;
   CPPUNIT_LOG_IS_NULL(pa1.get<double>()) ;
   CPPUNIT_LOG_EQUAL(pa1.get<A>(), &a1) ;

   CPPUNIT_LOG_IS_NULL(pi1.get<0>()) ;
   CPPUNIT_LOG_IS_NULL(pi1.get<2>()) ;
   CPPUNIT_LOG_EQUAL(pi1.get<1>(), &i1) ;

   CPPUNIT_LOG_IS_NULL(pi1.get<double>()) ;
   CPPUNIT_LOG_IS_NULL(pi1.get<A>()) ;
   CPPUNIT_LOG_EQUAL(pi1.get<int>(), &i1) ;
   CPPUNIT_LOG_EQ(pi1.get<const int>(), &i1) ;

   CPPUNIT_LOG_IS_NULL(pd1.get<1>()) ;
   CPPUNIT_LOG_IS_NULL(pd1.get<2>()) ;
   CPPUNIT_LOG_EQUAL(pd1.get<0>(), &d1) ;

   CPPUNIT_LOG_EQ((const void *)&d1, pd1) ;
   CPPUNIT_LOG_EQ((const void *)&a1, pa1) ;
   CPPUNIT_LOG_EQ((const void *)&i1, pi1) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(pd1 = &a1) ;
   CPPUNIT_LOG_IS_NULL(pd1.get<0>()) ;
   CPPUNIT_LOG_IS_NULL(pd1.get<1>()) ;
   CPPUNIT_LOG_EQUAL(pd1.get<2>(), &a1) ;
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

/*******************************************************************************
 UnittestTests
*******************************************************************************/
extern const char UNITTEST_FIXTURE[] = "unittest" ;
class UnittestTests : public unit::TestFixture<UNITTEST_FIXTURE> {

      void Test_Unittest_Diff_Empty() ;
      void Test_Unittest_Diff() ;
      void Test_Unittest_Diff_Nofile_Fail() ;
      void Test_Unittest_Diff_Mismatch_Fail() ;

      CPPUNIT_TEST_SUITE(UnittestTests) ;

      CPPUNIT_TEST(Test_Unittest_Diff_Empty) ;
      CPPUNIT_TEST(Test_Unittest_Diff) ;

      CPPUNIT_TEST_FAIL(Test_Unittest_Diff_Nofile_Fail) ;
      CPPUNIT_TEST_FAIL(Test_Unittest_Diff_Mismatch_Fail) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

void UnittestTests::Test_Unittest_Diff_Empty()
{
   CPPUNIT_LOG_RUN(data_ostream()) ;
   CPPUNIT_LOG_RUN(ensure_data_file_match()) ;
}

void UnittestTests::Test_Unittest_Diff()
{
   CPPUNIT_RUN(data_ostream() << "  Start\nHello, world!\nBye, baby...\n42\n    end\n") ;
   CPPUNIT_LOG_RUN(ensure_data_file_match()) ;
}

void UnittestTests::Test_Unittest_Diff_Nofile_Fail()
{
   CPPUNIT_LOG_RUN(ensure_data_file_match()) ;
}

void UnittestTests::Test_Unittest_Diff_Mismatch_Fail()
{
   CPPUNIT_RUN(data_ostream() << "Hello, world!\nBye, baby...\n") ;
   CPPUNIT_LOG_RUN(ensure_data_file_match()) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(UtilityTests::suite()) ;
   runner.addTest(UnittestTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "unittest.diag.ini", "Tests of various stuff from pcomn_utils.h") ;
}
