/*-*- mode: c++; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __TESTLOGGER_H
#define __TESTLOGGER_H
/*******************************************************************************
 FILE         :   TestLogger.h
 COPYRIGHT    :   Yakov Markovitch, 2003-2016. All rights reserved.

 DESCRIPTION  :   Logging extensions for CPPUnit
                  See LICENSE for information on usage/redistribution.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   3 Jun 2003
*******************************************************************************/
#include <cppunit/Portability.h>
#include <cppunit/TestCaller.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestAssert.h>
#include <fstream>

#include <exception>
#include <iostream>

#undef CPPUNIT_ASSERT_EQUAL
/// Redefinition of CPPUNIT_ASSERT_EQUAL, uses CppUnit::X::assertEquals
///
#define CPPUNIT_ASSERT_EQUAL(expected, actual)                          \
   (CppUnit::X::assertEquals((expected), (actual), CPPUNIT_SOURCELINE(), {}))

#ifdef __GNUC__
#include <cxxabi.h>
#include <array>
#define CPPUNIT_TYPENAME(t) (::CppUnit::Log::demangle(typeid(t).name()))
#else
#define CPPUNIT_TYPENAME(t) (typeid(t).name())
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
#define CPPUNIT_LOGSTREAM (CppUnit::Log::Logger<true>::logStream())

#define CPPUNIT_LOG(output) ((CPPUNIT_LOGSTREAM << output).flush())

#define CPPUNIT_LOG_LINE(output) ((CPPUNIT_LOGSTREAM << output << std::endl))

#define CPPUNIT_LOG_EXPRESSION(output) (CPPUNIT_LOGSTREAM << __LINE__ << (": "#output"=") << CppUnit::assertion_traits<decltype((output))>::toString((output)) << std::endl)

#define CPPUNIT_SETLOG(log) (CppUnit::Log::Logger<true>::setLogStream((log)))

#define CPPUNIT_LOG_ASSERT(condition)                             \
  (CPPUNIT_LOG(__CPPUNIT_CONCAT_SRC_LINE("TESTING: '") << (#condition) << "'... "),          \
   CPPUNIT_LOG((CPPUNIT_ASSERT(condition), "OK") << std::endl))

#define CPPUNIT_LOG_IS_TRUE(condition)                             \
  (CPPUNIT_LOG(__CPPUNIT_CONCAT_SRC_LINE("TESTING: '") << (#condition) << "' is true... "),   \
   CPPUNIT_LOG((CPPUNIT_ASSERT(condition), "OK") << std::endl))

#define CPPUNIT_LOG_BOOL_TRUE(condition)                             \
  (CPPUNIT_LOG(__CPPUNIT_CONCAT_SRC_LINE("TESTING: '") << (#condition) << "' is true... "),   \
   CPPUNIT_LOG((CPPUNIT_ASSERT(bool(condition)), "OK") << std::endl))

#define CPPUNIT_LOG_IS_FALSE(condition)                            \
  (CPPUNIT_LOG(__CPPUNIT_CONCAT_SRC_LINE("TESTING: '") << (#condition) << "' is false... "),  \
   CPPUNIT_LOG((CPPUNIT_ASSERT(!(condition)), "OK") << std::endl))

#define CPPUNIT_LOG_BOOL_FALSE(condition)                            \
  (CPPUNIT_LOG(__CPPUNIT_CONCAT_SRC_LINE("TESTING: '") << (#condition) << "' is false... "),  \
   CPPUNIT_LOG((CPPUNIT_ASSERT(!bool(condition)), "OK") << std::endl))

#define CPPUNIT_LOG_IS_NULL(expression)                           \
  (CPPUNIT_LOG(__CPPUNIT_CONCAT_SRC_LINE("TESTING: '") << (#expression) << "' is NULL... "), \
   CPPUNIT_LOG((CPPUNIT_ASSERT((expression) == nullptr), "OK") << std::endl))

#define CPPUNIT_LOG_EQUAL(actual, expected)                                   \
  (CPPUNIT_LOG(__CPPUNIT_CONCAT_SRC_LINE("EXPECTING: (") #actual ") == (" #expected << ")... "),         \
   CPPUNIT_LOG((CPPUNIT_ASSERT_EQUAL(expected, actual), "OK") << std::endl))

#define CPPUNIT_ASSERT_EQ(expected, actual) \
   (CppUnit::X::assertEq((expected), (actual), CPPUNIT_SOURCELINE(), {}))

#define CPPUNIT_LOG_EQ(actual, expected)                                \
   (CPPUNIT_LOG(__CPPUNIT_CONCAT_SRC_LINE("EXPECTING: (") #actual ") == (" #expected << ")... "), \
    CPPUNIT_LOG((CPPUNIT_ASSERT_EQ(expected, actual), "OK") << std::endl))

#define CPPUNIT_LOG_NOT_EQUAL(left, right)                                                  \
  (CPPUNIT_LOG(__CPPUNIT_CONCAT_SRC_LINE("EXPECTING: (") #left ") != (" #right << ")... "), \
   CPPUNIT_LOG((CppUnit::X::assertNotEquals(left, right, "(" #left ") != (" #right ")", \
                                            CPPUNIT_SOURCELINE()), "OK") << std::endl))


#define CPPUNIT_LOG_RUN(expression)                   \
do {                                                  \
   CPPUNIT_LOG((__CPPUNIT_CONCAT_SRC_LINE("RUNNING: '") #expression "'... ")) ;  \
   { expression ; }                                   \
   CPPUNIT_LOG("OK" << std::endl) ;                   \
} while(false)

#define CPPUNIT_LOG_NO_EXCEPTION(expression)          \
do {                                                  \
   CPPUNIT_LOG((__CPPUNIT_CONCAT_SRC_LINE("RUNNING: '") #expression "', EXPECTING: no exception... ")) ;  \
   CPPUNIT_ASSERT_NO_THROW(expression) ;              \
   CPPUNIT_LOG("OK" << std::endl) ;                   \
} while(false)


#define CPPUNIT_LOG_EXCEPTION(expression, expected)                  \
do {                                                                 \
   CPPUNIT_LOG((__CPPUNIT_CONCAT_SRC_LINE("RUNNING: '") #expression "', EXPECTING: '" #expected "'... ")) ; \
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

#define CPPUNIT_LOG_EXCEPTION_CODE(expression, expected, expected_code) \
do                                                                      \
   {                                                                    \
      long ___xc___ = (expected_code) ;                                 \
      CPPUNIT_LOG((__CPPUNIT_CONCAT_SRC_LINE("RUNNING: '") #expression "', EXPECTING: '" #expected) << '(' << #expected_code << ")'... ") ; \
      long *___pc___ = 0 ;                                              \
      long ___ac___ ;                                                   \
      try { expression ; }                                              \
      catch(const expected &__x__) { *(___pc___ = &___ac___) = __x__.code() ; } \
      if (!___pc___ || ___ac___ != ___xc___)                            \
         CppUnit::ExpectedExceptionCodeTraits<expected>::expectedException(___xc___, ___pc___); \
      CPPUNIT_LOG("OK" << std::endl) ;                                  \
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
      CppUnit::Log::logFailure<true>(x) ;                               \
      _exit(1) ;                                                        \
   }

namespace CppUnit {
namespace X {
template<typename T>
void assertEquals(const T &expected,
                  const T &actual,
                  SourceLine sourceLine,
                  const std::string &message)
{
  if (assertion_traits<T>::equal(expected,actual))
     return ;

  Asserter::failNotEqual(std::string(1, '\n').append(assertion_traits<T>::toString(expected)),
                         std::string(1, '\n').append(assertion_traits<T>::toString(actual)),
                         sourceLine,
                         message) ;
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
template<typename Actual, typename Expected>
inline void assertEq(const Expected &expected, const Actual &actual,
                     CppUnit::SourceLine line, const std::string &msg)
{
   X::assertEquals<Actual>(expected, actual, line, msg) ;
}

template <class T>
inline void assertNotEquals(const T &left, const T &right,
                            const std::string &expr, CppUnit::SourceLine line)
{
   if (left != right)
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
               CppUnit::Asserter::makeExpected(expr),
               CppUnit::Asserter::makeActual(actual)),
         line) ;
}

template<typename L, typename R>
inline void assertNotEq(const L &left, const R &right,
                        const std::string &expr, CppUnit::SourceLine line)
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
inline const char *demangle(const char *mangled)
{
   static __thread char buf[2048] ;
   int status = 0 ;
   size_t buflen = sizeof buf ;
   return abi::__cxa_demangle(mangled, buf, &buflen, &status) ;
}
#endif

template<bool = true>
struct Logger {
      /// Set global stream for CPPUnit test logging.
      /// The CPPUnit does not own the passed pointer, i.e. does not ever delete
      /// the passed stream object.
      /// NULL @a newlog turns logging off. Nonetheless the logStream() always returns a
      /// valid stream; if logging is off, the stream is dummy, i.e. it swallows any output
      /// without leving a trace.
      static std::ostream *setLogStream(std::ostream *newlog) ;
      static std::ostream &logStream() ;
   private:
      static std::ostream *log ;
} ;

template<> class Logger<false> {} ;

template<bool dummy>
std::ostream *Logger<dummy>::log = &std::cerr ;

template<bool dummy>
std::ostream *Logger<dummy>::setLogStream(std::ostream *newlog)
{
   std::ostream *result = log ;
   log = newlog ;
   return result ;
}

template<bool dummy>
std::ostream &Logger<dummy>::logStream()
{
   static std::ofstream nul ("/dev/nul") ;
   return *(log ? log : static_cast<std::ostream *>(&nul)) ;
}

inline void logExceptionWhat(const char *msg, const std::exception *x)
{
   CPPUNIT_LOG(msg << "\nWhat() : " << x->what() << std::endl) ;
}

inline void logExceptionWhat(const char *msg, ...)
{
   CPPUNIT_LOG(msg << std::endl) ;
}

template<bool dummy>
void logFailure(const Exception &x)
{
   Logger<dummy>::logStream() << "\nFAILURE" ;
   if (x.sourceLine().isValid())
      Logger<dummy>::logStream()
         << ": " << x.sourceLine().lineNumber() << ' ' << x.sourceLine().fileName() ;

   Logger<dummy>::logStream()
      << '\n' << x.message().shortDescription() << '\n' << x.message().details() << std::endl ;
}

} // end of namespace CppUnit::Log
} // end of namespace CppUnit

#endif /* __TESTLOGGER_H */
