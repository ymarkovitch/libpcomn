/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_iterator.cpp
 COPYRIGHT    :   Yakov Markovitch, 2003-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Test of pcomn::mapped_iterator and pcomn::xform_iterator

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   3 Apr 2003
*******************************************************************************/
#include <pcomn_iterator.h>
#include <pcomn_simplematrix.h>
#include <pcomn_unittest.h>

#include <iostream>
#include <algorithm>
#include <iterator>
#include <functional>
#include <list>
#include <vector>
#include <string>

typedef std::list<std::string> strlist_t ;
typedef std::vector<std::string> strvector_t ;
typedef std::vector<int> intvector_t ;
typedef std::list<int> intlist_t ;

using namespace pcomn ;

/*******************************************************************************
                     class IteratorTests
*******************************************************************************/
class IteratorTests : public CppUnit::TestFixture {

      void Test_Mapped_Iterator() ;
      void Test_XForm_Iterator() ;
      void Test_Iterator_Type_Traits() ;

      CPPUNIT_TEST_SUITE(IteratorTests) ;

      CPPUNIT_TEST(Test_Mapped_Iterator) ;
      CPPUNIT_TEST(Test_XForm_Iterator) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;


const char *num_names_[] =
{
   "null",
   "ein",
   "zwei",
   "drei",
   "vier",
   "funf",
   "sechs",
   "sieben",
   "acht",
   "neun",
   "zehn",
   "elf",
   "zwolf"
} ;

const char *num_nums[] =
{
   "0",
   "1",
   "2",
   "3",
   "4",
   "5",
   "6",
   "7",
   "8",
   "9",
   "10",
   "11",
   "12"
} ;

static simple_slice<const char *> num_names { num_names_ } ;

void IteratorTests::Test_Mapped_Iterator()
{
   intvector_t numvec {0, 2, 4, 1, 3, 11} ;
   intlist_t numlist (numvec.begin(), numvec.end()) ;

   CPPUNIT_LOG_EQUAL(strvector_t(const_mapped_iter(num_names, numvec.begin()), const_mapped_iter(num_names, numvec.end())),
                     (strvector_t{"null", "zwei", "vier", "ein", "drei", "elf"})) ;
   CPPUNIT_LOG_EQUAL(strlist_t(const_mapped_iter(num_names, numvec.begin()), const_mapped_iter(num_names, numvec.end())),
                     (strlist_t{"null", "zwei", "vier", "ein", "drei", "elf"})) ;
}

void IteratorTests::Test_XForm_Iterator()
{
   struct atoi_cvt : public std::unary_function<std::string, int> {
         int operator() (const std::string &num) const { return atoi(num.c_str()) ; }
   } ;

   strvector_t numnums (std::begin(num_nums), std::end(num_nums)) ;

   intvector_t numvec {0, 2, 4, 1, 3, 11} ;
   intlist_t numlist (numvec.begin(), numvec.end()) ;

   CPPUNIT_LOG_EQUAL(strvector_t(const_mapped_iter(num_names, xform_iter(const_mapped_iter(numnums, numvec.begin()), atoi_cvt())),
                                 const_mapped_iter(num_names, xform_iter(const_mapped_iter(numnums, numvec.end()), atoi_cvt()))),
                     (strvector_t{"null", "zwei", "vier", "ein", "drei", "elf"})) ;

   CPPUNIT_LOG_EQUAL(strvector_t(const_mapped_iter(num_names, xform_iter(const_mapped_iter(numnums, numlist.begin()), atoi_cvt())),
                                 const_mapped_iter(num_names, xform_iter(const_mapped_iter(numnums, numlist.end()), atoi_cvt()))),
                     (strvector_t{"null", "zwei", "vier", "ein", "drei", "elf"})) ;
}

struct Dummy ;

void IteratorTests::Test_Iterator_Type_Traits()
{
   PCOMN_STATIC_CHECK(!is_iterator<void>::value) ;
   PCOMN_STATIC_CHECK(!is_iterator<int>::value) ;

   PCOMN_STATIC_CHECK((is_iterator<Dummy *, std::random_access_iterator_tag>::value)) ;
   PCOMN_STATIC_CHECK(is_iterator<char *>::value) ;
   PCOMN_STATIC_CHECK(is_iterator<const char *>::value) ;
   PCOMN_STATIC_CHECK(is_iterator<strlist_t::iterator>::value) ;
   PCOMN_STATIC_CHECK(is_iterator<strlist_t::const_iterator>::value) ;
   PCOMN_STATIC_CHECK(is_iterator<intlist_t::iterator>::value) ;
   PCOMN_STATIC_CHECK(is_iterator<intlist_t::const_iterator>::value) ;

   PCOMN_STATIC_CHECK((is_iterator<intlist_t::const_iterator, std::forward_iterator_tag>::value)) ;
   //PCOMN_STATIC_CHECK((!is_iterator<intlist_t::const_iterator, std::random_access_iterator_tag>::value)) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(IteratorTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv, "unittest.diag.ini", "PCommon iterator tempates tests") ;
}
