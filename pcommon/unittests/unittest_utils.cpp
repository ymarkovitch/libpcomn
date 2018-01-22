/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_utils.cpp
 COPYRIGHT    :   Yakov Markovitch, 2011-2017. All rights reserved.
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
      void Test_TupleCompare() ;
      void Test_StreamUtils() ;
      void Test_String_Cast() ;
      void Test_Underlying_Int() ;
      void Test_Folding() ;

      CPPUNIT_TEST_SUITE(UtilityTests) ;

      CPPUNIT_TEST(Test_PtrTag) ;
      CPPUNIT_TEST(Test_TaggedPtrUnion) ;
      CPPUNIT_TEST(Test_TypeTraits) ;
      CPPUNIT_TEST(Test_TupleUtils) ;
      CPPUNIT_TEST(Test_StreamUtils) ;
      CPPUNIT_TEST(Test_String_Cast) ;
      CPPUNIT_TEST(Test_Underlying_Int) ;
      CPPUNIT_TEST(Test_Folding) ;

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
   CPPUNIT_LOG_IS_FALSE((std::is_trivially_copyable<std::pair<int, std::string> >::value)) ;
}

namespace {
struct visitor {
      template<typename... Args>
      void operator()(Args &&...args)
      {
         result += string_cast(sizeof...(Args)) + ":" + string_cast(const_tie(std::forward<Args>(args)...)) + "\n" ;
      }

      std::string &result ;
} ;
}

void UtilityTests::Test_TupleUtils()
{
   const std::tuple<> empty_tuple ;
   const std::tuple<std::string, int, const char *> t3 {"Hello", 3, "world"} ;
   const std::pair<int, double> p1 {20, 1.5} ;


   CPPUNIT_LOG_EQUAL(CPPUNIT_STRING(empty_tuple), std::string("()")) ;
   CPPUNIT_LOG_EQUAL(CPPUNIT_STRING(t3), std::string(R"(("Hello" 3 world))")) ;

   PCOMN_STATIC_CHECK(pcomn::tuplesize<std::tuple<> >() != -65535) ;
   PCOMN_STATIC_CHECK(pcomn::tuplesize<int>() != -65535) ;
   PCOMN_STATIC_CHECK(pcomn::tuplesize(t3) != -65535) ;

   CPPUNIT_LOG_EQ(pcomn::tuplesize<std::tuple<> >(), 0) ;
   CPPUNIT_LOG_EQ(pcomn::tuplesize<int>(), -1) ;
   CPPUNIT_LOG_EQ(pcomn::tuplesize(20), -1) ;
   CPPUNIT_LOG_EQ(pcomn::tuplesize(empty_tuple), 0) ;
   CPPUNIT_LOG_EQ(pcomn::tuplesize(t3), 3) ;

   CPPUNIT_LOG(std::endl) ;
   std::string s ;
   visitor v = {s} ;
   CPPUNIT_LOG_RUN(tuple_zip(v, p1)) ;
   CPPUNIT_LOG_EQ(s, "1:{20}\n1:{1.5}\n") ;
   s.clear() ;
   CPPUNIT_LOG_RUN(tuple_zip(v, p1, t3)) ;
   CPPUNIT_LOG_EQ(s, "2:{20,Hello}\n2:{1.5,3}\n") ;
   s.clear() ;
   CPPUNIT_LOG_RUN(tuple_zip(v, t3, p1)) ;
   CPPUNIT_LOG_EQ(s, "2:{Hello,20}\n2:{3,1.5}\n") ;
   s.clear() ;
   CPPUNIT_LOG_RUN(tuple_zip(v, empty_tuple, p1)) ;
   CPPUNIT_LOG_EQ(s, "") ;
}

namespace {
#if PCOMN_STL_CXX14
typedef std::less<void> lt ;
typedef std::equal_to<void> eq ;
#else
struct lt {
      template<typename T>
      bool operator()(const T &x, const T &y) const { return std::less<T>()(x, y) ; }
} ;
struct eq {
      template<typename T>
      bool operator()(const T &x, const T &y) const { return std::equal_to<T>()(x, y) ; }
} ;
#endif
}

void UtilityTests::Test_TupleCompare()
{
   using namespace pcomn ;

   CPPUNIT_LOG_IS_FALSE(std::less<std::tuple<>>()(std::tuple<>(), std::tuple<>())) ;

   CPPUNIT_LOG_IS_FALSE(less_tuple<lt>(unipair<int>(5, 10), unipair<int>(5, 10))) ;
   CPPUNIT_LOG_ASSERT(less_tuple<lt>(unipair<int>(5, 9), unipair<int>(5, 10))) ;
   CPPUNIT_LOG_ASSERT(less_tuple<lt>(unipair<int>(4, 15), unipair<int>(5, 10))) ;
   CPPUNIT_LOG_IS_FALSE(less_tuple<lt>(unipair<int>(5, 11), unipair<int>(5, 10))) ;

   CPPUNIT_LOG_IS_FALSE(less_tuple<lt>(std::tuple<int,int>(5, 10), std::tuple<int,int>(5, 10))) ;
   CPPUNIT_LOG_ASSERT(less_tuple<lt>(std::tuple<int,int>(5, 9), std::tuple<int,int>(5, 10))) ;
   CPPUNIT_LOG_ASSERT(less_tuple<lt>(std::tuple<int,int>(4, 15), std::tuple<int,int>(5, 10))) ;
   CPPUNIT_LOG_IS_FALSE(less_tuple<lt>(std::tuple<int,int>(5, 11), std::tuple<int,int>(5, 10))) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_IS_FALSE(less_tuple<lt>(single<std::string>("BBB"), single<std::string>("BBB"))) ;
   CPPUNIT_LOG_IS_FALSE(less_tuple<lt>(single<std::string>("BBB"), single<std::string>("BAA"))) ;
   CPPUNIT_LOG_ASSERT(less_tuple<lt>(single<std::string>("ABB"), single<std::string>("BAA"))) ;

   CPPUNIT_LOG_ASSERT(less_tuple<lt>(triple<int>(5, 10, 15), triple<int>(5, 10, 16))) ;
   CPPUNIT_LOG_IS_FALSE(less_tuple<lt>(triple<int>(5, 10, 15), triple<int>(5, 10, 15))) ;
   CPPUNIT_LOG_IS_FALSE(less_tuple<lt>(triple<int>(5, 10, 16), triple<int>(5, 10, 15))) ;
   CPPUNIT_LOG_ASSERT(less_tuple<lt>(triple<int>(5, 9, 16), triple<int>(5, 10, 15))) ;

   CPPUNIT_LOG_ASSERT(std::equal_to<std::tuple<>>()(std::tuple<>(), std::tuple<>())) ;
   CPPUNIT_LOG_ASSERT(equal_tuple<eq>(unipair<int>(5, 10), unipair<int>(5, 10))) ;

   CPPUNIT_LOG_IS_FALSE(equal_tuple<eq>(unipair<int>(5, 9), unipair<int>(5, 10))) ;
   CPPUNIT_LOG_IS_FALSE(equal_tuple<eq>(unipair<int>(4, 15), unipair<int>(5, 10))) ;
   CPPUNIT_LOG_IS_FALSE(equal_tuple<eq>(unipair<int>(5, 11), unipair<int>(5, 10))) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(equal_tuple<eq>(single<std::string>("BBB"), single<std::string>("BBB"))) ;
   CPPUNIT_LOG_IS_FALSE(equal_tuple<eq>(single<std::string>("BBB"), single<std::string>("BAA"))) ;
   CPPUNIT_LOG_IS_FALSE(equal_tuple<eq>(single<std::string>("ABB"), single<std::string>("BAA"))) ;

   CPPUNIT_LOG_ASSERT(equal_tuple<eq>(triple<std::string>("ABB", "ABB", "ABB"),
                                      triple<std::string>("ABB", "ABB", "ABB"))) ;
   CPPUNIT_LOG_IS_FALSE(equal_tuple<eq>(triple<std::string>("BAB", "ABB", "ABB"),
                                        triple<std::string>("ABB", "ABB", "ABB"))) ;
   CPPUNIT_LOG_IS_FALSE(equal_tuple<eq>(triple<std::string>("ABB", "BAB", "ABB"),
                                        triple<std::string>("ABB", "ABB", "ABB"))) ;
   CPPUNIT_LOG_IS_FALSE(equal_tuple<eq>(triple<std::string>("ABB", "ABB", "BAB"),
                                        triple<std::string>("ABB", "ABB", "ABB"))) ;

   CPPUNIT_LOG_ASSERT(equal_tuple<eq>(std::tuple<std::string, std::string>("ABB", "ABB"),
                                      std::tuple<std::string, std::string>("ABB", "ABB"))) ;
   CPPUNIT_LOG_IS_FALSE(equal_tuple<eq>(std::tuple<std::string, std::string>("BAB", "ABB"),
                                        std::tuple<std::string, std::string>("ABB", "ABB"))) ;
   CPPUNIT_LOG_IS_FALSE(equal_tuple<eq>(std::tuple<std::string, std::string>("ABB", "BAB"),
                                        std::tuple<std::string, std::string>("ABB", "ABB"))) ;
}

void UtilityTests::Test_StreamUtils()
{
   typedef imemstream::traits_type traits_type ;

   imemstream empty_imems ;
   CPPUNIT_LOG_ASSERT(empty_imems) ;
   CPPUNIT_LOG_ASSERT(empty_imems.eof()) ;
   CPPUNIT_LOG_EQ(empty_imems.get(), traits_type::eof()) ;
   CPPUNIT_LOG_IS_FALSE(empty_imems) ;
   CPPUNIT_LOG_ASSERT(empty_imems.reset()) ;
   CPPUNIT_LOG_ASSERT(empty_imems.eof()) ;

   CPPUNIT_LOG(std::endl) ;
   const char hello[] =
      "Hello 12 15\n"
      "Bye, baby!\n" ;
   imemstream imems1 (hello, sizeof hello - 1) ;
   std::string str ;
   std::string str2 ;
   int i = 0 ;

   CPPUNIT_LOG_ASSERT(imems1) ;
   CPPUNIT_LOG_IS_FALSE(imems1.eof()) ;
   CPPUNIT_LOG_EQ(imems1.get(), 'H') ;
   CPPUNIT_LOG_ASSERT(imems1) ;
   CPPUNIT_LOG_IS_FALSE(imems1.eof()) ;
   CPPUNIT_LOG_EQ(imems1.get(), 'e') ;
   CPPUNIT_LOG_ASSERT(imems1 >> str) ;
   CPPUNIT_LOG_EQ(str, "llo") ;
   CPPUNIT_LOG_ASSERT(imems1 >> i) ;
   CPPUNIT_LOG_EQ(i, 12) ;
   CPPUNIT_LOG_IS_FALSE(imems1.eof()) ;
   CPPUNIT_LOG_ASSERT(imems1.reset()) ;
   CPPUNIT_LOG_ASSERT(imems1) ;
   CPPUNIT_LOG_IS_FALSE(imems1.eof()) ;

   CPPUNIT_LOG_ASSERT(std::getline(imems1, str)) ;
   CPPUNIT_LOG_EQ(str, "Hello 12 15") ;
   CPPUNIT_LOG_ASSERT(std::getline(imems1, str2)) ;
   CPPUNIT_LOG_EQ(str2, "Bye, baby!") ;
   CPPUNIT_LOG_IS_FALSE(imems1.eof()) ;
   CPPUNIT_LOG_EQ(imems1.get(), traits_type::eof()) ;
   CPPUNIT_LOG_ASSERT(imems1.eof()) ;
   CPPUNIT_LOG_IS_FALSE(imems1) ;
   CPPUNIT_LOG_ASSERT(imems1.reset()) ;
   CPPUNIT_LOG_ASSERT(imems1) ;
   CPPUNIT_LOG_IS_FALSE(imems1.eof()) ;
   CPPUNIT_LOG_EQ(imems1.get(), 'H') ;

   CPPUNIT_LOG(std::endl) ;
   omemstream omems1 ;
   CPPUNIT_LOG_ASSERT(omems1) ;
   CPPUNIT_LOG_EQ(omems1.str().size(), 0) ;
   CPPUNIT_LOG_EQUAL(omems1.checkout(), std::string()) ;
   CPPUNIT_LOG_ASSERT(omems1) ;
   CPPUNIT_LOG_EQ(omems1.str().size(), 0) ;
   CPPUNIT_LOG_ASSERT(omems1 << 2 << ' ' << 3) ;
   CPPUNIT_LOG_EQ(omems1.str(), "2 3") ;
   CPPUNIT_LOG_EQUAL(omems1.checkout(), std::string("2 3")) ;
   CPPUNIT_LOG_ASSERT(omems1) ;
   CPPUNIT_LOG_EQ(omems1.str().size(), 0) ;
   CPPUNIT_LOG_ASSERT(omems1 << std::string('A', 50) + "\n" + std::string('b', 50)+ "\n" + std::string('C', 50)) ;
   CPPUNIT_LOG_EQ(omems1.str(), std::string('A', 50) + "\n" + std::string('b', 50)+ "\n" + std::string('C', 50)) ;
   CPPUNIT_LOG_EQUAL(omems1.checkout(), std::string('A', 50) + "\n" + std::string('b', 50)+ "\n" + std::string('C', 50)) ;
   CPPUNIT_LOG_ASSERT(omems1) ;
   CPPUNIT_LOG_EQ(omems1.str().size(), 0) ;

   CPPUNIT_LOG(std::endl) ;
   imemstream imems2 (hello) ;
   std::string line01 ;
   CPPUNIT_LOG_ASSERT(std::getline(imems2, line01)) ;
   CPPUNIT_LOG_EQ(line01, "Hello 12 15") ;
   CPPUNIT_LOG_ASSERT(std::getline(imems2, line01)) ;
   CPPUNIT_LOG_EQ(line01, "Bye, baby!") ;
   CPPUNIT_LOG_IS_FALSE(std::getline(imems2, line01)) ;
}

void UtilityTests::Test_String_Cast()
{
   CPPUNIT_LOG_EQUAL(string_cast("Hello!"), std::string("Hello!")) ;
   CPPUNIT_LOG_EQUAL(string_cast(std::string("Hello!")), std::string("Hello!")) ;

   CPPUNIT_LOG_EQ(string_cast(20), "20") ;
   CPPUNIT_LOG_EQ(string_cast("Hello, ", 20), "Hello, 20") ;
   CPPUNIT_LOG_EQ(string_cast("Hello, ", 1, 2, std::string("3")), "Hello, 123") ;

   std::ostringstream os ;
   CPPUNIT_LOG_ASSERT(print_values(os)) ;
   CPPUNIT_LOG_EQ(os.str(), "") ;
   CPPUNIT_LOG_ASSERT(print_values(os, '(', 10, ',', 0.25, ")")) ;
   CPPUNIT_LOG_EQ(os.str(), "(10,0.25)") ;
}

void UtilityTests::Test_Underlying_Int()
{
   enum A : char { N = 'A', M = 'B' } ;
   enum B : unsigned long long { NU = 500, MU = 100 } ;
   enum class C : int8_t { NC = -127, MC = 127 } ;

   CPPUNIT_LOG_EQUAL(underlying_int(N), 'A') ;
   CPPUNIT_LOG_EQUAL(underlying_int(M), 'B') ;
   CPPUNIT_LOG_EQUAL(underlying_int('Z'), 'Z') ;

   CPPUNIT_LOG_EQUAL(underlying_int(NU), 500ULL) ;
   CPPUNIT_LOG_EQUAL(underlying_int(MU), 100ULL) ;
   CPPUNIT_LOG_EQUAL(underlying_int(1024ULL), 1024ULL) ;

   CPPUNIT_LOG_EQUAL(underlying_int(C::NC), (int8_t)-127) ;
   CPPUNIT_LOG_EQUAL(underlying_int(C::MC), (int8_t)127) ;
   CPPUNIT_LOG_EQUAL(underlying_int((int8_t)-1), (int8_t)-1) ;

   CPPUNIT_LOG_EQUAL(underlying_int(true), true) ;
}

struct test_add {
      template<typename T>
      constexpr T operator()(T x, T y) const { return x + y ; }
} ;

void UtilityTests::Test_Folding()
{
   CPPUNIT_LOG_EQUAL((std::integral_constant<unsigned, fold_bitor(10U)>::value), 10U) ;
   CPPUNIT_LOG_EQUAL((std::integral_constant<int, fold_bitor(16, 64)>::value), 80) ;
   CPPUNIT_LOG_EQUAL((std::integral_constant<int, fold_bitor(16, 64, 17)>::value), 81) ;

   CPPUNIT_LOG_EQUAL((std::integral_constant<short, fold_left(test_add(), (short)5)>::value), (short)5) ;
   CPPUNIT_LOG_EQUAL((std::integral_constant<unsigned, fold_left(test_add(), 10, 20)>::value), 30U) ;
   CPPUNIT_LOG_EQUAL(fold_left(test_add(), 10U, 20U), 30U) ;

   CPPUNIT_LOG_ASSERT(one_of<15>::is(15)) ;
   CPPUNIT_LOG_IS_FALSE(one_of<0>::is(15)) ;
   CPPUNIT_LOG_ASSERT(one_of<0>::is(0)) ;
   CPPUNIT_LOG_ASSERT((one_of<7, 3, 12>::is(3))) ;
   CPPUNIT_LOG_IS_FALSE((one_of<7, 3, 12>::is(1))) ;

   enum class C : uint8_t { N1, N2, N3 = 40, N4, N5 } ;

   CPPUNIT_LOG_IS_FALSE(is_in<int>(3)) ;
   CPPUNIT_LOG_IS_FALSE(is_in<int>(0)) ;

   CPPUNIT_LOG_ASSERT((is_in<int, 7, 3, 12>(3))) ;
   CPPUNIT_LOG_ASSERT((bool_constant<is_in<int, 7, 3, 12>(3)>::value)) ;
   CPPUNIT_LOG_IS_FALSE((bool_constant<is_in<int, 7, 3, 12>(4)>::value)) ;

   CPPUNIT_LOG_ASSERT((is_in<C, C::N2, C::N5>(C::N2))) ;
   CPPUNIT_LOG_IS_FALSE((is_in<C>(C::N2))) ;
   CPPUNIT_LOG_ASSERT((is_in<C, C::N2>(C::N2))) ;
   CPPUNIT_LOG_IS_FALSE((is_in<C, C::N5, C::N3, C::N1>(C::N2))) ;
   CPPUNIT_LOG_ASSERT((is_in<C, C::N5, C::N3, C::N1>(C::N3))) ;

   CPPUNIT_LOG_ASSERT((is_in(C::N3, C::N5, C::N3))) ;
   CPPUNIT_LOG_IS_FALSE((is_in(C::N3, C::N5, C::N1))) ;

   C cc = C::N1 ;
   CPPUNIT_LOG_ASSERT((is_in(cc, C::N5, C::N1))) ;

   cc = C::N4 ;
   CPPUNIT_LOG_IS_FALSE((is_in(cc, C::N5, C::N1))) ;
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
   return pcomn::unit::run_tests
      <
         UtilityTests,
         UnittestTests
      >
      (argc, argv, "unittest.diag.ini", "Tests of various stuff from pcomn_utils.h") ;
}
