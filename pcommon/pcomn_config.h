/*-*- mode: c; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel" -*-*/
#ifndef __PCOMN_CONFIG_H
#define __PCOMN_CONFIG_H
/*******************************************************************************
 FILE         :   pcomn_config.h
 COPYRIGHT    :   Yakov Markovitch, 1997-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Configuration header (for PCOMMON to configure for different compilers,
                  OSes, etc.)

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   14 Nov 1997
*******************************************************************************/
#ifndef __PCOMN_PLATFORM_H
#error "Don't include pcomn_config.h directly, include pcomn_platform.h instead"
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

/*******************************************************************************
 Borland C++ compilers
*******************************************************************************/
#if defined(PCOMN_COMPILER_BORLAND)

#  define PCOMN_NONSTD_FP_PREDICATES  1
#  define PCOMN_NO_UNDERSCORE_CVT_FUN 1

/*******************************************************************************
 Microsoft C++ compilers
*******************************************************************************/
#elif defined(PCOMN_COMPILER_MS)

#  define PCOMN_NONSTD_UNIX_IO        1
#  define PCOMN_NONSTD_FP_PREDICATES  1
#  define PCOMN_NO_EX_RANDOM          1
#  define PCOMN_UNDERSCORE_CVT_FUN    1
#  define NOMINMAX                    1

#  pragma warning(disable : 4786 4355 4275 4251 4099)

/*******************************************************************************
 IBM C++ compilers
*******************************************************************************/
#elif defined(PCOMN_COMPILER_IBM)

#  define PCOMN_NO_LDOUBLE_MATHLIB    1
#  define PCOMN_NO_LOCALE_CONV        1
#  define PCOMN_UNDERSCORE_CVT_FUN    1

#  ifdef PCOMN_PL_AS400
#     define PCOMN_NO_STRCASE_CONV    1
#     define PCOMN_NO_LDOUBLE         1
#  endif

#  define PCOMN_NO_EX_RANDOM          1

#elif defined(PCOMN_COMPILER_GNU)

#  define PCOMN_NO_STRCASE_CONV       1
#  pragma GCC diagnostic ignored "-Wparentheses"

#endif

#if defined(PCOMN_NO_LDOUBLE) && !defined(PCOMN_NO_LDOUBLE_MATHLIB)

#  define PCOMN_NO_LDOUBLE_MATHLIB    1

#endif

/*
 * Alignment constants and macros
 */

#if defined(PCOMN_PL_AS400)
#  define PCOMN_STD_ALIGNMENT 16
#  define PCOMN_MIN_SAFE_ALIGNMENT 16
#else
#  define PCOMN_MIN_SAFE_ALIGNMENT 1
#  define PCOMN_STD_ALIGNMENT 8
#endif

#define PCOMN_PACKED_SIZE(sz,alg) \
   ((((sz)+(alg)-1)/(alg))*(alg))

#define PCOMN_STD_PACKED_SIZE(sz) \
   PCOMN_PACKED_SIZE((sz),PCOMN_STD_ALIGNMENT)

#define PCOMN_SAFE_PACKED_SIZE(sz) \
   PCOMN_PACKED_SIZE((sz),PCOMN_MIN_SAFE_ALIGNMENT)

#define PCOMN_USE(var) ((void)(var))

#if defined(PCOMN_PL_WINDOWS)

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

#if defined(PCOMN_PL_WINDOWS)
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

#ifdef PCOMN_COMPILER_GNU
#define PCOMN_PRETTY_FUNCTION __PRETTY_FUNCTION__
#define PCOMN_ATTR_PRINTF(format_pos, param_pos) __attribute__((format(printf, format_pos, param_pos)))
#define PCOMN_ALIGNED(a) __attribute__((aligned (a)))
#define GCC_MAKE_PRAGMA(text) _Pragma(#text)

#ifdef __noreturn
#undef __noreturn
#endif
#define __noreturn   __attribute__((__noreturn__))

#ifdef __deprecated
#undef __deprecated
#endif
#define __deprecated __attribute__((__deprecated__))

#ifdef __noinline
#undef __noinline
#endif
#define __noinline __attribute__((__noinline__))

#else
#define PCOMN_PRETTY_FUNCTION __FUNCTION__
#define PCOMN_ATTR_PRINTF(format_pos, param_pos)
#define PCOMN_ALIGNED(a)
#define GCC_MAKE_PRAGMA(text)

#ifndef __noreturn
#define __noreturn
#endif
#ifndef __deprecated
#define __deprecated
#endif
#ifndef __noinline
#define __noinline
#endif

#endif

#ifdef __cplusplus
#  define  ___inline_ inline
#elif defined(PCOMN_COMPILER_GNU)
#  define  __inline __inline__
#else
#  define  __inline
#endif

#ifdef __cplusplus
#include <typeinfo>
#endif   /* __cplusplus */

#endif /* __PCOMN_CONFIG_H */
