/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   regtest.cpp
 COPYRIGHT    :   Yakov Markovitch, 1998-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Regular expressions test

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   1 June 1998
*******************************************************************************/
#include <iostream>
#include <pcomn_regex.h>

inline std::ostream &operator<<(std::ostream &os, const reg_match &s)
{
   return PSUBEXP_EMPTY(s) < 0
      ? (os << "<NULL>")
      : (os << '<' << s.rm_so << " - " << s.rm_eo << '>') ;
}

int main(int argc, char *argv[])
{
   if (argc != 2)
   {
      std::cout << "Usage: regtest <regexp>" << std::endl ;
      return 255 ;
   }

   char inbuf[8192] ;

   try
   {
      pcomn::regex exp (argv[1]) ;
      exp.dump() ;

      while (std::cin.getline (inbuf, sizeof inbuf))
      {
         reg_match sub[36] ;
         std::cout << argv[1] << std::endl ;
         std::string input (inbuf) ;
         for (reg_match *first = sub, *last = exp.match(inbuf, sub) ; first != last ; ++first)
            std::cout << pcomn::str::substr(input, *first) << std::endl ;
      }

   }
   catch (const pcomn::regex_error &err)
   {
      std::cout << err.what() << " in expression \"" << err.expression() <<
         "\" at position " << err.position() << std::endl ;
   }

   return 0 ;
}
