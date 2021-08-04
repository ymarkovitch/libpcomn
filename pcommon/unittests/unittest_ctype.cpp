/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   unittest_ctype.cpp
 COPYRIGHT    :   Yakov Markovitch, 2019-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for ASCII character types, string<->hex conversions, etc.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   2 Jul 2019
*******************************************************************************/
#include <pcommon.h>
#include <pcomn_unittest.h>

#include <utility>

/*******************************************************************************
 Character handling tests
*******************************************************************************/
class CharacterHandlingTests : public CppUnit::TestFixture {

    void Test_ASCII_CharTypes() ;
    void Test_Hex_Conversions() ;

    CPPUNIT_TEST_SUITE(CharacterHandlingTests) ;

    CPPUNIT_TEST(Test_ASCII_CharTypes) ;
    CPPUNIT_TEST(Test_Hex_Conversions) ;

    CPPUNIT_TEST_SUITE_END() ;
} ;

using namespace pcomn ;

/*******************************************************************************
 CharacterHandlingTests
*******************************************************************************/
void CharacterHandlingTests::Test_ASCII_CharTypes()
{
    const auto charclass = [](auto test)
    {
        std::string str ;
        for (unsigned c = 1 ; c < 256 ; ++c)
            if (test((char)c))
                str.push_back((char)c) ;
        return str ;
    } ;

    CPPUNIT_LOG_IS_FALSE(isxdigit_ascii(0)) ;
    CPPUNIT_LOG_EQ(charclass([](int c) { return isxdigit_ascii(c) ; }), "0123456789ABCDEFabcdef") ;

    CPPUNIT_LOG_IS_FALSE(islower_ascii(0)) ;
    CPPUNIT_LOG_EQ(charclass([](int c) { return islower_ascii(c) ; }), "abcdefghijklmnopqrstuvwxyz") ;

    CPPUNIT_LOG_IS_FALSE(isupper_ascii(0)) ;
    CPPUNIT_LOG_EQ(charclass([](int c) { return isupper_ascii(c) ; }), "ABCDEFGHIJKLMNOPQRSTUVWXYZ") ;

    CPPUNIT_LOG_IS_FALSE(isalpha_ascii(0)) ;
    CPPUNIT_LOG_EQ(charclass([](int c) { return isalpha_ascii(c) ; }), "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz") ;

    CPPUNIT_LOG_IS_FALSE(isalnum_ascii(0)) ;
    CPPUNIT_LOG_EQ(charclass([](int c) { return isalnum_ascii(c) ; }), "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz") ;
}

void CharacterHandlingTests::Test_Hex_Conversions()
{
    std::vector<int> hexsample (256, -1) ;

    for (int c = 0 ; c < 0xA ; ++c)
        hexsample['0' + c] = c ;

    for (int c = 0 ; c < 6 ; ++c)
        hexsample['a' + c] = hexsample['A' + c] = 0xA + c ;

    std::vector<int> hexint ;
    for (int c = 0 ; c < 256 ; ++c)
        hexint.push_back(hexchartoi(c)) ;

    CPPUNIT_LOG_EQUAL(hexint, hexsample) ;
    CPPUNIT_LOG_EQUAL(hexchartoi(256), -1) ;
    CPPUNIT_LOG_EQUAL(hexchartoi(-1), -1) ;
    CPPUNIT_LOG_EQUAL(hexchartoi(-2), -1) ;
    CPPUNIT_LOG_EQUAL(hexchartoi(std::numeric_limits<int>::min()), -1) ;
    CPPUNIT_LOG_EQUAL(hexchartoi(std::numeric_limits<int>::max()), -1) ;
}

int main(int argc, char *argv[])
{
    return pcomn::unit::run_tests
        <
            CharacterHandlingTests
        >
        (argc, argv) ;
}
