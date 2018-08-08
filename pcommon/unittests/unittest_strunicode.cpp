/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_strunicode.cpp
 COPYRIGHT    :   Yakov Markovitch, 2007-2018. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittest for pcommon string conversions char <-> wchar_t

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   6 Jul 2007
*******************************************************************************/
#include <pcomn_string.h>
#include <pcomn_immutablestr.h>
#include <pcomn_unittest.h>

#ifdef PCOMN_PL_WINDOWS
#include <windows.h>
#define LOCALE_C  LANG_NEUTRAL
#define LOCALE_RU LANG_RUSSIAN
#define LOCALE_EN LANG_ENGLISH
#define LOCALE_DE LANG_GERMAN

#define LOCALE_RU_WIN LANG_RUSSIAN
#define LOCALE_DE_WIN LANG_GERMAN
#define SETLOCALE(loc) (SetThreadLocale(MAKELCID(MAKELANGID((loc), SUBLANG_NEUTRAL), SORT_DEFAULT)))
#else
#include <locale.h>
#define LOCALE_C "C"
#define LOCALE_RU "ru_RU.UTF8"
#define LOCALE_EN "en_US.UTF8"
#define LOCALE_DE "de_DE.UTF8"

#define LOCALE_RU_WIN "ru_RU.cp1251"
#define LOCALE_DE_WIN "de_DE" // ISO-8859-1
#define SETLOCALE(loc) (setlocale(LC_ALL, (loc)))
#endif

/*******************************************************************************
                     class StringTraitsTests
*******************************************************************************/
class StringConversionTests : public CppUnit::TestFixture {

      void Test_CharWchar_EmptyStr_Conversion() ;
      void Test_CharWchar_Conversion() ;
      void Test_CharWchar_LongStr_Conversion() ;

      CPPUNIT_TEST_SUITE(StringConversionTests) ;

      CPPUNIT_TEST(Test_CharWchar_EmptyStr_Conversion) ;
      CPPUNIT_TEST(Test_CharWchar_Conversion) ;
      CPPUNIT_TEST(Test_CharWchar_LongStr_Conversion) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;


/*******************************************************************************
 StringConversionTests
*******************************************************************************/
void StringConversionTests::Test_CharWchar_EmptyStr_Conversion()
{
   CPPUNIT_LOG_ASSERT(SETLOCALE(LOCALE_C)) ;
   CPPUNIT_LOG_EQUAL(std::wstring(), pcomn::stdstr<wchar_t>("")) ;
   CPPUNIT_LOG_EQUAL(std::wstring(), pcomn::stdstr<wchar_t>(L"")) ;
   CPPUNIT_LOG_EQUAL(std::string(), pcomn::stdstr<char>("")) ;
   CPPUNIT_LOG_EQUAL(std::string(), pcomn::stdstr<char>(L"")) ;
   CPPUNIT_LOG_EQUAL(std::wstring(), pcomn::stdstr<wchar_t>(L"")) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(SETLOCALE(LOCALE_DE)) ;
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<char>(L""), std::string()) ;
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<wchar_t>(""), std::wstring()) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(SETLOCALE(LOCALE_DE_WIN)) ;
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<char>(L""), std::string()) ;
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<wchar_t>(""), std::wstring()) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(SETLOCALE(LOCALE_RU)) ;
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<char>(L""), std::string()) ;
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<wchar_t>(""), std::wstring()) ;
   // Identity conversion
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<wchar_t>(L""), std::wstring()) ;
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<char>(""), std::string()) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(SETLOCALE(LOCALE_RU_WIN)) ;
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<char>(L""), std::string()) ;
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<wchar_t>(""), std::wstring()) ;
   // Identity conversion
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<wchar_t>(L""), std::wstring()) ;
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<char>(""), std::string()) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(SETLOCALE(LOCALE_EN)) ;
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<char>(L""), std::string()) ;
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<wchar_t>(""), std::wstring()) ;
   // Identity conversion
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<wchar_t>(L""), std::wstring()) ;
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<char>(""), std::string()) ;
}

void StringConversionTests::Test_CharWchar_Conversion()
{
   CPPUNIT_LOG_ASSERT(SETLOCALE(LOCALE_C)) ;
   CPPUNIT_LOG_EQUAL(std::wstring(L"Hello, world!"), pcomn::stdstr<wchar_t>("Hello, world!")) ;
   CPPUNIT_LOG_EQUAL(std::wstring(L"Hello, world!"), pcomn::stdstr<wchar_t>(L"Hello, world!")) ;
   CPPUNIT_LOG_EQUAL(std::string("Hello, world!"), pcomn::stdstr<char>("Hello, world!")) ;
   CPPUNIT_LOG_EQUAL(std::string("Hello, world!"), pcomn::stdstr<char>(L"Hello, world!")) ;
   CPPUNIT_LOG_EQUAL(std::wstring(L"Hello, world!"), pcomn::stdstr<wchar_t>(L"Hello, world!")) ;

   CPPUNIT_LOG_EQUAL(std::wstring(L"Hello, world!"), pcomn::stdstr<wchar_t>("Hello, world!")) ;
   CPPUNIT_LOG_EQUAL(std::wstring(L"Hello, world!"), pcomn::stdstr<wchar_t>(L"Hello, world!")) ;
   CPPUNIT_LOG_EQUAL(std::string("Hello, world!"), pcomn::stdstr<char>("Hello, world!")) ;
   CPPUNIT_LOG_EQUAL(std::string("Hello, world!"), pcomn::stdstr<char>(L"Hello, world!")) ;
   CPPUNIT_LOG_EQUAL(std::wstring(L"Hello, world!"), pcomn::stdstr<wchar_t>(L"Hello, world!")) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(SETLOCALE(LOCALE_DE)) ;
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<char>(PCOMN_HELLO_WORLD_DE_UCS),
                     std::string(PCOMN_HELLO_WORLD_DE_CHAR)) ;
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<wchar_t>(PCOMN_HELLO_WORLD_DE_CHAR),
                     std::wstring(PCOMN_HELLO_WORLD_DE_UCS)) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(SETLOCALE(LOCALE_DE_WIN)) ;
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<char>(PCOMN_HELLO_WORLD_DE_UCS),
                     std::string(PCOMN_HELLO_WORLD_DE_ISO8859_1)) ;
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<wchar_t>(PCOMN_HELLO_WORLD_DE_ISO8859_1),
                     std::wstring(PCOMN_HELLO_WORLD_DE_UCS)) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(SETLOCALE(LOCALE_RU)) ;
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<char>(PCOMN_HELLO_WORLD_RU_UCS),
                     std::string(PCOMN_HELLO_WORLD_RU_CHAR)) ;
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<wchar_t>(PCOMN_HELLO_WORLD_RU_CHAR),
                     std::wstring(PCOMN_HELLO_WORLD_RU_UCS)) ;
   // Identity conversion
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<wchar_t>(PCOMN_HELLO_WORLD_RU_UCS),
                     std::wstring(PCOMN_HELLO_WORLD_RU_UCS)) ;
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<char>(PCOMN_HELLO_WORLD_RU_CHAR),
                     std::string(PCOMN_HELLO_WORLD_RU_CHAR)) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(SETLOCALE(LOCALE_RU_WIN)) ;
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<char>(PCOMN_HELLO_WORLD_RU_UCS),
                     std::string(PCOMN_HELLO_WORLD_RU_1251)) ;
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<wchar_t>(PCOMN_HELLO_WORLD_RU_1251),
                     std::wstring(PCOMN_HELLO_WORLD_RU_UCS)) ;
   // Identity conversion
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<wchar_t>(PCOMN_HELLO_WORLD_RU_UCS),
                     std::wstring(PCOMN_HELLO_WORLD_RU_UCS)) ;
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<char>(PCOMN_HELLO_WORLD_RU_1251),
                     std::string(PCOMN_HELLO_WORLD_RU_1251)) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(SETLOCALE(LOCALE_EN)) ;
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<char>(PCOMN_HELLO_WORLD_EN_UCS),
                     std::string(PCOMN_HELLO_WORLD_EN_CHAR)) ;
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<wchar_t>(PCOMN_HELLO_WORLD_EN_CHAR),
                     std::wstring(PCOMN_HELLO_WORLD_EN_UCS)) ;
   // Identity conversion
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<wchar_t>(PCOMN_HELLO_WORLD_EN_UCS),
                     std::wstring(PCOMN_HELLO_WORLD_EN_UCS)) ;
   CPPUNIT_LOG_EQUAL(pcomn::stdstr<char>(PCOMN_HELLO_WORLD_EN_CHAR),
                     std::string(PCOMN_HELLO_WORLD_EN_CHAR)) ;
}

template<typename Char>
std::basic_string<Char> strrepeat(const std::basic_string<Char> &src, unsigned count)
{
   if (!count)
      return std::basic_string<Char>() ;
   const size_t sz = src.size() ;
   if (count == 1 || !sz)
      return src ;
   std::basic_string<Char> result ;
   result.reserve(count * sz) ;
   unsigned c = count ;
   do result += src ;
   while (--c) ;
   return result ;
}

void StringConversionTests::Test_CharWchar_LongStr_Conversion()
{
   CPPUNIT_LOG_EQUAL(strrepeat(std::string(), 0), std::string()) ;
   CPPUNIT_LOG_EQUAL(strrepeat(std::string(), 1), std::string()) ;
   CPPUNIT_LOG_EQUAL(strrepeat(std::string(), 3), std::string()) ;
   CPPUNIT_LOG_EQUAL(strrepeat(std::string("Hello, world!"), 0), std::string()) ;
   CPPUNIT_LOG_EQUAL(strrepeat(std::string("Hello, world!"), 1), std::string("Hello, world!")) ;
   CPPUNIT_LOG_EQUAL(strrepeat(std::string("Hello, world!\n"), 3),
                     std::string("Hello, world!\nHello, world!\nHello, world!\n")) ;

   CPPUNIT_LOG_ASSERT(SETLOCALE(LOCALE_C)) ;
   std::wstring WideHelloWorld_En_5121
      (strrepeat(std::wstring(L"Hello, world!\n"), 500), 0, 5121) ;
   std::wstring WideHelloWorld_En_1024 (WideHelloWorld_En_5121, 0, 1024) ;
   std::wstring WideHelloWorld_En_1023 (WideHelloWorld_En_5121, 0, 1023) ;
   std::wstring WideHelloWorld_En_1025 (WideHelloWorld_En_5121, 0, 1025) ;
   std::wstring WideHelloWorld_En_5120 (WideHelloWorld_En_5121, 0, 5120) ;
   std::wstring WideHelloWorld_En_5119 (WideHelloWorld_En_5121, 0, 5119) ;

   std::wstring WideHelloWorld_Ru_5121
      (strrepeat(std::wstring(PCOMN_HELLO_WORLD_RU_UCS L"\n"), 500), 0, 5121) ;
   std::wstring WideHelloWorld_Ru_1024 (WideHelloWorld_Ru_5121, 0, 1024) ;
   std::wstring WideHelloWorld_Ru_1023 (WideHelloWorld_Ru_5121, 0, 1023) ;
   std::wstring WideHelloWorld_Ru_1025 (WideHelloWorld_Ru_5121, 0, 1025) ;
   std::wstring WideHelloWorld_Ru_5120 (WideHelloWorld_Ru_5121, 0, 5120) ;
   std::wstring WideHelloWorld_Ru_5119 (WideHelloWorld_Ru_5121, 0, 5119) ;

   std::string HelloWorld_En_5121
      (strrepeat(std::string("Hello, world!\n"), 500), 0, 5121) ;
   std::string HelloWorld_En_1024 (HelloWorld_En_5121, 0, 1024) ;
   std::string HelloWorld_En_1023 (HelloWorld_En_5121, 0, 1023) ;
   std::string HelloWorld_En_1025 (HelloWorld_En_5121, 0, 1025) ;
   std::string HelloWorld_En_5120 (HelloWorld_En_5121, 0, 5120) ;
   std::string HelloWorld_En_5119 (HelloWorld_En_5121, 0, 5119) ;

   std::string HelloWorld_Ru_5121
      (strrepeat(std::string(PCOMN_HELLO_WORLD_RU_1251 "\n"), 500), 0, 5121) ;
   std::string HelloWorld_Ru_1024 (HelloWorld_Ru_5121, 0, 1024) ;
   std::string HelloWorld_Ru_1023 (HelloWorld_Ru_5121, 0, 1023) ;
   std::string HelloWorld_Ru_1025 (HelloWorld_Ru_5121, 0, 1025) ;
   std::string HelloWorld_Ru_5120 (HelloWorld_Ru_5121, 0, 5120) ;
   std::string HelloWorld_Ru_5119 (HelloWorld_Ru_5121, 0, 5119) ;

   #define CPPUNIT_TEST_STR_N(lang, length)                                \
   {                                                                 \
      CPPUNIT_LOG_EQUAL(WideHelloWorld_##lang##_##length,                  \
                        pcomn::stdstr<wchar_t>(HelloWorld_##lang##_##length.c_str())) ; \
      CPPUNIT_LOG_EQUAL(WideHelloWorld_##lang##_##length,               \
                        pcomn::stdstr<wchar_t>(WideHelloWorld_##lang##_##length.c_str())) ; \
      CPPUNIT_LOG_EQUAL(HelloWorld_##lang##_##length,                   \
                        pcomn::stdstr<char>(HelloWorld_##lang##_##length.c_str())) ; \
      CPPUNIT_LOG_EQUAL(HelloWorld_##lang##_##length,                   \
                        pcomn::stdstr<char>(WideHelloWorld_##lang##_##length.c_str())) ; \
      CPPUNIT_LOG_EQUAL(WideHelloWorld_##lang##_##length,               \
                        pcomn::stdstr<wchar_t>(WideHelloWorld_##lang##_##length.c_str())) ; \
   }

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_TEST_STR_N(En, 1023) ;
   CPPUNIT_TEST_STR_N(En, 1024) ;
   CPPUNIT_TEST_STR_N(En, 1025) ;
   CPPUNIT_TEST_STR_N(En, 5119) ;
   CPPUNIT_TEST_STR_N(En, 5120) ;
   CPPUNIT_TEST_STR_N(En, 5121) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(SETLOCALE(LOCALE_RU_WIN)) ;
   CPPUNIT_TEST_STR_N(Ru, 1023) ;
   CPPUNIT_TEST_STR_N(Ru, 1024) ;
   CPPUNIT_TEST_STR_N(Ru, 1025) ;
   CPPUNIT_TEST_STR_N(Ru, 5119) ;
   CPPUNIT_TEST_STR_N(Ru, 5120) ;
   CPPUNIT_TEST_STR_N(Ru, 5121) ;

#ifndef PCOMN_PL_WINDOWS
   CPPUNIT_LOG(std::endl) ;

   std::wstring WideHelloWorld_RuUtf8_X200
      (strrepeat(std::wstring(PCOMN_HELLO_WORLD_RU_UCS L"\n"), 200)) ;
   std::string  HelloWorld_RuUtf8_X200
      (strrepeat(std::string(PCOMN_HELLO_WORLD_RU_UTF8 "\n"), 200)) ;

   CPPUNIT_LOG_ASSERT(SETLOCALE(LOCALE_RU)) ;
   CPPUNIT_TEST_STR_N(RuUtf8, X200) ;
#endif
}

#ifdef PCOMN_PL_UNIX
#include <error.h>
static void CheckLocalePresence(const char *locname)
{
   if (!SETLOCALE(locname))
      error(1, 0, "This test requires locale '%s', which is absent on this system.", locname) ;
}
#endif

int main(int argc, char *argv[])
{
   // Check for locale presence first
   #ifdef PCOMN_PL_UNIX
   CheckLocalePresence(LOCALE_RU) ;
   CheckLocalePresence(LOCALE_DE) ;
   CheckLocalePresence(LOCALE_RU_WIN) ;
   CheckLocalePresence(LOCALE_DE_WIN) ;
   CheckLocalePresence(LOCALE_EN) ;
   #endif

   pcomn::unit::TestRunner runner ;
   runner.addTest(StringConversionTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "unittest.diag.ini", "String shims tests") ;
}
