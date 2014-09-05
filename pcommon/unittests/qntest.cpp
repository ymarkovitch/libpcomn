/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   qntest.cpp
 COPYRIGHT    :   Yakov Markovitch, 1999-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Qualified name test

 PROGRAMMED BY:   Yakov E. Markovitch
 CREATION DATE:   31 Jul 1999
*******************************************************************************/
#include <jmisc.h>
#include <iostream>

stds::ostream &print_qname (stds::ostream &os, const qualified_name &qn)
{
   os << "Name is " ;
   if (!qn)
      os << "illegal" << stds::endl ;
   else
   {
      os << "legal" ;

      if (qn.rooted())
         os << ", rooted" ;

      if (qn.qualified())
         os << ", qualified" ;

      os << stds::endl <<
         "Qualifier: " << qn.qual() << " (" << qn.qual(true) << ')' << stds::endl <<
         "Name: " << qn.name() << stds::endl <<
         "Fullname: " << qn << " (" << qn.fullname(true) << ')' << stds::endl ;
   }
   return os ;
}

void main (int argc, char *argv[])
{
   if (argc < 2)
      return ;

   qualified_name qn (argv[1], 0, qualified_name::TrailingDelim | qualified_name::FullString) ;

   stds::cout << argv[1] << stds::endl ;

   print_qname(stds::cout, qn) ;

   if (!!qn)
   {
      if (argc > 3)
      {
         stds::cout << "demangle(" << argv[2] << ", " << argv[3] << ')' << stds::endl ;
         stds::cout << qn.demangle(atoi(argv[2]), atoi(argv[3])) << stds::endl ;
      }
      print_qname (stds::cout, qn.append ("hello::world")) ;
      if (!!qn)
      {
         print_qname (stds::cout, qualified_name(qn.fullname(true), 0, qualified_name::AlreadyMangled)) ;
         print_qname (stds::cout, qualified_name(qn.fullname(true)+"!", 0, qualified_name::AlreadyMangled)) ;
      }
   }
}
