/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_simplematrix.cpp
 COPYRIGHT    :   Yakov Markovitch, 2009-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unit tests of simple_slice, simple_vector, simple_matrix et al.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   18 Mar 2009
*******************************************************************************/
#include <pcomn_simplematrix.h>
#include <pcomn_unittest.h>

/*******************************************************************************
                     class SimpleSliceTests
*******************************************************************************/
class SimpleSliceTests : public CppUnit::TestFixture {

      void Test_Simple_Slice_Construct() ;

      CPPUNIT_TEST_SUITE(SimpleSliceTests) ;

      CPPUNIT_TEST(Test_Simple_Slice_Construct) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

/*******************************************************************************
 DirectSmartPtrTests
*******************************************************************************/
void SimpleSliceTests::Test_Simple_Slice_Construct()
{
   using namespace pcomn ;

   typedef simple_slice<int> int_slice ;
   typedef simple_slice<const int> cint_slice ;
   typedef simple_slice<std::string> str_slice ;

   int_slice EmptyIntSlice ;
   cint_slice EmptyCIntSlice ;
   str_slice EmptyStrSlice ;

   CPPUNIT_LOG_ASSERT(EmptyIntSlice.empty()) ;
   CPPUNIT_LOG_ASSERT(EmptyCIntSlice.empty()) ;
   CPPUNIT_LOG_ASSERT(EmptyStrSlice.empty()) ;

   CPPUNIT_LOG_EQUAL(EmptyIntSlice.size(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(EmptyIntSlice.begin(), (int *)NULL) ;
   CPPUNIT_LOG_EQUAL(EmptyIntSlice.end(), (int *)NULL) ;

   CPPUNIT_LOG(std::endl) ;

   int IntArray[] = { 3, 1, 2 } ;
   int_slice IntArraySlice(IntArray) ;

   CPPUNIT_LOG_IS_FALSE(IntArraySlice.empty()) ;
   CPPUNIT_LOG_EQUAL(IntArraySlice.size(), (size_t)3) ;
   CPPUNIT_LOG_EQUAL(IntArraySlice.front(), 3) ;
   CPPUNIT_LOG_EQUAL(IntArraySlice.back(), 2) ;
   CPPUNIT_LOG_EQUAL(IntArraySlice[0], 3) ;
   CPPUNIT_LOG_EQUAL(IntArraySlice[1], 1) ;
   CPPUNIT_LOG_EQUAL(IntArraySlice[2], 2) ;

   CPPUNIT_LOG(std::endl) ;
   cint_slice CIntArraySlice(IntArray) ;
   CPPUNIT_LOG_EQUAL(CIntArraySlice.size(), (size_t)3) ;
   CPPUNIT_LOG_EQUAL(CIntArraySlice.front(), 3) ;
   CPPUNIT_LOG_EQUAL(CIntArraySlice.back(), 2) ;
   CPPUNIT_LOG_RUN(IntArraySlice[0] = 5) ;
   CPPUNIT_LOG_RUN(IntArraySlice[1] = 6) ;
   CPPUNIT_LOG_EQUAL(CIntArraySlice.front(), 5) ;
   CPPUNIT_LOG_RUN(IntArraySlice.back() = 13) ;
   CPPUNIT_LOG_EQUAL(CIntArraySlice[2], 13) ;
   CPPUNIT_LOG_EQUAL(IntArray[2], 13) ;

   CPPUNIT_LOG(std::endl) ;
   const int CIntArray[] = { 56, 67, 78, 89 } ;
   cint_slice CIntArraySlice1(CIntArray) ;
   CPPUNIT_LOG_EQUAL(CIntArraySlice1.size(), (size_t)4) ;
   CPPUNIT_LOG_EQUAL(CIntArraySlice1.front(), 56) ;
   CPPUNIT_LOG_EQUAL(CIntArraySlice1.back(), 89) ;

   CPPUNIT_LOG_RUN(CIntArraySlice1 = IntArraySlice) ;
   CPPUNIT_LOG_EQUAL(CIntArraySlice1.front(), 5) ;
   CPPUNIT_LOG_EQUAL(CIntArraySlice1.back(), 13) ;

   CPPUNIT_LOG(std::endl) ;
   int NewIntArray[] = { 77, 66 } ;
   const std::vector<int> CIntVector (CIntArray + 0, CIntArray + 4) ;
   std::vector<int> IntVector (NewIntArray + 0, NewIntArray + 2) ;
   cint_slice CIntVectorSlice1 (CIntVector) ;
   CPPUNIT_LOG_EQUAL(CIntVectorSlice1.size(), (size_t)4) ;
   cint_slice CIntVectorSlice2 (IntVector) ;
   CPPUNIT_LOG_EQUAL(CIntVectorSlice2.size(), (size_t)2) ;

   int_slice IntVectorSlice1 (IntVector) ;
   CPPUNIT_LOG_EQUAL(IntVectorSlice1.size(), (size_t)2) ;

   CPPUNIT_LOG_RUN(cint_slice().swap(CIntVectorSlice1)) ;
   CPPUNIT_LOG_EQUAL(CIntVectorSlice1.size(), (size_t)0) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(SimpleSliceTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "unittest.diag.ini", "Test slices, simple vectors, and matrices.") ;
}
