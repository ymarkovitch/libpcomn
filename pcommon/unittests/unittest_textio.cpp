/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_textio.cpp
 COPYRIGHT    :   Vadim Hohlov, Yakov Markovitch, 2007-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   PCommon library unittests.
                  Text I/O tests.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   10 Apr 2007
*******************************************************************************/
#include <pcomn_unittest.h>
#include <pcomn_textio.h>
#include <pcomn_string.h>
#include <pcomn_rawstream.h>
#include <pcomn_handle.h>
#include <pcomn_iostream.h>
#include <pcomn_fstream.h>

#include <fstream>
#include <sstream>

using namespace pcomn ;

/*******************************************************************************
                     class TextIOTests
*******************************************************************************/
class TextIOTests : public CppUnit::TestFixture {

      void Test_Reading() ;
      void Test_ReadingFile() ;
      void Test_Text_Writer() ;
      void Test_IO_Writers() ;
      void Test_IO_Readers() ;

      CPPUNIT_TEST_SUITE(TextIOTests) ;

      CPPUNIT_TEST(Test_Reading) ;
      CPPUNIT_TEST(Test_ReadingFile) ;
      CPPUNIT_TEST(Test_Text_Writer) ;
      CPPUNIT_TEST(Test_IO_Writers) ;
      CPPUNIT_TEST(Test_IO_Readers) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

/*******************************************************************************
 CollectionTests
*******************************************************************************/
void TextIOTests::Test_Reading()
{
   std::istringstream is("line 1\nline 2\r\n\rline 3 read with small buffer\n") ;

   pcomn::universal_text_reader<std::istream> reader(is) ;
   CPPUNIT_LOG_EQUAL(reader.eoltype(), (bigflag_t)eol_Undefined) ;

   std::string buf ;
   CPPUNIT_LOG_RUN(reader.readline(buf)) ;
   CPPUNIT_LOG_EQUAL(buf, std::string("line 1\n")) ;
   CPPUNIT_LOG_EQUAL(reader.eoltype(), (bigflag_t)eol_LF) ;

   CPPUNIT_LOG_RUN(reader.readline(buf="")) ;
   CPPUNIT_LOG_EQUAL(buf, std::string("line 2\n")) ;
   CPPUNIT_LOG_EQUAL(reader.eoltype(), (bigflag_t)eol_LF) ;

   CPPUNIT_LOG_RUN(reader.readline(buf="")) ;
   CPPUNIT_LOG_EQUAL(buf, std::string("\n")) ;
   CPPUNIT_LOG_EQUAL(reader.eoltype(), (bigflag_t)(eol_CRLF | eol_LF)) ;

   char small_buff[13] ;
   CPPUNIT_LOG_EQUAL(reader.readline(small_buff, sizeof small_buff), sizeof small_buff - 1) ;
   // new line flag - CR sets when the next line start reading
   CPPUNIT_LOG_EQUAL(reader.eoltype(), (bigflag_t)(eol_CRLF | eol_LF | eol_CR)) ;
   CPPUNIT_LOG_ASSERT(strcmp(small_buff, "line 3 read ") == 0) ;
   CPPUNIT_LOG_EQUAL(reader.readline(small_buff, sizeof small_buff), sizeof small_buff - 1) ;
   CPPUNIT_LOG_ASSERT(strcmp(small_buff, "with small b") == 0) ;
   CPPUNIT_LOG_EQUAL(reader.readline(small_buff, sizeof small_buff), sizeof "uffer") ;
   CPPUNIT_LOG_ASSERT(strcmp(small_buff, "uffer\n") == 0) ;
}

void TextIOTests::Test_ReadingFile()
{
   std::ifstream is(CPPUNIT_AT_TESTDIR("unittest_textio.dat").c_str(), std::ios_base::in | std::ios_base::binary) ;

   pcomn::universal_text_reader<std::istream> reader(is) ;

   std::string buf ;
   CPPUNIT_LOG_ASSERT(is) ;
   CPPUNIT_LOG_RUN(reader.readline(buf)) ;
   CPPUNIT_LOG_EQUAL(buf, std::string("string 1\n")) ;
   CPPUNIT_LOG_EQUAL(reader.eoltype(), (bigflag_t)eol_Undefined) ;

   CPPUNIT_LOG_RUN(reader.readline(buf="")) ;
   CPPUNIT_LOG_EQUAL(buf, std::string("string 2\n")) ;
   CPPUNIT_LOG_EQUAL(reader.eoltype(), (bigflag_t)eol_CRLF) ;
}

#ifdef PCOMN_PL_MS
  #define _NEWLINE_STR_     "\r\n"
  #define _NEWLINE_ENDCHAR_ '\n'
#elif PCOMN_PL_MAC
  #define _NEWLINE_STR_     "\r"
  #define _NEWLINE_ENDCHAR_ '\r'
#else // Unix, Linux, Cygwin
  #define _NEWLINE_STR_     "\n"
  #define _NEWLINE_ENDCHAR_ '\n'
#endif

bool check_line(const char **buffer, const char *compare)
{
   const char *start = *buffer ;
   const char *line_end = strchr(start, _NEWLINE_ENDCHAR_) + 1 ;
   *buffer = line_end ;
   return line_end && memcmp(start, compare, line_end - start)==0 ;
}

void TextIOTests::Test_Text_Writer()
{
   std::ostringstream os ;

   pcomn::universal_text_writer<std::ostream> writer(os) ;

   CPPUNIT_LOG_RUN(writer.writeline("first line")) ;
   CPPUNIT_LOG_RUN(writer.writeline("second line")) ;
   CPPUNIT_LOG_RUN(writer.writeline("third line and \nfifth line")) ;
   CPPUNIT_LOG_RUN(writer.write("sixth ")) ;
   CPPUNIT_LOG_RUN(writer.write("line\n")) ;
   CPPUNIT_LOG_RUN(writer.write("seventh line\n")) ;
   CPPUNIT_LOG_RUN(writer.write("eight line and \nninth line")) ;
   CPPUNIT_LOG_RUN(writer.writeline()) ;
   CPPUNIT_LOG_RUN(writer.writeline("")) ;
   CPPUNIT_LOG_RUN(writer.write("last \r line")) ;
   os << '\x00' ;

   const std::string &s = os.str() ;
   const char *buff = s.c_str() ;
   CPPUNIT_LOG(buff) ;
   CPPUNIT_LOG_ASSERT( check_line(&buff, "first line" _NEWLINE_STR_) ) ;
   CPPUNIT_LOG_ASSERT( check_line(&buff, "second line" _NEWLINE_STR_) ) ;
   CPPUNIT_LOG_ASSERT( check_line(&buff, "third line and " _NEWLINE_STR_) ) ;
   CPPUNIT_LOG_ASSERT( check_line(&buff, "fifth line" _NEWLINE_STR_) ) ;
   CPPUNIT_LOG_ASSERT( check_line(&buff, "sixth line" _NEWLINE_STR_) ) ;
   CPPUNIT_LOG_ASSERT( check_line(&buff, "seventh line" _NEWLINE_STR_) ) ;
   CPPUNIT_LOG_ASSERT( check_line(&buff, "eight line and " _NEWLINE_STR_) ) ;
   CPPUNIT_LOG_ASSERT( check_line(&buff, "ninth line" _NEWLINE_STR_) ) ;
   CPPUNIT_LOG_ASSERT( check_line(&buff, _NEWLINE_STR_) ) ;
   CPPUNIT_LOG_ASSERT( strcmp(buff, "last \r line")==0 ) ;
}

void TextIOTests::Test_IO_Writers()
{
   static const char BB[] = "Bye, baby!" ;
   static const char HW[] = "Hello, world!" ;

   std::string OutStr ;
   CPPUNIT_LOG("\n**** Writing to std::string ****\n") ;
   CPPUNIT_LOG_EQUAL(io::write_data(OutStr, "Hello,"), (ssize_t)6) ;
   CPPUNIT_LOG_EQUAL(io::write_data(OutStr, " world!"), (ssize_t)7) ;
   CPPUNIT_LOG_EQUAL(OutStr, std::string("Hello, world!")) ;

   CPPUNIT_LOG("\n**** Writing to std::ostream ****\n") ;
   std::ostringstream os ;
   CPPUNIT_LOG_EQUAL(io::write_data(os, BB), (ssize_t)10) ;
   CPPUNIT_LOG_EQUAL(io::write_data(os, HW, sizeof HW - 1), (ssize_t)(sizeof HW - 1)) ;
   CPPUNIT_LOG_EQUAL(os.str(), std::string("Bye, baby!Hello, world!")) ;

   CPPUNIT_LOG("\n**** Writing to FILE * ****\n") ;
   const std::string FILE_name (CPPUNIT_AT_PROGDIR("Test_IO_Writers.FILE.txt")) ;
   FILE_safehandle file (fopen(FILE_name.c_str(), "w")) ;
   CPPUNIT_LOG_ASSERT(file) ;
   CPPUNIT_LOG_EQUAL(io::write_data(file.get(), BB), (ssize_t)10) ;
   // Don't write the terminating null
   CPPUNIT_LOG_EQUAL(io::write_data(file.get(), HW, sizeof HW - 1), (ssize_t)(sizeof HW - 1)) ;
   CPPUNIT_LOG_RUN(file.reset()) ;
   CPPUNIT_LOG_EQUAL(unit::full_file(FILE_name), std::string("Bye, baby!Hello, world!")) ;
}

template<size_t n>
inline char *strfill(char (&buf)[n], int c)
{
   if (n)
   {
      memset(buf, c, n - 1) ;
      buf[n - 1] = 0 ;
   }
   return buf ;
}

void TextIOTests::Test_IO_Readers()
{
   static const char BB[] = "Bye, baby!" ;
   static const char HW[] = "Hello, world!" ;
   char buf[65536] ;

   strslice EmptySlice ;
   strfill(buf, '#') ;
   CPPUNIT_LOG("\n**** Reading from empty pcomn::strslice ****\n") ;
   CPPUNIT_LOG_EQUAL(io::read_data(EmptySlice, buf, sizeof buf), (ssize_t)0) ;
   CPPUNIT_LOG_EQUAL(strslice(buf, 0, 16), strslice("################")) ;
   CPPUNIT_LOG_EQUAL(EmptySlice, strslice()) ;

   strslice InSlice (BB) ;
   strfill(buf, '#') ;
   CPPUNIT_LOG("\n**** Reading from pcomn::strslice ****\n") ;
   CPPUNIT_LOG_EQUAL(io::read_data(InSlice, buf, sizeof buf), (ssize_t)10) ;
   CPPUNIT_LOG_EQUAL(strslice(buf, 0, 16), strslice("Bye, baby!######")) ;
   CPPUNIT_LOG_EQUAL(InSlice, strslice()) ;

   CPPUNIT_LOG_EXPRESSION(InSlice = HW) ;
   strfill(buf, '+') ;
   CPPUNIT_LOG_EQUAL(io::read_data(InSlice, buf, 7), (ssize_t)7) ;
   CPPUNIT_LOG_EQUAL(strslice(buf, 0, 10), strslice("Hello, +++")) ;
   strfill(buf, '+') ;
   CPPUNIT_LOG_EQUAL(io::read_data(InSlice, buf, 7), (ssize_t)6) ;
   CPPUNIT_LOG_EQUAL(strslice(buf, 0, 10), strslice("world!++++")) ;
   strfill(buf, '+') ;
   CPPUNIT_LOG_EQUAL(io::read_data(InSlice, buf, 7), (ssize_t)0) ;
   CPPUNIT_LOG_EQUAL(strslice(buf, 0, 10), strslice("++++++++++")) ;

   CPPUNIT_LOG("\n**** Reading from pcomn::strslice char-by-char ****\n") ;
   CPPUNIT_LOG_EXPRESSION(InSlice = BB) ;
   CPPUNIT_LOG_EQUAL(io::get_char(InSlice), (int)'B') ;
   CPPUNIT_LOG_EQUAL(io::get_char(InSlice), (int)'y') ;
   CPPUNIT_LOG_EQUAL(io::get_char(InSlice), (int)'e') ;
   CPPUNIT_LOG_EQUAL(io::read_data(InSlice, buf, 6), (ssize_t)6) ;
   CPPUNIT_LOG_EQUAL(io::get_char(InSlice), (int)'!') ;
   CPPUNIT_LOG_EQUAL(io::get_char(InSlice), -1) ;
   CPPUNIT_LOG_EQUAL(io::get_char(InSlice), -1) ;

   const std::string TestFileName (CPPUNIT_AT_TESTDIR("unittest_textio.dat")) ;

   CPPUNIT_LOG("\n**** Reading from FILE * ****\n") ;
   {
      FILE_safehandle file (fopen(TestFileName.c_str(), "r")) ;
      CPPUNIT_LOG_EQUAL(io::get_char(file.get()), (int)'s') ;
      CPPUNIT_LOG_EQUAL(io::get_char(file.get()), (int)'t') ;
      strfill(buf, '#') ;
      CPPUNIT_LOG_EQUAL(io::read_data(file.get(), buf, 6), (ssize_t)6) ;
      CPPUNIT_LOG_EQUAL(strslice(buf, 0, 8), strslice("ring 1##")) ;
      CPPUNIT_LOG_RUN(fseek(file.get(), -1, SEEK_END)) ;
      CPPUNIT_LOG_EQUAL(io::get_char(file.get()), (int)'3') ;
      CPPUNIT_LOG_EQUAL(io::get_char(file.get()), -1) ;
      CPPUNIT_LOG_ASSERT(feof(file.get())) ;
   }

   CPPUNIT_LOG("\n**** Reading from pcomn::binary_istream ****\n") ;
   {
      binary_ifdstream ifdstream (PCOMN_ENSURE_POSIX(open(TestFileName.c_str(), O_RDONLY), "open")) ;

      CPPUNIT_LOG_EQUAL(io::get_char(ifdstream), (int)'s') ;
      CPPUNIT_LOG_EQUAL(io::get_char(ifdstream), (int)'t') ;
      strfill(buf, '#') ;
      CPPUNIT_LOG_EQUAL(io::read_data(ifdstream, buf, 6), (ssize_t)6) ;
      CPPUNIT_LOG_EQUAL(strslice(buf, 0, 8), strslice("ring 1##")) ;
      CPPUNIT_LOG_RUN(lseek(ifdstream.fd(), -1, SEEK_END)) ;
      CPPUNIT_LOG_EQUAL(io::get_char(ifdstream), (int)'3') ;
      CPPUNIT_LOG_EQUAL(io::get_char(ifdstream), -1) ;
      CPPUNIT_LOG_ASSERT(ifdstream.eof()) ;
   }

   CPPUNIT_LOG("\n**** Reading from pcomn::binary_ibufstream ****\n") ;
   {
      binary_ifdstream ifdstream (PCOMN_ENSURE_POSIX(open(TestFileName.c_str(), O_RDONLY), "open")) ;
      binary_ibufstream ibufstream (ifdstream, 4096) ;

      CPPUNIT_LOG_EQUAL(io::get_char(ifdstream), (int)'s') ;
      CPPUNIT_LOG_EQUAL(io::get_char(ifdstream), (int)'t') ;
      strfill(buf, '#') ;
      CPPUNIT_LOG_EQUAL(io::read_data(ifdstream, buf, 6), (ssize_t)6) ;
      CPPUNIT_LOG_EQUAL(strslice(buf, 0, 8), strslice("ring 1##")) ;
   }

   CPPUNIT_LOG("\n**** Reading from pcomn::istream_over_iterator ****\n") ;
   {
      strslice InSlice (BB) ;
      istream_over_iterator<const char *> istream (InSlice.begin(), InSlice.end()) ;

      CPPUNIT_LOG_EQUAL(io::get_char(istream), (int)'B') ;
      CPPUNIT_LOG_EQUAL(io::get_char(istream), (int)'y') ;
      CPPUNIT_LOG_EQUAL(io::get_char(istream), (int)'e') ;
      strfill(buf, '#') ;
      CPPUNIT_LOG_EQUAL(io::read_data(istream, buf, 6), (ssize_t)6) ;
      CPPUNIT_LOG_EQUAL(strslice(buf, 0, 8), strslice(", baby##")) ;
      CPPUNIT_LOG_EQUAL(io::get_char(istream), (int)'!') ;
      CPPUNIT_LOG_EQUAL(io::get_char(istream), -1) ;
   }

   CPPUNIT_LOG("\n**** Reading from std::istrstream ****\n") ;
   {
      std::istringstream is (BB) ;

      CPPUNIT_LOG_EQUAL(io::get_char(is), (int)'B') ;
      CPPUNIT_LOG_EQUAL(io::get_char(is), (int)'y') ;
      CPPUNIT_LOG_EQUAL(io::get_char(is), (int)'e') ;
      strfill(buf, '#') ;
      CPPUNIT_LOG_EQUAL(io::read_data(is, buf, 6), (ssize_t)6) ;
      CPPUNIT_LOG_EQUAL(strslice(buf, 0, 8), strslice(", baby##")) ;
      CPPUNIT_LOG_EQUAL(io::get_char(is), (int)'!') ;
      CPPUNIT_LOG_EQUAL(io::get_char(is), -1) ;
   }
}

/*******************************************************************************
 main
*******************************************************************************/
int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(TextIOTests::suite()) ;
   return pcomn::unit::run_tests(runner, argc, argv, "unittest.diag.ini",
                                 "Universal newline text I/O tests") ;
}
