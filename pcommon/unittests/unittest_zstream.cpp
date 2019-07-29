/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_rawstream.cpp
 COPYRIGHT    :   Yakov Markovitch, 2007-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Test for raw zstreams.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   9 Oct 2011
*******************************************************************************/
#include <pcomn_zstream.h>
#include <pcomn_string.h>
#include <pcomn_unittest.h>

#include "pcomn_testhelpers.h"

#include <stdio.h>
#include <typeinfo>
#include <fstream>

using namespace pcomn ;
typedef raw_ios::pos_type pos_type ;

extern const char ZSTREAMTESTS[] = "zstream" ;

/*******************************************************************************
                     class ZStreamTests
*******************************************************************************/
class ZStreamTests : public unit::TestFixture<ZSTREAMTESTS> {

   private:
      void Test_RawZStream() ;

      CPPUNIT_TEST_SUITE(ZStreamTests) ;

      CPPUNIT_TEST(Test_RawZStream) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

template<typename Stream>
static bool next_eof(Stream &s) { char d ; return s.eof() || s.read(&d, 1).eof() ; }

void ZStreamTests::Test_RawZStream()
{
   std::ifstream t0 (str::cstr(CPPUNIT_AT_TESTDIR("zstream.testdata.0.gz"))) ;
   CPPUNIT_LOG_ASSERT(t0) ;
   std::ifstream t30000 (str::cstr(CPPUNIT_AT_TESTDIR("zstream.testdata.30000.gz"))) ;
   CPPUNIT_LOG_ASSERT(t30000) ;

   raw_izstream t0z (t0) ;
   CPPUNIT_LOG_ASSERT(t0z) ;
   CPPUNIT_LOG_RUN(t0z.exceptions(raw_ios::badbit)) ;

   raw_izstream t30000z (t30000) ;
   CPPUNIT_LOG_ASSERT(t30000z) ;
   CPPUNIT_LOG_RUN(t30000z.exceptions(raw_ios::badbit)) ;

   unit::checked_read_seqn<8>(t0z, 0, 0) ;
   CPPUNIT_LOG_ASSERT(next_eof(t0z)) ;
   unit::checked_read_seqn<8>(t30000z, 0, 30000) ;
   CPPUNIT_LOG_ASSERT(next_eof(t30000z)) ;

   {
      std::ofstream ot0 (str::cstr(at_data_dir("zstream.testout.0.gz"))) ;
      CPPUNIT_LOG_ASSERT(ot0) ;
      raw_ozstream ot0z (ot0) ;
      CPPUNIT_LOG_ASSERT(ot0z) ;

      std::ofstream ot20000 (str::cstr(at_data_dir("zstream.testdata.20000.gz"))) ;
      CPPUNIT_LOG_ASSERT(ot20000) ;
      raw_ozstream ot20000z (ot20000) ;
      CPPUNIT_LOG_ASSERT(ot20000z) ;

      unit::generate_seqn<8>(ot20000z, 0, 20000) ;
      CPPUNIT_LOG_ASSERT(ot20000z) ;
   }
   CPPUNIT_LOG_EQUAL(system(str::cstr("gzip -d " + at_data_dir("zstream.testout.0.gz"))), 0) ;
   std::ifstream t00 (str::cstr(at_data_dir("zstream.testout.0"))) ;
   CPPUNIT_LOG_ASSERT(t00) ;
   CPPUNIT_LOG_ASSERT(next_eof(t00)) ;

   CPPUNIT_LOG_EQUAL(system(str::cstr("gzip -d " + at_data_dir("zstream.testdata.20000.gz"))), 0) ;
   std::ifstream t20000 (str::cstr(at_data_dir("zstream.testdata.20000"))) ;
   CPPUNIT_LOG_ASSERT(t20000) ;
   unit::checked_read_seqn<8>(t20000, 0, 20000) ;
   CPPUNIT_LOG_ASSERT(next_eof(t20000)) ;
}

int main(int argc, char *argv[])
{
   unit::TestRunner runner ;
   runner.addTest(ZStreamTests::suite()) ;

   return
      unit::run_tests(runner, argc, argv, "unittest_zstream.diag.ini", "Raw zstream tests") ;
}
