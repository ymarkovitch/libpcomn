/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_keyedmutex.cpp
 COPYRIGHT    :   Yakov Markovitch, 2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for Lanes

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   15 Jan 2014
*******************************************************************************/
#include <pcomn_unittest_mt.h>
#include <pcomn_lanes.h>

#include <thread>
#include <atomic>

using namespace pcomn ;
using namespace pcomn::unit ;

/*******************************************************************************
                     class LanesTests
*******************************************************************************/
class LanesTests : public CppUnit::TestFixture {

      void Test_ThreadPack() ;
      void Test_Lanes() ;

      CPPUNIT_TEST_SUITE(LanesTests) ;

      CPPUNIT_TEST(Test_ThreadPack) ;
      CPPUNIT_TEST(Test_Lanes) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

/*******************************************************************************
                     class TestState
*******************************************************************************/
class TestState {
      PCOMN_NONASSIGNABLE(TestState) ;
   public:
      TestState() :
         _construct_thread(std::this_thread::get_id()),
         _state_id(++_global_state_count)
      {}

      TestState(const TestState &src) :
         _construct_thread(std::this_thread::get_id()),
         _state_id(src._state_id)
      {}

      unsigned id() const { return _state_id ; }
      std::thread::id construct_thread_id() const { return _construct_thread ; }

      friend std::ostream &operator<<(std::ostream &os, const TestState &v)
      {
         return os << "State" << v.id() << '@' << v.construct_thread_id() ;
      }

   private:
      std::thread::id _construct_thread ;
      unsigned        _state_id ;

      static std::atomic_uint _global_state_count ;
      static thread_local std::atomic_uint _thread_state_count ;
} ;

std::atomic_uint                TestState::_global_state_count ;
thread_local std::atomic_uint   TestState::_thread_state_count ;

typedef pcomn::lanes<TestState> test_lanes ;

/*******************************************************************************
 LanesTests
*******************************************************************************/
void LanesTests::Test_ThreadPack()
{
   ThreadPack tp (4) ;
   auto f1 = []() { CPPUNIT_LOG_LINE(std::this_thread::get_id()) ; } ;
   tp.submit_work(1, f1) ;
   tp.submit_work(3, f1) ;
   tp.launch() ;
   tp.submit_work(1, f1) ;
   tp.submit_work(0, f1) ;
   tp.submit_work(2, f1) ;
   tp.launch() ;
   tp.launch() ;
   tp.cancel() ;
}

void LanesTests::Test_Lanes()
{
   test_lanes t ;
   (void)t ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(LanesTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "lanes.diag.ini", "Lanes tests") ;
}
