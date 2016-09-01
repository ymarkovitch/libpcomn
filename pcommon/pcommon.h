/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMMON_H
#define __PCOMMON_H
/*******************************************************************************
 FILE         :   pcommon.h
 COPYRIGHT    :   Yakov Markovitch, 1996-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Common definitions for PCOMMON library

 CREATION DATE:   27 June 1996
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcomn_macros.h>
#include <pcstring.h>

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>

#ifdef __cplusplus
#include <stdexcept>
#include <system_error>
#include <type_traits>
#include <utility>

#ifdef PCOMN_COMPILER_GNU
#define PCOMN_DEMANGLE(name) (::pcomn::demangle((name), std::array<char, 1024>().begin(), 1024))
#define PCOMN_CLASSNAME(type, ...) (pcomn::demangled_typename<type, ##__VA_ARGS__>(std::array<char, 1024>().begin(), 1024))
#else
#define PCOMN_DEMANGLE(name) (name)
#define PCOMN_CLASSNAME(type, ...) (pcomn::demangled_typename<type, ##__VA_ARGS__>())
#endif

#define PCOMN_TYPENAME(type_or_value) PCOMN_DEMANGLE(typeid(type_or_value).name())
#define PCOMN_DEREFTYPENAME(value) ((value) ? PCOMN_TYPENAME(*(value)) : PCOMN_TYPENAME((value)))

#define HEXOUT(k) "0x" << std::hex << (k) << std::dec
#define PTROUT(k) '[' << (const void *)(k) << ']'
#define EXPROUT(e) #e" = " << (e)
#define STDEXCEPTOUT(x) PCOMN_TYPENAME(x) << "('" << (x).what() << "')"

#if defined(__GLIBC__)
#include <errno.h>
#define PCOMN_PROGRAM_SHORTNAME (program_invocation_short_name)
#define PCOMN_PROGRAM_FULLNAME  (program_invocation_name)
#elif defined(PCOMN_COMPILER_MS) || defined(PCOMN_COMPILER_BORLAND)
#include <string.h>
#ifdef PCOMN_COMPILER_BORLAND
#define __argv _argv
#endif
#define PCOMN_PROGRAM_SHORTNAME (strrchr(*__argv, '\\') ? strrchr(*__argv, '\\') + 1 : *__argv)
#define PCOMN_PROGRAM_FULLNAME  (*__argv)
#endif

/*******************************************************************************
 Argument-checking exception-throw macros
*******************************************************************************/
/// Check that an argument is nonzero and throw std::invalid_argument otherwise.
/// @ingroup ExceptionMacros
/// @param arg
/// @return @a arg value.
/// @note Expression !(arg) should be valid and should return some @em unspecified-boolean-type.
/// @throw std::invalid_argument
///
/// Since the macro returns the value of its argument, it can be used as a 'filter', e.g.
/// @code
/// int bar(void *data, size_t length) ;
/// int foo(const char *str)
/// {
///    return bar(str, strlen(PCOMN_ENSURE_ARG(str))) ;
/// }
/// @endcode
#define PCOMN_ENSURE_ARG(arg) (::pcomn::ensure_arg<std::invalid_argument>((arg), #arg, __FUNCTION__))

#define PCOMN_ENSURE_ARGX(arg, exception) (::pcomn::ensure_arg<exception>((arg), #arg, __FUNCTION__))

#define PCOMN_ASSERT_ARG(assertion) \
   (::pcomn::ensure_arg_assertion<std::invalid_argument>((assertion), #assertion, __FUNCTION__))

#define PCOMN_ASSERT_ARGX(assertion, exception) \
   (::pcomn::ensure_arg_assertion<exception>((assertion), #assertion, __FUNCTION__))

/// Throw an exception with a formatted message.
/// @hideinitializer @ingroup ExceptionMacros
#define PCOMN_THROWF(exception, format, ...)  (pcomn::throwf<exception>(format, ##__VA_ARGS__))

/// Throw an exception with a formatted message if a specified condition holds.
/// @hideinitializer @ingroup ExceptionMacros
#define PCOMN_THROW_IF(condition, exception, format, ...)   \
do {                                                        \
   if (condition)                                           \
      PCOMN_THROWF(exception, format, ##__VA_ARGS__) ;      \
} while(false)

namespace pcomn {

using std::nullptr_t ;

const size_t PCOMN_MSGBUFSIZE = 1024 ;

/******************************************************************************/
/** Exception class: indicates some functionality is not implemented yet.
*******************************************************************************/
class not_implemented_error : public std::logic_error {
   public:
      explicit not_implemented_error(const std::string &functionality) :
         std::logic_error(functionality + " is not implemented")
      {}
} ;

/******************************************************************************/
/** Exception class: indicates that some implementation-defined limit exceeded.
*******************************************************************************/
class implimit_error : public std::logic_error {
   public:
      explicit implimit_error(const std::string &limit_description) :
         std::logic_error("Implementation limit exceeded: " + limit_description)
      {}
} ;

/******************************************************************************/
/** Base class for boolean tags
*******************************************************************************/
struct bool_value {
      constexpr bool_value() : _value(false) {}
      explicit constexpr bool_value(bool value) : _value(value) {}
      explicit constexpr operator bool() const { return _value ; }
   private:
      const bool _value ;
} ;

/******************************************************************************/
/** A tag type to specify whether to raise exception on error for functions
 that allow to indicate failure with a special return value.
*******************************************************************************/
struct RaiseError : bool_value { using bool_value::bool_value ; } ;

static const RaiseError DONT_RAISE_ERROR (false) ;
static const RaiseError RAISE_ERROR      (true) ;

/******************************************************************************/
/** Not-a-pointer: something, which is both not NULL and is not a valid pointer
*******************************************************************************/
template<typename T = void>
struct not_a_pointer
{
      static constexpr T * const value =
      #ifdef PCOMN_COMPILER_GNU
          __builtin_constant_p(reinterpret_cast<T *>(~(intptr_t())))
         ? reinterpret_cast<T *>(~(intptr_t()))
         : reinterpret_cast<T *>(~(intptr_t()))
      #else
         reinterpret_cast<T *>(~(intptr_t()))
      #endif
         ;
} ;

template<typename T>
constexpr T * const not_a_pointer<T>::value ;

constexpr void * const NaP = not_a_pointer<>::value ;

/*******************************************************************************
 void * pointer arithmetics
*******************************************************************************/
template<typename T>
constexpr inline T *padd(T *p, ptrdiff_t offset)
{
   return (T *)((char *)p + offset) ;
}

constexpr inline ptrdiff_t pdiff(const void *p1, const void *p2)
{
   return (const char *)p1 - (const char *)p2 ;
}

template<typename T>
inline T *&preinc(T *&p, ptrdiff_t offset)
{
   return p = padd(p, offset) ;
}

template<typename T>
inline T *postinc(T *&p, ptrdiff_t offset)
{
   T *old = p ;
   preinc(p, offset) ;
   return old ;
}

template<typename T>
constexpr inline T *rebase(T *ptr, const void *oldbase, const void *newbase)
{
    return ptr ? (T *)padd(newbase, pdiff(ptr, oldbase)) : nullptr ;
}

/*******************************************************************************
 Flagset processing functions
*******************************************************************************/
template<typename T>
constexpr inline bool is_flags_equal(T flags, T test, T mask)
{
   return !((flags ^ test) & mask) ;
}

template<typename T>
constexpr inline bool is_flags_on(T flags, T mask)
{
   return is_flags_equal(flags, mask, mask) ;
}

template<typename T>
constexpr inline bool is_flags_off(T flags, T mask)
{
   return is_flags_equal<T>(~flags, mask, mask) ;
}

template<typename T>
inline T &set_flags_masked(T &target, T flagset, T mask)
{
   return (target &= ~mask) |= flagset & mask ;
}

template<typename T>
inline T &set_flags(T &target, bool value, T mask)
{
   return set_flags_masked(target, (T() - (T)value), mask) ;
}

template<typename T>
inline T &set_flags_on(T &flags, T mask)
{
   return flags |= mask ;
}

template<typename T>
inline T &set_flags_off(T &flags, T mask)
{
   return flags &= ~mask ;
}

template<typename T>
inline T &inv_flags(T &flags, T mask)
{
   return flags ^= mask ;
}

template<typename T>
constexpr inline T is_inverted(T flag, T mask, long test)
{
   return !(flag & mask) ^ !test ;
}

template<typename T>
constexpr inline T flag_if(T flag, bool cond)
{
   return (T)(static_cast<T>(-(long)!!cond) & flag) ;
}

template<typename T>
constexpr inline T sign(T val)
{
   return (val < 0) ? -1 : (!!val) ;
}

/*******************************************************************************
 Tagged pointers; only for pointers to types with alignment > 1
*******************************************************************************/
#define static_check_taggable_(T)  static_assert(alignof(T) > 1, "Attempt to tag a pointer to unaligned type")

/// Make a pointer "tagged", set its LSBit to 1
template<typename T>
constexpr inline T *tag_ptr(T *ptr)
{
   static_check_taggable_(T) ;
   return (T *)(reinterpret_cast<uintptr_t>(ptr) | 1) ;
}

/// Untag a pointer, clear its LSBit.
template<typename T>
constexpr inline T *untag_ptr(T *ptr)
{
   static_check_taggable_(T) ;
   return (T *)(reinterpret_cast<uintptr_t>(ptr) &~ (uintptr_t)1) ;
}

/// Flip the pointer's LSBit.
template<typename T>
constexpr inline T *fliptag_ptr(T *ptr)
{
   static_check_taggable_(T) ;
   return (T *)(reinterpret_cast<uintptr_t>(ptr) ^ 1) ;
}

template<typename T>
constexpr inline bool is_ptr_tagged(T *ptr)
{
   static_check_taggable_(T) ;
   return reinterpret_cast<uintptr_t>(ptr) & 1 ;
}

template<typename T>
constexpr inline bool is_ptr_tagged_or_null(T *ptr)
{
   static_check_taggable_(T) ;
   return (!(reinterpret_cast<uintptr_t>(ptr) & ~1)) | (reinterpret_cast<uintptr_t>(ptr) & 1) ;
}

/// If a pointer is tagged or NULL, return NULL; otherwise, return the untagged
/// pointer value.
template<typename T>
constexpr inline T *null_if_tagged_or_null(T *ptr)
{
   static_check_taggable_(T) ;
   return (T *)(reinterpret_cast<uintptr_t>(ptr) & ((reinterpret_cast<uintptr_t>(ptr) & 1) - 1)) ;
}

/// If a pointer is untagged or NULL, return NULL; otherwise, return the untagged
/// pointer value.
template<typename T>
constexpr inline T *null_if_untagged_or_null(T *ptr)
{
   static_check_taggable_(T) ;
   return (T *)(reinterpret_cast<uintptr_t>(ptr) & ((~uintptr_t() + ((reinterpret_cast<uintptr_t>(ptr) - 1) & 1)) & ~1)) ;
}

#undef static_check_taggable_
/*******************************************************************************
 Ranges handling
*******************************************************************************/
template<typename T>
constexpr inline bool inrange(const T &value, const T &left, const T &right)
{
   return !(value < left || right < value) ;
}

template<typename T>
constexpr inline bool xinrange(const T &value, const T &left, const T &right)
{
   return !(value < left) && value < right ;
}

template<typename T, typename R>
constexpr inline bool inrange(const T &value, const std::pair<R, R> &range)
{
   return inrange<R>(value, range.first, range.second) ;
}

template<typename T, typename R>
constexpr inline bool xinrange(const T &value, const std::pair<R, R> &range)
{
   return xinrange<R>(value, range.first, range.second) ;
}

template<typename T>
constexpr inline ptrdiff_t range_length(const std::pair<T, T> &range)
{
   return range.second - range.first ;
}

template<typename T>
constexpr inline bool range_empty(const std::pair<T, T> &range)
{
   return range.second == range.first ;
}

template<typename T, typename U>
inline typename std::enable_if<std::is_convertible<U, T>::value, T>::type
xchange(T &dest, U &&src)
{
   T old (std::move(dest)) ;
   dest = std::forward<U>(src) ;
   return std::move(old) ;
}

template<typename  T>
inline const T& midval(const T& minVal, const T& maxVal, const T& val)
{
   return std::min(maxVal, std::max(minVal, val)) ;
}

template<typename T>
inline void ordered_swap(T &op1, T &op2)
{
   using std::swap ;
   if (op2 < op1)
      swap(op1, op2) ;
}

template<typename T, typename Compare>
inline void ordered_swap(T &op1, T &op2, Compare comp)
{
   using std::swap ;
   if (comp(op2, op1))
      swap(op1, op2) ;
}

template<typename T>
inline std::pair<T, T> ordered_pair(T &&op1, T &&op2)
{
   return (op1 < op2)
      ? std::pair<T, T>(std::forward<T>(op1), std::forward<T>(op2))
      : std::pair<T, T>(std::forward<T>(op2), std::forward<T>(op1)) ;
}

template<typename T, typename Compare>
inline std::pair<T, T> ordered_pair(T &&op1, T &&op2, Compare &&comp)
{
   return std::forward<Compare>(comp)(op1, op2)
      ? std::pair<T, T>(std::forward<T>(op1), std::forward<T>(op2))
      : std::pair<T, T>(std::forward<T>(op2), std::forward<T>(op1)) ;
}

/*******************************************************************************
 Out-of-line exception throw
*******************************************************************************/
template<class X, typename... XArgs>
__noreturn __noinline
void throw_exception(XArgs&& ...args) { throw X(std::forward<XArgs>(args)...) ; }

template<typename Msg>
__noreturn __noinline
void throw_system_error(std::errc errcode, const Msg &msg)
{
   throw_exception<std::system_error>(std::make_error_code(errcode), msg) ;
}

template<typename Msg>
__noreturn __noinline
void throw_system_error(int errno_code, const Msg &msg)
{
   throw_exception<std::system_error>(errno_code, std::system_category(), msg) ;
}

/// Throw exception with formatted message
///
template<class X, size_t bufsize = PCOMN_MSGBUFSIZE>
__noreturn __noinline
void throwf(const char *, ...) PCOMN_ATTR_PRINTF(1, 2) ;

template<class X, size_t bufsize>
__noreturn __noinline
void throwf(const char *format, ...)
{
   char buf[bufsize] ;
   va_list parm ;
   va_start(parm, format) ;
   vsnprintf(buf, sizeof buf, format, parm) ;
   va_end(parm) ;

   throw X(buf) ;
}

template<class X, typename... XArgs>
inline void conditional_throw(bool test, XArgs && ...args)
{
   if (unlikely(test))
      throw_exception<X>(std::forward<XArgs>(args)...) ;
}

template<class X, typename V, typename... XArgs>
inline void ensure(V &&value, XArgs && ...args)
{
   conditional_throw<X>(!value, std::forward<XArgs>(args)...) ;
}

/// Check whether the value is not zero.
/// If !value is true, throw X.
template<class X, typename V, typename... XArgs>
inline V &&ensure_nonzero(V &&value, XArgs && ...args)
{
   ensure<X>(std::forward<V>(value), std::forward<XArgs>(args)...) ;
   return std::forward<V>(value) ;
}

namespace detail {
template<typename X>
__noinline __noreturn
void throw_arg_null(const char *arg_name, const char *function_name)
{
   char message[512] ;
   const char *aname = arg_name, *quote = "'" ;
   if (unlikely(!arg_name))
      aname = quote = "" ;
   snprintf(message, sizeof message, "Invalid (NULL) argument %s%s%s%s%s.",
            quote, aname, quote, function_name ? " is passed to " : "", function_name ? function_name : "") ;
   throw_exception<X>(message) ;
}

template<typename X>
__noinline __noreturn
void throw_arg_assert(const char *assertion_text, const char *function_name)
{
   char message[512] ;
   if (unlikely(!assertion_text))
      assertion_text = "" ;
   if (unlikely(!function_name))
      function_name = "" ;
   snprintf(message, sizeof message, "Arguments assertion '%s' failed%s%s.",
            assertion_text, *function_name ? " in " : "", function_name) ;
   throw_exception<X>(message) ;
}
}

template<class X, typename V>
inline V &&ensure_arg(V &&value, const char *arg_name, const char *function_name)
{
   if (unlikely(!value))
      detail::throw_arg_null<X>(arg_name, function_name) ;
   return std::forward<V>(value) ;
}

template<class X>
inline void ensure_arg_assertion(bool assertion, const char *assertion_text, const char *function_name)
{
   if (unlikely(!assertion))
      detail::throw_arg_assert<X>(assertion_text, function_name) ;
}

/// Ensure that a value is between specified minimum and maximum (inclusive).
/// @throw X(message) if value is out of range.
template<class X, typename V, typename B, typename... XArgs>
inline V &&ensure_range(V &&value, B &&minval, B &&maxval, XArgs && ...args)
{
   ensure<X>(!(value < minval || maxval < value), std::forward<XArgs>(args)...) ;
   return std::forward<V>(value) ;
}

template<class X, typename V, typename B, typename... XArgs>
inline V &&ensure_lt(V &&value, B &&bound, XArgs && ...args)
{
   ensure<X>(value < bound, std::forward<XArgs>(args)...) ;
   return std::forward<V>(value) ;
}

template<class X, typename V, typename B, typename... XArgs>
inline V &&ensure_le(V &&value, B &&bound, XArgs && ...args)
{
   ensure<X>(!(bound < value), std::forward<XArgs>(args)...) ;
   return std::forward<V>(value) ;
}

template<class X, typename V, typename B, typename... XArgs>
inline V &&ensure_gt(V &&value, B &&bound, XArgs && ...args)
{
   ensure<X>(bound < value, std::forward<XArgs>(args)...) ;
   return std::forward<V>(value) ;
}

template<class X, typename V, typename B, typename... XArgs>
inline V &&ensure_ge(V &&value, B &&bound, XArgs && ...args)
{
   ensure<X>(!(value < bound), std::forward<XArgs>(args)...) ;
   return std::forward<V>(value) ;
}

template<class X, typename V, typename B, typename... XArgs>
inline V &&ensure_eq(V &&value, B &&bound, XArgs && ...args)
{
   ensure<X>(value == bound, std::forward<XArgs>(args)...) ;
   return std::forward<V>(value) ;
}

template<class X, typename V, typename B, typename... XArgs>
inline V &&ensure_ne(V &&value, B &&bound, XArgs && ...args)
{
   ensure<X>(!(value == bound), std::forward<XArgs>(args)...) ;
   return std::forward<V>(value) ;
}

template<typename M>
inline void ensure_precondition(bool precondition, const M &message)
{
   ensure<std::invalid_argument>(precondition, message) ;
}

/*******************************************************************************
 Allocation errors handling
*******************************************************************************/
struct bad_alloc_msg : public std::bad_alloc {

      bad_alloc_msg() noexcept
      {
         *_errbuf = 0 ;
      }

      explicit bad_alloc_msg(const char *msg) noexcept : bad_alloc_msg()
      {
         if (msg)
            strncpyz(_errbuf, msg, sizeof _errbuf) ;
      }

      const char *what() const noexcept override
      {
         return *_errbuf ? _errbuf : std::bad_alloc::what() ;
      }
   private:
      char _errbuf[128] ;
} ;

template<bool> inline __noreturn void handle_bad_alloc() { throw_exception<std::bad_alloc>() ; }
template<> inline void handle_bad_alloc<false>() {}

template<typename T>
inline T *ensure_allocated(T *p)
{
   if (!p)
      handle_bad_alloc<true>() ;
   return p ;
}

/******************************************************************************/
/** A destruction policy for use by std::unique_ptr for objects allocated
 with malloc.
 Uses ::free() to deallocate memory.
*******************************************************************************/
struct malloc_delete {
      void operator()(const void *ptr) const noexcept { ::free(const_cast<void *>(ptr)) ; }
} ;

/******************************************************************************/
/** Directly call the destructor of the object pointed to by @a p.
*******************************************************************************/
template<class T>
inline T *destroy(T *p)
{
   if (p)
      p->~T() ;
   return p ;
}

template<class T>
inline void destroy_ref(T &r)
{
   r.~T() ;
}

} // end of namespace pcomn

/******************************************************************************/
/** Swap wrapper that calls std::swap on the arguments, but may also use ADL
 (Argument-Dependent Lookup, Koenig lookup) to use a specialised form.

 Use this instead of std::swap to enable namespace-level swap functions, defined in the
 same namespaces as classes they defined for. Note that if there is no such function
 for T, this wrapper will fallback to std::swap().
*******************************************************************************/
template<typename T>
inline void pcomn_swap(T &a, T &b)
{
   using std::swap ;
   swap(a, b) ;
}

/*******************************************************************************
 Hex digit to number and number to hex digit
*******************************************************************************/
inline int hextoi(char hexdigit)
{
  switch (hexdigit)
  {
    case '0': return 0 ; case '1': return 1 ; case '2': return 2 ; case '3': return 3 ;
    case '4': return 4 ; case '5': return 5 ; case '6': return 6 ; case '7': return 7 ;
    case '8': return 8 ; case '9': return 9 ;

    case 'a': case 'A': return 10 ; case 'b': case 'B': return 11 ;
    case 'c': case 'C': return 12 ; case 'd': case 'D': return 13 ;
    case 'e': case 'E': return 14 ; case 'f': case 'F': return 15 ;
  }
  return -1 ;
}

inline int itohex(unsigned num)
{
   static constexpr const char xc[16] =
      { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' } ;
   return xc[num & 0xfu] & (-(int)!(num &~ 0xfu)) ;
}

inline void *hextob(void *buf, size_t bufsz, const char *hexstr)
{
   if (!buf || !hexstr && bufsz)
      return nullptr ;

   uint8_t *data = static_cast<uint8_t *>(buf) ;
   for (const char *c = hexstr, *e = c + 2 * bufsz ; c != e ; c += 2, ++data)
   {
      const int d1 = hextoi(c[0]) ;
      if (d1 < 0)
         return nullptr ;
      const int d2 = hextoi(c[1]) ;
      if (d2 < 0)
         return nullptr ;
      *data = (d1 << 4) | d2 ;
   }
   return buf ;
}

/*******************************************************************************

*******************************************************************************/
#define PCOMN_NONCOPYABLE(Class)   Class(const Class&) = delete
#define PCOMN_NONASSIGNABLE(Class) void operator=(const Class&) = delete

/// Given that there is '<' operator for the type, define all remaining ordering operators
/// as global functions.
#define PCOMN_DEFINE_ORDER_FUNCTIONS(pfx, type)                         \
   pfx inline bool operator>(const type &lhs, const type &rhs) { return rhs < lhs ; } \
   pfx inline bool operator<=(const type &lhs, const type &rhs) { return !(rhs < lhs) ; } \
   pfx inline bool operator>=(const type &lhs, const type &rhs) { return !(lhs < rhs) ; }

/// Given that there are operators '==' and '<' for the type, define through them all
/// remaining relational operators as global functions.
#define PCOMN_DEFINE_RELOP_FUNCTIONS(pfx, type)                         \
   pfx inline bool operator!=(const type &lhs, const type &rhs) { return !(lhs == rhs) ; } \
   PCOMN_DEFINE_ORDER_FUNCTIONS(P_PASS(pfx), P_PASS(type))

/// Given that there is '<' operator for the type, define all remaining ordering operators
/// as type methods.
#define PCOMN_DEFINE_ORDER_METHODS(type)                          \
   bool operator>(const type &rhs) { return rhs < *this ; }       \
   bool operator<=(const type &rhs) { return !(rhs < *this) ; }   \
   bool operator>=(const type &rhs) { return !(*this < rhs) ; }

/// Given that there are operators '==' and '<' for the type, define through them all
/// remaining relational operators as type methods.
#define PCOMN_DEFINE_RELOP_METHODS(type)                          \
   bool operator!=(const type &rhs) { return !(*this == rhs) ; }  \
   PCOMN_DEFINE_ORDER_METHODS(P_PASS(type))

/// Define operators '+' and '-' for the type through corresponding augmented operations
/// '+=' and '-='.
#define PCOMN_DEFINE_ADDOP_FUNCTIONS(pfx, type)                         \
   pfx inline type operator+(const type &lhs, const type &rhs) { type ret (lhs) ; return std::move(ret += rhs) ; } \
   pfx inline type operator-(const type &lhs, const type &rhs) { type ret (lhs) ; return std::move(ret -= rhs) ; }

#define PCOMN_DEFINE_NONASSOC_ADDOP_FUNCTIONS(pfx, type, rhstype)       \
   pfx inline type operator+(const type &lhs, const rhstype &rhs) { type ret (lhs) ; return std::move(ret += rhs) ; } \
   pfx inline type operator-(const type &lhs, const rhstype &rhs) { type ret (lhs) ; return std::move(ret -= rhs) ; }

/// Define operators '+' and '-' for the type through corresponding augmented operations
/// '+=' and '-='.
#define PCOMN_DEFINE_ADDOP_METHODS(type)                                \
   type operator+(const type &rhs) const { type ret (*this) ; return std::move(ret += rhs) ; } \
   type operator-(const type &rhs) const { type ret (*this) ; return std::move(ret -= rhs) ; }

#define PCOMN_DEFINE_NONASSOC_ADDOP_METHODS(type, rhstype)              \
   type operator+(const rhstype &rhs) const { type ret (*this) ; return std::move(ret += rhs) ; } \
   type operator-(const rhstype &rhs) const { type ret (*this) ; return std::move(ret -= rhs) ; }

/// Define post(inc|dec)rement.
#define PCOMN_DEFINE_POSTCREMENT(type, op) \
   type operator op(int) { type result (*this) ; op *this ; return result ; }

/// Define both postincrement and postdecrement.
#define PCOMN_DEFINE_POSTCREMENT_METHODS(type) \
   PCOMN_DEFINE_POSTCREMENT(type, ++)          \
   PCOMN_DEFINE_POSTCREMENT(type, --)

/// Providing there is @a type ::swap, define namespace-level swap overload @a type
#define PCOMN_DEFINE_SWAP(type, ...) \
   __VA_ARGS__ inline void swap(type &lhs, type &rhs) { lhs.swap(rhs) ; }

/// For a type without state, define operators '==' and '!=' that are invariantly 'true'
/// and 'false' respectively
///
/// I.e. we actually say "since object of type T is, in fact, just a namespace, consider
/// any two T objects equal".
#define PCOMN_DEFINE_INVARIANT_EQ(prefix, type)                         \
   prefix constexpr inline bool operator==(const type &, const type &) { return true ; } \
   prefix constexpr inline bool operator!=(const type &, const type &) { return false ; }

#define PCOMN_DEFINE_INVARIANT_PRINT(prefix, type)                      \
   prefix inline std::ostream &operator<<(std::ostream &os, const type &) { return os << PCOMN_CLASSNAME(type) ; }

#define PCOMN_DEFINE_PRANGE(type, ...)                                  \
   __VA_ARGS__ inline auto pbegin(type &x) -> decltype(&*std::begin(x)) { return std::begin(x) ; } \
   __VA_ARGS__ inline auto pbegin(const type &x) -> decltype(&*std::begin(x)) { return std::begin(x) ; } \
   __VA_ARGS__ inline auto pend(type &x) -> decltype(&*std::end(x)) { return std::end(x) ; } \
   __VA_ARGS__ inline auto pend(const type &x) -> decltype(&*std::end(x)) { return std::end(x) ; }


/*******************************************************************************
 Demangling
*******************************************************************************/
#ifdef PCOMN_COMPILER_GNU
#include <cxxabi.h>
#include <array>
#include <string.h>
#endif

namespace pcomn {

#ifdef PCOMN_COMPILER_GNU

inline const char *demangle(const char *mangled, char *buf, size_t buflen)
{
   int status = 0 ;
   return abi::__cxa_demangle(mangled, buf, &buflen, &status) ;
}

template<typename T>
inline const char *demangled_typename_(std::true_type, char *buf, size_t buflen)
{
  demangle(typeid(typename std::add_pointer<typename std::remove_reference<T>::type>::type).name(),
           buf, buflen) ;
  if (buflen > 1)
  {
    const size_t len = strlen(buf) ;
    if (len && buf[len - 1] == '*')
      buf[len - 1] = 0 ;
  }
  return buf ;
}

template<typename T>
inline const char *demangled_typename_(std::false_type, char *buf, size_t buflen)
{
  return demangle(typeid(T).name(), buf, buflen) ;
}

template<typename T>
inline const char *demangled_typename(char *buf, size_t buflen)
{
  return demangled_typename_<T>
    (std::integral_constant<bool,
     std::is_class<typename std::remove_reference<T>::type>::value ||
     std::is_union<typename std::remove_reference<T>::type>::value>(),
     buf, buflen) ;
}

#else // end of PCOMN_COMPILER_GNU

template<typename T>
inline const char *demangled_typename_(std::false_type)
{
  return typeid(T).name() ;
}

template<typename T>
inline const char *demangled_typename_(std::true_type)
{
   return demangled_typename_<typename std::add_pointer<typename std::remove_reference<T>::type>::type>(std::false_type()) ;
}

template<typename T>
inline const char *demangled_typename()
{
  return demangled_typename_<T>
    (std::integral_constant<bool,
     std::is_class<typename std::remove_reference<T>::type>::value ||
     std::is_union<typename std::remove_reference<T>::type>::value>()) ;
}

#endif // PCOMN_COMPILER_GNU

} // end of pcomn namespace

#endif /* __cplusplus */
#endif /* __PCOMMON_H */
