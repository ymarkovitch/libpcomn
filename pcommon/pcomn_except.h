/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_EXCEPT_H
#define __PCOMN_EXCEPT_H
/*******************************************************************************
 FILE         :   pcomn_except.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2018. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Base PCOMMON exception classes

 CREATION DATE:   15 Feb 2000
*******************************************************************************/
#include <pcommon.h>

#include <stdexcept>
#include <string>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

/*******************************************************************************
 Exception throwing convenience macros
*******************************************************************************/
/// Throw an exception with an ostream-formatted message
/// @hideinitializer @ingroup ExceptionMacros
#define PCOMN_THROW_S(exception, msg, ...)                              \
do {                                                                    \
   pcomn::bufstr_ostream<pcomn::PCOMN_MSGBUFSIZE> msgstream ;                  \
   msgstream << msg << std::ends ;                                      \
   pcomn::throw_exception<exception >(msgstream.str(), ##__VA_ARGS__) ; \
} while(false)

/// Throw an exception with an ostream-formatted message if a specified condition holds
/// @hideinitializer @ingroup ExceptionMacros
#define PCOMN_THROW_S_IF(condition, exception, msg, ...)    \
do {                                                        \
   if (condition)                                           \
      PCOMN_THROW_S(exception, msg, ##__VA_ARGS__) ;        \
} while(false)

#define PCOMN_THROW_MSGF(exception, format, ...)                        \
do {                                                                    \
   char message[pcomn::PCOMN_MSGBUFSIZE] ;                                     \
   message[sizeof message - 1] = 0 ;                                    \
   const size_t sz = strlen(strncpy(message, PCOMN_PRETTY_FUNCTION, sizeof message - 3)) ; \
   message[sz] = ':' ; message[sz + 1] = '\n' ;                          \
   snprintf(message + sz + 2, sizeof message - sz - 2, format, ##__VA_ARGS__) ; \
   pcomn::throw_exception<exception >(message) ;                        \
} while(false)

/// Throw an exception with a formatted message if a specified condition holds.
/// @hideinitializer @ingroup ExceptionMacros
#define PCOMN_THROW_MSG_IF(condition, exception, format, ...)  \
do {                                                           \
   if (condition)                                              \
      PCOMN_THROW_MSGF(exception, format, ##__VA_ARGS__) ;     \
} while(false)

#define PCOMN_THROW_UNIMPLEMENTED() (::pcomn::throw_exception< ::pcomn::not_implemented_error>(PCOMN_PRETTY_FUNCTION))

#define PCOMN_THROW_SYSERROR(callname)  (::pcomn::throw_syserror<true>(__FUNCTION__, (callname)))

/// Throw system_error if the result of a function call is -1 (this indicates error in "classic" POSIX).
/// @ingroup ExceptionMacros
/// @param result    POSIX function result (e.g., PCOMN_ENSURE_POSIX(read(foofd, buf, sizeof buf), "open")).
/// @param callname  Name of the function that returned @a result.
/// @return @a result
#define PCOMN_ENSURE_POSIX(result, callname)  (::pcomn::ensure_posix((result), __FUNCTION__, (callname)))

#define PCOMN_CHECK_POSIX(posix_result, format, ...)                    \
do if (unlikely(pcomn::posix_fail((posix_result))))                     \
{                                                                       \
   char message[pcomn::PCOMN_MSGBUFSIZE] ;                                     \
   snprintf(message, sizeof message, format, ##__VA_ARGS__) ;           \
   pcomn::throw_exception<pcomn::system_error>(message) ;               \
} while(false)

#define PCOMN_ASSERT_POSIX(posix_result, exception, format, ...)        \
do if (unlikely(pcomn::posix_fail((posix_result))))                     \
      PCOMN_THROW_SYSREASON(exception, format, ##__VA_ARGS__) ;         \
while(false)

/// Throw system_error if the result of a function call is not 0.
/// @ingroup ExceptionMacros
/// @param result    Error code.
/// @param callname  Name of the function that returned @a result.
/// @return @a result
#define PCOMN_ENSURE_ENOERR(result, callname) (::pcomn::ensure_enoerr((result), __FUNCTION__, (callname)))

#define PCOMN_CHECK_ENOERR(result, format, ...)                                 \
do if (const int errno__##__LINE__ = (result)))                                 \
      pcomn::throw_syserror<true>(errno__##__LINE__, (format), ##__VA_ARGS__) ; \
while(false)

#define PCOMN_ASSERT_ENOERR(result, exception, format, ...)             \
do if (const int errno__##__LINE__ = (result)))                         \
      pcomn::throw_sysreason<exception >(errno__##__LINE__, format, ##__VA_ARGS__) ; \
while(false)

#define PCOMN_THROW_SYSREASON(exception, format, ...)                   \
do {                                                                    \
   const int errno##__LINE__ = pcomn::system_error::lasterr() ;         \
   pcomn::throw_sysreason<exception >(errno##__LINE__, format, ##__VA_ARGS__) ; \
} while(false)

#define PCOMN_ERRNO(call) (::pcomn::posix_errno((call)))

namespace pcomn {

/******************************************************************************/
/** Template class for creation of "compound" exceptions

 A compound exception has two bases: a virtual public "addmix" base and public
 "main" base. Admittedly, the "main" base is an exception class based, directly
 or indirectly, on std::exception.
*******************************************************************************/
template<class V, class E>
class compound_exception : public virtual V, public E {
   public:
      template<typename S>
      compound_exception(const S &message) : V(), E(message) {}
      /// This constructor is for E that does not accept arguments
      compound_exception() : V(), E() {}
      const char *what() const throw() { return E::what() ; }
   protected:
      typedef compound_exception<V, E> compound_type ;

      compound_exception(const char *begin, const char *end) ;
} ;

template<class V, class E>
__noinline compound_exception<V, E>::compound_exception(const char *begin, const char *end) :
    V(), E(std::string(begin, end))
{}

/******************************************************************************/
/** The base class for errors that can occur in OS/system library environment,
 like I/O errors, system API errors, etc.
*******************************************************************************/
class _PCOMNEXP environment_error : public std::runtime_error {
      typedef std::runtime_error ancestor ;
   public:
      environment_error() : ancestor("") {}

      explicit environment_error(const std::string &msg) : environment_error()
      {
         set_message(msg) ;
      }

      ~environment_error() throw() {}

      const char *what() const throw() { return _message.c_str() ; }

      void set_message(const std::string &msg) { _message = msg ; }

   private:
      std::string _message ;
} ;

/******************************************************************************/
/** Indicates OS/system library call error.
*******************************************************************************/
class _PCOMNEXP system_error : public virtual environment_error {
      #ifdef PCOMN_PL_UNIX
      struct errcode {
            errcode(int posix_code, int platform_specific) :
               _code(posix_code ? posix_code : platform_specific)
            {}
            int posix_code() const { return _code ; }
            int platform_code() const { return _code ; }
            __noinline static int lasterr() { return errno ; }
            static const char *errmsg(int code) { return strerror(code) ; }

            const int _code ;
      } ;
      #else
      struct errcode {
            errcode(int posix_code, int platform_code) :
               _platform_code(platform_code),
               _posix_code(posix_code)
            {}
            int posix_code() const { return _posix_code ; }
            int platform_code() const { return _platform_code ; }
            static _PCOMNEXP int lasterr() ;
            static _PCOMNEXP std::string errmsg(int code) ;

            const int _platform_code ;
            const int _posix_code ;
      } ;
      #endif

   public:
      enum PlatformSpecific { platform_specific } ;

      __noinline system_error() : _errcode(errno, 0)
      {
         set_message(strerror(posix_code())) ;
      }

      /// Creates an exception object with the message based on the Posix errorcode.
      explicit system_error(int errcode) : _errcode(errcode, 0)
      {
         set_message(strerror(errcode)) ;
      }
      /// Creates an exception object with the message based on the Posix errorcode.
      __noinline explicit system_error(const std::string &msg) : _errcode(errno, 0)
      {
         set_message(msg + ": " + strerror(posix_code())) ;
      }

      explicit system_error(const std::string &msg, int errcode) : _errcode(errcode, 0)
      {
         set_message(msg + ": " + strerror(errcode)) ;
      }

      /// Creates an exception object with the message based on the platform-specific OS errorcode.
      /// @param errcode System error code; the default value of this parameter is the
      /// system last error (errno on *Nix, GetLastError on Windows).
      system_error(PlatformSpecific, int errcode = errcode::lasterr()) :
         _errcode(0, errcode)
      {
         set_message(errcode::errmsg(errcode)) ;
      }

      /// Creates an exception object with the message based on the platform-specific OS errorcode.
      /// @param errcode   System error code; the default value of this parameter is the
      /// system last error (errno on *Nix, GetLastError on Windows).
      /// @param msg       The message that prepends system error message.
      system_error(PlatformSpecific, const std::string &msg, int errcode = errcode::lasterr()) :
         _errcode(0, errcode)
      {
         set_message(msg + ": " + errcode::errmsg(errcode)) ;
      }

      int posix_code() const { return _errcode.posix_code() ; }

      int platform_code() const { return _errcode.platform_code() ; }

      int code() const { return posix_code() ; }

      static int lasterr() { return errcode::lasterr() ; }

      static
      #ifdef PCOMN_PL_UNIX
      const char *
      #else
      std::string
      #endif
      syserrmsg(int code) { return errcode::errmsg(code) ; }

   private:
      errcode _errcode ;
} ;

/******************************************************************************/
/** Indicates timeout expiration.
*******************************************************************************/
class _PCOMNEXP timeout_error : public virtual environment_error {
   public:
      explicit timeout_error(const std::string &msg = std::string())
      {
         set_message(msg) ;
      }
} ;

/*******************************************************************************
 Tag exception classes
*******************************************************************************/
struct _PCOMNEXP object_closed : public std::runtime_error {
      object_closed() :
         std::runtime_error("The object is already closed")
      {}

      object_closed(const std::string &object) :
         std::runtime_error(object + ": the object is already closed")
      {}
} ;

/******************************************************************************/
/** Exception that indicates that a sequence is already closed and cannot be read/written.
*******************************************************************************/
struct _PCOMNEXP sequence_closed : public object_closed {

      sequence_closed() {}

      sequence_closed(const std::string &object) :
         object_closed(object)
      {}
} ;

/*******************************************************************************
 System/POSIX errors handling functions
*******************************************************************************/
template<bool>
__noreturn __noinline
void throw_syserror(const char *, const char *, int err = system_error::lasterr()) ;
template<bool>
__noreturn __noinline void throw_syserror(int, const char *, ...) PCOMN_ATTR_PRINTF(2, 3) ;

template<bool>
__noreturn __noinline
void throw_syserror(int err, const char *format, ...)
{
   char buf[PCOMN_MSGBUFSIZE] ;
   va_list parm ;
   va_start(parm, format) ;
   vsnprintf(buf, sizeof buf, format, parm) ;
   va_end(parm) ;
   throw_exception<system_error>(system_error::platform_specific, buf, err) ;
}

template<bool dummy>
__noreturn __noinline
void throw_syserror(const char *function_name, const char *call_name_or_message, int err)
{
   if (!strchr(call_name_or_message, ' '))
      throw_syserror<dummy>(err, "In '%s' calling '%s()'", function_name, call_name_or_message) ;
   throw_exception<system_error>(system_error::platform_specific, call_name_or_message, err) ;
}

template<typename T>
inline bool posix_fail(T posix_result) { return posix_result == static_cast<T>(-1) ; }

template<typename T>
inline int posix_errno(T posix_result) { return posix_fail(posix_result) ? errno : 0 ; }

template<typename T>
inline T ensure_posix(T result, const char *function_name, const char *call_name_or_message)
{
   if (unlikely(posix_fail(result)))
      throw_syserror<true>(function_name, call_name_or_message) ;
   return result ;
}

template<typename T>
inline T ensure_posix(T result, const char *message)
{
   if (unlikely(posix_fail(result)))
      throw_exception<system_error>(message, errno) ;
   return result ;
}

inline void ensure_enoerr(int result, const char *function_name, const char *call_name_or_message)
{
   if (unlikely(result))
      throw_syserror<true>(function_name, call_name_or_message, result) ;
}

template<class X>
__noreturn __noinline void throw_sysreason(int errno_, const char *, ...) PCOMN_ATTR_PRINTF(2, 3) ;
template<class X>
void throw_sysreason(int errno_, const char *format, ...)
{
   const unsigned esz = 512 ;
   char buf[PCOMN_MSGBUFSIZE + esz] ;

   va_list parm ;
   va_start(parm, format) ;
   vsnprintf(buf, PCOMN_MSGBUFSIZE, format, parm) ;
   va_end(parm) ;

   if (errno_)
   {
      const size_t len = strlen(buf) ;
      buf[sizeof buf - 1] = 0 ;
      buf[len] = ':' ; buf[len + 1] = ' ' ;
      strncpy(buf + (len + 2), system_error(system_error::platform_specific, errno_).what(), esz - 3) ;
   }
   throw_exception<X>(buf) ;
}

} // end of namepace pcomn

#endif /* __PCOMN_EXCEPT_H */
