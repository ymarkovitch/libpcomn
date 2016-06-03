/*-*- tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_hazardptr.cpp
 COPYRIGHT    :   Yakov Markovitch, 2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for hazard pointers.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   20 May 2016
*******************************************************************************/
#include <pcomn_hazardptr.h>
#include <pcomn_unittest.h>

using namespace pcomn ;

class HazardPointerTests : public CppUnit::TestFixture {

      void Test_HazardStorage_Init() ;
      void Test_HazardPointer_Init() ;

      CPPUNIT_TEST_SUITE(HazardPointerTests) ;

      CPPUNIT_TEST(Test_HazardStorage_Init) ;
      CPPUNIT_TEST(Test_HazardPointer_Init) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

/*******************************************************************************
 HazardPointerTests
*******************************************************************************/
void HazardPointerTests::Test_HazardStorage_Init()
{
   //typedef hazard_storage<0> storage_type ;

   auto st1 = new_hazard_storage<0>() ;

   CPPUNIT_LOG_EQ(st1->capacity(), HAZARD_DEFAULT_THREADCOUNT) ;
}

void HazardPointerTests::Test_HazardPointer_Init()
{
}

/*******************************************************************************

*******************************************************************************/
int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(HazardPointerTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv, nullptr, "Hazard pointers tests") ;
}
