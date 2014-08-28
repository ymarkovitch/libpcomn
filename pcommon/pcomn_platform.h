/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_PLATFORM_H
#define __PCOMN_PLATFORM_H
/*******************************************************************************
 FILE         :   pcomn_platform.h
 COPYRIGHT    :   Yakov Markovitch, 1996-2014. All rights reserved.
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
                       PCOMN_PL_WIN32   (Win32,Win32 Console,Win32s,DPMI32)
                       PCOMN_PL_MS      (All Microsoft platforms, alias for PCOMN_PL_WINDOWS)
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
                       PCOMN_MD_DPMI32     (Borland extender 32 bit)
                       PCOMN_MD_MT         (Multi-threaded mode)
                       PCOMN_MD_DLL        (The program module is a dynamic-loadable library)

                       PCOMN_MD_SUN
                       PCOMN_MD_HP

                       6) Processor type

                       PCOMN_CPU_LITTLE_ENDIAN
                       PCOMN_CPU_BIG_ENDIAN

                       7) Character coding

                       PCOMN_CHAR_ASCII
                       PCOMN_CHAR_EBCDIC

                       8) Platform standard alignment boundary

                       PCOMN_STD_ALIGNMENT

                       9) Platform standard path delimiters

                       PCOMN_PATH_DELIMS
                       PCOMN_PATH_NATIVE_DELIM

                       11) Function modifiers
                          PCOMN_CFUNC
                          extern "C" in C++ environment
                          empty in C environment

                          PCOMN_CDECL
                          __cdecl on MS platform
                          empty in all others

                       12) PCOMN_VOID_NULL
*******************************************************************************/
#include <pcomn_macros.h>

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

#ifdef PCOMN_PL_32BIT
#undef PCOMN_PL_32BIT
#endif
#ifdef PCOMN_PL_64BIT
#undef PCOMN_PL_64BIT
#endif

#ifdef PCOMN_COMPILER_BORLAND
#undef PCOMN_COMPILER_BORLAND
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
#  define PCOMN_PL_MS      1 /* Alias for PCOMN_PL_WINDOWS */

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
#elif defined(_M_IX86) || defined(__x86_64__) || defined(i386) || defined(_X86_) || defined(__THW_INTEL__)
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

#  define GCC_IGNORE_WARNING(warn) GCC_MAKE_PRAGMA(GCC diagnostic ignored "-W"#warn)
#  define GCC_RESTORE_WARNING(warn) GCC_MAKE_PRAGMA(GCC diagnostic warning "-W"#warn)

#if PCOMN_WORKAROUND(__GNUC_VER__, >= 460)
#  define GCC_DIAGNOSTIC_PUSH() GCC_MAKE_PRAGMA(GCC diagnostic push)
#  define GCC_DIAGNOSTIC_POP() GCC_MAKE_PRAGMA(GCC diagnostic pop)
#else
#  define GCC_DIAGNOSTIC_PUSH()
#  define GCC_DIAGNOSTIC_POP()
#endif
/*******************************************************************************
 Borland C++
*******************************************************************************/
/** Borland C++ macros.
*******************************************************************************/
#ifdef __BORLANDC__

#  ifndef PCOMN_PL_WINDOWS
#     error Borland C++ is supported only for Windows operating systems family.
#  endif
# if __TURBOC__ < 0x591
#   error "Borland C++ compiler versions less than 5.91 are not supported."
# endif

#  define PCOMN_COMPILER         BORLAND
#  define PCOMN_COMPILER_NAME    "Borland C++"
#  define PCOMN_COMPILER_BORLAND 1

#  if defined(__DPMI32__)
#     define PCOMN_MD_DPMI32
#  endif

#  define __EXPORT __declspec(dllexport)
#  define __IMPORT __declspec(dllimport)

#  define thread_local_trivial __declspec(thread)

#  if defined(__DLL__)
#     define PCOMN_MD_DLL     1
#  endif

#  if defined(__MT__)
#     define PCOMN_MD_MT      1
#  endif

#  ifndef __CONSOLE__
#     undef PCOMN_MD_CONSOLE
#     define PCOMN_MD_GUI     1
#  endif

/*******************************************************************************
 Microsoft Visual C++
*******************************************************************************/
/** Microsoft Visual C++.
*******************************************************************************/
#elif defined (_MSC_VER)

#  if (_MSC_VER < 1310)
#     error Versions of MSVC++ below 7.1 (AKA VS.NET 2003) are not supported.
#  endif

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

#  if defined (_MT)
#     define PCOMN_MD_MT   1
#  endif

#  ifndef __CONSOLE__
#     undef PCOMN_MD_CONSOLE
#     define PCOMN_MD_GUI     1
#  endif

/*******************************************************************************
 GNU compiler collection.
*******************************************************************************/
/** GNU C/C++, at least 4.1.
*******************************************************************************/
#elif defined (__GNUC__)
#  define __GNUC_VER__ (__GNUC__*100+__GNUC_MINOR__*10+__GNUC_PATCHLEVEL__)

#  if (__GNUC_VER__ < 410)
#     error GNU C/C++ __GNUC__.__GNUC_MINOR__ detected. Versions of GNU C/C++ below 4.1 are not supported.
#  endif

#  define PCOMN_COMPILER      GNU
#  define PCOMN_COMPILER_NAME "GCC"
#  define PCOMN_COMPILER_GNU  1
#  define PCOMN_COMPILER_C99  1

#  if defined(_REENTRANT)
#     define PCOMN_MD_MT      1
#  endif

#  define __EXPORT __attribute__((visibility("default")))
#  define __IMPORT __attribute__((visibility("default")))

#  define thread_local_trivial __thread

/*******************************************************************************
 IBM C++ compiler.
*******************************************************************************/
/** IBM C++.
*******************************************************************************/
#elif defined (__IBMC__) || defined (__IBMCPP__)

#  if defined (PCOMN_PL_WINDOWS)

#     define __EXPORT __declspec(dllexport)
#     define __IMPORT __declspec(dllimport)

#     define thread_local_trivial __declspec(thread)

#     if defined (__DLL__)
#        define PCOMN_MD_DLL        1
#     endif

#     if defined (_MT)
#        define PCOMN_MD_MT         1
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
 Only C++11 or better is supported.
 Note MSVC++ does not define a correct __cplusplus macro value, so we explicitly
 test for MSVC version (1800 is Visual Studio 2013 compiler)
*******************************************************************************/
#if defined(__cplusplus) && __cplusplus < 201103L && (!defined(_MSC_VER) || _MSC_VER < 1800)
#  error A compiler supporting C++11 standard is required to compile
#endif

/******************************************************************************/
/** @def likely
 Provide the compiler with branch prediction information, assering the expression is
 likely to be true.

 @param expr Expression that should evaluate to integer.
*******************************************************************************/
/** @def unlikely
 Provide the compiler with branch prediction information, assering the expression is
 likely to be false.

 @param expr Expression that should evaluate to integer.
*******************************************************************************/
#if !defined(likely) && !defined(unlikely)
#ifdef PCOMN_COMPILER_GNU
#define likely(expr)    __builtin_expect((expr), 1)
#define unlikely(expr)  __builtin_expect((expr), 0)
#else
#define likely(expr)    (expr)
#define unlikely(expr)  (expr)
#endif
#endif

/******************************************************************************/
/** NULL void pointer, regardless of C or C++

 The problem is that in C NULL is void *, in C++ NULL is int (at least until C++0x, wher
 @b null keyword will be introduced). So we need additional NULL pointer that is
 invariantly have pointer type.
*******************************************************************************/
#define PCOMN_VOID_NULL ((void *)0)

#include <sys/types.h>

#ifndef PCOMN_COMPILER_MS
/*
 * For some reason (I guess why), MS has no of the most POSIX/C99 headers,
 * including stdint.h
 */
#include <stdint.h>


/** Represents file-offset value.
 */
typedef off_t  fileoff_t ;
typedef size_t filesize_t ;

#else

#include <stddef.h>
/*
 * System types for MS compilers (with explicit sizes)
 */
typedef signed char     int8_t ;
typedef unsigned char   uint8_t ;
typedef short           int16_t ;
typedef unsigned short  uint16_t ;
typedef int             int32_t ;
typedef unsigned        uint32_t ;
typedef __int64         int64_t ;
typedef unsigned  __int64 uint64_t ;
typedef int64_t         intmax_t ;
typedef uint64_t        uintmax_t ;

/** Represents 64-bit file-offset value.
    While both 32- and 64-bit MS Windows have 64-bit filesystem, the file-offset
    type off_t is 32-bit.
 */
typedef int64_t   fileoff_t ;
typedef uint64_t  filesize_t ;

#endif

typedef int16_t   int16_le ;
typedef uint16_t  uint16_le ;
typedef int32_t   int32_le ;
typedef uint32_t  uint32_le ;
typedef int64_t    int64_le ;
typedef uint64_t   uint64_le ;

typedef int16_t   int16_be ;
typedef uint16_t  uint16_be ;
typedef int32_t   int32_be ;
typedef uint32_t  uint32_be ;
typedef int64_t   int64_be ;
typedef uint64_t  uint64_be ;

/*
 * MS 8.0 declares most POSIX functions deprecated!!!
 */
#if PCOMN_WORKAROUND(_MSC_VER, >= 1400)
#pragma warning(disable : 4996)
#endif

/*
   #define PCOMN_NO_LDOUBLE        1
   Defined if the long double is not supported by the compiler

   #define PCOMN_NO_LDOUBLE_MATHLIB 1
   Defined if the long double math library is not supported by the compiler

   #define PCOMN_NO_STRCASE_CONV   1
   Defined if strupr/strlwr are not supported by the compiler library

   #define PCOMN_NO_LOCALE_CONV    1
   Defined if _lstrupr/_lstrlwr are not supported by the compiler library

   #define PCOMN_NONSTD_UNIX_IO    1
   Defined if the compiler library uses UNIX I/O function with underscores (such as _open, etc.)
   instead of standard names

   #define PCOMN_NO_EX_RANDOM      1

   #define PCOMN_NO_UNDERSCORE_CVT_FUN 1
   Defined if the compiler library defines itoa, ltoa, etc. instead of _itoa, _ltoa, etc.

   #define PCOMN_UNDERSCORE_CVT_FUN 1
   Defined if the compiler library defines ONLY _itoa, _ltoa (without non-underscore versions)

*/

#include <pcomn_config.h>
#include <assert.h>

#ifdef __cplusplus
namespace pcomn {
#endif

typedef signed char        schar_t ;
typedef unsigned char      uchar_t ;
typedef unsigned char      byte_t ;

/* re-typedef stdint.h types */
typedef int8_t  int8 ;
typedef int16_t int16 ;
typedef int32_t int32 ;
typedef int64_t int64 ;

typedef uint8_t  uint8 ;
typedef uint16_t uint16 ;
typedef uint32_t uint32 ;
typedef uint64_t uint64 ;

typedef long long int            longlong_t ;
typedef unsigned long long int   ulonglong_t ;

/*
 * The data type with maximal alignment boundary
 */
typedef double pcomn_maxaligned_t ;

#ifdef __cplusplus

const size_t KiB = 1024 ;
const size_t MiB = 1024*KiB ;
const size_t GiB = 1024*MiB ;

/*******************************************************************************
 Endianness conversions
*******************************************************************************/
template<size_t type_size> struct uint_type {} ;
template<> struct uint_type<1> { typedef uint8 type ; } ;
template<> struct uint_type<2> { typedef uint16 type ; } ;
template<> struct uint_type<4> { typedef uint32 type ; } ;
template<> struct uint_type<8> { typedef uint64 type ; } ;

template<size_t type_size>
struct _endiannes_changer {
      template<typename T>
      static T &change(T &item)
      {
         typedef typename uint_type<type_size/2>::type uint_t ;
         uint_t *d = reinterpret_cast<uint_t *>(&item) ;
         // Attempting to use instruction-level parallelism
         uint_t tmp0 = d[0] ;
         uint_t tmp1 = d[1] ;
         d[0] = tmp1 ;
         d[1] = tmp0 ;
         _endiannes_changer<type_size/2>::change(d[0]) ;
         _endiannes_changer<type_size/2>::change(d[1]) ;
         return item ;
      }
} ;

template<>
struct _endiannes_changer<1> {
      template<typename T>
      static T &change(T &item) { return item ; }
} ;

// invert_endianness()  -  invert the parameter's endianness
//
template<typename T>
inline T &invert_endianness(T &item)
{
   return _endiannes_changer<sizeof item>::change(item) ;
}

template<typename T>
inline T &to_little_endian(T &item)
{
#ifdef PCOMN_CPU_LITTLE_ENDIAN
   return item ;
#else
   return _endiannes_changer<sizeof item>::change(item) ;
#endif
}

template<typename T>
inline T &to_big_endian(T &item)
{
#ifdef PCOMN_CPU_BIG_ENDIAN
   return item ;
#else
   return _endiannes_changer<sizeof item>::change(item) ;
#endif
}

template<typename T>
inline T value_to_little_endian(T item) { return to_little_endian(item) ; }
template<typename T>
inline T value_to_big_endian(T item) { return to_big_endian(item) ; }

template<typename T>
inline T &from_little_endian(T &item)
{
#ifdef PCOMN_CPU_LITTLE_ENDIAN
   return item ;
#else
   return _endiannes_changer<sizeof item>::change(item) ;
#endif
}

template<typename T>
inline T &from_big_endian(T &item)
{
#ifdef PCOMN_CPU_BIG_ENDIAN
   return item ;
#else
   return _endiannes_changer<sizeof item>::change(item) ;
#endif
}

template<typename T>
inline T value_from_little_endian(T item) { return from_little_endian(item) ; }
template<typename T>
inline T value_from_big_endian(T item) { return from_big_endian(item) ; }

template<class Data>
inline size_t __byte_pos(const Data &, size_t byte_num)
{
   assert(byte_num < sizeof (Data)) ;
#ifdef PCOMN_CPU_LITTLE_ENDIAN
   return byte_num ;
#else
   return sizeof (Data) - byte_num ;
#endif /* PCOMN_CPU_LITTLE_ENDIAN */
}

template<class ScalarType>
inline unsigned char __get_byte(const ScalarType *data, size_t byte_num)
{
   return reinterpret_cast<const unsigned char *>(data)[__byte_pos(*data, byte_num)] ;
}

template<class ScalarType>
inline void __put_byte(ScalarType *data, size_t byte_num, unsigned char byte)
{
   reinterpret_cast<unsigned char *>(data)[__byte_pos(*data, byte_num)] = byte ;
}

} // end of namespace pcomn
#endif

#ifndef PCOMN_COMPILER
#  error \
   "Unknown or unsupported C/C++ compiler. "    \
   "The following compilers are supported: " \
   "GCC 4.1 and higher; MSVC++ 13.10 and higher; Borland C++ 5.9.1 and higher."
#endif

#if PCOMN_WORKAROUND(_MSC_VER, >= 1300)
#define snprintf _snprintf
#endif

#if PCOMN_WORKAROUND(_MSC_VER, < 1800)
#define alignas(num) __declspec(align(num))
#endif

#if PCOMN_WORKAROUND(__GNUC_VER__, < 480)
#define alignas(num) __attribute__((aligned(num)))
#endif

/*******************************************************************************
 PCOMN_PLATFORM_HEADER is a macro to include pcommon headers from platform-dependent
 subdirectories (unix, win32)
*******************************************************************************/
#if defined(PCOMN_PL_WINDOWS)
#define PCOMN_PLATFORM_HEADER(header) P_STRINGIFY(win32/header)
#elif defined(PCOMN_PL_UNIX)
#define PCOMN_PLATFORM_HEADER(header) P_STRINGIFY(unix/header)
#else
#define PCOMN_PLATFORM_HEADER(header) "pcomn_unsupported.h"
#endif

#endif /* __PCOMN_PLATFORM_H */
