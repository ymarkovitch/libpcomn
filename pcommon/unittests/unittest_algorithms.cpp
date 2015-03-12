/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_algorithms.cpp
 COPYRIGHT    :   Yakov Markovitch, 2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for container algorithms, etc.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   10 Mar 2015
*******************************************************************************/
#include <pcomn_unittest.h>
#include <pcomn_calgorithm.h>

#include <vector>
#include <map>

/*******************************************************************************
                     class CAlgorithmsTests
*******************************************************************************/
class CAlgorithmsTests : public CppUnit::TestFixture {
   private:
      void Test_OSequence() ;

      CPPUNIT_TEST_SUITE(CAlgorithmsTests) ;

      CPPUNIT_TEST(Test_OSequence) ;

      CPPUNIT_TEST_SUITE_END() ;
   protected:
      const std::vector<std::string>   strvec {"zero", "one", "two", "three"} ;
      const std::vector<int>           intvec {1, 3, 5, 7, 11} ;
} ;

void CAlgorithmsTests::Test_OSequence()
{
   CPPUNIT_LOG_ASSERT(pcomn::both_ends(strvec) ==
                      pcomn::unipair<std::vector<std::string>::const_iterator>(strvec.begin(), strvec.end())) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(CAlgorithmsTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv, "unittest.trace.ini",
                             "Testing algorithms") ;
}
