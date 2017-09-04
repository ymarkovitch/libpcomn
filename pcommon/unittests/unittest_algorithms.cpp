/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_algorithms.cpp
 COPYRIGHT    :   Yakov Markovitch, 2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for container algorithms, etc.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   10 Mar 2015
*******************************************************************************/
#include <pcomn_unittest.h>

#include <pcomn_calgorithm.h>
#include <pcomn_algorithm.h>

#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

/*******************************************************************************
                     class AlgorithmsTests
*******************************************************************************/
class AlgorithmsTests : public CppUnit::TestFixture {
   private:
      void Test_Adjacent_Coalesce() ;

      CPPUNIT_TEST_SUITE(AlgorithmsTests) ;

      CPPUNIT_TEST(Test_Adjacent_Coalesce) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

/*******************************************************************************
                     class CAlgorithmsTests
*******************************************************************************/
class CAlgorithmsTests : public CppUnit::TestFixture {
   private:
      void Test_OSequence() ;
      void Test_MakeContainer() ;
      void Test_GetKeyedValue() ;

      CPPUNIT_TEST_SUITE(CAlgorithmsTests) ;

      CPPUNIT_TEST(Test_OSequence) ;
      CPPUNIT_TEST(Test_MakeContainer) ;
      CPPUNIT_TEST(Test_GetKeyedValue) ;

      CPPUNIT_TEST_SUITE_END() ;
   protected:
      const std::vector<std::string>   strvec {"zero", "one", "two", "three"} ;
      const std::vector<int>           intvec {1, 3, 5, 7, 11} ;
} ;

/*******************************************************************************
 AlgorithmsTests
*******************************************************************************/
void AlgorithmsTests::Test_Adjacent_Coalesce()
{
   using namespace pcomn ;
   typedef unipair<int> int_range ;

   const auto adj = [](const int_range &x, const int_range &y)
   {
      return x.second >= y.first ;
   } ;

   const auto merge_intrv = [](const int_range &x, const int_range &y)
   {
      return int_range{x.first, std::max(x.second, y.second)} ;
   } ;

   std::vector<int_range> v1
   {{10, 20}, {21, 30}, {21, 30}, {25, 27}, {29, 35}, {40, 45}, {50, 55}} ;

   CPPUNIT_LOG_ASSERT(adjacent_coalesce(v1.begin(), v1.begin(), adj, merge_intrv) == v1.begin()) ;
   auto result = adjacent_coalesce(v1.begin(), v1.end(), adj, merge_intrv) ;

   CPPUNIT_LOG_EXPRESSION(v1) ;
   CPPUNIT_LOG_EQ(result - v1.begin(), 4) ;
   CPPUNIT_LOG_EQUAL((std::vector<int_range>(v1.begin(), result)),
                     (std::vector<int_range>{{10, 20}, {21, 35}, {40, 45}, {50, 55}})) ;

   std::vector<int_range> v2 {{10, 20}} ;
   CPPUNIT_LOG_EXPRESSION(v2) ;
   CPPUNIT_LOG_EQUAL(truncate_container(v2, adjacent_coalesce(v2.begin(), v2.end(), adj, merge_intrv)),
                     (std::vector<int_range>{{10, 20}})) ;

   std::vector<int_range> v3 {{10, 20}, {25, 27}} ;
   CPPUNIT_LOG_EXPRESSION(v3) ;
   CPPUNIT_LOG_EQUAL(truncate_container(v3, adjacent_coalesce(v3.begin(), v3.end(), adj, merge_intrv)),
                     (std::vector<int_range>{{10, 20}, {25, 27}})) ;

   std::vector<int_range> v4 {{10, 20}, {15, 27}} ;
   CPPUNIT_LOG_EXPRESSION(v4) ;
   CPPUNIT_LOG_EQUAL(truncate_container(v4, adjacent_coalesce(v4.begin(), v4.end(), adj, merge_intrv)),
                     (std::vector<int_range>{{10, 27}})) ;

   std::vector<int_range> v5 {{10, 20}, {25, 30}, {27, 33}} ;
   CPPUNIT_LOG_EXPRESSION(v5) ;
   CPPUNIT_LOG_EQUAL(truncate_container(v5, adjacent_coalesce(v5.begin(), v5.end(), adj, merge_intrv)),
                     (std::vector<int_range>{{10, 20}, {25, 33}})) ;
}

/*******************************************************************************
 CAlgorithmsTests
*******************************************************************************/
void CAlgorithmsTests::Test_OSequence()
{
   CPPUNIT_LOG_ASSERT(pcomn::both_ends(strvec) ==
                      pcomn::unipair<std::vector<std::string>::const_iterator>(strvec.begin(), strvec.end())) ;
}

void CAlgorithmsTests::Test_MakeContainer()
{
   using namespace pcomn ;
   using namespace std::placeholders ;
   const int v1[] = {2, 4, 6} ;

   CPPUNIT_LOG_EQUAL(make_container<std::vector<unsigned>>(std::begin(v1), std::end(v1), [](int x) { return 3*x ; }),
                  (std::vector<unsigned>{6, 12, 18})) ;

   CPPUNIT_LOG_EQUAL(make_container<std::vector<unsigned>>(std::begin(v1), std::end(v1), std::bind(std::plus<int>(), _1, 10)),
                     (std::vector<unsigned>{12, 14, 16})) ;
}

void CAlgorithmsTests::Test_GetKeyedValue()
{
   using namespace pcomn ;

   const std::set<int> iset_0 ;
   const std::set<int> iset_1 {-5, 12, 1} ;

   CPPUNIT_LOG_EQUAL(get_keyed_value(iset_0, 1), 0) ;
   CPPUNIT_LOG_EQUAL(get_keyed_value(iset_0, 0), 0) ;
   CPPUNIT_LOG_EQUAL(get_keyed_value(iset_0, 0, 20), 20) ;

   CPPUNIT_LOG_EQUAL(get_keyed_value(iset_1, 1), 1) ;
   CPPUNIT_LOG_EQUAL(get_keyed_value(iset_1, 1, 20), 1) ;
   CPPUNIT_LOG_EQUAL(get_keyed_value(iset_1, -1, 20), 20) ;

   CPPUNIT_LOG(std::endl) ;

   const std::set<std::string> sset_0 ;
   const std::set<std::string> sset_1 {"", "hello", "world"} ;

   CPPUNIT_LOG_EQUAL(get_keyed_value(sset_0, " "), std::string()) ;
   CPPUNIT_LOG_EQUAL(get_keyed_value(sset_0, ""), std::string()) ;
   CPPUNIT_LOG_EQUAL(get_keyed_value(sset_0, "", "bye"), std::string("bye")) ;

   CPPUNIT_LOG_EQUAL(get_keyed_value(sset_1, " "), std::string()) ;
   CPPUNIT_LOG_EQUAL(get_keyed_value(sset_1, " ", "bye"), std::string("bye")) ;
   CPPUNIT_LOG_EQUAL(get_keyed_value(sset_1, "", "bye"), std::string()) ;
   CPPUNIT_LOG_EQUAL(get_keyed_value(sset_1, "hello", "bye"), std::string("hello")) ;
   CPPUNIT_LOG_EQUAL(get_keyed_value(sset_1, "world"), std::string("world")) ;

   std::map<std::string, std::string> smap_1 {{"hello", "world"}, {"bye", "baby"}} ;

   CPPUNIT_LOG_EQUAL(get_keyed_value(smap_1, " "), std::string()) ;
   CPPUNIT_LOG_EQUAL(get_keyed_value(smap_1, " ", "foo"), std::string("foo")) ;
   CPPUNIT_LOG_EQUAL(get_keyed_value(smap_1, "hello", "bye"), std::string("world")) ;
   CPPUNIT_LOG_EQUAL(get_keyed_value(smap_1, "hello"), std::string("world")) ;
   CPPUNIT_LOG_EQ(smap_1.count("bye"), 1) ;

   std::string value ;
   CPPUNIT_LOG_ASSERT(erase_keyed_value(smap_1, "bye", value)) ;
   CPPUNIT_LOG_EQUAL(value, std::string("baby")) ;
   CPPUNIT_LOG_EQ(smap_1.count("bye"), 0) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(AlgorithmsTests::suite()) ;
   runner.addTest(CAlgorithmsTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv, "unittest.trace.ini",
                             "Testing algorithms") ;
}
