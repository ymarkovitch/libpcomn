/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_keyedmutex.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for the keyed mutex.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   29 Aug 2008
*******************************************************************************/
#include <pcomn_unittest.h>
#include <pcomn_keyedmutex.h>
#include <pcomn_numeric.h>
#include <pcomn_stopwatch.h>

#include <thread>
#include <stdlib.h>

/*******************************************************************************
                     class KeyedMutexTests
*******************************************************************************/
class KeyedMutexTests : public CppUnit::TestFixture {

   private:
      void Test_DoublingPrimes() ;

      void Test_KeyedMutex_Constructor() ;
      void Test_KeyedRWMutex_Constructor() ;

      template<unsigned nKeys>
      void Test_KeyedMutex_Run() ;

      CPPUNIT_TEST_SUITE(KeyedMutexTests) ;

      CPPUNIT_TEST(Test_DoublingPrimes) ;
      CPPUNIT_TEST(Test_KeyedMutex_Constructor) ;
      CPPUNIT_TEST(Test_KeyedRWMutex_Constructor) ;

      CPPUNIT_TEST(Test_KeyedMutex_Run<8>) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

void KeyedMutexTests::Test_DoublingPrimes()
{
   CPPUNIT_LOG_EQUAL(pcomn::dprime_lbound(0), 3u) ;
   CPPUNIT_LOG_EQUAL(pcomn::dprime_ubound(0), 3u) ;
   CPPUNIT_LOG_EQUAL(pcomn::dprime_lbound(1), 3u) ;
   CPPUNIT_LOG_EQUAL(pcomn::dprime_ubound(1), 3u) ;
   CPPUNIT_LOG_EQUAL(pcomn::dprime_lbound(3), 3u) ;
   CPPUNIT_LOG_EQUAL(pcomn::dprime_ubound(3), 3u) ;
   CPPUNIT_LOG_EQUAL(pcomn::dprime_lbound(4), 3u) ;
   CPPUNIT_LOG_EQUAL(pcomn::dprime_ubound(4), 7u) ;
   CPPUNIT_LOG_EQUAL(pcomn::dprime_lbound(7), 7u) ;
   CPPUNIT_LOG_EQUAL(pcomn::dprime_ubound(7), 7u) ;

   CPPUNIT_LOG_EQUAL(pcomn::dprime_lbound(4294967290u), 3221225473u) ;
   CPPUNIT_LOG_EQUAL(pcomn::dprime_ubound(4294967290u), 4294967291u) ;

   CPPUNIT_LOG_EQUAL(pcomn::dprime_lbound(4294967295u), 4294967291u) ;
   CPPUNIT_LOG_EQUAL(pcomn::dprime_ubound(4294967295u), 4294967291u) ;
}

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

void KeyedMutexTests::Test_KeyedMutex_Constructor()
{
   pcomn::PTKeyedMutex<int> kmi (1) ;
   {
      std::lock_guard<pcomn::PTKeyedMutex<int> > lock (kmi, 2) ;
      CPPUNIT_LOG_IS_FALSE(kmi.try_lock(2)) ;
      CPPUNIT_LOG_IS_TRUE(kmi.try_lock(3)) ;
      CPPUNIT_LOG_IS_FALSE(kmi.try_lock(2)) ;
      CPPUNIT_LOG_IS_FALSE(kmi.try_lock(3)) ;

      CPPUNIT_LOG_RUN(lock.unlock()) ;
      CPPUNIT_LOG_IS_TRUE(kmi.try_lock(2)) ;
      CPPUNIT_LOG_IS_TRUE(kmi.unlock(3)) ;
      CPPUNIT_LOG_IS_FALSE(kmi.try_lock(2)) ;
      CPPUNIT_LOG_IS_TRUE(kmi.try_lock(3)) ;
      CPPUNIT_LOG_IS_FALSE(kmi.try_lock(3)) ;

      CPPUNIT_LOG_IS_TRUE(kmi.unlock(3)) ;
      CPPUNIT_LOG_IS_TRUE(kmi.unlock(2)) ;
   }

   talkative_string hello ("Hello!") ;

   std::cout << "Locking 'Hello'" << std::endl ;
   {
      pcomn::PTKeyedMutex<talkative_string> kms (1) ;

      std::lock_guard<pcomn::PTKeyedMutex<talkative_string> > lock (kms, hello) ;
      std::cout << "Locked 'Hello'" << std::endl ;
   }
   std::cout << "End of locking 'Hello'" << std::endl ;
}

void KeyedMutexTests::Test_KeyedRWMutex_Constructor()
{
   pcomn::PTKeyedRWMutex<int> kmi (1) ;
   {
      CPPUNIT_LOG_IS_TRUE(kmi.try_lock_shared(2)) ;
      CPPUNIT_LOG_IS_TRUE(kmi.try_lock_shared(2)) ;
      CPPUNIT_LOG_IS_TRUE(kmi.try_lock_shared(3)) ;
      CPPUNIT_LOG_IS_TRUE(kmi.try_lock(4)) ;
      CPPUNIT_LOG_IS_FALSE(kmi.try_lock_shared(4)) ;
      CPPUNIT_LOG_IS_FALSE(kmi.try_lock(4)) ;
      CPPUNIT_LOG_IS_FALSE(kmi.try_lock(2)) ;

      CPPUNIT_LOG_RUN(kmi.unlock(2)) ;
      CPPUNIT_LOG_IS_FALSE(kmi.try_lock(2)) ;
      CPPUNIT_LOG_RUN(kmi.unlock(2)) ;
      CPPUNIT_LOG_IS_TRUE(kmi.try_lock(2)) ;

      CPPUNIT_LOG(std::endl) ;
      CPPUNIT_LOG_RUN(kmi.unlock(2)) ;
      CPPUNIT_LOG_RUN(kmi.unlock(3)) ;
      CPPUNIT_LOG_RUN(kmi.unlock(4)) ;

      CPPUNIT_LOG_IS_TRUE(kmi.try_lock(2)) ;
      CPPUNIT_LOG_IS_TRUE(kmi.try_lock(3)) ;
      CPPUNIT_LOG_IS_TRUE(kmi.try_lock(4)) ;

      CPPUNIT_LOG_RUN(kmi.unlock(2)) ;
      CPPUNIT_LOG_RUN(kmi.unlock(3)) ;
      CPPUNIT_LOG_RUN(kmi.unlock(4)) ;
   }
   {
      pcomn::shared_lock<pcomn::PTKeyedRWMutex<int> > rlock (kmi, 2) ;
      std::lock_guard<pcomn::PTKeyedRWMutex<int> > wlock (kmi, 3) ;
   }
}

template<unsigned n>
void slow_increment(pcomn::PTKeyedMutex<unsigned*> &mutex, unsigned *target, unsigned count)
{
   unsigned *slots[n] ;
   unsigned seed ;

   pcomn::iota(slots + 0, slots + n, target) ;
   std::random_shuffle(slots + 0, slots + n) ;

   for (unsigned cnt = count ; cnt ; --cnt)
      for (unsigned **s = slots ; s != slots + n ; ++s)
      {
         std::lock_guard<pcomn::PTKeyedMutex<unsigned *> > lock (mutex, *s) ;
         unsigned v = **s ;
         msleep((int)(3 * (rand_r(&seed) / (RAND_MAX + 1.0)))) ;
         **s = v + 1 ;
      }
}

template<unsigned nKeys>
void KeyedMutexTests::Test_KeyedMutex_Run()
{
   static const auto incrementer = slow_increment<nKeys> ;
   typedef pcomn::PTKeyedMutex<unsigned *> keyed_mutex ;
   typedef unsigned buffer_type[nKeys] ;

   struct {
         static void check(unsigned *Buffer, unsigned Count)
         {
            CPPUNIT_LOG(std::endl) ;
            for (unsigned i = 0 ; i < nKeys ; ++i)
            {
               CPPUNIT_LOG("Key=" << Buffer + i << std::endl) ;
               CPPUNIT_LOG_EQUAL(Buffer[i], Count) ;
            }
         }
   } Tester ;

   {
      const unsigned Count = 200 ;
      const unsigned Threads = 1 ;

      buffer_type Buffer ;
      pcomn::raw_fill(Buffer, 0u) ;
      keyed_mutex IncMutex (4, 4) ;

      pcomn::PRealStopwatch sw ;

      sw.start() ;
      std::thread(incrementer, std::ref(IncMutex), Buffer, Count).join() ;
      sw.stop() ;

      CPPUNIT_LOG("\nCount=" << Count*Threads << ", " << Threads << " thread(s), " << sw << "s" << std::endl) ;

      Tester.check(Buffer, Count*Threads) ;
   }

   {
      const unsigned Count = 25000 ;
      const unsigned Threads = 2 ;

      buffer_type Buffer ;
      pcomn::raw_fill(Buffer, 0u) ;
      keyed_mutex IncMutex (nKeys/2) ;
      std::thread test_threads[Threads] ;
      pcomn::PRealStopwatch sw ;

      sw.start() ;

      for (auto &t: test_threads)
         t = std::move(std::thread(incrementer, std::ref(IncMutex), Buffer, Count)) ;
      for (auto &t: test_threads)
         t.join() ;

      sw.stop() ;

      CPPUNIT_LOG("\nCount=" << Count*Threads << ", " << Threads << " thread(s), " << sw << "s" << std::endl) ;

      Tester.check(Buffer, Count*Threads) ;
   }

   {
      const unsigned Count = 5000 ;
      const unsigned Threads = 10 ;

      buffer_type Buffer ;
      pcomn::raw_fill(Buffer, 0u) ;

      keyed_mutex IncMutex (4, 8) ;
      std::thread test_threads[Threads] ;

      pcomn::PRealStopwatch sw ;
      sw.start() ;

      for (auto &t: test_threads)
         t = std::move(std::thread(incrementer, std::ref(IncMutex), Buffer, Count)) ;
      for (auto &t: test_threads)
         t.join() ;

      sw.stop() ;

      CPPUNIT_LOG("\nCount=" << Count*Threads << ", " << Threads << " thread(s), " << sw << "s" << std::endl) ;

      Tester.check(Buffer, Count*Threads) ;
   }
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(KeyedMutexTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "keyedmutex.diag.ini", "Keyed mutex tests") ;
}
