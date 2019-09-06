/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_PLATFORM_H
#define __PCOMN_PLATFORM_H
/*******************************************************************************
 FILE         :   pcomn_platform.h
 COPYRIGHT    :   Yakov Markovitch, 1996-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Platform identification macros

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   9 Nov 1996
*******************************************************************************/
/** @file
    Platform identification macros.

                  OS identification:
                       PCOMN_PL_WINDOWS (Win32, Win64)
                       PCOMN_PL_WIN64
                       PCOMN_PL_WIN32   (Win32,Win32 Console,Win32s)
                       PCOMN_PL_MS      (Strict MS environment, no GNU)
                       PCOMN_PL_OS2     (IBM OS/2 2.x)
                       PCOMN_PL_AS400   (IBM AS/400)
                       PCOMN_PL_UNIX    (All UNIX platforms)
                       PCOMN_PL_LINUX   (Linux)
                       PCOMN_PL_SOLARIS (Open Solaris)
                       PCOMN_PL_FREEBSD (FreeBSD)
                       PCOMN_PL_NETBSD  (NetBSD)
                       PCOMN_PL_OPENBSD (OpenBSD)
                       PCOMN_PL_BSD     (All BSD platforms)

                  Program mode identification:

                       PCOMN_MD_CONSOLE    (Console mode program; makes sense for Windows)
                       PCOMN_MD_GUI        (GUI program, as opposite to console mode. Makes sense for Windows.)
                       PCOMN_MD_DLL        (The program module is a dynamic-loadable library)

                       6) CPU endiannes

                       PCOMN_CPU_LITTLE_ENDIAN
                       PCOMN_CPU_BIG_ENDIAN

                       7) CPU platform

                       PCOMN_PL_X86
                       PCOMN_PL_ARM64
                       PCOMN_PL_POWER8

                       8) Character encoding

                       PCOMN_CHAR_ASCII
                       PCOMN_CHAR_EBCDIC

                       9) Platform standard path delimiters

                       PCOMN_PATH_DELIMS
                       PCOMN_PATH_NATIVE_DELIM

                       10) Function modifiers
                          PCOMN_CFUNC
                          extern "C" in C++ environment
                          empty in C environment

                          PCOMN_CDECL
                          __cdecl on MS platform
                          empty in all others

   #define PCOMN_NO_STRCASE_CONV   1
   Defined if strupr/strlwr are not supported by the compiler library

   #define PCOMN_NO_LOCALE_CONV    1
   Defined if _lstrupr/_lstrlwr are not supported by the compiler library

   #define PCOMN_NONSTD_UNIX_IO    1
   Defined if the compiler library uses UNIX I/O function with underscores (such as _open, etc.)
   instead of standard names
*******************************************************************************/
#include <pcomn_macros.h>

#include <sys/types.h>
#include <stdint.h>
#include <assert.h>

#ifdef __cplusplus
#include <typeinfo>
#endif   /* __cplusplus */

/*******************************************************************************
 Take care of some macros taht should be defined _only_ here
*******************************************************************************/
#ifdef PCOMN_CPU_LITTLE_ENDIAN
#undef PCOMN_CPU_LITTLE_ENDIAN
#endif
#ifdef PCOMN_CPU_BIG_ENDIAN
#undef PCOMN_CPU_BIG_ENDIAN
#endif

#ifdef PCOMN_PL_WINDOWS
#undef PCOMN_PL_WINDOWS
#endif
#ifdef PCOMN_PL_WIN32
#undef PCOMN_PL_WIN32
#endif
#ifdef PCOMN_PL_WIN64
#undef PCOMN_PL_WIN64
#endif
#ifdef PCOMN_PL_UNIX
#undef PCOMN_PL_UNIX
#endif
#ifdef PCOMN_PL_UNIX64
#undef PCOMN_PL_UNIX64
#endif
#ifdef PCOMN_PL_UNIX32
#undef PCOMN_PL_UNIX32
#endif
#ifdef PCOMN_PL_POSIX
#undef PCOMN_PL_POSIX
#endif

#ifdef PCOMN_PL_32BIT
#undef PCOMN_PL_32BIT
#endif
#ifdef PCOMN_PL_64BIT
#undef PCOMN_PL_64BIT
#endif

#ifdef PCOMN_COMPILER_MS
#undef PCOMN_COMPILER_MS
#endif
#ifdef PCOMN_COMPILER_GNU
#undef PCOMN_COMPILER_GNU
#endif
#ifdef PCOMN_COMPILER_INTEL
#undef PCOMN_COMPILER_INTEL
#endif
#ifdef PCOMN_COMPILER_IBM
#undef PCOMN_COMPILER_IBM
#endif
#ifdef PCOMN_COMPILER_C99
#undef PCOMN_COMPILER_C99
#endif
#ifdef PCOMN_COMPILER_CXX11
#undef PCOMN_COMPILER_CXX11
#endif
#ifdef PCOMN_STL_CXX11
#undef PCOMN_STL_CXX11
#endif
#ifdef PCOMN_COMPILER_CXX14
#undef PCOMN_COMPILER_CXX14
#endif
#ifdef PCOMN_STL_CXX14
#undef PCOMN_STL_CXX14
#endif
#ifdef PCOMN_COMPILER_CXX17
#undef PCOMN_COMPILER_CXX17
#endif
#ifdef PCOMN_STL_CXX17
#undef PCOMN_STL_CXX17
#endif
#ifdef PCOMN_RTL_MS
#undef PCOMN_RTL_MS
#endif
#ifdef PCOMN_COMPILER
#undef PCOMN_COMPILER
#endif
#ifdef PCOMN_COMPILER_NAME
#undef PCOMN_COMPILER_NAME
#endif

#define PCOMN_MD_CONSOLE 1

/*******************************************************************************
 Operating system platforms. See http://predef.sourceforge.net
*******************************************************************************/
#if defined(unix) || defined(__unix__) || defined(__unix)
#  define PCOMN_PL_UNIX 1
#  if defined(linux) || defined(__linux)
#     define PCOMN_PL_LINUX   1
#  elif defined(sun) || defined(__sun)
#     define PCOMN_PL_SOLARIS 1
#  elif defined(__FreeBSD__)
#     define PCOMN_PL_FREEBSD 1
#     define PCOMN_PL_BSD 1
#  elif defined(__NetBSD__)
#     define PCOMN_PL_NETBSD 1
#     define PCOMN_PL_BSD 1
#  elif defined(__OpenBSD__)
#     define PCOMN_PL_OPENBSD 1
#     define PCOMN_PL_BSD 1
#  endif

#elif defined(_WIN64) || defined(_WIN32) || defined(__WIN64__) || defined(__WIN32__) || defined(__TOS_WIN__)
#  define PCOMN_PL_WINDOWS 1
#  define WIN32_LEAN_AND_MEAN 1 /* Don't include too much in windows.h. Without this,
                                 * there are problems wit winsocket2.h */

#elif defined(OS2) || defined(_OS2) || defined(__OS2__) || defined(__TOS_OS2__)
#  define PCOMN_PL_OS2     1

#elif defined(__TOS_OS400__)
#  define PCOMN_PL_AS400   1

#else
#  error Unknown OS platform.
#endif

/*******************************************************************************
 CPU architectures. See http://predef.sourceforge.net
*******************************************************************************/
#if defined(_M_X64) || defined(__x86_64__) || defined(__amd64__)
#  define PCOMN_PL_64BIT            1
#  define PCOMN_PL_X86              1
#  define PCOMN_CPU_LITTLE_ENDIAN   1
#elif defined(_M_IX86) || defined(i386) || defined(_X86_) || defined(__THW_INTEL__)
#  define PCOMN_PL_32BIT            1
#  define PCOMN_PL_X86              1
#  define PCOMN_CPU_LITTLE_ENDIAN   1
#elif defined(__ia64__) || defined(_M_IA64) || defined(_M_ALPHA)
#  define PCOMN_PL_64BIT            1
#  define PCOMN_CPU_BIG_ENDIAN      1
#elif defined(PCOMN_PL_AS400)
#  define PCOMN_PL_64BIT            1
#  define PCOMN_CPU_BIG_ENDIAN      1
#else
#  error Unknown processor architecture.
#endif

#if defined(PCOMN_PL_64BIT)
#  define PCOMN_PL_CPUBITS   64

#  if defined(PCOMN_PL_WINDOWS)
#     define PCOMN_PL_WIN64   1
#  elif defined(PCOMN_PL_UNIX)
#     define PCOMN_PL_UNIX64   1
#  endif
#else
#  define PCOMN_PL_CPUBITS   32

#  if defined(PCOMN_PL_WINDOWS)
#     define PCOMN_PL_WIN32   1
#  elif defined(PCOMN_PL_UNIX)
#     define PCOMN_PL_UNIX32  1
#  endif
#endif

/*******************************************************************************
 Atomic operation support
*******************************************************************************/
#if   defined(PCOMN_PL_X86)

// No native LL/SC
#  define PCOMN_NATIVE_CAS    1
#  define PCOMN_ATOMIC_WIDTH  2

#elif defined(PCOMN_PL_ARM64)

// No native CAS, LL/SC supported
#  define PCOMN_NATIVE_LLSC  1
// There is support for double-witdth LL/SC (lqarx/stqcx)
#  define PCOMN_ATOMIC_WIDTH 2

#elif defined(PCOMN_PL_POWER8)

// No native CAS, LL/SC supported
#  define PCOMN_NATIVE_LLSC  1
// There is support for double-witdth LL/SC (lqarx/stqcx)
#  define PCOMN_ATOMIC_WIDTH 2

#else
#  define PCOMN_ATOMIC_WIDTH 1
#endif

/***************************************************************************//**
 GNU C/C++, at least 5.1.
*******************************************************************************/
#if defined (__GNUC__)
#  define __GNUC_VER__ (__GNUC__*100+__GNUC_MINOR__*10+__GNUC_PATCHLEVEL__)
#  define __CLANG_VER__ (__clang_major__*100+__clang_minor__*10+__clang_patchlevel__)

#  define PCOMN_COMPILER      GNU
#  define PCOMN_COMPILER_NAME "GCC"
#  define PCOMN_COMPILER_GNU  1
#  define PCOMN_COMPILER_C99  1

#  define __EXPORT __attribute__((visibility("default")))
#  define __IMPORT __attribute__((visibility("default")))

#  define thread_local_trivial __thread
#  define  __inline __inline__

/***************************************************************************//**
 Microsoft Visual C++ macros
*******************************************************************************/
#elif defined (_MSC_VER)

#  define PCOMN_COMPILER      MS
#  define PCOMN_COMPILER_NAME "MSVC++"
#  define PCOMN_COMPILER_MS   1

#  define __EXPORT __declspec(dllexport)
#  define __IMPORT __declspec(dllimport)

#  define thread_local_trivial __declspec(thread)

#  ifndef _WIN32_WINNT
#     define _WIN32_WINNT 0x400
#  endif

#  if defined(_DLL)
#     define PCOMN_MD_DLL  1
#     define _RTLDLL       1
#  endif

#  ifndef __CONSOLE__
#     undef PCOMN_MD_CONSOLE
#     define PCOMN_MD_GUI     1
#  endif

// ALWAYS IGNORE stupid "forcing value to bool 'true' or 'false' (performance warning)"
#pragma warning(disable : 4800)

/***************************************************************************//**
 IBM C++.
*******************************************************************************/
#elif defined (__IBMC__) || defined (__IBMCPP__)

#  if defined (PCOMN_PL_WINDOWS)

#     define __EXPORT __declspec(dllexport)
#     define __IMPORT __declspec(dllimport)

#     define thread_local_trivial __declspec(thread)

#     if defined (__DLL__)
#        define PCOMN_MD_DLL        1
#     endif

#     if defined (__IMPORTLIB__)
#        define _RTLDLL
#     endif

#  else
#     define PCOMN_PL_OS2           1
#  endif

#else
#  error Unknown or unsupported compiler.
#endif

#ifdef PCOMN_PL_AS400
#  define PCOMN_CHAR_EBCDIC
#  define PCOMN_PACKED _Packed
#else
#  define PCOMN_CHAR_ASCII
#  define PCOMN_PACKED
#endif

/*******************************************************************************
 MSVC++ does not define a correct __cplusplus macro value, so we explicitly
 test for MSVC version (1900 is Visual Studio 2015 compiler)
*******************************************************************************/
#if defined(__cplusplus)
#  if __cplusplus >= 201103L || (defined(_MSC_VER) && (_MSC_VER >= 1900 || defined(__INTEL_CXX11_MODE__)))
#     define PCOMN_COMPILER_CXX11 1
#     define PCOMN_STL_CXX11      1
#     if (defined(_MSC_VER) && _MSC_VER >= 1900)
#        define PCOMN_STL_CXX14   1
#        define PCOMN_STL_CXX17   1
#        define PCOMN_RTL_MS      1
#     endif
#  endif
#  if __cplusplus >= 201402L && __cplusplus < 201500L
#     define PCOMN_COMPILER_CXX14 1
#     define PCOMN_STL_CXX14      1
#  endif
#  if __cplusplus >= 201500L
#     define PCOMN_COMPILER_CXX17 1
#     define PCOMN_STL_CXX17      1
#  endif
#endif

#ifndef PCOMN_COMPILER
#error Unknown or unsupported C/C++ compiler. GCC 5.1, or MSVC++ 19.00, or Intel 15.0 required.
#endif

/*******************************************************************************
 Only C++11 or better is supported.
 Note MSVC++ does not define a correct __cplusplus macro value, so we explicitly
 test for MSVC version (1900 is Visual Studio 2015 compiler)
*******************************************************************************/
#if PCOMN_COMPILER_GNU && __GNUC_VER__ < 500 && __CLANG_VER__ < 370
#  error GNU C/C++ or Clang version is too old. Versions of GNU C/C++ earlier 5.1 or Clang earlier 3.7 are not supported.
#elif PCOMN_COMPILER_MS && (__cplusplus  && _MSC_VER < 1900 && !defined(__INTEL_CXX11_MODE__))
#  error Microsoft C/C++ _MSC_VER detected. Versions of MSVC below 19.00 (Visual Studio 2015) are not supported.
#endif

#if defined(__cplusplus) && !(defined(PCOMN_STL_CXX14) || defined(PCOMN_STL_CXX17))
#  error A compiler supporting at least C++14 standard is required to compile
#endif

/*******************************************************************************
 POSIX environment
*******************************************************************************/
#if defined(PCOMN_PL_UNIX) || defined(PCOMN_COMPILER_GNU)
#  define PCOMN_PL_POSIX 1
#endif

#if defined(PCOMN_PL_WINDOWS) && !defined(PCOMN_PL_POSIX)
#  define PCOMN_PL_MS 1 /* Pure Windows */
#endif

// Ensure __GLIBCXX__ defined if we are being compiled with libstdc++
#if defined(__cplusplus) && defined(PCOMN_COMPILER_GNU)
#  include <cstddef>
#endif

#if defined(__GLIBCXX__)
#  define PCOMN_BEGIN_NAMESPACE_CXX11 _GLIBCXX_BEGIN_NAMESPACE_CXX11
#  define PCOMN_END_NAMESPACE_CXX11   _GLIBCXX_END_NAMESPACE_CXX11
#else
#  define PCOMN_BEGIN_NAMESPACE_CXX11
#  define PCOMN_END_NAMESPACE_CXX11
#endif

/*******************************************************************************
 SIMD instruction sets
*******************************************************************************/
#if defined(PCOMN_PL_X86)
#  if defined(PCOMN_COMPILER_GNU)

#     ifdef __SSE4_2__
#        define PCOMN_PL_SIMD_SSE42 1
#     endif
#     ifdef __AVX__
#        define PCOMN_PL_SIMD_AVX   1
#     endif
#     ifdef __AVX2__
#        define PCOMN_PL_SIMD_AVX2  1
#     endif

#  endif /* PCOMN_COMPILER_GNU && PCOMN_PL_X86 */
#endif /* PCOMN_PL_X86 */

/*******************************************************************************
 Export/import macros
*******************************************************************************/
#if defined(__PCOMMON_BUILD)
#  define _PCOMNEXP   __EXPORT
#elif defined(__PCOMMON_DLL)
#  define _PCOMNEXP   __IMPORT
#else
#  define _PCOMNEXP
#endif

/*******************************************************************************
 We support only 64-bit platforms
*******************************************************************************/
#ifdef PCOMN_PL_32BIT
#error Only 64-bit platforms are supported.
#endif
/*******************************************************************************
 Microsoft C++ compilers
*******************************************************************************/
#if defined(PCOMN_COMPILER_MS)

#  define PCOMN_NONSTD_UNIX_IO        1
#  define NOMINMAX                    1

#  pragma warning(disable : 4786 4355 4275 4251 4099)

/*******************************************************************************
 IBM C++ compilers
*******************************************************************************/
#elif defined(PCOMN_COMPILER_IBM)

#  define PCOMN_NO_LOCALE_CONV        1

#  ifdef PCOMN_PL_AS400
#     define PCOMN_NO_STRCASE_CONV    1
#  endif

#elif defined(PCOMN_COMPILER_GNU)

#  define PCOMN_NO_STRCASE_CONV       1
#  pragma GCC diagnostic ignored "-Wparentheses"

#endif

#define PCOMN_USE(var) ((void)(var))

#if defined(PCOMN_PL_MS)

#  define PCOMN_PATH_DELIMS "\\"
#  define PCOMN_PATH_NATIVE_DELIM '\\'
#  define PCOMN_PATH_FOREIGN_DELIM '/'
#  define PCOMN_NULL_FILE_NAME "NUL"

#elif defined(PCOMN_PL_AS400)

#  define PCOMN_PATH_DELIMS "/\\"
#  define PCOMN_PATH_NATIVE_DELIM '/'
#  define PCOMN_PATH_FOREIGN_DELIM '\\'

#else // UNIX, etc.

#  define PCOMN_PATH_DELIMS "/"
#  define PCOMN_PATH_NATIVE_DELIM '/'
#  define PCOMN_PATH_FOREIGN_DELIM '\\'
#  define PCOMN_NULL_FILE_NAME "/dev/null"

#endif

#if defined(PCOMN_PL_MS)
#  define PCOMN_EOL_NATIVE "\r\n"
#else
#  define PCOMN_EOL_NATIVE "\n"
#endif

#ifdef __cplusplus
#  define PCOMN_CFUNC extern "C"
#else
#  define PCOMN_CFUNC
#endif

#ifdef PCOMN_PL_MS
#  define PCOMN_CDECL __cdecl
#else
#  define PCOMN_CDECL
#endif

#define GCC_MAKE_PRAGMA(text)
#define MS_MAKE_PRAGMA(text)

/*******************************************************************************
 Macro definitions for extended attribute specifiers, like __noreturn, __noinline, etc.,
 for various compilers.
 We must explicitly _undefine_ them first.
*******************************************************************************/
#ifdef __noreturn
#undef __noreturn
#endif
#ifdef __may_alias
#undef __may_alias
#endif
#ifdef __noinline
#undef __noinline
#endif
#ifdef __cold
#undef __cold
#endif
#ifdef __forceinline
#undef __forceinline
#endif
#ifdef __restrict
#undef __restrict
#endif

#ifdef PCOMN_COMPILER_GNU
/*******************************************************************************
 GCC
*******************************************************************************/
#define PCOMN_PRETTY_FUNCTION __PRETTY_FUNCTION__
#define PCOMN_ATTR_PRINTF(format_pos, param_pos) __attribute__((format(printf, format_pos, param_pos)))
#undef GCC_MAKE_PRAGMA
#define GCC_MAKE_PRAGMA(text) _Pragma(#text)

#define __noreturn      __attribute__((__noreturn__))
#define __noinline      __attribute__((__noinline__))
#define __cold          __attribute__((__noinline__, __cold__))
#define __forceinline   inline __attribute__((__always_inline__))
#define __restrict      __restrict__
#define __may_alias     __attribute__((__may_alias__))

#ifndef __deprecated
#define __deprecated(...) __attribute__((deprecated(__VA_ARGS__)))
#endif

#elif defined(PCOMN_COMPILER_MS)
/*******************************************************************************
 Microsoft
*******************************************************************************/
#define PCOMN_PRETTY_FUNCTION __FUNCTION__
#define PCOMN_ATTR_PRINTF(format_pos, param_pos)
#undef MS_MAKE_PRAGMA
#define MS_MAKE_PRAGMA(arg, ...) __pragma(arg, ##__VA_ARGS__)

#define __noreturn      __declspec(noreturn)
#define __noinline      __declspec(noinline)
#define __cold          __noinline
#define __forceinline   inline __forceinline
#define __restrict      __restrict
#define __may_alias

#ifndef __deprecated
#define __deprecated(...) __declspec(deprecated(__VA_ARGS__))
#endif

#else
/*******************************************************************************
 Others
*******************************************************************************/
#define PCOMN_PRETTY_FUNCTION __FUNCTION__
#define PCOMN_ATTR_PRINTF(format_pos, param_pos)

#define __noreturn
#define __noinline
#define __cold
#define __forceinline inline
#define __restrict
#define __may_alias

#ifndef __deprecated
#define __deprecated(...)
#endif

#endif

/***************************************************************************//**
 @def likely
 Provide the compiler with branch prediction information, assering the expression is
 likely to be true.

 @param expr Expression that should evaluate to integer.

 @def unlikely
 Provide the compiler with branch prediction information, assering the expression is
 likely to be false.

 @param expr Expression that should evaluate to integer.
*******************************************************************************/
#if !defined(likely) && !defined(unlikely)
#ifdef PCOMN_COMPILER_GNU
#define likely(expr)    __builtin_expect(!!(expr), 1)
#define unlikely(expr)  __builtin_expect(!!(expr), 0)
#else
#define likely(expr)    (expr)
#define unlikely(expr)  (expr)
#endif
#endif

/***************************************************************************//**
 @def PCOMN_ALIGNED(alignment)

 Alignment declaration for C.
 C++ has alignas().

 @param alignment Alignment, must be the power of 2.
*******************************************************************************/
#ifdef PCOMN_COMPILER_GNU
#  define PCOMN_ALIGNED(a) __attribute__((aligned (a)))

#elif defined(PCOMN_COMPILER_MS)
#  define PCOMN_ALIGNED(a) __declspec(align(a))

#else
#  define PCOMN_ALIGNED(a)
#endif

/***************************************************************************//**
 @def PCOMN_ASSUME_ALIGNED(pointer, alignment)

 Return `pointer` and allow the compiler to assume that the returned `pointer` is at
 least `alignment` bytes aligned.

 @return `pointer`; note that in contrast to GCC's __builtin_assume_aligned the type of
 returned value matches `pointer` type (i.e. it is not necessarily `void *`).

 @param pointer
 @param alignment Assumed alignment, must be the power of 2.

 @note No-op for any non-GCC or non-Clang compiler.
*******************************************************************************/
#ifdef PCOMN_COMPILER_GNU
#ifdef __cplusplus
#  define PCOMN_ASSUME_ALIGNED(pointer, alignment) \
   static_cast<std::remove_reference_t<decltype(pointer)>>(__builtin_assume_aligned((pointer), alignment))
#else
#  define PCOMN_ASSUME_ALIGNED(pointer, alignment) ((typeof(pointer))__builtin_assume_aligned((pointer), alignment))
#endif
#else
#  define PCOMN_ASSUME_ALIGNED(pointer, alignment) (pointer)
#endif

/*******************************************************************************
 Warning control
*******************************************************************************/
// GCC warning control
#define GCC_IGNORE_WARNING(warn) GCC_MAKE_PRAGMA(GCC diagnostic ignored "-W"#warn)
#define GCC_ENABLE_WARNING(warn) GCC_MAKE_PRAGMA(GCC diagnostic warning "-W"#warn)
#define GCC_SETERR_WARNING(warn) GCC_MAKE_PRAGMA(GCC diagnostic error "-W"#warn)

#define GCC_DIAGNOSTIC_PUSH() GCC_MAKE_PRAGMA(GCC diagnostic push)
#define GCC_DIAGNOSTIC_PUSH_IGNORE(warn) GCC_DIAGNOSTIC_PUSH() GCC_IGNORE_WARNING(warn)
#define GCC_DIAGNOSTIC_PUSH_ENABLE(warn) GCC_DIAGNOSTIC_PUSH() GCC_ENABLE_WARNING(warn)
#define GCC_DIAGNOSTIC_PUSH_SETERR(warn) GCC_DIAGNOSTIC_PUSH() GCC_SETERR_WARNING(warn)
#define GCC_DIAGNOSTIC_POP() GCC_MAKE_PRAGMA(GCC diagnostic pop)

// MS warning control
#define MS_IGNORE_WARNING(warnlist) MS_MAKE_PRAGMA(warning(disable : warnlist))
#define MS_ENABLE_WARNING(warnlist) MS_MAKE_PRAGMA(warning(default : warnlist))

#define MS_DIAGNOSTIC_PUSH() MS_MAKE_PRAGMA(warning(push))
#define MS_DIAGNOSTIC_POP()  MS_MAKE_PRAGMA(warning(pop))

#define MS_PUSH_IGNORE_WARNING(warnlist) MS_DIAGNOSTIC_PUSH() MS_IGNORE_WARNING(warnlist)

/*******************************************************************************
 GCC options control
*******************************************************************************/
#define GCC_OPTIONS_PUSH() GCC_MAKE_PRAGMA(GCC push_options)
#define GCC_OPTIONS_POP()  GCC_MAKE_PRAGMA(GCC pop_options)

#ifdef __OPTIMIZE__
#  define GCC_OPTIMIZE_PUSH(mode)                  \
      GCC_OPTIONS_PUSH()                           \
      GCC_MAKE_PRAGMA(GCC optimize #mode)
#else
#  define GCC_OPTIMIZE_PUSH(mode) GCC_OPTIONS_PUSH()
#endif

/***************************************************************************//**
 Starting from 2008, MS declares most POSIX functions deprecated: suppress
 the deprecated warning for the MS completelly.
*******************************************************************************/
#if PCOMN_WORKAROUND(_MSC_VER, >= 1400)
#pragma warning(disable : 4996)
#endif

#if PCOMN_COMPILER_CXX14
#ifdef __deprecated
#undef __deprecated
#endif
#define __deprecated(...) [[deprecated(__VA_ARGS__)]]
#endif

/*******************************************************************************
 Integer typedefs and constants
*******************************************************************************/
#ifdef PCOMN_PL_WIN64
typedef intptr_t ssize_t ;
#endif

typedef ssize_t fileoff_t ;
typedef size_t  filesize_t ;

typedef int16_t   int16_le ;
typedef uint16_t  uint16_le ;
typedef int32_t   int32_le ;
typedef uint32_t  uint32_le ;
typedef int64_t   int64_le ;
typedef uint64_t  uint64_le ;

typedef int16_t   int16_be ;
typedef uint16_t  uint16_be ;
typedef int32_t   int32_be ;
typedef uint32_t  uint32_be ;
typedef int64_t   int64_be ;
typedef uint64_t  uint64_be ;

#ifdef __cplusplus
namespace pcomn {
#endif

typedef signed char        schar_t ;
typedef unsigned char      uchar_t ;
typedef unsigned char      byte_t ;

typedef long long int            longlong_t ;
typedef unsigned long long int   ulonglong_t ;

#ifdef __cplusplus

const size_t KiB = 1024 ;
const size_t MiB = 1024*KiB ;
const size_t GiB = 1024*MiB ;

/***************************************************************************//**
 A single-value enum for use as a tag for instantiation of static template
 data and code.
*******************************************************************************/
enum class Instantiate {} ;

constexpr Instantiate instantiate {} ;

/*******************************************************************************
 Endianness conversions
*******************************************************************************/
#ifdef PCOMN_CPU_LITTLE_ENDIAN
constexpr bool cpu_little_endian = true ;
#else
constexpr bool cpu_little_endian = false ;
#endif

constexpr bool cpu_big_endian = !cpu_little_endian ;

template<size_t type_size> struct uint_type {} ;
template<> struct uint_type<1> { typedef uint8_t  type ; } ;
template<> struct uint_type<2> { typedef uint16_t type ; } ;
template<> struct uint_type<4> { typedef uint32_t type ; } ;
template<> struct uint_type<8> { typedef uint64_t type ; } ;

template<size_t type_size> struct _byte_reverter ;

template<>
struct _byte_reverter<1> {
      template<typename T>
      static T &invert(T &item) { return item ; }
} ;

#if !defined(PCOMN_COMPILER_GNU) && !defined(PCOMN_COMPILER_MS)
template<size_t type_size>
struct _byte_reverter {
      template<typename T>
      static T &invert(T &item)
      {
         typedef typename uint_type<type_size/2>::type uint_t ;
         uint_t *d = reinterpret_cast<uint_t *>(&item) ;
         // Attempting to use instruction-level parallelism
         uint_t tmp0 = d[0] ;
         uint_t tmp1 = d[1] ;
         d[0] = tmp1 ;
         d[1] = tmp0 ;
         _byte_reverter<type_size/2>::invert(d[0]) ;
         _byte_reverter<type_size/2>::invert(d[1]) ;
         return item ;
      }
} ;
#else
constexpr inline uint_type<2>::type reverse_bytes(uint_type<2>::type v)
{
   return
#ifdef PCOMN_COMPILER_GNU
      __builtin_bswap16(v)
#else
      _byteswap_ushort(v)
#endif
      ;
}

constexpr inline uint_type<4>::type reverse_bytes(uint_type<4>::type v)
{
   return
#ifdef PCOMN_COMPILER_GNU
      __builtin_bswap32(v)
#else
      _byteswap_ulong(v)
#endif
      ;
}

constexpr inline uint_type<8>::type reverse_bytes(uint_type<8>::type v)
{
   return
#ifdef PCOMN_COMPILER_GNU
      __builtin_bswap64(v)
#else
      _byteswap_uint64(v)
#endif
      ;
}
#endif

constexpr inline uint_type<1>::type reverse_bytes(uint_type<1>::type v)
{
   return v ;
}

// Both GCC and  Visual Studio compilers have byte swapping intrinsics
template<size_t type_size>
struct _byte_reverter {
      template<typename T>
      static T &invert(T &item)
      {
         typedef typename uint_type<type_size>::type uint_t ;
         uint_t *d = reinterpret_cast<uint_t *>(&item) ;
         *d = reverse_bytes(*d) ;
         return item ;
      }
} ;

/// Invert the parameter's endianness
template<typename T>
constexpr inline T &invert_endianness(T &item)
{
   typedef typename uint_type<sizeof(T)>::type uint_t ;
   return
      (*reinterpret_cast<uint_t *>(&item) = reverse_bytes(*reinterpret_cast<uint_t *>(&item))), item ;
}

template<typename T>
constexpr inline T &to_little_endian(T &item)
{
#ifdef PCOMN_CPU_LITTLE_ENDIAN
   return item ;
#else
   return invert_endianness(item) ;
#endif
}

template<typename T>
constexpr inline T &to_big_endian(T &item)
{
#ifdef PCOMN_CPU_BIG_ENDIAN
   return item ;
#else
   return invert_endianness(item) ;
#endif
}

template<typename T>
constexpr inline T value_to_little_endian(T item) { return to_little_endian(item) ; }
template<typename T>
constexpr inline T value_to_big_endian(T item) { return to_big_endian(item) ; }

template<typename T>
constexpr inline T &from_little_endian(T &item)
{
#ifdef PCOMN_CPU_LITTLE_ENDIAN
   return item ;
#else
   return invert_endianness(item) ;
#endif
}

template<typename T>
constexpr inline T &from_big_endian(T &item)
{
#ifdef PCOMN_CPU_BIG_ENDIAN
   return item ;
#else
   return invert_endianness(item) ;
#endif
}

template<typename T>
constexpr inline T value_from_little_endian(T item) { return from_little_endian(item) ; }
template<typename T>
constexpr inline T value_from_big_endian(T item) { return from_big_endian(item) ; }

template<class Data>
inline size_t __byte_pos(const Data &, size_t byte_num)
{
   assert(byte_num < sizeof (Data)) ;
#ifdef PCOMN_CPU_LITTLE_ENDIAN
   return byte_num ;
#else
   return sizeof(Data) - byte_num ;
#endif /* PCOMN_CPU_LITTLE_ENDIAN */
}

template<typename T>
inline uint8_t get_byte(const T *data, size_t byte_num)
{
   return reinterpret_cast<const char *>(data)[__byte_pos(*data, byte_num)] ;
}

template<typename T>
inline void put_byte(T *data, size_t byte_num, uint8_t byte)
{
   reinterpret_cast<const char *>(data)[__byte_pos(*data, byte_num)] = byte ;
}
} // end of namespace pcomn

/***************************************************************************//**
 @var PCOMN_CACHELINE_SIZE
 Typical L1 cacheline size for the architecture the library is compiled for.
*******************************************************************************/
constexpr size_t PCOMN_CACHELINE_SIZE =
#if !defined(PCOMN_PL_POWER8)
   64
#else
   128
#endif
   ;

#endif // __cplusplus

/***************************************************************************//**
 @def PCOMN_PLATFORM_HEADER(header)
 A macro to include pcommon headers from platform-dependent subdirectories (unix, win32)
*******************************************************************************/
#if defined(PCOMN_PL_POSIX)
#define PCOMN_PLATFORM_HEADER(header) P_STRINGIFY(unix/header)
#elif defined(PCOMN_PL_WINDOWS)
#define PCOMN_PLATFORM_HEADER(header) P_STRINGIFY(win32/header)
#else
#define PCOMN_PLATFORM_HEADER(header) "pcomn_unsupported.h"
#endif

#endif /* __PCOMN_PLATFORM_H */
