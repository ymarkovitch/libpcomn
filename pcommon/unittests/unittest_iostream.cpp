/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_iostream.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for binary iostreams.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   9 Oct 2008
*******************************************************************************/
#include <pcomn_range.h>
#include <pcomn_unittest.h>
#include <pcomn_binstream.h>
#include <pcomn_fstream.h>
#include <pcomn_fileutils.h>
#include <pcomn_sys.h>

#include "pcomn_testhelpers.h"

#include <stdio.h>
#include <typeinfo>

#include <fstream>

/*******************************************************************************
                     class StreamFixture
*******************************************************************************/
class StreamFixture : public CppUnit::TestFixture {

   protected:
      static bool Cleanup_Stream(const char *filename)
      {
         return access(filename, 0) || !unlink(filename) ;
      }

      template<class IStream>
      void TestEmptyStreamGet(IStream &) ;

      template<class IStream>
      void TestEmptyStreamRead(IStream &) ;

      template<class IStream>
      void TestInputStream(IStream &) ;

      static const int DWIDTH = pcomn::unit::DWIDTH ;
      static const int SEQ_FROM = 2 ;
      static const int SEQ_TO = 40000 ;
} ;

template<class IStream>
void StreamFixture::TestEmptyStreamGet(IStream &EmptyStream)
{
   uint32_t buf = 0xCCCCCCCCU ;

   CPPUNIT_LOG(std::endl
               << "Testing an empty stream of type " << CPPUNIT_TYPENAME(EmptyStream)
               << " through interface of class " << CPPUNIT_TYPENAME(IStream)
               << " using 'get' method" << std::endl) ;

   CPPUNIT_LOG_EQUAL(EmptyStream.last_read(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(EmptyStream.total_read(), (size_t)0) ;
   CPPUNIT_LOG_IS_FALSE(EmptyStream.throw_eof()) ;

   // Even an empty stream should not have eof() state set until the first read
   // operation. Only read operation can set eof state.
   CPPUNIT_LOG_IS_FALSE(EmptyStream.eof()) ;
   CPPUNIT_LOG_EQUAL(EmptyStream.get(), -1) ;
   CPPUNIT_LOG_IS_TRUE(EmptyStream.eof()) ;
   CPPUNIT_LOG_EQUAL(EmptyStream.last_read(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(EmptyStream.total_read(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(EmptyStream.read(&buf, sizeof buf), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(buf, (uint32_t)0xCCCCCCCCU) ;
   CPPUNIT_LOG_EQUAL(EmptyStream.last_read(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(EmptyStream.total_read(), (size_t)0) ;

   CPPUNIT_LOG(std::endl) ;
   // Setting throw_eof state returns _previous_ state
   CPPUNIT_LOG_IS_FALSE(EmptyStream.throw_eof(true)) ;
   CPPUNIT_LOG_IS_TRUE(EmptyStream.throw_eof()) ;
   CPPUNIT_LOG_IS_TRUE(EmptyStream.eof()) ;
   CPPUNIT_LOG_EQUAL(EmptyStream.last_read(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(EmptyStream.total_read(), (size_t)0) ;
   CPPUNIT_LOG_EXCEPTION(EmptyStream.get(), pcomn::eof_error) ;
   CPPUNIT_LOG_IS_TRUE(EmptyStream.eof()) ;
   CPPUNIT_LOG_EQUAL(EmptyStream.last_read(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(EmptyStream.total_read(), (size_t)0) ;
   CPPUNIT_LOG_EXCEPTION(EmptyStream.read(&buf, sizeof buf), pcomn::eof_error) ;
   CPPUNIT_LOG_IS_TRUE(EmptyStream.eof()) ;
   CPPUNIT_LOG_EQUAL(EmptyStream.last_read(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(EmptyStream.total_read(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(buf, (uint32_t)0xCCCCCCCCU) ;
}

template<class IStream>
void StreamFixture::TestEmptyStreamRead(IStream &EmptyStream)
{
   uint32_t buf = 0xCCCCCCCCU ;

   CPPUNIT_LOG(std::endl
               << "Testing an empty stream of type " << CPPUNIT_TYPENAME(EmptyStream)
               << " through interface of class " << CPPUNIT_TYPENAME(IStream)
               << " using 'read' method" << std::endl) ;

   CPPUNIT_LOG_EQUAL(EmptyStream.last_read(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(EmptyStream.total_read(), (size_t)0) ;
   CPPUNIT_LOG_IS_FALSE(EmptyStream.throw_eof()) ;
   CPPUNIT_LOG_IS_FALSE(EmptyStream.eof()) ;

   CPPUNIT_LOG_EQUAL(EmptyStream.read(&buf, sizeof buf), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(buf, (uint32_t)0xCCCCCCCCU) ;
   CPPUNIT_LOG_IS_TRUE(EmptyStream.eof()) ;
   CPPUNIT_LOG_EQUAL(EmptyStream.last_read(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(EmptyStream.total_read(), (size_t)0) ;
   CPPUNIT_LOG(std::endl) ;

   // Setting throw_eof state returns _previous_ state
   CPPUNIT_LOG_IS_FALSE(EmptyStream.throw_eof(true)) ;
   CPPUNIT_LOG_IS_TRUE(EmptyStream.throw_eof()) ;
   CPPUNIT_LOG_IS_TRUE(EmptyStream.eof()) ;
   CPPUNIT_LOG_EQUAL(EmptyStream.last_read(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(EmptyStream.total_read(), (size_t)0) ;
   CPPUNIT_LOG_EXCEPTION(EmptyStream.read(&buf, sizeof buf), pcomn::eof_error) ;
   CPPUNIT_LOG_IS_TRUE(EmptyStream.eof()) ;
   CPPUNIT_LOG_EQUAL(EmptyStream.last_read(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(EmptyStream.total_read(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(buf, (uint32_t)0xCCCCCCCCU) ;
}

// Test any input stream.
// Test assumes that stream being tested is filled with text of sequential numbers
// with pcomn::unit::generate_sequence(stream, SEQ_FROM, SEQ_TO)
template<class IStream>
void StreamFixture::TestInputStream(IStream &Is)
{
   CPPUNIT_LOG(std::endl << "Testing input stream." << std::endl
               << "Actual type: " << CPPUNIT_TYPENAME(Is) << std::endl
               << "Interface type: " << CPPUNIT_TYPENAME(IStream) << std::endl) ;

   CPPUNIT_LOG_EQUAL(Is.last_read(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(Is.total_read(), (size_t)0) ;
   CPPUNIT_LOG_IS_FALSE(Is.throw_eof()) ;
   CPPUNIT_LOG_IS_FALSE(Is.eof()) ;

   CPPUNIT_LOG_EQUAL(Is.get(), (int)' ') ;
   CPPUNIT_LOG_EQUAL(Is.last_read(), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(Is.total_read(), (size_t)1) ;

   for (size_t n = 2 ; n < (size_t)DWIDTH ; ++n)
   {
      CPPUNIT_EQUAL(Is.get(), (int)' ') ;
      CPPUNIT_EQUAL(Is.last_read(), (size_t)1) ;
      CPPUNIT_EQUAL(Is.total_read(), n) ;
   }
   CPPUNIT_LOG_EQUAL(Is.get(), (int)'2') ;
   CPPUNIT_LOG_EQUAL(Is.last_read(), (size_t)1) ;
   CPPUNIT_LOG_EQUAL(Is.total_read(), (size_t)6) ;

   char buf[DWIDTH * 2 + 1] ;
   pcomn::unit::fillbuf(buf)[sizeof buf - 1] = 0 ;

   CPPUNIT_LOG_EQUAL(Is.read(buf, sizeof buf - 1), sizeof buf - 1) ;
   CPPUNIT_LOG_EQUAL(Is.last_read(), (size_t)12) ;
   CPPUNIT_LOG_EQUAL(Is.total_read(), (size_t)18) ;
   CPPUNIT_LOG_EQUAL(std::string(buf), std::string("     3     4")) ;
   CPPUNIT_LOG_IS_FALSE(Is.eof()) ;

   pcomn::binary_ostrstream StrStream ;
   char bigbuf[DWIDTH * SEQ_TO + 1] ;
   pcomn::unit::fillbuf(bigbuf)[sizeof buf - 1] = 0 ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EQUAL(Is.read(bigbuf, (SEQ_TO - 6)*DWIDTH), (size_t)((SEQ_TO - 6)*DWIDTH)) ;
   CPPUNIT_LOG_EQUAL(Is.last_read(), (size_t)((SEQ_TO - 6)*DWIDTH)) ;
   CPPUNIT_LOG_EQUAL(Is.total_read(), (size_t)((SEQ_TO - 3)*DWIDTH)) ;

   CPPUNIT_LOG_EQUAL(std::string(bigbuf + 0, bigbuf + Is.last_read()),
                     pcomn::unit::generate_sequence(StrStream.clear(), 5, SEQ_TO - 1).str()) ;
   // Read the rest as std::string
   CPPUNIT_LOG_EQUAL(Is.read(), pcomn::unit::generate_sequence(StrStream.clear(), SEQ_TO - 1, SEQ_TO).str()) ;
   // std::string binary_istream::read() can set eof state only if it has read no data
   CPPUNIT_LOG_IS_FALSE(Is.eof()) ;
   CPPUNIT_LOG_EQUAL(Is.last_read(), (size_t)DWIDTH) ;
   CPPUNIT_LOG_EQUAL(Is.total_read(), (size_t)((SEQ_TO - 2)*DWIDTH)) ;

   CPPUNIT_LOG(std::endl) ;
   // There should be no more data in the stream
   CPPUNIT_LOG_EQUAL(Is.read(), std::string()) ;
   CPPUNIT_LOG_IS_TRUE(Is.eof()) ;
   CPPUNIT_LOG_EQUAL(Is.last_read(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(Is.total_read(), (size_t)((SEQ_TO - 2)*DWIDTH)) ;
   CPPUNIT_LOG_EQUAL(Is.read(), std::string()) ;
   CPPUNIT_LOG_EQUAL(Is.last_read(), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(Is.total_read(), (size_t)((SEQ_TO - 2)*DWIDTH)) ;
   CPPUNIT_LOG_IS_TRUE(Is.eof()) ;
   CPPUNIT_LOG_EQUAL(Is.get(), EOF) ;
}

/*******************************************************************************
                     class BinaryStreamTests
*******************************************************************************/
class BinaryStreamTests : public StreamFixture {
   private:
      void Test_StringStream() ;
      void Test_Stream_Over_Iterator() ;
      void Test_IStream_Range() ;
      void Test_Delegating_IStream() ;
      void Test_OBufStream() ;
      void Test_IBufStream() ;
      void Test_FileStream() ;
      void Test_Readfile() ;

      CPPUNIT_TEST_SUITE(BinaryStreamTests) ;

      CPPUNIT_TEST(Test_StringStream) ;
      CPPUNIT_TEST(Test_Stream_Over_Iterator) ;
      CPPUNIT_TEST(Test_IStream_Range) ;
      CPPUNIT_TEST(Test_Delegating_IStream) ;
      CPPUNIT_TEST(Test_OBufStream) ;
      CPPUNIT_TEST(Test_IBufStream) ;
      CPPUNIT_TEST(Test_FileStream) ;
      CPPUNIT_TEST(Test_Readfile) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

void BinaryStreamTests::Test_StringStream()
{
   pcomn::binary_ostrstream Stream ;
   CPPUNIT_LOG_EQUAL(Stream.str(), std::string()) ;
   CPPUNIT_LOG_EQUAL(Stream.put('H').str(), std::string("H")) ;
   CPPUNIT_LOG_EQUAL(Stream.write("ello"), (size_t)4) ;
   CPPUNIT_LOG_EQUAL(Stream.str(), std::string("Hello")) ;
   CPPUNIT_LOG_EQUAL(Stream.put(',').str(), std::string("Hello,")) ;
   CPPUNIT_LOG_EQUAL(Stream.write(""), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(Stream.write("", 0), (size_t)0) ;
   CPPUNIT_LOG_EXCEPTION(Stream.write(NULL, 0), std::invalid_argument) ;
   CPPUNIT_LOG_EQUAL(Stream.write("world!", 0), (size_t)0) ;
   CPPUNIT_LOG_EQUAL(Stream.str(), std::string("Hello,")) ;
   CPPUNIT_LOG_EQUAL(Stream.write((const void *)" world!", 6), (size_t)6) ;
   CPPUNIT_LOG_EQUAL(Stream.str(), std::string("Hello, world")) ;
   CPPUNIT_LOG_EQUAL(Stream.put('!').str(), std::string("Hello, world!")) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(Stream = pcomn::binary_ostrstream("Hello, world")) ;
   CPPUNIT_LOG_EQUAL(Stream.str(), std::string("Hello, world")) ;
   CPPUNIT_LOG_EQUAL(Stream.put('!').str(), std::string("Hello, world!")) ;

   CPPUNIT_LOG(std::endl) ;
   pcomn::binary_ostrstream OtherStream ("Hello") ;
   pcomn::binary_ostream &BinaryStream = OtherStream ;
   CPPUNIT_LOG_EQUAL(BinaryStream.put(',').write(" world!"), (size_t)7) ;
   CPPUNIT_LOG_EQUAL(OtherStream.str(), std::string("Hello, world!")) ;
}

void BinaryStreamTests::Test_Stream_Over_Iterator()
{
   typedef pcomn::istream_over_iterator<const char *> charbuf_istream ;

   pcomn::binary_istream *AsBinaryStream ;

   CPPUNIT_LOG("Test an empty istream over iterator" << std::endl) ;

   charbuf_istream EmptyStream1 ;
   charbuf_istream EmptyStream2 ;
   CPPUNIT_LOG_RUN(AsBinaryStream = &EmptyStream2) ;

   TestEmptyStreamGet(EmptyStream1) ;
   TestEmptyStreamGet(*AsBinaryStream) ;

   charbuf_istream EmptyStream3 ;
   charbuf_istream EmptyStream4 ;
   CPPUNIT_LOG_RUN(AsBinaryStream = &EmptyStream4) ;
   TestEmptyStreamRead(EmptyStream3) ;
   TestEmptyStreamRead(*AsBinaryStream) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG("Test an istream over iterator filled with data" << std::endl) ;
   pcomn::binary_ostrstream Data ;
   CPPUNIT_LOG_RUN(pcomn::unit::generate_sequence(Data, SEQ_FROM, SEQ_TO)) ;
   charbuf_istream TestedStream (Data.str().c_str(), Data.str().c_str() + Data.str().size()) ;
   TestInputStream(TestedStream) ;
   charbuf_istream TestedBinaryStream (Data.str().c_str(), Data.str().c_str() + Data.str().size()) ;
   CPPUNIT_LOG_RUN(AsBinaryStream = &TestedBinaryStream) ;
   TestInputStream(*AsBinaryStream) ;
}

void BinaryStreamTests::Test_IStream_Range()
{
   typedef pcomn::istream_over_iterator<const char *>   charbuf_istream ;
   typedef pcomn::istream_range<pcomn::binary_istream>  irange ;

   charbuf_istream EmptyStream1 ;
   charbuf_istream EmptyStream2 ;
   CPPUNIT_LOG_ASSERT(!irange(EmptyStream1)) ;
   const char CountDown[] = "987654321" ;
   charbuf_istream StreamCountDown(CountDown + 0, CountDown + 9) ;
   CPPUNIT_LOG_EQUAL(pcomn::r_distance(irange(StreamCountDown)), (ptrdiff_t)9) ;
   charbuf_istream Stream2CountDown(CountDown + 0, CountDown + 9) ;

   std::string Test ;
   for (irange r (Stream2CountDown) ; r ; ++r)
      Test.append(1, *r) ;

   CPPUNIT_LOG_EQUAL(Test, std::string("987654321")) ;
}

void BinaryStreamTests::Test_Delegating_IStream()
{
   typedef pcomn::istream_over_iterator<const char *> charbuf_istream ;

   CPPUNIT_LOG_EXCEPTION(pcomn::delegating_istream(NULL), std::invalid_argument) ;
   charbuf_istream EmptyStream_1 ;
   charbuf_istream EmptyStream_2 ;
   pcomn::delegating_istream Delegate(EmptyStream_1) ;

   CPPUNIT_LOG_EQUAL(&Delegate.get_istream(),
                     static_cast<pcomn::binary_istream *>(&EmptyStream_1)) ;
   CPPUNIT_LOG_IS_FALSE(EmptyStream_1.eof()) ;
   CPPUNIT_LOG_IS_FALSE(Delegate.eof()) ;
   CPPUNIT_LOG_EQUAL(Delegate.get(), EOF) ;
   CPPUNIT_LOG_ASSERT(Delegate.eof()) ;
   CPPUNIT_LOG_ASSERT(EmptyStream_1.eof()) ;

   CPPUNIT_LOG_EQUAL(Delegate.total_read(), (size_t)0) ;
   CPPUNIT_LOG_ASSERT(Delegate.reset(EmptyStream_1).eof()) ;
   CPPUNIT_LOG_EQUAL(&Delegate.get_istream(),
                     static_cast<pcomn::binary_istream *>(&EmptyStream_1)) ;
   CPPUNIT_LOG_EXCEPTION(Delegate.reset(Delegate), std::invalid_argument) ;
   CPPUNIT_LOG_EXCEPTION(Delegate.reset(&Delegate), std::invalid_argument) ;
   CPPUNIT_LOG_ASSERT(Delegate.eof()) ;
   CPPUNIT_LOG_IS_FALSE(Delegate.reset(EmptyStream_2).eof()) ;
   CPPUNIT_LOG_EQUAL(&Delegate.get_istream(),
                     static_cast<pcomn::binary_istream *>(&EmptyStream_2)) ;

   CPPUNIT_LOG(std::endl) ;

   const char CountDown[] = "987654321" ;
   const char FooBar[] = "Foo. Bar." ;
   const char Quux[] = "quux" ;

   CPPUNIT_LOG_RUN(Delegate.reset(new charbuf_istream(CountDown + 0, CountDown + sizeof CountDown - 1))) ;
   CPPUNIT_LOG_EQUAL(Delegate.read(), std::string(CountDown)) ;
   CPPUNIT_LOG_EQUAL(Delegate.total_read(), strlen(CountDown)) ;
   CPPUNIT_LOG_EQUAL(Delegate.get_istream().total_read(), strlen(CountDown)) ;

   charbuf_istream FooBarStream(FooBar + 0, FooBar + sizeof FooBar - 1) ;
   CPPUNIT_LOG_EQUAL(&Delegate.reset(FooBarStream).get_istream(),
                     static_cast<pcomn::binary_istream *>(&FooBarStream)) ;
   CPPUNIT_LOG_EQUAL(Delegate.read(), std::string(FooBar)) ;
   CPPUNIT_LOG_EQUAL(Delegate.get_istream().total_read(), strlen(FooBar)) ;
   CPPUNIT_LOG_EQUAL(Delegate.total_read(), strlen(CountDown) + strlen(FooBar)) ;

   CPPUNIT_LOG_RUN(Delegate.reset(new charbuf_istream(Quux + 0, Quux + sizeof Quux - 1))) ;
   CPPUNIT_LOG_EQUAL(Delegate.read(), std::string(Quux)) ;
   CPPUNIT_LOG_EQUAL(Delegate.get_istream().total_read(), strlen(Quux)) ;
   CPPUNIT_LOG_EQUAL(Delegate.total_read(), strlen(CountDown) + strlen(FooBar) + strlen(Quux)) ;
}

void BinaryStreamTests::Test_OBufStream()
{
   pcomn::binary_ostrstream UnderlyingStream ;
   pcomn::binary_obufstream Stream (UnderlyingStream, 16) ;

   CPPUNIT_LOG_EQUAL(UnderlyingStream.str(), std::string()) ;
   CPPUNIT_LOG_RUN(Stream.put(' ').put('1')) ;
   CPPUNIT_LOG_EQUAL(UnderlyingStream.str(), std::string()) ;
   CPPUNIT_LOG_EQUAL(Stream.write(" 2"), (size_t)2) ;
   CPPUNIT_LOG_EQUAL(UnderlyingStream.str(), std::string()) ;
   CPPUNIT_LOG_EQUAL(Stream.write(" 3 4 5 6 7 8 910"), (size_t)16) ;
   CPPUNIT_LOG_EQUAL(UnderlyingStream.str(), std::string(" 1 2 3 4 5 6 7 8")) ;
   CPPUNIT_LOG_RUN(Stream.flush()) ;
   CPPUNIT_LOG_EQUAL(UnderlyingStream.str(), std::string(" 1 2 3 4 5 6 7 8 910")) ;
   for (char c = '1' ; c <= '9' ; ++c)
      Stream.put('1').put(c) ;
   CPPUNIT_LOG_EQUAL(UnderlyingStream.str(), std::string(" 1 2 3 4 5 6 7 8 9101112131415161718")) ;
   CPPUNIT_LOG_RUN(Stream.flush()) ;
   CPPUNIT_LOG_EQUAL(UnderlyingStream.str(), std::string(" 1 2 3 4 5 6 7 8 910111213141516171819")) ;
}

void BinaryStreamTests::Test_IBufStream()
{
   CPPUNIT_LOG_EXCEPTION(pcomn::binary_ibufstream(NULL, 64), std::invalid_argument) ;

   typedef pcomn::istream_over_iterator<const char *> charbuf_istream ;

   CPPUNIT_LOG("Test an empty ibufstream" << std::endl) ;

   charbuf_istream EmptyStream1 ;
   pcomn::binary_ibufstream EmptyBufStream1 (EmptyStream1, 64) ;
   pcomn::binary_ibufstream EmptyBufStream2 (std::unique_ptr<charbuf_istream>(new charbuf_istream), 64) ;
   pcomn::binary_istream *AsBinaryStream ;

   CPPUNIT_LOG_RUN(AsBinaryStream = &EmptyBufStream2) ;

   TestEmptyStreamGet(EmptyBufStream1) ;
   TestEmptyStreamGet(*AsBinaryStream) ;

   pcomn::binary_ibufstream EmptyBufStream3 (std::unique_ptr<charbuf_istream>(new charbuf_istream), 64) ;
   pcomn::binary_ibufstream EmptyBufStream4 (std::unique_ptr<charbuf_istream>(new charbuf_istream), 64) ;
   CPPUNIT_LOG_RUN(AsBinaryStream = &EmptyBufStream4) ;
   TestEmptyStreamRead(EmptyBufStream3) ;
   TestEmptyStreamRead(*AsBinaryStream) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG("Test an ibufstream with data" << std::endl) ;
   pcomn::binary_ostrstream Data ;
   CPPUNIT_LOG_RUN(pcomn::unit::generate_sequence(Data, SEQ_FROM, SEQ_TO)) ;

   {
      CPPUNIT_LOG("\n\nTesting a buffered input stream" << std::endl) ;
      charbuf_istream DataStream1 (Data.str().c_str(), Data.str().c_str() + Data.str().size()) ;
      pcomn::binary_ibufstream BufStream (DataStream1, 1024) ;
      TestInputStream(BufStream) ;
   }

   {
      CPPUNIT_LOG("\n\nTesting a buffered input stream with a large buffer" << std::endl) ;
      charbuf_istream DataStream1 (Data.str().c_str(), Data.str().c_str() + Data.str().size()) ;
      pcomn::binary_ibufstream BufStream (DataStream1, Data.str().size() + 1009) ;
      TestInputStream(BufStream) ;
   }

   {
      CPPUNIT_LOG("\n\nTesting a buffered input stream with a small buffer" << std::endl) ;
      charbuf_istream DataStream1 (Data.str().c_str(), Data.str().c_str() + Data.str().size()) ;
      pcomn::binary_ibufstream BufStream (DataStream1, 5) ;
      TestInputStream(BufStream) ;
   }

   {
      CPPUNIT_LOG("\n\nTesting a buffered input stream with a zero-sized buffer" << std::endl) ;
      charbuf_istream DataStream1 (Data.str().c_str(), Data.str().c_str() + Data.str().size()) ;
      pcomn::binary_ibufstream BufStream (DataStream1, 0) ;
      TestInputStream(BufStream) ;
   }

   {
      CPPUNIT_LOG("\n\nTesting a bounded input stream" << std::endl) ;

      pcomn::binary_ostrstream DataOverflow ;
      CPPUNIT_LOG_RUN(pcomn::unit::generate_sequence(DataOverflow, SEQ_FROM, SEQ_TO + 10)) ;
      charbuf_istream DataOverflowStream (DataOverflow.str().c_str(),
                                          DataOverflow.str().c_str() + DataOverflow.str().size()) ;

      pcomn::binary_ibufstream BoundedBufStream (DataOverflowStream, 1023) ;

      BoundedBufStream.set_bound(Data.str().size()) ;

      TestInputStream(BoundedBufStream) ;
      CPPUNIT_LOG_EQUAL(BoundedBufStream.total_read(), Data.str().size()) ;
   }
}

void BinaryStreamTests::Test_FileStream()
{
   pcomn::malloc_ptr<char[]> TmpName (tempnam(NULL, "pcomn")) ;
   int LastHandle ;

   {
      pcomn::binary_ifdstream FileStream
         (PCOMN_ENSURE_POSIX(::open(TmpName.get(), O_RDWR|O_CREAT|O_EXCL, 0644), "open")) ;

      CPPUNIT_LOG("fd=" << FileStream.fd() << std::endl) ;
      CPPUNIT_LOG_ASSERT(FileStream.fd() > 0) ;
      CPPUNIT_LOG_ASSERT(FileStream.owned()) ;

      CPPUNIT_LOG_RUN(LastHandle = FileStream.fd()) ;
   }

   ::unlink(TmpName.get()) ;
   CPPUNIT_LOG_ASSERT(dup(LastHandle) < 0 && errno == EBADF) ;

   CPPUNIT_LOG(std::endl) ;
   TmpName.reset(tempnam(NULL, "pcomn")) ;

   {
      pcomn::binary_ifdstream FileStream
         (PCOMN_ENSURE_POSIX(::open(TmpName.get(), O_RDWR|O_CREAT|O_EXCL, 0644), "open"), false) ;

      CPPUNIT_LOG("fd=" << FileStream.fd() << std::endl) ;
      CPPUNIT_LOG_ASSERT(FileStream.fd() > 0) ;
      CPPUNIT_LOG_IS_FALSE(FileStream.owned()) ;

      CPPUNIT_LOG_RUN(LastHandle = FileStream.fd()) ;

      TestEmptyStreamGet(FileStream) ;
   }
   ::unlink(TmpName.get()) ;
   pcomn::fd_safehandle FdGuard (LastHandle) ;
   CPPUNIT_LOG_ASSERT(LastHandle = PCOMN_ENSURE_POSIX(dup(LastHandle), "dup")) ;
   ::close(LastHandle) ;

   CPPUNIT_LOG(std::endl) ;
   TmpName.reset(tempnam(NULL, "pcomn")) ;
   {
      pcomn::binary_ifdstream FileStream
         (PCOMN_ENSURE_POSIX(::open(TmpName.get(), O_RDWR|O_CREAT|O_EXCL, 0644), "open")) ;

      pcomn::binary_ofdstream OFileStream (FileStream.fd(), false) ;

      CPPUNIT_LOG_RUN(pcomn::unit::generate_sequence(OFileStream, SEQ_FROM, SEQ_TO)) ;
      CPPUNIT_LOG_RUN(PCOMN_ENSURE_POSIX(::lseek(OFileStream.fd(), 0, SEEK_SET), "lseek")) ;

      TestInputStream(FileStream) ;
   }
   ::unlink(TmpName.get()) ;
}

void BinaryStreamTests::Test_Readfile()
{
   using namespace pcomn ;

   CPPUNIT_LOG_EQUAL(sys::fileaccess("fooo-baaar-quuuuuux.txt"), sys::ACC_NOEXIST) ;
   CPPUNIT_LOG_EXCEPTION_CODE(readfile("fooo-baaar-quuuuuux.txt"), pcomn::system_error, ENOENT) ;
   CPPUNIT_LOG_EXCEPTION_CODE(readfile("fooo-baaar-quuuuuux.txt"), pcomn::system_error, ENOENT) ;

   CPPUNIT_LOG_EQ(readfile(CPPUNIT_AT_TESTDIR("unittest.empty.lst")), "") ;
   CPPUNIT_LOG_EQ(readfile(CPPUNIT_AT_TESTDIR("unittest.1byte.lst")), "A") ;
   CPPUNIT_LOG_EQ(readfile(CPPUNIT_AT_TESTDIR("RawStreamTests.2.lst")), "42") ;
   CPPUNIT_LOG_EQ(readfile(strslice(CPPUNIT_AT_TESTDIR("RawStreamTests.2.lst"))), "42") ;
   const std::string r1 = readfile(CPPUNIT_AT_TESTDIR("RawStreamTests.Test_Ftream.lst")) ;

   CPPUNIT_LOG_EQ(r1.size(), 60000) ;
   unit::check_sequence(r1.data(), 0, 10000) ;
}

int main(int argc, char *argv[])
{
   return pcomn::unit::run_tests
      <
         BinaryStreamTests
      >
      (argc, argv) ;
}
