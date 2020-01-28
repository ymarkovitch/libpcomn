/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_journal_files.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Test for pcomn::jrn::MMapStorage files.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   17 Dec 2008
*******************************************************************************/
#include "unittest_journal.h"

#include <pcomn_journal/journmmap.h>

#include <pcomn_string.h>
#include <pcomn_unittest.h>
#include <pcomn_sys.h>

#include <typeinfo>
#include <fstream>
#include <memory>

#include <stdio.h>

namespace pj = pcomn::jrn ;

#define DECLARE_STATE(st) const pj::MMapStorage::RecFile::FileState st = pj::MMapStorage::RecFile::st
DECLARE_STATE(ST_TRANSIT) ;
DECLARE_STATE(ST_CLOSED) ;
DECLARE_STATE(ST_READABLE) ;
DECLARE_STATE(ST_CREATED) ;
DECLARE_STATE(ST_WRITABLE) ;
#undef DECLARE_STATE

/*******************************************************************************
                     class JournalFilesTests
*******************************************************************************/
class JournalFilesTests : public JournalFixture {
      void Test_JournalFile_Create() ;

      CPPUNIT_TEST_SUITE(JournalFilesTests) ;

      CPPUNIT_TEST(Test_JournalFile_Create) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

void JournalFilesTests::Test_JournalFile_Create()
{
   std::unique_ptr<pj::MMapStorage::SegmentFile> Segment ;
   CPPUNIT_LOG_RUN(Segment.reset(new pj::MMapStorage::SegmentFile(dirfd(), "hello_world.seg", 0, 0, 0600))) ;
   CPPUNIT_LOG_EQUAL(Segment->state(), ST_CREATED) ;
   CPPUNIT_LOG_EQUAL(Segment->data_begin(), (fileoff_t)0) ;
   CPPUNIT_LOG_EQUAL(Segment->data_end(), (fileoff_t)0) ;
   CPPUNIT_LOG_IS_TRUE(Segment->close()) ;
   CPPUNIT_LOG_EQUAL(Segment->state(), ST_CLOSED) ;
   CPPUNIT_LOG_IS_FALSE(Segment->close()) ;

   std::unique_ptr<pj::MMapStorage::CheckpointFile> Checkpoint ;
   CPPUNIT_LOG_RUN(Checkpoint.reset(new pj::MMapStorage::CheckpointFile(dirfd(), "hello_world.cp", 0, 0, 0600))) ;
   CPPUNIT_LOG_EQUAL(Checkpoint->state(), ST_CREATED) ;
   CPPUNIT_LOG_EQUAL(Checkpoint->data_begin(), (fileoff_t)0) ;
   CPPUNIT_LOG_EQUAL(Checkpoint->data_end(), (fileoff_t)0) ;
   CPPUNIT_LOG_IS_TRUE(Checkpoint->close()) ;
   CPPUNIT_LOG_EQUAL(Checkpoint->state(), ST_CLOSED) ;
   CPPUNIT_LOG_IS_FALSE(Checkpoint->close()) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(JournalFilesTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "unittest.journal.diag.ini", "Journal storage files tests") ;
}
