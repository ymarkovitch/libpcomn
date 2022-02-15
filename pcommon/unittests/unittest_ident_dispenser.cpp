/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_ident_dispenser.cpp
 COPYRIGHT    :   Yakov Markovitch, 2009-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for ident_dispenser and local_ident_dispenser.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   22 Feb 2009
*******************************************************************************/
#include <pcomn_unittest.h>
#include <pcomn_syncobj.h>
#include <pcomn_stopwatch.h>

#include <algorithm>
#include <thread>

#include <stdlib.h>

/*******************************************************************************
                     class IdentDispenserTests
*******************************************************************************/
class IdentDispenserTests : public CppUnit::TestFixture {

   private:
      template<typename AtomicInt>
      void Test_Dispenser_SingleThread() ;
      template<typename AtomicInt>
      void Test_Dispenser_MultiThread() ;

      template<typename AtomicInt>
      void Test_LocalIdentDispenser_SingleThread() ;
      template<typename AtomicInt>
      void Test_LocalIdentDispenser_MultiThread() ;

      CPPUNIT_TEST_SUITE(IdentDispenserTests) ;

      CPPUNIT_TEST(Test_Dispenser_SingleThread<int32_t>) ;
      CPPUNIT_TEST(Test_Dispenser_SingleThread<int64_t>) ;
      CPPUNIT_TEST(Test_Dispenser_SingleThread<uint64_t>) ;
      CPPUNIT_TEST(Test_Dispenser_MultiThread<int32_t>) ;
      CPPUNIT_TEST(Test_Dispenser_MultiThread<int64_t>) ;
      CPPUNIT_TEST(Test_Dispenser_MultiThread<uint64_t>) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

/*******************************************************************************
                     class LocalIdentDispenserTests
*******************************************************************************/
class LocalIdentDispenserTests : public CppUnit::TestFixture {
   private:
      template<typename AtomicInt, int increment=1>
      void Test_LocalDispenser_SingleThread() ;
      template<typename AtomicInt, int increment=1>
      void Test_LocalDispenser_MultiThread() ;

      CPPUNIT_TEST_SUITE(LocalIdentDispenserTests) ;

      CPPUNIT_TEST(Test_LocalDispenser_SingleThread<int32_t>) ;
      CPPUNIT_TEST(Test_LocalDispenser_SingleThread<int64_t>) ;
      CPPUNIT_TEST(Test_LocalDispenser_SingleThread<P_PASS(int64_t,256)>) ;
      CPPUNIT_TEST(Test_LocalDispenser_SingleThread<P_PASS(int64_t,16)>) ;
      CPPUNIT_TEST(Test_LocalDispenser_SingleThread<uint64_t>) ;
      CPPUNIT_TEST(Test_LocalDispenser_SingleThread<P_PASS(uint64_t,2)>) ;
      CPPUNIT_TEST(Test_LocalDispenser_MultiThread<int32_t>) ;
      CPPUNIT_TEST(Test_LocalDispenser_MultiThread<int64_t>) ;
      CPPUNIT_TEST(Test_LocalDispenser_MultiThread<P_PASS(int64_t,256)>) ;
      CPPUNIT_TEST(Test_LocalDispenser_MultiThread<uint64_t>) ;
      CPPUNIT_TEST(Test_LocalDispenser_MultiThread<P_PASS(uint64_t,16)>) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

/*******************************************************************************
 Common utilities
*******************************************************************************/
template<typename I>
struct eq_diff : std::binary_function<I, I, bool> {
      eq_diff(I d) : diff(d) {}

      bool operator()(I lhs, I rhs) const { return rhs - lhs == diff ; }

      I diff ;
} ;

template<typename I, typename ForwardIterator>
static void check_dispensed(I front, ForwardIterator from, ForwardIterator to, int increment = 1)
{
   if (from == to)
      return ;
   CPPUNIT_LOG_EQUAL(*from, front) ;

   const auto bad = std::adjacent_find(from, to, [increment](auto prev, auto next)
   {
      return !(prev < next && !((next - prev) % increment)) ;
   }) ;

   if (bad != to)
   {
      auto next_to_bad = bad ;
      ++next_to_bad ;
      CPPUNIT_LOG_EXPRESSION(std::make_pair(*bad, *next_to_bad)) ;
      CPPUNIT_LOG_EXPRESSION(std::distance(from, bad)) ;
      CPPUNIT_LOG_EXPRESSION(std::distance(from, to)) ;
      CPPUNIT_LOG_EXPRESSION(*std::next(from, std::distance(from, to) - 1)) ;
   }
   CPPUNIT_LOG_ASSERT(bad == to) ;
}


/*******************************************************************************
 IdentDispenserTests
*******************************************************************************/
template<typename Int>
struct TestRangeProvider {

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
         msleep((int)(4 * (rand() / (RAND_MAX + 0.0)))) ;
         _next = to ;

         return result_type(from, to) ;
      }
} ;

template<typename AtomicInt, typename RangeProvider>
class IdDispenserThread {
   public:
      typedef std::vector<AtomicInt> result_type ;
      typedef pcomn::ident_dispenser<AtomicInt, RangeProvider> dispenser_type ;

      IdDispenserThread(result_type &result, dispenser_type &dispenser, unsigned count) :
         _result(result),
         _dispenser(dispenser),
         _count(count),
         _thread([this]{ run() ; })
      {}

      void join() { _thread.join() ; }

   protected:
      void run()
      {
         unsigned seed ;

         for (unsigned cnt = _count ; cnt ; --cnt)
         {
            _result.push_back(_dispenser.allocate_id()) ;
            const unsigned r = rand_r(&seed) ;
            if ((r & 0x70) == 0x70)
               usleep((r & 2)*100) ;
         }
      }
   private:
      result_type &     _result ;
      dispenser_type &  _dispenser ;
      const unsigned    _count ;
      std::thread       _thread ;
} ;

template<typename AtomicInt>
void IdentDispenserTests::Test_Dispenser_SingleThread()
{
   typedef std::vector<AtomicInt> result_type ;

   pcomn::ident_dispenser<AtomicInt, TestRangeProvider<int>>
      Dispenser (TestRangeProvider<int>(0, 1111)) ;

   result_type Result (1000) ;

   for (typename result_type::iterator i = Result.begin(), e = Result.end() ; i != e ; ++i)
      *i = Dispenser.allocate_id() ;

   check_dispensed((AtomicInt)0, Result.begin(), Result.end()) ;
}


template<typename AtomicInt>
void IdentDispenserTests::Test_Dispenser_MultiThread()
{
   typedef IdDispenserThread<AtomicInt, TestRangeProvider<int>> TestThread ;

   pcomn::ident_dispenser<AtomicInt, TestRangeProvider<int>>
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

/*******************************************************************************
 LocalIdentDispenserTests
*******************************************************************************/
constexpr int blocksize = 256 ;

template<typename AtomicInt, int increment>
using dispenser_st = pcomn::local_ident_dispenser<LocalIdentDispenserTests, AtomicInt, blocksize, increment> ;

template<typename AtomicInt, int increment>
class LocalIdDispenserThread {
   public:
      typedef std::vector<AtomicInt> result_type ;
      typedef pcomn::local_ident_dispenser<LocalIdDispenserThread, AtomicInt, blocksize, increment> dispenser_type ;

      LocalIdDispenserThread(result_type &result, unsigned count) :
         _result(result),
         _count(count),
         _thread([this]{ run() ; })
      {}

      void join() { _thread.join() ; }

   protected:
      void run()
      {
         unsigned seed ;

         for (unsigned cnt = _count ; cnt ; --cnt)
         {
            _result.push_back(dispenser_type::allocate_id()) ;
            const unsigned r = rand_r(&seed) ;
            if ((r & 0x70) == 0x70)
               usleep((r & 2)*50) ;
         }
      }
   private:
      result_type &     _result ;
      const unsigned    _count ;
      std::thread       _thread ;
} ;

template<typename AtomicInt, int increment>
void LocalIdentDispenserTests::Test_LocalDispenser_SingleThread()
{
   typedef std::vector<AtomicInt> result_type ;
   typedef dispenser_st<AtomicInt, increment> dispenser_type ;

   result_type Result (1000) ;
   for (auto &id: Result)
      id = dispenser_type::allocate_id() ;

   check_dispensed((AtomicInt)blocksize, Result.begin(), Result.end(), increment) ;
}

template<typename AtomicInt, int increment>
void LocalIdentDispenserTests::Test_LocalDispenser_MultiThread()
{
   typedef LocalIdDispenserThread<AtomicInt, increment> TestThread ;

   constexpr size_t setsize = 16 ;
   constexpr size_t count =
#ifdef __PCOMN_DEBUG
      10000
#else
      20000
#endif
      ;

   std::vector<AtomicInt> ResultSet[setsize] ;
   std::unique_ptr<TestThread> ThreadSet[setsize] ;

   for (unsigned t = 0 ; t < setsize ; ++t)
      ThreadSet[t].reset(new TestThread(ResultSet[t], count)) ;

   for (unsigned t = 0 ; t < setsize ; ++t)
      ThreadSet[t]->join() ;

   for (unsigned t = 0 ; t < setsize ; ++t)
      CPPUNIT_EQUAL(ResultSet[t].size(), count) ;

   for (unsigned t = 0 ; t < setsize ; ++t)
   {
      const auto bad = std::adjacent_find(ResultSet[t].begin(), ResultSet[t].end(), [](auto prev, auto next)
      {
         return !(prev + increment <= next && !((next - prev) % increment)) ;
      }) ;
      if (bad != ResultSet[t].end())
      {
         CPPUNIT_LOG_EXPRESSION(std::make_pair(*bad, *(bad+1))) ;
      }
      CPPUNIT_ASSERT(bad == ResultSet[t].end()) ;
   }

   // Merge 'em
   std::vector<AtomicInt> result ;
   result.reserve(setsize*count) ;
   for (const auto &rv: ResultSet)
   {
      result.insert(result.end(), rv.begin(), rv.end()) ;
   }

   CPPUNIT_LOG_EQUAL(result.size(), setsize*count) ;
   CPPUNIT_LOG_RUN(std::sort(result.begin(), result.end())) ;

   check_dispensed((AtomicInt)blocksize, result.begin(), result.end(), increment) ;
}

/*******************************************************************************
 main
*******************************************************************************/
int main(int argc, char *argv[])
{
   return pcomn::unit::run_tests
       <
          IdentDispenserTests,
          LocalIdentDispenserTests
       >
       (argc, argv) ;
}
