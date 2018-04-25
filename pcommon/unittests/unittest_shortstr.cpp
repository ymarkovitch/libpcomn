/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_shortstr.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2017. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittest for pcomn::short_str<> template class

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   23 Nov 2006
*******************************************************************************/
#include <pcomn_shortstr.h>
#include <pcomn_unittest.h>
#include <vector>
#include <set>
#include <list>

using namespace pcomn ;

class ShortStringTests : public CppUnit::TestFixture {

      void Test_Constructors_Invariants() ;
      void Test_Comparison() ;
      void Test_Assignment() ;
      void Test_Charrepr_Function() ;

      CPPUNIT_TEST_SUITE(ShortStringTests) ;

      CPPUNIT_TEST(Test_Constructors_Invariants) ;
      CPPUNIT_TEST(Test_Comparison) ;
      CPPUNIT_TEST(Test_Assignment) ;
      CPPUNIT_TEST(Test_Charrepr_Function) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

/*******************************************************************************
 ShortStringTests
*******************************************************************************/
void ShortStringTests::Test_Constructors_Invariants()
{
   short_string<10> S10 ;
   const char * const EmptySptr = "" ;
   const std::string EmptyStd  ;

   CPPUNIT_LOG_EQUAL(S10.size(), (size_t)0) ;
   CPPUNIT_LOG_ASSERT(S10.empty()) ;
   CPPUNIT_LOG_EQUAL(S10.capacity(), (size_t)10) ;
   CPPUNIT_LOG_EQUAL(S10.begin(), S10.end()) ;
   CPPUNIT_LOG_ASSERT(S10.rbegin() == S10.rend()) ;
   CPPUNIT_LOG_EQUAL((int)*S10.end(), 0) ;

   CPPUNIT_LOG_ASSERT(S10 == EmptySptr) ;
   CPPUNIT_LOG_IS_FALSE(S10 != EmptySptr) ;
   CPPUNIT_LOG_IS_FALSE(S10 < EmptySptr) ;
   CPPUNIT_LOG_IS_FALSE(S10 > EmptySptr) ;

   short_string<3> SOther ("Hello") ;
   const std::string HelStr ("Hel")   ;
   const std::string HelloStr ("Hello")   ;

   CPPUNIT_LOG_EQUAL(SOther.size(), (size_t)3) ;
   CPPUNIT_LOG_IS_FALSE(SOther.empty()) ;
   CPPUNIT_LOG_EQUAL(SOther.capacity(), (size_t)3) ;
   CPPUNIT_LOG_EQUAL(SOther.end() - SOther.begin(), (ptrdiff_t)3) ;
   CPPUNIT_LOG_EQUAL((int)*SOther.end(), 0) ;

   CPPUNIT_LOG_ASSERT(SOther == HelStr) ;
   CPPUNIT_LOG_ASSERT(SOther <= HelStr) ;
   CPPUNIT_LOG_ASSERT(SOther >= HelStr) ;
   CPPUNIT_LOG_IS_FALSE(SOther != HelStr) ;
   CPPUNIT_LOG_IS_FALSE(SOther < HelStr) ;
   CPPUNIT_LOG_IS_FALSE(SOther > HelStr) ;

   CPPUNIT_LOG_IS_FALSE(SOther == HelloStr) ;
   CPPUNIT_LOG_ASSERT(SOther != HelloStr) ;
   CPPUNIT_LOG_ASSERT(SOther < HelloStr) ;
   CPPUNIT_LOG_IS_FALSE(SOther > HelloStr) ;

   CPPUNIT_LOG(SOther << std::endl) ;
   CPPUNIT_LOG_EQUAL(std::string((SOther = "Bye!").c_str()), std::string("Bye")) ;
   CPPUNIT_LOG(SOther << std::endl) ;
   CPPUNIT_LOG_EQUAL(std::string((SOther = "By").c_str()), std::string("By")) ;
   CPPUNIT_LOG(SOther << std::endl) ;
   CPPUNIT_LOG_EQUAL(SOther.size(), (size_t)2) ;
   CPPUNIT_LOG_EQUAL(SOther.capacity(), (size_t)3) ;
}

void ShortStringTests::Test_Comparison()
{
}

void ShortStringTests::Test_Assignment()
{
}

void ShortStringTests::Test_Charrepr_Function()
{
   typedef short_string<7> charrepr_t ;
   CPPUNIT_LOG_EQUAL(charrepr('A'), charrepr_t("'A'")) ;
   CPPUNIT_LOG_EQUAL(charrepr('\''), charrepr_t("'\\''")) ;
   CPPUNIT_LOG_EQUAL(charrepr('\\'), charrepr_t("'\\\\'")) ;
   CPPUNIT_LOG_EQUAL(charrepr(0), charrepr_t("'\\x00'")) ;
   CPPUNIT_LOG_EQUAL(charrepr(255), charrepr_t("'\\xFF'")) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(ShortStringTests::suite()) ;
   return pcomn::unit::run_tests(runner, argc, argv, "unittest.diag.ini",
                                 "Short string tests.") ;
}
