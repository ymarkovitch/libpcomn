/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   test_mappediter.cpp
 COPYRIGHT    :   Yakov Markovitch, 2003-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Test of pcomn::mapped_iterator and pcomn::xform_iterator

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   3 Apr 2003
*******************************************************************************/
#include <pcomn_iterator.h>
#include <pcomn_simplematrix.h>

#include <iostream>
#include <algorithm>
#include <iterator>
#include <functional>
#include <list>
#include <vector>
#include <string>

typedef std::list<std::string> stringlist_t ;
typedef std::vector<std::string> stringvector_t ;
typedef std::vector<int> intvector_t ;
typedef std::list<int> intlist_t ;
typedef std::ostream_iterator<const char *> ostream_cstr_iterator ;

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

static pcomn::simple_slice<const char *> num_names(num_names_) ;

static void test_mapped_iterator()
{
   std::cout << std::endl << "Testing pcomn::mapped_iterator" << std::endl << std::endl ;

   intvector_t numvec ;
   intlist_t numlist ;

   numvec.push_back(0) ;
   numvec.push_back(2) ;
   numvec.push_back(4) ;
   numvec.push_back(1) ;
   numvec.push_back(3) ;
   numvec.push_back(11) ;

   std::copy(numvec.begin(), numvec.end(), std::back_inserter(numlist)) ;
   std::copy(numlist.begin(), numlist.end(), std::ostream_iterator<int>(std::cout, " ")) ;
   std::cout << std::endl ;
   std::copy(pcomn::const_mapped_iter(num_names, numvec.begin()),
             pcomn::const_mapped_iter(num_names, numvec.end()),
             ostream_cstr_iterator(std::cout, " ")) ;
   std::cout << std::endl ;
   std::copy(pcomn::const_mapped_iter(num_names, numlist.begin()),
             pcomn::const_mapped_iter(num_names, numlist.end()),
             ostream_cstr_iterator(std::cout, " ")) ;
   std::cout << std::endl ;
}

struct atoi_cvt : public std::unary_function<std::string, int> {
      int operator() (const std::string &num) const
      {
         return atoi(num.c_str()) ;
      }
} ;

static void test_xform_iterator()
{
   std::cout << std::endl << "Testing both pcomn::xform_iterator and pcomn::mapped_iterator" << std::endl << std::endl ;

   stringvector_t numnums(num_nums + 0, num_nums + P_ARRAY_COUNT(num_nums)) ;
   intvector_t numvec ;
   intlist_t numlist ;

   numvec.push_back(0) ;
   numvec.push_back(2) ;
   numvec.push_back(4) ;
   numvec.push_back(1) ;
   numvec.push_back(3) ;
   numvec.push_back(11) ;

   std::copy(numvec.begin(), numvec.end(), std::back_inserter(numlist)) ;
   std::copy(numlist.begin(), numlist.end(), std::ostream_iterator<int>(std::cout, " ")) ;
   std::cout << std::endl ;
   std::copy(pcomn::const_mapped_iter(num_names,
                                      pcomn::xform_iter(pcomn::const_mapped_iter(numnums, numvec.begin()),
                                                      atoi_cvt())),
             pcomn::const_mapped_iter(num_names,
                                      pcomn::xform_iter(pcomn::const_mapped_iter(numnums, numvec.end()),
                                                      atoi_cvt())),
             ostream_cstr_iterator(std::cout, " ")) ;
   std::cout << std::endl ;
   std::copy(pcomn::const_mapped_iter(num_names,
                                      pcomn::xform_iter(pcomn::const_mapped_iter(numnums, numlist.begin()),
                                                      atoi_cvt())),
             pcomn::const_mapped_iter(num_names,
                                      pcomn::xform_iter(pcomn::const_mapped_iter(numnums, numlist.end()),
                                                      atoi_cvt())),
             ostream_cstr_iterator(std::cout, " ")) ;
   std::cout << std::endl ;
}


int main()
{
   test_mapped_iterator() ;
   test_xform_iterator() ;
   return 0 ;
}
