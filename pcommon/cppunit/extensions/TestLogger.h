/*-*- mode: c++; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __TESTLOGGER_H
#define __TESTLOGGER_H
/*******************************************************************************
 FILE         :   TestLogger.h
 COPYRIGHT    :   Yakov Markovitch, 2003-2020. All rights reserved.

 DESCRIPTION  :   Logging extensions for CPPUnit
                  See LICENSE for information on usage/redistribution.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   3 Jun 2003
*******************************************************************************/
#include <cppunit/Portability.h>
#include <cppunit/TestCaller.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>
#include <cppunit/SourceLine.h>

#include <exception>
#include <fstream>
#include <type_traits>
#include <utility>

#undef CPPUNIT_ASSERT_EQUAL
/// Redefinition of CPPUNIT_ASSERT_EQUAL, uses CppUnit::X::assertEquals
///
#define CPPUNIT_ASSERT_EQUAL(expected, actual)                          \
   (CppUnit::X::assertEquals((expected), (actual), CPPUNIT_SOURCELINE()))

#undef CPPUNIT_SOURCELINE

#define CPPUNIT_SOURCELINE() \
   (CppUnit::Log::Logger<>::sourceLine(__FILE__, __LINE__))

#ifdef __GNUC__
#include <cxxabi.h>
#include <array>
#define CPPUNIT_TYPENAME(t) (::CppUnit::Log::demangle(typeid(t).name()))
#define CPPUNIT_NOINLINE __attribute__((__noinline__))
#else
#define CPPUNIT_TYPENAME(t) (typeid(t).name())
#define CPPUNIT_NOINLINE
#endif

#define CPPUNIT_DEREFTYPENAME(t) ((t) ? CPPUNIT_TYPENAME(*t) : CPPUNIT_TYPENAME(t))

#define CPPUNIT_IS_FALSE(condition) CPPUNIT_ASSERT(!(condition))
#define CPPUNIT_IS_TRUE(condition)  CPPUNIT_ASSERT(condition)
#define CPPUNIT_RUN(expression) { expression ; }
#define CPPUNIT_EQUAL(actual, expected) CPPUNIT_ASSERT_EQUAL(expected, actual)
#define CPPUNIT_EXCEPTION(expression, expected)  CPPUNIT_ASSERT_THROW(expression, expected)

#define __CPPUNIT_STRINGIFY_DIRECT(item) #item
#define __CPPUNIT_STRINGIFY_INDIRECT(item) __CPPUNIT_STRINGIFY_DIRECT(item)
#define __CPPUNIT_CONCAT_SRC_LINE(text) __CPPUNIT_STRINGIFY_INDIRECT(__LINE__) ": " text

/*******************************************************************************
 Logging macros for running tests
*******************************************************************************/
#ifndef CPPUNIT_USE_SYNC_LOGSTREAM
#  define CPPUNIT_LOGSTREAM (CppUnit::Log::Logger<>::logStream())
#else
#include <mutex>
#  define CPPUNIT_LOGSTREAM (CppUnit::Log::Logger<>::syncstream())
#endif /* CPPUNIT_USE_SYNC_LOGSTREAM */

#define CPPUNIT_LOG(output) ((CPPUNIT_LOGSTREAM << output).flush())

#define CPPUNIT_LOG_MESSAGE(msg) (CppUnit::Log::logMessage((msg)))

#define CPPUNIT_LOG_LINE(output) ((CPPUNIT_LOGSTREAM << output << '\n').flush())

#define CPPUNIT_LOG_EXPRESSION(output) (CPPUNIT_LOGSTREAM << __LINE__ << (": "#output"=") << CppUnit::assertion_traits<decltype((output))>::toString((output)) << std::endl)

#define CPPUNIT_SETLOG(log) (CppUnit::Log::Logger<>::setLogStream((log)))

#define CPPUNIT_LOG_ASSERT(condition)                             \
   (CPPUNIT_LOG_MESSAGE((__CPPUNIT_CONCAT_SRC_LINE("TESTING: '" #condition "'... "))), \
    CPPUNIT_LOG_MESSAGE((CPPUNIT_ASSERT(condition), "OK\n")))

#define CPPUNIT_LOG_IS_TRUE(condition)                                  \
   (CPPUNIT_LOG_MESSAGE((__CPPUNIT_CONCAT_SRC_LINE("TESTING: '") #condition "' is true... ")), \
    CPPUNIT_LOG_MESSAGE((CPPUNIT_ASSERT(condition), "OK\n")))

#define CPPUNIT_LOG_BOOL_TRUE(condition)                             \
  (CPPUNIT_LOG_MESSAGE((__CPPUNIT_CONCAT_SRC_LINE("TESTING: '") #condition "' is true... ")), \
   CPPUNIT_LOG_MESSAGE((CPPUNIT_ASSERT(bool(condition)), "OK\n")))

#define CPPUNIT_LOG_IS_FALSE(condition)                            \
  (CPPUNIT_LOG_MESSAGE((__CPPUNIT_CONCAT_SRC_LINE("TESTING: '") #condition "' is false... ")), \
   CPPUNIT_LOG_MESSAGE((CPPUNIT_ASSERT(!(condition)), "OK\n")))

#define CPPUNIT_LOG_BOOL_FALSE(condition)                            \
  (CPPUNIT_LOG_MESSAGE((__CPPUNIT_CONCAT_SRC_LINE("TESTING: '") #condition "' is false... ")), \
   CPPUNIT_LOG_MESSAGE((CPPUNIT_ASSERT(!bool(condition)), "OK\n")))

#define CPPUNIT_LOG_IS_NULL(expression)                           \
  (CPPUNIT_LOG_MESSAGE((__CPPUNIT_CONCAT_SRC_LINE("TESTING: '") #expression "' is NULL... ")), \
   CPPUNIT_LOG_MESSAGE((CPPUNIT_ASSERT((expression) == nullptr), "OK\n")))

#define CPPUNIT_LOG_EQUAL(actual, expected)                             \
  (CPPUNIT_LOG_MESSAGE((__CPPUNIT_CONCAT_SRC_LINE("EXPECTING: (") #actual ") == (" #expected ")... ")), \
   CPPUNIT_LOG_MESSAGE((CPPUNIT_ASSERT_EQUAL(expected, actual), "OK\n")))

#define CPPUNIT_ASSERT_EQ(expected, actual) \
   (CppUnit::X::assertEq((expected), (actual), CPPUNIT_SOURCELINE()))

#define CPPUNIT_EQ(actual, expected)            \
   CPPUNIT_ASSERT_EQ((expected), (actual))

#define CPPUNIT_LOG_EQ(actual, expected)                                \
   (CPPUNIT_LOG_MESSAGE((__CPPUNIT_CONCAT_SRC_LINE("EXPECTING: (") #actual ") == (" #expected ")... ")), \
    CPPUNIT_LOG_MESSAGE((CPPUNIT_ASSERT_EQ(expected, actual), "OK\n")))

#define CPPUNIT_LOG_NOT_EQUAL(left, right)                              \
   (CPPUNIT_LOG_MESSAGE((__CPPUNIT_CONCAT_SRC_LINE("EXPECTING: (") #left ") != (" #right ")... ")), \
    CPPUNIT_LOG((CppUnit::X::assertNotEquals(left, right, "(" #left ") != (" #right ")", \
                                             CPPUNIT_SOURCELINE()), "OK") << std::endl))


#define CPPUNIT_LOG_RUN(expression)                   \
do {                                                  \
   CPPUNIT_LOG_MESSAGE((__CPPUNIT_CONCAT_SRC_LINE("RUNNING: '") #expression "'... ")) ;  \
   { expression ; }                                   \
   CPPUNIT_LOG_MESSAGE("OK\n") ;                      \
} while(false)

#define CPPUNIT_LOG_NO_EXCEPTION(expression)          \
do {                                                  \
   CPPUNIT_LOG_MESSAGE((__CPPUNIT_CONCAT_SRC_LINE("RUNNING: '") #expression "', EXPECTING: no exception... ")) ;  \
   CPPUNIT_ASSERT_NO_THROW(expression) ;              \
   CPPUNIT_LOG_MESSAGE("OK\n") ;                      \
} while(false)


#define CPPUNIT_LOG_EXCEPTION(expression, expected)                  \
do {                                                                 \
   CPPUNIT_LOG_MESSAGE((__CPPUNIT_CONCAT_SRC_LINE("RUNNING: '") #expression "', EXPECTING: '" #expected "'... ")) ; \
   CppUnit::Message __cpput_msg__("expected exception not thrown") ;    \
   __cpput_msg__.addDetail("Expected: " __CPPUNIT_STRINGIFY_DIRECT(expected)) ; \
                                                                        \
   try {                                                                \
      expression ;                                                      \
   } catch (const expected &x) {                                        \
      CppUnit::Log::logExceptionWhat("OK", &x) ;                        \
      break ;                                                           \
   } catch (const std::exception &e) {                                  \
      __cpput_msg__.addDetail(std::string("Actual  : ") + CPPUNIT_TYPENAME(e), \
                              std::string("What()  : ") + e.what()) ;   \
   } catch (...) { __cpput_msg__.addDetail( "Actual  : unknown.") ; }   \
   CppUnit::Asserter::fail(__cpput_msg__, CPPUNIT_SOURCELINE()) ;       \
} while(false)

#define CPPUNIT_LOG_FAILURE(expression)                                 \
   do {                                                                 \
   CPPUNIT_LOG_MESSAGE((__CPPUNIT_CONCAT_SRC_LINE("RUNNING: '") #expression "', EXPECTING: exception... ")) ; \
   try {                                                                \
      CppUnit::Message __cpput_msg__("expected exception not thrown") ; \
      expression ;                                                      \
      CppUnit::Asserter::fail(__cpput_msg__, CPPUNIT_SOURCELINE()) ;    \
   } catch (const std::exception &x) { CppUnit::Log::logExceptionWhat("OK", &x) ; \
   } catch (...) { CPPUNIT_LOG_MESSAGE("OK\n") ; }                      \
} while(false)

#define CPPUNIT_LOG_EXCEPTION_CODE(expression, expected, expected_code) \
do                                                                      \
   {                                                                    \
      long ___xc___ = (expected_code) ;                                 \
      CPPUNIT_LOG((__CPPUNIT_CONCAT_SRC_LINE("RUNNING: '") #expression "', EXPECTING: '" #expected) << '(' << #expected_code << ")'... ") ; \
      long *___pc___ = nullptr, ___ac___ = 0 ;                          \
      try { expression ; }                                              \
      catch(const expected &__x__) {                                    \
         *(___pc___ = &___ac___) = __x__.code() ;                       \
         CppUnit::Log::logExceptionWhat("", &__x__) ;                   \
      }                                                                 \
      if (!___pc___ || ___ac___ != ___xc___)                            \
         CppUnit::ExpectedExceptionCodeTraits<expected>::expectedException(___xc___, ___pc___); \
      CPPUNIT_LOG_MESSAGE("OK\n") ;                                     \
   } while(false)

#define CPPUNIT_LOG_EXCEPTION_MSG(expression, expected, expected_msg_substr) \
do                                                                      \
   {                                                                    \
      CppUnit::Message __cpput_msg__("expected exception not thrown") ;       \
       __cpput_msg__.addDetail("Expected: " __CPPUNIT_STRINGIFY_DIRECT(expected)) ; \
      const pcomn::strslice ___xs___ = (expected_msg_substr) ;          \
      CPPUNIT_LOG((__CPPUNIT_CONCAT_SRC_LINE("RUNNING: '") #expression "', EXPECTING: " #expected) << " containing '" << ___xs___ << "'... ") ; \
      bool __cpput_x_thrown__ = true ;                                  \
      try { expression ; __cpput_x_thrown__ = false ; }                  \
      catch(const expected &__x__) { \
         CppUnit::ExpectedExceptionCodeTraits<expected>::expectedException(___xs___, __x__.what()); \
         CppUnit::Log::logExceptionWhat("OK", &__x__) ;                 \
      }                                                                 \
      if (!__cpput_x_thrown__) CppUnit::Asserter::fail(__cpput_msg__, CPPUNIT_SOURCELINE()) ; \
   } while(false)

/*******************************************************************************
 Failure handler for stacks other than of the main thread (or of a thread where
 TestRunner::run() was called), e.g. to use CppUnit macros in threads/contexts.
 Note the result of assertion failure is process abort.
*******************************************************************************/
#ifdef __GNUC__
#define CPPUNIT_FAIL_HANDLER_START() try { CPPUNIT_LOG_LINE("\n*** " << __PRETTY_FUNCTION__) ;
#else
#define CPPUNIT_FAIL_HANDLER_START() try { CPPUNIT_LOG_LINE("\n*** " << __FUNCTION__) ;
#endif

#define CPPUNIT_FAIL_HANDLER_END()                                      \
   } catch (const CppUnit::Exception &x) {                              \
      CppUnit::Log::logFailure<>(x) ;                                   \
      _exit(1) ;                                                        \
   }

namespace CppUnit {
namespace X {

enum class nval : bool { _ } ;

template<typename S, typename T>
using enable_if_string__t = typename std::enable_if<std::is_convertible<S, std::string>::value, T>::type ;

template<typename T, typename S>
enable_if_string__t<S, void>
assertEquals(const T &expected,
             const T &actual,
             const SourceLine &sourceLine,
             S &&message)
{
  if (assertion_traits<T>::equal(expected,actual))
     return ;

  Asserter::failNotEqual(std::string(1, '\n').append(assertion_traits<T>::toString(expected)),
                         std::string(1, '\n').append(assertion_traits<T>::toString(actual)),
                         sourceLine,
                         std::forward<S>(message)) ;
}

template<typename T>
void assertEquals(const T &expected, const T &actual, const SourceLine &sourceLine)
{
   static const char * const es = "" ;
   X::assertEquals(expected, actual, sourceLine, es) ;
}

/// Asserts that two values are equals.
///
/// Equality and string representation can be defined with an appropriate
/// CppUnit::assertion_traits class.
/// A diagnostic is printed if actual and expected values disagree.
///
/// Differs from CPPUNIT_ASSERT_EQUAL in that @a expected and @a actual need @em not
/// be of the same type, implicit convertibility from @a expected to @a actual's type
/// is enough.
///
/// Requirement for @a expected and @a actual parameters:
/// - @a expected is @em implicitly @em convertible to the type of @a actual
/// - They are serializable into a std::strstream using operator <<.
/// - They can be compared using operator ==.
///
/// The last two requirements (serialization and comparison) can be
/// removed by specializing the CppUnit::assertion_traits.
///
template<typename Actual, typename Expected, typename S>
inline enable_if_string__t<S, void> assertEq(Expected &&expected, Actual &&actual,
                                             const SourceLine &line, S &&msg)
{
   X::assertEquals<typename std::remove_reference<Actual>::type>
      (std::forward<Expected>(expected), std::forward<Actual>(actual), line, std::forward<S>(msg)) ;
}

template<typename Actual, typename Expected>
void assertEq(Expected &&expected, Actual &&actual, const SourceLine &line)
{
   static const char * const es = "" ;
   X::assertEq(std::forward<Expected>(expected), std::forward<Actual>(actual), line, es) ;
}

template<typename T, typename S>
enable_if_string__t<S, void> assertNotEquals(const T &left, const T &right,
                                             S &&expr, const SourceLine &line)
{

   if (!assertion_traits<T>::equal(left, right))
      return ;
   const std::string leftrepr(CppUnit::assertion_traits<T>::toString(left)) ;
   const std::string rightrepr(CppUnit::assertion_traits<T>::toString(right)) ;
   const std::string actual(leftrepr == rightrepr ?
         ("both operands of != operator have the same string representation: '") +
               leftrepr + "'" :
         ("operands of != operator have following string representations, respectively: '") +
               leftrepr + "' and '" + rightrepr + "'") ;
   CppUnit::Asserter::fail(
         CppUnit::Message(
               "not equal assertion failed",
               CppUnit::Asserter::makeExpected(std::string(std::forward<S>(expr))),
               CppUnit::Asserter::makeActual(actual)),
         line) ;
}

template<typename L, typename R>
void assertNotEq(const L &left, const R &right,
                 const std::string &expr, const SourceLine &line)
{
   X::assertNotEquals<L>(left, right, expr, line) ;
}

} // end of namespace CppUnit::X
} // end of namespace CppUnit

/*******************************************************************************
 CppUnit::Log

 Global logging.
 It is not recommended to use CppUnit::Log directly; use CPPUNIT_LOG family
 of macros instead.
*******************************************************************************/
namespace CppUnit {
namespace Log {

#ifdef __GNUC__
__attribute__((__noinline__))
inline const char *demangle(const char *mangled)
{
   static __thread char buf[2048] ;
   int status = 0 ;
   size_t buflen = sizeof buf ;
   return abi::__cxa_demangle(mangled, buf, &buflen, &status) ;
}
#endif

template<X::nval = {}>
struct Logger {
      /// Set global stream for CPPUnit test logging.
      /// The CPPUnit does not own the passed pointer, i.e. does not ever delete
      /// the passed stream object.
      /// NULL @a newlog turns logging off. Nonetheless the logStream() always returns a
      /// valid stream; if logging is off, the stream is dummy, i.e. it swallows any output
      /// without leving a trace.
      static std::ostream *setLogStream(std::ostream *newlog) ;
      static std::ostream &logStream() ;
      static SourceLine sourceLine(const char *file, int line) ;

      #ifdef CPPUNIT_USE_SYNC_LOGSTREAM
      class syncstream final : private std::basic_streambuf<char>, public std::ostream {
            typedef std::ostream                ancestor ;
            typedef std::basic_streambuf<char>  streambuf_type ;
         public:
            using ancestor::traits_type ;
            using ancestor::int_type ;

            syncstream() ;
            ~syncstream() override ;

         private:
            std::mutex     _mutex ;
            std::string    _data ;
            std::ostream & _wrapped ;

         private:
            int_type overflow(int_type c = traits_type::eof()) override ;
      } ;
      #endif /* CPPUNIT_USE_SYNC_LOGSTREAM */

   private:
      static std::ostream *         log ;
      static std::ostream::fmtflags log_fmtflags ;
      static unsigned               log_precision ;
      static unsigned               log_width ;
} ;

/*******************************************************************************
 Logger<n>
*******************************************************************************/
template<X::nval n>
std::ostream *Logger<n>::log = &std::cerr ;

template<X::nval n>
std::ostream::fmtflags Logger<n>::log_fmtflags = log->flags() ;
template<X::nval n>
unsigned Logger<n>::log_precision = log->precision() ;
template<X::nval n>
unsigned Logger<n>::log_width = log->width() ;

template<X::nval n>
std::ostream *Logger<n>::setLogStream(std::ostream *newlog)
{
   std::ostream *result = log ;
   log = newlog ;
   if (log)
   {
      log_fmtflags = log->flags() ;
      log_precision = log->precision() ;
      log_width = log->width() ;
   }
   return result ;
}

template<X::nval n>
std::ostream &Logger<n>::logStream()
{
   static std::ofstream nul ("/dev/nul") ;
   if (log)
   {
      log->flags(log_fmtflags) ;
      log->precision(log_precision) ;
      log->width(log_width) ;
   }
   return *(log ? log : static_cast<std::ostream *>(&nul)) ;
}

template<X::nval n>
SourceLine Logger<n>::sourceLine(const char *file, int line)
{
   return SourceLine(file, line) ;
}

#ifdef CPPUNIT_USE_SYNC_LOGSTREAM

template<X::nval n>
CPPUNIT_NOINLINE Logger<n>::syncstream::syncstream() :
   ancestor(static_cast<streambuf_type *>(this)),
   _wrapped([]()->std::ostream &
   {
      static std::ofstream nul ("/dev/nul") ;
      if (!Logger::log)
         return nul ;
      return *Logger::log ;
   }())
{
   flags(Logger::log_fmtflags) ;
   precision(Logger::log_precision) ;
   width(Logger::log_width) ;
}

template<X::nval n>
CPPUNIT_NOINLINE Logger<n>::syncstream::~syncstream()
{
   const char * const tail = pbase() + _data.size() ;
   _data.append(tail, pptr() - tail) ;

   std::lock_guard<std::mutex> guard (_mutex) ;
   _wrapped << _data << std::flush ;
}

template<X::nval n>
CPPUNIT_NOINLINE auto Logger<n>::syncstream::overflow(int_type c) -> int_type
{
   if (traits_type::eq_int_type(c, traits_type::eof()))
      return traits_type::not_eof(c) ;

   const size_t capacity = _data.capacity() ;
   const size_t max_size = _data.max_size() ;

   if (pptr() >= epptr())
   {
      if (capacity >= max_size - 1)
         return traits_type::eof() ;

      std::string new_buffer ;
      // Grow buffer
      new_buffer.reserve(std::min(max_size, std::max<size_t>(16, 2 * (capacity + 1))) - 1) ;
      if (pbase())
         new_buffer.assign(pbase(), epptr() - pbase()) ;
      _data.swap(new_buffer) ;

      // Reinit stream buffer
      char * const pstart = const_cast<char *>(_data.data()) ;
      setp(pstart, pstart + _data.capacity()) ;
      pbump((unsigned)_data.size()) ;
      setg(pstart, pstart, pstart) ;
   }
   *pptr() = traits_type::to_char_type(c) ;
   pbump(1) ;
   return c ;
}

#endif /* CPPUNIT_USE_SYNC_LOGSTREAM */

/*******************************************************************************
 Logging functions
*******************************************************************************/
template<typename S>
void logExceptionWhat(S &&msg, const std::exception *x)
{
   CPPUNIT_LOG(std::forward<S>(msg) << "\nWhat() : " << x->what() << '\n') ;
}

template<typename S>
void logMessage(S &&msg)
{
   CPPUNIT_LOG(std::forward<S>(msg)) ;
}

template<typename S>
void logExceptionWhat(S &&msg, ...)
{
   CPPUNIT_LOG_LINE(std::forward<S>(msg)) ;
}

template<X::nval n = {}>
void logFailure(const Exception &x)
{
   auto &&stream = CPPUNIT_LOGSTREAM ;
   stream << "\nFAILURE" ;
   if (x.sourceLine().isValid())
      stream << ": " << x.sourceLine().lineNumber() << ' ' << x.sourceLine().fileName() ;

   stream << '\n' << x.message().shortDescription() << '\n' << x.message().details() << std::endl ;
}

} // end of namespace CppUnit::Log
} // end of namespace CppUnit

#endif /* __TESTLOGGER_H */
