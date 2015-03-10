/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_omanip.cpp
 COPYRIGHT    :   Yakov Markovitch, 2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for binary iostreams.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   10 Mar 2015
*******************************************************************************/
#include <pcomn_omanip.h>
#include <pcomn_unittest.h>
#include <pcomn_tuple.h>

#include <vector>
#include <sstream>

/*******************************************************************************
                     class OmanipTests
*******************************************************************************/
class OmanipTests : public CppUnit::TestFixture {
   private:
      void Test_OSequence() ;

      CPPUNIT_TEST_SUITE(OmanipTests) ;

      CPPUNIT_TEST(Test_OSequence) ;

      CPPUNIT_TEST_SUITE_END() ;
   protected:
      const std::vector<std::string>   strvec {"zero", "one", "two", "three"} ;
      const std::vector<int>           intvec {1, 3, 5, 7, 11} ;
} ;

void OmanipTests::Test_OSequence()
{
   {
      std::ostringstream os ;
      CPPUNIT_LOG_ASSERT(os << pcomn::osequence(strvec.begin() + 0, strvec.begin() + 2)) ;
      CPPUNIT_LOG_EQ(os.str(), "zero\none\n") ;
   }
   {
      std::ostringstream os ;
      CPPUNIT_LOG_ASSERT(os << pcomn::osequence(strvec.begin() + 0, strvec.begin() + 2, "->")) ;
      CPPUNIT_LOG_EQ(os.str(), "zero->one->") ;
   }
   {
      std::ostringstream os ;
      CPPUNIT_LOG_ASSERT(os << pcomn::osequence(strvec.begin() + 0, strvec.begin() + 2, 0)) ;
      CPPUNIT_LOG_EQ(os.str(), "zero0one0") ;
   }
   {
      std::ostringstream os ;
      CPPUNIT_LOG_ASSERT(os << pcomn::osequence(strvec.begin() + 0, strvec.begin() + 2, "[[", "]]")) ;
      CPPUNIT_LOG_EQ(os.str(), "[[zero]][[one]]") ;
   }
   {
      std::ostringstream os ;
      CPPUNIT_LOG_ASSERT(os << pcomn::ocontdelim(intvec, ":-:")) ;
      CPPUNIT_LOG_EQ(os.str(), "1:-:3:-:5:-:7:-:11") ;
   }
   {
      std::ostringstream os ;
      CPPUNIT_LOG_ASSERT(os << pcomn::ocontdelim(strvec)) ;
      CPPUNIT_LOG_EQ(os.str(), "zero, one, two, three") ;
   }
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(OmanipTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv, "unittest.trace.ini",
                             "Testing ostream manipulators") ;
}
