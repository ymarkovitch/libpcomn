/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   unittest_fixringbuf.cpp
 COPYRIGHT    :   Yakov Markovitch, 2022

 DESCRIPTION  :   Unittests for pcomn::fixed_ring_buffer

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   7 May 2022
*******************************************************************************/
#include <pcomn_fixringbuf.h>
#include <pcomn_unittest.h>

using namespace pcomn ;
using namespace pcomn::unit ;

/*******************************************************************************
              class FixedRingBufferTests
*******************************************************************************/
class FixedRingBufferTests : public CppUnit::TestFixture {

    void Test_FixRingBuf_Construct() ;
    void Test_FixRingBuf_Iterators() ;

    CPPUNIT_TEST_SUITE(FixedRingBufferTests) ;

    CPPUNIT_TEST(Test_FixRingBuf_Construct) ;
    CPPUNIT_TEST(Test_FixRingBuf_Iterators) ;

    CPPUNIT_TEST_SUITE_END() ;
} ;

void FixedRingBufferTests::Test_FixRingBuf_Construct()
{
    fixed_ring_buffer<int> ring0 ;
}

void FixedRingBufferTests::Test_FixRingBuf_Iterators()
{
}

/*******************************************************************************
 main
*******************************************************************************/
int main(int argc, char *argv[])
{
    return unit::run_tests
        <
            FixedRingBufferTests
        >
        (argc, argv) ;
}
