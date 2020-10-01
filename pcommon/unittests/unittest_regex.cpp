/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_regex.cpp
 COPYRIGHT    :   Yakov Markovitch, 2007-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unit test fixture for pcomn::regex and pcomn::wildcard_matcher

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   1 Aug 2007
*******************************************************************************/
#include <pcomn_immutablestr.h>
#include <pcomn_unittest.h>
#include <pcomn_strslice.h>
#include <pcomn_regex.h>

#include "pcomn_testhelpers.h"

#include <iostream>
#include <utility>

#ifdef __PCOMN_RE2EX_H
typedef pcomn::re2ex regex_test_type ;
#else
typedef pcomn::regex regex_test_type ;
#endif

template<typename C>
struct TestStrings {
      static const C * const UnmatchedBracketRe ;
      static const C * const UnmatchedParenRe ;

      static const C * const XorRe ;
      static const C * const IdentRe ;
      static const C * const ColonQualIdentRe ;
      static const C * const DotQualIdentRe ;
      static const C * const IdentOffs1Re ;
      static const C IdentOffs1BufRe[] ;
      static const C ColonQualIdent2Re[] ;
      static C ColonQualIdent2EndAnchoredRe[] ;

      static const C * const Xor ;
      static const C * const Ident1 ;
      static const C * const Ident2 ;
      static const C Ident3Buf[] ;
      static C Ident4Buf[] ;
      static const C ColonQualIdent1[] ;
      static const C ColonQualIdent2[] ;
} ;

PCOMN_DEFINE_TEST_STR(XorRe, "[\\^]") ;
PCOMN_DEFINE_TEST_STR(IdentRe, "[_A-Za-z][_A-Za-z0-9]*") ;
PCOMN_DEFINE_TEST_STR(ColonQualIdentRe, "([_A-Za-z][_A-Za-z0-9]*)(::([_A-Za-z][_A-Za-z0-9]*))*") ;
PCOMN_DEFINE_TEST_STR(UnmatchedBracketRe, "([_A-Za-z)*") ;
PCOMN_DEFINE_TEST_STR(UnmatchedParenRe, "([_A-Za-z]*") ;
PCOMN_DEFINE_TEST_STR(DotQualIdentRe, "([_A-Za-z][_A-Za-z0-9]*)([.]([_A-Za-z][_A-Za-z0-9]*))*") ;

PCOMN_DEFINE_TEST_BUF(const, ColonQualIdent2Re, "([_A-Za-z][_A-Za-z0-9]*)::([_A-Za-z][_A-Za-z0-9]*)") ;
PCOMN_DEFINE_TEST_BUF(P_EMPTY_ARG, ColonQualIdent2EndAnchoredRe, "([_A-Za-z][_A-Za-z0-9]*)::([_A-Za-z][_A-Za-z0-9]*)$") ;

PCOMN_DEFINE_TEST_STR(IdentOffs1Re, "([_A-Za-z][_A-Za-z0-9]*") ;
PCOMN_DEFINE_TEST_BUF(const, IdentOffs1BufRe, "([_A-Za-z][_A-Za-z0-9]*") ;

PCOMN_DEFINE_TEST_STR(Xor, " ^ ") ;
PCOMN_DEFINE_TEST_STR(Ident1, "FooBar") ;
PCOMN_DEFINE_TEST_STR(Ident2, "_01HelloWorld") ;
PCOMN_DEFINE_TEST_BUF(const, Ident3Buf, "  No!") ;
PCOMN_DEFINE_TEST_BUF(P_EMPTY_ARG, Ident4Buf, "yEs... ") ;
PCOMN_DEFINE_TEST_BUF(const, ColonQualIdent1, "Foo::Bar") ;
PCOMN_DEFINE_TEST_BUF(const, ColonQualIdent2, "Foo::Bar::Quux") ;

template<typename Char, typename Str, class Rx = pcomn::regex>
class RegexTests : public CppUnit::TestFixture {
   public:
      typedef Char   char_type ;
      typedef Str    string_type ;
      typedef Rx     regex_type ;

      typedef TestStrings<char_type>               Strings ;
      typedef std::basic_string<char_type>         stdstring ;
      typedef pcomn::basic_strslice<char_type>     slice ;
      typedef pcomn::immutable_string<char_type>   istring ;

      typedef RegexTests<char_type, string_type, regex_type> self ;

   private:
      void Test_Construct() ;
      void Test_Match() ;
      void Test_Index() ;
      void Test_WildcardMatch() ;

      CPPUNIT_TEST_SUITE(RegexTests) ;

      CPPUNIT_TEST(Test_Construct) ;
      CPPUNIT_TEST(Test_Match) ;
      CPPUNIT_TEST(Test_Index) ;
      CPPUNIT_TEST(Test_WildcardMatch) ;

      CPPUNIT_TEST_SUITE_END() ;

      static string_type StringIdentRe ;
      static string_type StringIdentOffs1Re ;
} ;

template<typename Char, typename Str, class Rx>
Str RegexTests<Char, Str, Rx>::StringIdentRe = RegexTests<Char, Str, Rx>::Strings::IdentRe ;
template<typename Char, typename Str, class Rx>
Str RegexTests<Char, Str, Rx>::StringIdentOffs1Re = RegexTests<Char, Str, Rx>::Strings::IdentOffs1Re ;

template<typename S>
static typename pcomn::enable_if_string<S, S>::type rstring(const S &s) { return s ; }

template<typename C>
static std::basic_string<C> rstring(const pcomn::basic_strslice<C> &s) { return s.stdstring() ; }

/*******************************************************************************
 RegexTests
*******************************************************************************/
template<typename Char, typename Str, class Rx>
void RegexTests<Char, Str, Rx>::Test_Construct()
{
   // First test bad regexps
   CPPUNIT_LOG_EXCEPTION_CODE(regex_type _(Strings::UnmatchedBracketRe),
                              pcomn::regex_error, PREG_UNMATCHED_BRACKETS) ;
   CPPUNIT_LOG_EXCEPTION_CODE(regex_type _(Strings::UnmatchedParenRe),
                              pcomn::regex_error, PREG_UNMATCHED_PARENTHESIS) ;
   // Then test good regexps
   CPPUNIT_LOG_RUN(regex_type _(Strings::IdentRe)) ;
   CPPUNIT_LOG_RUN(regex_type _(rstring(StringIdentRe))) ;
   CPPUNIT_LOG_RUN(regex_type _(Strings::ColonQualIdentRe)) ;
   CPPUNIT_LOG_RUN(regex_type _(Strings::DotQualIdentRe)) ;
   CPPUNIT_LOG_RUN(regex_type _(Strings::ColonQualIdent2Re)) ;
   CPPUNIT_LOG_RUN(regex_type _(Strings::ColonQualIdent2EndAnchoredRe)) ;

   CPPUNIT_LOG_EXCEPTION_CODE(regex_type _(Strings::IdentOffs1Re),
                              pcomn::regex_error, PREG_UNMATCHED_PARENTHESIS) ;
   CPPUNIT_LOG_EXCEPTION_CODE(regex_type _(Strings::IdentOffs1BufRe),
                              pcomn::regex_error, PREG_UNMATCHED_PARENTHESIS) ;
   CPPUNIT_LOG_EXCEPTION_CODE(regex_type _(rstring(StringIdentOffs1Re)),
                              pcomn::regex_error, PREG_UNMATCHED_PARENTHESIS) ;
}

template<typename Char, typename Str, class Rx>
void RegexTests<Char, Str, Rx>::Test_Match()
{
   regex_type rx ;
   string_type TestStr ;
   reg_match Matched ;
   reg_match Sub[32] ;

   CPPUNIT_LOG_RUN(rx = regex_type(Strings::IdentRe)) ;
   CPPUNIT_LOG_ASSERT(PSUBEXP_MATCHED(Matched = rx.match(Strings::Ident1))) ;
   CPPUNIT_LOG_EQUAL(PSUBEXP_LENGTH(Matched), 6) ;

   CPPUNIT_LOG_ASSERT(PSUBEXP_MATCHED(Matched = rx.match(Strings::Ident3Buf))) ;
   CPPUNIT_LOG_EQUAL(PSUBEXP_LENGTH(Matched), 2) ;
   CPPUNIT_LOG_EQUAL(self::slice(Strings::Ident3Buf, Matched), self::slice("No")) ;

   CPPUNIT_LOG_RUN(TestStr = Strings::Ident3Buf) ;
   CPPUNIT_LOG_ASSERT(PSUBEXP_MATCHED(Matched = rx.match(TestStr))) ;
   CPPUNIT_LOG_EQUAL(PSUBEXP_LENGTH(Matched), 2) ;
   CPPUNIT_LOG_EQUAL(self::slice(TestStr, Matched), self::slice("No")) ;

   CPPUNIT_LOG_ASSERT(PSUBEXP_MATCHED(Matched = rx.match(Strings::Ident4Buf))) ;
   CPPUNIT_LOG_EQUAL(PSUBEXP_LENGTH(Matched), 3) ;
   CPPUNIT_LOG_EQUAL(self::slice(Strings::Ident4Buf, Matched), self::slice("yEs")) ;

   CPPUNIT_LOG_RUN(rx = regex_type(Strings::XorRe)) ;
   CPPUNIT_LOG_ASSERT(PSUBEXP_MATCHED(Matched = rx.match(Strings::Xor))) ;
   CPPUNIT_LOG_EQUAL(PSUBEXP_LENGTH(Matched), 1) ;
   CPPUNIT_LOG_EQUAL(self::slice(Strings::Xor, Matched), self::slice("^")) ;

   CPPUNIT_LOG_RUN(rx = regex_type(Strings::ColonQualIdentRe)) ;
   CPPUNIT_LOG_EQUAL(rx.match(Strings::ColonQualIdent2, Sub) - Sub, (ptrdiff_t)4) ;
   CPPUNIT_LOG_EQUAL(Sub[0], make_reg_match(0, 14)) ;
   CPPUNIT_LOG_EQUAL(Sub[1], make_reg_match(0, 3)) ;
   // Note that, when '*' or '+' are after parenthesis, the last matched are returned
   CPPUNIT_LOG_EQUAL(Sub[2], make_reg_match(8, 14)) ;
   CPPUNIT_LOG_EQUAL(Sub[3], make_reg_match(10, 14)) ;
}

template<typename Char, typename Str, class Rx>
void RegexTests<Char, Str, Rx>::Test_Index()
{
}

template<typename Char, typename Str, class Rx>
void RegexTests<Char, Str, Rx>::Test_WildcardMatch()
{
   pcomn::wildcard_matcher matcher ;

   CPPUNIT_LOG_ASSERT(matcher.match("")) ;
   CPPUNIT_LOG_ASSERT(!matcher.match(" ")) ;
   CPPUNIT_LOG_ASSERT(!matcher.match("a")) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(matcher = pcomn::wildcard_matcher("a")) ;
   CPPUNIT_LOG_ASSERT(matcher.match("a")) ;
   CPPUNIT_LOG_ASSERT(!matcher.match("A")) ;
   CPPUNIT_LOG_ASSERT(!matcher.match("A")) ;
   CPPUNIT_LOG_ASSERT(!matcher.match("aa")) ;
   CPPUNIT_LOG_ASSERT(!matcher.match("")) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(matcher = pcomn::wildcard_matcher("b")) ;
   CPPUNIT_LOG_ASSERT(!matcher.match("a")) ;
   CPPUNIT_LOG_ASSERT(!matcher.match("A")) ;
   CPPUNIT_LOG_ASSERT(matcher.match("b")) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(matcher = pcomn::wildcard_matcher("*llo, world!")) ;
   CPPUNIT_LOG_ASSERT(matcher.match("llo, world!")) ;
   CPPUNIT_LOG_ASSERT(!matcher.match("LLO, world!")) ;
   CPPUNIT_LOG_ASSERT(!matcher.match("llo, world! ")) ;
   CPPUNIT_LOG_ASSERT(!matcher.match("Hello, world! ")) ;
   CPPUNIT_LOG_ASSERT(matcher.match("Hello, world!")) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(matcher = pcomn::wildcard_matcher("*llo. world?")) ;
   CPPUNIT_LOG_ASSERT(!matcher.match("llo, world?")) ;
   CPPUNIT_LOG_ASSERT(!matcher.match("llo, world!")) ;
   CPPUNIT_LOG_ASSERT(matcher.match("llo. world!")) ;
   CPPUNIT_LOG_ASSERT(matcher.match("Hello. world!")) ;
   CPPUNIT_LOG_RUN(matcher = pcomn::wildcard_matcher("llo. world?")) ;
   CPPUNIT_LOG_ASSERT(matcher.match("llo. world!")) ;
   CPPUNIT_LOG_ASSERT(!matcher.match("Hello. world!")) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(matcher = pcomn::wildcard_matcher("**llo. world?")) ;
   CPPUNIT_LOG_ASSERT(!matcher.match("llo, world?")) ;
   CPPUNIT_LOG_ASSERT(!matcher.match("llo, world!")) ;
   CPPUNIT_LOG_ASSERT(matcher.match("llo. world!")) ;
   CPPUNIT_LOG_ASSERT(matcher.match("Hello. world!")) ;
   CPPUNIT_LOG_RUN(matcher = pcomn::wildcard_matcher("llo. world?")) ;
   CPPUNIT_LOG_ASSERT(matcher.match("llo. world!")) ;
   CPPUNIT_LOG_ASSERT(!matcher.match("Hello. world!")) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(matcher = pcomn::wildcard_matcher("[0-9A-Z]*llo[.?] world?")) ;
   CPPUNIT_LOG_ASSERT(matcher.match("Hello? world!")) ;
   CPPUNIT_LOG_ASSERT(!matcher.match(" Hello? world!")) ;
   CPPUNIT_LOG_ASSERT(!matcher.match("hello? world!")) ;

   // If there is no contents in the character class,
   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(matcher = pcomn::wildcard_matcher("[]")) ;
   CPPUNIT_LOG_IS_FALSE(matcher.match("")) ;
   CPPUNIT_LOG_ASSERT(matcher.match("[]")) ;
   CPPUNIT_LOG_IS_FALSE(matcher.match("]")) ;
   CPPUNIT_LOG_IS_FALSE(matcher.match("[")) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(matcher = pcomn::wildcard_matcher("[R]")) ;
   CPPUNIT_LOG_ASSERT(matcher.match("R")) ;
   CPPUNIT_LOG_IS_FALSE(matcher.match("r")) ;
   CPPUNIT_LOG_IS_FALSE(matcher.match("U")) ;
   CPPUNIT_LOG_IS_FALSE(matcher.match("u")) ;
   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_RUN(matcher = pcomn::wildcard_matcher("[R]")) ;
   CPPUNIT_LOG_ASSERT(matcher.match("R")) ;
   CPPUNIT_LOG_IS_FALSE(matcher.match("r")) ;
   CPPUNIT_LOG_IS_FALSE(matcher.match("U")) ;
   CPPUNIT_LOG_IS_FALSE(matcher.match("u")) ;

   CPPUNIT_LOG_RUN(matcher = pcomn::wildcard_matcher("[!A-C]")) ;
   CPPUNIT_LOG_ASSERT(matcher.match("R")) ;
   CPPUNIT_LOG_IS_FALSE(matcher.match("A")) ;
   CPPUNIT_LOG_IS_FALSE(matcher.match("B")) ;
   CPPUNIT_LOG_IS_FALSE(matcher.match("C")) ;
   CPPUNIT_LOG_ASSERT(matcher.match("!")) ;
   CPPUNIT_LOG_ASSERT(matcher.match("a")) ;
   CPPUNIT_LOG_ASSERT(matcher.match(" ")) ;
   CPPUNIT_LOG_ASSERT(matcher.match("E")) ;

   CPPUNIT_LOG_RUN(matcher = pcomn::wildcard_matcher("[^A-C]")) ;
   CPPUNIT_LOG_ASSERT(matcher.match("A")) ;
   CPPUNIT_LOG_ASSERT(matcher.match("B")) ;
   CPPUNIT_LOG_ASSERT(matcher.match("C")) ;
   CPPUNIT_LOG_ASSERT(matcher.match("^")) ;
   CPPUNIT_LOG_IS_FALSE(matcher.match("!")) ;
   CPPUNIT_LOG_IS_FALSE(matcher.match("a")) ;
   CPPUNIT_LOG_IS_FALSE(matcher.match(" ")) ;
   CPPUNIT_LOG_IS_FALSE(matcher.match("E")) ;

   CPPUNIT_LOG_RUN(matcher = pcomn::wildcard_matcher("[^A-C]", false)) ;
   CPPUNIT_LOG_ASSERT(matcher.match("R")) ;
   CPPUNIT_LOG_IS_FALSE(matcher.match("A")) ;
   CPPUNIT_LOG_IS_FALSE(matcher.match("B")) ;
   CPPUNIT_LOG_IS_FALSE(matcher.match("C")) ;
   CPPUNIT_LOG_ASSERT(matcher.match("!")) ;
   CPPUNIT_LOG_ASSERT(matcher.match("^")) ;
   CPPUNIT_LOG_ASSERT(matcher.match("a")) ;
   CPPUNIT_LOG_ASSERT(matcher.match(" ")) ;
   CPPUNIT_LOG_ASSERT(matcher.match("E")) ;

   CPPUNIT_LOG_RUN(matcher = pcomn::wildcard_matcher("[^]")) ;
   CPPUNIT_LOG_ASSERT(matcher.match("^")) ;
   CPPUNIT_LOG_IS_FALSE(matcher.match("")) ;
   CPPUNIT_LOG_IS_FALSE(matcher.match("[")) ;
   CPPUNIT_LOG_IS_FALSE(matcher.match("]")) ;
}

/*******************************************************************************
                     class RegexCallModeTests
*******************************************************************************/
template<typename R = pcomn::regex>
class RegexCallModeTests : public CppUnit::TestFixture {
   public:
      typedef R regex_type ;

   private:
      void Test_Call_Match() ;
      void Test_Call_IsMatched() ;
      void Test_Call_Index() ;

      CPPUNIT_TEST_SUITE(RegexCallModeTests) ;

      CPPUNIT_TEST(Test_Call_Match) ;
      CPPUNIT_TEST(Test_Call_IsMatched) ;
      CPPUNIT_TEST(Test_Call_Index) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

template<typename R>
void RegexCallModeTests<R>::Test_Call_Match()
{
   regex_type RX ;
   reg_match Matched = make_reg_match(1, 2) ;
   reg_match Sub[64] ;
   const char * const str = " 15.Hello_344::_World+-990" ;
   const char * const estr = "" ;
   const pcomn::strslice eslice ;

   CPPUNIT_LOG_RUN(RX = regex_type("^$")) ;
   CPPUNIT_LOG_ASSERT(PSUBEXP_MATCHED(Matched = RX.match(estr))) ;
   CPPUNIT_LOG_EQUAL(PSUBEXP_LENGTH(Matched), 0) ;
   CPPUNIT_LOG_EQUAL(PSUBEXP_BO(Matched), 0) ;

   Matched = make_reg_match(1, 2) ;
   CPPUNIT_LOG_ASSERT(PSUBEXP_MATCHED(Matched = RX.match(eslice))) ;
   CPPUNIT_LOG_EQUAL(PSUBEXP_LENGTH(Matched), 0) ;
   CPPUNIT_LOG_EQUAL(PSUBEXP_BO(Matched), 0) ;

   Matched = make_reg_match(1, 2) ;
   CPPUNIT_LOG_ASSERT(PSUBEXP_MATCHED(Matched = RX.match(pcomn::strslice(estr)))) ;
   CPPUNIT_LOG_EQUAL(PSUBEXP_LENGTH(Matched), 0) ;
   CPPUNIT_LOG_EQUAL(PSUBEXP_BO(Matched), 0) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(RX = regex_type("[0-9][1-9]")) ;
   Matched = make_reg_match(1, 2) ;
   CPPUNIT_LOG_IS_FALSE(PSUBEXP_MATCHED(Matched = RX.match(eslice))) ;
   Matched = make_reg_match(0, 2) ;
   CPPUNIT_LOG_ASSERT(PSUBEXP_MATCHED(Matched = RX.match(str))) ;
   CPPUNIT_LOG_EQUAL(Matched, make_reg_match(1, 3)) ;

   CPPUNIT_LOG_RUN(RX = regex_type("[0-9][1-9]$")) ;
   Matched = make_reg_match(1, 2) ;
   CPPUNIT_LOG_IS_FALSE(PSUBEXP_MATCHED(Matched = RX.match(str))) ;
   CPPUNIT_LOG_ASSERT(PSUBEXP_MATCHED(Matched = RX.match(pcomn::strslice(str, 0, 3)))) ;
   CPPUNIT_LOG_EQUAL(Matched, make_reg_match(1, 3)) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(RX = regex_type("^.+([0-9]+)([^:3]+)([0-9])+")) ;
   Matched = make_reg_match(1, 2) ;
   CPPUNIT_LOG_ASSERT(PSUBEXP_MATCHED(Matched = RX.match(str))) ;
   CPPUNIT_LOG_EQUAL(Matched, make_reg_match(0, 26)) ;
   CPPUNIT_LOG_EQUAL(RX.match(str, Sub) - Sub, (ptrdiff_t)4) ;
   CPPUNIT_LOG_EQUAL(Sub[0], make_reg_match(0, 26)) ;
   CPPUNIT_LOG_EQUAL(Sub[1], make_reg_match(23, 24)) ;
   CPPUNIT_LOG_EQUAL(Sub[2], make_reg_match(24, 25)) ;
   CPPUNIT_LOG_EQUAL(Sub[3], make_reg_match(25, 26)) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(RX.match(pcomn::strslice(str, 0, 25), Sub) - Sub, (ptrdiff_t)4) ;
   CPPUNIT_LOG_EQUAL(Sub[0], make_reg_match(0, 13)) ;
   CPPUNIT_LOG_EQUAL(Sub[1], make_reg_match(10, 11)) ;
   CPPUNIT_LOG_EQUAL(Sub[2], make_reg_match(11, 12)) ;
   CPPUNIT_LOG_EQUAL(Sub[3], make_reg_match(12, 13)) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(RX.match(pcomn::strslice(str, 0, 12), Sub) - Sub, (ptrdiff_t)4) ;
   CPPUNIT_LOG_EQUAL(Sub[0], make_reg_match(0, 12)) ;
   CPPUNIT_LOG_EQUAL(Sub[1], make_reg_match(2, 3)) ;
   // Note that, when '*' or '+' are after parenthesis, the last matched are returned
   CPPUNIT_LOG_EQUAL(Sub[2], make_reg_match(3, 10)) ;
   CPPUNIT_LOG_EQUAL(Sub[3], make_reg_match(11, 12)) ;
}

template<typename R>
void RegexCallModeTests<R>::Test_Call_IsMatched()
{
   regex_type RX ;
   reg_match Sub[64] ;
   const char * const str = " 15.Hello_344::_World+-990" ;
   const char * const estr = "" ;
   const pcomn::strslice eslice ;

   CPPUNIT_LOG_RUN(RX = regex_type("^$")) ;
   CPPUNIT_LOG_ASSERT(RX.is_matched(estr)) ;
   CPPUNIT_LOG_ASSERT(RX.is_matched(eslice)) ;
   CPPUNIT_LOG_IS_FALSE(RX.is_matched(str)) ;
   CPPUNIT_LOG_IS_FALSE(RX.is_matched(pcomn::strslice(str, 0, 1))) ;
   CPPUNIT_LOG_ASSERT(RX.is_matched(pcomn::strslice(str, 1, 1))) ;

   *Sub = make_reg_match(1, 2) ;
   CPPUNIT_LOG_ASSERT(RX.is_matched(estr, Sub)) ;
   CPPUNIT_LOG_EQUAL(PSUBEXP_LENGTH(*Sub), 0) ;
   CPPUNIT_LOG_EQUAL(PSUBEXP_BO(*Sub), 0) ;
   CPPUNIT_LOG_IS_FALSE(PSUBEXP_MATCHED(Sub[1])) ;

   *Sub = make_reg_match(1, 2) ;
   CPPUNIT_LOG_ASSERT(RX.is_matched(pcomn::strslice(str, 1, 1), Sub)) ;
   CPPUNIT_LOG_EQUAL(PSUBEXP_LENGTH(*Sub), 0) ;
   CPPUNIT_LOG_EQUAL(PSUBEXP_BO(*Sub), 0) ;
   CPPUNIT_LOG_IS_FALSE(PSUBEXP_MATCHED(Sub[1])) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(RX = regex_type("(Hell)o.([0-9]*)")) ;

   Sub[0] = Sub[1] = Sub[2] = Sub[3] = make_reg_match(1000, 2000) ;
   CPPUNIT_LOG_ASSERT(RX.is_matched(pcomn::strslice(str, 0, 15), Sub + 0, 4)) ;
   CPPUNIT_LOG_EQUAL(Sub[0], make_reg_match(4, 13)) ;
   CPPUNIT_LOG_EQUAL(Sub[1], make_reg_match(4, 8)) ;
   CPPUNIT_LOG_EQUAL(Sub[2], make_reg_match(10, 13)) ;
   CPPUNIT_LOG_IS_FALSE(PSUBEXP_MATCHED(Sub[3])) ;

   Sub[0] = Sub[1] = Sub[2] = Sub[3] = make_reg_match(1000, 2000) ;
   CPPUNIT_LOG_ASSERT(RX.is_matched(pcomn::strslice(str, 0, 15), Sub + 0, 2)) ;
   CPPUNIT_LOG_EQUAL(Sub[0], make_reg_match(4, 13)) ;
   CPPUNIT_LOG_EQUAL(Sub[1], make_reg_match(4, 8)) ;
   CPPUNIT_LOG_EQUAL(Sub[2], make_reg_match(1000, 2000)) ;
}

template<typename R>
void RegexCallModeTests<R>::Test_Call_Index()
{
   regex_type RX ;
   const char * const str = " 15.Hello_344::_World+-990" ;
   const char * const estr = "" ;
   const pcomn::strslice eslice ;

   CPPUNIT_LOG_RUN(RX = regex_type("^$")) ;
   CPPUNIT_LOG_EQUAL(RX.last_submatch_ndx(estr), 0) ;
   CPPUNIT_LOG_EQUAL(RX.last_submatch_ndx(eslice), 0) ;
   CPPUNIT_LOG_EQUAL(RX.last_submatch_ndx(str), -1) ;
   CPPUNIT_LOG_EQUAL(RX.last_submatch_ndx(pcomn::strslice(str)), -1) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(RX = regex_type("^.+(([1-5]+)([A-Z.])*)")) ;
   CPPUNIT_LOG_EQUAL(RX.last_submatch_ndx(str), 2) ;
   CPPUNIT_LOG_EQUAL(RX.last_submatch_ndx(pcomn::strslice(str)), 2) ;
   CPPUNIT_LOG_EQUAL(RX.last_submatch_ndx(pcomn::strslice(str, 0, 7)), 3) ;
}

/*******************************************************************************
 RegexpQuoteTests
*******************************************************************************/
class RegexpQuoteTests : public CppUnit::TestFixture {

      void Test_Regexp_Quote() ;

      CPPUNIT_TEST_SUITE(RegexpQuoteTests) ;

      CPPUNIT_TEST(Test_Regexp_Quote) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

void RegexpQuoteTests::Test_Regexp_Quote()
{
   CPPUNIT_LOG_EQUAL(pcomn::regexp_quote(""), std::string()) ;
   CPPUNIT_LOG_EQUAL(pcomn::regexp_quote("Hello "), std::string("Hello ")) ;
   CPPUNIT_LOG_EQUAL(pcomn::regexp_quote("Hello."), std::string("Hello\\.")) ;
   CPPUNIT_LOG_EQUAL(pcomn::regexp_quote("^a\\.bc([a-z])+?*$"), std::string("\\^a\\\\\\.bc\\(\\[a-z\\]\\)\\+\\?\\*\\$")) ;

   CPPUNIT_LOG_EQUAL(pcomn::regex(pcomn::regexp_quote("^a\\.bc([a-z])+?*$")).match("tyui^a\\.bc([a-z])+?*$kl"),
                     make_reg_match(4,  21)) ;
}

int main(int, char *argv[])
{
   pcomn::unit::TestRunner runner ;

   runner.addTest(RegexTests<char, const char *, regex_test_type>::suite()) ;
   runner.addTest(RegexTests<char, std::string, regex_test_type>::suite()) ;
   runner.addTest(RegexTests<char, pcomn::istring, regex_test_type>::suite()) ;
   runner.addTest(RegexTests<char, pcomn::strslice, regex_test_type>::suite()) ;
   runner.addTest(RegexpQuoteTests::suite()) ;
   runner.addTest(RegexCallModeTests<regex_test_type>::suite()) ;
   return
      !runner.run() ;
}
