/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel" -*-*/
#ifndef __PCOMN_CONFIG_H
#define __PCOMN_CONFIG_H
/*******************************************************************************
 FILE         :   pcomn_config.h
 COPYRIGHT    :   Yakov Markovitch, 1997-2018. All rights reserved.
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
   #define PCOMN_NO_STRCASE_CONV   1
   Defined if strupr/strlwr are not supported by the compiler library

   #define PCOMN_NO_LOCALE_CONV    1
   Defined if _lstrupr/_lstrlwr are not supported by the compiler library

   #define PCOMN_NONSTD_UNIX_IO    1
   Defined if the compiler library uses UNIX I/O function with underscores (such as _open, etc.)
   instead of standard names
*/

#if PCOMN_COMPILER_CXX14
#define __deprecated(...) [[deprecated(__VA_ARGS__)]]
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

#ifdef PCOMN_COMPILER_GNU
/*******************************************************************************
 GCC
*******************************************************************************/
#define PCOMN_PRETTY_FUNCTION __PRETTY_FUNCTION__
#define PCOMN_ATTR_PRINTF(format_pos, param_pos) __attribute__((format(printf, format_pos, param_pos)))
#define PCOMN_ALIGNED(a) __attribute__((aligned (a)))
#undef GCC_MAKE_PRAGMA
#define GCC_MAKE_PRAGMA(text) _Pragma(#text)

#ifdef __noreturn
#undef __noreturn
#endif
#define __noreturn   __attribute__((__noreturn__))

#ifdef __may_alias
#undef __may_alias
#endif
#define __may_alias  __attribute__((__may_alias__))

#ifdef __noinline
#undef __noinline
#endif
#define __noinline __attribute__((__noinline__))

#ifdef __noinline
#undef __noinline
#endif
#define __noinline __attribute__((__noinline__))

#ifdef __cold
#undef __cold
#endif
#define __cold __attribute__((__noinline__, __cold__))

#ifdef __forceinline
#undef __forceinline
#endif
#define __forceinline inline __attribute__((__always_inline__))

#ifdef __restrict
#undef __restrict
#endif
#define __restrict __restrict__

#ifndef __deprecated
#define __deprecated(...) __attribute__((deprecated(__VA_ARGS__)))
#endif

#elif defined(PCOMN_COMPILER_MS)
/*******************************************************************************
 Microsoft
*******************************************************************************/
#define PCOMN_PRETTY_FUNCTION __FUNCTION__
#define PCOMN_ATTR_PRINTF(format_pos, param_pos)
#define PCOMN_ALIGNED(a) __declspec(align(a))
#undef MS_MAKE_PRAGMA
#define MS_MAKE_PRAGMA(arg, ...) __pragma(arg, ##__VA_ARGS__)

#ifdef __noreturn
#undef __noreturn
#endif
#define __noreturn __declspec(noreturn)

#ifdef __noinline
#undef __noinline
#endif
#define __noinline __declspec(noinline)

#ifdef __cold
#undef __cold
#endif
#define __cold __noinline

#ifdef __forceinline
#undef __forceinline
#endif
#define __forceinline inline __forceinline

#ifndef __deprecated
#define __deprecated(...) __declspec(deprecated(__VA_ARGS__))
#endif

#ifdef __may_alias
#undef __may_alias
#endif
#define __may_alias

#ifdef __restrict
#undef __restrict
#endif
#define __restrict __restrict

#else
/*******************************************************************************
 Others
*******************************************************************************/
#define PCOMN_PRETTY_FUNCTION __FUNCTION__
#define PCOMN_ATTR_PRINTF(format_pos, param_pos)
#define PCOMN_ALIGNED(a)

#ifndef __noreturn
#define __noreturn
#endif
#ifndef __deprecated
#define __deprecated(...)
#endif
#ifndef __noinline
#define __noinline
#endif
#ifndef __cold
#define __cold
#endif

#endif

#ifdef PCOMN_COMPILER_GNU
#  define  __inline __inline__
#endif

#ifdef __cplusplus
#include <typeinfo>
#endif   /* __cplusplus */

#endif /* __PCOMN_CONFIG_H */
