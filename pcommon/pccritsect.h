/*-*- mode: c; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((inclass . ++)) -*-*/
#ifndef __PCCRITSECT_H
#define __PCCRITSECT_H
/*******************************************************************************
 FILE         :   pccritsect.h
 COPYRIGHT    :   Yakov Markovitch, 1996-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Conditional critical section support for procedural C code.

 CREATION DATE:   18 Dec 1996
*******************************************************************************/
#include <pcomn_platform.h>

#if !defined(PCOMN_MD_MT)

#  define PCOMN_CRITICAL_SECTION(name) int name
#  define INIT_CRITICAL_SECTION(name) ((void)0)
#  define DEL_CRITICAL_SECTION(name) ((void)0)
#  define ENTER_CRITICAL_SECTION(name) ((void)0)
#  define LEAVE_CRITICAL_SECTION(name) ((void)0)

#elif defined(PCOMN_PL_WINDOWS)
#  include <windows.h>

#  define PCOMN_CRITICAL_SECTION(name) CRITICAL_SECTION name
#  define INIT_CRITICAL_SECTION(name) (InitializeCriticalSection (&name))
#  define DEL_CRITICAL_SECTION(name) (DeleteCriticalSection (&name))
#  define ENTER_CRITICAL_SECTION(name) (EnterCriticalSection (&name))
#  define LEAVE_CRITICAL_SECTION(name) (LeaveCriticalSection (&name))

#else
#include <pthread.h>

#  define PCOMN_CRITICAL_SECTION(name) pthread_mutex_t name
#  define INIT_CRITICAL_SECTION(name) {const pthread_mutex_t __##name = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP ; name = __##name ;}
#  define DEL_CRITICAL_SECTION(name) (pthread_mutex_destroy(&name))
#  define ENTER_CRITICAL_SECTION(name) (pthread_mutex_lock(&name))
#  define LEAVE_CRITICAL_SECTION(name) (pthread_mutex_unlock(&name))

#endif


#endif /* __PCCRITSECT_H */
