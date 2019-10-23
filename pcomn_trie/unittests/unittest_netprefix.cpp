/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   unittest_netprefix.cpp
 COPYRIGHT    :   Yakov Markovitch, 2019

 DESCRIPTION  :

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   23 Oct 2019
*******************************************************************************/
#include <pcomn_netprefix.h>
#include <pcomn_unittest.h>

using namespace pcomn ;

/*******************************************************************************
                            class ShortestNetPrefixSetTests
*******************************************************************************/
class ShortestNetPrefixSetTests : public CppUnit::TestFixture {

      void Test_BitTupleSelect() ;

      CPPUNIT_TEST_SUITE(ShortestNetPrefixSetTests) ;

      CPPUNIT_TEST(Test_BitTupleSelect) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

/*******************************************************************************
 ShortestNetPrefixSetTests
*******************************************************************************/
void ShortestNetPrefixSetTests::Test_BitTupleSelect()
{
}

int main(int argc, char *argv[])
{
   return pcomn::unit::run_tests
      <
         ShortestNetPrefixSetTests
      >
      (argc, argv) ;
}
