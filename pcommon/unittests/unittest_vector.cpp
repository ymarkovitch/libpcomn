/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_vector.cpp
 COPYRIGHT    :   Yakov Markovitch, 2009-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unit tests of simple_slice, simple_vector, static_vector et al.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   18 Mar 2009
*******************************************************************************/
#include <pcomn_vector.h>
#include <pcomn_unittest.h>

/*******************************************************************************
                     class SimpleVectorTests
*******************************************************************************/
class SimpleVectorTests : public CppUnit::TestFixture {

      void Test_Simple_Slice_Construct() ;
      void Test_Simple_Vector_Construct() ;
      void Test_Trivial_Set() ;

      CPPUNIT_TEST_SUITE(SimpleVectorTests) ;

      CPPUNIT_TEST(Test_Simple_Slice_Construct) ;
      CPPUNIT_TEST(Test_Simple_Vector_Construct) ;
      CPPUNIT_TEST(Test_Trivial_Set) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

typedef pcomn::simple_slice<int> int_slice ;
typedef pcomn::simple_cslice<int> cint_slice ;
typedef pcomn::simple_slice<std::string> str_slice ;

typedef pcomn::simple_vector<int> int_vec ;
typedef pcomn::simple_vector<const int> cint_vec ;
typedef pcomn::simple_vector<std::string> str_vec ;

typedef std::vector<int> int_vector ;

/*******************************************************************************
 SimpleVectorTests
*******************************************************************************/
void SimpleVectorTests::Test_Simple_Slice_Construct()
{
   using namespace pcomn ;

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

   CPPUNIT_LOG(std::endl) ;

   // Check multilevel pointers
   std::vector<int*> ppIvector (3) ;
   const simple_cslice<const int *> cpIslice {ppIvector} ;

   CPPUNIT_LOG_EQ(cpIslice.size(), 3) ;
   CPPUNIT_LOG_EQ(cpIslice.begin(), ppIvector.data()) ;

   std::vector<const int*> cppIvector (7) ;
   simple_cslice<const int*> cpIslice1 {cppIvector} ;

   CPPUNIT_LOG_EQ(cpIslice1.size(), 7) ;
   CPPUNIT_LOG_EQ(cpIslice1.begin(), cppIvector.data()) ;

   simple_slice<int*> pIslice {ppIvector} ;
   simple_cslice<const int*> cpIslice2 = pIslice ;

   CPPUNIT_LOG_EQUAL(cpIslice2.size(), ppIvector.size()) ;
   CPPUNIT_LOG_EQ(cpIslice2.begin(), ppIvector.data()) ;

   cpIslice1 = pIslice ;
}

void SimpleVectorTests::Test_Simple_Vector_Construct()
{
   using namespace pcomn ;

   int_vec  EmptyIntVec ;
   cint_vec EmptyCIntVec ;
   str_vec  EmptyStrVec ;

   CPPUNIT_LOG_ASSERT(EmptyIntVec.empty()) ;
   CPPUNIT_LOG_ASSERT(EmptyCIntVec.empty()) ;
   CPPUNIT_LOG_ASSERT(EmptyStrVec.empty()) ;

   CPPUNIT_LOG_EQUAL(EmptyIntVec.size(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(EmptyIntVec.begin(), (int *)NULL) ;
   CPPUNIT_LOG_EQUAL(EmptyIntVec.end(), (int *)NULL) ;

   CPPUNIT_LOG(std::endl) ;

   int IntArray[] = { 3, 1, 2 } ;
   int_vec IntArrayVec(IntArray) ;

   CPPUNIT_LOG_IS_FALSE(IntArrayVec.empty()) ;
   CPPUNIT_LOG_EQUAL(IntArrayVec.size(), (size_t)3) ;
   CPPUNIT_LOG_EQUAL(IntArrayVec.front(), 3) ;
   CPPUNIT_LOG_EQUAL(IntArrayVec.back(), 2) ;
   CPPUNIT_LOG_EQUAL(IntArrayVec[0], 3) ;
   CPPUNIT_LOG_EQUAL(IntArrayVec[1], 1) ;
   CPPUNIT_LOG_EQUAL(IntArrayVec[2], 2) ;

   CPPUNIT_LOG(std::endl) ;

   cint_vec CIntArrayVec(IntArray) ;
   CPPUNIT_LOG_EQUAL(CIntArrayVec.size(), (size_t)3) ;
   CPPUNIT_LOG_EQUAL(CIntArrayVec.front(), 3) ;
   CPPUNIT_LOG_EQUAL(CIntArrayVec.back(), 2) ;
   CPPUNIT_LOG_RUN(IntArrayVec[0] = 5) ;
   CPPUNIT_LOG_RUN(IntArrayVec[1] = 6) ;
   CPPUNIT_LOG_EQUAL(CIntArrayVec.front(), 3) ;
   CPPUNIT_LOG_EQUAL(CIntArrayVec.back(), 2) ;

   CPPUNIT_LOG(std::endl) ;
   const int CIntArray[] = { 56, 67, 78, 89 } ;
   cint_vec CIntArrayVec1(CIntArray) ;

   CPPUNIT_LOG_EQUAL(CIntArrayVec1.size(), (size_t)4) ;
   CPPUNIT_LOG_EQUAL(CIntArrayVec1.front(), 56) ;
   CPPUNIT_LOG_EQUAL(CIntArrayVec1.back(), 89) ;

   CPPUNIT_LOG_RUN(CIntArrayVec1 = IntArrayVec) ;
   CPPUNIT_LOG_EQUAL(CIntArrayVec1.front(), 5) ;
   CPPUNIT_LOG_EQUAL(CIntArrayVec1.back(), 2) ;

   CPPUNIT_LOG(std::endl) ;

   int_slice IntSlice1 (IntArrayVec)  ;
   cint_slice CIntSlice1 (IntArrayVec) ;
   cint_slice CIntSlice2 (CIntArrayVec) ;

   auto f1 = [](const simple_cslice<int> &){} ;
   auto f2 = [](simple_slice<int>) {} ;
   auto f3 = [](simple_cslice<int>) {} ;
   f1(IntArrayVec) ;
   f2(IntArrayVec) ;
   f3(IntArrayVec) ;

   CPPUNIT_LOG_ASSERT(IntSlice1 == IntSlice1) ;
   CPPUNIT_LOG_ASSERT(IntSlice1 == CIntSlice1) ;
   CPPUNIT_LOG_ASSERT(CIntSlice1 == IntSlice1) ;
   CPPUNIT_LOG_ASSERT(IntSlice1 == IntArrayVec) ;
   CPPUNIT_LOG_ASSERT(IntArrayVec == IntSlice1) ;
}

void SimpleVectorTests::Test_Trivial_Set()
{
   using namespace pcomn ;

   typedef trivial_set<int> test_set ;

   test_set empty ;
   CPPUNIT_LOG_ASSERT(empty.empty()) ;
   CPPUNIT_LOG_EQ(empty.size(), 0) ;
   CPPUNIT_LOG_ASSERT(empty.begin() == empty.end()) ;

   test_set single (2) ;
   CPPUNIT_LOG_IS_FALSE(single.empty()) ;
   CPPUNIT_LOG_EQ(single.size(), 1) ;
   CPPUNIT_LOG_ASSERT(single.begin() != single.end()) ;
   CPPUNIT_LOG_EQ(std::distance(single.begin(), single.end()), 1) ;
   CPPUNIT_LOG_EQ(single.front(), 2) ;

   const int vlanset[] = { 3, 6, 1, 2, 1, 13, 11, 12, 10, 15 } ;
   test_set many (std::begin(vlanset), std::end(vlanset)) ;
   CPPUNIT_LOG_IS_FALSE(many.empty()) ;
   CPPUNIT_LOG_EQ(many.size(), 9) ;
   CPPUNIT_LOG_ASSERT(many.begin() != many.end()) ;
   CPPUNIT_LOG_EQ(std::distance(many.begin(), many.end()), 9) ;

   // Test insertion
   auto ins1 = empty.insert(5) ;
   CPPUNIT_LOG_IS_FALSE(empty.empty()) ;
   CPPUNIT_LOG_EQ(empty.size(), 1) ;
   CPPUNIT_LOG_IS_FALSE(empty.begin() == empty.end()) ;
   CPPUNIT_LOG_EQ(*empty.begin(), 5) ;
   CPPUNIT_LOG_EQ(std::distance(empty.begin(), empty.end()), 1) ;

   CPPUNIT_LOG_EQ(ins1, std::make_pair(empty.begin(), true)) ;
   CPPUNIT_LOG_EQ(empty.front(), 5) ;

   CPPUNIT_LOG(std::endl) ;
   auto ins0 = empty.insert(5) ;
   CPPUNIT_LOG_EQ(empty.size(), 1) ;
   CPPUNIT_LOG_EQ(empty.front(), 5) ;
   CPPUNIT_LOG_EQ(ins0, std::make_pair(empty.begin(), false)) ;

   auto ins2 = single.insert(-19) ;
   CPPUNIT_LOG_EQ(single.size(), 2) ;

   CPPUNIT_LOG_EQ(std::distance(single.begin(), single.end()), 2) ;
   CPPUNIT_LOG_EQ(single.front(), -19) ;
   CPPUNIT_LOG_EQ(*single.begin(), -19) ;
   CPPUNIT_LOG_EQ(*std::next(single.begin()), 2) ;
   CPPUNIT_LOG_EQ(ins2, std::make_pair(single.begin(), true)) ;

   auto ins3 = single.insert(7) ;
   CPPUNIT_LOG_EQ(single.size(), 3) ;

   CPPUNIT_LOG_EQ(single.front(), -19) ;
   CPPUNIT_LOG_EQ(*single.begin(), -19) ;
   CPPUNIT_LOG_EQ(*std::next(single.begin()), 2) ;
   CPPUNIT_LOG_EQ(*std::next(std::next(single.begin())), 7) ;
   CPPUNIT_LOG_EQ(ins3, std::make_pair(std::prev(single.end()), true)) ;
}

int main(int argc, char *argv[])
{
   return pcomn::unit::run_tests
      <
         SimpleVectorTests
      >
      (argc, argv) ;
}
