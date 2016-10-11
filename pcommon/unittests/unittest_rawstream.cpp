/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_rawstream.cpp
 COPYRIGHT    :   Yakov Markovitch, 2007-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Test for raw streams.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   2 Aug 2007
*******************************************************************************/
#include <pcomn_rawstream.h>
#include <pcomn_string.h>
#include <pcomn_unittest.h>

#include "pcomn_testhelpers.h"

using namespace pcomn::unit ;

/*******************************************************************************
                     class RawStreamTests
*******************************************************************************/
class RawStreamTests : public CppUnit::TestFixture {

   private:
      void Test_StdStream() ;
      void Test_StdStream_Exceptions() ;
      void Test_FStream() ;
      void Test_FStream_Exceptions() ;
      void Test_MemStream() ;
      void Test_CacheStream() ;
      void Test_CacheStream_Eof() ;

      CPPUNIT_TEST_SUITE(RawStreamTests) ;

      CPPUNIT_TEST(Test_StdStream) ;
      CPPUNIT_TEST(Test_StdStream_Exceptions) ;
      CPPUNIT_TEST(Test_FStream) ;
      CPPUNIT_TEST(Test_FStream_Exceptions) ;
      CPPUNIT_TEST(Test_MemStream) ;
      CPPUNIT_TEST(Test_CacheStream) ;
      CPPUNIT_TEST(Test_CacheStream_Eof) ;

      CPPUNIT_TEST_SUITE_END() ;

      void Check_Write_Stream(pcomn::raw_ostream &stream) ;
      void Check_Read_Stream(pcomn::raw_istream &stream) ;
      static bool Cleanup_Stream(const char *filename) ;
} ;

typedef pcomn::raw_ios::pos_type pos_type ;

void RawStreamTests::Check_Write_Stream(pcomn::raw_ostream &)
{
}

void RawStreamTests::Check_Read_Stream(pcomn::raw_istream &)
{
}

bool RawStreamTests::Cleanup_Stream(const char *filename)
{
   return access(filename, 0) || !unlink(filename) ;
}

void RawStreamTests::Test_StdStream()
{
   const char *name ;
   CPPUNIT_LOG_ASSERT(Cleanup_Stream(name = "RawStreamTests.Test_StdStream.lst")) ;

   CPPUNIT_LOG(std::endl << "Creating raw_ostream" << std::endl) ;
   pcomn::raw_stdostream os (new std::ofstream(name), true) ;

   CPPUNIT_LOG_IS_TRUE(os.stream().good()) ;
   CPPUNIT_LOG_IS_TRUE(os.good()) ;
   CPPUNIT_LOG_EQUAL(os.tell(), (pos_type)0) ;
   CPPUNIT_LOG_RUN(pcomn::unit::generate_sequence(os, 0, 2000)) ;
   CPPUNIT_LOG_EQUAL(os.tell(), (pos_type)12000) ;
   CPPUNIT_LOG_RUN(pcomn::unit::generate_sequence(os.stream(), 2000, 6000)) ;
   CPPUNIT_LOG_RUN(pcomn::unit::generate_sequence(os, 6000, 6001)) ;
   CPPUNIT_LOG_EQUAL(os.tell(), (pos_type)36006) ;
   CPPUNIT_LOG_RUN(pcomn::unit::generate_sequence(os.stream(), 6001, 6002)) ;
   CPPUNIT_LOG_RUN(pcomn::unit::generate_sequence(os.stream(), 6002, 10000)) ;
   CPPUNIT_LOG_EQUAL(os.tell(), (pos_type)60000) ;
   CPPUNIT_LOG_RUN(os.close()) ;
   CPPUNIT_LOG_IS_FALSE(os.is_open()) ;
   CPPUNIT_LOG_IS_FALSE(os.write(name, 1)) ;

   CPPUNIT_LOG(std::endl << "Creating raw_istream" << std::endl) ;
   pcomn::raw_stdistream is (new std::ifstream(name), true) ;
   CPPUNIT_LOG_IS_TRUE(is.stream().good()) ;
   CPPUNIT_LOG_IS_TRUE(is.good()) ;
   CPPUNIT_LOG_EQUAL(is.tell(), (pos_type)0) ;

   CPPUNIT_LOG(std::endl) ;
   pcomn::unit::checked_read_sequence(is.stream(), 0, 1) ;
   CPPUNIT_LOG_ASSERT(is.good()) ;
   CPPUNIT_LOG_ASSERT(is.stream().good()) ;
   CPPUNIT_LOG_ASSERT(!is.eof()) ;
   CPPUNIT_LOG_ASSERT(!is.stream().eof()) ;
   pcomn::unit::checked_read_sequence(is, 1, 100) ;
   CPPUNIT_LOG_EQUAL(is.seek(1200, pcomn::raw_ios::beg), (pos_type)1200) ;
   CPPUNIT_LOG_ASSERT(!is.eof()) ;
   CPPUNIT_LOG_EQUAL(is.tell(), (pos_type)1200) ;
   pcomn::unit::checked_read_sequence(is.stream(), 200, 4000) ;
   CPPUNIT_LOG_EQUAL(is.tell(), (pos_type)24000) ;
   CPPUNIT_LOG_EQUAL(is.seek(-6000, pcomn::raw_ios::cur), (pos_type)18000) ;
   CPPUNIT_LOG_EQUAL(is.tell(), (pos_type)18000) ;
   pcomn::unit::checked_read_sequence(is, 3000, 10000) ;
   CPPUNIT_LOG_IS_FALSE(is.eof()) ;
   CPPUNIT_LOG_IS_TRUE(is.good()) ;

   CPPUNIT_LOG(std::endl) ;
   // Testing the end-of-file condition
   char buf ;
   CPPUNIT_LOG_IS_TRUE(is.read(&buf, 1).eof()) ;
   CPPUNIT_LOG_IS_FALSE(is) ;
}

void RawStreamTests::Test_StdStream_Exceptions()
{
   CPPUNIT_LOG_ASSERT(!access(PCOMN_NULL_FILE_NAME, 0)) ;

   pcomn::raw_stdistream is ;
   char buf[7] = {0, 0, 0, 0, 0, 0, 0} ;

   CPPUNIT_LOG_RUN(is.owns(true)) ;
   CPPUNIT_LOG_ASSERT(is.open(new std::ifstream(PCOMN_NULL_FILE_NAME)).is_open()) ;
   CPPUNIT_LOG_RUN(is.exceptions(pcomn::raw_ios::eofbit)) ;
   CPPUNIT_LOG_EQUAL(is.exceptions(), (unsigned)pcomn::raw_ios::eofbit) ;
   CPPUNIT_LOG_EXCEPTION_CODE(is.read(buf, sizeof buf - 1), pcomn::failure_exception, pcomn::raw_ios::eofbit) ;
   CPPUNIT_LOG_IS_FALSE(is.bad()) ;
   CPPUNIT_LOG_IS_TRUE(is.eof()) ;
   CPPUNIT_LOG_IS_FALSE(is.good()) ;
   CPPUNIT_LOG_IS_TRUE(is.fail()) ;
   CPPUNIT_LOG_EQUAL(is.last_read(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(is.rdstate(), (unsigned)(pcomn::raw_ios::eofbit | pcomn::raw_ios::failbit)) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(is.open(new std::ifstream(PCOMN_NULL_FILE_NAME)).is_open()) ;
   CPPUNIT_LOG_RUN(is.exceptions(pcomn::raw_ios::failbit)) ;
   CPPUNIT_LOG_EQUAL(is.exceptions(), (unsigned)(pcomn::raw_ios::failbit)) ;
   CPPUNIT_LOG_EXCEPTION_CODE(is.read(buf, sizeof buf - 1), pcomn::failure_exception, pcomn::raw_ios::failbit) ;
   CPPUNIT_LOG_EQUAL(is.last_read(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(is.rdstate(), (unsigned)(pcomn::raw_ios::failbit|pcomn::raw_ios::eofbit)) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(is.open(new std::ifstream(PCOMN_NULL_FILE_NAME)).is_open()) ;
   CPPUNIT_LOG_RUN(is.exceptions(pcomn::raw_ios::eofbit)) ;
   CPPUNIT_LOG_RUN(is.stream().exceptions(std::ios_base::failbit)) ;
   CPPUNIT_LOG_EQUAL(is.exceptions(), (unsigned)(pcomn::raw_ios::eofbit)) ;

   #if _GLIBCXX_USE_CXX11_ABI && PCOMN_WORKAROUND(__GNUC_VER__, < 700)
   CPPUNIT_LOG_FAILURE(is.read(buf, sizeof buf - 1)) ;
   #else
   CPPUNIT_LOG_EXCEPTION(is.read(buf, sizeof buf - 1), std::ios_base::failure) ;
   #endif

   CPPUNIT_LOG_EQUAL(is.last_read(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(is.rdstate(), (unsigned)(pcomn::raw_ios::failbit|pcomn::raw_ios::eofbit)) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(is.open(new std::ifstream(PCOMN_NULL_FILE_NAME)).is_open()) ;
   CPPUNIT_LOG_RUN(is.exceptions(pcomn::raw_ios::goodbit)) ;
   CPPUNIT_LOG_RUN(is.stream().exceptions(std::ios_base::failbit)) ;
   CPPUNIT_LOG_ASSERT(!is.read(buf, sizeof buf - 1)) ;
   CPPUNIT_LOG_EQUAL(is.last_read(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(is.rdstate(), (unsigned)(pcomn::raw_ios::failbit|pcomn::raw_ios::eofbit)) ;
   CPPUNIT_LOG_ASSERT(is.is_open()) ;
   CPPUNIT_LOG_RUN(is.close()) ;
   CPPUNIT_LOG_IS_FALSE(is.is_open()) ;

   CPPUNIT_LOG("\nThe answer to life, universe and everything" << std::endl) ;
   const std::string &a42 = CPPUNIT_AT_TESTDIR("RawStreamTests.2.lst") ;
   const char * const THE_ANSWER = a42.c_str() ;

   std::ifstream answer (THE_ANSWER) ;

   CPPUNIT_LOG_RUN(answer.exceptions(std::ios::eofbit)) ;

   #if _GLIBCXX_USE_CXX11_ABI && PCOMN_WORKAROUND(__GNUC_VER__, < 700)
   CPPUNIT_LOG_FAILURE(answer.read(buf, sizeof buf - 1)) ;
   #else
   CPPUNIT_LOG_EXCEPTION(answer.read(buf, sizeof buf - 1), std::ios::failure) ;
   #endif

   CPPUNIT_LOG_EQUAL(std::string(buf), std::string("42")) ;
   CPPUNIT_LOG_IS_FALSE(answer.bad()) ;
   CPPUNIT_LOG_IS_TRUE(answer.eof()) ;
   CPPUNIT_LOG_IS_FALSE(answer.good()) ;
   CPPUNIT_LOG_IS_TRUE(answer.fail()) ;
   CPPUNIT_LOG_EQUAL(answer.rdstate(), (std::ios::iostate)(std::ios::eofbit | std::ios::failbit)) ;

   CPPUNIT_LOG_RUN(buf[1] = buf[0] = 0) ;
   CPPUNIT_LOG_ASSERT(is.open(new std::ifstream(THE_ANSWER)).is_open()) ;
   CPPUNIT_LOG_RUN(is.exceptions(pcomn::raw_ios::eofbit)) ;
   CPPUNIT_LOG_EQUAL(is.exceptions(), (unsigned)pcomn::raw_ios::eofbit) ;
   CPPUNIT_LOG_EXCEPTION_CODE(is.read(buf, sizeof buf - 1), pcomn::failure_exception, pcomn::raw_ios::eofbit) ;
   CPPUNIT_LOG_IS_FALSE(is.bad()) ;
   CPPUNIT_LOG_IS_TRUE(is.eof()) ;
   CPPUNIT_LOG_IS_FALSE(is.good()) ;
   CPPUNIT_LOG_IS_TRUE(is.fail()) ;
   CPPUNIT_LOG_EQUAL(is.last_read(), (size_t)2) ;
   CPPUNIT_LOG_EQUAL(is.rdstate(), (unsigned)(pcomn::raw_ios::eofbit | pcomn::raw_ios::failbit)) ;
}

void RawStreamTests::Test_FStream()
{
   const char *name ;
   CPPUNIT_LOG_ASSERT(Cleanup_Stream(name = "RawStreamTests.Test_Ftream.lst")) ;

   CPPUNIT_LOG(std::endl << "Creating raw_ofstream" << std::endl) ;
   pcomn::raw_ofstream os (name) ;

   CPPUNIT_LOG_IS_TRUE(os.good()) ;
   CPPUNIT_LOG_EQUAL(os.tell(), (pos_type)0) ;
   CPPUNIT_LOG_RUN(pcomn::unit::generate_sequence(os, 0, 2000)) ;
   CPPUNIT_LOG_EQUAL(os.tell(), (pos_type)12000) ;
   CPPUNIT_LOG_RUN(pcomn::unit::generate_sequence(os, 2000, 6001)) ;
   CPPUNIT_LOG_EQUAL(os.tell(), (pos_type)36006) ;
   CPPUNIT_LOG_RUN(pcomn::unit::generate_sequence(os, 6001, 6002)) ;
   CPPUNIT_LOG_RUN(pcomn::unit::generate_sequence(os, 6002, 10000)) ;
   CPPUNIT_LOG_EQUAL(os.tell(), (pos_type)60000) ;
   CPPUNIT_LOG_RUN(os.close()) ;
   CPPUNIT_LOG_IS_FALSE(os.is_open()) ;
   CPPUNIT_LOG_IS_FALSE(os.write(name, 1)) ;

   CPPUNIT_LOG(std::endl << "Creating raw_ifstream" << std::endl) ;
   pcomn::raw_ifstream is (name) ;
   CPPUNIT_LOG_IS_TRUE(is.good()) ;
   CPPUNIT_LOG_EQUAL(is.tell(), (pos_type)0) ;

   CPPUNIT_LOG(std::endl) ;
   pcomn::unit::checked_read_sequence(is, 0, 1) ;
   CPPUNIT_LOG_ASSERT(is.good()) ;
   CPPUNIT_LOG_ASSERT(!is.eof()) ;
   pcomn::unit::checked_read_sequence(is, 1, 100) ;
   CPPUNIT_LOG_EQUAL(is.seek(1200, pcomn::raw_ios::beg), (pos_type)1200) ;
   CPPUNIT_LOG_ASSERT(!is.eof()) ;
   CPPUNIT_LOG_EQUAL(is.tell(), (pos_type)1200) ;
   pcomn::unit::checked_read_sequence(is, 200, 4000) ;
   CPPUNIT_LOG_EQUAL(is.tell(), (pos_type)24000) ;
   CPPUNIT_LOG_EQUAL(is.seek(-6000, pcomn::raw_ios::cur), (pos_type)18000) ;
   CPPUNIT_LOG_EQUAL(is.tell(), (pos_type)18000) ;
   pcomn::unit::checked_read_sequence(is, 3000, 10000) ;
   CPPUNIT_LOG_IS_FALSE(is.eof()) ;
   CPPUNIT_LOG_IS_TRUE(is.good()) ;

   CPPUNIT_LOG(std::endl) ;
   // Testing the end-of-file condition
   char buf ;
   CPPUNIT_LOG_IS_TRUE(is.read(&buf, 1).eof()) ;
   CPPUNIT_LOG_IS_FALSE(is) ;
}

void RawStreamTests::Test_FStream_Exceptions()
{
   CPPUNIT_LOG_ASSERT(!access(PCOMN_NULL_FILE_NAME, 0)) ;

   pcomn::raw_ifstream is (PCOMN_NULL_FILE_NAME) ;
   char buf[7] = {0, 0, 0, 0, 0, 0, 0} ;

   CPPUNIT_LOG_ASSERT(is.is_open()) ;
   CPPUNIT_LOG_ASSERT(!!is) ;
   CPPUNIT_LOG_EQUAL(is.rdstate(), (unsigned)pcomn::raw_ios::goodbit) ;
   CPPUNIT_LOG_RUN(is.exceptions(pcomn::raw_ios::eofbit)) ;
   CPPUNIT_LOG_EQUAL(is.exceptions(), (unsigned)pcomn::raw_ios::eofbit) ;
   CPPUNIT_LOG_EXCEPTION_CODE(is.read(buf, sizeof buf - 1), pcomn::failure_exception, pcomn::raw_ios::eofbit) ;
   CPPUNIT_LOG_IS_FALSE(is.bad()) ;
   CPPUNIT_LOG_IS_TRUE(is.eof()) ;
   CPPUNIT_LOG_IS_FALSE(is.good()) ;
   CPPUNIT_LOG_IS_TRUE(is.fail()) ;
   CPPUNIT_LOG_EQUAL(is.last_read(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(is.rdstate(), (unsigned)(pcomn::raw_ios::eofbit | pcomn::raw_ios::failbit)) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(is.open(PCOMN_NULL_FILE_NAME).is_open()) ;
   CPPUNIT_LOG_RUN(is.exceptions(pcomn::raw_ios::failbit)) ;
   CPPUNIT_LOG_EQUAL(is.exceptions(), (unsigned)(pcomn::raw_ios::failbit)) ;
   CPPUNIT_LOG_EXCEPTION_CODE(is.read(buf, sizeof buf - 1), pcomn::failure_exception, pcomn::raw_ios::failbit) ;
   CPPUNIT_LOG_EQUAL(is.last_read(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(is.rdstate(), (unsigned)(pcomn::raw_ios::failbit|pcomn::raw_ios::eofbit)) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(is.open(PCOMN_NULL_FILE_NAME).is_open()) ;
   CPPUNIT_LOG_RUN(is.exceptions(pcomn::raw_ios::goodbit)) ;
   CPPUNIT_LOG_ASSERT(!is.read(buf, sizeof buf - 1)) ;
   CPPUNIT_LOG_EQUAL(is.last_read(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(is.rdstate(), (unsigned)(pcomn::raw_ios::failbit|pcomn::raw_ios::eofbit)) ;
   CPPUNIT_LOG_ASSERT(is.is_open()) ;
   CPPUNIT_LOG_RUN(is.close()) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_IS_FALSE(is.is_open()) ;
   CPPUNIT_LOG_ASSERT(!is.read(buf, sizeof buf - 1)) ;
   CPPUNIT_LOG_EQUAL(is.last_read(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(is.rdstate(), (unsigned)(pcomn::raw_ios::closebit)) ;
   CPPUNIT_LOG_IS_FALSE(is.is_open()) ;
   CPPUNIT_LOG_RUN(is.exceptions(pcomn::raw_ios::badbit)) ;
   CPPUNIT_LOG_EXCEPTION_CODE(is.read(buf, sizeof buf - 1),
                              pcomn::failure_exception, pcomn::raw_ios::closebit) ;
}

void RawStreamTests::Test_MemStream()
{
   using namespace pcomn ;

   FILE *testdata_file = fopen(CPPUNIT_AT_TESTDIR("rawstream.testdata.lst").c_str(), "rb") ;
   size_t readbytes = 0 ;
   char testdata[60001] ;
   testdata[sizeof testdata - 1] = 0 ;
   if (testdata_file)
   {
      readbytes = fread(testdata, 1, sizeof testdata - 1, testdata_file) ;
      fclose(testdata_file) ;
   }
   CPPUNIT_LOG_ASSERT(testdata_file) ;
   CPPUNIT_LOG_EQUAL(readbytes, (size_t)60000) ;

   CPPUNIT_LOG_EXCEPTION(pcomn::raw_imemstream(NULL, 1), std::invalid_argument) ;

   pcomn::raw_imemstream empty1 ;
   pcomn::raw_imemstream empty2 (testdata, 0) ;

   CPPUNIT_LOG_IS_FALSE(empty1.data()) ;
   CPPUNIT_LOG_EQUAL(empty1.size(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(empty2.size(), (size_t)0) ;

   char buf[7] = {0, 0, 0, 0, 0, 0, 0} ;
   pcomn::raw_imemstream is (testdata, 60) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(is.data(), (const void *)testdata) ;
   CPPUNIT_LOG_EQUAL(is.size(), (size_t)60) ;
   pcomn::unit::checked_read_sequence(is, 0, 9) ;
   CPPUNIT_LOG_EQUAL(is.tell(), (pos_type)54) ;
   pcomn::unit::checked_read_sequence(is, 9, 10) ;
   CPPUNIT_LOG_ASSERT(is.good()) ;
   CPPUNIT_LOG_EQUAL(is.tell(), (pos_type)60) ;
   CPPUNIT_LOG_EQUAL(is.read(buf, 1).last_read(), (size_t)0) ;
   CPPUNIT_LOG_ASSERT(is.eof()) ;
   CPPUNIT_LOG_ASSERT(!is) ;
   CPPUNIT_LOG_EQUAL(is.tell(), (pos_type)60) ;
   CPPUNIT_LOG_EQUAL(is.seek(18), (pos_type)18) ;
   pcomn::unit::checked_read_sequence(is, 3, 8) ;

   CPPUNIT_LOG(std::endl) ;
   char obuf[800] ;
   char a = 'a' ;

   memset(obuf, 0, sizeof obuf) ;

   raw_omemstream oempty1 (0) ;
   raw_omemstream oempty2 (obuf, 0) ;

   CPPUNIT_LOG_ASSERT(oempty1) ;
   CPPUNIT_LOG_IS_FALSE(oempty1.eof()) ;
   CPPUNIT_LOG_IS_FALSE(oempty1.bad()) ;
   CPPUNIT_LOG_IS_FALSE(oempty1.fail()) ;
   CPPUNIT_LOG_ASSERT(oempty2) ;
   CPPUNIT_LOG_IS_FALSE(oempty2.eof()) ;
   CPPUNIT_LOG_IS_FALSE(oempty2.bad()) ;
   CPPUNIT_LOG_IS_FALSE(oempty2.fail()) ;

   CPPUNIT_LOG_EQUAL(oempty1.size(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(oempty1.maxsize(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(oempty2.size(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(oempty2.maxsize(), (size_t)0) ;

   CPPUNIT_LOG_IS_FALSE(oempty1.write(&a, 1)) ;
   CPPUNIT_LOG_IS_FALSE(oempty2.write(&a, 1)) ;

   CPPUNIT_LOG_EQUAL(oempty1.size(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(oempty1.maxsize(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(oempty2.size(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(oempty2.maxsize(), (size_t)0) ;

   CPPUNIT_LOG_IS_FALSE(oempty1.eof()) ;
   CPPUNIT_LOG_IS_FALSE(oempty1.bad()) ;
   CPPUNIT_LOG_IS_TRUE(oempty1.fail()) ;
   CPPUNIT_LOG_IS_FALSE(oempty2.eof()) ;
   CPPUNIT_LOG_IS_FALSE(oempty2.bad()) ;
   CPPUNIT_LOG_IS_TRUE(oempty2.fail()) ;

   CPPUNIT_LOG_EQUAL(*obuf, '\0') ;

   CPPUNIT_LOG(std::endl) ;
   raw_omemstream ostr10 (10) ;
   CPPUNIT_LOG_ASSERT(ostr10.write("Hello", 5)) ;
   CPPUNIT_LOG_EQUAL(ostr10.maxsize(), (size_t)10) ;
   CPPUNIT_LOG_EQUAL(ostr10.size(), (size_t)5) ;
   CPPUNIT_LOG_IS_FALSE(ostr10.fail()) ;

   CPPUNIT_LOG_IS_FALSE(ostr10.write(", world!", 9)) ;
   CPPUNIT_LOG_EQUAL(ostr10.maxsize(), (size_t)10) ;
   CPPUNIT_LOG_EQUAL(ostr10.size(), (size_t)10) ;
   CPPUNIT_LOG_IS_TRUE(ostr10.fail()) ;
   CPPUNIT_LOG_EQUAL(std::string((const char *)ostr10.data(), (const char *)ostr10.data() + 10), std::string("Hello, wor")) ;

   CPPUNIT_LOG(std::endl) ;
   raw_omemstream ostr ;
   CPPUNIT_LOG_EQUAL(ostr.size(), size_t()) ;
   CPPUNIT_LOG_EQUAL(ostr.maxsize(), (size_t)-1) ;
   generate_seqn<8>(ostr, 0, 20000) ;
   CPPUNIT_LOG_EQUAL(ostr.size(), (size_t)160000) ;
   CPPUNIT_LOG_EQUAL(ostr.maxsize(), (size_t)-1) ;
   checked_read_seqn<8>(ostr.data(), 0, 20000) ;
}

void RawStreamTests::Test_CacheStream()
{
   char buf[7] = {0, 0, 0, 0, 0, 0, 0} ;
   pcomn::raw_ifstream testdata ;

   CPPUNIT_LOG_RUN(testdata.exceptions(pcomn::raw_ios::badbit)) ;
   CPPUNIT_LOG_ASSERT(testdata.open(CPPUNIT_AT_TESTDIR("rawstream.testdata.lst")).is_open()) ;
   CPPUNIT_LOG_ASSERT(testdata.good()) ;

   pcomn::raw_icachestream cacher (&testdata, false) ;

   CPPUNIT_LOG_EQUAL(cacher.tell(), (pos_type)0) ;
   CPPUNIT_LOG_ASSERT(!cacher.caching()) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_startpos(), (pos_type)0) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_endpos(), (pos_type)0) ;
   CPPUNIT_LOG_EQUAL(cacher.seek(2, pcomn::raw_ios::beg), (pos_type)2) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_startpos(), (pos_type)2) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_endpos(), (pos_type)2) ;
   CPPUNIT_LOG_EQUAL(cacher.tell(), (pos_type)2) ;
   CPPUNIT_LOG_EQUAL(cacher.seek(1, pcomn::raw_ios::beg), (pos_type)1) ;
   CPPUNIT_LOG_ASSERT(cacher.read(buf, 1).fail()) ;
   CPPUNIT_LOG_EQUAL(cacher.seek(-1, pcomn::raw_ios::cur), (pos_type)0) ;
   CPPUNIT_LOG_EQUAL(cacher.seek(6), (pos_type)6) ;
   CPPUNIT_LOG_EQUAL(cacher.rdstate(), (unsigned)0) ;
   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_EQUAL(cacher.cache_startpos(), (pos_type)6) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_endpos(), (pos_type)6) ;
   CPPUNIT_LOG_EQUAL(cacher.tell(), (pos_type)6) ;
   CPPUNIT_LOG_EQUAL(testdata.tell(), (pos_type)6) ;
   CPPUNIT_LOG_EQUAL(cacher.read(buf, 6).last_read(), (size_t)6) ;
   CPPUNIT_LOG_EQUAL(cacher.rdstate(), (unsigned)0) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_startpos(), (pos_type)12) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_endpos(), (pos_type)12) ;
   CPPUNIT_LOG_EQUAL(testdata.tell(), (pos_type)12) ;
   CPPUNIT_LOG_EQUAL(cacher.tell(), (pos_type)12) ;

   pcomn::unit::checked_read_sequence(cacher, 2, 7) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_startpos(), (pos_type)42) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_endpos(), (pos_type)42) ;
   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_RUN(cacher.start_caching()) ;
   CPPUNIT_LOG_ASSERT(cacher.caching()) ;
   CPPUNIT_LOG_EQUAL(cacher.seek(6, pcomn::raw_ios::cur), (pos_type)48) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_startpos(), (pos_type)42) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_endpos(), (pos_type)48) ;
   CPPUNIT_LOG_EQUAL(cacher.seek(-7, pcomn::raw_ios::cur), (pos_type)41) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_startpos(), (pos_type)42) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_endpos(), (pos_type)48) ;
   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_EQUAL(cacher.tell(), (pos_type)41) ;
   CPPUNIT_LOG_EQUAL(cacher.seek(42, pcomn::raw_ios::beg), (pos_type)42) ;
   pcomn::unit::checked_read_sequence(cacher, 7, 15) ;
   CPPUNIT_LOG_EQUAL(cacher.tell(), (pos_type)90) ;
   CPPUNIT_LOG_EQUAL(testdata.tell(), (pos_type)90) ;
   CPPUNIT_LOG_ASSERT(cacher.caching()) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_startpos(), (pos_type)42) ;
   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_EQUAL(cacher.seek(10, pcomn::raw_ios::beg), (pos_type)10) ;
   CPPUNIT_LOG_EQUAL(cacher.read(buf, 1).last_read(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(cacher.rdstate(), (unsigned)pcomn::raw_ios::failbit) ;

   CPPUNIT_LOG_EQUAL(cacher.seek(48, pcomn::raw_ios::beg), (pos_type)48) ;
   CPPUNIT_LOG_EQUAL(cacher.tell(), (pos_type)48) ;
   CPPUNIT_LOG_EQUAL(testdata.tell(), (pos_type)90) ;

   pcomn::unit::checked_read_sequence(cacher, 8, 10) ;
   CPPUNIT_LOG_EQUAL(cacher.tell(), (pos_type)60) ;
   CPPUNIT_LOG_EQUAL(testdata.tell(), (pos_type)90) ;

   CPPUNIT_LOG_EQUAL(cacher.seek(90, pcomn::raw_ios::beg), (pos_type)90) ;
   CPPUNIT_LOG_EQUAL(cacher.seek(-30, pcomn::raw_ios::cur), (pos_type)60) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_startpos(), (pos_type)42) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_endpos(), (pos_type)90) ;
   CPPUNIT_LOG(std::endl) ;

   pcomn::unit::checked_read_sequence(cacher, 10, 2000) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_startpos(), (pos_type)42) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_endpos(), (pos_type)12000) ;
   CPPUNIT_LOG_EQUAL(cacher.tell(), (pos_type)12000) ;
   CPPUNIT_LOG_EQUAL(testdata.tell(), (pos_type)12000) ;
   CPPUNIT_LOG_EQUAL(cacher.seek(36, pcomn::raw_ios::beg), (pos_type)36) ;
   CPPUNIT_LOG_EQUAL(cacher.tell(), (pos_type)36) ;
   CPPUNIT_LOG_EQUAL(testdata.tell(), (pos_type)12000) ;
   CPPUNIT_LOG_EQUAL(cacher.read(buf, 1).last_read(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(cacher.rdstate(), (unsigned)pcomn::raw_ios::failbit) ;
   CPPUNIT_LOG_EQUAL(cacher.tell(), (pos_type)36) ;
   CPPUNIT_LOG_EQUAL(testdata.tell(), (pos_type)12000) ;
   CPPUNIT_LOG_EQUAL(cacher.seek(6, pcomn::raw_ios::cur), cacher.cache_startpos()) ;
   pcomn::unit::checked_read_sequence(cacher, 7, 4000) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_startpos(), (pos_type)42) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_endpos(), (pos_type)24000) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(cacher.stop_caching()) ;
   CPPUNIT_LOG_IS_FALSE(cacher.caching()) ;
   CPPUNIT_LOG_EQUAL(cacher.tell(), (pos_type)24000) ;
   CPPUNIT_LOG("Cache should be flashed only after the first reading operation beneath the cache end."
               << std::endl) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_startpos(), (pos_type)42) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_endpos(), (pos_type)24000) ;
   CPPUNIT_LOG_EQUAL(testdata.tell(), (pos_type)24000) ;
   CPPUNIT_LOG_EQUAL(cacher.tell(), (pos_type)24000) ;
   CPPUNIT_LOG_EQUAL(cacher.seek(-600, pcomn::raw_ios::cur), (pos_type)23400) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_endpos(), (pos_type)24000) ;
   pcomn::unit::checked_read_sequence(cacher, 3900, 3950) ;
   CPPUNIT_LOG_EQUAL(testdata.tell(), (pos_type)24000) ;
   CPPUNIT_LOG_EQUAL(cacher.tell(), (pos_type)23700) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_startpos(), (pos_type)42) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_endpos(), (pos_type)24000) ;
   pcomn::unit::checked_read_sequence(cacher, 3950, 6000) ;
   CPPUNIT_LOG_EQUAL(cacher.tell(), (pos_type)36000) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_startpos(), (pos_type)36000) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_endpos(), (pos_type)36000) ;
   CPPUNIT_LOG_EQUAL(testdata.tell(), (pos_type)36000) ;
   CPPUNIT_LOG_IS_FALSE(cacher.eof()) ;
   CPPUNIT_LOG_ASSERT(cacher.good()) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(cacher.start_caching()) ;
   CPPUNIT_LOG_ASSERT(cacher.caching()) ;
   CPPUNIT_LOG_EQUAL((int)cacher.tell(), 36000) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_startpos(), (pos_type)36000) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_endpos(), (pos_type)36000) ;
   CPPUNIT_LOG_EQUAL((int)testdata.tell(), 36000) ;
   CPPUNIT_LOG_IS_FALSE(cacher.eof()) ;
   CPPUNIT_LOG_ASSERT(cacher.good()) ;
   CPPUNIT_LOG_EQUAL(cacher.seek(-12, pcomn::raw_ios::end), (pos_type)59988) ;
   CPPUNIT_LOG_EQUAL(cacher.tell(), (pos_type)59988) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_startpos(), (pos_type)36000) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_endpos(), (pos_type)60000) ;
   CPPUNIT_LOG_EQUAL((int)testdata.tell(), 60000) ;
   CPPUNIT_LOG_IS_FALSE(cacher.eof()) ;
   CPPUNIT_LOG_ASSERT(cacher.good()) ;
   pcomn::unit::checked_read_sequence(cacher, 9998, 9999) ;
   CPPUNIT_LOG_ASSERT(cacher.read(buf, sizeof buf).eof()) ;
   CPPUNIT_LOG_EQUAL(cacher.tell(), (pos_type)60000) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_startpos(), (pos_type)36000) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_endpos(), (pos_type)60000) ;
   CPPUNIT_LOG_EQUAL(testdata.tell(), (pos_type)60000) ;
   CPPUNIT_LOG_EQUAL(cacher.seek(-6, pcomn::raw_ios::cur), (pos_type)59994) ;
   pcomn::unit::checked_read_sequence(cacher, 9999, 10000) ;
   CPPUNIT_LOG_IS_FALSE(cacher.eof()) ;
   CPPUNIT_LOG_ASSERT(cacher.good()) ;
   CPPUNIT_LOG_EQUAL(cacher.seek(1, pcomn::raw_ios::end), (pos_type)-1) ;
   CPPUNIT_LOG_IS_TRUE(cacher.fail()) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_startpos(), (pos_type)36000) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_endpos(), (pos_type)60000) ;
   CPPUNIT_LOG_RUN(cacher.start_caching()) ;
   CPPUNIT_LOG_ASSERT(cacher.is_open()) ;
   CPPUNIT_LOG_RUN(cacher.close()) ;
   CPPUNIT_LOG_IS_FALSE(cacher.is_open()) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_startpos(), (pos_type)0) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_endpos(), (pos_type)0) ;
   CPPUNIT_LOG_IS_FALSE(cacher.caching()) ;
}

void RawStreamTests::Test_CacheStream_Eof()
{
   char buf[7] = {0, 0, 0, 0, 0, 0, 0} ;
   pcomn::raw_ifstream testdata ;

   CPPUNIT_LOG_ASSERT(testdata.open(CPPUNIT_AT_TESTDIR("rawstream.testdata.lst")).is_open()) ;
   CPPUNIT_LOG_ASSERT(testdata.good()) ;

   pcomn::raw_icachestream cacher (&testdata, false) ;

   CPPUNIT_LOG_EQUAL(cacher.tell(), (pos_type)0) ;
   CPPUNIT_LOG_ASSERT(!cacher.caching()) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_startpos(), (pos_type)0) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_endpos(), (pos_type)0) ;
   CPPUNIT_LOG_RUN(cacher.start_caching()) ;
   CPPUNIT_LOG_ASSERT(cacher.caching()) ;
   pcomn::unit::checked_read_sequence(cacher, 0, 9999) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_startpos(), (pos_type)0) ;
   CPPUNIT_LOG_EQUAL(cacher.cache_endpos(), (pos_type)59994) ;
   CPPUNIT_LOG_EQUAL(cacher.tell(), (pos_type)59994) ;
   CPPUNIT_LOG_EQUAL(cacher.read(buf, 7).last_read(), (size_t)6) ;
   CPPUNIT_LOG_RUN(cacher.stop_caching()) ;
   CPPUNIT_LOG_EQUAL(cacher.seek(0), (pos_type)0) ;
   CPPUNIT_LOG_EQUAL(cacher.rdstate(), (unsigned)0) ;
   CPPUNIT_LOG_EQUAL(testdata.rdstate(), (unsigned)0) ;
   pcomn::unit::checked_read_sequence(cacher, 0, 10000) ;
   CPPUNIT_LOG_EQUAL(cacher.read(buf, 1).last_read(), (size_t)0) ;
   CPPUNIT_LOG_ASSERT(cacher.eof()) ;
   CPPUNIT_LOG_ASSERT(testdata.eof()) ;
   CPPUNIT_LOG_EQUAL(cacher.seek(0, pcomn::raw_ios::cur), (pos_type)60000) ;
   CPPUNIT_LOG_ASSERT(cacher.good()) ;
   CPPUNIT_LOG_ASSERT(testdata.good()) ;
   CPPUNIT_LOG_EQUAL(cacher.read(buf, 1).last_read(), (size_t)0) ;
   CPPUNIT_LOG_ASSERT(cacher.eof()) ;
   CPPUNIT_LOG_ASSERT(testdata.eof()) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(RawStreamTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv, "unittest_rawstream.diag.ini", "Rawstream tests") ;
}
