/*-*- tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_cacher.cpp
 COPYRIGHT    :   Yakov Markovitch, 2009-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for pcomn::cacher.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   8 Oct 2009
*******************************************************************************/
#include <pcomn_cacher.h>
#include <pcomn_string.h>
#include <pcomn_pool.h>
#include <pcomn_unittest.h>
#include <pcomn_function.h>
#include <pcomn_smartptr.h>

#include <vector>
#include <iterator>
#include <type_traits>

#include <sys/types.h>

using namespace pcomn ;

/*******************************************************************************
                            struct CacherItem
*******************************************************************************/
struct CacherItem : public PRefCount, std::pair<std::string, int> {
      CacherItem() {}
      CacherItem(const std::string &key, int value) :
         std::pair<std::string, int>(key, value)
      {}

      ~CacherItem()
      {
         std::cerr << "~CacherItem" << *this << std::endl ;
         destroyed.push_back(*this) ;
      }

      static std::vector<std::pair<std::string, int> > destroyed ;
} ;

std::vector<std::pair<std::string, int> > CacherItem::destroyed ;

typedef shared_intrusive_ptr<CacherItem> citem_ptr ;

namespace CppUnit {
template<>
struct assertion_traits<CacherItem> : assertion_traits<std::pair<std::string, int> > {} ;

} // end of namespace CppUnit

/*******************************************************************************
                            CacherTests
*******************************************************************************/
class CacherTests : public CppUnit::TestFixture  {

      void Test_Cacher_Basic() ;
      void Test_Cacher_LRU() ;

      CPPUNIT_TEST_SUITE(CacherTests) ;

      CPPUNIT_TEST(Test_Cacher_Basic) ;
      CPPUNIT_TEST(Test_Cacher_LRU) ;

      CPPUNIT_TEST_SUITE_END() ;

   public:
      void setUp()
      {
         CacherItem::destroyed.clear() ;
      }
} ;

template<typename V, typename X, typename H, typename C>
static std::vector<typename std::remove_cv<typename std::remove_reference<typename cacher<V, X, H, C>::key_type>::type>::type>
cacher_keys(const cacher<V, X, H, C> &c)
{
   decltype(cacher_keys(c)) result ;
   c.keys(std::back_inserter(result)) ;
   return result ;
}

/*******************************************************************************
 CacherTests
*******************************************************************************/
struct item_name : std::unary_function<citem_ptr, std::string> {
      const std::string &operator()(const citem_ptr &item) const
      {
         CPPUNIT_IS_TRUE(item) ;
         return item->first ;
      }
} ;

typedef cacher<citem_ptr, item_name> test_cacher ;

void CacherTests::Test_Cacher_Basic()
{
   test_cacher Cacher ;

   CPPUNIT_LOG_EQUAL(Cacher.size(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(cacher_keys(Cacher), std::vector<std::string>()) ;
   CPPUNIT_LOG_IS_FALSE(Cacher.erase("FooBar")) ;
   CPPUNIT_LOG_EQUAL(Cacher.size_limit(), (size_t)-1) ;
   CPPUNIT_LOG_IS_FALSE(Cacher.exists("FooBar")) ;

   citem_ptr Item ;
   citem_ptr NewItem ;
   CPPUNIT_LOG_IS_FALSE(Cacher.get("FooBar", Item, false)) ;
   CPPUNIT_LOG_IS_FALSE(Item) ;
   CPPUNIT_LOG_EQUAL(Cacher.size(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(CacherItem::destroyed.size(), (size_t)0) ;
   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_ASSERT(Cacher.put(citem_ptr(new CacherItem("FooBar", 13)))) ;
   CPPUNIT_LOG_EQUAL(CacherItem::destroyed.size(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(Cacher.size(), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(cacher_keys(Cacher), CPPUNIT_STRVECTOR(("FooBar"))) ;

   Item.reset() ;
   CPPUNIT_LOG_RUN(NewItem = new CacherItem("Quux", 13)) ;
   CPPUNIT_LOG_ASSERT(Cacher.put(NewItem, Item)) ;
   CPPUNIT_LOG_ASSERT(NewItem) ;
   CPPUNIT_LOG_EQUAL(NewItem, Item) ;
   CPPUNIT_LOG_EQUAL(CacherItem::destroyed.size(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(Cacher.size(), (size_t)2) ;
   CPPUNIT_LOG_EQUAL(cacher_keys(Cacher), CPPUNIT_STRVECTOR(("Quux")("FooBar"))) ;

   CPPUNIT_LOG(std::endl) ;
   test_cacher EmptyCacher ((size_t)1) ;
   CPPUNIT_LOG_EQUAL(EmptyCacher.size_limit(), (size_t)1) ;
   Item.reset() ; NewItem.reset() ;
   CPPUNIT_LOG_RUN(Item = new CacherItem("FooBar", 14)) ;
   CPPUNIT_LOG_RUN(NewItem = new CacherItem("Quux", 13)) ;
   CPPUNIT_LOG_EQUAL(cacher_keys(EmptyCacher), std::vector<std::string>()) ;

   CPPUNIT_LOG_ASSERT(EmptyCacher.put(Item)) ;
   CPPUNIT_LOG_EQUAL(CacherItem::destroyed.size(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(EmptyCacher.size(), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(cacher_keys(EmptyCacher), CPPUNIT_STRVECTOR(("FooBar"))) ;

   CPPUNIT_LOG_RUN(Item.reset()) ;
   CPPUNIT_LOG_EQUAL(CacherItem::destroyed.size(), (size_t)0) ;
   CPPUNIT_LOG_ASSERT(EmptyCacher.put(NewItem)) ;
   CPPUNIT_LOG_EQUAL(CacherItem::destroyed.size(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(EmptyCacher.size(), (size_t)2) ;
   CPPUNIT_LOG_EQUAL(CPPUNIT_SORTED(cacher_keys(EmptyCacher)), CPPUNIT_STRVECTOR(("FooBar")("Quux"))) ;

   CPPUNIT_LOG_IS_FALSE(EmptyCacher.put(NewItem)) ;
   CPPUNIT_LOG_EQUAL(CacherItem::destroyed.size(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(EmptyCacher.size(), (size_t)2) ;
   CPPUNIT_LOG_EQUAL(CPPUNIT_SORTED(cacher_keys(EmptyCacher)), CPPUNIT_STRVECTOR(("FooBar")("Quux"))) ;

   CPPUNIT_LOG_RUN(NewItem = new CacherItem("Xyzzy", 15)) ;
   CPPUNIT_LOG_EQUAL(CacherItem::destroyed.size(), (size_t)0) ;
   CPPUNIT_LOG_ASSERT(EmptyCacher.put(NewItem)) ;
   CPPUNIT_LOG_EQUAL(CacherItem::destroyed.size(), (size_t)2) ;
   CPPUNIT_LOG_EQUAL(EmptyCacher.size(), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(cacher_keys(EmptyCacher), CPPUNIT_STRVECTOR(("Xyzzy"))) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(NewItem.reset()) ;
   CPPUNIT_LOG_EQUAL(CacherItem::destroyed.size(), (size_t)2) ;
   CPPUNIT_LOG_EQUAL(EmptyCacher.set_size_limit(0), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(EmptyCacher.size_limit(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(EmptyCacher.size(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(CacherItem::destroyed.size(), (size_t)3) ;
   CPPUNIT_LOG_EQUAL(cacher_keys(EmptyCacher), CPPUNIT_STRVECTOR()) ;

   CPPUNIT_LOG_ASSERT(EmptyCacher.put(citem_ptr(new CacherItem("Xyzzy", 15)))) ;
   CPPUNIT_LOG_EQUAL(EmptyCacher.size(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(CacherItem::destroyed.size(), (size_t)4) ;
   CPPUNIT_LOG_EQUAL(cacher_keys(EmptyCacher), CPPUNIT_STRVECTOR()) ;
}

void CacherTests::Test_Cacher_LRU()
{
   test_cacher Cacher ;
}

/*******************************************************************************
                            KeyedPoolTests
*******************************************************************************/
class KeyedPoolTests : public CppUnit::TestFixture  {

      void Test_Keyed_Pool_Defaults() ;
      void Test_Keyed_Pool_Basic() ;
      void Test_Keyed_Pool_Erase() ;
      void Test_Keyed_Pool_Lru() ;

      CPPUNIT_TEST_SUITE(KeyedPoolTests) ;

      CPPUNIT_TEST(Test_Keyed_Pool_Defaults) ;
      CPPUNIT_TEST(Test_Keyed_Pool_Basic) ;
      CPPUNIT_TEST(Test_Keyed_Pool_Erase) ;
      CPPUNIT_TEST(Test_Keyed_Pool_Lru) ;

      CPPUNIT_TEST_SUITE_END() ;

   public:
      typedef std::pair<std::string, int> value_pair ;
      struct PoolItem : value_pair {
            PoolItem(const std::string &key, int value) :
               value_pair(key, value)
            {}

            ~PoolItem()
            {
               if (!first.empty() || second)
               {
                  std::cerr << "\n~PoolItem" << *this << std::endl ;
                  destroyed.push_back(*this) ;
               }
            }

            const value_pair &value() const { return *this ; }

            void swap(PoolItem &other)
            {
               std::swap(*static_cast<value_pair *>(this), *static_cast<value_pair *>(&other)) ;
            }

            friend void swap(PoolItem &left, PoolItem &right) { left.swap(right) ; }

            static std::vector<value_pair> destroyed ;
      } ;

      struct PoolItemNoCopy : value_pair {
            PoolItemNoCopy() {}
            PoolItemNoCopy(const std::string &key, int value) :
               value_pair(key, value)
            {}

            ~PoolItemNoCopy()
            {
               if (!first.empty() || second)
               {
                  std::cerr << "\n~PoolItemNoCopy" << *this << std::endl ;
                  destroyed.push_back(*this) ;
               }
            }

            const value_pair &value() const { return *this ; }

            void swap(PoolItemNoCopy &other)
            {
               std::swap(*static_cast<value_pair *>(this), *static_cast<value_pair *>(&other)) ;
            }

            friend void swap(PoolItemNoCopy &left, PoolItemNoCopy &right) { left.swap(right) ; }

            void clear() { *static_cast<value_pair *>(this) = value_pair() ; }

            static std::vector<value_pair> destroyed ;
         private:
            PCOMN_NONCOPYABLE(PoolItemNoCopy) ;
      } ;

      typedef keyed_pool<std::string, PoolItem> test_pool ;
      typedef keyed_pool<std::string, PoolItemNoCopy> test_pool_nocopy ;
      typedef std::pair<std::string, size_t> key_itemcount ;

      void setUp() { CleanupRegistry() ; }

      void CleanupRegistry()
      {
         swap_clear(PoolItem::destroyed) ;
         swap_clear(PoolItemNoCopy::destroyed) ;
      }
} ;

/*******************************************************************************
 KeyedPoolTests
*******************************************************************************/
std::vector<KeyedPoolTests::value_pair> KeyedPoolTests::PoolItem::destroyed ;
std::vector<KeyedPoolTests::value_pair> KeyedPoolTests::PoolItemNoCopy::destroyed ;

void KeyedPoolTests::Test_Keyed_Pool_Defaults()
{
   test_pool pool (0) ;
   CPPUNIT_LOG_EQUAL(pool.size(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(pool.size_limit(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(pool.key_count(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(pool.erase(""), (size_t)0) ;

   PoolItem dummy ("dummy", 13) ;
   CPPUNIT_LOG_IS_FALSE(pool.checkout("", dummy)) ;
   CPPUNIT_LOG_EQUAL(dummy.value(), value_pair("dummy", 13)) ;

   CPPUNIT_LOG_EQUAL(dummy.value(), value_pair("dummy", 13)) ;
   CPPUNIT_LOG_EQUAL(PoolItem::destroyed, std::vector<value_pair>()) ;
}

void KeyedPoolTests::Test_Keyed_Pool_Basic()
{
   PoolItem dummy ("dummy", 13) ;
   {
      test_pool TestPool (0) ;

      CPPUNIT_LOG_IS_FALSE(TestPool.checkout("", dummy)) ;
      CPPUNIT_LOG_EQUAL(dummy.value(), value_pair("dummy", 13)) ;
      CPPUNIT_LOG_RUN(TestPool.put("dummy1", dummy)) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)0) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)0) ;
      CPPUNIT_LOG_EQUAL(TestPool.erase("dummy1"), (size_t)0) ;
   }
   CPPUNIT_LOG_EQUAL(PoolItem::destroyed, std::vector<value_pair>()) ;
   CleanupRegistry() ;

   CPPUNIT_LOG(std::endl) ;
   PoolItemNoCopy dummy_nocopy ("dummy_nocopy", 13) ;
   const PoolItemNoCopy dummy_nocopy_orig ("dummy_nocopy", 13) ;
   {
      test_pool_nocopy TestPool (0) ;

      CPPUNIT_LOG_RUN(TestPool.checkin("dummy1", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)0) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)0) ;
      CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, CPPUNIT_CONTAINER(std::vector<value_pair>,
                                                                     (value_pair("dummy_nocopy", 13)))) ;
   }
   CleanupRegistry() ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, std::vector<value_pair>()) ;
   CPPUNIT_LOG_RUN(PoolItemNoCopy("dummy_nocopy", 13).swap(dummy_nocopy)) ;
   {
      test_pool_nocopy TestPool (1) ;

      CPPUNIT_LOG_RUN(TestPool.checkin("dummy1", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)1) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)1) ;
      CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, std::vector<value_pair>()) ;
      CPPUNIT_LOG_EQUAL(dummy_nocopy, PoolItemNoCopy()) ;
      CPPUNIT_LOG_IS_FALSE(TestPool.checkout("d", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(dummy_nocopy, PoolItemNoCopy()) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)1) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)1) ;

      CPPUNIT_LOG_ASSERT(TestPool.checkout("dummy1", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)0) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)0) ;
      CPPUNIT_LOG_EQUAL(dummy_nocopy, dummy_nocopy_orig) ;
      CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, std::vector<value_pair>()) ;
   }
   CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, std::vector<value_pair>()) ;
   CleanupRegistry() ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, std::vector<value_pair>()) ;
   CPPUNIT_EQUAL(dummy_nocopy, dummy_nocopy_orig) ;
   {
      test_pool_nocopy TestPool (1) ;

      CPPUNIT_LOG_RUN(TestPool.checkin("dummy1", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)1) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)1) ;
      CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, std::vector<value_pair>()) ;
      CPPUNIT_LOG_EQUAL(dummy_nocopy, PoolItemNoCopy()) ;
   }
   CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, CPPUNIT_CONTAINER(std::vector<value_pair>,
                                                                  (value_pair("dummy_nocopy", 13)))) ;
   CleanupRegistry() ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, std::vector<value_pair>()) ;
   {
      test_pool_nocopy TestPool (2) ;

      CPPUNIT_LOG_RUN(PoolItemNoCopy("dummy2_nocopy", 7).swap(dummy_nocopy)) ;
      CPPUNIT_LOG_RUN(TestPool.checkin("dummy1", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)1) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)1) ;
      CPPUNIT_LOG_RUN(PoolItemNoCopy("dummy1_nocopy", 13).swap(dummy_nocopy)) ;
      CPPUNIT_LOG_RUN(TestPool.checkin("dummy1", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)2) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)1) ;

      CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, std::vector<value_pair>()) ;
      CPPUNIT_LOG_EQUAL(dummy_nocopy, PoolItemNoCopy()) ;
   }
   CPPUNIT_LOG_EQUAL(CPPUNIT_SORTED(PoolItemNoCopy::destroyed), CPPUNIT_CONTAINER(std::vector<value_pair>,
                                                                                  (value_pair("dummy1_nocopy", 13))
                                                                                  (value_pair("dummy2_nocopy", 7)))) ;
}

void KeyedPoolTests::Test_Keyed_Pool_Erase()
{
   PoolItem dummy ("dummy", 13) ;
   PoolItemNoCopy dummy_nocopy ("dummy_nocopy", 13) ;

   const PoolItemNoCopy dummy_nocopy_orig ("dummy_nocopy", 13) ;
   const PoolItemNoCopy bar_nocopy ("bar_nocopy", 13) ;
   const PoolItemNoCopy foo_nocopy ("foo_nocopy", 7) ;

   CPPUNIT_LOG("\n**** Eviction test, size 1 ****" << std::endl) ;
   CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, std::vector<value_pair>()) ;
   CPPUNIT_EQUAL(dummy_nocopy, dummy_nocopy_orig) ;
   {
      test_pool_nocopy TestPool (1) ;

      CPPUNIT_LOG_RUN(TestPool.checkin("dummy1", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(dummy_nocopy, PoolItemNoCopy()) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)1) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)1) ;
      CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, std::vector<value_pair>()) ;

      CPPUNIT_LOG_RUN(PoolItemNoCopy("foo_nocopy", 7).swap(dummy_nocopy)) ;
      CPPUNIT_LOG_RUN(TestPool.checkin("dummy1", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(dummy_nocopy, PoolItemNoCopy()) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)1) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)1) ;
      CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, CPPUNIT_CONTAINER(std::vector<value_pair>,
                                                                     (value_pair("dummy_nocopy", 13)))) ;

      CPPUNIT_LOG_RUN(PoolItemNoCopy("bar_nocopy", 13).swap(dummy_nocopy)) ;
      CPPUNIT_LOG_RUN(TestPool.checkin("dummy2", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(dummy_nocopy, PoolItemNoCopy()) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)1) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)1) ;
      CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, CPPUNIT_CONTAINER(std::vector<value_pair>,
                                                                     (value_pair("dummy_nocopy", 13))
                                                                     (value_pair("foo_nocopy", 7)))) ;

   }
   CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, CPPUNIT_CONTAINER(std::vector<value_pair>,
                                                                  (value_pair("dummy_nocopy", 13))
                                                                  (value_pair("foo_nocopy", 7))
                                                                  (value_pair("bar_nocopy", 13)))) ;
   CleanupRegistry() ;

   CPPUNIT_LOG("\n**** Erase test, 1 item ****" << std::endl) ;
   CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, std::vector<value_pair>()) ;
   CPPUNIT_LOG_RUN(PoolItemNoCopy("dummy_nocopy", 13).swap(dummy_nocopy)) ;
   {
      test_pool_nocopy TestPool (1) ;

      CPPUNIT_LOG_RUN(TestPool.checkin("dummy1", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(dummy_nocopy, PoolItemNoCopy()) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)1) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)1) ;
      CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, std::vector<value_pair>()) ;
      CPPUNIT_LOG_RUN(TestPool.erase("d")) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)1) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)1) ;
      CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, std::vector<value_pair>()) ;
      CPPUNIT_LOG_RUN(TestPool.erase("dummy1")) ;
      CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, CPPUNIT_CONTAINER(std::vector<value_pair>,
                                                                     (value_pair("dummy_nocopy", 13)))) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)0) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)0) ;
   }
   CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, CPPUNIT_CONTAINER(std::vector<value_pair>,
                                                                  (value_pair("dummy_nocopy", 13)))) ;
   CleanupRegistry() ;

   CPPUNIT_LOG("\n**** Erase test, multiple items ****" << std::endl) ;
   CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, std::vector<value_pair>()) ;
   CPPUNIT_LOG_RUN(PoolItemNoCopy("dummy_nocopy", 13).swap(dummy_nocopy)) ;
   {
      test_pool_nocopy TestPool (3) ;

      CPPUNIT_LOG_RUN(TestPool.checkin("dummy1", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)1) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)1) ;
      CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, std::vector<value_pair>()) ;
      CPPUNIT_LOG_RUN(PoolItemNoCopy("foo_nocopy", 7).swap(dummy_nocopy)) ;
      CPPUNIT_LOG_RUN(TestPool.checkin("dummy1", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)2) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)1) ;
      CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, std::vector<value_pair>()) ;

      CPPUNIT_LOG_RUN(PoolItemNoCopy("bar_nocopy", 13).swap(dummy_nocopy)) ;
      CPPUNIT_LOG_RUN(TestPool.checkin("dummy2", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)3) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)2) ;
      CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, std::vector<value_pair>()) ;

      CPPUNIT_LOG_EQUAL(TestPool.erase("dummy1"), (size_t)2) ;

      CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, CPPUNIT_CONTAINER(std::vector<value_pair>,
                                                                     (value_pair("foo_nocopy", 7))
                                                                     (value_pair("dummy_nocopy", 13)))) ;
   }
   CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, CPPUNIT_CONTAINER(std::vector<value_pair>,
                                                                  (value_pair("foo_nocopy", 7))
                                                                  (value_pair("dummy_nocopy", 13))
                                                                  (value_pair("bar_nocopy", 13)))) ;
   CleanupRegistry() ;

   CPPUNIT_LOG("\n**** Checkout with key removal test, multiple items ****" << std::endl) ;
   CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, std::vector<value_pair>()) ;
   CPPUNIT_LOG_RUN(PoolItemNoCopy("dummy_nocopy", 13).swap(dummy_nocopy)) ;
   {
      test_pool_nocopy TestPool (5) ;

      CPPUNIT_LOG_RUN(TestPool.checkin("dummy1", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)1) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)1) ;
      CPPUNIT_LOG_RUN(PoolItemNoCopy("foo_nocopy", 7).swap(dummy_nocopy)) ;
      CPPUNIT_LOG_RUN(TestPool.checkin("dummy1", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)2) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)1) ;

      CPPUNIT_LOG_RUN(PoolItemNoCopy("bar_nocopy", 13).swap(dummy_nocopy)) ;
      CPPUNIT_LOG_RUN(TestPool.checkin("dummy2", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)3) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)2) ;
      CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, std::vector<value_pair>()) ;

      CPPUNIT_LOG_ASSERT(TestPool.checkout("dummy1", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, std::vector<value_pair>()) ;
      // Last in - first out
      CPPUNIT_EQUAL(dummy_nocopy, foo_nocopy) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)2) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)2) ;

      CPPUNIT_LOG_ASSERT(TestPool.checkout("dummy1", dummy_nocopy)) ;
      CPPUNIT_EQUAL(dummy_nocopy, dummy_nocopy_orig) ;
      CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, CPPUNIT_CONTAINER(std::vector<value_pair>,
                                                                     (value_pair("foo_nocopy", 7)))) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)1) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)1) ;

      CPPUNIT_LOG_IS_FALSE(TestPool.checkout("dummy1", dummy_nocopy)) ;
      CPPUNIT_EQUAL(dummy_nocopy, dummy_nocopy_orig) ;

      CPPUNIT_LOG_ASSERT(TestPool.checkout("dummy2", dummy_nocopy)) ;
      CPPUNIT_EQUAL(dummy_nocopy, bar_nocopy) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)0) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)0) ;
      CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, CPPUNIT_CONTAINER(std::vector<value_pair>,
                                                                     (value_pair("foo_nocopy", 7))
                                                                     (value_pair("dummy_nocopy", 13)))) ;
   }
   CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, CPPUNIT_CONTAINER(std::vector<value_pair>,
                                                                  (value_pair("foo_nocopy", 7))
                                                                  (value_pair("dummy_nocopy", 13)))) ;
   CleanupRegistry() ;
}

void KeyedPoolTests::Test_Keyed_Pool_Lru()
{
   PoolItem dummy ("dummy", 13) ;
   PoolItemNoCopy dummy_nocopy ("dummy_nocopy", 13) ;

   const PoolItemNoCopy dummy_nocopy_orig ("dummy_nocopy", 13) ;
   const PoolItemNoCopy bar_nocopy ("bar_nocopy", 13) ;
   const PoolItemNoCopy foo_nocopy ("foo_nocopy", 7) ;
   const PoolItemNoCopy quux_nocopy ("quux_nocopy", 777) ;
   const PoolItemNoCopy xyzzy_nocopy ("xyzzy_nocopy", 123) ;

   CPPUNIT_LOG("\n**** Lru eviction test ****" << std::endl) ;
   CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, std::vector<value_pair>()) ;
   CPPUNIT_EQUAL(dummy_nocopy, dummy_nocopy_orig) ;
   {
      test_pool_nocopy TestPool (5) ;

      CPPUNIT_LOG_RUN(TestPool.checkin("dummy1", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)1) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)1) ;
      CPPUNIT_LOG_RUN(PoolItemNoCopy("foo_nocopy", 7).swap(dummy_nocopy)) ;
      CPPUNIT_LOG_RUN(TestPool.checkin("dummy1", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)2) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)1) ;
      CPPUNIT_LOG_RUN(PoolItemNoCopy("bar_nocopy", 13).swap(dummy_nocopy)) ;
      CPPUNIT_LOG_RUN(TestPool.checkin("dummy2", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)3) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)2) ;
      CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, std::vector<value_pair>()) ;

      CPPUNIT_LOG_RUN(PoolItemNoCopy("quux_nocopy", 777).swap(dummy_nocopy)) ;
      CPPUNIT_LOG_RUN(TestPool.checkin("dummy1", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)4) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)2) ;

      CPPUNIT_LOG_RUN(PoolItemNoCopy("xyzzy_nocopy", 123).swap(dummy_nocopy)) ;
      CPPUNIT_LOG_RUN(TestPool.checkin("dummy2", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)5) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)2) ;

      CPPUNIT_LOG_RUN(PoolItemNoCopy("bar_nocopy", 1).swap(dummy_nocopy)) ;
      CPPUNIT_LOG_RUN(TestPool.checkin("dummy2", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)5) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)2) ;

      // Evicted
      CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, CPPUNIT_CONTAINER(std::vector<value_pair>,
                                                                  (value_pair("dummy_nocopy", 13)))) ;

      CPPUNIT_LOG_RUN(PoolItemNoCopy("xyzzy_nocopy", 567).swap(dummy_nocopy)) ;
      CPPUNIT_LOG_RUN(TestPool.checkin("dummy3", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)5) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)3) ;

      // Evicted
      CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, CPPUNIT_CONTAINER(std::vector<value_pair>,
                                                                     (value_pair("dummy_nocopy", 13))
                                                                     (value_pair("foo_nocopy", 7)))) ;


      CPPUNIT_LOG_RUN(PoolItemNoCopy("xyzzy_nocopy", 891).swap(dummy_nocopy)) ;
      CPPUNIT_LOG_RUN(TestPool.checkin("dummy3", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)5) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)3) ;

      // Evicted
      CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, CPPUNIT_CONTAINER(std::vector<value_pair>,
                                                                     (value_pair("dummy_nocopy", 13))
                                                                     (value_pair("foo_nocopy", 7))
                                                                     (value_pair("bar_nocopy", 13)))) ;


      CPPUNIT_LOG_RUN(PoolItemNoCopy("world_nocopy", 0).swap(dummy_nocopy)) ;
      CPPUNIT_LOG_RUN(TestPool.checkin("dummy4", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)5) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)4) ;

      // Evicted
      CPPUNIT_LOG_EQUAL(PoolItemNoCopy::destroyed, CPPUNIT_CONTAINER(std::vector<value_pair>,
                                                                     (value_pair("dummy_nocopy", 13))
                                                                     (value_pair("foo_nocopy", 7))
                                                                     (value_pair("bar_nocopy", 13))
                                                                     (value_pair("quux_nocopy", 777)))) ;

      CPPUNIT_LOG(std::endl) ;
      std::vector<key_itemcount> KeyInfo ;
      // dummy1 must be empty
      CPPUNIT_LOG_RUN(TestPool.keys(std::back_inserter(KeyInfo))) ;
      CPPUNIT_LOG_EQUAL(CPPUNIT_SORTED(KeyInfo), CPPUNIT_CONTAINER(std::vector<key_itemcount>,
                                                                   (key_itemcount("dummy1", 0))
                                                                   (key_itemcount("dummy2", 2))
                                                                   (key_itemcount("dummy3", 2))
                                                                   (key_itemcount("dummy4", 1)))) ;

      dummy_nocopy.clear() ;
      CPPUNIT_LOG_ASSERT(TestPool.checkout("dummy2", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(dummy_nocopy.value(), value_pair("bar_nocopy", 1)) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)4) ;
      pcomn::swap_clear(KeyInfo) ;
      CPPUNIT_LOG_RUN(TestPool.keys(std::back_inserter(KeyInfo))) ;
      CPPUNIT_LOG_EQUAL(CPPUNIT_SORTED(KeyInfo), CPPUNIT_CONTAINER(std::vector<key_itemcount>,
                                                                   (key_itemcount("dummy1", 0))
                                                                   (key_itemcount("dummy2", 1))
                                                                   (key_itemcount("dummy3", 2))
                                                                   (key_itemcount("dummy4", 1)))) ;

      dummy_nocopy.clear() ;
      CPPUNIT_LOG_IS_FALSE(TestPool.checkout("dummy1", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(dummy_nocopy.value(), value_pair()) ;
      pcomn::swap_clear(KeyInfo) ;
      CPPUNIT_LOG_RUN(TestPool.keys(std::back_inserter(KeyInfo))) ;
      CPPUNIT_LOG_EQUAL(CPPUNIT_SORTED(KeyInfo), CPPUNIT_CONTAINER(std::vector<key_itemcount>,
                                                                   (key_itemcount("dummy1", 0))
                                                                   (key_itemcount("dummy2", 1))
                                                                   (key_itemcount("dummy3", 2))
                                                                   (key_itemcount("dummy4", 1)))) ;


      CPPUNIT_LOG_RUN(PoolItemNoCopy("xyzzy_nocopy", 234).swap(dummy_nocopy)) ;
      CPPUNIT_LOG_RUN(TestPool.checkin("dummy3", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)5) ;
      CPPUNIT_LOG_EQUAL(TestPool.key_count(), (size_t)4) ;

      CPPUNIT_LOG_RUN(PoolItemNoCopy("xyzzy_nocopy", 101).swap(dummy_nocopy)) ;
      CPPUNIT_LOG_RUN(TestPool.checkin("dummy3", dummy_nocopy)) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)5) ;
      pcomn::swap_clear(KeyInfo) ;
      CPPUNIT_LOG_RUN(TestPool.keys(std::back_inserter(KeyInfo))) ;
      CPPUNIT_LOG_EQUAL(CPPUNIT_SORTED(KeyInfo), CPPUNIT_CONTAINER(std::vector<key_itemcount>,
                                                                   (key_itemcount("dummy1", 0))
                                                                   (key_itemcount("dummy2", 0))
                                                                   (key_itemcount("dummy3", 4))
                                                                   (key_itemcount("dummy4", 1)))) ;

      CPPUNIT_LOG(std::endl) ;
      CPPUNIT_LOG_EQUAL(TestPool.erase("dummy4"), (size_t)1) ;
      pcomn::swap_clear(KeyInfo) ;
      CPPUNIT_LOG_RUN(TestPool.keys(std::back_inserter(KeyInfo))) ;
      CPPUNIT_LOG_EQUAL(CPPUNIT_SORTED(KeyInfo), CPPUNIT_CONTAINER(std::vector<key_itemcount>,
                                                                   (key_itemcount("dummy1", 0))
                                                                   (key_itemcount("dummy2", 0))
                                                                   (key_itemcount("dummy3", 4)))) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)4) ;

      CPPUNIT_LOG(std::endl) ;
      CPPUNIT_LOG_RUN(PoolItemNoCopy("foo_nocopy", 7).swap(dummy_nocopy)) ;
      // Must collect empty keys on this checkin (at the moment, more than half of the
      // keys are empty)
      CPPUNIT_LOG_RUN(TestPool.checkin("dummy5", dummy_nocopy)) ;
      pcomn::swap_clear(KeyInfo) ;
      CPPUNIT_LOG_RUN(TestPool.keys(std::back_inserter(KeyInfo))) ;
      CPPUNIT_LOG_EQUAL(CPPUNIT_SORTED(KeyInfo), CPPUNIT_CONTAINER(std::vector<key_itemcount>,
                                                                   (key_itemcount("dummy3", 4))
                                                                   (key_itemcount("dummy5", 1)))) ;
      CPPUNIT_LOG_EQUAL(TestPool.size(), (size_t)5) ;
   }
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(CacherTests::suite()) ;
   runner.addTest(KeyedPoolTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "unittest.cacher.trace.ini", "pcomn::cacher and pcomn::keyed_pool tests") ;
}
