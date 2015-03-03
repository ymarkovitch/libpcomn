/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_journal.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Jornalling tests.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   18 Dec 2008
*******************************************************************************/
#include "unittest_journal.h"
#include "test_journal.h"

#include <pcomn_journmmap.h>
#include <pcomn_string.h>
#include <pcomn_sys.h>
#include <pcomn_path.h>

#include <memory>

namespace pj = pcomn::jrn ;

typedef std::unique_ptr<JournallableStringMap> map_safeptr ;

/*******************************************************************************
                     class JournalTests
*******************************************************************************/
class JournalTests : public JournalFixture {

      void Test_Journal_Create() ;
      void Test_Journal_Create_Write() ;
      void Test_Journal_Create_RW() ;
      void Test_Journal_File_Kinds() ;
      void Test_Journal_Paths() ;
      void Test_Journal_With_Segdir() ;
      void Test_Journal_No_Segdir() ;
      void Test_Journal_Open() ;
      void Test_Journal_Open_Invalid() ;
      void Test_Journal_Open_Corrupt() ;
      void Test_Journal_Open_Segment_Corrupt() ;
      void Test_Journal_Open_Read_Write() ;
      void Test_Journal_Op_Version() ;

      CPPUNIT_TEST_SUITE(JournalTests) ;

      CPPUNIT_TEST(Test_Journal_Create) ;
      CPPUNIT_TEST(Test_Journal_Create_Write) ;
      CPPUNIT_TEST(Test_Journal_Create_RW) ;
      CPPUNIT_TEST(Test_Journal_File_Kinds) ;
      CPPUNIT_TEST(Test_Journal_Paths) ;
      CPPUNIT_TEST(Test_Journal_With_Segdir) ;
      CPPUNIT_TEST(Test_Journal_No_Segdir) ;
      CPPUNIT_TEST(Test_Journal_Open) ;
      CPPUNIT_TEST(Test_Journal_Open_Invalid) ;
      CPPUNIT_TEST(Test_Journal_Open_Corrupt) ;
      CPPUNIT_TEST(Test_Journal_Open_Segment_Corrupt) ;
      CPPUNIT_TEST(Test_Journal_Open_Read_Write) ;
      CPPUNIT_TEST(Test_Journal_Op_Version) ;

      CPPUNIT_TEST_SUITE_END() ;

      void Test_Journal_Abspaths(const std::string &journdir, const std::string &segdir) ;
      void Test_Journal_Relpaths(const std::string &journdir, const std::string &segdir) ;
      void Test_Journal_Segpaths(const std::string &journdir, unsigned flags) ;
} ;

static const pj::MMapStorage::FilenameKind NK_UNKNOWN = pj::MMapStorage::NK_UNKNOWN ;
static const pj::MMapStorage::FilenameKind NK_CHECKPOINT = pj::MMapStorage::NK_CHECKPOINT ;
static const pj::MMapStorage::FilenameKind NK_SEGDIR = pj::MMapStorage::NK_SEGDIR ;
static const pj::MMapStorage::FilenameKind NK_SEGMENT = pj::MMapStorage::NK_SEGMENT ;

void JournalTests::Test_Journal_Create()
{
   CPPUNIT_LOG_EXCEPTION(pj::Port(NULL), std::invalid_argument) ;
   std::unique_ptr<pj::MMapStorage> Storage ;

   CPPUNIT_LOG_EQUAL(ls(dataDir()), CPPUNIT_STRSET((".")(".."))) ;

   // No separate segments directory
   CPPUNIT_LOG_RUN(Storage.reset(new pj::MMapStorage(journalPath("empty1"), ""))) ;

   CPPUNIT_LOG_EQUAL(Storage->dirname(), pcomn::path::abspath<std::string>(dataDir())) ;
   CPPUNIT_LOG_EQUAL(Storage->name(), std::string("empty1")) ;

   CPPUNIT_LOG_EQUAL(ls(dataDir()), CPPUNIT_STRSET((".")("..")
                                                   (pj::MMapStorage::build_filename("empty1", NK_CHECKPOINT)))) ;

   CPPUNIT_LOG_RUN(Storage.reset()) ;

   CPPUNIT_LOG_EQUAL(ls(dataDir()), CPPUNIT_STRSET((".")(".."))) ;

   // Separate segments directory
   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(PCOMN_ENSURE_POSIX(mkdir(pcomn::str::cstr(journalPath("segments")), 0777), "mkdir")) ;
   CPPUNIT_LOG_RUN(Storage.reset(new pj::MMapStorage(journalPath("empty1"), "segments"))) ;

   CPPUNIT_LOG_EQUAL(Storage->dirname(), pcomn::path::abspath<std::string>(dataDir())) ;
   CPPUNIT_LOG_EQUAL(Storage->name(), std::string("empty1")) ;

   CPPUNIT_LOG_EQUAL(ls(dataDir()), CPPUNIT_STRSET((".")("..")
                                                   (pj::MMapStorage::build_filename("empty1", NK_CHECKPOINT))
                                                   (pj::MMapStorage::build_filename("empty1", NK_SEGDIR))
                                                   ("segments"))) ;

   CPPUNIT_LOG_EQUAL(ls(journalPath("segments")), CPPUNIT_STRSET((".")(".."))) ;

   CPPUNIT_LOG_RUN(Storage.reset()) ;

   CPPUNIT_LOG_EQUAL(ls(dataDir()), CPPUNIT_STRSET((".")("..")("segments"))) ;
}

void JournalTests::Test_Journal_Create_Write()
{
   JournallableStringMap Map ;
   CPPUNIT_LOG_EQUAL(Map.state(), pj::Journallable::ST_INITIAL) ;

   std::unique_ptr<pj::Port> PortP ;

   CPPUNIT_LOG_RUN(PortP.reset(new pj::Port(new pj::MMapStorage(journalPath("test1"), "")))) ;
   CPPUNIT_LOG_IS_NULL(Map.set_journal(PortP.get())) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(Map.size(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(Map.clear().size(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(Map.insert("Hello", "world!").size(), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(Map.insert("Bye", "baby!").size(), (size_t)2) ;
   CPPUNIT_LOG_EQUAL(Map.insert("foo", "bar").size(), (size_t)3) ;
   CPPUNIT_LOG_EQUAL(Map.insert("bar", "foobar").size(), (size_t)4) ;

   pj::generation_t CpGen ;
   CPPUNIT_LOG_ASSERT(CpGen = Map.take_checkpoint()) ;
   CPPUNIT_LOG("Generation: " << CpGen << std::endl) ;
}

void JournalTests::Test_Journal_Create_RW()
{
   std::unique_ptr<JournallableStringMap> Map (new JournallableStringMap) ;
   std::unique_ptr<pj::Port> PortP ;
   std::unique_ptr<pj::MMapStorage> St ;

   CPPUNIT_LOG_RUN(St.reset(new pj::MMapStorage(journalPath("open_rwtest"), pj::MD_RDWR, pj::OF_CREAT))) ;
   CPPUNIT_LOG_EQUAL(St->state(), pj::Storage::SST_CREATED) ;

   CPPUNIT_LOG_RUN(PortP.reset(new pj::Port(St.release()))) ;
   CPPUNIT_LOG_IS_NULL(Map->set_journal(PortP.get())) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(Map->insert("foo", "bar").insert("bar", "foobar").size(), (size_t)2) ;

   Map.reset(new JournallableStringMap) ;
   PortP.reset() ;

   CPPUNIT_LOG_RUN(PortP.reset(new pj::Port(new pj::MMapStorage(journalPath("open_rwtest"), pj::MD_RDWR)))) ;
   CPPUNIT_LOG_RUN(Map->restore_from(*PortP, true)) ;
   CPPUNIT_LOG_EQUAL(Map->data(),
                     CPPUNIT_STRMAP(std::string,
                                    (std::make_pair("foo", "bar"))
                                    (std::make_pair("bar", "foobar")))) ;

   CPPUNIT_LOG_EQUAL(Map->insert("Hello", "world!").size(), (size_t)3) ;

   CPPUNIT_LOG(std::endl) ;
   Map.reset(new JournallableStringMap) ;
   PortP.reset() ;

   CPPUNIT_LOG_RUN(St.reset(new pj::MMapStorage(journalPath("open_rwtest"), pj::MD_RDWR, pj::OF_CREAT))) ;
   CPPUNIT_LOG_EQUAL(St->state(), pj::Storage::SST_READABLE) ;
   CPPUNIT_LOG_RUN(PortP.reset(new pj::Port(St.release()))) ;
   CPPUNIT_LOG_RUN(Map->restore_from(*PortP, true)) ;
   CPPUNIT_LOG_EQUAL(Map->data(),
                     CPPUNIT_STRMAP(std::string,
                                    (std::make_pair("Hello", "world!"))
                                    (std::make_pair("foo", "bar"))
                                    (std::make_pair("bar", "foobar")))) ;

   CPPUNIT_LOG_EQUAL(Map->insert("Bye", "baby!").size(), (size_t)4) ;

   CPPUNIT_LOG(std::endl) ;
   Map.reset(new JournallableStringMap) ;
   PortP.reset() ;

   CPPUNIT_LOG_RUN(PortP.reset(new pj::Port(new pj::MMapStorage(journalPath("open_rwtest"), pj::MD_RDONLY)))) ;
   CPPUNIT_LOG_RUN(Map->restore_from(*PortP, false)) ;
   CPPUNIT_LOG_EQUAL(Map->data(),
                     CPPUNIT_STRMAP(std::string,
                                    (std::make_pair("Hello", "world!"))
                                    (std::make_pair("Bye", "baby!"))
                                    (std::make_pair("foo", "bar"))
                                    (std::make_pair("bar", "foobar")))) ;
}

void JournalTests::Test_Journal_File_Kinds()
{
   const std::string &CheckpointPath =
      journalPath(pj::MMapStorage::build_filename("opentest1", NK_CHECKPOINT)) ;
   const std::string &SeglinkPath =
      journalPath(pj::MMapStorage::build_filename("opentest1", NK_SEGDIR)) ;
   const std::string &SegPath = journalPath("segments") ;

   {
      JournallableStringMap Map ;
      CPPUNIT_LOG_EQUAL(Map.state(), pj::Journallable::ST_INITIAL) ;

      std::unique_ptr<pj::Port> PortP ;

      CPPUNIT_LOG_RUN(PCOMN_ENSURE_POSIX(mkdir(pcomn::str::cstr(SegPath), 0777), "mkdir")) ;

      CPPUNIT_LOG_RUN(PortP.reset(new pj::Port(new pj::MMapStorage(journalPath("opentest1"), "segments")))) ;
      CPPUNIT_LOG_IS_NULL(Map.set_journal(PortP.get())) ;

      CPPUNIT_LOG(std::endl) ;
      CPPUNIT_LOG_EQUAL(Map
                        .insert("Hello", "world!")
                        .insert("Bye", "baby!")
                        .insert("foo", "bar")
                        .insert("bar", "foobar")
                        .size(), (size_t)4) ;
   }

   CPPUNIT_LOG_ASSERT(!access(pcomn::str::cstr(CheckpointPath), F_OK)) ;
   CPPUNIT_LOG_ASSERT(!access(pcomn::str::cstr(SeglinkPath), F_OK)) ;
   CPPUNIT_LOG_ASSERT(!access(pcomn::str::cstr
                             (SeglinkPath + "/" +
                              pj::MMapStorage::build_filename("opentest1", NK_SEGMENT, 0)),
                              F_OK)) ;

   const std::string &Seg0Path = SegPath + "/" +
      pj::MMapStorage::build_filename("opentest1", NK_SEGMENT, 0) ;

   const std::string &Seg1Path = SegPath + "/" +
      pj::MMapStorage::build_filename("opentest1", NK_SEGMENT, 1) ;

   CPPUNIT_LOG_ASSERT(!access(pcomn::str::cstr(Seg0Path), F_OK)) ;
   CPPUNIT_LOG_ASSERT(access(pcomn::str::cstr(Seg1Path), F_OK)) ;

   // Read file headers
   CPPUNIT_LOG(std::endl) ;
   pcomn::fd_safehandle FileCP ;

   CPPUNIT_LOG_RUN(FileCP.reset(PCOMN_ENSURE_POSIX(open(pcomn::str::cstr(CheckpointPath), O_RDONLY),
                                                   "open"))) ;

   CPPUNIT_LOG_EQUAL((int)lseek(FileCP, 0, SEEK_CUR), 0) ;
   CPPUNIT_LOG_EQUAL(pj::MMapStorage::file_kind(FileCP), pj::MMapStorage::KIND_CHECKPOINT) ;
   // Ensure file offset isn't changed
   CPPUNIT_LOG_EQUAL((int)lseek(FileCP, 0, SEEK_CUR), 0) ;

   pcomn::fd_safehandle FileSEG ;
   CPPUNIT_LOG_RUN(FileSEG.reset(PCOMN_ENSURE_POSIX(open(pcomn::str::cstr(Seg0Path), O_RDONLY),
                                                    "open"))) ;
   CPPUNIT_LOG_EQUAL((int)lseek(FileSEG, 5, SEEK_SET), 5) ;
   CPPUNIT_LOG_EQUAL(pj::MMapStorage::file_kind(FileSEG), pj::MMapStorage::KIND_SEGMENT) ;
   // Ensure file offset isn't changed
   CPPUNIT_LOG_EQUAL((int)lseek(FileSEG, 0, SEEK_CUR), 5) ;
}

void JournalTests::Test_Journal_Abspaths(const std::string &journdir, const std::string &segdir)
{
   CPPUNIT_LOG("\nTesting absolute paths:" << std::endl) ;

   const std::string &CheckpointName = pj::MMapStorage::build_filename("pathtest", NK_CHECKPOINT) ;
   const std::string &SegLinkName = pj::MMapStorage::build_filename("pathtest", NK_SEGDIR) ;
   const std::string &SegmentName = pj::MMapStorage::build_filename("pathtest", NK_SEGMENT, 0) ;

   const std::string &JournalPath = journalPath(journdir + "/" + CheckpointName) ;
   const std::string &SeglinkPath = journalPath(journdir + "/" + SegLinkName) ;

   const std::string &SegPath = journalPath(segdir) ;

   CPPUNIT_LOG("Journal path: '" << JournalPath << "'\n"
               << "Seglink path: '" << SeglinkPath << "'\n"
               << "Segment path: '" << SegPath << std::endl) ;

   CPPUNIT_LOG_RUN(PCOMN_ENSURE_POSIX(mkdir(pcomn::str::cstr(journalPath(journdir)), 0777), "mkdir")) ;
   CPPUNIT_LOG_RUN(PCOMN_ENSURE_POSIX(mkdir(pcomn::str::cstr(SegPath), 0777), "mkdir")) ;

   std::unique_ptr<JournallableStringMap> Map ;
   std::unique_ptr<pj::Port> PortP ;
   CPPUNIT_LOG_RUN(PortP.reset(new pj::Port(new pj::MMapStorage(journalPath(journdir + "/pathtest"),
                                                                pcomn::path::abspath<std::string>(SegPath))))) ;
   CPPUNIT_LOG_IS_NULL((Map = map_safeptr(new JournallableStringMap))->set_journal(PortP.get())) ;
   CPPUNIT_LOG_EQUAL(Map->insert("Hello", "world!").size(), (size_t)1) ;
   Map.reset() ;
   PortP.reset() ;

   CPPUNIT_LOG_ASSERT(!access(pcomn::str::cstr(JournalPath), F_OK)) ;
   CPPUNIT_LOG_ASSERT(!access(pcomn::str::cstr(SeglinkPath), F_OK)) ;
   CPPUNIT_LOG_ASSERT(S_ISLNK(linkstat(SeglinkPath).st_mode)) ;
   CPPUNIT_LOG_EQUAL(linkdata(SeglinkPath), pcomn::path::abspath<std::string>(SegPath)) ;

   CPPUNIT_LOG_ASSERT(!access(pcomn::str::cstr(SegPath + "/" + SegmentName), F_OK)) ;

   CPPUNIT_LOG_EQUAL(ls(journalPath(journdir)), CPPUNIT_STRSET((".")("..")
                                                               (CheckpointName)
                                                               (SegLinkName))) ;

   CPPUNIT_LOG_EQUAL(ls(SegPath), CPPUNIT_STRSET((".")("..")(SegmentName))) ;
}

void JournalTests::Test_Journal_Relpaths(const std::string &journdir, const std::string &segdir)
{
   const bool NoSegDir = segdir.empty() || segdir == "." ;

   std::ostream &log = CPPUNIT_LOG("\nTesting relative paths") ;
   if (NoSegDir)
      log << " - no segments directory:" << std::endl ;
   else
      log << " with segments directory '" << segdir << "':"  << std::endl ;

   const std::string &CheckpointName = pj::MMapStorage::build_filename("pathtest", NK_CHECKPOINT) ;
   const std::string &SegLinkName = pj::MMapStorage::build_filename("pathtest", NK_SEGDIR) ;
   const std::string &SegmentName = pj::MMapStorage::build_filename("pathtest", NK_SEGMENT, 0) ;

   const std::string &JournalPath = journalPath(journdir + "/" + CheckpointName) ;
   const std::string &SeglinkPath = journalPath(journdir + "/" + SegLinkName) ;

   const std::string &SegPath = journalPath(NoSegDir ? journdir : journdir + "/" + segdir) ;

   CPPUNIT_LOG("Journal path: '" << JournalPath << "'\n"
               << "Seglink path: '" << SeglinkPath << "'\n"
               << "Segment path: '" << SegPath << std::endl) ;

   CPPUNIT_LOG_RUN(PCOMN_ENSURE_POSIX(mkdir(pcomn::str::cstr(journalPath(journdir)), 0777), "mkdir")) ;
   // Don't check mkdir result - attempts to make an already exixsting path are allowed
   CPPUNIT_LOG_RUN(mkdir(pcomn::str::cstr(SegPath), 0777)) ;

   std::set<std::string> JournalPathContents (ls(journalPath(journdir))) ;

   std::unique_ptr<JournallableStringMap> Map ;
   std::unique_ptr<pj::Port> PortP ;
   CPPUNIT_LOG_RUN(PortP.reset(new pj::Port(new pj::MMapStorage(journalPath(journdir + "/pathtest"), segdir)))) ;
   CPPUNIT_LOG_IS_NULL((Map = map_safeptr(new JournallableStringMap))->set_journal(PortP.get())) ;
   CPPUNIT_LOG_EQUAL(Map->insert("Hello", "world!").size(), (size_t)1) ;
   Map.reset() ;
   PortP.reset() ;

   CPPUNIT_LOG_ASSERT(!access(pcomn::str::cstr(JournalPath), F_OK)) ;

   if (NoSegDir)
   {
      // No seglink
      CPPUNIT_LOG_NOT_EQUAL(access(pcomn::str::cstr(SeglinkPath), F_OK), 0) ;
      // The segment is in the same directory as the checkpoint
      CPPUNIT_LOG_EQUAL(access(pcomn::str::cstr(journalPath(journdir + "/" + SegmentName)), F_OK), 0) ;

      CPPUNIT_LOG_EQUAL(ls(journalPath(journdir)), CPPUNIT_STRSET((".")("..")
                                                                  (CheckpointName)
                                                                  (SegmentName))) ;
   }
   else
   {
      // There is a seglink in the journal directory
      CPPUNIT_LOG_EQUAL(access(pcomn::str::cstr(SeglinkPath), F_OK), 0) ;
      CPPUNIT_LOG_ASSERT(S_ISLNK(linkstat(SeglinkPath).st_mode)) ;
      CPPUNIT_LOG_EQUAL(linkdata(SeglinkPath), segdir) ;

      JournalPathContents.insert(CheckpointName) ;
      JournalPathContents.insert(SegLinkName) ;

      CPPUNIT_LOG_EQUAL(ls(journalPath(journdir)), JournalPathContents) ;

      CPPUNIT_LOG_EQUAL(ls(SegPath),
                        CPPUNIT_STRSET((".")("..")(SegmentName))) ;
   }
}

void JournalTests::Test_Journal_Segpaths(const std::string &journdir, unsigned flags)
{
   const bool NoSegDir = !!(flags & pj::MMapStorage::OF_NOSEGDIR) ;

   CPPUNIT_LOG("\nTesting journal segments paths" << (NoSegDir ? " - no " : " with ")
               << "segments directory" << std::endl) ;

   const std::string &CheckpointName = pj::MMapStorage::build_filename("segtest", NK_CHECKPOINT) ;
   const std::string &SegLinkName = pj::MMapStorage::build_filename("segtest", NK_SEGDIR) ;
   const std::string &SegmentName = pj::MMapStorage::build_filename("segtest", NK_SEGMENT, 1) ;

   const std::string &JournalPath = journalPath(journdir + "/" + CheckpointName) ;
   const std::string &SeglinkPath = journalPath(journdir + "/" + SegLinkName) ;

   const std::string &SegPath = journalPath(journdir + "/segments") ;

   CPPUNIT_LOG("Journal path: '" << JournalPath << "'\n"
               << "Seglink path: '" << SeglinkPath << "'\n"
               << "Segment path: '" << SegPath << std::endl) ;

   CPPUNIT_LOG_RUN(PCOMN_ENSURE_POSIX(mkdir(pcomn::str::cstr(journalPath(journdir)), 0777), "mkdir")) ;
   // Don't check mkdir result - attempts to make an already exixsting path are allowed
   CPPUNIT_LOG_RUN(mkdir(pcomn::str::cstr(SegPath), 0777)) ;

   std::set<std::string> JournalPathContents (ls(journalPath(journdir))) ;

   CPPUNIT_LOG_EQUAL(JournalPathContents, CPPUNIT_STRSET((".")("..")("segments"))) ;

   std::unique_ptr<JournallableStringMap> Map ;
   std::unique_ptr<pj::Port> PortP ;

   CPPUNIT_LOG_RUN(PortP.reset(new pj::Port(new pj::MMapStorage
                                            (journalPath(journdir + "/segtest"), "segments", flags)))) ;

   CPPUNIT_LOG_IS_NULL((Map = map_safeptr(new JournallableStringMap))->set_journal(PortP.get())) ;
   CPPUNIT_LOG_EQUAL(Map->insert("Hello", "world!").size(), (size_t)1) ;
   CPPUNIT_LOG_RUN(Map->take_checkpoint()) ;
   CPPUNIT_LOG_EQUAL(Map->insert("Bye", "baby!").size(), (size_t)2) ;
   Map.reset() ;
   PortP.reset() ;

   CPPUNIT_LOG_ASSERT(!access(pcomn::str::cstr(JournalPath), F_OK)) ;

   CPPUNIT_LOG(std::endl) ;

   if (NoSegDir)
   {
      // No seglink
      // The segment is in the same directory as the checkpoint
      CPPUNIT_LOG_EQUAL(ls(journalPath(journdir)),
                        CPPUNIT_STRSET((".")("..")("segments")(CheckpointName)(SegmentName))) ;
      CPPUNIT_LOG_EQUAL(ls(SegPath),
                        CPPUNIT_STRSET((".")(".."))) ;
   }
   else
   {
      CPPUNIT_LOG_EQUAL(ls(journalPath(journdir)),
                        CPPUNIT_STRSET((".")("..")("segments")(CheckpointName)(SegLinkName))) ;
      CPPUNIT_LOG_EQUAL(ls(SegPath),
                        CPPUNIT_STRSET((".")("..")(SegmentName))) ;
      CPPUNIT_LOG_EQUAL(ls(SeglinkPath),
                        CPPUNIT_STRSET((".")("..")(SegmentName))) ;
   }

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(PortP.reset(new pj::Port(new pj::MMapStorage(journalPath(journdir + "/segtest"), pj::MD_RDONLY)))) ;
   CPPUNIT_LOG_RUN((Map = map_safeptr(new JournallableStringMap))->restore_from(*PortP, false)) ;

   CPPUNIT_LOG_EQUAL(Map->insert("Hello", "world!").insert("Bye", "baby!").size(), (size_t)2) ;

   CPPUNIT_LOG(std::endl) ;
   Map.reset() ;
   PortP.reset() ;

   CPPUNIT_LOG_RUN
      (PortP.reset(new pj::Port(new pj::MMapStorage(journalPath(journdir + "/segtest"),
                                                    pj::MD_RDONLY, pj::MMapStorage::OF_NOSEGDIR)))) ;

   CPPUNIT_LOG_RUN((Map = map_safeptr(new JournallableStringMap))->restore_from(*PortP, false)) ;

   if (NoSegDir)
      CPPUNIT_LOG_EQUAL(Map->insert("Hello", "world!").insert("Bye", "baby!").size(), (size_t)2) ;
   else
      // We cannot find the segment, since it is in the segments directory
      CPPUNIT_LOG_EQUAL(Map->insert("Hello", "world!").size(), (size_t)1) ;
}

void JournalTests::Test_Journal_Paths()
{
   Test_Journal_Relpaths("reltest1", "segments") ;
   Test_Journal_Relpaths("reltest2", "") ;
   Test_Journal_Relpaths("reltest3", ".") ;

   Test_Journal_Abspaths("abstest1", "segments") ;

   CPPUNIT_LOG_RUN(PCOMN_ENSURE_POSIX(mkdir(pcomn::str::cstr(journalPath("abstest2.segdir")), 0777), "mkdir")) ;
   Test_Journal_Abspaths("abstest2", "abstest2.segdir/segments") ;
}


void JournalTests::Test_Journal_With_Segdir()
{
   Test_Journal_Segpaths("segtest_with_segdir", 0) ;
}

void JournalTests::Test_Journal_No_Segdir()
{
   Test_Journal_Segpaths("segtest_no_segdir", pj::MMapStorage::OF_NOSEGDIR) ;
}

void JournalTests::Test_Journal_Open()
{
   const std::string &JournalPath = journalPath("opentest1") ;
   const std::string &SegPath = journalPath("segments") ;

   {
      JournallableStringMap Map ;
      std::unique_ptr<pj::Port> PortP ;

      PCOMN_ENSURE_POSIX(mkdir(pcomn::str::cstr(SegPath), 0777), "mkdir") ;

      CPPUNIT_LOG_RUN(PortP.reset(new pj::Port(new pj::MMapStorage(journalPath("opentest1"), "segments")))) ;
      CPPUNIT_LOG_IS_NULL(Map.set_journal(PortP.get())) ;

      CPPUNIT_LOG(std::endl) ;
      CPPUNIT_LOG_EQUAL(Map
                        .insert("Hello", "world!")
                        .insert("Bye", "baby!")
                        .insert("foo", "bar")
                        .insert("bar", "foobar")
                        .size(), (size_t)4) ;
   }

   std::unique_ptr<pj::Port> PortP ;
   // Open journal for reading
   CPPUNIT_LOG_RUN(PortP.reset(new pj::Port(new pj::MMapStorage(JournalPath, pj::MD_RDONLY)))) ;

   JournallableStringMap RestoredMap ;

   CPPUNIT_LOG_EQUAL(RestoredMap.state(), pj::Journallable::ST_INITIAL) ;
   CPPUNIT_LOG_RUN(RestoredMap.restore_from(*PortP, false)) ;
   CPPUNIT_LOG_EQUAL(RestoredMap.state(), pj::Journallable::ST_RESTORED) ;
   CPPUNIT_LOG_IS_NULL(RestoredMap.journal()) ;
   CPPUNIT_LOG_EQUAL(RestoredMap.size(), (size_t)4) ;

   CPPUNIT_LOG_EQUAL(RestoredMap.data(),
                     CPPUNIT_STRMAP(std::string,
                                    (std::make_pair("Hello", "world!"))
                                    (std::make_pair("Bye", "baby!"))
                                    (std::make_pair("foo", "bar"))
                                    (std::make_pair("bar", "foobar")))) ;

   // Should fail - there is no writable journal
   CPPUNIT_LOG_EXCEPTION(RestoredMap.take_checkpoint(), pj::state_error) ;
}

void JournalTests::Test_Journal_Open_Invalid()
{
}

void JournalTests::Test_Journal_Open_Corrupt()
{
}

void JournalTests::Test_Journal_Open_Segment_Corrupt()
{
}

void JournalTests::Test_Journal_Open_Read_Write()
{
   const std::string &JournalPath = journalPath("opentest2") ;
   const std::string &SegPath = journalPath("opentest2.segments") ;
   pj::generation_t PrevGeneration = 0 ;
   pj::generation_t LastGeneration ;

   {
      JournallableStringMap Map ;
      std::unique_ptr<pj::Port> PortP ;

      PCOMN_ENSURE_POSIX(mkdir(pcomn::str::cstr(SegPath), 0777), "mkdir") ;

      CPPUNIT_LOG_RUN(PortP.reset(new pj::Port(new pj::MMapStorage(JournalPath, "opentest2.segments")))) ;
      CPPUNIT_LOG_IS_NULL(Map.set_journal(PortP.get())) ;
      CPPUNIT_LOG(std::endl) ;

      CPPUNIT_LOG_EQUAL(Map
                        .insert("Hello", "world!")
                        .insert("Bye", "baby!")
                        .insert("foo", "bar")
                        .insert("bar", "foobar")
                        .size(), (size_t)4) ;

      CPPUNIT_LOG_RUN(LastGeneration = Map.take_checkpoint()) ;
      CPPUNIT_LOG("LastGeneration=" << LastGeneration << std::endl) ;
      CPPUNIT_LOG_ASSERT(LastGeneration > 32) ;
      LastGeneration = PrevGeneration ;

      CPPUNIT_LOG_EQUAL(Map
                        .erase("foo")
                        .insert("restaurant", "at")
                        .insert("the", "end")
                        .size(), (size_t)5) ;
   }

   std::unique_ptr<pj::Port> PortP ;
   // Open journal for reading
   CPPUNIT_LOG_RUN(PortP.reset(new pj::Port(new pj::MMapStorage(JournalPath, pj::MD_RDONLY)))) ;

   JournallableStringMap RestoredMap ;

   CPPUNIT_LOG_RUN(RestoredMap.restore_from(*PortP, false)) ;
   CPPUNIT_LOG_IS_NULL(RestoredMap.journal()) ;
   CPPUNIT_LOG_EQUAL(RestoredMap.size(), (size_t)5) ;

   CPPUNIT_LOG_EQUAL(RestoredMap.data(),
                     CPPUNIT_STRMAP(std::string,
                                    (std::make_pair("Hello", "world!"))
                                    (std::make_pair("Bye", "baby!"))
                                    (std::make_pair("restaurant", "at"))
                                    (std::make_pair("the", "end"))
                                    (std::make_pair("bar", "foobar")))) ;
}

void JournalTests::Test_Journal_Op_Version()
{
   const std::string &JournalPath = journalPath("opvertest") ;
   const std::string &SegPath = journalPath("opvertest.segments") ;

   {
      JournallableStringMap Map ;
      std::unique_ptr<pj::Port> PortP ;

      PCOMN_ENSURE_POSIX(mkdir(pcomn::str::cstr(SegPath), 0777), "mkdir") ;

      CPPUNIT_LOG_RUN(PortP.reset(new pj::Port(new pj::MMapStorage(JournalPath, "opvertest.segments")))) ;
      CPPUNIT_LOG_IS_NULL(Map.set_journal(PortP.get())) ;
      CPPUNIT_LOG(std::endl) ;

      CPPUNIT_LOG_EQUAL(Map
                        .insert("World", "hello!")
                        .insert("Hello")
                        .size(), (size_t)2) ;

      CPPUNIT_LOG_EQUAL(Map.data(),
                        CPPUNIT_STRMAP(std::string,
                                       (std::make_pair("Hello", "HELLO-HELLO"))
                                       (std::make_pair("World", "hello!"))
                           )) ;
   }

   std::unique_ptr<pj::Port> PortP ;
   // Open journal for reading
   CPPUNIT_LOG_RUN(PortP.reset(new pj::Port(new pj::MMapStorage(JournalPath, pj::MD_RDONLY)))) ;

   JournallableStringMap RestoredMap ;

   CPPUNIT_LOG_RUN(RestoredMap.restore_from(*PortP, false)) ;
   CPPUNIT_LOG_IS_NULL(RestoredMap.journal()) ;
   CPPUNIT_LOG_EQUAL(RestoredMap.size(), (size_t)2) ;

   CPPUNIT_LOG_EQUAL(RestoredMap.data(),
                     CPPUNIT_STRMAP(std::string,
                                    (std::make_pair("Hello", "HELLO-HELLO"))
                                    (std::make_pair("World", "hello!"))
                        )) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(JournalTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "unittest.journal.diag.ini", "Journalling tests") ;
}
