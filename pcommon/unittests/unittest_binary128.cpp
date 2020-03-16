/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   unittest_binary128.cpp
 COPYRIGHT    :   Yakov Markovitch, 2019-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for large fixed binary types (128- and 256-bit)

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   12 Nov 2019
*******************************************************************************/
#include <pcomn_binary128.h>
#include <pcomn_unittest.h>

#include <type_traits>
#include <new>

using namespace pcomn ;
using namespace pcomn::unit ;

/*******************************************************************************
              class LargeBinaryTests
*******************************************************************************/
class LargeBinaryTests : public CppUnit::TestFixture {

    void Test_Binary128() ;
    void Test_Binary128_Operators() ;
    void Test_Binary256() ;

    CPPUNIT_TEST_SUITE(LargeBinaryTests) ;

    CPPUNIT_TEST(Test_Binary128) ;
    CPPUNIT_TEST(Test_Binary128_Operators) ;
    CPPUNIT_TEST(Test_Binary256) ;

    CPPUNIT_TEST_SUITE_END() ;
} ;

void LargeBinaryTests::Test_Binary128()
{
    const binary128_t binary128_1 (0xf4, 0x7a, 0xc1, 0x0b, 0x58, 0xcc, 0x43, 0x72,
                                   0xa5, 0x67, 0x0e, 0x02, 0xb2, 0xc3, 0xd4, 0x78) ;

    const binary128_t binary128_2 (0xf47a, 0xc10b, 0x58cc, 0x4372,
                                   0xa567, 0x0e02, 0xb2c3, 0xd478) ;

    const binary128_t binary128_3 (0x123456780a0b0c0dULL,
                                   0x1a1b1c1d2a2b2c2dULL) ;
    (void)binary128_1 ;
    (void)binary128_2 ;
    (void)binary128_3 ;
}

void LargeBinaryTests::Test_Binary128_Operators()
{
}

void LargeBinaryTests::Test_Binary256()
{
    PCOMN_STATIC_CHECK(!binary256_t()) ;
    PCOMN_STATIC_CHECK(binary256_t::size() == 32) ;
    PCOMN_STATIC_CHECK(binary256_t::slen() == 64) ;

    PCOMN_STATIC_CHECK(!binary256_t(0, 0, 0, 0)) ;
    PCOMN_STATIC_CHECK(binary256_t(0, 0, 0, 1)) ;

    static constexpr binary256_t b1(0, 0, 0, 1) ;
    PCOMN_STATIC_CHECK(*(b1.idata() + 3) == 1) ;

    CPPUNIT_LOG_EQUAL(binary256_t(0, 0, 0, 1), binary256_t(0, 0, 0, 1)) ;
    CPPUNIT_LOG_NOT_EQUAL(binary256_t(0, 0, 0, 1), binary256_t()) ;
    CPPUNIT_LOG_NOT_EQUAL(binary256_t(0, 3, 0, 1), binary256_t(0, 0, 0, 1)) ;
    CPPUNIT_LOG_EQUAL(binary256_t(0, 3, 0, 1), binary256_t(0, 3, 0, 1)) ;
    CPPUNIT_LOG_NOT_EQUAL(binary256_t(0, 3, 0, 1), binary256_t(0, 3, 0, 2)) ;

    CPPUNIT_LOG_EQ(string_cast(binary256_t(0, 3, 0, 1)),
                   "0000000000000001000000000000000000000000000000030000000000000000") ;

    CPPUNIT_LOG_EQUAL(binary256_t(string_cast(binary256_t(0, 3, 0, 1)).c_str()), binary256_t(0, 3, 0, 1)) ;
}

/*******************************************************************************
 main
*******************************************************************************/
int main(int argc, char *argv[])
{
    return unit::run_tests
        <
            LargeBinaryTests
        >
        (argc, argv) ;
}
