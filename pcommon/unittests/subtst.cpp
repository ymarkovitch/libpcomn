/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   subtst.cpp
 COPYRIGHT    :   Yakov Markovitch, 1999-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Substring tests

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   31 Jul 1999
*******************************************************************************/
#include <pcomn_substring.h>

#include <utility>
#include <iostream>

void main()
{
   pcomn::substring a1, a2 ;
   std::string s1, s2 ;
   a1 < a2 ;
   a1 == a2 ;
   a1 != a2 ;
   a1 < "Hello!" ;
   a1 > "Hello!" ;
   "Hello!" > a1 ;
   "Hello!" < a1 ;
   "Hello!" == a1 ;
   "Hello!" != a1 ;
   a1 < s1 ;
   s1 < a1 ;
   s1 != pcomn::substring(s2, 2) ;
}
