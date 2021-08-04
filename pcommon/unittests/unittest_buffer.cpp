/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_vector.cpp
 COPYRIGHT    :   Yakov Markovitch, 2009-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unit tests of simple_slice, simple_vector, static_vector et al.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   18 Mar 2009
*******************************************************************************/
#include <pcomn_buffer.h>
#include <pcomn_unittest.h>
#include <pcomn_strslice.h>

/*******************************************************************************
                     class BufferTraitsTest
*******************************************************************************/
class BufferTraitsTest : public CppUnit::TestFixture {

      void Test_Buffer_Traits() ;

      CPPUNIT_TEST_SUITE(BufferTraitsTest) ;

      CPPUNIT_TEST(Test_Buffer_Traits) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

/*******************************************************************************
 BufferTraitsTest
*******************************************************************************/
void BufferTraitsTest::Test_Buffer_Traits()
{
   using namespace pcomn ;

   std::vector<uint32_t> d (2047) ;
   const std::vector<uint32_t> &cd = d ;

   CPPUNIT_LOG_EQUAL(buf::size(d), d.size()*sizeof(uint32_t)) ;
   CPPUNIT_LOG_EQUAL(buf::size(cd), d.size()*sizeof(uint32_t)) ;

   iovec_t v1 ;
   CPPUNIT_LOG_RUN(v1 = make_iovec(d.data(), buf::size(d))) ;

   CPPUNIT_LOG_EQ(buf::data(v1), d.data()) ;
   CPPUNIT_LOG_EQ(buf::size(v1), buf::size(d)) ;
}

int main(int argc, char *argv[])
{
   return pcomn::unit::run_tests
      <
         BufferTraitsTest
      >
      (argc, argv) ;
}
