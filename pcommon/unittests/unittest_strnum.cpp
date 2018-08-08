/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_strnum.cpp
 COPYRIGHT    :   Yakov Markovitch, 2006-2018. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests of conversions string<->integer

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   7 May 2008
*******************************************************************************/
#include <pcomn_strnum.h>
#include <pcomn_unittest.h>

#include <vector>
#include <iterator>

class StrNumTests : public CppUnit::TestFixture {

      void Test_NumToStr() ;
      void Test_NumToIter() ;
      void Test_StrToNum() ;

      CPPUNIT_TEST_SUITE(StrNumTests) ;

      CPPUNIT_TEST(Test_NumToStr) ;
      CPPUNIT_TEST(Test_NumToIter) ;
      CPPUNIT_TEST(Test_StrToNum) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

/*******************************************************************************
 StrNumTests
*******************************************************************************/
void StrNumTests::Test_NumToStr()
{
   char buf1[1] ;
   char buf6[6] ;
   char buf65[65] ;
   wchar_t wbuf1[1] ;
   wchar_t wbuf6[6] ;

   CPPUNIT_LOG_EQUAL(pcomn::numtostr(15, (char *)NULL, 10), (char *)NULL) ;
   CPPUNIT_LOG_EQUAL(pcomn::numtostr(15, (char *)NULL, 0), (char *)NULL) ;

   CPPUNIT_LOG(std::endl) ;
   pcomn::unit::fillbuf(buf1) ;
   CPPUNIT_LOG_EQUAL(pcomn::numtostr(15, buf1), buf1 + 0) ;
   CPPUNIT_LOG_EQUAL(*buf1, '\0') ;
   pcomn::unit::fillbuf(wbuf1) ;
   CPPUNIT_LOG_EQUAL(pcomn::numtostr(15, wbuf1), wbuf1 + 0) ;
   CPPUNIT_LOG_EQUAL(*wbuf1, L'\0') ;

   pcomn::unit::fillbuf(buf6) ;
   CPPUNIT_LOG_EQUAL(pcomn::numtostr(123456789, buf6), buf6 + 0) ;
   CPPUNIT_LOG_EQUAL(buf6[5], '\0') ;
   CPPUNIT_LOG_EQUAL(std::string(buf6), std::string("12345")) ;
   pcomn::unit::fillbuf(wbuf6) ;
   CPPUNIT_LOG_EQUAL(pcomn::numtostr(123456789, wbuf6), wbuf6 + 0) ;
   CPPUNIT_LOG_EQUAL(wbuf6[5], L'\0') ;
   CPPUNIT_LOG_EQUAL(std::wstring(wbuf6), std::wstring(L"12345")) ;

   CPPUNIT_LOG(std::endl) ;
   pcomn::unit::fillbuf(buf65) ;
   CPPUNIT_LOG_EQUAL(std::string(pcomn::numtostr(37, buf65)), std::string("37")) ;
   CPPUNIT_LOG_EQUAL(std::string(pcomn::numtostr(37, buf65, 0)), std::string("37")) ;
   CPPUNIT_LOG_EQUAL(std::string(pcomn::numtostr(-37, buf65, 0)), std::string("-37")) ;
   CPPUNIT_LOG_EQUAL(std::string(pcomn::numtostr(37, buf65, 1)), std::string()) ;
   CPPUNIT_LOG_EQUAL(std::string(pcomn::numtostr(37, buf65, 37)), std::string()) ;
   CPPUNIT_LOG_EQUAL(std::string(pcomn::numtostr(37, buf65, 2)), std::string("100101")) ;
   CPPUNIT_LOG_EQUAL(std::string(pcomn::numtostr(37, buf65, 16)), std::string("25")) ;
   CPPUNIT_LOG_EQUAL(std::string(pcomn::numtostr(-37, buf65, 16)), std::string("-25")) ;
   CPPUNIT_LOG_EQUAL(std::string(pcomn::numtostr(37, buf65, 36)), std::string("11")) ;
   CPPUNIT_LOG_EQUAL(std::string(pcomn::numtostr(-37, buf65, 36)), std::string("-11")) ;
   CPPUNIT_LOG_EQUAL(std::string(pcomn::numtostr(71, buf65, 36)), std::string("1Z")) ;
   CPPUNIT_LOG_EQUAL(std::string(pcomn::numtostr(-71, buf65, 36)), std::string("-1Z")) ;
   CPPUNIT_LOG_EQUAL(std::string(pcomn::numtostr(0xFFFFFFFFFFFFFFFFULL, buf65, 2)),
                     std::string("1111111111111111111111111111111111111111111111111111111111111111")) ;
   CPPUNIT_LOG_EQUAL(std::string(pcomn::numtostr(-0x7FFFFFFFFFFFFFFFLL, buf65, 2)),
                     std::string("-111111111111111111111111111111111111111111111111111111111111111")) ;

   CPPUNIT_LOG_EQUAL(pcomn::strslice(PCOMN_NUMTOSTR(-0x7FFFFFFFFFFFFFFFLL, 2)),
                     pcomn::strslice("-111111111111111111111111111111111111111111111111111111111111111")) ;

   CPPUNIT_LOG_EQUAL(pcomn::numtostr<std::string>(-37, 16), std::string("-25")) ;
   CPPUNIT_LOG_EQUAL(pcomn::numtostr<std::string>(-37), std::string("-37")) ;

   CPPUNIT_LOG_EQUAL(pcomn::strslice(PCOMN_NUMTOSTR10(-37)), pcomn::strslice("-37")) ;
}

void StrNumTests::Test_NumToIter()
{
   std::vector<char> Container ;
   std::back_insert_iterator<std::vector<char> > Inserter (Container) ;

   CPPUNIT_LOG_RUN(pcomn::numtoiter(0, Inserter)) ;
   CPPUNIT_LOG_EQUAL(Container.size(), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(*Container.begin(), '0') ;
   CPPUNIT_LOG_RUN(Container.clear()) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(pcomn::numtoiter(15, Inserter, 16)) ;
   CPPUNIT_LOG_EQUAL(Container.size(), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(*Container.begin(), 'F') ;
   CPPUNIT_LOG_RUN(Container.clear()) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(pcomn::numtoiter(-37, Inserter)) ;
   CPPUNIT_LOG_EQUAL(Container.size(), (size_t)3) ;
   CPPUNIT_LOG_EQUAL(std::string(Container.begin(), Container.end()), std::string("-37")) ;
   CPPUNIT_LOG_RUN(Container.clear()) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(pcomn::numtoiter(0xFFFFFFFFFFFFFFFFULL, Inserter, 2)) ;
   CPPUNIT_LOG_EQUAL(Container.size(), (size_t)64) ;
   CPPUNIT_LOG_EQUAL(std::string(Container.begin(), Container.end()),
                     std::string("1111111111111111111111111111111111111111111111111111111111111111")) ;
   CPPUNIT_LOG_RUN(Container.clear()) ;
   CPPUNIT_LOG_RUN(pcomn::numtoiter(-0x7FFFFFFFFFFFFFFFLL, Inserter, 2)) ;
   CPPUNIT_LOG_EQUAL(Container.size(), (size_t)64) ;
   CPPUNIT_LOG_EQUAL(std::string(Container.begin(), Container.end()),
                     std::string("-111111111111111111111111111111111111111111111111111111111111111")) ;
   CPPUNIT_LOG_RUN(Container.clear()) ;
}

void StrNumTests::Test_StrToNum()
{
   CPPUNIT_LOG_EQUAL(pcomn::strtonum<int>("0"), 0) ;
   CPPUNIT_LOG_EQUAL(pcomn::strtonum<int>("123"), 123) ;
   CPPUNIT_LOG_EQUAL(pcomn::strtonum<int>("-123"), -123) ;
   CPPUNIT_LOG_EQUAL(pcomn::strtonum_def<int>("-123 ", 0), -123) ;

   CPPUNIT_LOG_EQUAL(pcomn::strtonum<long long>("0"), 0LL) ;
   CPPUNIT_LOG_EQUAL(pcomn::strtonum<long long>("123"), 123LL) ;
   CPPUNIT_LOG_EQUAL(pcomn::strtonum<long long>("-123 "), -123LL) ;
   CPPUNIT_LOG_EQUAL(pcomn::strtonum_def<long long>("-123 ", 0), -123LL) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(StrNumTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "unittest.diag.ini", "Test of conversions string<->integer") ;
}
