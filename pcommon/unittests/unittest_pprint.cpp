/*-*- tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_pprint.cpp
 COPYRIGHT    :   Yakov Markovitch, 2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for pretty printing

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   22 Dec 2015
*******************************************************************************/
#include <pcomn_pprint.h>
#include <pcomn_omanip.h>
#include <pcomn_unittest.h>

#include <vector>
#include <sstream>

using namespace pcomn ;

/*******************************************************************************
                     class PrettyPrintTests
*******************************************************************************/
class PrettyPrintTests : public CppUnit::TestFixture {
   private:
      void Test_PPrint_Ostream() ;

      CPPUNIT_TEST_SUITE(PrettyPrintTests) ;

      CPPUNIT_TEST(Test_PPrint_Ostream) ;

      CPPUNIT_TEST_SUITE_END() ;

   public:
      struct test_omemstream : omemstream {
            size_t overflow_count = 0 ;
            size_t flush_count = 0 ;
         protected:
            int_type overflow(int_type c) override
            {
               ++overflow_count ;
               flush_count += !!traits_type::eq_int_type(c, traits_type::eof()) ;
               return omemstream::overflow(c) ;
            }
      } ;
} ;

void PrettyPrintTests::Test_PPrint_Ostream()
{
   test_omemstream otest01 ;
   pprint_ostream pp01 (otest01) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(PrettyPrintTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv, "unittest.trace.ini",
                             "Testing pretty printer") ;
}
