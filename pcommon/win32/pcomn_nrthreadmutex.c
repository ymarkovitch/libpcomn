/*-*- tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets: ((inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_nrthreadmutex.c
 COPYRIGHT    :   Yakov Markovitch, 2000-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Fast non-recursive mutex. Has the same logic as Event. I.e. from the one hand,
                  thread can block itself trying to acquire NRMutex it already held even by itself,
                  but on the other hand acquire and release on one lock can be performed by
                  different threads (which is impossible for, say, CRITICAL_SECTION).

 PROGRAMMED BY:   Yakov Markovitch

 CREATION DATE:   20 Feb 2000
*******************************************************************************/
#include <win32/pcomn_nrthreadmutex.h>

BOOL InitializeNonRecursiveMutex(PNRMUTEX mutex)
{
   mutex->owned = -1 ;  /* No threads have entered NonRecursiveMutex */
   mutex->thread_id = 0 ;
   mutex->hevent = CreateEventA(NULL, FALSE, FALSE, NULL) ;
   return mutex->hevent != NULL ;  /* TRUE if the mutex is created */
}

#define InterlockedCompareExchange(dest,exchange,comperand) (ixchg((dest), (exchange), (comperand)))

VOID DeleteNonRecursiveMutex(PNRMUTEX mutex)
{
   /* No in-use check */
   CloseHandle(mutex->hevent) ;
   mutex->hevent = NULL ; /* Just in case */
}

DWORD EnterNonRecursiveMutex(PNRMUTEX mutex, BOOL wait)
{
   /* Assume that the thread waits successfully */
   DWORD ret ;

   if (!wait)
   {
      if (InterlockedCompareExchange((PVOID *)&mutex->owned, (PVOID)0, (PVOID)-1) != (PVOID)-1)
         return WAIT_TIMEOUT ;
      ret = WAIT_OBJECT_0 ;
   }
   else
      /* InterlockedIncrement(&mutex->owned) == 0 means that no thread currently owns the mutex */
      ret = InterlockedIncrement(&mutex->owned) ?
         /* Someone already acquired the mutex, let's wait... */
         WaitForSingleObject(mutex->hevent, INFINITE) : WAIT_OBJECT_0 ;

   mutex->thread_id = GetCurrentThreadId() ; /* We acquired it (in fact, we never use this value,
                                              * but it is very covenient for debug and incurs zero overhead) */
   return ret ;
}

BOOL LeaveNonRecursiveMutex(PNRMUTEX mutex)
{
   /* We don't own the mutex */
   mutex->thread_id = 0 ;
   return
      InterlockedDecrement(&mutex->owned) < 0 ||
      SetEvent(mutex->hevent) ; /* Other threads are waiting, wake one on them up */
}
