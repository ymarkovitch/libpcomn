/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   unittest_memringbuf.cpp
 COPYRIGHT    :   Yakov Markovitch, 2022

 DESCRIPTION  :   Unittests for pcomn::memory_ring_buffer

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   11 May 2022
*******************************************************************************/
#include <pcomn_memringbuf.h>
#include <pcomn_unittest.h>

using namespace pcomn ;
using namespace pcomn::unit ;

/*******************************************************************************
              class MemoryRingTestss
*******************************************************************************/
class MemoryRingTestss : public CppUnit::TestFixture {

    void Test_Memring_Create() ;
    void Test_Memring_Alloc() ;

    CPPUNIT_TEST_SUITE(MemoryRingTestss) ;

    CPPUNIT_TEST(Test_Memring_Create) ;
    CPPUNIT_TEST(Test_Memring_Alloc) ;

    CPPUNIT_TEST_SUITE_END() ;
} ;

void MemoryRingTestss::Test_Memring_Create()
{
}

void MemoryRingTestss::Test_Memring_Alloc()
{
}

/*******************************************************************************
 main
*******************************************************************************/
int main(int argc, char *argv[])
{
    return unit::run_tests
        <
            MemoryRingTestss
        >
        (argc, argv) ;
}
