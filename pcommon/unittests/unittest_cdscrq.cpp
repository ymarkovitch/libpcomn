/*-*- tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_cdscrq.cpp
 COPYRIGHT    :   Yakov Markovitch, 2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for CRQ and LCRQ queues.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   7 Aug 2016
*******************************************************************************/
#include <pcomn_cdscrq.h>
#include <pcomn_iterator.h>
#include <pcomn_unittest.h>

#include "pcomn_testcds.h"

#include <thread>
#include <numeric>

using namespace pcomn ;

#if defined(_NDEBUG) || defined(__OPTIMIZE__)
static const size_t PER_PRODUCER = 3000000 ;
#else
static const size_t PER_PRODUCER =  300000 ;
#endif

namespace CppUnit {
template<>
struct assertion_traits<pcomn::crqslot_tag> {

      static bool equal(const pcomn::crqslot_tag &x, const pcomn::crqslot_tag &y)
      {
         return x._tag == y._tag ;
      }

      static std::string toString(const pcomn::crqslot_tag &v)
      {
         pcomn::omemstream os ;
         os << '(' << (v.is_safe() ? 's' : 'u') << ',' << (v.is_empty() ? '_' : '*') << ',' << v.ndx() << ')' ;
         return os.checkout() ;
      }
} ;
} // end of namespace CppUnit

/*******************************************************************************
 class CRQTests
*******************************************************************************/
class CRQTests : public CppUnit::TestFixture {

   public:
      typedef std::unique_ptr<std::string> string_ptr ;

      typedef crq<int>         int_crq ;
      typedef crq<string_ptr>  string_crq ;

   private:
      void Test_CRQ_Data() ;
      void Test_CRQ_Init() ;
      void Test_CRQ_SingleThread() ;

      template<size_t producers_count, size_t consumers_count, size_t per_producer_items>
      void Test_CRQ_NxN()
      {
         std::unique_ptr<int_crq> icrq {int_crq::make_crq(0)} ;
         unit::TantrumQueueTest(*icrq, producers_count, consumers_count, per_producer_items) ;
      }

      CPPUNIT_TEST_SUITE(CRQTests) ;

      CPPUNIT_TEST(Test_CRQ_Data) ;
      CPPUNIT_TEST(Test_CRQ_Init) ;
      CPPUNIT_TEST(Test_CRQ_SingleThread) ;

      CPPUNIT_TEST(P_PASS(Test_CRQ_NxN<1, 1, 1>)) ;
      CPPUNIT_TEST(P_PASS(Test_CRQ_NxN<1, 1, 32>)) ;
      CPPUNIT_TEST(P_PASS(Test_CRQ_NxN<1, 1, 60>)) ;
      CPPUNIT_TEST(P_PASS(Test_CRQ_NxN<1, 1, 61>)) ;
      CPPUNIT_TEST(P_PASS(Test_CRQ_NxN<1, 1, 62>)) ;
      CPPUNIT_TEST(P_PASS(Test_CRQ_NxN<1, 1, PER_PRODUCER>)) ;

      CPPUNIT_TEST(P_PASS(Test_CRQ_NxN<2, 1, 1>)) ;
      CPPUNIT_TEST(P_PASS(Test_CRQ_NxN<2, 1, 32>)) ;
      CPPUNIT_TEST(P_PASS(Test_CRQ_NxN<2, 1, 60>)) ;
      CPPUNIT_TEST(P_PASS(Test_CRQ_NxN<2, 1, 61>)) ;
      CPPUNIT_TEST(P_PASS(Test_CRQ_NxN<2, 1, 62>)) ;
      CPPUNIT_TEST(P_PASS(Test_CRQ_NxN<2, 1, PER_PRODUCER>)) ;

      CPPUNIT_TEST(P_PASS(Test_CRQ_NxN<2, 2, 1>)) ;
      CPPUNIT_TEST(P_PASS(Test_CRQ_NxN<2, 2, 32>)) ;
      CPPUNIT_TEST(P_PASS(Test_CRQ_NxN<2, 2, 60>)) ;
      CPPUNIT_TEST(P_PASS(Test_CRQ_NxN<2, 2, 61>)) ;
      CPPUNIT_TEST(P_PASS(Test_CRQ_NxN<2, 2, 62>)) ;
      CPPUNIT_TEST(P_PASS(Test_CRQ_NxN<2, 2, PER_PRODUCER>)) ;

      CPPUNIT_TEST(P_PASS(Test_CRQ_NxN<1, 2, 1>)) ;
      CPPUNIT_TEST(P_PASS(Test_CRQ_NxN<1, 2, 32>)) ;
      CPPUNIT_TEST(P_PASS(Test_CRQ_NxN<1, 2, 60>)) ;
      CPPUNIT_TEST(P_PASS(Test_CRQ_NxN<1, 2, 61>)) ;
      CPPUNIT_TEST(P_PASS(Test_CRQ_NxN<1, 2, 62>)) ;
      CPPUNIT_TEST(P_PASS(Test_CRQ_NxN<1, 2, PER_PRODUCER>)) ;

      CPPUNIT_TEST(P_PASS(Test_CRQ_NxN<2, 3, PER_PRODUCER>)) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

struct uint_5 {
      constexpr uint_5(unsigned val = 5) : value(val) {}
      constexpr operator unsigned() const { return value ; }
      unsigned value ;
} ;

/*******************************************************************************
 CRQTests
*******************************************************************************/
void CRQTests::Test_CRQ_Data()
{
   crqslot_tag tag_empty ;
   CPPUNIT_LOG_EQ(tag_empty.ndx(), 0) ;
   CPPUNIT_LOG_ASSERT(tag_empty.is_empty()) ;
   CPPUNIT_LOG_ASSERT(tag_empty.is_safe()) ;
   CPPUNIT_LOG_EQ(tag_empty.set_ndx(5).ndx(), 5) ;

   CPPUNIT_LOG_IS_FALSE(tag_empty.test_and_set(crqslot_tag::value_bit_pos, std::memory_order_relaxed)) ;
   CPPUNIT_LOG_IS_FALSE(tag_empty.is_empty()) ;
   CPPUNIT_LOG_ASSERT(tag_empty.is_safe()) ;

   CPPUNIT_LOG_ASSERT(tag_empty.test_and_set(crqslot_tag::value_bit_pos, std::memory_order_relaxed)) ;
   CPPUNIT_LOG_IS_FALSE(tag_empty.is_empty()) ;
   CPPUNIT_LOG_ASSERT(tag_empty.is_safe()) ;

   CPPUNIT_LOG_EQ(tag_empty.ndx(), 5) ;
   CPPUNIT_LOG_EQ(tag_empty.set_ndx(0xFFFFFFFFFFFF).ndx(), 0xFFFFFFFFFFFF) ;
   CPPUNIT_LOG_IS_FALSE(tag_empty.is_empty()) ;
   CPPUNIT_LOG_ASSERT(tag_empty.is_safe()) ;

   CPPUNIT_LOG(std::endl) ;

   crqslot_tag empty_tag0 ;
   CPPUNIT_LOG_EQ(empty_tag0.set_ndx(0xFFFFFFFFFFFF).ndx(), 0xFFFFFFFFFFFF) ;
   CPPUNIT_LOG_IS_FALSE(empty_tag0.test_and_set(crqslot_tag::unsafe_bit_pos, std::memory_order_relaxed)) ;

   crqslot_data data_empty {empty_tag0} ;
   CPPUNIT_LOG_EQ(data_empty.ndx(), 0xFFFFFFFFFFFF) ;
   CPPUNIT_LOG_ASSERT(data_empty.is_empty()) ;
   CPPUNIT_LOG_IS_FALSE(empty_tag0.is_safe()) ;
   CPPUNIT_LOG_IS_FALSE(data_empty.is_safe()) ;

   typedef crq_slot<uint_5> crq_slot_5 ;

   crq_slot_5 slot_empty ;
   CPPUNIT_LOG_EQ(slot_empty.ndx(), 0) ;
   CPPUNIT_LOG_ASSERT(slot_empty.is_empty()) ;
   CPPUNIT_LOG_ASSERT(slot_empty.is_safe()) ;
   CPPUNIT_LOG_EQ(slot_empty.value(), 5) ;
}

void CRQTests::Test_CRQ_Init()
{
   std::unique_ptr<int_crq>    i_crq0 ;
   std::unique_ptr<string_crq> s_crq0 ;

   CPPUNIT_LOG_RUN(i_crq0.reset(int_crq::make_crq(0, 1))) ;
   CPPUNIT_LOG_RUN(s_crq0.reset(string_crq::make_crq(0))) ;

   CPPUNIT_LOG_ASSERT(i_crq0->empty()) ;
   CPPUNIT_LOG_ASSERT(s_crq0->empty()) ;

   CPPUNIT_LOG_EQ(i_crq0->memsize(), sys::pagesize()) ;
   CPPUNIT_LOG_EQ(s_crq0->memsize(), sys::pagesize()) ;

   CPPUNIT_LOG_EQ(i_crq0->capacity(), sys::pagesize()/PCOMN_CACHELINE_SIZE - 3) ;
   CPPUNIT_LOG_EQ(s_crq0->capacity(), sys::pagesize()/PCOMN_CACHELINE_SIZE - 3) ;
   CPPUNIT_LOG_EQ(s_crq0->modulo(), sys::pagesize()/PCOMN_CACHELINE_SIZE) ;
   CPPUNIT_LOG_EQ(s_crq0->initndx(), 0) ;

   CPPUNIT_LOG_EQ(i_crq0->head_fetch_and_next(), 0) ;
   CPPUNIT_LOG_EQ(i_crq0->head_fetch_and_next(), 1) ;
   for (uintptr_t i = 1 ; ++i < i_crq0->capacity() - 1 ; )
      CPPUNIT_EQUAL(i_crq0->head_fetch_and_next(), i) ;

   CPPUNIT_LOG_EQ(i_crq0->head_fetch_and_next(), i_crq0->capacity() - 1) ;
   CPPUNIT_LOG_EQ(i_crq0->pos(i_crq0->capacity() - 2), i_crq0->capacity() - 2) ;
   CPPUNIT_LOG_EQ(i_crq0->pos(i_crq0->capacity() - 1), i_crq0->capacity() - 1) ;

   CPPUNIT_LOG_EQ(i_crq0->head_fetch_and_next(), i_crq0->initndx() + i_crq0->modulo()) ;
   CPPUNIT_LOG_EQ(i_crq0->pos(i_crq0->initndx() + i_crq0->modulo()), 0) ;


   CPPUNIT_LOG(std::endl) ;
   std::unique_ptr<int_crq> i_crq1 ;

   CPPUNIT_LOG_RUN(i_crq1.reset(int_crq::make_crq(779))) ;
   CPPUNIT_LOG_ASSERT(i_crq1->empty()) ;
   CPPUNIT_LOG_EQ(i_crq1->memsize(), sys::pagesize()) ;

   CPPUNIT_LOG_EQ(i_crq1->initndx(), 779) ;

   CPPUNIT_LOG_EQ(i_crq1->head_fetch_and_next(), 779) ;
   CPPUNIT_LOG_EQ(i_crq1->head_fetch_and_next(), 780) ;
   for (uintptr_t i = 780 ; ++i < i_crq1->initndx() + i_crq1->capacity() ; )
      CPPUNIT_EQUAL(i_crq1->head_fetch_and_next(), i) ;

   CPPUNIT_LOG_EQ(i_crq1->head_fetch_and_next(), i_crq1->initndx() + i_crq1->modulo()) ;
   CPPUNIT_LOG_EQ(i_crq1->pos(i_crq1->initndx() + i_crq1->modulo()), 0) ;

   CPPUNIT_LOG(std::endl) ;

   std::unique_ptr<int_crq> i_crq2 ;
   CPPUNIT_LOG_RUN(i_crq2.reset(int_crq::make_crq(779))) ;
   CPPUNIT_LOG_ASSERT(i_crq2->empty()) ;
   CPPUNIT_LOG_EQ(i_crq2->memsize(), sys::pagesize()) ;

   CPPUNIT_LOG_EQ(i_crq2->initndx(), 779) ;

   CPPUNIT_LOG_EQ(i_crq2->tail_fetch_and_next(), crqslot_tag(779)) ;
   CPPUNIT_LOG_EQ(i_crq2->tail_fetch_and_next(), crqslot_tag(780)) ;
   for (uintptr_t i = 780 ; ++i < i_crq2->initndx() + i_crq2->capacity() ; )
      CPPUNIT_EQUAL(i_crq2->tail_fetch_and_next(), crqslot_tag(i)) ;

   CPPUNIT_LOG_EQ(i_crq2->tail_fetch_and_next(), crqslot_tag(i_crq2->initndx() + i_crq2->modulo())) ;
}

void CRQTests::Test_CRQ_SingleThread()
{
   std::unique_ptr<int_crq> i_crq0 {int_crq::make_crq(0)} ;
   using std::make_pair ;

   CPPUNIT_LOG_ASSERT(i_crq0->empty()) ;
   CPPUNIT_LOG_ASSERT(i_crq0->enqueue(100)) ;
   CPPUNIT_LOG_IS_FALSE(i_crq0->empty()) ;
   CPPUNIT_LOG_EQ(i_crq0->dequeue(), make_pair(100, true)) ;
   CPPUNIT_LOG_ASSERT(i_crq0->empty()) ;
   CPPUNIT_LOG_EQ(i_crq0->dequeue(), make_pair(0, false)) ;
   CPPUNIT_LOG_ASSERT(i_crq0->empty()) ;
   CPPUNIT_LOG_EQ(i_crq0->dequeue(), make_pair(0, false)) ;
   CPPUNIT_LOG_ASSERT(i_crq0->enqueue(200)) ;
   CPPUNIT_LOG_EQ(i_crq0->dequeue(), make_pair(200, true)) ;

   CPPUNIT_LOG_ASSERT(i_crq0->enqueue(300)) ;
   CPPUNIT_LOG_ASSERT(i_crq0->enqueue(400)) ;
   CPPUNIT_LOG_EQ(i_crq0->dequeue(), make_pair(300, true)) ;
   CPPUNIT_LOG_ASSERT(i_crq0->enqueue(500)) ;
   CPPUNIT_LOG_EQ(i_crq0->dequeue(), make_pair(400, true)) ;
   CPPUNIT_LOG_EQ(i_crq0->dequeue(), make_pair(500, true)) ;
   CPPUNIT_LOG_EQ(i_crq0->dequeue(), make_pair(0, false)) ;

   CPPUNIT_LOG(std::endl) ;
   size_t count ;
   size_t success = 0 ;
   for (count = 0 ; count++ < i_crq0->capacity() ;)
      CPPUNIT_EQUAL((success += i_crq0->enqueue(count * 10)), count) ;

   CPPUNIT_LOG_IS_FALSE(i_crq0->enqueue(count * 10)) ;
   CPPUNIT_LOG_IS_FALSE(i_crq0->enqueue(count * 10)) ;

   for (count = 0 ; count++ < i_crq0->capacity() ;)
      CPPUNIT_EQUAL(i_crq0->dequeue(), make_pair((int)count * 10, true)) ;

   CPPUNIT_LOG_EQ(i_crq0->dequeue(), make_pair(0, false)) ;
   CPPUNIT_LOG_IS_FALSE(i_crq0->enqueue(count * 10)) ;
   CPPUNIT_LOG_EQ(i_crq0->dequeue(), make_pair(0, false)) ;

   CPPUNIT_LOG(std::endl) ;

   std::unique_ptr<int_crq> i_crq1 {int_crq::make_crq(0)} ;

   for (count = 0 ; count++ < i_crq1->capacity() * 2 ;)
      CPPUNIT_EQUAL(i_crq1->dequeue(), make_pair(0, false)) ;

   CPPUNIT_LOG_ASSERT(i_crq1->enqueue(777)) ;
   CPPUNIT_LOG_EQ(i_crq1->dequeue(), make_pair(777, true)) ;
   CPPUNIT_LOG_EQ(i_crq1->dequeue(), make_pair(0, false)) ;
}

/*******************************************************************************

*******************************************************************************/
int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(CRQTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv, nullptr, "Tests of CRQ and LCRQ") ;
}
