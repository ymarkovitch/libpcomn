/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_simplematrix.cpp
 COPYRIGHT    :   Yakov Markovitch, 2009-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unit tests of simple_matrix

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   18 Mar 2009
*******************************************************************************/
#include <iostream>
#include <pcomn_simplematrix.h>
#include <pcomn_unittest.h>

/*******************************************************************************
 SimpleMatrixTests
*******************************************************************************/
class SimpleMatrixTests : public CppUnit::TestFixture {

      void Test_Simple_Matrix_Construct() ;

      CPPUNIT_TEST_SUITE(SimpleMatrixTests) ;

      CPPUNIT_TEST(Test_Simple_Matrix_Construct) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

typedef std::vector<int> int_vector ;

/*******************************************************************************
 SimpleMatrixTests
*******************************************************************************/
void SimpleMatrixTests::Test_Simple_Matrix_Construct()
{
   using namespace pcomn ;

   typedef matrix_slice<std::string>   string_mslice ;
   typedef simple_matrix<std::string>  string_matrix ;
   typedef matrix_slice<const std::string> string_mcslice ;
   typedef simple_matrix<int> int_matrix ;

   string_mslice mslice0 ;
   string_mcslice mc0 (mslice0) ;

   std::string data1[] = {"1", "2", "3", "4", "5", "6"} ;

   string_mslice mslice1_3x2 (data1, 3, 2) ;
   string_mslice mslice1_2x3 (data1, 2, 3) ;

   string_mslice::iterator i3x2 = mslice1_3x2.begin() ;
   string_mslice::const_iterator ci3x2 (i3x2) ;

   CPPUNIT_LOG_EQ(*i3x2,        (string_vector{"1", "2"})) ;
   CPPUNIT_LOG_EQ(*(ci3x2 + 1), (string_vector{"3", "4"})) ;
   CPPUNIT_LOG_EQ(*(ci3x2 + 2), (string_vector{"5", "6"})) ;

   CPPUNIT_LOG_EQ(mslice1_3x2[0], (string_vector{"1", "2"})) ;
   CPPUNIT_LOG_EQ(mslice1_3x2[1], (string_vector{"3", "4"})) ;
   CPPUNIT_LOG_EQ(mslice1_3x2[2], (string_vector{"5", "6"})) ;

   CPPUNIT_LOG_EQ(mslice1_2x3[0], (string_vector{"1", "2", "3"})) ;
   CPPUNIT_LOG_EQ(mslice1_2x3[1], (string_vector{"4", "5", "6"})) ;

   CPPUNIT_LOG(std::endl) ;
   string_matrix matrix0_3x2 (mslice0) ;

   string_matrix matrix1_3x2 (mslice1_3x2) ;
   string_matrix matrix2_3x2 (1, 4, "Hello!") ;

   CPPUNIT_LOG_EQ(matrix0_3x2.dim(), unipair<size_t>(0, 0)) ;
   CPPUNIT_LOG_ASSERT(matrix0_3x2.empty()) ;

   CPPUNIT_LOG_EQ(matrix1_3x2.dim(), unipair<size_t>(3, 2)) ;

   CPPUNIT_LOG_EQ(matrix1_3x2[0], (string_vector{"1", "2"})) ;
   CPPUNIT_LOG_EQ(matrix1_3x2[1], (string_vector{"3", "4"})) ;
   CPPUNIT_LOG_EQ(matrix1_3x2[2], (string_vector{"5", "6"})) ;

   CPPUNIT_LOG_ASSERT(matrix0_3x2.empty()) ;
   CPPUNIT_LOG_RUN(matrix0_3x2 = matrix1_3x2) ;
   CPPUNIT_LOG_EQ(matrix1_3x2.dim(), unipair<size_t>(3, 2)) ;
   CPPUNIT_LOG_EQ(matrix0_3x2.dim(), unipair<size_t>(3, 2)) ;

   CPPUNIT_LOG_EQ(matrix1_3x2[0], (string_vector{"1", "2"})) ;
   CPPUNIT_LOG_EQ(matrix1_3x2[2], (string_vector{"5", "6"})) ;

   CPPUNIT_LOG_EQ(matrix0_3x2[0], (string_vector{"1", "2"})) ;
   CPPUNIT_LOG_EQ(matrix0_3x2[2], (string_vector{"5", "6"})) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQ(matrix2_3x2.dim(), unipair<size_t>(1, 4)) ;
   CPPUNIT_LOG_EQ(matrix2_3x2[0], (string_vector(4, "Hello!"))) ;

   CPPUNIT_LOG_EQ((matrix2_3x2 = std::move(matrix1_3x2)).dim(), unipair<size_t>(3, 2)) ;
   CPPUNIT_LOG_EQ(matrix1_3x2.dim(), unipair<size_t>(0, 0)) ;
   CPPUNIT_LOG_ASSERT(matrix1_3x2.empty()) ;

   CPPUNIT_LOG_EQ(matrix2_3x2[0], (string_vector{"1", "2"})) ;
   CPPUNIT_LOG_EQ(matrix2_3x2[2], (string_vector{"5", "6"})) ;
   CPPUNIT_LOG_EQ(matrix2_3x2.dim(), unipair<size_t>(3, 2)) ;

   CPPUNIT_LOG(std::endl) ;
   int_matrix matrix3_4x3 (3, {
         {2, 4, 6},
         {1, 3, 5},
         {20, 40, 60},
         {10, 30, 50}}) ;

   int_matrix matrix4_0x0 (0, {}) ;

   CPPUNIT_LOG_EXCEPTION_MSG(int_matrix(3, {{2, 4, 6}, {1, 3}, {20, 40, 60}}),
                             std::invalid_argument, "mismatch") ;

   CPPUNIT_LOG_EQ(matrix4_0x0.dim(), unipair<size_t>(0, 0)) ;
   CPPUNIT_LOG_EQ(matrix3_4x3.dim(), unipair<size_t>(4, 3)) ;

   CPPUNIT_LOG_EQ(matrix3_4x3[0], (int_vector{2, 4, 6})) ;
   CPPUNIT_LOG_EQ(matrix3_4x3[1], (int_vector{1, 3, 5})) ;
   CPPUNIT_LOG_EQ(matrix3_4x3[2], (int_vector{20, 40, 60})) ;
   CPPUNIT_LOG_EQ(matrix3_4x3[3], (int_vector{10, 30, 50})) ;

   CPPUNIT_LOG_EXPRESSION(matrix4_0x0) ;
   CPPUNIT_LOG_EXPRESSION(matrix3_4x3) ;
   CPPUNIT_LOG_EQUAL(matrix3_4x3, matrix3_4x3) ;

   CPPUNIT_LOG_LINE("\n************* Test resizable matrix")  ;
   typedef simple_matrix<std::string, true> string_rmatrix ;

   string_rmatrix rmatrix0_3x2 (mslice0) ;

   string_rmatrix rmatrix1_3x2 (mslice1_3x2) ;
   string_rmatrix rmatrix2_3x2 (1, 4, "Hello!") ;

   CPPUNIT_LOG_EQ(rmatrix0_3x2.dim(), unipair<size_t>(0, 0)) ;
   CPPUNIT_LOG_ASSERT(rmatrix0_3x2.empty()) ;

   CPPUNIT_LOG_EQ(rmatrix1_3x2.dim(), unipair<size_t>(3, 2)) ;

   CPPUNIT_LOG_EQ(rmatrix1_3x2[0], (string_vector{"1", "2"})) ;
   CPPUNIT_LOG_EQ(rmatrix1_3x2[1], (string_vector{"3", "4"})) ;
   CPPUNIT_LOG_EQ(rmatrix1_3x2[2], (string_vector{"5", "6"})) ;

   CPPUNIT_LOG_ASSERT(rmatrix0_3x2.empty()) ;
   CPPUNIT_LOG_RUN(rmatrix0_3x2 = rmatrix1_3x2) ;
   CPPUNIT_LOG_EQ(rmatrix1_3x2.dim(), unipair<size_t>(3, 2)) ;
   CPPUNIT_LOG_EQ(rmatrix0_3x2.dim(), unipair<size_t>(3, 2)) ;

   CPPUNIT_LOG_EQ(rmatrix1_3x2[0], (string_vector{"1", "2"})) ;
   CPPUNIT_LOG_EQ(rmatrix1_3x2[2], (string_vector{"5", "6"})) ;

   CPPUNIT_LOG_EQ(rmatrix0_3x2[0], (string_vector{"1", "2"})) ;
   CPPUNIT_LOG_EQ(rmatrix0_3x2[2], (string_vector{"5", "6"})) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQ(rmatrix2_3x2.dim(), unipair<size_t>(1, 4)) ;
   CPPUNIT_LOG_EQ(rmatrix2_3x2[0], (string_vector(4, "Hello!"))) ;

   CPPUNIT_LOG_EQ((rmatrix2_3x2 = std::move(rmatrix1_3x2)).dim(), unipair<size_t>(3, 2)) ;
   CPPUNIT_LOG_EQ(rmatrix1_3x2.dim(), unipair<size_t>(0, 0)) ;
   CPPUNIT_LOG_ASSERT(rmatrix1_3x2.empty()) ;

   CPPUNIT_LOG_EQ(rmatrix2_3x2[0], (string_vector{"1", "2"})) ;
   CPPUNIT_LOG_EQ(rmatrix2_3x2[2], (string_vector{"5", "6"})) ;
   CPPUNIT_LOG_EQ(rmatrix2_3x2.dim(), unipair<size_t>(3, 2)) ;

   CPPUNIT_LOG_RUN(rmatrix2_3x2.resize(5)) ;
   CPPUNIT_LOG_EQ(rmatrix2_3x2.dim(), unipair<size_t>(5, 2)) ;
   CPPUNIT_LOG_EQ(rmatrix2_3x2[0], (string_vector{"1", "2"})) ;
   CPPUNIT_LOG_EQ(rmatrix2_3x2[2], (string_vector{"5", "6"})) ;
   CPPUNIT_LOG_EQ(rmatrix2_3x2[3], (string_vector{"", ""})) ;
   CPPUNIT_LOG_EQ(rmatrix2_3x2[4], (string_vector{"", ""})) ;

   CPPUNIT_LOG_RUN(rmatrix2_3x2.resize(2)) ;
   CPPUNIT_LOG_EQ(rmatrix2_3x2.dim(), unipair<size_t>(2, 2)) ;
   CPPUNIT_LOG_EQ(rmatrix2_3x2[0], (string_vector{"1", "2"})) ;
   CPPUNIT_LOG_EQ(rmatrix2_3x2[1], (string_vector{"3", "4"})) ;
}

int main(int argc, char *argv[])
{
   return pcomn::unit::run_tests
      <
         SimpleMatrixTests
      >
      (argc, argv) ;
}
