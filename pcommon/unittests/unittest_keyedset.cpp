/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_keyedset.cpp
 COPYRIGHT    :   Yakov Markovitch, 2014-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for a keyed set adapters.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   19 Sep 2014
*******************************************************************************/
#include <pcomn_keyedset.h>
#include <pcomn_string.h>
#include <pcomn_unittest.h>

#include <typeinfo>
#include <memory>
#include <fstream>

#include <stdio.h>

/*******************************************************************************
                     class KeyedSetTests
*******************************************************************************/
class KeyedSetTests : public CppUnit::TestFixture {

   private:
      void Test_Keyed_Set_Insert() ;
      void Test_Keyed_Set_Erase() ;
      void Test_Keyed_Set_Extract_Key() ;

      CPPUNIT_TEST_SUITE(KeyedSetTests) ;

      CPPUNIT_TEST(Test_Keyed_Set_Insert) ;
      CPPUNIT_TEST(Test_Keyed_Set_Erase) ;
      CPPUNIT_TEST(Test_Keyed_Set_Extract_Key) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

void KeyedSetTests::Test_Keyed_Set_Insert()
{
   test_inthashtable IntHash(4) ;
   CPPUNIT_LOG_EQUAL(IntHash.max_load_factor(), (float)0.75) ;
   CPPUNIT_LOG_EQUAL(IntHash.bucket_count(), (size_t)6) ;

   CPPUNIT_LOG_ASSERT(IntHash.insert(10).second) ;
   CPPUNIT_LOG(IntHash << std::endl) ;
   CPPUNIT_LOG_EQUAL(IntHash.size(), (size_t)1) ;
   CPPUNIT_LOG_ASSERT(IntHash.insert(4).second) ;
   CPPUNIT_LOG_EQUAL(IntHash.size(), (size_t)2) ;
   CPPUNIT_LOG_EQUAL(*IntHash.insert(11).first, 11L) ;
   CPPUNIT_LOG_EQUAL(IntHash.size(), (size_t)3) ;
   CPPUNIT_LOG(IntHash << " load_factor=" << IntHash.load_factor() << std::endl) ;

   CPPUNIT_LOG_EQUAL(IntHash.count(4), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(IntHash.count(11), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(IntHash.count(10), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(IntHash.count(5), (size_t)0) ;

   CPPUNIT_LOG_EQUAL(*IntHash.find(4), 4L) ;
   CPPUNIT_LOG_EQUAL(*IntHash.find(11), 11L) ;
   CPPUNIT_LOG_EQUAL(*IntHash.find(10), 10L) ;
   CPPUNIT_LOG_EQUAL(IntHash.find(5), IntHash.end()) ;

   CPPUNIT_LOG_ASSERT(IntHash.insert(5).second) ;
   CPPUNIT_LOG(IntHash << std::endl) ;
   CPPUNIT_LOG_ASSERT(IntHash.insert(26).second) ;
   CPPUNIT_LOG(IntHash << std::endl) ;
   CPPUNIT_LOG_ASSERT(IntHash.insert(28).second) ;
   CPPUNIT_LOG(IntHash << std::endl) ;
   CPPUNIT_LOG(IntHash << std::endl) ;

   CPPUNIT_LOG_EQUAL(*IntHash.find(4), 4L) ;
   CPPUNIT_LOG_EQUAL(*IntHash.find(11), 11L) ;
   CPPUNIT_LOG_EQUAL(*IntHash.find(10), 10L) ;
   CPPUNIT_LOG_EQUAL(*IntHash.find(5), 5L) ;
   CPPUNIT_LOG_EQUAL(*IntHash.find(26), 26L) ;
   CPPUNIT_LOG_EQUAL(*IntHash.find(28), 28L) ;
   CPPUNIT_LOG_EQUAL(IntHash.find(55), IntHash.end()) ;
}

void KeyedSetTests::Test_Keyed_Set_Erase()
{
   test_inthashtable IntHash ;
   CPPUNIT_LOG_EQUAL(IntHash.erase(20), (size_t)0) ;
   CPPUNIT_LOG(IntHash << std::endl) ;
   CPPUNIT_LOG_EQUAL(IntHash.size(), (size_t)0) ;
   long Value = 30 ;
   CPPUNIT_LOG_EQUAL(IntHash.erase(20, Value), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(Value, 30L) ;

   CPPUNIT_LOG_ASSERT(IntHash.insert(20).second) ;
   CPPUNIT_LOG_EQUAL(IntHash.size(), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(IntHash.erase(20), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(IntHash.size(), (size_t)0) ;
   CPPUNIT_LOG(IntHash << std::endl) ;
   CPPUNIT_LOG_EQUAL(IntHash.begin(), IntHash.end()) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG("Testing erasing in the presence of collisions" << std::endl) ;
   const int BucketCount = IntHash.bucket_count() ;
   CPPUNIT_LOG("Bucket count = " << BucketCount << std::endl) ;

   CPPUNIT_LOG_ASSERT(IntHash.insert(BucketCount + 4).second) ;
   CPPUNIT_LOG_ASSERT(IntHash.insert(4).second) ;
   CPPUNIT_LOG_ASSERT(IntHash.insert(BucketCount + 5).second) ;
   CPPUNIT_LOG_ASSERT(IntHash.insert(5).second) ;
   CPPUNIT_LOG_EQUAL(IntHash.size(), (size_t)4) ;
   CPPUNIT_LOG(IntHash << std::endl) ;
   CPPUNIT_LOG_EQUAL(IntHash.erase(BucketCount + 4), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(IntHash.size(), (size_t)3) ;
   CPPUNIT_LOG(IntHash << std::endl) ;
   CPPUNIT_LOG_EQUAL(IntHash.count(BucketCount + 4), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(IntHash.count(4), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(*IntHash.find(4), 4L) ;
   CPPUNIT_LOG_EQUAL(*IntHash.find(5), 5L) ;
   CPPUNIT_LOG_EQUAL(*IntHash.find(BucketCount + 5), BucketCount + 5L) ;
   CPPUNIT_LOG_EQUAL(IntHash.erase(4), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(IntHash.size(), (size_t)2) ;
   CPPUNIT_LOG_EQUAL(IntHash.count(4), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(*IntHash.find(5), 5L) ;
   CPPUNIT_LOG_EQUAL(*IntHash.find(BucketCount + 5), BucketCount + 5L) ;
   CPPUNIT_LOG_EQUAL(std::distance(IntHash.begin(), IntHash.end()), (ptrdiff_t)2) ;

   CPPUNIT_LOG_EQUAL(CPPUNIT_SORTED(std::vector<int>(IntHash.begin(), IntHash.end())),
                     CPPUNIT_CONTAINER(std::vector<int>, (5)(BucketCount + 5))) ;

   CPPUNIT_LOG_RUN(IntHash.erase(IntHash.find(BucketCount + 5))) ;

   CPPUNIT_LOG_EQUAL(CPPUNIT_SORTED(std::vector<int>(IntHash.begin(), IntHash.end())),
                     CPPUNIT_CONTAINER(std::vector<int>, (5))) ;
}

static const char * const Hello = "Hello" ;
static const char * const World = "world" ;
static const char * const Foo = "Foo" ;
static const char * const Bar = "Bar" ;
static const char * const Quux = "Quux" ;
static const char * const Xyzzy = "Xyzzy" ;
static const char * const Baby = "Baby" ;
static const char * const Bye = "Bye" ;

struct KeyedHashval {
      uint64_t key ;
      char     str[64] ;
} ;

struct extract_key : std::unary_function<KeyedHashval, uint64_t> {
      uint64_t operator()(const KeyedHashval &val) const { return val.key ; }
} ;

void KeyedSetTests::Test_Keyed_Set_Extract_Key()
{
   typedef pcomn::closed_hashtable<KeyedHashval, extract_key> testtable_type ;
   CPPUNIT_LOG_EQUAL(typeid(testtable_type::key_type), typeid(uint64_t)) ;

   testtable_type TestHash ;

   KeyedHashval values[] = {
      { 3467, "v:3467" },
      { 7777, "v:7777" },
      { 7133, "v:7133" },
      { 0, "v:0" }
   } ;

   CPPUNIT_LOG_RUN(TestHash.insert(values + 0, values + 4)) ;

   CPPUNIT_LOG(TestHash << std::endl) ;
   CPPUNIT_LOG_EQUAL(TestHash.size(), (size_t)4) ;

   CPPUNIT_LOG_EQUAL(TestHash.count(3467), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(TestHash.count(7777), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(TestHash.count(7133), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(TestHash.count(0), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(TestHash.count(1), (size_t)0) ;

   CPPUNIT_LOG_EQUAL(std::string(TestHash.find(3467)->str), std::string("v:3467")) ;
   CPPUNIT_LOG_EQUAL(std::string(TestHash.find(7777)->str), std::string("v:7777")) ;
   CPPUNIT_LOG_EQUAL(std::string(TestHash.find(7133)->str), std::string("v:7133")) ;
   CPPUNIT_LOG_EQUAL(std::string(TestHash.find(0)->str), std::string("v:0")) ;

   CPPUNIT_LOG_ASSERT(TestHash.erase(7777)) ;
   CPPUNIT_LOG_ASSERT(TestHash.erase(7133)) ;
   CPPUNIT_LOG_EQUAL(TestHash.count(3467), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(TestHash.count(7777), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(TestHash.count(7133), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(TestHash.count(0), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(std::string(TestHash.find(3467)->str), std::string("v:3467")) ;
   CPPUNIT_LOG_EQUAL(std::string(TestHash.find(0)->str), std::string("v:0")) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(KeyedSetTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv, "unittest.diag.ini", "Keyed set adapters tests") ;
}
