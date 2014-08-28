/*-*- tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets: ((innamespace . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_thread.cpp
 COPYRIGHT    :   Yakov Markovitch, 1997-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Win32 platform-specific thread functionality.

 CREATION DATE:   9 Feb 2008
*******************************************************************************/
#ifndef PCOMN_PL_WINDOWS
#error This file contains Windows-specific part of PCommon threads, but the platform is not Windows.
#endif

#include <process.h>

namespace pcomn {

/*******************************************************************************
 pcomn::thread_id
*******************************************************************************/
static thread_id _main ;

thread_id::thread_id() :
   _id(GetCurrentThreadId())
{}

thread_id thread_id::mainThread()
{
   if (!_main)
      _main = thread_id() ;
   return _main ;
}

/*******************************************************************************
 BasicThread
*******************************************************************************/
typedef std::lock_guard<std::recursive_mutex> PGuard ;

static unsigned __stdcall _pcomn_thread_proc(void *) ;

void BasicThread::create_thread()
{
   NOXCHECK(!is_created()) ;

   _handle = (thread_handle_t)_beginthreadex(NULL, _stack_size, _pcomn_thread_proc, this, CREATE_SUSPENDED, &_id) ;
   if (!_handle)
      throw pcomn::system_error(pcomn::system_error::platform_specific) ;

   if (_priority != PtyNormal)
      // Maybe we shouldn't ignore errors at this point, but for the moment
      // it's too complex to process, so I decided not to fill my head with it. Later.
      if (!_set_priority(_priority))
         _priority = PtyNormal ;
}

bool BasicThread::close_thread()
{
   NOXCHECK(is_created()) ;
   return !!CloseHandle(handle()) ;
}

bool BasicThread::suspend_self()
{
   return (int)SuspendThread(handle()) >= 0 ;
}

bool BasicThread::resume_thread()
{
   return (int)ResumeThread(handle()) >= 0 ;
}

bool BasicThread::join_thread()
{
   return WaitForSingleObject(handle(), INFINITE) == WAIT_OBJECT_0 ;
}

bool BasicThread::set_priority(int new_priority)
{
   return SetThreadPriority(handle(), new_priority) != 0 ;
}

int BasicThread::get_priority() const
{
   NOXCHECK(is_created()) ;
   int result = GetThreadPriority(handle()) ;
   if (result != THREAD_PRIORITY_ERROR_RETURN)
      return result ;
   return _priority ;
}

void BasicThread::yield(unsigned long milliseconds)
{
   Sleep(milliseconds) ;
}

bool BasicThread::maybe_suspend()
{
   return true ;
}

static unsigned __stdcall _pcomn_thread_proc(void *context)
{
   return (unsigned)pcomn_thread_proc(context) ;
}

/*******************************************************************************
 PThreadSuspender
*******************************************************************************/
PThreadSuspender::~PThreadSuspender()
{
   if (_handle)
      CloseHandle(_handle) ;
}

void *PThreadSuspender::identity()
{
   // Get the "real" thread handle
   if (!_identity)
   {
      HANDLE current_thread = GetCurrentThread() ;
      HANDLE current_process = GetCurrentProcess() ;
      HANDLE real_thread = 0 ;
      if (!DuplicateHandle(current_process, current_thread, current_process, &real_thread,
                           0, FALSE, DUPLICATE_SAME_ACCESS))
         throw pcomn::system_error(pcomn::system_error::platform_specific) ;
      _identity = GetCurrentThreadId() ;
      _handle = real_thread ;
   }
   return _handle ;
}

void PThreadSuspender::suspend()
{
   HANDLE current_thread = identity() ;
   if (_identity != GetCurrentThreadId())
      throw std::logic_error("PThreadSuspender: attempt to suspend non-current thread") ;

   if ((long)SuspendThread(current_thread) < 0)
      throw pcomn::system_error(pcomn::system_error::platform_specific) ;
}

bool PThreadSuspender::resume(void *)
{
   if (_handle)
   {
      long result = ResumeThread(_handle) ;
      if (result < 0)
         throw pcomn::system_error(pcomn::system_error::platform_specific) ;
      return result != 0 ;
   }
   return false ;
}

} // end of namespace pcomn
