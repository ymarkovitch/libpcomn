/*-*- tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_mutex.cpp
 COPYRIGHT    :   Yakov Markovitch, 2009-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for mutexes.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   2 Feb 2009
*******************************************************************************/
#include <pcomn_unittest.h>
#include <pcomn_syncobj.h>
#include <pcomn_synccomplex.h>

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

void MutexTests::Test_Scoped_Lock()
{
   pcomn::event_mutex nrmutex ;
   std::recursive_mutex rmutex ;

   CPPUNIT_LOG_ASSERT(nrmutex.try_lock()) ;
   CPPUNIT_LOG_RUN(nrmutex.unlock()) ;
   CPPUNIT_LOG_ASSERT(rmutex.try_lock()) ;
   CPPUNIT_LOG_RUN(rmutex.unlock()) ;
   {
      PCOMN_SCOPE_LOCK(guard1, nrmutex) ;
      PCOMN_SCOPE_LOCK(guard2, rmutex) ;

      CPPUNIT_LOG_IS_FALSE(nrmutex.try_lock()) ;
   }
   CPPUNIT_LOG_ASSERT(nrmutex.try_lock()) ;
   CPPUNIT_LOG_RUN(nrmutex.unlock()) ;
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

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(MutexTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "mutex.diag.ini", "Tests of various mutexes") ;
}
