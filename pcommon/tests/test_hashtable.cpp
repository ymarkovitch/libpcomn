/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   test_hashtable.cpp
 COPYRIGHT    :   Yakov Markovitch, 2002-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Hash table tests

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   30 Aug 2002
*******************************************************************************/
#include <pcomn_hashtable.h>
#include <pcomn_string.h>
#include <pcomn_utils.h>
#include <pcounter.h>

#include <string>
#include <map>
#include <iostream>
#include <fstream>

typedef PTCounter<0> counter_type ;
typedef pcomn::hashtable<std::string, counter_type> word_table ;
typedef std::map<std::string, counter_type>        word_map ;

//
// Print out a list of strings.
//
template <typename K, typename V, class H, class C, class L>
inline std::ostream& operator<< (std::ostream& os, const pcomn::hashtable<K, V, H, C, L>& tbl)
{
   os << "size=" << tbl.size() << " buckets=" << tbl.capacity() << std::endl ;
   std::copy(tbl.begin(), tbl.end(), std::ostream_iterator<word_table::entry_type::Parent>(os,"\n")) ;
   return os ;
}

class test_error : public std::runtime_error {
   public:
      test_error(const std::string &message) :
         std::runtime_error(message)
      {}
} ;

class HashTableFixture {
   public:
      word_table _table ;
      word_map   _map ;

      // Read a file, split it by words and count unique words, using both
      // std::map
      void test1(const char *filename) ;
      // Check whether all words from the map present in the table
      void test2() ;
      // Check whether there is no duplicate words in the table
      void test3() ;
      // Remove all entries from the table, one-by-one
      void test4() ;
      // Test for automatic hash table shrink/grow
      void test5() ;
} ;

void HashTableFixture::test1(const char *filename)
{
   std::cout << "Test 1. Checking whether a test and a hash table produce equal results." << std::endl ;

   std::string word ;
   PTSafePtr<std::istream> _is ;
   std::istream &is = filename ? *(_is = new std::ifstream(filename)) : std::cin ;

   while(is >> word)
   {
      _table.insert(word, 0).first->value()++ ;
      ++_map[word] ;
   }
   std::cout << "word_map size=" << _map.size()
             << " word_table size=" << _table.size() << std::endl ;
   if (_map.size() != _table.size())
      throw test_error ("Sizes are different.") ;

   std::cout << "Test 1 OK" << std::endl ;
}

void HashTableFixture::test2()
{
   std::cout << "Test 2. Checking whether all unique words from the source present in the table." << std::endl ;

   // Check whether all words from the map present in the table
   word_map::iterator end = _map.end() ;
   for(word_map::iterator iter = _map.begin() ; iter != end ; ++iter)
   {
      word_table::iterator found (_table.find((*iter).first)) ;
      if (!found)
         throw test_error ("'" + (*iter).first + "' is absent in the table but present in the map.") ;
      if (*found != *iter)
         throw test_error ("Key/value pair found in the table by key '" + (*iter).first + "' doesn't match") ;
   }

   std::cout << "Test 2 OK" << std::endl ;
}

void HashTableFixture::test3()
{
   std::cout << "Test 3. Checking for duplicate entries." << std::endl ;
   // Checking for duplicate entries
   word_map tmpmap ;
   word_table::iterator end = _table.end() ;
   for(word_table::iterator iter = _table.begin() ; iter != end ; ++iter)
      if (tmpmap[iter->key()]++)
         throw test_error ("Duplicate key '" + iter->key() + " in the table.") ;

   std::cout << "Test 3 OK" << std::endl ;
}


void HashTableFixture::test4()
{
   std::cout << "Test 3. Deleting all table entries one-by-one." << std::endl ;
   std::cout << "Table size before test is " << _table.size() << std::endl ;
   // Check whether all words from the map present in the table
   word_table::iterator end = _table.end() ;
   for(word_table::iterator iter = _table.begin() ; iter != end ; )
      if (!_table.erase(iter++))
         throw test_error ("Attempt to remove an entry pointed to by a valid iterator failed") ;
   std::cout << "Table size after test is " << _table.size() << std::endl ;
   if (_table.size())
         throw test_error (strprintf(
                              "The table is not empty after all entries has been deleted. size=%u", _table.size())) ;
   else if (_table.begin() != _table.end())
      throw test_error (strprintf("The begin and end iterators by the table are unequal")) ;

   std::cout << "Test 4 OK" << std::endl ;
}

void HashTableFixture::test5()
{
   /*
   int cnt = 0 ;
   for(word_map::const_iterator wbeg = wmap.begin(), wend = wmap.end() ; wbeg != wend ; ++wbeg)
   {
      tbl.erase((*wbeg).first) ;
      if (cnt % 9)
      {
         tbl.insert((*wbeg).first + (*wbeg).first, 0) ;
         if (cnt % 7)
            tbl.erase((*wbeg).first + (*wbeg).first) ;
         tbl.insert((*wbeg).first + (*wbeg).first + (*wbeg).first, 0) ;
         tbl.erase((*wbeg).first + (*wbeg).first + (*wbeg).first) ;
         tbl.insert((*wbeg).first + (*wbeg).first + (*wbeg).first + (*wbeg).first, 0) ;
         tbl.erase((*wbeg).first + (*wbeg).first + (*wbeg).first + (*wbeg).first) ;
      }
      ++cnt ;
   }
   */
}

int main(int, char *argv[])
{
   DIAG_INITTRACE("pcommontest.ini") ;

   try {
      HashTableFixture tests ;

      tests.test1(argv[1]) ;
      tests.test2() ;
      tests.test3() ;
      tests.test4() ;
      std::cout << "All tests completed OK" << std::endl ;
      return 0 ;
   }
   catch(const test_error &x)
   {
      std::cout << "Test failed: " << x.what() << std::endl ;
   }
   catch(const std::exception &x)
   {
      std::cout << "Error: " << x.what() << std::endl ;
   }
   return 1 ;
}
