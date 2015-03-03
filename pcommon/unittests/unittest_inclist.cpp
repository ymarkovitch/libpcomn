/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_inclist.cpp
 COPYRIGHT    :   Yakov Markovitch, 2009-2015. All rights reserved.
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

      const char *name ;

   private:
      std::vector<std::string> * registry ;
      pcomn::incdlist_node       listnode ;

   public:
      PCOMN_INCLIST_DEFINE(unmanaged_list, pcomn::incdlist, ListItem, listnode) ;
      PCOMN_INCLIST_DEFINE(managed_list, pcomn::incdlist_managed, ListItem, listnode) ;
} ;

inline std::ostream &operator<<(std::ostream &os, const ListItem &v)
{
   return os << "'" << v.name << "'" ;
}

struct SListItem {
      SListItem(const char *nm) : name(nm) {}

      const char * const name ;

      SListItem *next1() const { return _next1 ; }
      SListItem *next2() const { return _next2 ; }

   private:
      SListItem *_next1 = nullptr ;
      SListItem *_next2 = nullptr ;

      PCOMN_NONCOPYABLE(SListItem) ;
      PCOMN_NONASSIGNABLE(SListItem) ;
   public:
      typedef pcomn::incslist<SListItem, &SListItem::_next1> slist1_type ;
      typedef pcomn::incslist<SListItem, &SListItem::_next2> slist2_type ;
} ;

/*******************************************************************************
                     class InclusiveListTests
*******************************************************************************/
class InclusiveListTests : public CppUnit::TestFixture {

      void Test_Unmanaged_Double_List() ;
      void Test_Single_List() ;

      CPPUNIT_TEST_SUITE(InclusiveListTests) ;

      CPPUNIT_TEST(Test_Unmanaged_Double_List) ;
      CPPUNIT_TEST(Test_Single_List) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

/*******************************************************************************
 InclusiveListTests
*******************************************************************************/
void InclusiveListTests::Test_Unmanaged_Double_List()
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

void InclusiveListTests::Test_Single_List()
{
   using namespace pcomn ;

   SListItem e1("1"), e2("2"), e3("3"), e4("4"), e5("5"), e6("6"), e7("7") ;

   SListItem::slist1_type s1_1, s1_2 ;
   SListItem::slist2_type s2_1, s2_2 ;

   CPPUNIT_LOG_IS_NULL(e1.next1()) ;
   CPPUNIT_LOG_IS_NULL(e1.next2()) ;
   CPPUNIT_LOG_IS_NULL(e7.next1()) ;
   CPPUNIT_LOG_IS_NULL(e7.next2()) ;
   CPPUNIT_LOG_EQ(s1_1.size(), 0) ;
   CPPUNIT_LOG_EQ(s2_1.size(), 0) ;

   CPPUNIT_LOG_ASSERT(s1_1.begin() == s1_1.end()) ;
   CPPUNIT_LOG_IS_FALSE(s1_1.begin() != s1_1.end()) ;
   CPPUNIT_LOG_ASSERT(s2_1.begin() == s2_1.end()) ;
   CPPUNIT_LOG_IS_FALSE(s2_1.begin() != s2_1.end()) ;

   CPPUNIT_LOG_EQ(std::distance(s1_1.begin(), s1_1.end()), 0) ;
   CPPUNIT_LOG_EQ(std::distance(s2_1.begin(), s2_1.end()), 0) ;
   CPPUNIT_LOG_ASSERT(s1_1.empty()) ;
   CPPUNIT_LOG_ASSERT(s2_1.empty()) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(s1_1.push_front(e1)) ;
   CPPUNIT_LOG_EQ(s1_1.size(), 1) ;
   CPPUNIT_LOG_EQ(std::distance(s1_1.begin(), s1_1.end()), 1) ;
   CPPUNIT_LOG_EQ(strslice(s1_1.front().name), "1") ;

   CPPUNIT_LOG_RUN(s1_1.push_front(e3)) ;
   CPPUNIT_LOG_EQ(s1_1.size(), 2) ;
   CPPUNIT_LOG_EQ(std::distance(s1_1.begin(), s1_1.end()), 2) ;
   CPPUNIT_LOG_EQ(strslice(s1_1.front().name), "3") ;

   auto s1i = s1_1.begin() ;
   CPPUNIT_LOG_EQ(strslice(s1i->name), "3") ;
   CPPUNIT_LOG_EQ(strslice((*s1i).name), "3") ;

   SListItem::slist1_type::const_iterator s1ci = s1i ;
   CPPUNIT_LOG_EQ(strslice((++s1i)->name), "1") ;
   CPPUNIT_LOG_ASSERT(s1i != s1_1.begin()) ;
   CPPUNIT_LOG_ASSERT(s1i != s1_1.end()) ;
   CPPUNIT_LOG_ASSERT(s1i != s1_1.cbegin()) ;
   CPPUNIT_LOG_ASSERT(s1i != s1_1.cend()) ;

   CPPUNIT_LOG_ASSERT(s1ci == s1_1.cbegin()) ;
   CPPUNIT_LOG_ASSERT(s1ci != s1_1.cend()) ;
   CPPUNIT_LOG_ASSERT(s1ci == s1_1.begin()) ;
   CPPUNIT_LOG_ASSERT(s1ci != s1_1.end()) ;
   CPPUNIT_LOG_EQ(strslice(s1ci->name), "3") ;
   CPPUNIT_LOG_EQ(strslice((s1ci++)->name), "3") ;
   CPPUNIT_LOG_EQ(strslice(s1ci->name), "1") ;

   CPPUNIT_LOG_ASSERT(s1i == s1ci) ;
   CPPUNIT_LOG_EQ(strslice((s1i++)->name), "1") ;
   CPPUNIT_LOG_ASSERT(s1i == s1_1.end()) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(InclusiveListTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv, "unittest.diag.ini", "Test inclusive lists.") ;
}
