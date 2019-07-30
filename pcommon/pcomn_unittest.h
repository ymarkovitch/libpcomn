/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_UNITTEST_H
#define __PCOMN_UNITTEST_H
/*******************************************************************************
 FILE         :   pcomn_unittest.h
 COPYRIGHT    :   Yakov Markovitch, 2003-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Helpers for unit testing with CPPUnit

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   10 Jun 2003
*******************************************************************************/
/** @file
 Helper classes, functions, types, macros for CppUnit-based unittests.

 class TestFixture<private_dirname>

 @code
 extern const char FOOBARTEST_FIXTURE[] = "foobar-test" ;
 class FooBarTests : public pcomn::unit::TestFixture<FOOBARTEST_FIXTURE> {...} ;
 @endcode

 CppUnit::assertion_traits_sequence<sequence> for any sequence with std::begin(seq),
 std::end(seq), std::size(seq)
*******************************************************************************/

#include <pcomn_platform.h>
GCC_DIAGNOSTIC_PUSH_IGNORE(unused-result)
#include <pcomn_macros.h>
#include <pcomn_trace.h>
#include <pcomn_tuple.h>
#include <pcomn_meta.h>
#include <pcomn_except.h>
#include <pcomn_string.h>
#include <pcomn_strslice.h>
#include <pcomn_cstrptr.h>
#include <pcomn_path.h>
#include <pcomn_function.h>
#include <pcomn_safeptr.h>
#include <pcomn_unistd.h>
#include <pcomn_handle.h>
#include <pcomn_sys.h>

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
#include <stdexcept>
#include <mutex>
#include <chrono>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#define CPPUNIT_PROGDIR (::pcomn::unit::TestEnvironment::progdir())
#define CPPUNIT_TESTDIR (::pcomn::unit::TestEnvironment::testdir())

#define CPPUNIT_AT_PROGDIR(p1, ...) (::pcomn::unit::TestEnvironment::at_progdir((p1 ##__VA_ARGS__)))
#define CPPUNIT_AT_TESTDIR(p1, ...) (::pcomn::unit::TestEnvironment::at_srcdir((p1 ##__VA_ARGS__)))

namespace pcomn {
namespace unit {

enum DiffOptions : unsigned {
   DF_SENSE_TRAIL_SPACE  = 0x01,
   DF_SENSE_SPACE_CHANGE = 0x02,
   DF_SENSE_TABS         = 0x04,
   DF_SENSE_BLANK_LINES  = 0x08
} ;

PCOMN_DEFINE_FLAG_ENUM(DiffOptions) ;

constexpr DiffOptions DF_SENSE_ALL = DF_SENSE_TRAIL_SPACE|DF_SENSE_SPACE_CHANGE|DF_SENSE_TABS|DF_SENSE_BLANK_LINES ;

/*******************************************************************************
 Test environment
*******************************************************************************/
template<novalue = {}>
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
template<novalue n>
std::string test_environment<n>::_progdir (".") ;
template<novalue n>
std::string test_environment<n>::_testdir ;

template<novalue n>
std::string test_environment<n>::join_path(const std::string &dir, const strslice &path)
{
   if (!path || path.size() == 1 && path.front() == '.')
      return dir ;
   if (path.front() == PCOMN_PATH_NATIVE_DELIM)
      return std::string(path) ;
   return (dir + PCOMN_PATH_NATIVE_DELIM).append(path.begin(), path.end()) ;
}

template<novalue n>
std::string test_environment<n>::at_srcdir(const strslice &path)
{
   if (_testdir.empty())
   {
      const char *dir = getenv("PCOMN_TESTDIR") ;
      #ifndef PCOMN_TESTDIR
      ensure_nonzero<std::logic_error>
         (dir, "PCOMN_TESTDIR environment variable value is not specified, cannot use CPPUNIT_TESTDIR") ;
      #else
      dir = dir ? dir : P_STRINGIFY_I(PCOMN_TESTDIR) ;
      #endif

      _testdir = dir ;
   }
   return join_path(testdir(), path) ;
}

template<novalue n>
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

template<novalue n>
int test_environment<n>::prepare_test_environment(int argc, char ** const argv, const char *diag_profile, const char *title)
{
   #if PCOMN_PL_MS
   if (!IsDebuggerPresent())
   {
      // Prevent Windows from showing dialog boxes on abort
      _set_abort_behavior(0, _CALL_REPORTFAULT);
      _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
      _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
      _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
      _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
      SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX) ;
   }
   #endif

   #ifdef __GLIBC__
   // Suppress printing the program name and a colon by GLIBC's error_at_line()
   error_print_progname = []{} ;
   #endif

   if (diag_profile && *diag_profile)
      DIAG_INITTRACE(diag_profile) ;

   // Output syslog messages to stderr to prevent clogging syslog by unittest messages
   // generated by LOGPX macros
   diag::register_syslog(STDERR_FILENO) ;

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
 Test path resolving
*******************************************************************************/
typedef test_environment<> TestEnvironment ;

inline std::string resolve_test_path(const CppUnit::Test &tests, const std::string &name, bool top = true)
{
   return TestEnvironment::resolve_test_path(tests, name, top) ;
}
inline int prepare_test_environment(int argc, char ** const argv, const char *diag_profile = {},
                                    const char *title = {})
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
      operator std::ostream &() const { return stream() ; }

      template<typename T>
      std::ostream &operator<<(T &&v) const
      {
         return stream() << std::forward<T>(v) ;
      }

      std::ostream &operator<<(std::ios_base &(*f)(std::ios_base&)) const
      {
         return stream() << f ;
      }

      std::ostream &operator<<(std::ostream &(*f)(std::ostream&)) const
      {
         return stream() << f ;
      }

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
      operator std::ostream &() const { return stream() ; }

      template<typename T>
      std::ostream &operator<<(T &&v) const
      {
         return stream() << std::forward<T>(v) ;
      }

      std::ostream &operator<<(std::ios_base &(*f)(std::ios_base&)) const
      {
         return stream() << f ;
      }

      std::ostream &operator<<(std::ostream &(*f)(std::ostream&)) const
      {
         return stream() << f ;
      }

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
template<novalue = {}>
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

template<novalue n>
char TestListener<n>::_current_fullname[2048] ;
template<novalue n>
const char *TestListener<n>::_current_shortname = _current_fullname ;

template<novalue n>
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

   public: // overridden from TestRunner (to avoid hidden virtual function warning)
      void run(CppUnit::TestResult &controller, const std::string &testPath) override
      {
         ancestor::run(controller, testPath) ;
      }

   private:
      TestProgressListener _listener ;
} ;

/***************************************************************************//**
 Unique test fixture base class

 To get the path to a file in the unittest sources directory:
  @code
  at_src_dir_abs("foo.txt") ;
  at_src_dir_abs("bar/foo.lst") ;
  @endcode

 To get the path to a file in the directory, which is automatically created
 for every test, run:
  @code
  at_data_dir("foo.out") ;
  at_data_dir_abs("bar.out") ;
  @endcode

 Files for output:
    data_file(): per-test-function writeable output file name ($TESTNAME.out)
    const std::string &data_file()

    data_ostream() :thread-locked per-test-function output data stream;
    the corresponding file is data_file()
*******************************************************************************/
template<const char *private_dirname>
class TestFixture : public CppUnit::TestFixture {
      #ifdef PCOMN_COMPILER_MS
      PCOMN_STATIC_CHECK(!!private_dirname) ;
      #else
      PCOMN_STATIC_CHECK(private_dirname != nullptr) ;
      #endif
   public:
      typedef ostream_lock<TestFixture<private_dirname>> locked_out ;

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

      const std::string &data_dir_abs() const
      {
         ensure_datadir() ;
         return _datadir_abs ;
      }

      /// Get per-test-function writeable output file name ($TESTNAME.out)
      const std::string &data_file() const
      {
         ensure_datadir() ;
         return _datafile ;
      }

      /// Get writeable output file name ($TESTNAME.$n.out)
      /// @note If @a n is 0, returns data_file()
      std::string data_file(unsigned n) const ;

      /// Get thread-locked per-test-function output data stream (@see data_file())
      locked_out data_ostream() const ;

      /// Get relative path for a filename in the temporary writeable directory (@see
      /// data_dir())
      ///
      /// The path is relative to the current working directory
      std::string at_data_dir(const strslice &filename) const ;
      /// Get absolute path for a filename in the temporary writeable directory
      /// (@see data_dir())
      std::string at_data_dir_abs(const strslice &filename) const ;

      /// Get absolute path for a filename specified relative to test sources directory
      std::string at_src_dir_abs(const strslice &filename) const ;
      /// Same as at_src_dir_abs, backward compatibility
      std::string at_testdir_abs(const strslice &filename) const
      {
         return at_src_dir_abs(filename) ;
      }

      /// Run external diff to compare current content of data_file(out_ndx) with
      /// @a data_sample_filename
      ///
      /// @param data_sample_filename The name of a sample file to compare the result of
      /// test output to data_file(out_ndx) with
      ///
      /// If @a data_sample_filename is not absolute, it is considered relative to the
      /// test sources directory (@see at_src_dir_abs())
      ///
      void ensure_data_file_match(const strslice &data_sample_filename = {}, unsigned out_ndx = 0,
                                  DiffOptions options = {}) const ;

      void ensure_data_file_match(unsigned out_ndx, DiffOptions options) const
      {
         ensure_data_file_match({}, out_ndx, options) ;
      }

      virtual void cleanupDirs()
      {
         _datadir_ready = true ;
         CPPUNIT_LOG(_datadir << " cleanup." << std::endl) ;
         system(("chmod --preserve-root --quiet -R u+w '" + _datadir_abs + "'").c_str()) ;
         CPPUNIT_ASSERT((unsigned)system(("rm --preserve-root -rf '" + _datadir_abs + "'").c_str()) <= 1) ;
         _datadir_ready = false ;
      }

      void setUp() override
      {
         char buf[2048] ;
         _data_basedir = CPPUNIT_PROGDIR + (PCOMN_PATH_DELIMS "data") ;

         snprintf(buf, sizeof buf, "%s%c%s.%s",
                  pcomn::str::cstr(_data_basedir), PCOMN_PATH_NATIVE_DELIM, private_dirname, testname()) ;
         _datadir = buf ;
         _datadir_abs = path::abspath<std::string>(buf) ;

         snprintf(buf, sizeof buf, "%s%c%s.out",
                  pcomn::str::cstr(_data_basedir), PCOMN_PATH_NATIVE_DELIM, testname()) ;
         _datafile = path::abspath<std::string>(buf) ;

         cleanupDirs() ;
      }

      void tearDown() override
      {
         _out.reset() ;
      }
   private:
      std::string    _data_basedir ;
      std::string    _datadir ;
      std::string    _datadir_abs ;
      std::string    _datafile ;
      mutable bool   _datadir_ready ; /* true is datadir is created */
      mutable std::unique_ptr<std::ofstream> _out ;

      void ensure_datadir() const ;

      #ifndef PCOMN_PL_MS
      static bool mkdir_with_parents(const std::string &path)
      {
         return !system(("mkdir -p " + path).c_str()) ;
      }
      #else
      static bool mkdir_with_parents(const std::string &path)
      {
         const auto s = pcomn::sys::filestat(path.c_str()) ;
         return (s.st_mode & S_IFMT) == S_IFDIR || !s && !system(("md " + path).c_str()) ;
      }
      #endif
} ;

template<const char *private_dirname>
__noinline void TestFixture<private_dirname>::ensure_datadir() const
{
   if (_datadir_ready)
      return ;
   CPPUNIT_ASSERT(mkdir_with_parents(_datadir)) ;
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
         CPPUNIT_ASSERT(_datadir_ready || mkdir_with_parents(_data_basedir)) ;
         std::unique_ptr<std::ofstream> new_stream (new std::ofstream(data_file().c_str())) ;
         PCOMN_THROW_IF(!*new_stream, environment_error, "Cannot open " P_STRSLICEQF " for writing", P_STRSLICEV(data_file())) ;
         _out = std::move(new_stream) ;
      }
   }
   return locked_out(*_out) ;
}

template<const char *private_dirname>
__noinline std::string TestFixture<private_dirname>::data_file(unsigned ndx) const
{
   const auto &s = path::splitext(_datafile) ;
   return !ndx ? data_file() : s.first + std::string(".") + std::to_string(ndx) + s.second ;
}

template<const char *private_dirname>
void TestFixture<private_dirname>::ensure_data_file_match(const strslice &data_sample_filename,
                                                          unsigned out_file_ndx, DiffOptions opts) const
{
   static constexpr DiffOptions sense_space = DF_SENSE_TRAIL_SPACE|DF_SENSE_SPACE_CHANGE|DF_SENSE_TABS ;
   const char * const diffopts[] =
   {
      (opts & sense_space)           ? "-u" : ((opts |= sense_space), "-u -w"),
      (opts & DF_SENSE_TRAIL_SPACE)  ? "" : " -Z",
      (opts & DF_SENSE_SPACE_CHANGE) ? "" : " -b",
      (opts & DF_SENSE_TABS)         ? "" : " -E",
      (opts & DF_SENSE_BLANK_LINES)  ? "" : " -B"
   } ;

   const std::string outfile_filename = data_file(out_file_ndx) ;

   const std::string sample_filename { at_src_dir_abs(data_sample_filename
                                                      ? std::string(data_sample_filename)
                                                      : std::string(path::basename(outfile_filename)) + ".tst") } ;

   char command[2048] ;
   auto rundiff = [&](const char *opt, const char *redir, const char *redir_ext)
   {
      bufprintf(command, "diff %s%s%s%s%s" "%s" " '%s' '%s'" "%s%s%s",

                diffopts[0], diffopts[1], diffopts[2], diffopts[3], diffopts[4],
                opt,
                sample_filename.c_str(), outfile_filename.c_str(),
                *redir || *redir_ext ? " >" : "", redir, redir_ext) ;

      CPPUNIT_LOG_LINE("\n" << command) ;
      const int status = system(command) ;
      if (status < 0)
         CPPUNIT_FAIL("DIFF FAILURE: Error running diff command") ;
      switch (WEXITSTATUS(status))
      {
         case 0:  return true ;
         case 1:  return false ;
         default: CPPUNIT_FAIL(bufprintf(command,
                                         "DIFF FAILURE: Either '%s' or '%s' does not exist",
                                         outfile_filename.c_str(), sample_filename.c_str())) ;
      }
      return false ;
   } ;

   if (!out_file_ndx && _out)
      data_ostream().stream() << std::flush ;

   if (!rundiff(" -q", "", ""))
   {
      // There is a difference
      rundiff("", outfile_filename.c_str(), ".diff") ;
      CPPUNIT_FAIL(bufprintf(command, "MISMATCH '%s' and '%s'", outfile_filename.c_str(), sample_filename.c_str())) ;
   }
}

inline int run_tests(TestRunner &runner, int argc, char ** const argv,
                     const char *diag_profile = nullptr, const char *title = nullptr)
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

   prepare_test_environment(argc, argv, diag_profile, title) ;
   return
      !runner.run(test_path) ;
}

template<typename S1, typename S2, typename... SN>
inline void add_test_suites(TestRunner &runner)
{
   runner.addTest(S1::suite()) ;
   add_test_suites<S2, SN...>(runner) ;
}

template<typename S>
inline void add_test_suites(TestRunner &runner)
{
   runner.addTest(S::suite()) ;
}

template<typename TestSuite1, typename... TestSuiteN>
inline int run_tests(int argc, char ** const argv,
                     const char *diag_profile = nullptr, const char *title = nullptr)
{
   pcomn::unit::TestRunner runner ;
   add_test_suites<TestSuite1, TestSuiteN...>(runner) ;
   return
      run_tests(runner, argc, argv, diag_profile, title) ;
}

/******************************************************************************/
/** Convert an object to a string as specified by CppUnit::assertion_traits

 This is in contrast to pcomn::string_cast(), which always uses std::ostream
 output operator to stringify objects
*******************************************************************************/
template<typename T>
std::string to_string(const T &value)
{
   return CppUnit::assertion_traits<T>::toString(value) ;
}

} // end of namespace pcomn::unit
} // end of namespace pcomn

/*******************************************************************************
 Assertion traits for STL containers, pcommon containers, pairs, tuples, ets.
*******************************************************************************/
namespace CppUnit {

template<typename T>
struct assertion_traits<T &> : assertion_traits<T> {} ;

template<typename T>
struct assertion_traits<const T> : assertion_traits<T> {} ;

template<typename T>
struct assertion_traits<std::reference_wrapper<T>> : assertion_traits<T> {} ;

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

template<typename Seq, char delim = ' ', char before = '(', char after = ')'>
struct assertion_traits_sequence {

      static bool equal(const Seq &, const Seq &) ;
      static std::string toString(const Seq &) ;
} ;

template<typename Seq, char d, char b, char a>
bool assertion_traits_sequence<Seq, d, b, a>::equal(const Seq &x, const Seq &y)
{
   using namespace std ;
   return
      size(x) == size(y) && std::equal(begin(x), end(x), begin(y), assertion_traits_eq()) ;
}

template<typename Seq, char d, char b, char a>
std::string assertion_traits_sequence<Seq, d, b, a>::toString(const Seq &value)
{
   using namespace std ;
   std::string result (1, b) ;
   std::for_each(begin(value), end(value), stringify_item(result, d)) ;
   return result.append(1, a) ;
}

template<typename M>
struct assertion_traits_matrix : assertion_traits_sequence<M, '\n', '\n', '\n'> {
      typedef assertion_traits_sequence<M, '\n', '\n', '\n'> ancestor ;

      static bool equal(const M &x, const M &y)
      {
         return x.columns() == y.columns() && ancestor::equal(x, y) ;
      }
      static std::string toString(const M &m)
      {
         std::string desc
            (pcomn::strprintf("%lux%lu", (unsigned long)m.rows(), (unsigned long)m.columns())) ;
         return m.empty()
            ? std::move(desc)
            : std::move(desc) + "\n========" + ancestor::toString(m) + "========\n" ;
      }
} ;

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

template<typename K, typename T, typename C>
struct assertion_traits<std::map<K, T, C>> : assertion_traits_sequence<std::map<K, T, C>> {} ;
template<typename K, typename C>
struct assertion_traits<std::set<K, C>> : assertion_traits_sequence<std::set<K, C>> {} ;
template<typename K, typename C>
struct assertion_traits<std::multiset<K, C>> : assertion_traits_sequence<std::multiset<K, C>> {} ;

#define _CPPUNIT_ASSERTION_TRAITS_SEQUENCE(sequence)                    \
   template<typename T>                                                 \
   struct assertion_traits<sequence<T>> : assertion_traits_sequence<sequence<T>> {}

#define _CPPUNIT_ASSERTION_TRAITS_MATRIX(matrix)                        \
   template<typename T>                                                 \
   struct assertion_traits<matrix<T>> : assertion_traits_matrix<matrix<T>> {}

_CPPUNIT_ASSERTION_TRAITS_SEQUENCE(std::vector) ;
_CPPUNIT_ASSERTION_TRAITS_SEQUENCE(std::list) ;
_CPPUNIT_ASSERTION_TRAITS_SEQUENCE(std::deque) ;
_CPPUNIT_ASSERTION_TRAITS_SEQUENCE(pcomn::simple_vector) ;
_CPPUNIT_ASSERTION_TRAITS_SEQUENCE(pcomn::simple_slice) ;

_CPPUNIT_ASSERTION_TRAITS_MATRIX(pcomn::matrix_slice) ;

template<typename T, size_t maxsize>
struct assertion_traits<pcomn::static_vector<T, maxsize>> :
         assertion_traits_sequence<pcomn::static_vector<T, maxsize>> {} ;

template<typename T, bool r>
struct assertion_traits<pcomn::simple_matrix<T, r>> :
         assertion_traits_matrix<pcomn::simple_matrix<T, r>> {} ;

#undef _CPPUNIT_ASSERTION_TRAITS_SEQUENCE
#undef _CPPUNIT_ASSERTION_TRAITS_MATRIX

template<>
inline std::string assertion_traits<std::type_info>::toString(const std::type_info &x)
{
   return PCOMN_DEMANGLE(x.name()) ;
}

template<>
std::string assertion_traits<unsigned char>::toString(const unsigned char &value)
{
   return std::to_string((unsigned)value) ;
}

template<>
std::string assertion_traits<signed char>::toString(const signed char &value)
{
   return std::to_string((int)value) ;
}

template<typename T1, typename T2>
struct assertion_traits<std::pair<T1, T2>> {
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
std::string assertion_traits<std::pair<T1, T2>>::toString(const type &value)
{
   return std::string(1, '{') + pcomn::unit::to_string(value.first) + ',' + pcomn::unit::to_string(value.second) + '}' ;
}

template<typename... A>
struct assertion_traits<std::tuple<A...>> {
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

template<typename Rep, intmax_t nom, intmax_t denom>
struct assertion_traits<std::chrono::duration<Rep, std::ratio<nom, denom>>> {
      typedef std::chrono::duration<Rep, std::ratio<nom, denom>> type ;

      static bool equal(const type &lhs, const type &rhs) { return lhs == rhs ; }
      static std::string toString(const type &value) ;
} ;

template<typename Rep, intmax_t nom, intmax_t denom>
std::string assertion_traits<std::chrono::duration<Rep, std::ratio<nom, denom>>>::toString(const type &value)
{
   return pcomn::string_cast(nom * value.count(), '/', denom) ;
}

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
struct assertion_traits<std::basic_string<C>> : assertion_traits_str<std::basic_string<C>> {} ;

template<typename C>
struct assertion_traits<pcomn::basic_cstrptr<C>> : assertion_traits_str<pcomn::basic_cstrptr<C>> {} ;

template<typename C>
struct assertion_traits<pcomn::basic_strslice<C>> : assertion_traits_str<pcomn::basic_strslice<C>> {
      static std::string toString(const pcomn::basic_strslice<C> &value)
      {
         return assertion_traits_str<pcomn::basic_strslice<C>>::quote(value.begin(), value.size()) ;
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
 Containter constructor macros.
*******************************************************************************/
#define CPPUNIT_STRING(value) (::pcomn::unit::to_string(value))
#define CPPUNIT_CONTAINER(type, items) (::pcomn::container_inserter< type >() items .container())

#define CPPUNIT_STRVECTOR(...)    CPPUNIT_CONTAINER(std::vector<std::string>, __VA_ARGS__)
#define CPPUNIT_STRSET(...)       CPPUNIT_CONTAINER(std::set<std::string>, __VA_ARGS__)
#define CPPUNIT_STRMAP(type, ...) CPPUNIT_CONTAINER(P_PASS(std::map<std::string, type >), __VA_ARGS__)

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

/*******************************************************************************
 Helper functions
*******************************************************************************/
namespace pcomn {
namespace unit {

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

__noinline inline
std::string full_file(FILE *file)
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
__noinline pcomn::enable_if_strchar_t<S, char, std::string>
full_file(const S &name)
{
   FILE * const file = fopen(pcomn::str::cstr(name), "r") ;
   PCOMN_ASSERT_POSIX(file ? 0 : -1, std::runtime_error,
                      "Cannot open '%s' for reading", pcomn::str::cstr(name)) ;
   return full_file(pcomn::FILE_safehandle(file).handle()) ;
}

template<typename S>
__noinline enable_if_strchar_t<S, char, string_vector>
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

} // end of namespace pcomn::unit
} // end of namespace pcomn

GCC_DIAGNOSTIC_POP()
#endif /* __PCOMN_UNITTEST_H */
