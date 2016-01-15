/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_mxmutex.cpp
 COPYRIGHT    :   Yakov Markovitch, 2009-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for multiplexed mutex.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   12 May 2009
*******************************************************************************/
#include <pcomn_unittest.h>
#include <pcomn_mxmutex.h>
#include <pcomn_synccomplex.h>

#include <stdlib.h>

/*******************************************************************************
                     class MultiplexedMutexTests
*******************************************************************************/
class MultiplexedMutexTests : public CppUnit::TestFixture {

   private:
      void Test_MxMutex_Constructor() ;
      void Test_KeyedMxMutex_Constructor() ;

      CPPUNIT_TEST_SUITE(MultiplexedMutexTests) ;

      CPPUNIT_TEST(Test_MxMutex_Constructor) ;
      CPPUNIT_TEST(Test_KeyedMxMutex_Constructor) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

struct talkative_string : public std::string {

      template<typename S>
      talkative_string(const S &init) :
         std::string(init)
      {
         std::cout << "\nConstructed at " << this << " '" << *this << "'" << std::endl ;
      }

      talkative_string(const talkative_string &other) :
         std::string(other)
      {
         std::cout << "\nCopy-constructed " << this << " from " << &other << " '" << *this << "'" << std::endl ;
      }

      ~talkative_string()
      {
         std::cout << "\nDestructed at " << this << " '" << *this << "'" << std::endl ;
      }
} ;

inline size_t hasher(const talkative_string &str)
{
   return pcomn::hasher<std::string>(str) ;
}

void MultiplexedMutexTests::Test_MxMutex_Constructor()
{
   typedef pcomn::PTMxMutex<std::mutex, 10, int> mutex10 ;
   typedef pcomn::PTMxMutex<std::mutex, 1, int> mutex1 ;
   typedef pcomn::PTMxMutex<std::mutex, 2, int> mutex2 ;
   typedef pcomn::PTMxMutex<std::mutex, 16, int> mutex16 ;
   typedef pcomn::PTMxMutex<std::mutex, 17, int> mutex17 ;

   mutex10 mi ;
   CPPUNIT_LOG_EQUAL(mutex1::capacity(), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(mutex2::capacity(), (size_t)3) ;
   CPPUNIT_LOG_EQUAL(mutex16::capacity(), (size_t)17) ;
   CPPUNIT_LOG_EQUAL(mutex17::capacity(), (size_t)37) ;
   CPPUNIT_LOG_EQUAL(mi.capacity(), (size_t)17) ;

   {
      std::lock_guard<mutex10> lock (mi, 2) ;
      CPPUNIT_LOG_IS_FALSE(mi.try_lock(2)) ;
      CPPUNIT_LOG_IS_TRUE(mi.try_lock(3)) ;
      CPPUNIT_LOG_IS_FALSE(mi.try_lock(2)) ;
      CPPUNIT_LOG_IS_FALSE(mi.try_lock(3)) ;

      CPPUNIT_LOG_RUN(lock.unlock()) ;
      CPPUNIT_LOG_IS_TRUE(mi.try_lock(2)) ;
      CPPUNIT_LOG_RUN(mi.unlock(3)) ;
      CPPUNIT_LOG_IS_FALSE(mi.try_lock(2)) ;
      CPPUNIT_LOG_IS_TRUE(mi.try_lock(3)) ;
      CPPUNIT_LOG_IS_FALSE(mi.try_lock(3)) ;

      CPPUNIT_LOG_RUN(mi.unlock(3)) ;
      CPPUNIT_LOG_RUN(mi.unlock(2)) ;
   }

   talkative_string hello ("Hello!") ;

   typedef pcomn::PTMxMutex<std::mutex, 17, talkative_string> smutex17 ;

   std::cout << "Locking 'Hello'" << std::endl ;
   {
      smutex17 kms ;
      PCOMN_SCOPE_LOCK (guard, kms, hello) ;
      std::cout << "Locked 'Hello'" << std::endl ;
      CPPUNIT_LOG_IS_FALSE(kms.try_lock(hello)) ;
   }
   std::cout << "End of locking 'Hello'" << std::endl ;
}

void MultiplexedMutexTests::Test_KeyedMxMutex_Constructor()
{
   typedef pcomn::PTMxMutex<pcomn::shared_mutex, 10, int> mutex10 ;

   mutex10 mi ;
   {
      CPPUNIT_LOG_IS_TRUE(mi.try_lock_shared(2)) ;
      CPPUNIT_LOG_IS_TRUE(mi.try_lock_shared(2)) ;
      CPPUNIT_LOG_IS_TRUE(mi.try_lock_shared(3)) ;
      CPPUNIT_LOG_IS_TRUE(mi.try_lock(4)) ;
      CPPUNIT_LOG_IS_FALSE(mi.try_lock_shared(4)) ;
      CPPUNIT_LOG_IS_FALSE(mi.try_lock(4)) ;
      CPPUNIT_LOG_IS_FALSE(mi.try_lock(2)) ;

      CPPUNIT_LOG_RUN(mi.unlock(2)) ;
      CPPUNIT_LOG_IS_FALSE(mi.try_lock(2)) ;
      CPPUNIT_LOG_RUN(mi.unlock(2)) ;
      CPPUNIT_LOG_IS_TRUE(mi.try_lock(2)) ;

      CPPUNIT_LOG(std::endl) ;
      CPPUNIT_LOG_RUN(mi.unlock(2)) ;
      CPPUNIT_LOG_RUN(mi.unlock(3)) ;
      CPPUNIT_LOG_RUN(mi.unlock(4)) ;

      CPPUNIT_LOG_IS_TRUE(mi.try_lock(2)) ;
      CPPUNIT_LOG_IS_TRUE(mi.try_lock(3)) ;
      CPPUNIT_LOG_IS_TRUE(mi.try_lock(4)) ;

      CPPUNIT_LOG_RUN(mi.unlock(2)) ;
      CPPUNIT_LOG_RUN(mi.unlock(3)) ;
      CPPUNIT_LOG_RUN(mi.unlock(4)) ;
   }
   {
      pcomn::shared_lock<mutex10> rlock (mi, 2) ;
      std::lock_guard<mutex10> wlock (mi, 3) ;
   }

   CPPUNIT_LOG(std::endl) ;
   talkative_string bye ("Bye") ;
   talkative_string hello ("Hello") ;

   typedef pcomn::PTMxMutex<pcomn::shared_mutex, 17, talkative_string> smutex17 ;

   std::cout << "Locking 'Bye'" << std::endl ;
   {
      smutex17 kms ;
      PCOMN_SCOPE_R_LOCK (guard, kms, bye) ;
      std::cout << "Locked 'Bye'" << std::endl ;

      CPPUNIT_LOG_IS_TRUE(kms.try_lock_shared(bye)) ;
      CPPUNIT_LOG_RUN(kms.unlock(bye)) ;
      CPPUNIT_LOG_IS_FALSE(kms.try_lock(bye)) ;
   }
   std::cout << "End of locking 'Hello'" << std::endl ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(MultiplexedMutexTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "mxmutex.diag.ini", "Multiplexed mutex tests") ;
}
