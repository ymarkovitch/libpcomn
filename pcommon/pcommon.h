/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMMON_H
#define __PCOMMON_H
/*******************************************************************************
 FILE         :   pcommon.h
 COPYRIGHT    :   Yakov Markovitch, 1996-2020. All rights reserved.
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

/*******************************************************************************
 C++ code starts here
*******************************************************************************/
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
   (::pcomn::ensure_arg_assertion<std::invalid_argument>(!!(assertion), #assertion, __FUNCTION__))

#define PCOMN_ASSERT_ARGX(assertion, exception) \
   (::pcomn::ensure_arg_assertion<exception>(!!(assertion), #assertion, __FUNCTION__))

/// Throw an exception with a formatted message.
/// @hideinitializer @ingroup ExceptionMacros
#define PCOMN_THROWF(exception, format, ...)  (pcomn::throwf<exception>(format, ##__VA_ARGS__))

/// Throw an exception with a formatted message if a specified condition holds.
/// @hideinitializer @ingroup ExceptionMacros
#define PCOMN_THROW_IF(condition, exception, format, ...)   \
do {                                                        \
   if (unlikely((condition)))                               \
      PCOMN_THROWF(exception, format, ##__VA_ARGS__) ;      \
} while(false)

namespace pcomn {

using std::nullptr_t ;

const size_t PCOMN_MSGBUFSIZE = 1024 ;

/*******************************************************************************
 Forward declarations
*******************************************************************************/
template<typename> struct basic_strslice ;
typedef basic_strslice<char>     strslice ;
typedef basic_strslice<wchar_t>  wstrslice ;

/***************************************************************************//**
 Exception class: indicates some functionality is not implemented yet.
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

/***************************************************************************//**
 Base class for boolean tags
*******************************************************************************/
template<typename Derived>
struct bool_value {
      constexpr bool_value() = default ;
      explicit constexpr bool_value(bool value) : _value(value) {}
      constexpr operator bool() const { return _value ; }

      constexpr Derived operator!() const { return Derived(!_value) ; }

   private:
      bool _value = false ;
} ;

/***************************************************************************//**
 A tag type to specify whether to raise exception on error for functions
 that allow to indicate failure with a special return value.
*******************************************************************************/
struct RaiseError : bool_value<RaiseError> { using bool_value::bool_value ; } ;

constexpr const RaiseError DONT_RAISE_ERROR {false} ;
constexpr const RaiseError RAISE_ERROR      {true} ;

/******************************************************************************/
/** Not-a-pointer: something, which is both not NULL and is not a valid pointer
*******************************************************************************/
template<typename T = void>
struct not_a_pointer
{
      constexpr not_a_pointer() = default ;

      template<typename U>
      operator U*() const { return reinterpret_cast<U*>(~(intptr_t())) ; }
} ;

constexpr const not_a_pointer<> NaP = not_a_pointer<>{} ;

/******************************************************************************/
/** Single-value enum
*******************************************************************************/
enum class novalue : bool { _ } ;

constexpr const novalue NaV = novalue::_ ;

/*******************************************************************************
 void * pointer arithmetics
*******************************************************************************/
template<typename T>
constexpr inline T *padd(T *p, ptrdiff_t offset)
{
   return (T *)((char *)p + offset) ;
}

template<typename T, typename U>
constexpr inline T *pradd(U *p, ptrdiff_t offset)
{
   return (T *)((char *)p + offset) ;
}

inline ptrdiff_t pdiff(const void *p1, const void *p2)
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

template<typename T>
constexpr inline auto bufsize(size_t count) -> decltype(sizeof(T)*count)
{
   return sizeof(T) * count ;
}

template<typename T>
constexpr inline typename std::enable_if<std::is_same<T, void>::value, size_t>::type
bufsize(size_t count)
{
   return count ;
}

template<typename T>
constexpr inline auto bufsize(const volatile T *, size_t count) -> decltype(bufsize<T>(count))
{
   return bufsize<T>(count) ;
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

template<typename T, typename U>
inline T &set_flags(T &target, bool value, U mask)
{
   return set_flags_masked<T>(target, (T)(T() - (T)value), mask) ;
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

/// Select flags value or 0 depending on a boolean condition.
/// Facilitates e.g. setting OR-combination of flags where presence of each flags
/// depends on some individual bool value.
/// @code
/// enum FOOBAR { FL_FOO = 0x01, FL_BAR = 0x02 } ;
/// unsigned foobar = flags_if(FL_FOO, is_foo()) | flags_if(FL_BAR, is_bar()) ;
/// @endcode
template<typename T>
constexpr inline T flags_if(T flags, bool cond)
{
   return (T)(static_cast<T>(-(long long)!!cond) & flags) ;
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
constexpr inline auto range_length(const std::pair<T, T> &range) -> decltype(range.second-range.first)
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
constexpr inline const T& midval(const T& minVal, const T& maxVal, const T& val)
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
constexpr inline std::pair<T, T> ordered_pair(T &&op1, T &&op2)
{
   return (op1 < op2)
      ? std::pair<T, T>(std::forward<T>(op1), std::forward<T>(op2))
      : std::pair<T, T>(std::forward<T>(op2), std::forward<T>(op1)) ;
}

template<typename T, typename Compare>
constexpr inline std::pair<T, T> ordered_pair(T &&op1, T &&op2, Compare &&comp)
{
   return std::forward<Compare>(comp)(op1, op2)
      ? std::pair<T, T>(std::forward<T>(op1), std::forward<T>(op2))
      : std::pair<T, T>(std::forward<T>(op2), std::forward<T>(op1)) ;
}

/*******************************************************************************
 Out-of-line exception throw
*******************************************************************************/
template<class X, typename... XArgs>
__noreturn __cold
void throw_exception(XArgs&& ...args) { throw X(std::forward<XArgs>(args)...) ; }

/// Throw exception with formatted message
///
template<class X, size_t bufsize = PCOMN_MSGBUFSIZE>
__noreturn __cold
void throwf(const char *, ...) PCOMN_ATTR_PRINTF(1, 2) ;

template<class X, size_t bufsize>
__noreturn __cold
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
__noreturn __cold
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
__noreturn __cold
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

template<bool> inline void handle_bad_alloc() {}
template<> inline __noreturn void handle_bad_alloc<true>() { throw_exception<std::bad_alloc>() ; }

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

template<typename T>
constexpr inline T *as_ptr_mutable(const T *p) { return const_cast<T *>(p) ; }

template<typename T>
constexpr inline T &as_mutable(const T &v) { return const_cast<T &>(v) ; }

template<typename T>
constexpr inline T &as_lvalue(T &&v) { return v ; }

template<size_t alignment>
constexpr inline bool is_aligned_to(const void *p)
{
   static_assert(!((alignment - 1) & alignment),
                 "Invalid alignment specifiesd, the alignment must be a power of 2") ;
   return !((uintptr_t)p & (alignment - 1)) ;
}

template<typename T>
constexpr inline bool is_aligned_as(const void *p) { return is_aligned_to<alignof(T)>(p) ; }

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

/***************************************************************************//**
 ASCII-only fast character class test functions.
 Branchless, ILP-friendly, 1-2 cycles per test.

 @note Handle utf8 correctly, simply return false for any non-ascii characters.
*******************************************************************************/
/**@{*/
constexpr inline bool isdigit_ascii(int c)
{
   return (unsigned)c - (unsigned)'0' < 10U ;
}

constexpr inline bool isxdigit_ascii(int c)
{
   return isdigit_ascii(c) | ((unsigned)c - (unsigned)'a' < 6U) | ((unsigned)c - (unsigned)'A' < 6U) ;
}

constexpr inline bool islower_ascii(int c)
{
   return (unsigned)c - (unsigned)'a' < 26U ;
}

constexpr inline bool isupper_ascii(int c)
{
   return (unsigned)c - (unsigned)'A' < 26U ;
}

constexpr inline bool isalpha_ascii(int c)
{
   return isupper_ascii(c) | islower_ascii(c) ;
}

constexpr inline bool isalnum_ascii(int c)
{
   return ((unsigned)c - (unsigned)'0' < 10) | isalpha_ascii(c) ;
}
/**@}*/

/*******************************************************************************
 Hex digit to number and number to hex digit
*******************************************************************************/
inline int hexchartoi(int hexdigit)
{
   constexpr static int8_t v[] =
   {
      // 10 items (decimal digits)
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
      // 7 items
      -1,-1,-1,-1,-1,-1,-1,
      // 6 items ('A'-'F')
      10, 11, 12, 13, 14, 15,
      // 26 items
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      // 6 items ('a'-'f')
      10, 11, 12, 13, 14, 15,

      // There are 73 items below
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
      -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
   } ;

   static_assert(sizeof v == 128, "Invalid array v size") ;

   const int32_t offs = (int8_t)hexdigit - '0' ;
   return v[offs & 0x7f] | (offs >> 31) ;
}

inline int itohexchar(unsigned num)
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
      const int d1 = hexchartoi(c[0]) ;
      if (d1 < 0)
         return nullptr ;
      const int d2 = hexchartoi(c[1]) ;
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
   pfx inline auto operator> (const type &x, const type &y)->decltype(!(x<y)) { return !!(y < x) ; } \
   pfx inline auto operator<=(const type &x, const type &y)->decltype(!(x<y)) { return !(y < x) ; }  \
   pfx inline auto operator>=(const type &x, const type &y)->decltype(!(x<y)) { return !(x < y) ; }

/// Given that there are operators '==' and '<' for the type, define through them all
/// remaining relational operators as global functions.
#define PCOMN_DEFINE_RELOP_FUNCTIONS(pfx, type)                         \
   pfx inline auto operator!=(const type &x, const type &y)->decltype(!(x==y)) { return !(x == y) ; } \
   PCOMN_DEFINE_ORDER_FUNCTIONS(P_PASS(pfx), P_PASS(type))

/// Given that there is '<' operator for the type, define all remaining ordering operators
/// as type methods.
#define PCOMN_DEFINE_ORDER_METHODS(type, ...)                           \
   __VA_ARGS__ bool operator>(const type &rhs) const { return rhs < *this ; }       \
   __VA_ARGS__ bool operator<=(const type &rhs) const { return !(rhs < *this) ; }   \
   __VA_ARGS__ bool operator>=(const type &rhs) const { return !(*this < rhs) ; }

/// Given that there are operators '==' and '<' for the type, define through them all
/// remaining relational operators as type methods.
#define PCOMN_DEFINE_RELOP_METHODS(type, ...)                           \
   __VA_ARGS__ bool operator!=(const type &rhs) const { return !(*this == rhs) ; } \
   PCOMN_DEFINE_ORDER_METHODS(P_PASS(type), ##__VA_ARGS__)

/// Define operators '+' and '-' for the type through corresponding augmented operations
/// '+=' and '-='.
#define PCOMN_DEFINE_ADDOP_FUNCTIONS(pfx, type)                         \
   pfx inline type operator+(const type &lhs, const type &rhs) { type ret(lhs) ; ret += rhs ; return ret ; } \
   pfx inline type operator-(const type &lhs, const type &rhs) { type ret(lhs) ; ret -= rhs ; return ret ; }

#define PCOMN_DEFINE_NONASSOC_ADDOP_FUNCTIONS(pfx, type, rhstype)       \
   pfx inline type operator+(const type &lhs, const rhstype &rhs) { type ret(lhs) ; ret += rhs ; return ret ; } \
   pfx inline type operator-(const type &lhs, const rhstype &rhs) { type ret(lhs) ; ret -= rhs ; return ret ; }

#define PCOMN_DEFINE_COMMUTATIVE_OP_FUNCTIONS(pfx, op, type, othertype)  \
   pfx inline type operator+(const type &self, const othertype &other) { type ret(self) ; ret op##= other ; return ret ; } \
   pfx inline type operator+(const othertype &other, const type &self) { return self op other ; }

/// Define operators '+' and '-' for the type through corresponding augmented operations
/// '+=' and '-='.
#define PCOMN_DEFINE_ADDOP_METHODS(type)                                \
   type operator+(const type &rhs) const { type ret(*this) ; ret += rhs ; return ret ; } \
   type operator-(const type &rhs) const { type ret(*this) ; ret -= rhs ; return ret ; }

#define PCOMN_DEFINE_NONASSOC_ADDOP_METHODS(type, rhstype)              \
   type operator+(const rhstype &rhs) const { type ret(*this) ; ret += rhs ; return ret ; } \
   type operator-(const rhstype &rhs) const { type ret(*this) ; ret -= rhs ; return ret ; }

/// Define post(inc|dec)rement.
#define PCOMN_DEFINE_POSTCREMENT(type, op) \
   type operator op(int) { type result (*this) ; op *this ; return result ; }

/// Define both postincrement and postdecrement.
#define PCOMN_DEFINE_POSTCREMENT_METHODS(type) \
   PCOMN_DEFINE_POSTCREMENT(type, ++)          \
   PCOMN_DEFINE_POSTCREMENT(type, --)

/// Providing there is @a type ::swap, define namespace-level swap overload @a type
#define PCOMN_DEFINE_SWAP(type, ...) \
   __VA_ARGS__ inline void swap(type &lhs, type &rhs) noexcept { lhs.swap(rhs) ; }

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

/// Define bit flag operations (|,&,~) over the specified enum type.
#define PCOMN_DEFINE_FLAG_ENUM(enum_type)                               \
   PCOMN_STATIC_CHECK(std::is_enum<enum_type>()) ;                      \
   constexpr inline enum_type operator&(enum_type x, enum_type y)       \
   {                                                                    \
      typedef std::underlying_type_t<enum_type> int_type ;              \
      return (enum_type)((int_type)x & (int_type)y) ;                   \
   }                                                                    \
   constexpr inline enum_type operator|(enum_type x, enum_type y)       \
   {                                                                    \
      typedef std::underlying_type_t<enum_type> int_type ;              \
      return (enum_type)((int_type)x | (int_type)y) ;                   \
   }                                                                    \
   constexpr inline enum_type operator^(enum_type x, enum_type y)       \
   {                                                                    \
      typedef std::underlying_type_t<enum_type> int_type ;              \
      return (enum_type)((int_type)x ^ (int_type)y) ;                   \
   }                                                                    \
   constexpr inline enum_type operator~(enum_type x)                    \
   {                                                                    \
      return (enum_type)(~(std::underlying_type_t<enum_type>)x) ;       \
   }                                                                    \
   inline enum_type &operator|=(enum_type &x, enum_type y) { return x = x | y ; } \
   inline enum_type &operator&=(enum_type &x, enum_type y) { return x = x & y ; } \
   inline enum_type &operator^=(enum_type &x, enum_type y) { return x = x ^ y ; }


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

/// @note This function never returns NULL and and the returned string is always
/// null-terminated.
inline const char *demangle(const char *mangled, char *buf, size_t buflen)
{
   if (!buflen || !buf)
      return "" ;
   *buf = 0 ;
   if (mangled && *mangled)
   {
      int status = 0 ;
      size_t len = buflen ;
      // On failure, return the source name
      if (!abi::__cxa_demangle(mangled, buf, &len, &status))
         strncpy(buf, mangled, buflen - 1)[buflen - 1] = 0 ;
   }
   return buf ;
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
