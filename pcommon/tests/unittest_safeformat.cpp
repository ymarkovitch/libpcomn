/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_safeformat.cpp
 COPYRIGHT    :   Yakov Markovitch, 2006-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unit test fixture for PCommon safe formats

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   7 Dec 2006
*******************************************************************************/
#include <pcomn_immutablestr.h>
#include <pcomn_safeformat.h>
#include <pcomn_unittest.h>

#include <iostream>
#include <utility>

template<typename Char, size_t n>
inline Char *fill_buffer(Char (&buffer)[n], Char c)
{
   pcomn::raw_fill(buffer + 0, buffer + n - 1, c) ;
   buffer[n - 1] = 0 ;
   return buffer ;
}

template<typename C>
struct TestStrings {
      static const C * const FooBar_Format ;
      static const C * const FooBar_Result ;
      static const C * const Foo ;
      static const C Bar[] ;
} ;

PCOMN_DEFINE_TEST_STR(FooBar_Format, "%d %ss, %.1f %ss") ;
PCOMN_DEFINE_TEST_STR(FooBar_Result, "13 Foos, 0.5 Bars") ;
PCOMN_DEFINE_TEST_STR(Foo, "Foo") ;
PCOMN_DEFINE_TEST_BUF(const, Bar, "Bar") ;

template<typename Char, typename Format>
class SafeFormatTests : public CppUnit::TestFixture {

   public:
      typedef TestStrings<Char> Strings ;
      typedef std::basic_string<Char>        stdstring ;
      typedef pcomn::immutable_string<Char>  istring ;
      typedef Format                         format_type ;

   private:
      void Test_Sprintf_StdString_Buffer() ;
      void Test_Sprintf_Char_Buffer() ;
      void Test_FPrintf() ;
      void Test_Tuple_Strprintf() ;

      CPPUNIT_TEST_SUITE(SafeFormatTests) ;

      CPPUNIT_TEST(Test_Sprintf_StdString_Buffer) ;
      CPPUNIT_TEST(Test_Sprintf_Char_Buffer) ;
      CPPUNIT_TEST(Test_FPrintf) ;
      CPPUNIT_TEST(Test_Tuple_Strprintf) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

/*******************************************************************************
 SafeFormatTests
*******************************************************************************/
template<typename Char, typename Format>
void SafeFormatTests<Char, Format>::Test_Sprintf_StdString_Buffer()
{
   stdstring buffer ;
   const format_type format (Strings::FooBar_Format) ;

   CPPUNIT_LOG_EQUAL((int)pcomn::xsprintf(buffer, format)(13)(Strings::Foo)(0.5)(Strings::Bar),
                     (int)pcomn::str::len(Strings::FooBar_Result)) ;
   CPPUNIT_LOG_EQUAL(buffer, stdstring(Strings::FooBar_Result)) ;
}

template<typename Char, typename Format>
void SafeFormatTests<Char, Format>::Test_Sprintf_Char_Buffer()
{
   Char small_buffer[8] ;
   Char big_buffer[32] ;
   fill_buffer<Char>(small_buffer, 'A') ;
   fill_buffer<Char>(big_buffer, 'A') ;

   const format_type format (Strings::FooBar_Format) ;
   CPPUNIT_LOG_EQUAL((int)pcomn::xsprintf(big_buffer, format)(13)(Strings::Foo)(0.5)(Strings::Bar),
                     (int)pcomn::str::len(Strings::FooBar_Result)) ;
   CPPUNIT_LOG_EQUAL(stdstring(big_buffer), stdstring(Strings::FooBar_Result)) ;

   CPPUNIT_LOG_ASSERT(pcomn::xsprintf(small_buffer, format)(13)(Strings::Foo)(0.5)(Strings::Bar) < 0) ;
   // Check we are nonetheless terminated by 0
   CPPUNIT_LOG_EQUAL(small_buffer[P_ARRAY_COUNT(small_buffer) - 1], Char()) ;

   fill_buffer<Char>(big_buffer, 'A') ;
   Char *char_buffer = big_buffer ;
   CPPUNIT_LOG_EQUAL((int)pcomn::xsprintf(char_buffer, sizeof(big_buffer), format)
                     (13)(Strings::Foo)(0.5)(Strings::Bar),
                     (int)pcomn::str::len(Strings::FooBar_Result)) ;
   CPPUNIT_LOG_EQUAL(stdstring(char_buffer), stdstring(Strings::FooBar_Result)) ;

   fill_buffer<Char>(big_buffer, 'A') ;
   CPPUNIT_LOG_EQUAL((int)pcomn::xsprintf(char_buffer, pcomn::str::len(Strings::FooBar_Result) + 1, format)
                     (13)(Strings::Foo)(0.5)(Strings::Bar),
                     (int)pcomn::str::len(Strings::FooBar_Result)) ;
   CPPUNIT_LOG_EQUAL(stdstring(char_buffer), stdstring(Strings::FooBar_Result)) ;

   fill_buffer<Char>(big_buffer, 'A') ;
   CPPUNIT_LOG_ASSERT(pcomn::xsprintf(char_buffer, pcomn::str::len(Strings::FooBar_Result), format)
                      (13)(Strings::Foo)(0.5)(Strings::Bar) < 0) ;
   // Check we are nonetheless terminated by 0
   CPPUNIT_LOG_EQUAL(big_buffer[P_ARRAY_COUNT(big_buffer) - 1], Char()) ;
}

template<typename Char, typename Format>
void SafeFormatTests<Char, Format>::Test_FPrintf()
{
}


template<typename Char, typename Format>
void SafeFormatTests<Char, Format>::Test_Tuple_Strprintf()
{
   /*
   const format_type format (Strings::FooBar_Format) ;
   CPPUNIT_LOG_EQUAL(pcomn::strprintf_tp<stdstring>
                     (format, PCOMN_TPARGS(13, Strings::Foo, 0.5, Strings::Bar)),
                     stdstring(Strings::FooBar_Result)) ;
   CPPUNIT_LOG_EQUAL(pcomn::strprintf_tp<istring>
                     (format, PCOMN_TPARGS(13, Strings::Foo, 0.5, Strings::Bar)),
                     istring(Strings::FooBar_Result)) ;
   CPPUNIT_LOG_EQUAL(pcomn::strprintf_tp<istring>
                     (format, PCOMN_TPARGS(13, istring(Strings::Foo), 0.5, Strings::Bar)),
                     istring(Strings::FooBar_Result)) ;
   CPPUNIT_LOG_EQUAL(pcomn::strprintf_tp<istring>
                     (format, PCOMN_TPARGS(13, stdstring(Strings::Foo), 0.5, Strings::Bar)),
                     istring(Strings::FooBar_Result)) ;
   */
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(SafeFormatTests<char, const char *>::suite()) ;
   runner.addTest(SafeFormatTests<char, std::string>::suite()) ;
   runner.addTest(SafeFormatTests<char, pcomn::istring>::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "unittest.diag.ini", "Safe format tests") ;
}
