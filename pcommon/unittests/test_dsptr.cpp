/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   test_dsptr.cpp
 COPYRIGHT    :   Yakov Markovitch, 2000-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   A simple test of direct smartpointers

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   18 Jul 2000
*******************************************************************************/
#include <sptrbase.h>
#include <string>
#include <iostream>

class Test : public PRefCount {
   public:
      Test(const std::string &nm) :
         name(nm)
      {
         std::cout << (void *)this << " with name '" << name << "' constructed" << std::endl ;
      }

      ~Test()
      {
         std::cout << (void *)this << " with name '" << name << "' destructed" << std::endl ;
      }

      const std::string name ;
} ;

void main()
{
   shared_intrusive_ptr<Test> tp(new Test("first")) ;
   shared_intrusive_ptr<Test> to ;
   std::cout << "to=tp" << std::endl ;
   to = tp ;
   std::cout << "to == '" << to->name << "' at " << &*to << std::endl ;
   std::cout << "tp=new Test" << std::endl ;
   tp = new Test("second") ;
   std::cout << "to=NULL" << std::endl ;
   to = NULL ;
   std::cout << "tp == '" << tp->name << "' at " << &*tp << std::endl ;
   std::cout << "...and now finishing..." << std::endl ;
}
