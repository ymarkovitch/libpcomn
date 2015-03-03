/*-*- mode: c; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((inclass . ++)) -*-*/
#ifndef __PCOMN_NRTHREADMUTEX_H
#define __PCOMN_NRTHREADMUTEX_H
/*******************************************************************************
 FILE         :   pcomn_nrthreadmutex.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Fast non-recursive mutex. Has the same logic as Event. I.e. from the one hand,
                  thread can block itself trying to acquire NRMutex it already held even by itself,
                  but on the other hand acquire and release on one lock can be performed by
                  different threads (which is impossible for, say, CRITICAL_SECTION).

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   20 Feb 2000
*******************************************************************************/
#include <windows.h>
#include <pcomn_def.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NRMUTEX {
      LONG   owned ;       /* <0 - free, >=0 - acquired (>0 - number of waiting threads)  */
      DWORD  thread_id ;   /* Thread which acquired mutex. For debug purposes. */
      HANDLE hevent ;
} NRMUTEX, *PNRMUTEX ;

_PCOMNEXP BOOL InitializeNonRecursiveMutex(PNRMUTEX mutex) ;

_PCOMNEXP VOID DeleteNonRecursiveMutex(PNRMUTEX mutex) ;

_PCOMNEXP DWORD EnterNonRecursiveMutex(PNRMUTEX mutex, BOOL wait) ;

_PCOMNEXP BOOL LeaveNonRecursiveMutex(PNRMUTEX mutex) ;

#ifdef __cplusplus
}
#endif

#endif /* __PCOMN_NRTHREADMUTEX_H */
