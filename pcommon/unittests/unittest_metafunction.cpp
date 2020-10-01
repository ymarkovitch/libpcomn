/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_metafunction.cpp
 COPYRIGHT    :   Yakov Markovitch, 2007-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittest for pcomn_meta

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   28 Nov 2006
*******************************************************************************/
#include <pcomn_meta.h>
#include <pcomn_unittest.h>

#include <algorithm>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <vector>

#include <string.h>

using namespace pcomn ;

template<typename T>
using is_double = std::is_same<T, double> ;

template<typename T, typename U>
using is_same_valtype = std::is_same<pcomn::valtype_t<T>, pcomn::valtype_t<U> > ;

template<typename T>
using is_double_val = is_same_valtype<T, double> ;

class MetafunctionTests : public CppUnit::TestFixture {

      void Test_Has_Dep_Type() ;
      void Test_Ensure_Arg() ;
      void Test_Count_Types() ;
      void Test_Rebind_Contaner() ;
      void Test_Transfer_CV() ;
      void Test_Pointer_Rank() ;
      void Test_Pointer_CVV() ;

      CPPUNIT_TEST_SUITE(MetafunctionTests) ;

      CPPUNIT_TEST(Test_Has_Dep_Type) ;
      CPPUNIT_TEST(Test_Ensure_Arg) ;
      CPPUNIT_TEST(Test_Count_Types) ;
      CPPUNIT_TEST(Test_Rebind_Contaner) ;
      CPPUNIT_TEST(Test_Transfer_CV) ;
      CPPUNIT_TEST(Test_Pointer_Rank) ;
      CPPUNIT_TEST(Test_Pointer_CVV) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

/*******************************************************************************
 MetafunctionTests
*******************************************************************************/
void MetafunctionTests::Test_Has_Dep_Type()
{
   CPPUNIT_LOG_ASSERT((pcomn::has_key_type<std::map<std::string, int> >::value)) ;
   CPPUNIT_LOG_ASSERT((pcomn::has_key_type<const std::map<std::string, int> >::value)) ;
   CPPUNIT_LOG_ASSERT((pcomn::has_const_iterator<std::map<std::string, int> >::value)) ;

   CPPUNIT_LOG_IS_FALSE((pcomn::has_key_type<std::vector<std::string> >::value)) ;
}

void MetafunctionTests::Test_Ensure_Arg()
{
   const std::unique_ptr<int> &uptr_cref = std::unique_ptr<int>() ;
   std::unique_ptr<int> uptr ;
   std::unique_ptr<int> &uptr_ref = uptr ;

   CPPUNIT_LOG_ASSERT((std::is_reference<decltype(PCOMN_ENSURE_ARG(uptr_cref))>::value)) ;
   CPPUNIT_LOG_ASSERT((std::is_reference<decltype(PCOMN_ENSURE_ARG(uptr))>::value)) ;
   CPPUNIT_LOG_ASSERT((std::is_reference<decltype(PCOMN_ENSURE_ARG(uptr_ref))>::value)) ;

   CPPUNIT_LOG_ASSERT((std::is_const<std::remove_reference<decltype(PCOMN_ENSURE_ARG(uptr_cref))>::type>::value)) ;
   CPPUNIT_LOG_IS_FALSE((std::is_const<std::remove_reference<decltype(PCOMN_ENSURE_ARG(uptr))>::type>::value)) ;
   CPPUNIT_LOG_IS_FALSE((std::is_const<std::remove_reference<decltype(PCOMN_ENSURE_ARG(uptr_ref))>::type>::value)) ;
}

void MetafunctionTests::Test_Count_Types()
{
   using namespace std ;
   CPPUNIT_LOG_EQ(count_types_if<is_double>::value, 0) ;
   CPPUNIT_LOG_EQ((count_types_if<is_double, int>::value), 0) ;
   CPPUNIT_LOG_EQ((count_types_if<is_double, double>::value), 1) ;
   CPPUNIT_LOG_EQ((count_types_if<is_double, double, double>::value), 2) ;
   CPPUNIT_LOG_EQ((count_types_if<is_double, int, double>::value), 1) ;
   CPPUNIT_LOG_EQ((count_types_if<is_double, const double, double, int>::value), 1) ;
   CPPUNIT_LOG_EQ((count_types_if<is_double, const double, double, int, double>::value), 2) ;
   CPPUNIT_LOG_EQ((count_types_if<is_double_val, const double, double, int, double>::value), 3) ;
}

void MetafunctionTests::Test_Rebind_Contaner()
{
   PCOMN_STATIC_CHECK(std::is_same<std::vector<int>, rebind_t<std::vector<double>, int> >::value) ;
   PCOMN_STATIC_CHECK(std::is_same<std::map<std::string, int>, rebind_t<std::map<double, char>, std::string, int> >::value) ;
}

void MetafunctionTests::Test_Transfer_CV()
{
   CPPUNIT_LOG_ASSERT((std::is_same_v<transfer_cv_t<int, double>, double>)) ;
   CPPUNIT_LOG_ASSERT((std::is_same_v<transfer_cv_t<const int, double>, const double>)) ;
   CPPUNIT_LOG_IS_FALSE((std::is_same_v<transfer_cv_t<const int, double>, double>)) ;
   CPPUNIT_LOG_IS_FALSE((std::is_same_v<transfer_cv_t<const int, double>, volatile double>)) ;

   CPPUNIT_LOG_ASSERT((std::is_same_v<transfer_cv_t<volatile int, double>, volatile double>)) ;
   CPPUNIT_LOG_ASSERT((std::is_same_v<transfer_cv_t<volatile int, volatile double>, volatile double>)) ;
   CPPUNIT_LOG_ASSERT((std::is_same_v<transfer_cv_t<const int, volatile double>, const volatile double>)) ;
   CPPUNIT_LOG_ASSERT((std::is_same_v<transfer_cv_t<volatile int, const double>, const volatile double>)) ;
   CPPUNIT_LOG_ASSERT((std::is_same_v<transfer_cv_t<const volatile int, const double>, const volatile double>)) ;
   CPPUNIT_LOG_ASSERT((std::is_same_v<transfer_cv_t<const volatile int, double>, const volatile double>)) ;

   CPPUNIT_LOG_IS_FALSE((std::is_same_v<transfer_cv_t<volatile int, double>, double>)) ;
   CPPUNIT_LOG_IS_FALSE((std::is_same_v<transfer_cv_t<volatile int, double>, const double>)) ;
}

template<typename>
struct pointer_rank : std::integral_constant<size_t,0> {} ;

template<typename T>
struct pointer_rank<T const> : pointer_rank<T> {} ;
template<typename T>
struct pointer_rank<T volatile> : pointer_rank<T> {} ;
template<typename T>
struct pointer_rank<T const volatile> : pointer_rank<T> {} ;

template<typename T>
struct pointer_rank<T*> : std::integral_constant<size_t,pointer_rank<T>::value+1> {} ;

template<typename T>
constexpr size_t pointer_rank_v = pointer_rank<T>::value ;

template<typename T>
struct pointer_cvv { static constexpr unsigned _() { return 0 ; } } ;

template<typename T>
struct pointer_cvv<T*const> : pointer_cvv<T*> {} ;
template<typename T>
struct pointer_cvv<T*volatile> : pointer_cvv<T*> {} ;
template<typename T>
struct pointer_cvv<T*const volatile> : pointer_cvv<T*> {} ;

template<typename T>
struct pointer_cvv<T*> :
   std::integral_constant<unsigned, (std::is_const_v<T> << 1U)|std::is_volatile_v<T>|(pointer_cvv<T>::_() << 2U)>
{
      static constexpr unsigned _() { return pointer_cvv::value ; }
} ;

template<typename T>
constexpr auto pointer_cvv_v = pointer_cvv<T>::value ;

void MetafunctionTests::Test_Pointer_Rank()
{
   CPPUNIT_LOG_EQ(pointer_rank_v<int>, 0) ;
   CPPUNIT_LOG_EQ(pointer_rank_v<int*>, 1) ;
   CPPUNIT_LOG_EQ(pointer_rank_v<const int**>, 2) ;
   CPPUNIT_LOG_EQ(pointer_rank_v<int* const *>, 2) ;
   CPPUNIT_LOG_EQ(pointer_rank_v<int* const volatile *>, 2) ;
   CPPUNIT_LOG_EQ(pointer_rank_v<const int* volatile *>, 2) ;
   CPPUNIT_LOG_EQ(pointer_rank_v<void***>, 3) ;
   CPPUNIT_LOG_EQ(pointer_rank_v<void>, 0) ;
}

void MetafunctionTests::Test_Pointer_CVV()
{
   CPPUNIT_LOG_EQUAL(pointer_cvv_v<int*>, 0b00u) ;
   CPPUNIT_LOG_EQUAL(pointer_cvv_v<const int*>, 0b10u) ;
   CPPUNIT_LOG_EQUAL(pointer_cvv_v<volatile int*>, 0b01u) ;
   CPPUNIT_LOG_EQUAL(pointer_cvv_v<const volatile int*>, 0b11u) ;

   CPPUNIT_LOG_EQUAL(pointer_cvv_v<void**>, 0U) ;
   CPPUNIT_LOG_EQUAL(pointer_cvv_v<void**const>, 0U) ;
   CPPUNIT_LOG_EQUAL(pointer_cvv_v<const void**const>, 0b1000u) ;

   CPPUNIT_LOG_EQUAL(pointer_cvv_v<void***>, 0U) ;
   CPPUNIT_LOG_EQUAL(pointer_cvv_v<const void***>, 0b100000u) ;
   CPPUNIT_LOG_EQUAL(pointer_cvv_v<void* const**>, 0b001000u) ;
   CPPUNIT_LOG_EQUAL(pointer_cvv_v<void**const*>, 0b000010u) ;

   CPPUNIT_LOG_EQUAL(pointer_cvv_v<const volatile double***>, 0b110000u) ;
   CPPUNIT_LOG_EQUAL(pointer_cvv_v<volatile void* const**>, 0b011000u) ;
   CPPUNIT_LOG_EQUAL(pointer_cvv_v<volatile void* const volatile* const*>, 0b011110u) ;
}

/*******************************************************************************
 main
*******************************************************************************/
int main(int argc, char *argv[])
{
   return pcomn::unit::run_tests
       <
           MetafunctionTests
       >
       (argc, argv) ;
}
