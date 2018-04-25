/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_timespec.cpp
 COPYRIGHT    :   Yakov Markovitch, 2011-2017. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests of time point and time interval classes.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   7 Apr 2011
*******************************************************************************/
#include <pcomn_timespec.h>
#include <pcomn_unittest.h>

class TimePointTests : public CppUnit::TestFixture {

      void Test_TimePoint_Init() ;
      void Test_TimePoint_String() ;

      CPPUNIT_TEST_SUITE(TimePointTests) ;

      CPPUNIT_TEST(Test_TimePoint_Init) ;
      CPPUNIT_TEST(Test_TimePoint_String) ;

      CPPUNIT_TEST_SUITE_END() ;

   public:
      void setUp()
      {
         tz = getenv("TZ") ;
         setenv("TZ", "Europe/Moscow", 1) ;
         tzset() ;
      }
      void tearDown()
      {
         if (tz)
            setenv("TZ", tz, 1) ;
         else
            unsetenv("TZ") ;
         tzset() ;
      }

   private:
      const char *tz ;
} ;

using namespace pcomn ;

/*******************************************************************************
 TimePointTests
*******************************************************************************/
void TimePointTests::Test_TimePoint_Init()
{
   static const time_t mt = 1302131463 ;

   CPPUNIT_LOG_EQUAL(time_point::from_time(mt).as_time(), mt) ;
   CPPUNIT_LOG_EQUAL(time_point(mt*pcomn::Ts).as_time(), mt) ;
   CPPUNIT_LOG_EQUAL(time_point(mt*pcomn::Ts), time_point::from_time(mt)) ;

   CPPUNIT_LOG(std::endl) ;

   struct tm tm ;
   CPPUNIT_LOG_RUN(tm = time_point(mt*pcomn::Ts).as_tm(time_point::LOCAL)) ;
   CPPUNIT_LOG_EQUAL(tm.tm_year, 111) ;
   CPPUNIT_LOG_EQUAL(tm.tm_mon, 3) ;
   // Local Moscow time
   CPPUNIT_LOG_EQUAL(tm.tm_mday, 7) ;
   CPPUNIT_LOG_EQUAL(tm.tm_hour, 3) ;
   CPPUNIT_LOG_EQUAL(tm.tm_min, 11) ;
   CPPUNIT_LOG_EQUAL(tm.tm_sec, 3) ;

   CPPUNIT_LOG_RUN(tm = time_point(mt*pcomn::Ts).as_tm(time_point::GMT)) ;
   CPPUNIT_LOG_EQUAL(tm.tm_year, 111) ;
   CPPUNIT_LOG_EQUAL(tm.tm_mon, 3) ;
   CPPUNIT_LOG_EQUAL(tm.tm_mday, 6) ;
   CPPUNIT_LOG_EQUAL(tm.tm_hour, 23) ;
   CPPUNIT_LOG_EQUAL(tm.tm_min, 11) ;
   CPPUNIT_LOG_EQUAL(tm.tm_sec, 3) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(tm = time_point(time_point::GMT, 2011, 3, 6, 23, 11, 3).as_tm(time_point::GMT)) ;
   CPPUNIT_LOG_EQUAL(tm.tm_year, 111) ;
   CPPUNIT_LOG_EQUAL(tm.tm_mon, 3) ;
   CPPUNIT_LOG_EQUAL(tm.tm_mday, 6) ;
   CPPUNIT_LOG_EQUAL(tm.tm_hour, 23) ;
   CPPUNIT_LOG_EQUAL(tm.tm_min, 11) ;
   CPPUNIT_LOG_EQUAL(tm.tm_sec, 3) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(tm = time_point(time_point::LOCAL, 2011, 3, 6, 23, 11, 3).as_tm(time_point::LOCAL)) ;
   CPPUNIT_LOG_EQUAL(tm.tm_year, 111) ;
   CPPUNIT_LOG_EQUAL(tm.tm_mon, 3) ;
   CPPUNIT_LOG_EQUAL(tm.tm_mday, 6) ;
   CPPUNIT_LOG_EQUAL(tm.tm_hour, 23) ;
   CPPUNIT_LOG_EQUAL(tm.tm_min, 11) ;
   CPPUNIT_LOG_EQUAL(tm.tm_sec, 3) ;
   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_EQUAL(time_point(time_point::GMT, 2011, 3, 6, 23, 11, 3).as_useconds(), mt*pcomn::Ts) ;
   CPPUNIT_LOG_EQUAL(time_point(time_point::LOCAL, 2011, 3, 6, 23, 11, 3).as_useconds(), mt*pcomn::Ts - 4*pcomn::Thr) ;
}

void TimePointTests::Test_TimePoint_String()
{
   // 2011-04-06 23:11:03 GMT
   // 2011-04-07 02:11:03 MSK
   const time_point &tp = time_point::from_time(1302131463) ;

   CPPUNIT_LOG_EQUAL(tp.string(time_point::GMT), std::string("2011-04-06 23:11:03.000")) ;
   CPPUNIT_LOG_EQUAL(tp.string(time_point::LOCAL), std::string("2011-04-07 03:11:03.000")) ;
   CPPUNIT_LOG_EQUAL(tp.http_string(), std::string("Wed, 06 Apr 2011 23:11:03 GMT")) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(TimePointTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "unittest.diag.ini", "Tests of time point and time interval classes") ;
}
