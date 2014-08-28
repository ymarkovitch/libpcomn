/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_inclist.cpp
 COPYRIGHT    :   Yakov Markovitch, 2009-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unit tests of smartpointers.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   10 Feb 2009
*******************************************************************************/
#include <pcomn_incdlist.h>
#include <pcomn_unittest.h>

#include <string>
#include <iostream>

struct ListItem {
      ListItem(const char *nm, std::vector<std::string> *r = NULL) :
         name(nm),
         registry(r)
      {}

      ~ListItem()
      {
         if (registry)
            registry->push_back(name) ;
      }

      PCOMN_INCLIST_DECLARE(unmanaged_list) ;
      PCOMN_INCLIST_DECLARE(managed_list) ;

      const char *name ;

   private:
      std::vector<std::string> * registry ;
      pcomn::incdlist_node       listnode ;
} ;

PCOMN_INCLIST_DEFINE(ListItem, unmanaged_list, listnode, pcomn::incdlist) ;
PCOMN_INCLIST_DEFINE(ListItem, managed_list, listnode, pcomn::incdlist_managed) ;

inline std::ostream &operator<<(std::ostream &os, const ListItem &v)
{
   return os << "'" << v.name << "'" ;
}

/*******************************************************************************
                     class InclusiveDoublyLinkedListTests
*******************************************************************************/
class InclusiveDoublyLinkedListTests : public CppUnit::TestFixture {

      void Test_Unmanaged_List() ;

      CPPUNIT_TEST_SUITE(InclusiveDoublyLinkedListTests) ;

      CPPUNIT_TEST(Test_Unmanaged_List) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

/*******************************************************************************
 InclusiveDoublyLinkedListTests
*******************************************************************************/
void InclusiveDoublyLinkedListTests::Test_Unmanaged_List()
{
   ListItem::unmanaged_list List ;
   CPPUNIT_LOG_IS_FALSE(List.owns()) ;
   CPPUNIT_LOG_EQUAL(List.size(), (size_t)0) ;
   CPPUNIT_LOG_ASSERT(List.empty()) ;
   CPPUNIT_LOG_ASSERT(List.begin() == List.end()) ;
   CPPUNIT_LOG_IS_FALSE(List.begin() != List.end()) ;

   ListItem Item1 ("Item1") ;
   ListItem Item3 ("Item3") ;
   {
      ListItem Item2 ("Item2") ;

      CPPUNIT_LOG(std::endl) ;
      CPPUNIT_LOG_RUN(List.push_back(Item2)) ;
      CPPUNIT_LOG_IS_FALSE(List.empty()) ;
      CPPUNIT_LOG_EQUAL(List.size(), (size_t)1) ;

      CPPUNIT_LOG_RUN(List.push_back(Item3)) ;
      CPPUNIT_LOG_EQUAL(List.size(), (size_t)2) ;
      CPPUNIT_LOG_RUN(List.push_front(Item1)) ;
      CPPUNIT_LOG_EQUAL(List.size(), (size_t)3) ;

      CPPUNIT_LOG_EQUAL(&List.front(), &Item1) ;
      CPPUNIT_LOG_EQUAL(&List.back(), &Item3) ;

      ListItem::unmanaged_list::iterator ListIter = List.begin() ;
      CPPUNIT_LOG(std::endl) ;

      CPPUNIT_LOG_EQUAL(&*ListIter, &Item1) ;
      CPPUNIT_LOG_EQUAL(&*++ListIter, &Item2) ;
      CPPUNIT_LOG_EQUAL(&*++ListIter, &Item3) ;
      CPPUNIT_LOG_ASSERT(ListIter != List.end()) ;
      CPPUNIT_LOG_ASSERT(++ListIter == List.end()) ;
   }
   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(List.size(), (size_t)2) ;

   CPPUNIT_LOG_EQUAL(&List.front(), &Item1) ;
   CPPUNIT_LOG_EQUAL(&List.back(), &Item3) ;

   ListItem::unmanaged_list::iterator ListIter = List.begin() ;
   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(&*ListIter, &Item1) ;
   CPPUNIT_LOG_EQUAL(&*++ListIter, &Item3) ;
   CPPUNIT_LOG_ASSERT(ListIter != List.end()) ;
   CPPUNIT_LOG_ASSERT(++ListIter == List.end()) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(InclusiveDoublyLinkedListTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "unittest.diag.ini", "Test inclusive lists.") ;
}
