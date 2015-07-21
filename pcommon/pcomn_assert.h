/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_ASSERT_H
#define __PCOMN_ASSERT_H
/*******************************************************************************
 FILE         :   pcomn_assert.h
 COPYRIGHT    :   Yakov Markovitch, 1999-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Inline-debugging macros for C and C++

 CREATION DATE:   11 Jan 1999
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcomn_macros.h>

#include <assert.h>
#include <stdlib.h>

/*
   Undefine all C-enabled macros.
   They will be defined later.
*/

#undef NOXPRECONDITION
#undef NOXPRECONDITIONX

#undef NOXCHECK
#undef NOXCHECKX

#undef PARANOID_NOXCHECK

#define NOXPRECONDITION(condition)    NOXPRECONDITIONX((condition), #condition)
#define NOXCHECK(condition)           NOXCHECKX((condition), #condition)
#define PARANOID_NOXCHECK(condition)  PARANOID_NOXCHECKX((condition), #condition)

#define PCOMN_FAIL(msg)                                                 \
   ((void)(pcomn_fail("Failure: %s, file %s, line %d\n", (char *)(msg), __FILE__, __LINE__)))

#define PCOMN_ENSURE(p, s) (likely(!!(p)) ? (1) : (PCOMN_FAIL(s), 0))

#define PCOMN_VERIFY(p)                                                 \
   (unlikely(!(p)) ? (pcomn_fail(                                       \
                         "Verify failed: %s, file %s, line %d\n",       \
                         (char *)#p, __FILE__, __LINE__), 0) : (1))

#define NOXFAIL(msg) PCOMN_FAIL(msg)

#ifndef __PCOMN_DEBUG

#define NOXPRECONDITIONX(p,s)   ((void)0)
#define NOXCHECKX(p,s)          ((void)0)
#define PARANOID_NOXCHECKX(p,s) ((void)0)
#define NOXVERIFY(p)            ((bool)(p))
#define NOXDBG(...)

#define PCOMN_DEBUG_FAIL(msg) ((void)0)

#else

#define _NOXCHECKX(condition, message, type)                            \
   (!(condition) ? (pcomn_fail(                                         \
                       (#type " violated: %s, file %s, line %d\n"),     \
                       (char *)message, __FILE__, __LINE__)) : (void)0)

#define NOXPRECONDITIONX(condition, message) _NOXCHECKX((condition), message, Precondition)
#define NOXCHECKX(condition, message) _NOXCHECKX((condition), message, Check)
#define NOXVERIFY(p) PCOMN_VERIFY(p)

#if __PCOMN_DEBUG < 255
#define PARANOID_NOXCHECKX(condition, message) ((void)0)
#else
#define PARANOID_NOXCHECKX(condition, message) _NOXCHECKX((condition), message, Paranoid check)
#endif

#define NOXDBG(...) P_PASS_I(__VA_ARGS__)

#define PCOMN_DEBUG_FAIL(msg) PCOMN_FAIL(msg)

#endif


#ifdef PCOMN_COMPILER_BORLAND

#  define __pcomn_assert_fail__(fmt, msg, file, line)         \
   __assertfail((char *)(fmt), (char *)(msg), (char *)(file), (line))

#elif defined(PCOMN_COMPILER_MS)

// We need the following declaration to overcome the effect _NDEBUG macro,
// which turns off the corresponding declaration in the <assert.h>
#if _MSC_VER < 1300

PCOMN_CFUNC _CRTIMP void __cdecl _assert(void *, void *, unsigned) ;

#  define __pcomn_assert_fail__(fmt, msg, file, line) _assert((char *)(msg), (char *)(file), (line))

#else // VC++ 7.1

PCOMN_CFUNC _CRTIMP void __cdecl _assert(const char *, const char *, unsigned) ;

#ifndef _DEBUG
#  define __pcomn_assert_fail__(fmt, msg, file, line) (_assert((char *)(msg), (char *)(file), (line)))
#else
#include <wtypes.h>
#include <stdio.h>
#include <signal.h>
#include <stdint.h>

#ifndef __cplusplus
#  define __pcomn_assert_fail__(fmt, msg, file, line)                   \
   ((intptr_t)GetStdHandle(STD_ERROR_HANDLE) > 0 &&                          \
    (fputc('\n', stderr) && fprintf(stderr, (char *)(fmt), (char *)(msg), (char *)(file), (line)) > 0 && \
     (_flushall(), raise(SIGABRT), exit(3), 1)) || (_assert((char *)(msg), (char *)(file), (line)), 1))
#else /* __cplusplus */
template<typename N>
__noreturn __noinline void __pcomn_assert_fail__(const char *fmt, const char *msg, const char *file, N line)
{
   // This is a hack: there is no good and simple way to know in MSVC whether
   // the application is console or windows.
   if ((intptr_t)GetStdHandle(STD_ERROR_HANDLE) > 0)
   {
      ::fputc('\n', stderr) ;
      ::fprintf(stderr, fmt, msg, file, static_cast<unsigned>(line)) ;
      ::_flushall() ;
      ::raise(SIGABRT) ;
      ::exit(3) ;
   }
   else
      _assert(msg, file, line) ;
}
#endif /* __cplusplus */
#endif /* _DEBUG */

#endif // VC++ 7.1

#elif defined(PCOMN_COMPILER_GNU)

#include <error.h>
#include <stdlib.h>
#include <stdio.h>

static __inline
void __pcomn_assert_fail__(const char *fmt, const char *msg, const char *file, unsigned line)
{
   putc('\n', stderr) ;
   error_at_line(0, 0, file, line, fmt, msg, file, line) ;
   abort() ;
}

#else /* Unknown compiler */
PCOMN_CFUNC void __passertfail(const char *fmt, const char *msg, const char *file, int line) ;
#  define __pcomn_assert_fail__(fmt, msg, file, line) (__passertfail((fmt), (msg), (file), (line)))
#endif

#if defined(__PCOMN_DEBUG)

#if defined(PCOMN_PL_WINDOWS)

/* Damn! Borland C++ Builder has broken wtypes.h */
#ifdef PCOMN_COMPILER_BORLAND
#include <rpc.h>
#include <rpcndr.h>
#endif

#include <wtypes.h>
#ifdef __cplusplus
extern "C" {
#endif
DECLSPEC_IMPORT BOOL WINAPI IsDebuggerPresent() ;
DECLSPEC_IMPORT void WINAPI DebugBreak(void) ;
#ifdef __cplusplus
}
#endif

#elif defined(PCOMN_PL_LINUX) && defined(PCOMN_PL_X86)
#include <stdlib.h>

#define IsDebuggerPresent() (!!getenv("PCOMN_DEBUGGING"))
static inline void DebugBreak() { __asm__("int3") ; }

#else

#  define __pcomn_debug_fail__(fmt, msg, file, line) __pcomn_assert_fail__((fmt), (msg), (file), (line))

#endif /* End of __PCOMN_DEBUG && PCOMN_PL_WINDOWS */

#define __pcomn_debug_fail__(fmt, msg, file, line) \
   (IsDebuggerPresent() ? DebugBreak() : (__pcomn_assert_fail__((fmt), (msg), (file), (line))))

#else

#  define __pcomn_debug_fail__(fmt, msg, file, line) __pcomn_assert_fail__((fmt), (msg), (file), (line))

#endif /* __PCOMN_DEBUG */

#ifdef __cplusplus
template<typename N>
__noinline __noreturn void pcomn_fail(const char *fmt, const char *msg, const char *file, N line) noexcept
{
   __pcomn_debug_fail__(fmt, msg, file, line) ;
   abort() ;
}
#else
#  define pcomn_fail(fmt,msg,file,line) __pcomn_debug_fail__((fmt), (msg), (file), (line))
#endif

#ifdef __cplusplus
#define PCOMN_STATIC_CHECK(...) static_assert((__VA_ARGS__), #__VA_ARGS__)
#endif // __cplusplus

#endif // __PCOMN_ASSERT_H
