/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   test_bitarray.cpp
 COPYRIGHT    :   Yakov Markovitch, 2001-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Test of pcomn::bitarray container

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   11 Mar 2001
*******************************************************************************/
#include <pcomn_bitarray.h>
#include <iostream>
#include <algorithm>
#include <iterator>

static void test(size_t sz)
{
   std::cout << std::endl << "Testing "  << sz << std::endl ;

   pcomn::bitarray b(sz) ;
   b[51] = b[44] = b[43] = b[40] = true ;
   pcomn::bitarray b1(sz) ;
   pcomn::bitarray b2(b) ;
   pcomn::bitarray b3(43, true) ;
   b2[40] = false ;
   b1.set() ;
   b1[51] = b1[44] = b1[43] = b1[40] = false ;

   // The following 3 lines have no particular sense and must be no-op if
   // iterator assignment implementation is correct.
   pcomn::bitarray::iterator i1 (b1.begin()) ;
   pcomn::bitarray::iterator i2 (b.begin()) ;
   i2 = i1 ;

   std::cout << b << std::endl
             << b2 << std::endl
             << (b & b3) << std::endl
             << (b3 & b) << std::endl
             << ~b << std::endl
             << (b << 10) << std::endl
             << (b >> 10) << std::endl
             << b.count() << ", " << (~b).count() << std::endl
             << "b" << (b == b1 ? "==" : "!=") << "b1" << std::endl
             << "b" << (b == ~b1 ? "==" : "!=") << "~b1" << std::endl
             << b1 << std::endl
             << (b << 8) << std::endl
             << pcomn::mask(b1, b << 8) << std::endl ;

   std::copy(b.begin(), b.end(), std::ostream_iterator<int>(std::cout << std::endl)) ;
   std::cout << std::endl ;
}

static void test_positional_iterator()
{
   std::cout << std::endl << std::endl << "Testing positional iterator." << std::endl << std::endl ;

   pcomn::bitarray empty ;
   pcomn::bitarray i1(1, false) ;
   pcomn::bitarray i33(33) ;

   std::cout << "Positions in an empty bitarray of size 0:" << std::endl ;
   std::copy(empty.begin_positional(), empty.end_positional(),
             std::ostream_iterator<int>(std::cout, " ")) ;

   std::cout << std::endl << "Positions in an empty bitarray of size 1:" << std::endl ;
   std::copy(i1.begin_positional(), i1.end_positional(),
             std::ostream_iterator<int>(std::cout, " ")) ;

   std::cout << std::endl << "Positions in a filled bitarray of size 1:" << std::endl ;
   i1.flip() ;
   std::copy(i1.begin_positional(), i1.end_positional(),
             std::ostream_iterator<int>(std::cout, " ")) ;

   std::cout << std::endl << "Positions in an empty bitarray of size 33:" << std::endl ;
   std::copy(i33.begin_positional(), i33.end_positional(),
             std::ostream_iterator<int>(std::cout, " ")) ;
   std::cout << std::endl ;
}

static void test_positional_iterator(size_t sz)
{
   pcomn::bitarray b(sz) ;
   b[0] = b[1] = b[31] = b[40] = b[43] = b[44] = b[51] = b[96] = b[126] = true ;
   std::copy(b.begin_positional(), b.end_positional(),
             std::ostream_iterator<int>(std::cout << std::endl, " ")) ;
   b.flip() ;
   std::copy(b.begin_positional(), b.end_positional(),
             std::ostream_iterator<int>(std::cout << std::endl, " ")) ;
   std::cout << std::endl ;
}

int main()
{
   test(80) ;
   test(79) ;
   test(81) ;
   test(88) ;

   test_positional_iterator() ;

   test_positional_iterator(127) ;
   test_positional_iterator(128) ;
   test_positional_iterator(129) ;
   test_positional_iterator(130) ;
}
