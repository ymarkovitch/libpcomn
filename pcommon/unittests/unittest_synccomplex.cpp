/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_synccomplex.cpp
 COPYRIGHT    :   Yakov Markovitch, 2009-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for the keyed mutex.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   22 Feb 2009
*******************************************************************************/
#include <pcomn_unittest.h>
#include <pcomn_synccomplex.h>
#include <pcomn_thread.h>
#include <pcomn_stopwatch.h>

#include <algorithm>
#include <stdlib.h>

/*******************************************************************************
                     class IdentDispenserTests
*******************************************************************************/
class IdentDispenserTests : public CppUnit::TestFixture {

   private:
      template<typename AtomicInt>
      void Test_IdentDispenser_SingleThread() ;
      template<typename AtomicInt>
      void Test_IdentDispenser_MultiThread() ;

      CPPUNIT_TEST_SUITE(IdentDispenserTests) ;

      CPPUNIT_TEST(Test_IdentDispenser_SingleThread<pcomn::atomic32_t>) ;
      CPPUNIT_TEST(Test_IdentDispenser_SingleThread<pcomn::atomic64_t>) ;
      CPPUNIT_TEST(Test_IdentDispenser_SingleThread<pcomn::uatomic64_t>) ;
      CPPUNIT_TEST(Test_IdentDispenser_MultiThread<pcomn::atomic32_t>) ;
      CPPUNIT_TEST(Test_IdentDispenser_MultiThread<pcomn::atomic64_t>) ;
      CPPUNIT_TEST(Test_IdentDispenser_MultiThread<pcomn::uatomic64_t>) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;


template<typename Int>
struct TestRangeProvider  {

      typedef std::pair<Int, Int> result_type ;

      TestRangeProvider(Int from, Int step) :
         _next(from),
         _step(step)
      {}

      Int _next ;
      Int _step ;

      result_type operator()()
      {
         const Int from = _next ;
         const Int to = from + _step ;
         // Introduce a race condition
         msleep((int)(5 * (rand() / (RAND_MAX + 0.0)))) ;
         _next = to ;

         return result_type(from, to) ;
      }
} ;

template<typename I>
struct eq_diff : std::binary_function<I, I, bool> {
      eq_diff(I d) : diff(d) {}

      bool operator()(I lhs, I rhs) const { return rhs - lhs == diff ; }

      I diff ;
} ;

template<typename I, typename ForwardIterator>
static void check_dispensed(I front, ForwardIterator from, ForwardIterator to)
{
   if (from == to)
      return ;
   CPPUNIT_LOG_EQUAL(*from, front) ;
   CPPUNIT_LOG_ASSERT(std::adjacent_find(from, to, std::not2(eq_diff<I>(1))) == to) ;
}

template<typename AtomicInt, typename RangeProvider>
class IdDispenserThread : public pcomn::BasicThread {
      typedef pcomn::BasicThread ancestor ;
   public:
      typedef std::vector<AtomicInt> result_type ;
      typedef pcomn::PTIdentDispenser<AtomicInt, RangeProvider> dispenser_type ;

      IdDispenserThread(result_type &result, dispenser_type &dispenser, unsigned count) :
         _result(result),
         _dispenser(dispenser),
         _count(count)
      {}

   protected:
      int run()
      {
         unsigned seed ;

         for (unsigned cnt = _count ; cnt ; --cnt)
         {
            _result.push_back(_dispenser.allocate_id()) ;
            const unsigned r = rand_r(&seed) ;
            if ((r & 0x70) == 0x70)
               msleep(r & 2) ;
         }

         return 0 ;
      }
   private:
      result_type &     _result ;
      dispenser_type &  _dispenser ;
      const unsigned    _count ;
} ;

template<typename AtomicInt>
void IdentDispenserTests::Test_IdentDispenser_SingleThread()
{
   typedef std::vector<AtomicInt> result_type ;

   pcomn::PTIdentDispenser<AtomicInt, TestRangeProvider<int> >
      Dispenser (TestRangeProvider<int>(0, 1111)) ;

   result_type Result (1000) ;

   for (typename result_type::iterator i = Result.begin(), e = Result.end() ; i != e ; ++i)
      *i = Dispenser.allocate_id() ;

   check_dispensed((AtomicInt)0, Result.begin(), Result.end()) ;
}


template<typename AtomicInt>
void IdentDispenserTests::Test_IdentDispenser_MultiThread()
{
   typedef IdDispenserThread<AtomicInt, TestRangeProvider<int> > TestThread ;

   pcomn::PTIdentDispenser<AtomicInt, TestRangeProvider<int> >
      Dispenser (TestRangeProvider<int>(0, 509)) ;

   const size_t setsize = 32 ;
   const size_t count =
#ifdef __PCOMN_DEBUG
      10000
#else
      20000
#endif
      ;

   std::vector<AtomicInt> ResultSet[setsize] ;
   std::unique_ptr<TestThread> ThreadSet[setsize] ;

   for (unsigned t = 0 ; t < setsize ; ++t)
      ThreadSet[t].reset(new TestThread(ResultSet[t], Dispenser, count)) ;

   for (unsigned t = 0 ; t < setsize ; ++t)
      ThreadSet[t]->start() ;
   for (unsigned t = 0 ; t < setsize ; ++t)
      ThreadSet[t]->join() ;

   for (unsigned t = 0 ; t < setsize ; ++t)
      CPPUNIT_EQUAL(ResultSet[t].size(), count) ;

   for (unsigned t = 0 ; t < setsize ; ++t)
      CPPUNIT_ASSERT(std::adjacent_find(ResultSet[t].begin(), ResultSet[t].end(), std::greater_equal<AtomicInt>()) ==
                     ResultSet[t].end()) ;

   // Merge 'em
   for (unsigned sz = setsize ; sz ; sz >>= 1)
      for (unsigned t = 0 ; t < sz/2 ; ++t)
      {
         std::vector<AtomicInt> result (ResultSet[t].size() * 2) ;
         std::merge(ResultSet[t].begin(), ResultSet[t].end(),
                    ResultSet[t + sz/2].begin(), ResultSet[t + sz/2].end(),
                    result.begin()) ;

         result.swap(ResultSet[t]) ;
         std::vector<AtomicInt>().swap(ResultSet[t + sz/2]) ;
      }
   CPPUNIT_LOG_EQUAL(ResultSet[0].size(), (size_t)(setsize*count)) ;
   check_dispensed((AtomicInt)0, ResultSet[0].begin(), ResultSet[0].end()) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(IdentDispenserTests::suite()) ;

   return
      pcomn::unit::run_tests
      (runner, argc, argv, "synccomplex.diag.ini",
       "Tests of nontrivial synchronization objects (queues, producer/consumer, etc.)") ;
}
