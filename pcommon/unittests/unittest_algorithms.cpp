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

      CPPUNIT_TEST_SUITE(CAlgorithmsTests) ;

      CPPUNIT_TEST(Test_OSequence) ;

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

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(AlgorithmsTests::suite()) ;
   runner.addTest(CAlgorithmsTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv, "unittest.trace.ini",
                             "Testing algorithms") ;
}
