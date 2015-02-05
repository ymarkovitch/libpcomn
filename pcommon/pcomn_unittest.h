/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_UNITTEST_H
#define __PCOMN_UNITTEST_H
/*******************************************************************************
 FILE         :   pcomn_unittest.h
 COPYRIGHT    :   Yakov Markovitch, 2003-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Helpers for unit testing with CPPUnit

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   10 Jun 2003
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcomn_macros.h>
#include <pcomn_trace.h>
#include <pcomn_handle.h>
#include <pcomn_tuple.h>
#include <pcomn_meta.h>
#include <pcomn_except.h>
#include <pcomn_string.h>
#include <pcomn_strslice.h>
#include <pcomn_cstrptr.h>
#include <pcomn_path.h>
#include <pcomn_function.h>
#include <pcomn_safeptr.h>

#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestFailure.h>
#include <cppunit/TextTestProgressListener.h>
#include <cppunit/TestPath.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestLogger.h>

#include <fstream>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <tuple>
#include <stdexcept>
#include <mutex>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

// GNU C++ 4.3 and higher uses variadic templates for tuples definition
#if PCOMN_WORKAROUND(__GNUC_VER__, >= 430)
#pragma GCC system_header
#endif
#ifdef __GXX_EXPERIMENTAL_CXX0X__
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#define CPPUNIT_PROGDIR (::pcomn::unit::TestEnvironment::progdir())
#define CPPUNIT_TESTDIR (::pcomn::unit::TestEnvironment::testdir())

#define CPPUNIT_AT_PROGDIR(p1, ...) (::pcomn::unit::TestEnvironment::at_progdir((p1 ##__VA_ARGS__)))
#define CPPUNIT_AT_TESTDIR(p1, ...) (::pcomn::unit::TestEnvironment::at_srcdir((p1 ##__VA_ARGS__)))

namespace pcomn {
namespace unit {

/*******************************************************************************

*******************************************************************************/
template<nullptr_t = nullptr>
struct test_environment {

      friend std::string resolve_test_path(const CppUnit::Test &, const std::string &, bool) ;
      friend int prepare_test_environment(int argc, char ** const argv, const char *diag_profile, const char *title) ;

      static const std::string &progdir() { return _progdir ; }
      static const std::string &testdir() { return _testdir ; }

      static std::string at_progdir(const strslice &path)
      {
         return join_path(progdir(), path) ;
      }
      static std::string at_srcdir(const strslice &path) ;

   private:
      static std::string _progdir ;
      static std::string _testdir ;

   private:
      static std::string resolve_test_path(const CppUnit::Test &tests, const std::string &name, bool top) ;
      static int prepare_test_environment(int argc, char ** const argv, const char *diag_profile, const char *title) ;
      static std::string join_path(const std::string &dir, const strslice &path) ;
} ;

/*******************************************************************************
 test_environment
*******************************************************************************/
template<nullptr_t n>
std::string test_environment<n>::_progdir {"."} ;
template<nullptr_t n>
std::string test_environment<n>::_testdir ;

template<nullptr_t n>
std::string test_environment<n>::join_path(const std::string &dir, const strslice &path)
{
   if (!path || path.size() == 1 && path.front() == '.')
      return dir ;
   if (path.front() == PCOMN_PATH_NATIVE_DELIM)
      return std::string(path) ;
   return (dir + "/").append(path.begin(), path.end()) ;
}

template<nullptr_t n>
std::string test_environment<n>::at_srcdir(const strslice &path)
{
   if (_testdir.empty())
   {
      const char *dir = getenv("PCOMN_TESTDIR") ;
      #ifndef PCOMN_TESTDIR
      ensure_nonzero<std::logic_error>
         (dir, "PCOMN_TESTDIR environment variable value is not specified, cannot use CPPUNIT_TESTDIR") ;
      #else
      dir = P_STRINGIFY_I(PCOMN_TESTDIR) ;
      #endif

      _testdir = dir ;
   }
   return join_path(testdir(), path) ;
}

template<nullptr_t n>
std::string test_environment<n>::resolve_test_path(const CppUnit::Test &tests, const std::string &name, bool top)
{
   using namespace CppUnit ;
   if (!top)
   {
      if (tests.getChildTestCount())
         for (int i = tests.getChildTestCount() ; i-- ;)
         {
            const std::string &child_path = resolve_test_path(*tests.getChildTestAt(i), name, false) ;
            if (!child_path.empty())
               return ("/" + tests.getName()).append(child_path) ;
         }
      else if (pcomn::str::endswith(tests.getName(), name))
         return "/" + tests.getName() ;

      return std::string() ;
   }

   TestPath path ;
   std::string pathstr ;

   if (tests.findTestPath(name, path))
      pathstr = path.toString() ;
   else if (stringchr(name, ':') == -1)
      pathstr = resolve_test_path(tests, "::" + name, false) ;

   return pathstr ;
}

template<nullptr_t n>
int test_environment<n>::prepare_test_environment(int argc, char ** const argv, const char *diag_profile, const char *title)
{
   if (diag_profile && *diag_profile)
      DIAG_INITTRACE(diag_profile) ;

   // Disable syslog output by LOGPX... macros for tests to avoid cluttering syslog
   diag_setmode(diag::DisableSyslog, true) ;

   _progdir = argv && *argv && strrchr(*argv, PCOMN_PATH_NATIVE_DELIM)
      ? std::string(*argv, strrchr(*argv, PCOMN_PATH_NATIVE_DELIM))
      : std::string(".") ;

   if (const char * const testdir = getenv("PCOMN_TESTDIR"))
      _testdir = testdir ;

   CPPUNIT_SETLOG(&std::cout) ;
   if (title && *title)
      CPPUNIT_LOG(title << std::endl) ;
   char namebuf[4096] = "" ;
   CPPUNIT_LOG("Current working directory is '" << (getcwd(namebuf, sizeof namebuf - 1), namebuf) << '\'') ;
   return 0 ;
}

/*******************************************************************************

*******************************************************************************/
typedef test_environment<> TestEnvironment ;

inline std::string resolve_test_path(const CppUnit::Test &tests, const std::string &name, bool top = true)
{
   return TestEnvironment::resolve_test_path(tests, name, top) ;
}
inline int prepare_test_environment(int argc, char ** const argv, const char *diag_profile, const char *title)
{
   return TestEnvironment::prepare_test_environment(argc, argv, diag_profile, title) ;
}


/*******************************************************************************
                     struct ostream_lock
*******************************************************************************/
template<typename Tag>
struct ostream_lock {
      ostream_lock(std::ostream &os) ;
      ostream_lock(ostream_lock &&other) : _ostream(other._ostream), _guard(&_lock)
      {
         other._guard = nullptr ;
      }

      ~ostream_lock() ;

      std::ostream &stream() const { return _ostream ; }

   private:
      std::ostream &        _ostream ;
      std::recursive_mutex *_guard ;

      static std::recursive_mutex _lock ;
} ;

template<typename Tag>
std::recursive_mutex ostream_lock<Tag>::_lock ;

template<typename Tag>
ostream_lock<Tag>::ostream_lock(std::ostream &os) :
   _ostream(os), _guard(&_lock)
{
   _lock.lock() ;
}

template<typename Tag>
ostream_lock<Tag>::~ostream_lock()
{
   if (_guard)
   {
      stream().flush() ;
      _lock.unlock() ;
   }
}

typedef ostream_lock<std::ostream> StreamLock ;

/*******************************************************************************

*******************************************************************************/
template<typename Tag>
struct unique_locked_ostream {
      unique_locked_ostream(unique_locked_ostream &&) = default ;

      unique_locked_ostream(std::ostream &unowned_stream) ;
      explicit unique_locked_ostream(std::ostream *owned_stream) ;
      explicit unique_locked_ostream(const strslice &filename) ;

      ~unique_locked_ostream() ;

      std::ostream &stream() const { return _lock.stream() ; }

   private:
      safe_ref<std::ostream>  _streamp ;
      ostream_lock<Tag>       _lock ;
} ;

template<typename Tag>
unique_locked_ostream<Tag>::unique_locked_ostream(std::ostream *owned_stream) :
   _streamp(owned_stream), _lock(_streamp.get())
{}

template<typename Tag>
unique_locked_ostream<Tag>::unique_locked_ostream(std::ostream &unowned_stream) :
   _streamp(unowned_stream), _lock(_streamp.get())
{}

template<typename Tag>
unique_locked_ostream<Tag>::unique_locked_ostream(const strslice &filename) :
   unique_locked_ostream(new std::ofstream(std::string(filename).c_str()))
{
   PCOMN_THROW_IF(!stream(), environment_error, "Cannot open " P_STRSLICEQF " for writing", P_STRSLICEV(filename)) ;
}

template<typename Tag>
unique_locked_ostream<Tag>::~unique_locked_ostream() = default ;

/*******************************************************************************
                     class TestProgressListener
*******************************************************************************/
template<nullptr_t = nullptr>
class TestListener : public CppUnit::TextTestProgressListener {
   public:
      void startTest(CppUnit::Test *test)
      {
         setCurrentName(test) ;
         CPPUNIT_LOG(std::endl << std::endl << "*** " << testFullName() << std::endl) ;
      }

      void addFailure(const CppUnit::TestFailure &failure)
      {
         CPPUNIT_LOG((failure.isError() ? "ERROR" : "FAILURE") << std::endl
                     << failure.thrownException()->what() << std::endl) ;
      }

      static const char *testFullName() { return _current_fullname ; }
      static const char *testShortName() { return _current_shortname ; }

   private:
      static char _current_fullname[2048] ;
      static const char *_current_shortname ;

      static void setCurrentName(CppUnit::Test *test) ;
      static void resetCurrentName() { *_current_fullname = 0 ; }
} ;

template<nullptr_t n>
char TestListener<n>::_current_fullname[2048] ;
template<nullptr_t n>
const char *TestListener<n>::_current_shortname = _current_fullname ;

template<nullptr_t n>
void TestListener<n>::setCurrentName(CppUnit::Test *test)
{
   const std::string &name = test->getName() ;
   const size_t len = std::min(sizeof _current_fullname - 1, name.size()) ;
   strncpy(_current_fullname, name.c_str(), len) ;
   _current_fullname[len] = 0 ;
   if ((_current_shortname = strrchr(_current_fullname, ':')) != NULL)
      ++_current_shortname ;
   else
      _current_shortname = _current_fullname ;
}

typedef TestListener<> TestProgressListener ;

/*******************************************************************************
                     class TestRunner
*******************************************************************************/
class TestRunner : public CppUnit::TextUi::TestRunner {
      typedef CppUnit::TextUi::TestRunner ancestor ;
   public:
      TestRunner(CppUnit::Outputter *outputter = NULL) :
         ancestor(outputter)
      {
         eventManager().addListener(&_listener) ;
      }

      bool run(const std::string &test_name = "")
      {
         return ancestor::run(test_name, false, true, false) ;
      }

      const CppUnit::TestSuite *suite() const { return m_suite ; }

      static const char *testFullName() { return TestProgressListener::testFullName() ; }
      static const char *testShortName() { return TestProgressListener::testShortName() ; }

   private:
      TestProgressListener _listener ;
} ;

/******************************************************************************/
/** Unique test fixture base class

 To get the path to a file in the unittest sources directory:
  @code
  at_testdir_abs("foo.txt") ;
  at_testdir_abs("bar/foo.lst") ;
  @endcode

 To get the path to a file in the directory, which is automatically created for every
  test run:

  @code
  at_data_dir("foo.out") ;
  at_data_dir_abs("bar.out") ;
  @endcode
*******************************************************************************/
template<const char *private_dirname>
class TestFixture : public CppUnit::TestFixture {
   public:
      typedef ostream_lock<TestFixture<private_dirname> > locked_out ;

      const char *testname() const { return TestRunner::testShortName() ; }

      /// Get the temporary writeable directory allocated/dedicated for the runnning test
      /// function (not fixture!)
      const std::string &dataDir() const
      {
         ensure_datadir() ;
         return _datadir ;
      }
      /// @overload
      const std::string &data_dir() const { return dataDir() ; }

      /// Get per-test-function writeable output file name ($TESTNAME.out)
      const std::string &data_file() const { return _datafile ; }

      /// Get thread-locked per-test-function output data stream (@see data_file())
      locked_out data_ostream() const ;

      /// Get relative path for a filename in the temporary writeable directory (@see
      /// data_dir())
      ///
      /// The path is relative to the current working directory
      std::string at_data_dir(const strslice &filename) const ;
      /// Get absolute path for a filename in the temporary writeable directory (@see data_dir())
      std::string at_data_dir_abs(const strslice &filename) const ;

      /// Get absolute path for a filename specified relative to test sources directory
      std::string at_src_dir_abs(const strslice &filename) const ;
      /// Same as at_src_dir_abs, backward compatibility
      std::string at_testdir_abs(const strslice &filename) const
      {
         return at_src_dir_abs(filename) ;
      }

      void ensure_data_file_match(const strslice &data_sample_filename = {}) const ;

      virtual void cleanupDirs()
      {
         _datadir_ready = true ;
         CPPUNIT_LOG(_datadir << " cleanup." << std::endl) ;
         CPPUNIT_ASSERT((unsigned)system(("rm -rf " + _datadir).c_str()) <= 1) ;
         _datadir_ready = false ;
      }

      void setUp()
      {
         const char * const dirname = private_dirname ? private_dirname : "test" ;

         char buf[2048] ;
         _data_basedir = CPPUNIT_PROGDIR + "/data" ;
         snprintf(buf, sizeof buf, "%s/%s.%s", pcomn::str::cstr(_data_basedir), dirname, testname()) ;
         _datadir = buf ;
         snprintf(buf, sizeof buf, "%s/%s.out", pcomn::str::cstr(_data_basedir), testname()) ;
         _datafile = path::abspath<std::string>(buf) ;

         cleanupDirs() ;
      }

      void tearDown()
      {
         _out.reset() ;
      }
   private:
      std::string    _data_basedir ;
      std::string    _datadir ;
      std::string    _datafile ;
      mutable bool   _datadir_ready ; /* true is datadir is created */
      mutable std::unique_ptr<std::ofstream> _out ;

      void ensure_datadir() const ;
} ;

template<const char *private_dirname>
__noinline void TestFixture<private_dirname>::ensure_datadir() const
{
   if (_datadir_ready)
      return ;
   CPPUNIT_ASSERT(!system(("mkdir -p " + _datadir).c_str())) ;
   _datadir_ready = true ;
}

template<const char *private_dirname>
__noinline std::string TestFixture<private_dirname>::at_data_dir(const strslice &filename) const
{
   return path::joinpath<std::string>(data_dir(), filename) ;
}

template<const char *private_dirname>
__noinline std::string TestFixture<private_dirname>::at_data_dir_abs(const strslice &filename) const
{
   return path::abspath<std::string>(this->at_data_dir(filename)) ;
}

template<const char *private_dirname>
__noinline std::string TestFixture<private_dirname>::at_src_dir_abs(const strslice &filename) const
{
   return path::abspath<std::string>(CPPUNIT_AT_TESTDIR(filename)) ;
}

template<const char *private_dirname>
typename TestFixture<private_dirname>::locked_out TestFixture<private_dirname>::data_ostream() const
{
   if (!_out)
   {
      locked_out guard (std::cout) ;
      if (!_out)
      {
         // Ensure $CPPUNIT_PROGDIR/data is here
         CPPUNIT_ASSERT(_datadir_ready || !system(("mkdir -p " + _data_basedir).c_str())) ;
         std::unique_ptr<std::ofstream> new_stream (new std::ofstream(data_file().c_str())) ;
         PCOMN_THROW_IF(!*new_stream, environment_error, "Cannot open " P_STRSLICEQF " for writing", P_STRSLICEV(data_file())) ;
         _out = std::move(new_stream) ;
      }
   }
   return locked_out(*_out) ;
}

template<const char *private_dirname>
void TestFixture<private_dirname>::ensure_data_file_match(const strslice &data_sample_filename) const
{
   static const char diffopts[] = "-u" ;
   const std::string sample_filename { at_src_dir_abs(data_sample_filename
                                                      ? std::string(data_sample_filename)
                                                      : std::string(path::basename(data_file())) + ".tst") } ;
   char command[2048] ;
   auto rundiff = [&](const char *opt, const char *redir, const char *redir_ext)
   {
      bufprintf(command, "diff %s %s '%s' '%s'%s%s%s",
                diffopts, opt, sample_filename.c_str(), data_file().c_str(),
                *redir || *redir_ext ? " >" : "", redir, redir_ext) ;
      CPPUNIT_LOG_LINE("\n" << command) ;
      const int status = system(command) ;
      if (status < 0)
         CPPUNIT_FAIL("Error running diff command") ;
      switch (WEXITSTATUS(status))
      {
         case 0:  return true ;
         case 1:  return false ;
         default: CPPUNIT_FAIL(bufprintf(command, "Either '%s' or '%s' does not exist", data_file().c_str(), sample_filename.c_str())) ;
      }
   } ;

   if (_out)
      data_ostream().stream() << std::flush ;

   if (!rundiff("-q", "", ""))
   {
      // There is a difference
      rundiff("", data_file().c_str(), ".diff") ;
      CPPUNIT_FAIL(bufprintf(command, "'%s' and '%s' differ", data_file().c_str(), sample_filename.c_str())) ;
   }
}

/*******************************************************************************
 Helper functions
*******************************************************************************/
template<typename T>
std::string to_string(const T &value)
{
   return CppUnit::assertion_traits<T>::toString(value) ;
}

template<typename Char, class Traits>
bool equal_streams(std::basic_istream<Char, Traits> *lhs_stream,
                   std::basic_istream<Char, Traits> *rhs_stream)
{
   typedef std::basic_istream<Char, Traits> istream_type ;
   if (lhs_stream == rhs_stream)
      return true ;
   if (!lhs_stream || !rhs_stream)
      return false ;
   istream_type &lhs = *lhs_stream ;
   istream_type &rhs = *rhs_stream ;

   std::basic_string<Char, Traits>
      lineLeft,
      lineRight ;
   while (lhs && rhs)
   {
      do std::getline(lhs, lineLeft) ;
      while(lineLeft.empty() && lhs) ;
      do std::getline(rhs, lineRight) ;
      while(lineRight.empty() && rhs) ;
      if (lineLeft != lineRight)
         return false ;
   }
   return lhs.eof() && rhs.eof() ;
}

template<typename Char, class Traits>
inline bool equal_streams(std::basic_istream<Char, Traits> *lhs_stream,
                          std::basic_istream<Char, Traits> &&rhs_stream)
{
   return equal_streams(lhs_stream, &rhs_stream) ;
}

template<typename Char, class Traits>
inline bool equal_streams(std::basic_istream<Char, Traits> &&lhs_stream,
                          std::basic_istream<Char, Traits> *rhs_stream)
{
   return equal_streams(&lhs_stream, rhs_stream) ;
}

template<typename Char, class Traits>
inline bool equal_streams(std::basic_istream<Char, Traits> &&lhs_stream,
                          std::basic_istream<Char, Traits> &&rhs_stream)
{
   return equal_streams(&lhs_stream, &rhs_stream) ;
}

static std::string full_file(FILE *file)
{
   PCOMN_ENSURE_ARG(file) ;
   char buf[4096] ;
   std::string result ;
   while (!feof(file))
   {
      CPPUNIT_ASSERT(!ferror(file)) ;
      result.append(buf, fread(buf, 1, sizeof buf, file)) ;
   }
   return result ;
}

template<typename S>
__noinline typename pcomn::enable_if_strchar<S, char, std::string>::type
full_file(const S &name)
{
   return full_file(pcomn::FILE_safehandle(fopen(pcomn::str::cstr(name), "r")).handle()) ;
}

template<typename S>
__noinline typename pcomn::enable_if_strchar<S, char, string_vector>::type
file_lines(const S &name)
{
   std::ifstream file (pcomn::str::cstr(name)) ;
   CPPUNIT_ASSERT(file) ;
   string_vector result ;
   for (std::string line ; std::getline(file, line) ; result.push_back(line)) ;
   return result ;
}

template<typename Char, size_t n>
inline Char *fillbuf(Char (&buf)[n], Char filler)
{
   for (unsigned i = 0 ; i < n ; buf[i++] = filler) ;
   return buf ;
}

template<typename Char, size_t n>
inline Char *fillbuf(Char (&buf)[n]) { return fillbuf(buf, (Char)'\xCC') ; }

template<typename Char, size_t n>
inline Char *fillstrbuf(Char (&buf)[n], Char filler)
{
   fillbuf(buf, filler) ;
   buf[n-1] = 0 ;
   return buf ;
}

template<typename Char, size_t n>
inline Char *fillstrbuf(Char (&buf)[n])  { return fillstrbuf(buf, (Char)0) ; }

inline void pause()
{
   std::cerr << "Press ENTER to continue..." << std::flush ;
   getchar() ;
}

const int DWIDTH = 6 ;

#define PCOMN_CHECK_TESTSEQ_BOUNDS(buf, begin, end, width)              \
   PCOMN_THROW_MSG_IF                                                   \
   (begin < end && (snprintf(buf, (width + 1), "%d", end - 1) > (width) || \
                    snprintf(buf, (width + 1), "%*d", (width), begin) > (width)), \
    std::invalid_argument,                                              \
    "%d or %d is out of range: cannot print it into a field of width %d", begin, end, (width))

/*******************************************************************************
 generate_sequence
*******************************************************************************/
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

inline int run_tests(TestRunner &runner, int argc, char ** const argv,
                     const char *diag_profile, const char *title)
{
   std::string test_path ;
   if (argc >= 2)
   {
      test_path = resolve_test_path(*runner.suite(), argv[1]) ;
      if (test_path.empty())
      {
         std::cerr << "Cannot find test '" << argv[1] << "'" << std::endl ;
         return EXIT_FAILURE ;
      }
   }

   // Output syslog messages to stderr to prevent clogging syslog by unittest messages
   // generated by LOGPX macros
   diag::register_syslog(STDERR_FILENO) ;

   prepare_test_environment(argc, argv, diag_profile, title) ;
   return
      !runner.run(test_path) ;
}

} // end of namespace pcomn::unit

} // end of namespace pcomn

/*******************************************************************************
 Unix-specific handling: process forking, pipes, etc.
*******************************************************************************/
#ifdef PCOMN_PL_UNIX
#include <pcomn_string.h>
#include <pcomn_except.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

namespace pcomn {
namespace unit {

struct forkcmd {
      explicit forkcmd(bool wait_term = true) :
         _pid(fork()),
         _status(0),
         _wait(wait_term)
      {
         if (pid())
         {
            std::cout << "Forked " << pid() << std::endl ;
            PCOMN_THROW_MSG_IF(pid() < 0, std::runtime_error, "Error while forking: %s", strerror(errno)) ;
         }
      }

      ~forkcmd()
      {
         if (pid() <= 0)
            return ;
         terminate() ;
      }

      pid_t pid() const { return _pid ; }

      bool is_child() const { return !pid() ; }

      int close()
      {
         PCOMN_THROW_MSG_IF(!pid(), std::runtime_error, "Child is already terminated") ;
         PCOMN_THROW_MSG_IF(terminate() < 0, std::runtime_error, "Error while terminating: %s", strerror(errno)) ;
         return _status ;
      }

   private:
      const std::string _cmd ;
      pid_t             _pid ;
      int               _status ;
      bool              _wait ;

      int terminate()
      {
         if (pid() <= 0)
            return 0 ;
         pid_t p = _pid ;
         _pid = 0 ;
         if (_wait)
            return waitpid(p, &_status, 0) ;
         if (int result = waitpid(p, &_status, WNOHANG))
            return result ;
         std::cout << "Killing " << p << std::endl ;
         if (int result = kill(p, SIGTERM))
            return result ;
         return waitpid(p, &_status, 0) ;
      }

      forkcmd(const forkcmd &) ;
} ;

/*******************************************************************************
                            struct spawncmd
*******************************************************************************/
struct spawncmd {
      explicit spawncmd(const std::string &cmd, bool wait_term = true) :
         _cmd(cmd),
         _pid(fork()),
         _status(0),
         _wait(wait_term)
      {
         if (pid())
         {
            std::cout << "Spawned " << pid() << std::endl ;
            PCOMN_THROW_MSG_IF(pid() < 0, std::runtime_error, "Error attempting to spawn shell command '%s': %s", cmd.c_str(), strerror(errno)) ;
         }
         else
         {
            setsid() ;
            execl("/bin/sh", "sh", "-c", cmd.c_str(), NULL) ;
            exit(127) ;
         }
      }

      ~spawncmd()
      {
         if (pid() <= 0)
            return ;
         terminate() ;
      }

      pid_t pid() const { return _pid ; }

      int close()
      {
         PCOMN_THROW_MSG_IF(!pid(), std::runtime_error, "Child is already terminated") ;
         PCOMN_THROW_MSG_IF(terminate() < 0, std::runtime_error, "Error terminating shell command '%s': %s", _cmd.c_str(), strerror(errno)) ;
         PCOMN_THROW_MSG_IF(_status == 127, std::runtime_error, "Failure running the shell. Cannot run shell command '%s'", _cmd.c_str()) ;
         return _status ;
      }

   private:
      const std::string _cmd ;
      pid_t             _pid ;
      int               _status ;
      bool              _wait ;

      int terminate()
      {
         if (pid() <= 0)
            return 0 ;
         pid_t p = _pid ;
         _pid = 0 ;
         if (_wait)
            return waitpid(p, &_status, 0) ;
         if (int result = waitpid(p, &_status, WNOHANG))
            return result ;
         std::cout << "Killing " << p << std::endl ;
         if (int result = kill(-getpgid(p), SIGTERM))
            return result ;
         return waitpid(p, &_status, 0) ;
      }

      spawncmd(const spawncmd &) ;
} ;

struct netpipe : private spawncmd {

      netpipe(unsigned inport, unsigned outport, bool wait_term = true) :
         spawncmd(pcomn::strprintf("nc -vv -l -p %u | tee /dev/stderr | nc -vv localhost %u", inport, outport))
      {
         sleep(1) ;
      }
} ;

} // end of namespace pcomn::unit
} // end of namespace pcomn

#endif /* PCOMN_PL_UNIX */

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

/*******************************************************************************
 Containter traits.
*******************************************************************************/
namespace CppUnit {

struct stringify_item {
      stringify_item(std::string &result, char delimiter) :
         _result(result), _delimiter(delimiter), _count(0)
      {}

      template<typename T>
      void operator()(const T &value) const
      {
         (_count++ ? (_result += _delimiter) : _result) += pcomn::unit::to_string(value) ;
      }
   private:
      std::string &     _result ;
      const char        _delimiter ;
      mutable unsigned  _count ;
} ;

struct assertion_traits_eq {
      template<typename T>
      bool operator()(const T &x, const T &y) const { return assertion_traits<T>::equal(x, y) ; }
} ;

template<typename Seq, char delim = ' '>
struct assertion_traits_sequence {

      static bool equal(const Seq &, const Seq &) ;
      static std::string toString(const Seq &) ;
} ;

template<typename Seq, char delim>
bool assertion_traits_sequence<Seq, delim>::equal(const Seq &lhs, const Seq &rhs)
{
   return
      lhs.size() == rhs.size() &&
      std::equal(lhs.begin(), lhs.end(), rhs.begin(), assertion_traits_eq()) ;
}

template<typename Seq, char delim>
std::string assertion_traits_sequence<Seq, delim>::toString(const Seq &value)
{
   std::string result (1, '(') ;
   std::for_each(value.begin(), value.end(), stringify_item(result, delim)) ;
   return result.append(1, ')') ;
}

template<typename Unordered>
struct assertion_traits_unordered {

      static bool equal(const Unordered &lhs, const Unordered &rhs)
      {
         if (&lhs == &rhs || lhs.empty() && rhs.empty())
            return true ;

         typedef typename Unordered::value_type value_type ;
         std::list<value_type> lhs_copy (lhs.begin(), lhs.end()) ;
         std::list<value_type> rhs_copy (rhs.begin(), rhs.end()) ;
         lhs_copy.sort() ;
         rhs_copy.sort() ;
         return lhs_copy == rhs_copy ;
      }

      static std::string toString(const Unordered &value)
      {
         typedef typename Unordered::const_iterator const_iterator ;
         std::string result ;
         for (const_iterator iter (value.begin()), end (value.end()) ; iter != end ; ++iter)
            result.append(pcomn::unit::to_string(*iter)).append(" ") ;
         return result ;
      }
} ;

template<typename K, typename T>
struct assertion_traits<std::map<K, T> > : assertion_traits_sequence<std::map<K, T> > {} ;

#define _CPPUNIT_ASSERTION_TRAITS_SEQUENCE(sequence)                    \
   template<typename T>                                                 \
   struct assertion_traits<sequence<T> > : assertion_traits_sequence<sequence<T> > {}

_CPPUNIT_ASSERTION_TRAITS_SEQUENCE(std::vector) ;
_CPPUNIT_ASSERTION_TRAITS_SEQUENCE(std::list) ;
_CPPUNIT_ASSERTION_TRAITS_SEQUENCE(std::deque) ;
_CPPUNIT_ASSERTION_TRAITS_SEQUENCE(std::set) ;
_CPPUNIT_ASSERTION_TRAITS_SEQUENCE(std::multiset) ;

#undef _CPPUNIT_ASSERTION_TRAITS_SEQUENCE

template<> struct assertion_traits<std::type_info> {
      static bool equal(const std::type_info &x, const std::type_info &y)
      {
         return x == y ;
      }

      static std::string toString(const std::type_info &x)
      {
         return std::string(x.name()) ;
      }
} ;

template<typename T1, typename T2>
struct assertion_traits<std::pair<T1, T2> > {
      typedef std::pair<T1, T2> type ;

      static bool equal(const type &lhs, const type &rhs)
      {
         return
            assertion_traits<T1>::equal(lhs.first, rhs.first) &&
            assertion_traits<T2>::equal(lhs.second, rhs.second) ;
      }

      static std::string toString(const type &value) ;
} ;

template<typename T1, typename T2>
std::string assertion_traits<std::pair<T1, T2> >::toString(const type &value)
{
   return std::string(1, '{') + pcomn::unit::to_string(value.first) + ',' + pcomn::unit::to_string(value.second) + '}' ;
}

template<typename... A>
struct assertion_traits<std::tuple<A...> > {
      typedef std::tuple<A...> type ;

      static bool equal(const type &lhs, const type &rhs)
      {
         return lhs == rhs ;
      }

      static std::string toString(const type &value)
      {
         std::string result (1, '(') ;
         pcomn::tuple_for_each<const type>::apply(value, stringify_item(result, ' ')) ;
         return result.append(1, ')') ;
      }
} ;

template<typename S>
struct assertion_traits_str {
      static bool equal(const S &lhs, const S &rhs) { return lhs == rhs ; }
      static std::string toString(const S &value)
      {
         return quote(pcomn::str::cstr(value), pcomn::str::len(value)) ;
      }

   protected:
      template<typename C>
      static std::string quote(const C *begin, size_t sz) ;
} ;

template<typename S>
template<typename C>
std::string assertion_traits_str<S>::quote(const C *begin, size_t sz)
{
   std::string result ;
   result.reserve(sz + 2) ;
   result += '"' ;
   for (const C *end = begin + sz ; begin != end ; ++begin)
   {
      const C c = *begin ;
      if (c == '\\' || c == '"')
         result += '\\' ;
      result += c ;
   }
   return std::move(result += '"') ;
}

template<typename C>
struct assertion_traits<std::basic_string<C> > : assertion_traits_str<std::basic_string<C> > {} ;

template<typename C>
struct assertion_traits<pcomn::basic_cstrptr<C> > : assertion_traits_str<pcomn::basic_cstrptr<C> > {} ;

template<typename C>
struct assertion_traits<pcomn::basic_strslice<C> > : assertion_traits_str<pcomn::basic_strslice<C> > {
      static std::string toString(const pcomn::basic_strslice<C> &value)
      {
         return assertion_traits_str<pcomn::basic_strslice<C> >::quote(value.begin(), value.size()) ;
      }
} ;

/*******************************************************************************
 Traits for CPPUNIT_LOG_EXCEPTION_CODE and CPPUNIT_LOG_EXCEPTION_MSG
*******************************************************************************/
template<typename ExceptionType>
struct ExpectedExceptionCodeTraits {
      static void expectedException(long code, const long *actual_code) ;
      static void expectedException(const pcomn::strslice &expected_substr,
                                    const pcomn::strslice &actual_msg) ;
} ;

template<typename ExceptionType>
void ExpectedExceptionCodeTraits<ExceptionType>::expectedException(long expected_code,
                                                                   const long *actual_code)
{
   std::string message ("Expected exception of type ") ;
   char codebuf[32] ;
   sprintf(codebuf, "%ld", expected_code) ;
   message
      .append(PCOMN_TYPENAME(ExceptionType))
      .append(", errcode=")
      .append(codebuf)
      .append(", but got ") ;
   if (!actual_code)
      message += "none" ;
   else
   {
      sprintf(codebuf, "%ld", *actual_code) ;
      message
         .append("errcode=")
         .append(codebuf) ;
   }
   throw CppUnit::Exception(CppUnit::Message(message)) ;
}

template<typename ExceptionType>
void ExpectedExceptionCodeTraits<ExceptionType>::expectedException(const pcomn::strslice &expected_substr,
                                                                   const pcomn::strslice &actual_msg)
{
   if (std::search(actual_msg.begin(), actual_msg.end(), expected_substr.begin(), expected_substr.end()) != actual_msg.end())
      return ;

   std::string message ("Expected exception ") ;
   message
      .append(PCOMN_TYPENAME(ExceptionType))
      .append(" with message containing '").append(expected_substr.begin(), expected_substr.end()).append("'")
      .append(", but got the message '").append(actual_msg.begin(), actual_msg.end()).append("'") ;
   throw CppUnit::Exception(CppUnit::Message(message)) ;
}

} // end of namespace CppUnit

/*******************************************************************************
 Output to locked streams
*******************************************************************************/
template<typename T, typename Tag>
inline std::ostream &operator<<(const pcomn::unit::ostream_lock<Tag> &os, T &&v)
{
   return os.stream() << std::forward<T>(v) ;
}

template<typename T, typename Tag>
inline std::ostream &operator<<(const pcomn::unit::unique_locked_ostream<Tag> &os, T &&v)
{
   return os.stream() << std::forward<T>(v) ;
}

/*******************************************************************************
 Containter constructor macros.
*******************************************************************************/
#define CPPUNIT_STRING(value) (::pcomn::unit::to_string(value))
#define CPPUNIT_CONTAINER(type, items) (::pcomn::container_inserter< type >() items .container())

#define CPPUNIT_STRVECTOR(items)    CPPUNIT_CONTAINER(std::vector<std::string>, items)
#define CPPUNIT_STRSET(items)       CPPUNIT_CONTAINER(std::set<std::string>, items)
#define CPPUNIT_STRMAP(type, items) CPPUNIT_CONTAINER(P_PASS(std::map<std::string, type >), items)

// Extremely inefficient, but it is all the same for testing!
template<typename Container>
Container CPPUNIT_SORTED(const Container &container)
{
   Container result (container) ;
   std::sort(result.begin(), result.end()) ;
   return std::move(result) ;
}

template<typename Container>
Container &CPPUNIT_SORTED(Container &&container)
{
   std::sort(container.begin(), container.end()) ;
   return container ;
}

#endif /* __PCOMN_UNITTEST_H */
