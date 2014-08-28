/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_thread.cc
 COPYRIGHT    :   Yakov Markovitch, 1997-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Posix Threads platform-specific thread functionality.

 CREATION DATE:   9 Feb 2008
*******************************************************************************/
#ifndef PCOMN_PL_UNIX
#error This file contains UNIX-specific part of PCommon threads, but the platform is not UNIX.
#endif

#include <pcomn_sys.h>

#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <errno.h>
#include <signal.h>

namespace pcomn {

const int SIGRESUMETHREAD = (SIGRTMIN + SIGRTMAX)/2 ;

/*******************************************************************************
 pcomn::thread_id
*******************************************************************************/
static sigset_t resume_signal ;

static thread_id init_threads()
{
   // Block SIGRESUMETHREAD for the whole process
   sigemptyset(&resume_signal) ;
   sigaddset(&resume_signal, SIGRESUMETHREAD) ;
   if (sigprocmask(SIG_BLOCK, &resume_signal, NULL) < 0)
   {
      perror("Error initializing PCOMMON threads library: cannot block SIGRESUMETHREAD") ;
      abort() ;
   }
   // Return the ID of the main thread
   return thread_id() ;
}

static thread_id main_thread_id = init_threads() ;

thread_id::thread_id() :
   _id(pthread_self())
{}

thread_id thread_id::mainThread()
{
   // Take care of that this method can be called before main()
   if (!main_thread_id)
      main_thread_id = thread_id() ;
   return main_thread_id ;
}

/*******************************************************************************
 BasicThread
*******************************************************************************/
bool BasicThread::is_current() const
{
   return handle() == pthread_self() ;
}

void BasicThread::create_thread()
{
   NOXCHECK(!is_created()) ;
   #ifndef _POSIX_THREAD_ATTR_STACKSIZE
   _stack_size = 0 ;
   #endif

   if (!_stack_size)
      PCOMN_ENSURE_ENOERR(pthread_create(&_handle, NULL, pcomn_thread_proc, this), "pthread_create") ;
   else
   {
      const size_t psize = pcomn::sys::pagesize() ;
      pthread_attr_t thread_attr ;
      PCOMN_ENSURE_ENOERR(pthread_attr_init(&thread_attr), "pthread_attr_init") ;
      // Round the requested stack size up to a whole number of pages
      PCOMN_ENSURE_ENOERR(pthread_attr_setstacksize(&thread_attr, (_stack_size + psize - 1)/psize*psize),
                          "pthread_attr_init") ;

      PCOMN_ENSURE_ENOERR(pthread_create(&_handle, &thread_attr, pcomn_thread_proc, this), "pthread_create") ;
   }
   _id = _handle ;
}

bool BasicThread::close_thread()
{
   pthread_t thread = handle() ;
   if (!thread)
      return true ;
   _handle = 0 ;
   const int result = pthread_detach(thread) ;
   WARNPX(PCOMN_Threads, result != 0, DBGL_ALWAYS,
          "Error detaching thread " << *this << " handle=" << HEXOUT(thread) << ": " << system_error(result).what()) ;
   return result == 0 ;
}

bool BasicThread::maybe_suspend()
{
   sigset_t every_signal ;
   sigset_t current_set ;
   int received = 0 ;

   sigfillset(&every_signal) ;

   // Block all until we're resumed
   PCOMN_ENSURE_ENOERR(pthread_sigmask(SIG_BLOCK, &every_signal, &current_set), "pthread_sigmask") ;

   sigwait(&resume_signal, &received) ;

   NOXCHECK(received == SIGRESUMETHREAD) ;
   // Restore the signal mask.
   PCOMN_ENSURE_ENOERR(pthread_sigmask(SIG_SETMASK, &current_set, NULL), "pthread_sigmask") ;
   return true ;
}

bool BasicThread::suspend_self()
{
   NOXCHECK(is_current()) ;
   return maybe_suspend() ;
}

bool BasicThread::resume_thread()
{
   if (int result = pthread_kill(handle(), SIGRESUMETHREAD))
   {
      errno = result ;
      return false ;
   }
   return true ;
}

bool BasicThread::join_thread()
{
   NOXCHECK(!is_current()) ;
   NOXCHECK(is_created()) ;
   void *retval = NULL ;
   const int result = pthread_join(handle(), &retval) ;
   _retval = reinterpret_cast<intptr_t>(retval) ;
   if (!result)
      return true ;
   errno = result ;
   return false ;
}

bool BasicThread::set_priority(int /*new_priority*/)
{
   // TODO: not implemented (and may well never be implemented: only superuser can change thread priority)
   return true ;
}

int BasicThread::get_priority() const
{
   // TODO: not implemented (and may well never be implemented: only superuser can change thread priority)
   NOXCHECK(is_created()) ;
   return _priority ;
}

void BasicThread::yield(unsigned long milliseconds)
{
   if (!milliseconds)
#ifdef __USE_GNU
      pthread_yield() ;
#else
      shed_yield() ;
#endif
   else
      usleep((useconds_t)milliseconds * 1000) ;
}

} // end of namespace pcomn
