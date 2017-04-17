/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_sys.cpp
 COPYRIGHT    :   Yakov Markovitch, 2009-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for routines from pcomn::sys namespace

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   24 Jan 2009
*******************************************************************************/
#include <pcomn_unittest.h>
#include <pcomn_string.h>
#include <pcomn_except.h>
#include <pcomn_iterator.h>
#include <pcomn_handle.h>
#include <pcomn_sys.h>

#include "pcomn_testhelpers.h"

#include <iostream>

using namespace pcomn ;

/*******************************************************************************
                     class SysDirTests
*******************************************************************************/
extern const char SYSDIR_FIXTURE[] = "sysdir" ;
class SysDirTests : public pcomn::unit::TestFixture<SYSDIR_FIXTURE> {

      void Test_Opendir() ;

      CPPUNIT_TEST_SUITE(SysDirTests) ;

      CPPUNIT_TEST(Test_Opendir) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

void SysDirTests::Test_Opendir()
{
   using namespace pcomn ;
   std::vector<std::string> content ;

   CPPUNIT_LOG_ASSERT((intptr_t)sys::opendir(str::cstr(dataDir()), sys::ODIR_CLOSE_DIR, appender(content), RAISE_ERROR) >= 0) ;
   CPPUNIT_LOG_EQUAL(CPPUNIT_SORTED(content), CPPUNIT_STRVECTOR((".")(".."))) ;
   swap_clear(content) ;

   /****************************************************************************
    Linux-only (open a directory as a file descriptor)
   ****************************************************************************/
   #if defined(PCOMN_PL_LINUX)
   CPPUNIT_LOG_ASSERT(sys::opendirfd(str::cstr(dataDir() + "/foo"), sys::ODIR_CLOSE_DIR, appender(content), DONT_RAISE_ERROR) < 0) ;
   CPPUNIT_LOG_EQUAL(content, CPPUNIT_STRVECTOR()) ;
   CPPUNIT_LOG_EXCEPTION(sys::opendirfd(str::cstr(dataDir() + "/foo"), sys::ODIR_CLOSE_DIR, appender(content), RAISE_ERROR), system_error) ;
   CPPUNIT_LOG_EQUAL(content, CPPUNIT_STRVECTOR()) ;
   // Test defaults: must be DONT_RAISE_ERROR
   CPPUNIT_LOG_ASSERT(sys::opendirfd(str::cstr(dataDir() + "/foo"), sys::ODIR_CLOSE_DIR, appender(content)) < 0) ;
   CPPUNIT_LOG_EQUAL(content, CPPUNIT_STRVECTOR()) ;
   CPPUNIT_LOG_ASSERT(sys::opendirfd(str::cstr(dataDir() + "/foo"), 0, appender(content)) < 0) ;
   CPPUNIT_LOG_EQUAL(content, CPPUNIT_STRVECTOR()) ;
   #endif

   /****************************************************************************
    Both linux and windows
   ****************************************************************************/
   CPPUNIT_LOG_IS_NULL(sys::opendir(str::cstr(dataDir() + "/foo"), sys::ODIR_CLOSE_DIR, appender(content), DONT_RAISE_ERROR)) ;
   CPPUNIT_LOG_EQUAL(content, CPPUNIT_STRVECTOR()) ;
   CPPUNIT_LOG_EXCEPTION(sys::opendir(str::cstr(dataDir() + "/foo"), sys::ODIR_CLOSE_DIR, appender(content), RAISE_ERROR), system_error) ;
   CPPUNIT_LOG_EQUAL(content, CPPUNIT_STRVECTOR()) ;
   // Test defaults: must be DONT_RAISE_ERROR
   CPPUNIT_LOG_IS_NULL(sys::opendir(str::cstr(dataDir() + "/foo"), sys::ODIR_CLOSE_DIR, appender(content))) ;
   CPPUNIT_LOG_EQUAL(content, CPPUNIT_STRVECTOR()) ;
   CPPUNIT_LOG_IS_NULL(sys::opendir(str::cstr(dataDir() + "/foo"), 0, appender(content))) ;
   CPPUNIT_LOG_EQUAL(content, CPPUNIT_STRVECTOR()) ;

   CPPUNIT_LOG(std::endl) ;
   unit::generate_seqn_file<4>(dataDir() + "/bar", 1) ;
   unit::generate_seqn_file<4>(dataDir() + "/quux", 2) ;

   dir_safehandle dh ;
   CPPUNIT_LOG_RUN(dh.reset(sys::opendir(str::cstr(dataDir()), 0, appender(content), RAISE_ERROR))) ;
   CPPUNIT_LOG_EQUAL(CPPUNIT_SORTED(content), CPPUNIT_STRVECTOR((".")("..")("bar")("quux"))) ;
   swap_clear(content) ;

   // No fstatat() on Windows
   #if ! defined(PCOMN_PL_WINDOWS)
   {
      fd_safehandle dfd ;
      CPPUNIT_LOG_RUN(dfd.reset(sys::opendirfd(str::cstr(dataDir()), 0, appender(content), RAISE_ERROR))) ;
      CPPUNIT_LOG_EQUAL(CPPUNIT_SORTED(content), CPPUNIT_STRVECTOR((".")("..")("bar")("quux"))) ;
      swap_clear(content) ;
      CPPUNIT_LOG_ASSERT(dfd.handle() >= 0) ;
      CPPUNIT_LOG_EQUAL(sys::filesize(dfd.handle(), "bar"), (fileoff_t)4) ;
      CPPUNIT_LOG_EQUAL(sys::filesize(dfd.handle(), "quux"), (fileoff_t)8) ;
   }
   #endif

   swap_clear(content) ;
   CPPUNIT_LOG_EQUAL(CPPUNIT_SORTED(sys::ls(str::cstr(dataDir()), 0, appender(content), RAISE_ERROR).container()),
                     CPPUNIT_STRVECTOR((".")("..")("bar")("quux"))) ;
   swap_clear(content) ;
   CPPUNIT_LOG_EQUAL(CPPUNIT_SORTED(sys::ls(str::cstr(dataDir()), sys::ODIR_SKIP_DOT, appender(content), RAISE_ERROR).container()),
                     CPPUNIT_STRVECTOR(("..")("bar")("quux"))) ;
   swap_clear(content) ;
   CPPUNIT_LOG_EQUAL(CPPUNIT_SORTED(sys::ls(str::cstr(dataDir()), sys::ODIR_SKIP_DOTDOT, appender(content), RAISE_ERROR).container()),
                     CPPUNIT_STRVECTOR((".")("bar")("quux"))) ;
   swap_clear(content) ;
   CPPUNIT_LOG_EQUAL(CPPUNIT_SORTED(sys::ls(str::cstr(dataDir()), sys::ODIR_SKIP_DOTS, appender(content), RAISE_ERROR).container()),
                     CPPUNIT_STRVECTOR(("bar")("quux"))) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(SysDirTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "unittest.diag.ini", "pcomn::sys tests") ;
}
