/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   ivecttst.cpp
 COPYRIGHT    :   Yakov Markovitch, 1999-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   ivector<> tests

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   31 Jul 1999
*******************************************************************************/
#include <ivector.h>

#include <iomanip.h>
#include <algorithm>

class TestObject
{
   public:
      TestObject() :
         num (lastNum++)
      {
         std::cout << std::setw(5) << num << " constructor" << std::endl ;
      }

      ~TestObject()
      {
         std::cout << std::setw(5) << num << " destructor" << std::endl ;
      }

      bool operator == (const TestObject &) const
      {
         return true ;
      }

      static unsigned long lastNum ;

   private:
      unsigned long num ;
} ;

unsigned long TestObject::lastNum ;

typedef ivector<TestObject> ivtest ;

class test_generator
{
   public:
      TestObject *operator()() const
      {
         return new TestObject ;
      }
} ;

void firstTest()
{
   TestObject::lastNum = 0 ;

   std::cout << "Test 1" << std::endl ;
   {
      ivtest iv1 ;
      ivtest iv2 (0, true) ;
      ivtest iv3 (10, true) ;

      std::generate_n(std::back_inserter(iv1), 10, test_generator()) ;
      std::generate_n(std::back_inserter(iv2), 10, test_generator()) ;
   }
   cout << "End of test 1" << endl ;
}

void secondTest()
{
   TestObject::lastNum = 0 ;

   std::cout << "Test 2" << std::endl ;
   {
      ivtest iv1 ;
      ivtest iv2 (0, true) ;

      std::generate_n(std::back_inserter(iv1), 10, test_generator()) ;
      std::generate_n(std::back_inserter(iv2), 10, test_generator()) ;

      iv2.resize (iv2.size()+1) ;
      iv2.resize (iv2.size()-2) ;
      iv2.insert (iv2.begin()+3, iv1.begin()+5, iv1.end()) ;
      iv2.erase (iv2.begin()) ;
      iv2.erase (iv2.end()-1) ;
      iv2.erase (iv2.begin()+3, iv2.end()-3) ;
   }
   std::cout << "End of test 2" << std::endl ;
}

void main()
{
   firstTest() ;
   secondTest() ;
   std::cout << "That's all..." << std::endl ;
}
