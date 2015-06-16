/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_TESTHELPERS_H
#define __PCOMN_TESTHELPERS_H
/*******************************************************************************
 FILE         :   pcomn_testhelpers.h
 COPYRIGHT    :   Yakov Markovitch, 2003-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Helpers for unit testing with CPPUnit

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   10 Jun 2003
*******************************************************************************/
#include <pcomn_unittest.h>

#ifdef __GXX_EXPERIMENTAL_CXX0X__
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

namespace pcomn {
namespace unit {

/*******************************************************************************
 generate_sequence
*******************************************************************************/
const int DWIDTH = 6 ;

#define PCOMN_CHECK_TESTSEQ_BOUNDS(buf, begin, end, width)              \
   PCOMN_THROW_MSG_IF                                                   \
   (begin < end && (snprintf(buf, (width + 1), "%d", end - 1) > (long long)(width) || \
                    snprintf(buf, (width + 1), "%*d", (width), begin) > (long long)(width)), \
    std::invalid_argument,                                              \
    "%d or %d is out of range: cannot print it into a field of width %d", begin, end, (width))

template<class OStream>
pcomn::disable_if_t<std::is_pointer<OStream>::value, OStream &>
generate_sequence(OStream &os, int begin, int end)
{
   const size_t bufsz = DWIDTH + 1 ;
   char buf[bufsz] ;

   PCOMN_CHECK_TESTSEQ_BOUNDS(buf, begin, end, DWIDTH) ;

   if (begin < end)
      // "begin" is already printed by PCOMN_CHECK_TESTSEQ_BOUNDS
      for(os.write(buf, DWIDTH) ; ++begin < end ; os.write(buf, DWIDTH))
         snprintf(buf, bufsz, "%*d", DWIDTH, begin)  ;

   return os ;
}

inline void *generate_sequence(void *buf, int begin, int end)
{
   char tmpbuf[DWIDTH + 1] ;

   PCOMN_CHECK_TESTSEQ_BOUNDS(tmpbuf, begin, end, DWIDTH) ;

   // "begin" is already printed by PCOMN_CHECK_TESTSEQ_BOUNDS
   if (begin < end)
   {
      char *cbuf = (char *)buf ;
      for (memcpy(buf, tmpbuf, DWIDTH) ; ++begin < end ; memcpy(cbuf += DWIDTH, tmpbuf, DWIDTH))
         snprintf(tmpbuf, DWIDTH + 1, "%*d", DWIDTH, begin) ;
   }
   return buf ;
}

template<class IStream>
pcomn::disable_if_t<std::is_pointer<IStream>::value, void>
checked_read_sequence(IStream &is, int from, int to)
{
   CPPUNIT_LOG("Reading from " << from << " to " << to << " through " << CPPUNIT_TYPENAME(IStream) << std::endl) ;

   const int begin = from ;
   for (char buf[DWIDTH + 1] = "\0\0\0\0\0\0" ; from < to ; ++from)
      if (is.read(buf, sizeof buf - 1))
      {
         char *end ;
         CPPUNIT_ASSERT_EQUAL((long)from, strtol(buf, &end, 10)) ;
         CPPUNIT_EQUAL(buf + DWIDTH, end) ;
      }
      else
      {
         CPPUNIT_LOG((is.eof() ? "EOF" : "Failure") << " reading item " << from
                     << " at offset " << (from - begin)*DWIDTH
                     << " from " << CPPUNIT_TYPENAME(is) << std::endl) ;
         CPPUNIT_ASSERT(false) ;
      }
   CPPUNIT_LOG("OK" << std::endl) ;
}

inline void check_sequence(const void *buf, int from, int to)
{
   char *cbuf = (char *)buf ;
   for (char tmpbuf[DWIDTH + 1] ; from < to ; ++from, cbuf += DWIDTH)
   {
      char *end ;

      memcpy(memset(tmpbuf, 0, sizeof tmpbuf), cbuf, DWIDTH) ;
      CPPUNIT_EQUAL(strtol(tmpbuf, &end, 10), (long)from) ;
      CPPUNIT_EQUAL(tmpbuf + DWIDTH, end) ;
   }
}

inline void checked_read_sequence(const void *buf, int from, int to)
{
   CPPUNIT_LOG("Checking buffer " << buf << " from " << from << " to " << to << std::endl) ;
   check_sequence(buf, from, to) ;
   CPPUNIT_LOG("OK" << std::endl) ;
}

/*******************************************************************************
 generate_seqn<>
*******************************************************************************/
template<unsigned n, class OStream>
pcomn::disable_if_t<std::is_pointer<OStream>::value, OStream &>
generate_seqn(OStream &os, int begin, int end)
{
   const size_t bufsz = n + 1 ;
   char buf[bufsz] ;

   PCOMN_CHECK_TESTSEQ_BOUNDS(buf, begin, end, n - 1) ;
   buf[n-1] = '\n' ;

   if (begin < end)
      // "begin" is already printed by PCOMN_CHECK_TESTSEQ_BOUNDS
      for(os.write(buf, n) ; ++begin < end ; os.write(buf, n))
         snprintf(buf, bufsz, "%*d\n", n - 1, begin)  ;

   return os ;
}

template<unsigned n>
void *generate_seqn(void *buf, int begin, int end)
{
   const size_t bufsz = n + 1 ;
   char tmpbuf[bufsz] ;

   PCOMN_CHECK_TESTSEQ_BOUNDS(tmpbuf, begin, end, (int)n - 1) ;
   tmpbuf[n-1] = '\n' ;

   // "begin" is already printed by PCOMN_CHECK_TESTSEQ_BOUNDS
   if (begin < end)
   {
      char *cbuf = (char *)buf ;
      for (memcpy(buf, tmpbuf, n) ; ++begin < end ; memcpy(cbuf += n, tmpbuf, n))
         snprintf(tmpbuf, bufsz, "%*d\n", n - 1, begin) ;
   }
   return buf ;
}

template<unsigned n, typename S>
typename pcomn::enable_if_strchar<S, char, void>::type
generate_seqn_file(const S &filename, int begin, int end)
{
   std::ofstream os (pcomn::str::cstr(filename)) ;
   PCOMN_THROW_MSG_IF(!os, std::runtime_error, "Cannot open '%s' for writing", pcomn::str::cstr(filename)) ;
   generate_seqn<n>(os, begin, end) ;
}

template<unsigned n, typename S>
typename pcomn::enable_if_strchar<S, char, void>::type
generate_seqn_file(const S &filename, int end = 0)
{
   generate_seqn_file<n>(filename, 0, end) ;
}

template<unsigned n, class IStream>
pcomn::disable_if_t<std::is_pointer<IStream>::value, void>
checked_read_seqn(IStream &is, int from, int to)
{
   CPPUNIT_LOG("Reading from " << from << " to " << to << " through " << CPPUNIT_TYPENAME(IStream) << std::endl) ;

   const size_t bufsz = n + 1 ;
   char buf[bufsz] ;
   const int begin = from ;
   for (memset(buf, 0, bufsz) ; from < to ; ++from)
      if (is.read(buf, sizeof buf - 1))
      {
         char *end ;
         CPPUNIT_ASSERT_EQUAL((long)from, strtol(buf, &end, 10)) ;
         CPPUNIT_EQUAL(buf + (n - 1), end) ;
         CPPUNIT_EQUAL(*end, '\n') ;
      }
      else
      {
         CPPUNIT_LOG((is.eof() ? "EOF" : "Failure") << " reading item " << from
                     << " at offset " << (from - begin)*n
                     << " from " << CPPUNIT_TYPENAME(is) << std::endl) ;
         CPPUNIT_ASSERT(false) ;
      }
   CPPUNIT_LOG("OK" << std::endl) ;
}

template<unsigned n, typename S>
typename pcomn::enable_if_strchar<S, char, void>::type
checked_read_seqn_file(const S &filename, int begin, int end)
{
   std::ifstream is (pcomn::str::cstr(filename)) ;
   PCOMN_THROW_MSG_IF(!is, std::runtime_error, "Cannot open '%s' for reading", pcomn::str::cstr(filename)) ;
   checked_read_seqn<n>(is, begin, end) ;
}

template<unsigned n>
void check_seqn(const void *buf, int from, int to)
{
   const char *cbuf = (const char *)buf ;
   for (char tmpbuf[n + 1] ; from < to ; ++from, cbuf += n)
   {
      char *end ;
      memcpy(memset(tmpbuf, 0, sizeof tmpbuf), cbuf, n) ;
      CPPUNIT_EQUAL(strtol(tmpbuf, &end, 10), (long)from) ;
      CPPUNIT_EQUAL(tmpbuf + (n - 1), end) ;
      CPPUNIT_EQUAL(*end, '\n') ;
   }
}

template<unsigned n>
void checked_read_seqn(const void *buf, int from, int to)
{
   CPPUNIT_LOG("Checking buffer " << buf << " from " << from << " to " << to << std::endl) ;
   check_seqn<n>(buf, from, to) ;
   CPPUNIT_LOG("OK" << std::endl) ;
}

#undef PCOMN_CHECK_TESTSEQ_BOUNDS

template<typename S>
typename pcomn::enable_if_strchar<S, char, void>::type
generate_file(const S &filename, const pcomn::strslice &content)
{
   std::ofstream os (pcomn::str::cstr(filename)) ;
   PCOMN_THROW_MSG_IF(!os, std::runtime_error, "Cannot open '%s' for writing", pcomn::str::cstr(filename)) ;
   os.write(content.begin(), content.size()) ;
}

} // end of namespace pcomn::unit
} // end of namespace pcomn

/*******************************************************************************
 "Hello, world!" in different languages and encodings.
*******************************************************************************/
#define PCOMN_HELLO_WORLD_EN_UTF8                        \
   "A greeting to the world in English: 'Hello, world!'"
#define PCOMN_HELLO_WORLD_EN_UCS                            \
   L"A greeting to the world in English: 'Hello, world!'"

#define PCOMN_HELLO_WORLD_DE_UTF8                                    \
   "Der Gr\xc3\xbc\xc3\x9f an der Welt auf Deutsch: 'Hallo, Welt!'"

#define PCOMN_HELLO_WORLD_DE_ISO8859_1                      \
   "Der Gr\xfc\xdf an der Welt auf Deutsch: 'Hallo, Welt!'"

#define PCOMN_HELLO_WORLD_DE_UCS                                  \
   L"Der Gr\x00fc\x00df an der Welt auf Deutsch: 'Hallo, Welt!'"

#define PCOMN_HELLO_WORLD_RU_UTF8 "\
\xd0\x9f\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82\xd1\x81\xd1\x82\xd0\xb2\xd0\xb8\xd0\xb5\x20\xd0\xbc\
\xd0\xb8\xd1\x80\xd1\x83\x20\xd0\xbf\xd0\xbe\x2d\xd1\x80\xd1\x83\xd1\x81\xd1\x81\xd0\xba\xd0\xb8\x3a\
\x20\x27\xd0\x9f\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82\x2c\x20\xd0\xbc\xd0\xb8\xd1\x80\x21\x27"

#define PCOMN_HELLO_WORLD_RU_1251 "\
\xcf\xf0\xe8\xe2\xe5\xf2\xf1\xf2\xe2\xe8\xe5\x20\xec\xe8\xf0\xf3\x20\xef\xee\x2d\
\xf0\xf3\xf1\xf1\xea\xe8\x3a\x20\x27\xcf\xf0\xe8\xe2\xe5\xf2\x2c\x20\xec\xe8\xf0\x21\x27"

#define PCOMN_HELLO_WORLD_RU_UCS L"\
\x041f\x0440\x0438\x0432\x0435\x0442\x0441\x0442\x0432\x0438\x0435\x0020\x043c\x0438\x0440\x0443\
\x0020\x043f\x043e\x002d\x0440\x0443\x0441\x0441\x043a\x0438\x003a\x0020\x0027\x041f\x0440\x0438\
\x0432\x0435\x0442\x002c\x0020\x043c\x0438\x0440\x0021\x0027"

#ifdef PCOMN_PL_WINDOWS
#define PCOMN_HELLO_WORLD_RU_CHAR PCOMN_HELLO_WORLD_RU_1251
#define PCOMN_HELLO_WORLD_DE_CHAR PCOMN_HELLO_WORLD_DE_ISO8859_1
#else
#define PCOMN_HELLO_WORLD_RU_CHAR PCOMN_HELLO_WORLD_RU_UTF8
#define PCOMN_HELLO_WORLD_DE_CHAR PCOMN_HELLO_WORLD_DE_UTF8
#endif

#define PCOMN_HELLO_WORLD_EN_CHAR PCOMN_HELLO_WORLD_EN_UTF8

/*******************************************************************************
 Defining test string literals.
*******************************************************************************/
#define PCOMN_DEFINE_TEST_STR(name, value)                              \
   template<> const char * const TestStrings<char>::name = value ;      \
   template<> const wchar_t * const TestStrings<wchar_t>::name = P_CAT(L, #value)

#define PCOMN_DEFINE_TEST_BUF(cvqual, name, value)                      \
   template<> cvqual char TestStrings<char>::name[] = value ;           \
   template<> cvqual wchar_t TestStrings<wchar_t>::name[] = P_CAT(L, #value)


#endif /* __PCOMN_TESTHELPERS_H */
