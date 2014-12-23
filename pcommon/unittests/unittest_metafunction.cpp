/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_metafunction.cpp
 COPYRIGHT    :   Yakov Markovitch, 2007-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittest for pcomn_metafunction

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   28 Nov 2006
*******************************************************************************/
#include <pcomn_metafunctional.h>
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

class MetafunctionTests : public CppUnit::TestFixture {

      void Test_Static_Fill() ;
      void Test_Has_Dep_Type() ;
      void Test_Ensure_Arg() ;
      void Test_Count_Types() ;

      CPPUNIT_TEST_SUITE(MetafunctionTests) ;

      CPPUNIT_TEST(Test_Static_Fill) ;
      CPPUNIT_TEST(Test_Has_Dep_Type) ;
      CPPUNIT_TEST(Test_Ensure_Arg) ;
      CPPUNIT_TEST(Test_Count_Types) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

/*******************************************************************************
 MetafunctionTests
*******************************************************************************/
template<unsigned sz>
struct TestBuf {
      TestBuf(int init = 0)
      {
         memset(data, init, sizeof data) ;
      }
      int check(int pattern) const
      {
         const char *iter = data ;
         return
            *iter != pattern ||
            (iter = std::adjacent_find(iter, data + sizeof data, std::not_equal_to<char>())) != data + sizeof data ?
            iter - data : -1 ;
      }
      char data[sz] ;
} ;

void MetafunctionTests::Test_Static_Fill()
{
#define CHECKFILL(sz)                                                   \
   {                                                                    \
      TestBuf<sz> TestBuf##sz ;                                         \
      CPPUNIT_LOG_EQUAL(pcomn::static_fill<-1>(&TestBuf##sz)->check(-1), -1) ; \
      TestBuf<sz> TestBuf_C##sz ;                                       \
      CPPUNIT_LOG_EQUAL((int)*(pcomn::static_fill<-1>(TestBuf_C##sz.data) + (sz-1)), -1) ; \
   }
   CHECKFILL(1) ;
   CHECKFILL(2) ;
   CHECKFILL(3) ;
   CHECKFILL(4) ;
   CHECKFILL(5) ;
   CHECKFILL(6) ;
   CHECKFILL(7) ;
   CHECKFILL(8) ;
   CHECKFILL(9) ;
   CHECKFILL(11) ;
   CHECKFILL(15) ;
   CHECKFILL(16) ;
   CHECKFILL(29) ;
}

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

template<typename T>
using is_double = std::is_same<T, double> ;

template<typename T, typename U>
using is_same_valtype = std::is_same<pcomn::valtype_t<T>, pcomn::valtype_t<U> > ;

template<typename T>
using is_double_val = is_same_valtype<T, double> ;

void MetafunctionTests::Test_Count_Types()
{
   using namespace pcomn ;
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

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(MetafunctionTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "unittest.diag.ini", "pcomn_metafunction tests") ;
}
