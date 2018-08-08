/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)) -*-*/
/*******************************************************************************
 FILE         :   test_readfile.cpp
 COPYRIGHT    :   Yakov Markovitch, 2018. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   16 May 2017
*******************************************************************************/
#include <pcomn_fileutils.h>
#include <iostream>

using namespace pcomn ;

int main(int, char *[])
{
   try {
      std::cout << readfile(fileno(stdin)) << std::flush ;
   }
   catch (const std::exception &x)
   {
      std::cerr << STDEXCEPTOUT(x) << std::endl ;
      return 1 ;
   }
   return 0 ;
}
