/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_mmap.cpp
 COPYRIGHT    :   Yakov Markovitch, 2007-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Test for memory-mapping classes.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   15 Aug 2007
*******************************************************************************/
#include <pcomn_mmap.h>
#include <pcomn_handle.h>
#include <pcomn_rawstream.h>
#include <pcomn_unittest.h>
#include <pcomn_sys.h>

#include "pcomn_testhelpers.h"

#include <memory>

#include <sys/stat.h>

typedef pcomn::raw_ios::pos_type pos_type ;

static size_t file_size(int fd)
{
   return pcomn::ensure_ge<pcomn::system_error>(pcomn::sys::filesize(fd), 0) ;
}

/*******************************************************************************
                     class MMapTests
*******************************************************************************/
class MMapTests : public CppUnit::TestFixture {

   private:
      void Test_MemMapFile() ;
      void Test_MemMapFileRead() ;
      void Test_MemMapFileWrite() ;
      void Test_MemMapFileReadWrite() ;
      void Test_MemMapEmptyFile() ;

      CPPUNIT_TEST_SUITE(MMapTests) ;

      CPPUNIT_TEST(Test_MemMapFile) ;
      CPPUNIT_TEST(Test_MemMapFileRead) ;
      CPPUNIT_TEST(Test_MemMapFileWrite) ;
      CPPUNIT_TEST(Test_MemMapFileReadWrite) ;
      CPPUNIT_TEST(Test_MemMapEmptyFile) ;

      CPPUNIT_TEST_SUITE_END() ;

      static bool Cleanup_Stream(const char *filename) ;
      static void Create_Stream(const char *filename, unsigned to)
      {
         pcomn::raw_ofstream os (filename) ;
         CPPUNIT_LOG_RUN(pcomn::unit::generate_sequence(os, 0, to)) ;
      }
      static std::string Create_Str(int from, int to)
      {
         std::string result ;
         while(from < to)
            result += pcomn::strprintf("%*d", pcomn::unit::DWIDTH, from++) ;
         return result ;
      }
} ;

bool MMapTests::Cleanup_Stream(const char *filename)
{
   return (access(filename, 0) && errno == ENOENT) || !unlink(filename) ;
}

void MMapTests::Test_MemMapFile()
{
   const char *name ;
   CPPUNIT_LOG_ASSERT(Cleanup_Stream(name = "MMapTests.Test_MemMapFile.lst")) ;
   Create_Stream(name, 11000) ;

   std::unique_ptr<pcomn::PMemMappedFile> MMFile ;
   pcomn::fd_safehandle fd ;

   CPPUNIT_LOG_RUN(fd = pcomn::ensure_ge<pcomn::system_error>(open(name, O_RDONLY), 0)) ;
   CPPUNIT_LOG_RUN(MMFile.reset(new pcomn::PMemMappedFile(fd))) ;
   CPPUNIT_LOG_EQ(MMFile->filemode(), O_RDONLY) ;
   CPPUNIT_LOG_ASSERT(fd != MMFile->handle()) ;
   CPPUNIT_LOG_ASSERT(MMFile->handle() > 0) ;
   CPPUNIT_LOG_RUN(fd = 0) ;
   CPPUNIT_LOG_RUN(MMFile = NULL) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EXCEPTION(MMFile.reset(new pcomn::PMemMappedFile(-1)), pcomn::environment_error) ;
   CPPUNIT_LOG_RUN(fd = pcomn::ensure_ge<pcomn::system_error>(open(name, O_RDONLY), 0)) ;
   CPPUNIT_LOG_EQ(file_size(fd), 66000) ;
   CPPUNIT_LOG_RUN(MMFile.reset(new pcomn::PMemMappedFile(fd, 6000, O_RDONLY))) ;
   CPPUNIT_LOG_EQ(MMFile->requested_size(), 6000) ;
   CPPUNIT_LOG_EQ(file_size(fd), 66000) ;
}

void MMapTests::Test_MemMapFileRead()
{
   const char *name ;
   CPPUNIT_LOG_ASSERT(Cleanup_Stream(name = "MMapTests.Test_MemMapFileRead.lst")) ;
   Create_Stream(name, 11000) ;

   std::unique_ptr<pcomn::PMemMappedFile> MMFile ;

   {
      pcomn::fd_safehandle fd ;
      CPPUNIT_LOG_RUN(fd = pcomn::ensure_ge<pcomn::system_error>(open(name, O_RDONLY), 0)) ;
      CPPUNIT_LOG_RUN(MMFile.reset(new pcomn::PMemMappedFile(fd))) ;
      const size_t fsize = pcomn::sys::filesize(fd) ;
      CPPUNIT_LOG_ASSERT(fd.close()) ;
      pcomn::PMemMapping MemMapping (*MMFile) ;
      CPPUNIT_LOG_RUN(MMFile = NULL) ;
      CPPUNIT_LOG_EQUAL(MemMapping.size(), fsize) ;
      CPPUNIT_LOG_EQUAL(std::string(MemMapping.cdata(), 12), Create_Str(0, 2)) ;
   }

   {
      pcomn::PMemMapping MemMapping (name, 10, 12006) ;
      CPPUNIT_LOG_ASSERT(MemMapping) ;
      CPPUNIT_LOG_EQUAL(MemMapping.size(), (size_t)11996) ;

      CPPUNIT_LOG_EQUAL(std::string(MemMapping.cdata() + 2, MemMapping.cdata() + 62),
                        Create_Str(2, 12)) ;
      CPPUNIT_LOG_EQUAL(std::string(MemMapping.cdata() + 11990, MemMapping.cdata() + 11996),
                        Create_Str(2000, 2001)) ;
   }

   CPPUNIT_LOG_ASSERT(std::string(pcomn::PMemMapping(name).cdata(), 66000) == Create_Str(0, 11000)) ;
}

void MMapTests::Test_MemMapFileWrite()
{
   const char *name ;
   CPPUNIT_LOG_ASSERT(Cleanup_Stream(name = "MMapTests.Test_MemMapFileWrite.lst")) ;

   std::unique_ptr<pcomn::PMemMappedFile> MMFile ;

   {
      pcomn::fd_safehandle fd ;
      CPPUNIT_LOG_RUN(fd = pcomn::ensure_ge<pcomn::system_error>(open(name, O_WRONLY|O_CREAT, S_IREAD|S_IWRITE), 0)) ;
      CPPUNIT_LOG_EQUAL(file_size(fd), (size_t)0) ;
      CPPUNIT_LOG_RUN(MMFile.reset(new pcomn::PMemMappedFile(fd))) ;
      CPPUNIT_LOG_EQUAL(MMFile->requested_size(), (size_t)-1) ;
      CPPUNIT_LOG_RUN(MMFile.reset()) ;
      CPPUNIT_LOG_EQUAL(file_size(fd), (size_t)0) ;
      CPPUNIT_LOG_ASSERT(fd.close()) ;

      CPPUNIT_LOG(std::endl) ;
      CPPUNIT_LOG_RUN(fd = pcomn::ensure_ge<pcomn::system_error>(open(name, O_RDWR|O_CREAT, S_IREAD|S_IWRITE), 0)) ;
      CPPUNIT_LOG_EQUAL(file_size(fd), (size_t)0) ;
      CPPUNIT_LOG_RUN(MMFile.reset(new pcomn::PMemMappedFile(fd, 17, O_WRONLY))) ;
      CPPUNIT_LOG_EQUAL(file_size(fd), (size_t)17) ;
      {
         pcomn::PMemMapping MemMapping (*MMFile, O_WRONLY) ;
         CPPUNIT_LOG_RUN(MMFile = NULL) ;
         CPPUNIT_LOG_RUN(memcpy(MemMapping.cdata() + 2, Create_Str(0, 1).c_str(), 6)) ;
         CPPUNIT_LOG_RUN(memcpy(MemMapping.cdata() + 9, Create_Str(1, 2).c_str(), 6)) ;
      }
      CPPUNIT_LOG_EQUAL(file_size(fd), (size_t)17) ;
      CPPUNIT_LOG_RUN(MMFile = NULL) ;
   }
   pcomn::raw_ifstream TestFile (name) ;
   char Buf[128] = "111111111111111111" ;
   CPPUNIT_LOG_ASSERT(TestFile.read(Buf, 2)) ;
   CPPUNIT_LOG_EQUAL(TestFile.last_read(), (size_t)2) ;
   CPPUNIT_LOG_EQUAL((int)Buf[0], 0) ;
   CPPUNIT_LOG_EQUAL((int)Buf[1], 0) ;
   pcomn::unit::checked_read_sequence(TestFile, 0, 1) ;
   Buf[0] = Buf[1] = '1' ;
   CPPUNIT_LOG_EQUAL(TestFile.read(Buf, 1).last_read(), (size_t)1) ;
   CPPUNIT_LOG_EQUAL((int)Buf[0], 0) ;
   Buf[0] = Buf[1] = '1' ;
   pcomn::unit::checked_read_sequence(TestFile, 1, 2) ;
   CPPUNIT_LOG_ASSERT(TestFile.read(Buf, 2)) ;
   CPPUNIT_LOG_EQUAL(TestFile.last_read(), (size_t)2) ;
   CPPUNIT_LOG_EQUAL((int)Buf[0], 0) ;
   CPPUNIT_LOG_EQUAL((int)Buf[1], 0) ;
   CPPUNIT_LOG_ASSERT(TestFile.read(Buf, 1).eof()) ;
}

void MMapTests::Test_MemMapFileReadWrite()
{
   const char *name ;
   CPPUNIT_LOG_ASSERT(Cleanup_Stream(name = "MMapTests.Test_MemMapFileReadWrite.lst")) ;

   std::unique_ptr<pcomn::PMemMapping> Mapping ;
   pcomn::fd_safehandle fd ;

   CPPUNIT_LOG_RUN(fd = pcomn::ensure_ge<pcomn::system_error>(open(name, O_RDWR|O_CREAT, S_IREAD|S_IWRITE), 0)) ;
   CPPUNIT_LOG_EQUAL(file_size(fd), (size_t)0) ;
   CPPUNIT_LOG_RUN(Mapping.reset(new pcomn::PMemMapping(fd, O_RDWR))) ;
   CPPUNIT_LOG_EQUAL(Mapping->requested_size(), (size_t)-1) ;
   CPPUNIT_LOG_EQUAL(Mapping->size(), (size_t)0) ;
   CPPUNIT_LOG_IS_NULL(*Mapping) ;
   CPPUNIT_LOG_RUN(Mapping.reset()) ;
   CPPUNIT_LOG_EQUAL(file_size(fd), (size_t)0) ;
   CPPUNIT_LOG_ASSERT(fd.close()) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(fd = pcomn::ensure_ge<pcomn::system_error>(open(name, O_RDWR|O_CREAT, S_IREAD|S_IWRITE), 0)) ;
   CPPUNIT_LOG_EQUAL(file_size(fd), (size_t)0) ;
   CPPUNIT_LOG_RUN(Mapping.reset(new pcomn::PMemMapping(fd, 37, 54, O_RDWR))) ;
   CPPUNIT_LOG_EQUAL(file_size(fd), (size_t)54) ;
   CPPUNIT_LOG_EQ(write(fd, Create_Str(10, 18).c_str(), 48), 48) ;
   CPPUNIT_LOG_RUN(memcpy(Mapping->cdata() + 11, Create_Str(3, 4).c_str(), 6)) ;
   CPPUNIT_LOG_EQ(file_size(fd), 54) ;

   CPPUNIT_LOG(std::endl) ;
   pcomn::safe_handle<pcomn::FILE_handle_tag> TestFile ;
   char Buf[128] ;
   CPPUNIT_LOG(memset(Buf, '1',  sizeof Buf)) ;
   CPPUNIT_LOG_ASSERT(TestFile = fopen(name, "r")) ;
   CPPUNIT_LOG_EQUAL(std::string(fgets(Buf, 49, TestFile)), Create_Str(10, 18)) ;
   CPPUNIT_LOG_EQUAL(std::string(fgets(Buf + 48, 7, TestFile)), Create_Str(3, 4)) ;
   CPPUNIT_LOG_EQUAL(std::string(Mapping->cdata() + 5, Mapping->cdata() + 17), Create_Str(17, 18) + Create_Str(3, 4)) ;
 }

void MMapTests::Test_MemMapEmptyFile()
{
   CPPUNIT_LOG_RUN(pcomn::PMemMapping(CPPUNIT_AT_TESTDIR("unittest.1byte.lst").c_str())) ;
   // Creating a memory mapping over a non-regular file (here "/dev/null" or "NUL") is an error.
   CPPUNIT_LOG_EXCEPTION(pcomn::PMemMapping(PCOMN_NULL_FILE_NAME, 0, 1),
                         pcomn::environment_error) ;
   // But, as it is zero-length, zero-length zero-offset mapping is allowed
   CPPUNIT_LOG_IS_NULL(pcomn::PMemMapping(PCOMN_NULL_FILE_NAME)) ;

   // Creating a mapping over a zero-length regular file is OK and resulting mapping
   // pointer is NULL
   CPPUNIT_LOG_IS_NULL(pcomn::PMemMapping(CPPUNIT_AT_TESTDIR("unittest.empty.lst").c_str())) ;
   CPPUNIT_LOG_IS_NULL(pcomn::PMemMapping(CPPUNIT_AT_TESTDIR("unittest.empty.lst").c_str(), 1, -1)) ;
   CPPUNIT_LOG_IS_NULL(pcomn::PMemMapping(CPPUNIT_AT_TESTDIR("unittest.1byte.lst").c_str(), 0, 0)) ;
   CPPUNIT_LOG_IS_NULL(pcomn::PMemMapping(CPPUNIT_AT_TESTDIR("unittest.1byte.lst").c_str(), 1, 0)) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(MMapTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv, NULL, "Raw memory-mapped files tests") ;
}
