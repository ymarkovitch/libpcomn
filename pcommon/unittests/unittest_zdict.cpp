/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   unittest_zdict.cpp
 COPYRIGHT    :   Yakov Markovitch, 2019-2020

 DESCRIPTION  :   Test pcommon wrappers for ZSTD dictionary compression

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   8 Oct 2019
*******************************************************************************/
#include <pcomn_zstd.h>
#include <pcomn_string.h>
#include <pcomn_unittest.h>

#include "pcomn_testhelpers.h"

#include <stdio.h>
#include <typeinfo>
#include <fstream>

using namespace pcomn ;

extern const char ZDICTTESTS[] = "zdict" ;

/*******************************************************************************
                     class ZDictTests
*******************************************************************************/
class ZDictTests : public unit::TestFixture<ZDICTTESTS> {

    void Test_MakeDict() ;

    CPPUNIT_TEST_SUITE(ZDictTests) ;

    CPPUNIT_TEST(Test_MakeDict) ;

    CPPUNIT_TEST_SUITE_END() ;
} ;

void ZDictTests::Test_MakeDict()
{
}

int main(int argc, char *argv[])
{
    return pcomn::unit::run_tests
        <
            ZDictTests
        >
        (argc, argv) ;
}
