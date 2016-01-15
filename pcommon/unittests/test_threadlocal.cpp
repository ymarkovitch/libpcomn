/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   test_threadlocal.cpp
 COPYRIGHT    :   Yakov Markovitch, 2014-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   14 Jan 2014
*******************************************************************************/
#include <pcomn_trace.h>
#include <pcomn_utils.h>

#include <thread>
#include <iterator>

using namespace pcomn ;

struct A {
      A() { std::cout << "A() " << this << std::endl ; }
      ~A() { std::cout << "~A() " << this << std::endl ; }
      int x = 5 ;
} ;

struct B {
      B() { std::cout << "B() " << this << std::endl ; }
      ~B() { std::cout << "~B() " << this << std::endl ; }
      int x = 10 ;
} ;

thread_local A a1 ;
thread_local A a2 ;

int main(int, char *[])
{
   std::cout << "ENTERED the main thread" << std::endl ;
   //std::cout << a1.x << std::endl ;
   //std::cout << a2.x << std::endl ;
   try
   {
      std::thread t1 ([]()
                      {
                         std::cout << "Started thread " << std::this_thread::get_id() << std::endl ;
                         (void)a2 ;
                         (void)a1 ;
                         //std::cout << a1.x << std::endl ;
                         //std::cout << a2.x << std::endl ;
                      }) ;

      std::cout << "Joining " << t1.get_id() << std::endl ;
      t1.join() ;
      std::cout << "Joined " << t1.get_id() << std::endl ;
      std::cout << "EXITING the main thread" << std::endl ;
      return 0 ;
   }
   catch(const std::exception &x)
   {
      std::cout << "Exception " << PCOMN_TYPENAME(x) << ": " << x.what() << std::endl ;
   }
   return 1 ;
}
