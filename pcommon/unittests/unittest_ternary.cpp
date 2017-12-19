/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_ternary.cpp
 COPYRIGHT    :   Yakov Markovitch, 2017. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests of ternary logic

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   19 Dec 2017
*******************************************************************************/
#include <pcomn_ternary.h>
#include <pcomn_unittest.h>

#include <utility>

/*******************************************************************************
 Ternary logic tests
*******************************************************************************/
class TernaryLogicTests : public CppUnit::TestFixture {

      void Test_TLogic_Constructor() ;
      void Test_TLogic_Compare() ;
      void Test_TLogic_Logic() ;

      CPPUNIT_TEST_SUITE(TernaryLogicTests) ;

      CPPUNIT_TEST(Test_TLogic_Constructor) ;
      CPPUNIT_TEST(Test_TLogic_Compare) ;
      CPPUNIT_TEST(Test_TLogic_Logic) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

using namespace pcomn ;

/*******************************************************************************
 TernaryLogicTests
*******************************************************************************/
void TernaryLogicTests::Test_TLogic_Constructor()
{
   tlogic_t b1 ;
   tlogic_t b10(tlogic_t::False) ;
   tlogic_t b100(uint8_t{}) ;

   tlogic_t b2 (tlogic_t::True) ;
   tlogic_t b20 ((uint8_t)tlogic_t::True) ;

   tlogic_t b3(tlogic_t::Nothing) ;
   tlogic_t b30((uint8_t)tlogic_t::Nothing) ;

   tlogic_t b4(true) ;
   tlogic_t b5(false) ;

   CPPUNIT_LOG_EQUAL((tlogic_t::State)b1, tlogic_t::False) ;
   CPPUNIT_LOG_EQ((uint8_t)b1, tlogic_t::False) ;
   CPPUNIT_LOG_EQUAL((char)b1, 'F') ;
   CPPUNIT_LOG_EQ(std::to_string(b1), "F") ;

   CPPUNIT_LOG_EQUAL((tlogic_t::State)b10, tlogic_t::False) ;
   CPPUNIT_LOG_EQ((uint8_t)b10, tlogic_t::False) ;
   CPPUNIT_LOG_EQUAL((char)b10, 'F') ;
   CPPUNIT_LOG_EQ(std::to_string(b10), "F") ;

   CPPUNIT_LOG_EQUAL((tlogic_t::State)b100, tlogic_t::False) ;
   CPPUNIT_LOG_EQ((uint8_t)b100, tlogic_t::False) ;
   CPPUNIT_LOG_EQUAL((char)b100, 'F') ;
   CPPUNIT_LOG_EQ(std::to_string(b100), "F") ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL((tlogic_t::State)b2, tlogic_t::True) ;
   CPPUNIT_LOG_EQ((uint8_t)b2, tlogic_t::True) ;
   CPPUNIT_LOG_EQUAL((char)b2, 'T') ;
   CPPUNIT_LOG_EQ(std::to_string(b2), "T") ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL((tlogic_t::State)b20, tlogic_t::True) ;
   CPPUNIT_LOG_EQ((uint8_t)b20, tlogic_t::True) ;
   CPPUNIT_LOG_EQUAL((char)b20, 'T') ;
   CPPUNIT_LOG_EQ(std::to_string(b20), "T") ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL((tlogic_t::State)b3, tlogic_t::Nothing) ;
   CPPUNIT_LOG_EQ((uint8_t)b3, tlogic_t::Nothing) ;
   CPPUNIT_LOG_EQUAL((char)b3, 'N') ;
   CPPUNIT_LOG_EQ(std::to_string(b3), "N") ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL((tlogic_t::State)b30, tlogic_t::Nothing) ;
   CPPUNIT_LOG_EQ((uint8_t)b30, tlogic_t::Nothing) ;
   CPPUNIT_LOG_EQUAL((char)b30, 'N') ;
   CPPUNIT_LOG_EQ(std::to_string(b30), "N") ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL((tlogic_t::State)b4, tlogic_t::True) ;
   CPPUNIT_LOG_EQ((uint8_t)b4, tlogic_t::True) ;
   CPPUNIT_LOG_EQUAL((char)b4, 'T') ;
   CPPUNIT_LOG_EQ(std::to_string(b4), "T") ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL((tlogic_t::State)b5, tlogic_t::False) ;
   CPPUNIT_LOG_EQ((uint8_t)b5, tlogic_t::False) ;
   CPPUNIT_LOG_EQUAL((char)b5, 'F') ;
   CPPUNIT_LOG_EQ(std::to_string(b5), "F") ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQ((char)(b5 = tlogic_t::Nothing), 'N') ;
   CPPUNIT_LOG_EQ((char)(b5 = true), 'T') ;
   CPPUNIT_LOG_EQ((char)(b5 = false), 'F') ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQ(TFALSE, tlogic_t::False) ;
   CPPUNIT_LOG_EQ(TTRUE, tlogic_t::True) ;
   CPPUNIT_LOG_EQ(TNOTHING, tlogic_t::Nothing) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQ(string_cast(b1), "F") ;
   CPPUNIT_LOG_EQ(string_cast(b2), "T") ;
   CPPUNIT_LOG_EQ(string_cast(b3), "N") ;
}

void TernaryLogicTests::Test_TLogic_Compare()
{
   constexpr tlogic_t b1  (tlogic_t::False) ;
   constexpr tlogic_t b10 (tlogic_t::False) ;

   constexpr tlogic_t b2  (tlogic_t::True) ;
   constexpr tlogic_t b20 (tlogic_t::True) ;

   constexpr tlogic_t b3  (tlogic_t::Nothing) ;
   constexpr tlogic_t b30 (tlogic_t::Nothing) ;

   CPPUNIT_LOG_EQUAL(b1, b1) ;
   CPPUNIT_LOG_EQUAL(b1, b10) ;
   CPPUNIT_LOG_EQUAL(b10, b1) ;
   CPPUNIT_LOG_EQUAL(b2, b20) ;
   CPPUNIT_LOG_EQUAL(b20, b2) ;
   CPPUNIT_LOG_EQUAL(b3, b30) ;
   CPPUNIT_LOG_EQUAL(b30, b3) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(!(b1 != b1)) ;
   CPPUNIT_LOG_ASSERT(!(b1 != b10)) ;
   CPPUNIT_LOG_ASSERT(b2 != b1) ;
   CPPUNIT_LOG_ASSERT(b2 != b3) ;
   CPPUNIT_LOG_ASSERT(b1 != b3) ;

   CPPUNIT_LOG_ASSERT(b1 < b2) ;
   CPPUNIT_LOG_ASSERT(b1 < b3) ;
   CPPUNIT_LOG_ASSERT(b1 <= b2) ;
   CPPUNIT_LOG_ASSERT(b1 <= b3) ;
   CPPUNIT_LOG_ASSERT(b2 > b3) ;
   CPPUNIT_LOG_ASSERT(b2 >= b3) ;
   CPPUNIT_LOG_ASSERT(b1 <= b10) ;
   CPPUNIT_LOG_ASSERT(b10 <= b1) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(TFALSE, b1) ;
   CPPUNIT_LOG_EQUAL(TTRUE, b2) ;
   CPPUNIT_LOG_EQUAL(TNOTHING, b3) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(TFALSE.as_bool(true), false) ;
   CPPUNIT_LOG_EQUAL(TFALSE.as_bool(false), false) ;

   CPPUNIT_LOG_EQUAL(TTRUE.as_bool(true), true) ;
   CPPUNIT_LOG_EQUAL(TTRUE.as_bool(false), true) ;

   CPPUNIT_LOG_EQUAL(TNOTHING.as_bool(true), true) ;
   CPPUNIT_LOG_EQUAL(TNOTHING.as_bool(false), false) ;
}

void TernaryLogicTests::Test_TLogic_Logic()
{
   const tlogic_t b1 (tlogic_t::False) ;
   const tlogic_t b2 (tlogic_t::True) ;
   const tlogic_t b3 (tlogic_t::Nothing) ;

   CPPUNIT_LOG_EQ(!b1, tlogic_t::True) ;
   CPPUNIT_LOG_EQ(!b2, tlogic_t::False) ;
   CPPUNIT_LOG_EQ(!b3, tlogic_t::Nothing) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(TFALSE && TFALSE, TFALSE) ;
   CPPUNIT_LOG_EQUAL(TFALSE || TFALSE, TFALSE) ;
   CPPUNIT_LOG_EQUAL(TTRUE  && TTRUE,  TTRUE) ;
   CPPUNIT_LOG_EQUAL(TTRUE  || TTRUE,  TTRUE) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(TFALSE && TTRUE,  TFALSE) ;
   CPPUNIT_LOG_EQUAL(TTRUE  && TFALSE, TFALSE) ;
   CPPUNIT_LOG_EQUAL(TFALSE || TTRUE,  TTRUE) ;
   CPPUNIT_LOG_EQUAL(TTRUE  || TFALSE, TTRUE) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(TFALSE   && TNOTHING, TFALSE) ;
   CPPUNIT_LOG_EQUAL(TNOTHING && TFALSE,   TFALSE) ;

   CPPUNIT_LOG_EQUAL(TTRUE    && TNOTHING, TNOTHING) ;
   CPPUNIT_LOG_EQUAL(TNOTHING && TTRUE,    TNOTHING) ;

   CPPUNIT_LOG_EQUAL(TFALSE   || TNOTHING, TNOTHING) ;
   CPPUNIT_LOG_EQUAL(TNOTHING || TFALSE,   TNOTHING) ;

   CPPUNIT_LOG_EQUAL(TTRUE    || TNOTHING, TTRUE) ;
   CPPUNIT_LOG_EQUAL(TNOTHING || TTRUE,    TTRUE) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(false    && TNOTHING, TFALSE) ;
   CPPUNIT_LOG_EQUAL(TNOTHING && false,   TFALSE) ;

   CPPUNIT_LOG_EQUAL(true     && TNOTHING, TNOTHING) ;
   CPPUNIT_LOG_EQUAL(TNOTHING && true,    TNOTHING) ;

   CPPUNIT_LOG_EQUAL(false    || TNOTHING, TNOTHING) ;
   CPPUNIT_LOG_EQUAL(TNOTHING || false,   TNOTHING) ;

   CPPUNIT_LOG_EQUAL(true     || TNOTHING, TTRUE) ;
   CPPUNIT_LOG_EQUAL(TNOTHING || true,    TTRUE) ;
}

int main(int argc, char *argv[])
{
   return pcomn::unit::run_tests
      <
         TernaryLogicTests
      >
      (argc, argv, "unittest.diag.ini", "Tests of ternary logic") ;
}
