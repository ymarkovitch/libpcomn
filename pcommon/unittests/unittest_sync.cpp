/*-*- tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_mutex.cpp
 COPYRIGHT    :   Yakov Markovitch, 2009-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for mutexes.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   2 Feb 2009
*******************************************************************************/
#include <pcomn_unittest.h>
#include <pcomn_syncobj.h>

#include <thread>
#include <mutex>
#include <chrono>

#include <stdlib.h>

/*******************************************************************************
 class MutexTests
*******************************************************************************/
class MutexTests : public CppUnit::TestFixture {

   private:
      void Test_Scoped_Lock() ;
      void Test_Scoped_ReadWriteLock() ;

      CPPUNIT_TEST_SUITE(MutexTests) ;

      CPPUNIT_TEST(Test_Scoped_Lock) ;
      CPPUNIT_TEST(Test_Scoped_ReadWriteLock) ;

      CPPUNIT_TEST_SUITE_END() ;

} ;

/*******************************************************************************
 class MutexTests
*******************************************************************************/
class PromiseLockTests : public CppUnit::TestFixture {

      void Test_Promise_SingleThreaded() ;
      void Test_Promise_MultiThreaded() ;

      CPPUNIT_TEST_SUITE(PromiseLockTests) ;

      CPPUNIT_TEST(Test_Promise_SingleThreaded) ;
      CPPUNIT_TEST(Test_Promise_MultiThreaded) ;

      CPPUNIT_TEST_SUITE_END() ;

   private:
      std::timed_mutex  armed ;
      std::thread       watchdog ;

   public:
      void setUp()
      {
         armed.lock() ;

         watchdog = std::thread([&]{
            if (!armed.try_lock_for(std::chrono::seconds(5)))
            {
               CPPUNIT_LOG_LINE("ERROR: THE TEST DEADLOCKED") ;
               exit(3) ;
            }
         }) ;
      }
      void tearDown()
      {
         armed.unlock() ;
         watchdog.join() ;
      }
} ;

/*******************************************************************************
 MutexTests
*******************************************************************************/
void MutexTests::Test_Scoped_Lock()
{
   std::recursive_mutex rmutex ;

   CPPUNIT_LOG_ASSERT(rmutex.try_lock()) ;
   CPPUNIT_LOG_RUN(rmutex.unlock()) ;
   {
      PCOMN_SCOPE_LOCK(guard2, rmutex) ;
   }
}

void MutexTests::Test_Scoped_ReadWriteLock()
{
   pcomn::shared_mutex rwmutex ;

   CPPUNIT_LOG_ASSERT(rwmutex.try_lock_shared()) ;
   CPPUNIT_LOG_RUN(rwmutex.unlock_shared()) ;

   CPPUNIT_LOG_ASSERT(rwmutex.try_lock()) ;
   CPPUNIT_LOG_RUN(rwmutex.unlock()) ;

   {
      PCOMN_SCOPE_R_LOCK(rguard1, rwmutex) ;

      CPPUNIT_LOG_IS_TRUE(rguard1) ;

      CPPUNIT_LOG_IS_TRUE(rwmutex.try_lock_shared()) ;
      CPPUNIT_LOG_RUN(rwmutex.unlock_shared()) ;
      CPPUNIT_LOG_IS_FALSE(rwmutex.try_lock()) ;

      CPPUNIT_LOG_IS_TRUE(rguard1.try_lock()) ;
      CPPUNIT_LOG_RUN(rguard1.unlock()) ;

      CPPUNIT_LOG_IS_FALSE(rguard1) ;
      CPPUNIT_LOG_RUN(rwmutex.unlock_shared()) ;
   }
   CPPUNIT_LOG_ASSERT(rwmutex.try_lock()) ;
   CPPUNIT_LOG_RUN(rwmutex.unlock()) ;
   {
      PCOMN_SCOPE_W_LOCK(wguard1, rwmutex) ;

      CPPUNIT_LOG_IS_FALSE(rwmutex.try_lock_shared()) ;
      CPPUNIT_LOG_IS_FALSE(rwmutex.try_lock()) ;
   }
   CPPUNIT_LOG_ASSERT(rwmutex.try_lock()) ;
   CPPUNIT_LOG_RUN(rwmutex.unlock()) ;
   {
      PCOMN_SCOPE_W_XLOCK(wguard1, rwmutex) ;

      CPPUNIT_LOG_IS_TRUE(wguard1) ;

      CPPUNIT_LOG_IS_FALSE(rwmutex.try_lock_shared()) ;
      CPPUNIT_LOG_IS_FALSE(rwmutex.try_lock()) ;

      CPPUNIT_LOG_IS_TRUE(wguard1) ;
   }
   CPPUNIT_LOG_ASSERT(rwmutex.try_lock()) ;
   CPPUNIT_LOG_RUN(rwmutex.unlock()) ;
}

/*******************************************************************************
 PromiseLockTests
*******************************************************************************/
using namespace pcomn ;
void PromiseLockTests::Test_Promise_SingleThreaded()
{
   using namespace std ;
   promise_lock lock0 {false} ;

   CPPUNIT_LOG_RUN(lock0.wait()) ;
   CPPUNIT_LOG_RUN(lock0.wait()) ;
   CPPUNIT_LOG_RUN(lock0.unlock()) ;
   CPPUNIT_LOG_RUN(lock0.wait()) ;

   CPPUNIT_LOG(std::endl) ;

   promise_lock lock1 ;
   CPPUNIT_LOG_RUN(lock1.unlock()) ;
   CPPUNIT_LOG_RUN(lock1.wait()) ;
   CPPUNIT_LOG_RUN(lock1.unlock()) ;
   CPPUNIT_LOG_RUN(lock1.wait()) ;

   promise_lock lock2 {true} ;
   CPPUNIT_LOG_RUN(lock2.unlock()) ;
   CPPUNIT_LOG_RUN(lock2.wait()) ;
   CPPUNIT_LOG_RUN(lock2.unlock()) ;
   CPPUNIT_LOG_RUN(lock2.wait()) ;

   promise_lock lock3 {true} ;
   CPPUNIT_LOG_RUN(lock3.unlock()) ;
   CPPUNIT_LOG_RUN(lock3.unlock()) ;
   CPPUNIT_LOG_RUN(lock3.wait()) ;
}

void PromiseLockTests::Test_Promise_MultiThreaded()
{
   using namespace std ;
   vector<int> v1, v2, v3, v4 ;
   for (vector<int> *v: {&v1, &v2, &v3, &v4})
      v->reserve(100) ;

   std::thread t1, t2, t3, t4 ;
   promise_lock lock1 ;
   t1 = thread([&]{
      v1.push_back(10001) ;
      lock1.wait() ;
      v1.push_back(10002) ;
      lock1.wait() ;
      v1.push_back(10003) ;
   }) ;
   CPPUNIT_LOG_RUN(this_thread::sleep_for(chrono::milliseconds(100))) ;
   CPPUNIT_LOG_EQ(v1.size(), 1) ;
   CPPUNIT_LOG_EQ(v1.front(), 10001) ;
   CPPUNIT_LOG_RUN(this_thread::sleep_for(chrono::milliseconds(100))) ;
   CPPUNIT_LOG_EQ(v1.size(), 1) ;
   CPPUNIT_LOG_EQ(v1.front(), 10001) ;
   CPPUNIT_LOG_RUN(lock1.unlock()) ;
   CPPUNIT_LOG_RUN(this_thread::sleep_for(chrono::milliseconds(100))) ;
   CPPUNIT_LOG_EQ(v1.size(), 3) ;
   CPPUNIT_LOG_EQUAL(v1, (std::vector<int>{10001, 10002, 10003})) ;
   CPPUNIT_LOG_RUN(t1.join()) ;

   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_RUN(v1.clear()) ;
   CPPUNIT_LOG_RUN(v1.resize(100)) ;
   CPPUNIT_LOG_RUN(v1.clear()) ;

   promise_lock lock2 {true} ;
   promise_lock lock3 {true} ;
   t1 = thread([&]{
      lock3.wait() ;
      v1.push_back(10007) ;
      lock2.wait() ;
      v1.push_back(10008) ;
      lock2.wait() ;
      v1.push_back(10009) ;
   }) ;
   t2 = thread([&]{
      lock3.wait() ;
      v2.push_back(20007) ;
      lock2.wait() ;
      v2.push_back(20008) ;
      lock2.wait() ;
      v2.push_back(20009) ;
   }) ;
   t3 = thread([&]{
      lock3.wait() ;
      v3.push_back(30007) ;
      lock2.wait() ;
      v3.push_back(30008) ;
      lock2.wait() ;
      v3.push_back(30009) ;
   }) ;
   t4 = thread([&]{
      lock3.wait() ;
      v4.push_back(40007) ;
      lock2.wait() ;
      v4.push_back(40008) ;
      lock2.wait() ;
      v4.push_back(40009) ;
   }) ;
   CPPUNIT_LOG_RUN(this_thread::sleep_for(chrono::milliseconds(100))) ;
   CPPUNIT_LOG_EQ(v1.size(), 0) ;
   CPPUNIT_LOG_EQ(v2.size(), 0) ;
   CPPUNIT_LOG_EQ(v3.size(), 0) ;
   CPPUNIT_LOG_EQ(v4.size(), 0) ;

   CPPUNIT_LOG_RUN(lock3.unlock()) ;
   CPPUNIT_LOG_RUN(this_thread::sleep_for(chrono::milliseconds(100))) ;

   vector<int> v11 (v1), v21 (v2), v31 (v3), v41 (v4) ;

   CPPUNIT_LOG_RUN(lock2.unlock()) ;
   CPPUNIT_LOG_RUN(this_thread::sleep_for(chrono::milliseconds(100))) ;

   vector<int> v12 (v1), v22 (v2), v32 (v3), v42 (v4) ;

   CPPUNIT_LOG_RUN(lock2.wait()) ;
   CPPUNIT_LOG_RUN(lock3.wait()) ;

   for (thread *t: {&t1, &t2, &t3, &t4})
      t->join() ;

   CPPUNIT_LOG_EQUAL(v11, (std::vector<int>{10007})) ;
   CPPUNIT_LOG_EQUAL(v21, (std::vector<int>{20007})) ;
   CPPUNIT_LOG_EQUAL(v31, (std::vector<int>{30007})) ;
   CPPUNIT_LOG_EQUAL(v41, (std::vector<int>{40007})) ;

   CPPUNIT_LOG_EQUAL(v12, (std::vector<int>{10007, 10008, 10009})) ;
   CPPUNIT_LOG_EQUAL(v22, (std::vector<int>{20007, 20008, 20009})) ;
   CPPUNIT_LOG_EQUAL(v32, (std::vector<int>{30007, 30008, 30009})) ;
   CPPUNIT_LOG_EQUAL(v42, (std::vector<int>{40007, 40008, 40009})) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(MutexTests::suite()) ;
   runner.addTest(PromiseLockTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "sync.diag.ini", "Tests of various synchronization primitives") ;
}
