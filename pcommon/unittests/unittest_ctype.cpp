/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   unittest_ctype.cpp
 COPYRIGHT    :   Yakov Markovitch, 2019. All rights reserved.
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

    CPPUNIT_TEST_SUITE(CharacterHandlingTests) ;

    CPPUNIT_TEST(Test_ASCII_CharTypes) ;

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

int main(int argc, char *argv[])
{
    return pcomn::unit::run_tests
        <
            CharacterHandlingTests
        >
        (argc, argv) ;
}
