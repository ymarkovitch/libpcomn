/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_indicator_observer.cpp
 COPYRIGHT    :   Yakov Markovitch, 2006-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unit tests of change_indicator/change_observer,
                  multi_indicator<>/multi_observer<>

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   28 Nov 2006
*******************************************************************************/
#include <pcomn_indicator.h>
#include <pcomn_unittest.h>

#include <memory>

class MultipleChangeIndicatorTests : public CppUnit::TestFixture {

      void Test_Disconnected_Indicator() ;
      void Test_Disconnected_Observer() ;
      void Test_Connected() ;

      CPPUNIT_TEST_SUITE(MultipleChangeIndicatorTests) ;

      CPPUNIT_TEST(Test_Disconnected_Indicator) ;
      CPPUNIT_TEST(Test_Disconnected_Observer) ;
      CPPUNIT_TEST(Test_Connected) ;

      CPPUNIT_TEST_SUITE_END() ;

   public:
      void setUp()
      {}

      void tearDown()
      {}
} ;

using namespace pcomn ;

void MultipleChangeIndicatorTests::Test_Disconnected_Indicator()
{
   multi_indicator<2> indicator2 ;
   CPPUNIT_LOG_EQUAL(indicator2.IndicatorsCount, 2U) ;

   multi_indicator_base &foo_indicator_2 = indicator2 ;

   CPPUNIT_LOG_EQUAL(foo_indicator_2.size(), 2U) ;
   CPPUNIT_LOG_EQUAL(foo_indicator_2.valid_flags(), (bigflag_t)0x3) ;

   multi_indicator<2> dummy_indicator_2 (indicator2) ; /* Should compile the copy constructor! */

   const multi_indicator_base &foo_indicator_1 = multi_indicator<1>() ;
   CPPUNIT_LOG_EQUAL(foo_indicator_1.size(), 1U) ;
   CPPUNIT_LOG_EQUAL(foo_indicator_1.valid_flags(), (bigflag_t)0x1) ;

   const multi_indicator_base &foo_indicator_31 = multi_indicator<31>() ;
   CPPUNIT_LOG_EQUAL(foo_indicator_31.size(), 31U) ;
   CPPUNIT_LOG_EQUAL(foo_indicator_31.valid_flags(), (bigflag_t)0x7fffffff) ;
}

void MultipleChangeIndicatorTests::Test_Disconnected_Observer()
{
   multi_observer<5> dummy_observer ;
   CPPUNIT_LOG_EQUAL(dummy_observer.IndicatorsCount, 2U) ;
   CPPUNIT_LOG_EQUAL(dummy_observer.size(), 2U) ;
   CPPUNIT_LOG_EQUAL(dummy_observer.ObservedIndicators, (bigflag_t)5) ;
   CPPUNIT_LOG_EQUAL(dummy_observer.observed_indicators(), (bigflag_t)5) ;
   CPPUNIT_LOG_IS_FALSE(dummy_observer.is_indicator_alive()) ;
   CPPUNIT_LOG_EQUAL(dummy_observer.is_outofdate(), (bigflag_t)-1) ;
}

void MultipleChangeIndicatorTests::Test_Connected()
{
   std::unique_ptr<multi_indicator<1> > FooIndicator1 ;
   multi_observer<1> Observer1 ;

   CPPUNIT_LOG_RUN(FooIndicator1.reset(new multi_indicator<1>)) ;
   CPPUNIT_LOG_IS_FALSE(Observer1.is_indicator_alive()) ;
   CPPUNIT_LOG_EQUAL(Observer1.is_outofdate(), (bigflag_t)-1) ;
   CPPUNIT_LOG_IS_FALSE(Observer1.validate()) ;
   CPPUNIT_LOG_EQUAL(Observer1.is_outofdate(), (bigflag_t)-1) ;
   CPPUNIT_LOG_RUN(Observer1.reset(FooIndicator1.get())) ;
   CPPUNIT_LOG_IS_TRUE(Observer1.is_indicator_alive()) ;
   CPPUNIT_LOG_EQUAL(Observer1.is_outofdate(), 1U) ;
   CPPUNIT_LOG_ASSERT(Observer1.validate()) ;
   CPPUNIT_LOG_EQUAL(Observer1.is_outofdate(), 0U) ;
   CPPUNIT_LOG_ASSERT(Observer1.validate()) ;
   CPPUNIT_LOG_EQUAL(Observer1.is_outofdate(), 0U) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(Observer1.reset(FooIndicator1.get())) ;
   CPPUNIT_LOG_EQUAL(Observer1.is_outofdate(), 0U) ;
   CPPUNIT_LOG_ASSERT(Observer1.is_indicator_alive()) ;
   CPPUNIT_LOG_RUN(Observer1.invalidate()) ;
   CPPUNIT_LOG_ASSERT(Observer1.is_indicator_alive()) ;
   CPPUNIT_LOG_EQUAL(Observer1.is_outofdate(), 1U) ;
   CPPUNIT_LOG_ASSERT(Observer1.validate()) ;
   CPPUNIT_LOG_EQUAL(Observer1.is_outofdate(), 0U) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EXCEPTION(FooIndicator1->change_single(1), std::out_of_range) ;
   CPPUNIT_LOG_EQUAL(Observer1.is_outofdate(), 0U) ;
   CPPUNIT_LOG_EXCEPTION(FooIndicator1->change(3), std::out_of_range) ;
   CPPUNIT_LOG_EQUAL(Observer1.is_outofdate(), 0U) ;
   CPPUNIT_LOG_RUN(FooIndicator1->change(0)) ;
   CPPUNIT_LOG_EQUAL(Observer1.is_outofdate(), 0U) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(FooIndicator1->change_single(0)) ;
   CPPUNIT_LOG_EQUAL(Observer1.is_outofdate(), 1U) ;
   CPPUNIT_LOG_ASSERT(Observer1.validate()) ;
   CPPUNIT_LOG_EQUAL(Observer1.is_outofdate(), 0U) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(FooIndicator1->change(1)) ;
   CPPUNIT_LOG_EQUAL(Observer1.is_outofdate(), 1U) ;
   CPPUNIT_LOG_ASSERT(Observer1.validate()) ;
   CPPUNIT_LOG_EQUAL(Observer1.is_outofdate(), 0U) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(FooIndicator1.reset()) ;
   CPPUNIT_LOG_EQUAL(Observer1.is_outofdate(), (bigflag_t)-1) ;
   CPPUNIT_LOG_IS_FALSE(Observer1.is_indicator_alive()) ;
   CPPUNIT_LOG_RUN(FooIndicator1.reset(new multi_indicator<1>)) ;
   CPPUNIT_LOG_RUN(Observer1.reset(FooIndicator1.get())) ;
   CPPUNIT_LOG_EQUAL(Observer1.is_outofdate(), 1U) ;
   CPPUNIT_LOG_IS_TRUE(Observer1.is_indicator_alive()) ;
   CPPUNIT_LOG_ASSERT(Observer1.validate()) ;
   CPPUNIT_LOG_EQUAL(Observer1.is_outofdate(), 0U) ;

   CPPUNIT_LOG(std::endl) ;
   std::unique_ptr<multi_indicator<9> > FooIndicator9 ;
   std::unique_ptr<multi_indicator<31> > FooIndicator31 ;

   CPPUNIT_LOG_RUN(FooIndicator9.reset(new multi_indicator<9>)) ;
   CPPUNIT_LOG_RUN(FooIndicator31.reset(new multi_indicator<31>)) ;
   CPPUNIT_LOG_EQUAL(FooIndicator9->size(), 9U) ;
   CPPUNIT_LOG_EQUAL(FooIndicator9->valid_flags(), 0x1ffU) ;
   CPPUNIT_LOG_EQUAL(FooIndicator31->size(), 31U) ;
   CPPUNIT_LOG_EQUAL(FooIndicator31->valid_flags(), 0x7fffffffU) ;

   multi_observer<0xA> ObserverA (FooIndicator9.get()) ;
   multi_observer<0x103> Observer103 (FooIndicator9.get()) ;

   CPPUNIT_LOG_IS_TRUE(ObserverA.is_indicator_alive()) ;
   CPPUNIT_LOG_EQUAL(ObserverA.is_outofdate(), 0xAU) ;
   CPPUNIT_LOG_IS_TRUE(Observer103.is_indicator_alive()) ;
   CPPUNIT_LOG_EQUAL(Observer103.is_outofdate(), 0x103U) ;

   CPPUNIT_LOG_IS_TRUE(Observer103.validate()) ;
   CPPUNIT_LOG_EQUAL(Observer103.is_outofdate(), 0U) ;
   CPPUNIT_LOG_EQUAL(ObserverA.is_outofdate(), 0xAU) ;
   CPPUNIT_LOG_RUN(FooIndicator9->change_single(8)) ;
   CPPUNIT_LOG_EQUAL(Observer103.is_outofdate(), 0x100U) ;
   CPPUNIT_LOG_EQUAL(ObserverA.is_outofdate(), 0xAU) ;
   CPPUNIT_LOG_IS_TRUE(ObserverA.validate()) ;
   CPPUNIT_LOG_IS_TRUE(Observer103.validate()) ;
   CPPUNIT_LOG_EQUAL(Observer103.is_outofdate(), 0U) ;
   CPPUNIT_LOG_EQUAL(ObserverA.is_outofdate(), 0U) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EXCEPTION(FooIndicator9->change(0x200), std::out_of_range) ;
   CPPUNIT_LOG_EQUAL(Observer103.is_outofdate(), 0U) ;
   CPPUNIT_LOG_EQUAL(ObserverA.is_outofdate(), 0U) ;
   CPPUNIT_LOG_RUN(FooIndicator9->change(0x1f2)) ;
   CPPUNIT_LOG_EQUAL(Observer103.is_outofdate(), 0x102U) ;
   CPPUNIT_LOG_EQUAL(ObserverA.is_outofdate(), 0x2U) ;
   CPPUNIT_LOG_IS_TRUE(ObserverA.validate()) ;
   CPPUNIT_LOG_IS_TRUE(Observer103.validate()) ;
   CPPUNIT_LOG_EQUAL(Observer103.is_outofdate(), 0U) ;
   CPPUNIT_LOG_EQUAL(ObserverA.is_outofdate(), 0U) ;
   CPPUNIT_LOG_RUN(FooIndicator9 = NULL) ;
   CPPUNIT_LOG_EQUAL(Observer103.is_outofdate(), (bigflag_t)-1) ;
   CPPUNIT_LOG_EQUAL(ObserverA.is_outofdate(), (bigflag_t)-1) ;
   CPPUNIT_LOG_IS_FALSE(ObserverA.is_indicator_alive()) ;
   CPPUNIT_LOG_IS_FALSE(Observer103.is_indicator_alive()) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(MultipleChangeIndicatorTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "unittest.diag.ini", "change_indicator/change_observer tests") ;
}
