/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_base64.cpp

 DESCRIPTION  :   Test for base64 decoding

 CREATION DATE:   20 Apr 2018
*******************************************************************************/
#include <pcomn_binascii.h>
#include <pcomn_unittest.h>

class Base64DecondeTests : public CppUnit::TestFixture {

      void Test_Simple() ;
      void Test_PartedSimple() ;
      void Test_SkippedInvalid() ;
      void Test_CheckSizes() ;

      CPPUNIT_TEST_SUITE(Base64DecondeTests) ;

      CPPUNIT_TEST(Test_Simple) ;
      CPPUNIT_TEST(Test_PartedSimple) ;
      CPPUNIT_TEST(Test_SkippedInvalid) ;
      CPPUNIT_TEST(Test_CheckSizes) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

const char base64[] = "0JDQutGG0LjQvtC90LXRgNC90L7QtSDQvtCx0YnQtdGB0YLQstC+ICLQndCw0YPRh9C90L4t0YLQtdGF0L3QuNGH" ;
const char utf8[] = "Акционерное общество \"Научно-технич" ;

#define CPPUNIT_LOG_EQ_STRN(_s1, _s2, _len)                                \
   CPPUNIT_LOG_EQUAL(pcomn::strslice(_s1, ((const char*)(_s1)) + (_len)), \
                     pcomn::strslice(_s2, ((const char*)(_s2)) + (_len)))


std::size_t operator "" _SZ (unsigned long long int x)
{
  return x;
}

/*******************************************************************************
 Base64Test
*******************************************************************************/
void Base64DecondeTests::Test_Simple()
{
   char buf[1024] = {} ;

   size_t base64_len = sizeof(base64)-1 ;
   size_t res_len ;
   CPPUNIT_LOG_EQUAL(res_len = a2b_base64(base64, &base64_len, buf, sizeof(buf)), sizeof(utf8)-1) ;
   CPPUNIT_LOG_EQUAL(base64_len, sizeof(base64)-1) ;
   CPPUNIT_LOG_EQ_STRN(utf8, buf, res_len) ;
}

void Base64DecondeTests::Test_PartedSimple()
{
    char buf[1024] = {} ;
    char *buf_ptr = buf ;
    size_t res ;

    size_t base64_len ;// = sizeof(base64)-1 ;
    //size_t res_len = 0  ;

    base64_len = 18 ;
    CPPUNIT_LOG_EQUAL(res = a2b_base64(base64, &base64_len, buf, sizeof(buf)), 13_SZ) ;
    CPPUNIT_LOG_EQUAL(base64_len, 16_SZ) ;
    CPPUNIT_LOG_EQ_STRN(utf8, buf, res) ;

    base64_len = 17 ;
    CPPUNIT_LOG_EQUAL(res = a2b_base64(base64, &base64_len, buf, sizeof(buf)), 12_SZ) ;
    CPPUNIT_LOG_EQUAL(base64_len, 16_SZ) ;
    CPPUNIT_LOG_EQ_STRN(utf8, buf, res) ;
    buf_ptr += res ;


    base64_len = 9 ;
    CPPUNIT_LOG_EQUAL(res = a2b_base64(base64 + 16, &base64_len, buf_ptr, sizeof(buf)), 6_SZ) ;
    CPPUNIT_LOG_EQUAL(base64_len, 8_SZ) ;
    buf_ptr += res ;
    CPPUNIT_LOG_EQ_STRN(utf8, buf, buf_ptr - buf) ;
}

void Base64DecondeTests::Test_SkippedInvalid()
{
    char buf[1024] = {} ;

    {
        size_t base64_len = sizeof(base64)-1 ;
        size_t res_len ;
        CPPUNIT_LOG_EQUAL(res_len = a2b_base64(base64, &base64_len, buf, sizeof(buf)), sizeof(utf8)-1) ;
        CPPUNIT_LOG_EQUAL(base64_len, sizeof(base64)-1) ;
        CPPUNIT_LOG_EQ_STRN(utf8, buf, res_len) ;
    }

    {
        // not full base64
        const char skip_upto_0[] = "__________0JD__________----------------" ;
        size_t ascii_len = sizeof(skip_upto_0)-1 ;
        size_t res_len ;
        CPPUNIT_LOG_RUN(res_len = a2b_base64(skip_upto_0, &ascii_len, buf, sizeof(buf))) ;
        CPPUNIT_LOG_EQUAL(res_len & ~0x3, 0_SZ) ;
        CPPUNIT_LOG_EQUAL(ascii_len, 10_SZ) ;
        CPPUNIT_LOG_EQUAL(skip_upto_0 + ascii_len, strchr(skip_upto_0, '0')) ;
    }

    {
        // not full base64
        const char skip_upto_0[] = "__________0JDQutGG______=___---------------" ;
        size_t ascii_len = sizeof(skip_upto_0)-1 ;
        size_t res_len ;
        CPPUNIT_LOG_EQUAL(res_len = a2b_base64(skip_upto_0, &ascii_len, buf, sizeof(buf)), 6_SZ) ;
        CPPUNIT_LOG_EQ_STRN(utf8, buf, res_len) ;
        CPPUNIT_LOG_EQUAL(ascii_len, sizeof(skip_upto_0)-1) ;
    }

    {
        const char fullskip[] = "____________________---------------" ;
        size_t ascii_len = sizeof(fullskip)-1 ;
        size_t res_len ;
        CPPUNIT_LOG_EQUAL(res_len = a2b_base64(fullskip, &ascii_len, buf, sizeof(buf)), 0_SZ) ;
        CPPUNIT_LOG_EQUAL(ascii_len, sizeof(fullskip)-1) ;
        ascii_len = sizeof(fullskip)-1 ;
        CPPUNIT_LOG_EQUAL(skip_invalid_base64(fullskip, ascii_len) - fullskip, (ptrdiff_t)ascii_len) ;
    }

    {
        const char full_decode[] = "____________________---------------QQ==" ;
        size_t ascii_len = sizeof(full_decode)-1 ;
        size_t res_len ;
        CPPUNIT_LOG_EQUAL(res_len = a2b_base64(full_decode, &ascii_len, buf, sizeof(buf)), 1_SZ) ;
        CPPUNIT_LOG_EQUAL(*buf, 'A') ;
        CPPUNIT_LOG_EQUAL(ascii_len, sizeof(full_decode)-1 ) ;
        ascii_len = sizeof(full_decode)-1 ;
        CPPUNIT_LOG_EQUAL(skip_invalid_base64(full_decode, ascii_len) - full_decode, (ptrdiff_t)ascii_len - 4) ;
    }

    {
        const char full_decode[] = "_______________Q_____-------Q----=----=" ;
        size_t ascii_len = sizeof(full_decode)-1 ;
        size_t res_len ;
        CPPUNIT_LOG_EQUAL(res_len = a2b_base64(full_decode, &ascii_len, buf, sizeof(buf)), 1_SZ) ;
        CPPUNIT_LOG_EQUAL(*buf, 'A') ;
        CPPUNIT_LOG_EQUAL(ascii_len, sizeof(full_decode)-1 ) ;
        CPPUNIT_LOG_EQUAL(skip_invalid_base64(full_decode, ascii_len), strchr(full_decode, 'Q')) ;
    }

    {
        const char skip_upto_PAD[] = "____________________---------------=" ;
        size_t ascii_len = sizeof(skip_upto_PAD)-1 ;
        size_t res_len ;
        CPPUNIT_LOG_EQUAL(res_len = a2b_base64(skip_upto_PAD, &ascii_len, buf, sizeof(buf)), 0_SZ) ;
        CPPUNIT_LOG_EQUAL(ascii_len, sizeof(skip_upto_PAD)-1 - 1) ;
        CPPUNIT_LOG_EQUAL(skip_upto_PAD + ascii_len, strchr(skip_upto_PAD, '=')) ;
        ascii_len = sizeof(skip_upto_PAD)-1 ;
        CPPUNIT_LOG_EQUAL(skip_invalid_base64(skip_upto_PAD, ascii_len) - skip_upto_PAD, (ptrdiff_t)ascii_len - 1) ;
    }
}

void Base64DecondeTests::Test_CheckSizes()
{
    char buf[1024] = {} ;

    size_t base64_len = sizeof(base64)-1 ;
    size_t res_len ;
    CPPUNIT_LOG_EQUAL(res_len = a2b_base64(base64, &base64_len, buf, 6), 6_SZ) ;
    CPPUNIT_LOG_EQ_STRN(utf8, buf, 6) ;
    CPPUNIT_LOG_EQUAL(base64_len, 8_SZ) ;

    base64_len = sizeof(base64)-1 ;
    CPPUNIT_LOG_EQUAL(res_len = a2b_base64(base64, &base64_len, buf, 5), 5_SZ) ;
    CPPUNIT_LOG_EQ_STRN(utf8, buf, 5) ;
    CPPUNIT_LOG_EQUAL(base64_len, 4_SZ) ;
    CPPUNIT_LOG_EQUAL(base64_len, b2a_strlen_base64((res_len/3)*3)) ;
}


int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(Base64DecondeTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv, "unittest.diag.ini", "pcomn_base64 tests") ;
}
