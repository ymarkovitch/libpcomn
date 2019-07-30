/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_enum.cpp
 COPYRIGHT    :   Yakov Markovitch, 2016-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittest for describing enums

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   12 Oct 2016
*******************************************************************************/
#include <pcomn_flgout.h>
#include <pcomn_omanip.h>
#include <pcomn_unittest.h>

class EnumDescriptionTests : public CppUnit::TestFixture {

      void Test_DescribeEnum() ;

      CPPUNIT_TEST_SUITE(EnumDescriptionTests) ;

      CPPUNIT_TEST(Test_DescribeEnum) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

enum Test1 {
   t1,
   t2,
   t3
} ;

enum class Test2 {
   ct1,
   ct2,
   ct3,
   ct4
} ;

namespace Foo {
enum class Test2 {
   fct1,
   fct2,
   fct3,
   fct4
} ;
enum Test1 {
   ft1,
   ft2,
   ft3
} ;
}

PCOMN_DESCRIBE_ENUM(Foo::Test1, ft1, ft2, ft3) ;
PCOMN_DESCRIBE_ENUM(Foo::Test2, fct1, fct2, fct3, fct4) ;
PCOMN_DESCRIBE_ENUM(Test1, t1, t2, t3) ;
PCOMN_DESCRIBE_ENUM(Test2, ct1, ct2, ct3, ct4) ;

/*******************************************************************************
 EnumDescriptionTests
*******************************************************************************/
void EnumDescriptionTests::Test_DescribeEnum()
{
   using namespace pcomn ;
   CPPUNIT_LOG_EQ(enum_name(t1), "t1") ;
   CPPUNIT_LOG_EQ(enum_name(Foo::Test2::fct2), "fct2") ;
   CPPUNIT_LOG_EQ(enum_name(Test2::ct3), "ct3") ;
   CPPUNIT_LOG_EQ(enum_name(Foo::ft3), "ft3") ;
}

int main(int argc, char *argv[])
{
   return pcomn::unit::run_tests
      <
         EnumDescriptionTests
      >
      (argc, argv, "unittest.diag.ini", "Pseudorandom generators tests") ;
}
