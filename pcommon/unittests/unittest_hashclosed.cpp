/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_hashclosed.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for a closed hashtable.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   20 Apr 2008
*******************************************************************************/
#include <pcomn_hashclosed.h>
#include <pcomn_string.h>
#include <pcomn_unittest.h>

#include <typeinfo>
#include <memory>
#include <fstream>

#include <stdio.h>

namespace CppUnit {

template<>
inline std::string assertion_traits<pcomn::bucket_state>::toString(const pcomn::bucket_state &s)
{
   return std::to_string((uintptr_t)s) ;
}

}

/*******************************************************************************
                     class ClosedHashTests
*******************************************************************************/
class ClosedHashTests : public CppUnit::TestFixture {

   protected:
      // A hashtable with a hash function that allows to create predictable collisions,
      // which enables testing of a hash table behaviour in the presence of collisions.
      typedef pcomn::closed_hashtable<long, pcomn::identity, pcomn::hash_identity<long> > test_inthashtable ;

   private:
      void Test_Hash_Functions() ;
      void Test_Hashtable_Bucket() ;
      void Test_Closed_Hash_Empty() ;
      void Test_Closed_Hash_Init() ;
      void Test_Closed_Hash_Insert_One() ;
      void Test_Closed_Hash_Insert() ;
      void Test_Closed_Hash_Erase() ;
      void Test_Static_Optimization_Insert() ;
      void Test_Static_Optimization_Erase() ;
      void Test_Static_Optimization_Grow() ;
      void Test_Closed_Hash_Copy() ;
      void Test_Closed_Hash_Move() ;
      void Test_Closed_Hash_Extract_Key() ;

      CPPUNIT_TEST_SUITE(ClosedHashTests) ;

      CPPUNIT_TEST(Test_Hash_Functions) ;
      CPPUNIT_TEST(Test_Hashtable_Bucket) ;
      CPPUNIT_TEST(Test_Closed_Hash_Empty) ;
      CPPUNIT_TEST(Test_Closed_Hash_Init) ;
      CPPUNIT_TEST(Test_Closed_Hash_Insert_One) ;
      CPPUNIT_TEST(Test_Closed_Hash_Insert) ;
      CPPUNIT_TEST(Test_Closed_Hash_Erase) ;
      CPPUNIT_TEST(Test_Static_Optimization_Insert) ;
      CPPUNIT_TEST(Test_Static_Optimization_Erase) ;
      CPPUNIT_TEST(Test_Static_Optimization_Grow) ;
      CPPUNIT_TEST(Test_Closed_Hash_Copy) ;
      CPPUNIT_TEST(Test_Closed_Hash_Move) ;
      CPPUNIT_TEST(Test_Closed_Hash_Extract_Key) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

static const char * const StrArray[] =
{
   "Str1", "Str2", "Str3", "Str4", "Str5", "Str6", "Str7", "Str8", "Str9", "Str10",
   "Str1", "Str2", "Str3", "Str4", "Str5", "Str6", "Str7", "Str8", "Str9", "Str10"
} ;

void ClosedHashTests::Test_Hash_Functions()
{
   CPPUNIT_LOG_NOT_EQUAL(pcomn::hash_fn<int>()(0), (size_t)0) ;
   CPPUNIT_LOG_NOT_EQUAL(pcomn::hash_fn<int>()(1), (size_t)1) ;
   CPPUNIT_LOG_NOT_EQUAL(pcomn::hash_fn<int>()(1), pcomn::hash_fn<int>()(0)) ;

   CPPUNIT_LOG_EQUAL(pcomn::hash_fn<int>()(13), pcomn::hash_fn<long>()(13)) ;
   CPPUNIT_LOG_EQUAL(pcomn::hash_fn<unsigned short>()(13), pcomn::hash_fn<int>()(13)) ;

   CPPUNIT_LOG_EQUAL(pcomn::hash_fn<unsigned char>()(13), (size_t)13) ;
   CPPUNIT_LOG_EQUAL(pcomn::hash_fn<bool>()(true), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(pcomn::hash_fn<bool>()(false), (size_t)0) ;

   CPPUNIT_LOG_EQUAL(pcomn::hash_fn_raw<int>()(13), (size_t)13) ;
   CPPUNIT_LOG_EQUAL(pcomn::hash_fn_raw<int>()(0), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(pcomn::hash_fn_raw<std::string>()("Hello!"), pcomn::hash_fn<std::string>()("Hello!")) ;

   const char * const Hello = "Hello, world!" ;
   CPPUNIT_LOG_ASSERT(pcomn::hasher(Hello) != pcomn::hasher((const void *)Hello)) ;
   CPPUNIT_LOG_EQUAL(pcomn::hasher(Hello), pcomn::hasher("Hello, world!")) ;
   CPPUNIT_LOG_EQUAL(pcomn::hasher(Hello), pcomn::hasher(std::string("Hello, world!"))) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(pcomn::hash_fn_seq<std::string>()(CPPUNIT_CONTAINER(std::vector<const char *>, ("Foo")("Bar"))),
                     pcomn::Hash().append_data("Foo").append_data("Bar").value()) ;

   CPPUNIT_LOG_NOT_EQUAL(pcomn::hash_fn_seq<std::string>()(CPPUNIT_CONTAINER(std::vector<const char *>, ("Foo"))),
                     pcomn::Hash().append_data("Foo").append_data("Bar").value()) ;

   const char * const Foo = "Foo" ;
   const char * const Bar = "Bar" ;
   CPPUNIT_LOG_EQUAL(pcomn::hash_fn_seq<const char *>()(CPPUNIT_CONTAINER(std::vector<const char *>, (Foo)(Bar))),
                     pcomn::Hash().append_data(Foo).append_data(Bar).value()) ;
   CPPUNIT_LOG_NOT_EQUAL(pcomn::hash_fn_seq<const void *>()(CPPUNIT_CONTAINER(std::vector<const char *>, (Foo)(Bar))),
                         pcomn::Hash().append_data(Foo).append_data(Bar).value()) ;

   CPPUNIT_LOG_EQUAL(pcomn::hash_fn_seq<const char *>()(CPPUNIT_CONTAINER(std::vector<const char *>, (Foo)(Bar))),
                     pcomn::Hash().append(pcomn::hasher(Foo)).append_data(Bar).value()) ;
   CPPUNIT_LOG_EQUAL(pcomn::hash_fn_seq<const char *>()(CPPUNIT_CONTAINER(std::vector<const char *>, (Foo)(Bar))),
                     pcomn::Hash(pcomn::hasher(Foo)).append_data(Bar).value()) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(pcomn::hash_fn_seq<int>()(CPPUNIT_CONTAINER(std::vector<int>, (1)(2)(3))),
                     pcomn::Hash().append_data(1).append_data(2).append_data(3).value()) ;
   CPPUNIT_LOG_NOT_EQUAL((pcomn::hash_fn_seq<int, pcomn::hash_fn_raw<int> >()(CPPUNIT_CONTAINER(std::vector<int>, (1)(2)(3)))),
                         pcomn::Hash().append_data(1).append_data(2).append_data(3).value()) ;
   CPPUNIT_LOG_EQUAL((pcomn::hash_fn_seq<int, pcomn::hash_fn_raw<int> >()(CPPUNIT_CONTAINER(std::vector<int>, (1)(2)(3)))),
                     pcomn::Hash().append(1).append(2).append(3).value()) ;
}

void ClosedHashTests::Test_Hashtable_Bucket()
{
   using namespace pcomn ;

   closed_hashtable_bucket<int> IntBucket ;
   closed_hashtable_bucket<void *> PBucket ;
   closed_hashtable_bucket<const char *> CBucket ;

   CPPUNIT_LOG_EQUAL(sizeof(PBucket), sizeof(void *)) ;
   CPPUNIT_LOG_EQUAL(sizeof(CBucket), sizeof(char *)) ;
   CPPUNIT_LOG_ASSERT(sizeof(IntBucket) > sizeof(int)) ;

   CPPUNIT_LOG_EQUAL(IntBucket.state(), bucket_state::Empty) ;
   CPPUNIT_LOG_ASSERT(IntBucket.is_available()) ;
   CPPUNIT_LOG_EQUAL(CBucket.state(), bucket_state::Empty) ;
   CPPUNIT_LOG_ASSERT(CBucket.is_available()) ;

   const char * const Hello = "Hello, world!" ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(IntBucket.set_value(3)) ;
   CPPUNIT_LOG_RUN(CBucket.set_value(Hello)) ;

   CPPUNIT_LOG_EQUAL(IntBucket.state(), bucket_state::Valid) ;
   CPPUNIT_LOG_IS_FALSE(IntBucket.is_available()) ;
   CPPUNIT_LOG_EQUAL(CBucket.state(), bucket_state::Valid) ;
   CPPUNIT_LOG_IS_FALSE(CBucket.is_available()) ;
   CPPUNIT_LOG_EQUAL(IntBucket.value(), 3) ;
   CPPUNIT_LOG_EQUAL(CBucket.value(), Hello) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(IntBucket.set_state(bucket_state::Deleted)) ;
   CPPUNIT_LOG_RUN(CBucket.set_state(bucket_state::Deleted)) ;
   CPPUNIT_LOG_EQUAL(IntBucket.state(), bucket_state::Deleted) ;
   CPPUNIT_LOG_ASSERT(IntBucket.is_available()) ;
   CPPUNIT_LOG_EQUAL(CBucket.state(), bucket_state::Deleted) ;
   CPPUNIT_LOG_ASSERT(CBucket.is_available()) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(IntBucket.set_value(3)) ;
   CPPUNIT_LOG_RUN(CBucket.set_value(Hello)) ;

   CPPUNIT_LOG_EQUAL(IntBucket.state(), bucket_state::Valid) ;
   CPPUNIT_LOG_IS_FALSE(IntBucket.is_available()) ;
   CPPUNIT_LOG_EQUAL(CBucket.state(), bucket_state::Valid) ;
   CPPUNIT_LOG_IS_FALSE(CBucket.is_available()) ;
   CPPUNIT_LOG_EQUAL(IntBucket.value(), 3) ;
   CPPUNIT_LOG_EQUAL(CBucket.value(), Hello) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(IntBucket.set_state(bucket_state::End)) ;
   CPPUNIT_LOG_RUN(CBucket.set_state(bucket_state::End)) ;
   CPPUNIT_LOG_EQUAL(IntBucket.state(), bucket_state::End) ;
   CPPUNIT_LOG_IS_FALSE(IntBucket.is_available()) ;
   CPPUNIT_LOG_EQUAL(CBucket.state(), bucket_state::End) ;
   CPPUNIT_LOG_IS_FALSE(CBucket.is_available()) ;
}

void ClosedHashTests::Test_Closed_Hash_Empty()
{
   pcomn::closed_hashtable<int> IntHash ;
   pcomn::closed_hashtable<const char *> CharPtrHash ;

   CPPUNIT_LOG_ASSERT(IntHash.empty()) ;
   CPPUNIT_LOG_EQUAL(IntHash.size(), (size_t)0) ;
   CPPUNIT_LOG(IntHash << std::endl) ;
   CPPUNIT_LOG_EQUAL(IntHash.erase(0), (size_t)0) ;
   CPPUNIT_LOG(IntHash << std::endl) ;
   CPPUNIT_LOG_EQUAL(IntHash.count(1), (size_t)0) ;
   CPPUNIT_LOG_RUN(IntHash.clear()) ;
   CPPUNIT_LOG_ASSERT(IntHash.empty()) ;
   CPPUNIT_LOG_EQUAL(IntHash.size(), (size_t)0) ;

   CPPUNIT_LOG_ASSERT(CharPtrHash.empty()) ;
   CPPUNIT_LOG_EQUAL(CharPtrHash.size(), (size_t)0) ;
   CPPUNIT_LOG(CharPtrHash << std::endl) ;
   CPPUNIT_LOG_EQUAL(CharPtrHash.erase("Hello"), (size_t)0) ;
   CPPUNIT_LOG(CharPtrHash << std::endl) ;
   CPPUNIT_LOG_EQUAL(CharPtrHash.count("Hello"), (size_t)0) ;
   CPPUNIT_LOG_RUN(CharPtrHash.clear()) ;
   CPPUNIT_LOG_ASSERT(CharPtrHash.empty()) ;
   CPPUNIT_LOG_EQUAL(CharPtrHash.size(), (size_t)0) ;
   CPPUNIT_LOG_ASSERT(CharPtrHash.find("Hello") == CharPtrHash.end()) ;
}

void ClosedHashTests::Test_Closed_Hash_Init()
{
   test_inthashtable IntHash(4) ;
   CPPUNIT_LOG_EQ(IntHash.max_load_factor(), 0.75) ;
   CPPUNIT_LOG_EQUAL(IntHash.bucket_count(), (size_t)6) ;

   test_inthashtable IntHash2 ({4, 0.5}) ;
   CPPUNIT_LOG_EQ(IntHash2.max_load_factor(), 0.5) ;
   CPPUNIT_LOG_EQ(IntHash2.bucket_count(), 8) ;

   test_inthashtable IntHash3 ({1, {}}) ;
   CPPUNIT_LOG_EQ(IntHash3.max_load_factor(), 0.75) ;
   CPPUNIT_LOG_EQ(IntHash3.bucket_count(), 2) ;

   test_inthashtable IntHash4 ({1, -1}) ;
   CPPUNIT_LOG_EQ(IntHash4.max_load_factor(), 0.75) ;
   CPPUNIT_LOG_EQ(IntHash4.bucket_count(), 2) ;

   test_inthashtable IntHash5 ({0, 1}) ;
   CPPUNIT_LOG_EQ(IntHash5.max_load_factor(), 0.875) ;

   test_inthashtable IntHash6 ({0, 0.05}) ;
   CPPUNIT_LOG_EQ(IntHash6.max_load_factor(), 0.125) ;
}

void ClosedHashTests::Test_Closed_Hash_Insert_One()
{
   pcomn::closed_hashtable<long> IntHash ;
   CPPUNIT_LOG_ASSERT(IntHash.empty()) ;
   CPPUNIT_LOG_EQUAL(IntHash.size(), (size_t)0) ;
   CPPUNIT_LOG_EQ(IntHash.load_factor(), 1.0) ;
   CPPUNIT_LOG(IntHash << std::endl) ;
   CPPUNIT_LOG_ASSERT(IntHash.insert(20).second) ;
   CPPUNIT_LOG(IntHash << " load_factor=" << IntHash.load_factor() << std::endl) ;
   CPPUNIT_LOG_EQUAL(IntHash.size(), (size_t)1) ;
   CPPUNIT_LOG_IS_FALSE(IntHash.empty()) ;
   CPPUNIT_LOG_ASSERT(0.0 < IntHash.load_factor() && IntHash.load_factor() < 1.0) ;
   CPPUNIT_LOG_IS_TRUE(IntHash.begin() == IntHash.begin()) ;
   CPPUNIT_LOG_IS_TRUE(IntHash.end() == IntHash.end()) ;
   CPPUNIT_LOG_IS_FALSE(IntHash.begin() == IntHash.end()) ;
   CPPUNIT_LOG_IS_TRUE(IntHash.begin() != IntHash.end()) ;
   CPPUNIT_LOG_EQUAL((int)std::distance(IntHash.begin(), IntHash.end()), 1) ;

   pcomn::closed_hashtable<long>::iterator iter = IntHash.begin() ;
   CPPUNIT_LOG("begin=" << IntHash.begin() << " end=" << IntHash.end() << std::endl) ;
   CPPUNIT_LOG_EQUAL(++iter, IntHash.end()) ;
   CPPUNIT_LOG_RUN(iter = IntHash.begin()) ;
   CPPUNIT_LOG_EQUAL(iter++, IntHash.begin()) ;
   CPPUNIT_LOG_EQUAL(iter, IntHash.end()) ;
   CPPUNIT_LOG_EQUAL(*IntHash.begin(), 20L) ;
   CPPUNIT_LOG_EQUAL(IntHash.find(20), IntHash.begin()) ;
   CPPUNIT_LOG_EQUAL(IntHash.find(19), IntHash.end()) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(iter = IntHash.begin()) ;
   CPPUNIT_LOG_IS_FALSE(IntHash.insert(20).second) ;
   CPPUNIT_LOG_EQUAL(IntHash.insert(20).first, iter) ;
   CPPUNIT_LOG_EQUAL(IntHash.size(), (size_t)1) ;
}

void ClosedHashTests::Test_Closed_Hash_Insert()
{
   test_inthashtable IntHash(4) ;
   CPPUNIT_LOG_EQ(IntHash.max_load_factor(), 0.75) ;
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
   CPPUNIT_LOG_EXPRESSION(IntHash) ;
   CPPUNIT_LOG_ASSERT(IntHash.insert(26).second) ;
   CPPUNIT_LOG_EXPRESSION(IntHash) ;
   CPPUNIT_LOG_ASSERT(IntHash.insert(28).second) ;
   CPPUNIT_LOG_EXPRESSION(IntHash) ;
   CPPUNIT_LOG_EXPRESSION(IntHash) ;

   CPPUNIT_LOG_EQUAL(*IntHash.find(4), 4L) ;
   CPPUNIT_LOG_EQUAL(*IntHash.find(11), 11L) ;
   CPPUNIT_LOG_EQUAL(*IntHash.find(10), 10L) ;
   CPPUNIT_LOG_EQUAL(*IntHash.find(5), 5L) ;
   CPPUNIT_LOG_EQUAL(*IntHash.find(26), 26L) ;
   CPPUNIT_LOG_EQUAL(*IntHash.find(28), 28L) ;
   CPPUNIT_LOG_EQUAL(IntHash.find(55), IntHash.end()) ;
}

void ClosedHashTests::Test_Closed_Hash_Erase()
{
   test_inthashtable IntHash ;
   CPPUNIT_LOG_EQUAL(IntHash.erase(20), (size_t)0) ;
   CPPUNIT_LOG_EXPRESSION(IntHash) ;
   CPPUNIT_LOG_EQUAL(IntHash.size(), (size_t)0) ;
   long Value = 30 ;
   CPPUNIT_LOG_EQUAL(IntHash.erase(20, Value), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(Value, 30L) ;

   CPPUNIT_LOG_ASSERT(IntHash.insert(20).second) ;
   CPPUNIT_LOG_EQUAL(IntHash.size(), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(IntHash.erase(20), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(IntHash.size(), (size_t)0) ;
   CPPUNIT_LOG_EXPRESSION(IntHash) ;
   CPPUNIT_LOG_EQUAL(IntHash.begin(), IntHash.end()) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG("Testing erasing in the presence of collisions" << std::endl) ;
   const int BucketCount = (int)IntHash.bucket_count() ;
   CPPUNIT_LOG("Bucket count = " << BucketCount << std::endl) ;

   CPPUNIT_LOG_ASSERT(IntHash.insert(BucketCount + 4).second) ;
   CPPUNIT_LOG_ASSERT(IntHash.insert(4).second) ;
   CPPUNIT_LOG_ASSERT(IntHash.insert(BucketCount + 5).second) ;
   CPPUNIT_LOG_ASSERT(IntHash.insert(5).second) ;
   CPPUNIT_LOG_EQUAL(IntHash.size(), (size_t)4) ;
   CPPUNIT_LOG_EXPRESSION(IntHash) ;
   CPPUNIT_LOG_EQUAL(IntHash.erase(BucketCount + 4), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(IntHash.size(), (size_t)3) ;
   CPPUNIT_LOG_EXPRESSION(IntHash) ;
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

void ClosedHashTests::Test_Static_Optimization_Insert()
{
   pcomn::closed_hashtable<const char *> CHash ;
   CPPUNIT_LOG(PCOMN_TYPENAME(CHash) << '=' << CHash
               << " sizeof(" << PCOMN_TYPENAME(CHash) << ")=" << sizeof CHash << std::endl) ;

   CPPUNIT_LOG_EQ(CHash.max_load_factor(), 0.75) ;
   CPPUNIT_LOG_EQUAL(CHash.bucket_count(), (size_t)4) ;

   CPPUNIT_LOG_ASSERT(CHash.insert(Foo).second) ;
   CPPUNIT_LOG(CHash << std::endl) ;
   CPPUNIT_LOG_EQUAL(CHash.size(), (size_t)1) ;
   CPPUNIT_LOG_ASSERT(CHash.insert(World).second) ;
   CPPUNIT_LOG(CHash << std::endl) ;
   CPPUNIT_LOG_EQUAL(CHash.size(), (size_t)2) ;
   CPPUNIT_LOG_ASSERT(CHash.insert(Bar).second) ;
   CPPUNIT_LOG(CHash << std::endl) ;
   CPPUNIT_LOG_EQUAL(CHash.size(), (size_t)3) ;
   CPPUNIT_LOG_IS_FALSE(CHash.insert(World).second) ;
   CPPUNIT_LOG(CHash << std::endl) ;
   CPPUNIT_LOG_EQUAL(CHash.size(), (size_t)3) ;
   CPPUNIT_LOG_ASSERT(CHash.insert(Hello).second) ;
   CPPUNIT_LOG(CHash << std::endl) ;
   CPPUNIT_LOG_EQUAL(CHash.size(), (size_t)4) ;
   CPPUNIT_LOG_EQ(CHash.load_factor(), 1.0) ;

   CPPUNIT_LOG_EQUAL(CHash.count(Foo), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(CHash.count(Bar), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(CHash.count(Hello), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(CHash.count(Quux), (size_t)0) ;

   CPPUNIT_LOG_EQUAL(*CHash.find(Hello), Hello) ;
   CPPUNIT_LOG_EQUAL(*CHash.find(Foo), Foo) ;
   CPPUNIT_LOG_EQUAL(*CHash.find(World), World) ;
   CPPUNIT_LOG_EQUAL(*CHash.find(Bar), Bar) ;
   CPPUNIT_LOG_EQUAL(CHash.find(Quux), CHash.end()) ;
}

void ClosedHashTests::Test_Static_Optimization_Erase()
{
   pcomn::closed_hashtable<const char *> CHash ;
   CPPUNIT_LOG_EQ(CHash.max_load_factor(), 0.75) ;
   CPPUNIT_LOG_EQUAL(CHash.bucket_count(), (size_t)4) ;

   CPPUNIT_LOG_ASSERT(CHash.insert(Foo).second) ;
   CPPUNIT_LOG(CHash << std::endl) ;
   CPPUNIT_LOG_EQUAL(CHash.size(), (size_t)1) ;
   CPPUNIT_LOG_ASSERT(CHash.insert(World).second) ;
   CPPUNIT_LOG(CHash << std::endl) ;
   CPPUNIT_LOG_EQUAL(CHash.size(), (size_t)2) ;
   CPPUNIT_LOG_ASSERT(CHash.insert(Bar).second) ;
   CPPUNIT_LOG(CHash << std::endl) ;
   CPPUNIT_LOG_EQUAL(CHash.size(), (size_t)3) ;
   const char *Erased = NULL ;
   CPPUNIT_LOG_EQUAL(CHash.erase(World, Erased), (size_t)1) ;
   CPPUNIT_LOG_ASSERT(Erased) ;
   CPPUNIT_LOG_EQUAL(Erased, World) ;
   CPPUNIT_LOG_IS_FALSE(CHash.insert(Bar).second) ;
   CPPUNIT_LOG_ASSERT(CHash.insert(Hello).second) ;
   CPPUNIT_LOG_ASSERT(CHash.insert(World).second) ;
   CPPUNIT_LOG(CHash << std::endl) ;

   CPPUNIT_LOG_EQUAL(CHash.erase(Foo), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(CHash.erase(Bar), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(CHash.erase(World), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(CHash.erase(Hello), (size_t)1) ;
   CPPUNIT_LOG(CHash << std::endl) ;

   CPPUNIT_LOG_EQUAL(CHash.size(), (size_t)0) ;
   CPPUNIT_LOG_ASSERT(CHash.insert(Hello).second) ;
   CPPUNIT_LOG(CHash << std::endl) ;
   CPPUNIT_LOG_EQUAL(CHash.size(), (size_t)1) ;
}

void ClosedHashTests::Test_Static_Optimization_Grow()
{
   pcomn::closed_hashtable<const char *> CHash ;
   CPPUNIT_LOG_EQ(CHash.max_load_factor(), 0.75) ;
   CPPUNIT_LOG_EQUAL(CHash.bucket_count(), (size_t)4) ;

   CPPUNIT_LOG_ASSERT(CHash.insert(Foo).second) ;
   CPPUNIT_LOG(CHash << std::endl) ;
   CPPUNIT_LOG_EQUAL(CHash.size(), (size_t)1) ;
   CPPUNIT_LOG_ASSERT(CHash.insert(World).second) ;
   CPPUNIT_LOG(CHash << std::endl) ;
   CPPUNIT_LOG_EQUAL(CHash.size(), (size_t)2) ;
   CPPUNIT_LOG_ASSERT(CHash.insert(Bar).second) ;
   CPPUNIT_LOG(CHash << std::endl) ;
   CPPUNIT_LOG_EQUAL(CHash.size(), (size_t)3) ;
   CPPUNIT_LOG_IS_FALSE(CHash.insert(World).second) ;
   CPPUNIT_LOG(CHash << std::endl) ;
   CPPUNIT_LOG_EQUAL(CHash.size(), (size_t)3) ;
   CPPUNIT_LOG_ASSERT(CHash.insert(Hello).second) ;
   CPPUNIT_LOG(CHash << std::endl) ;
   CPPUNIT_LOG_EQUAL(CHash.size(), (size_t)4) ;
   CPPUNIT_LOG_EQ(CHash.load_factor(), 1.0) ;
   CPPUNIT_LOG_ASSERT(CHash.insert(Quux).second) ;
   CPPUNIT_LOG(CHash << std::endl) ;
   CPPUNIT_LOG_EQUAL(CHash.size(), (size_t)5) ;
   CPPUNIT_LOG_ASSERT(CHash.load_factor() < 1.0) ;
   CPPUNIT_LOG_EQUAL(*CHash.find(Hello), Hello) ;

   CPPUNIT_LOG_EQUAL(*CHash.find(Foo), Foo) ;
   CPPUNIT_LOG_EQUAL(*CHash.find(World), World) ;
   CPPUNIT_LOG_EQUAL(*CHash.find(Bar), Bar) ;
   CPPUNIT_LOG_EQUAL(*CHash.find(Quux), Quux) ;
   CPPUNIT_LOG_EQUAL(*CHash.find(Hello), Hello) ;
   CPPUNIT_LOG_EQUAL(CHash.find(Xyzzy), CHash.end()) ;
   CPPUNIT_LOG_ASSERT(CHash.insert(Xyzzy).second) ;
   CPPUNIT_LOG_ASSERT(CHash.insert(Bye).second) ;
   CPPUNIT_LOG_ASSERT(CHash.insert(Baby).second) ;
   CPPUNIT_LOG(CHash << std::endl) ;
   CPPUNIT_LOG_ASSERT(CHash.load_factor() < CHash.max_load_factor()) ;
}

void ClosedHashTests::Test_Closed_Hash_Copy()
{
   pcomn::closed_hashtable<const char *> CHash ;
   CPPUNIT_LOG_EQ(CHash.max_load_factor(), 0.75) ;
   CPPUNIT_LOG_EQUAL(CHash.bucket_count(), (size_t)4) ;

   CPPUNIT_LOG_ASSERT(CHash.insert(Foo).second) ;
   CPPUNIT_LOG_ASSERT(CHash.insert(World).second) ;
   CPPUNIT_LOG_ASSERT(CHash.insert(Bar).second) ;

   pcomn::closed_hashtable<const char *> CHashCopy(CHash) ;
   CPPUNIT_LOG_EQUAL(CHash.size(), (size_t)3) ;
   CPPUNIT_LOG_EQUAL(CHashCopy.size(), (size_t)3) ;
   CPPUNIT_LOG_EQUAL(CHash.bucket_count(), (size_t)4) ;
   CPPUNIT_LOG_EQUAL(CHashCopy.bucket_count(), (size_t)4) ;
   CPPUNIT_LOG(CHash << std::endl << CHashCopy << std::endl) ;

   CPPUNIT_LOG_EQUAL(*CHash.find(Foo), Foo) ;
   CPPUNIT_LOG_EQUAL(*CHash.find(World), World) ;
   CPPUNIT_LOG_EQUAL(*CHash.find(Bar), Bar) ;
   CPPUNIT_LOG_EQUAL(CHash.find(Hello), CHash.end()) ;

   CPPUNIT_LOG_EQUAL(*CHashCopy.find(Foo), Foo) ;
   CPPUNIT_LOG_EQUAL(*CHashCopy.find(World), World) ;
   CPPUNIT_LOG_EQUAL(*CHashCopy.find(Bar), Bar) ;
   CPPUNIT_LOG_EQUAL(CHashCopy.find(Hello), CHashCopy.end()) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(CHash.insert(Bye).second) ;
   CPPUNIT_LOG_ASSERT(CHash.insert(Baby).second) ;
   CPPUNIT_LOG_EQUAL(CHash.size(), (size_t)5) ;
   CPPUNIT_LOG_EQUAL(CHash.bucket_count(), (size_t)8) ;

   pcomn::closed_hashtable<const char *> CHashDynCopy(CHash) ;
   CPPUNIT_LOG_EQUAL(*CHashDynCopy.find(Foo), Foo) ;
   CPPUNIT_LOG_EQUAL(*CHashDynCopy.find(World), World) ;
   CPPUNIT_LOG_EQUAL(*CHashDynCopy.find(Bar), Bar) ;
   CPPUNIT_LOG_EQUAL(CHashDynCopy.find(Hello), CHashDynCopy.end()) ;
   CPPUNIT_LOG_EQUAL(*CHashDynCopy.find(Bye), Bye) ;
   CPPUNIT_LOG_EQUAL(*CHashDynCopy.find(Baby), Baby) ;
   CPPUNIT_LOG_EQUAL(CHashDynCopy.size(), (size_t)5) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(CHashDynCopy.erase(Bye)) ;
   CPPUNIT_LOG_EQUAL(CHashDynCopy.size(), (size_t)4) ;

   pcomn::closed_hashtable<const char *> CHashStatCopy(CHashDynCopy) ;
   CPPUNIT_LOG_EQUAL(CHashStatCopy.size(), (size_t)4) ;
   CPPUNIT_LOG_EQUAL(CHashStatCopy.bucket_count(), (size_t)4) ;
   CPPUNIT_LOG_EQUAL(*CHashStatCopy.find(Foo), Foo) ;
   CPPUNIT_LOG_EQUAL(*CHashStatCopy.find(World), World) ;
   CPPUNIT_LOG_EQUAL(*CHashStatCopy.find(Bar), Bar) ;
   CPPUNIT_LOG_EQUAL(CHashStatCopy.find(Hello), CHashStatCopy.end()) ;
   CPPUNIT_LOG_EQUAL(*CHashStatCopy.find(Baby), Baby) ;
}

void ClosedHashTests::Test_Closed_Hash_Move()
{
   pcomn::closed_hashtable<const char *> CHash ;
   CPPUNIT_LOG_EQ(CHash.max_load_factor(), 0.75) ;
   CPPUNIT_LOG_EQUAL(CHash.bucket_count(), (size_t)4) ;

   CPPUNIT_LOG_ASSERT(CHash.insert(Foo).second) ;
   CPPUNIT_LOG_ASSERT(CHash.insert(World).second) ;
   CPPUNIT_LOG_ASSERT(CHash.insert(Bar).second) ;
   CPPUNIT_LOG_EQUAL(CHash.size(), (size_t)3) ;

   pcomn::closed_hashtable<const char *> CHashCopy(std::move(CHash)) ;
   CPPUNIT_LOG_EQ(CHashCopy.size(), 3) ;
   CPPUNIT_LOG_EQ(CHash.size(), 0) ;
   CPPUNIT_LOG_EQUAL(CHash.bucket_count(), (size_t)4) ;
   CPPUNIT_LOG_EQUAL(CHashCopy.bucket_count(), (size_t)4) ;
   CPPUNIT_LOG(CHash << std::endl << CHashCopy << std::endl) ;

   CPPUNIT_LOG_EQUAL(*CHashCopy.find(Foo), Foo) ;
   CPPUNIT_LOG_EQUAL(*CHashCopy.find(World), World) ;
   CPPUNIT_LOG_EQUAL(*CHashCopy.find(Bar), Bar) ;
   CPPUNIT_LOG_EQUAL(CHashCopy.find(Hello), CHashCopy.end()) ;

   CPPUNIT_LOG_EQUAL(CHash.find(Foo), CHash.end()) ;
   CPPUNIT_LOG_EQUAL(CHash.find(World), CHash.end()) ;
   CPPUNIT_LOG_EQUAL(CHash.find(Bar), CHash.end()) ;
   CPPUNIT_LOG_EQUAL(CHash.find(Hello), CHash.end()) ;


   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(CHashCopy.insert(Bye).second) ;
   CPPUNIT_LOG_ASSERT(CHashCopy.insert(Baby).second) ;
   CPPUNIT_LOG_EQUAL(CHashCopy.size(), (size_t)5) ;
   CPPUNIT_LOG_EQUAL(CHashCopy.bucket_count(), (size_t)8) ;

   pcomn::closed_hashtable<const char *> CHashDynCopy(std::move(CHashCopy)) ;
   CPPUNIT_LOG_EQUAL(*CHashDynCopy.find(Foo), Foo) ;
   CPPUNIT_LOG_EQUAL(*CHashDynCopy.find(World), World) ;
   CPPUNIT_LOG_EQUAL(*CHashDynCopy.find(Bar), Bar) ;
   CPPUNIT_LOG_EQUAL(CHashDynCopy.find(Hello), CHashDynCopy.end()) ;
   CPPUNIT_LOG_EQUAL(*CHashDynCopy.find(Bye), Bye) ;
   CPPUNIT_LOG_EQUAL(*CHashDynCopy.find(Baby), Baby) ;
   CPPUNIT_LOG_EQUAL(CHashDynCopy.size(), (size_t)5) ;

   CPPUNIT_LOG_EQ(CHashCopy.size(), 0) ;
   CPPUNIT_LOG_EQ(CHashCopy.bucket_count(), 4) ;
   CPPUNIT_LOG(CHashCopy << std::endl << CHashDynCopy << std::endl) ;

   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_ASSERT(CHashDynCopy.erase(Bye)) ;
   CPPUNIT_LOG_EQ(CHashDynCopy.size(), 4) ;

   pcomn::closed_hashtable<const char *> CHashAssign ;
   CPPUNIT_LOG_EQ(CHashAssign.size(), 0) ;
   CPPUNIT_LOG_EQ(CHashAssign.bucket_count(), 4) ;
   CPPUNIT_LOG_ASSERT(CHashAssign.insert(Xyzzy).second) ;
   CPPUNIT_LOG_EQ(CHashAssign.size(), 1) ;
   CPPUNIT_LOG_EQ(CHashAssign.bucket_count(), 4) ;

   CPPUNIT_LOG(CHashAssign << std::endl << CHashDynCopy << std::endl) ;

   CHashAssign = std::move(CHashDynCopy) ;

   CPPUNIT_LOG_EQUAL(*CHashAssign.find(Foo), Foo) ;
   CPPUNIT_LOG_EQUAL(*CHashAssign.find(World), World) ;
   CPPUNIT_LOG_EQUAL(*CHashAssign.find(Bar), Bar) ;

   CPPUNIT_LOG_EQUAL(CHashAssign.find(Hello), CHashAssign.end()) ;
   CPPUNIT_LOG_EQUAL(CHashAssign.find(Bye), CHashAssign.end()) ;

   CPPUNIT_LOG_EQUAL(*CHashAssign.find(Baby), Baby) ;
   CPPUNIT_LOG_EQ(CHashAssign.size(), 4) ;
   CPPUNIT_LOG_EQ(CHashAssign.bucket_count(), 8) ;

   CPPUNIT_LOG_ASSERT(CHashDynCopy.insert(Xyzzy).second) ;
   CPPUNIT_LOG_EQ(CHashDynCopy.size(), 1) ;
   CPPUNIT_LOG_EQ(CHashDynCopy.bucket_count(), 4) ;
}

struct KeyedHashval {
      uint64_t key ;
      char     str[64] ;
} ;

struct extract_key : std::unary_function<KeyedHashval, uint64_t> {
      uint64_t operator()(const KeyedHashval &val) const { return val.key ; }
} ;

void ClosedHashTests::Test_Closed_Hash_Extract_Key()
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
   runner.addTest(ClosedHashTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv, "unittest.diag.ini", "Closed hashtable tests") ;
}
