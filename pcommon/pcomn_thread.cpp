/*-*- tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets: ((innamespace . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_thread.cpp
 COPYRIGHT    :   Yakov Markovitch, 1997-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Platform-independent part of BasicThread implementation.

 CREATION DATE:   29 Jan 1997
*******************************************************************************/
#include <pcomn_thread.h>
#include <pcomn_trace.h>
#include <pcomn_diag.h>
#include <pcomn_except.h>

namespace pcomn {

/*******************************************************************************
 BasicThread
*******************************************************************************/
typedef std::lock_guard<std::recursive_mutex> Guard ;

BasicThread::BasicThread(JoinMode jmode, size_t stack_size, Priority priority) :
   _state(SNew),
   _join_mode(jmode),
   _stop(false),
   _priority(priority),

   _handle(0),
   _id(0),
   _stack_size(stack_size),
   _retval(0)
{}

BasicThread::BasicThread(const BasicThread &source) :
   Runnable(),
   _state(SNew),
   _join_mode(source._join_mode),
   _stop(false),
   _priority(source._priority),

   _handle(0),
   _id(0),
   _stack_size(source._stack_size),
   _retval(0)
{}

BasicThread::~BasicThread()
{
   destroy_thread(false) ;
}

void BasicThread::destroy_thread(bool derived_class) throw()
{
   if (state() == SNew)
      // The thread object has no corresponding OS thread, nothing to do
      return ;

   TRACEPX(PCOMN_Threads, DBGL_ALWAYS, "Destroying thread " << *this) ;

   _stop = true ;
   // If we have already created the real thread but had no time to really start it,
   // get it to quick finish
   if (state() == SCreated)
   {
      resume() ;
      join() ;
   }
   else if (!is_completed())
   {
      NOXCHECK(_join_mode != JoinDetached) ;
      // Abort on attempt to destroy still running thread
      PCOMN_ENSURE(derived_class && _join_mode == JoinAuto,
                   "Attempt to destroy a manually joinable thread, which is not joined") ;
      join() ;
   }

   // Close underlying native thread
   close_thread() ;

   _state = SNew ;
}

void *pcomn_thread_proc(void *context)
{
   BasicThread::maybe_suspend() ;

   BasicThread *thread_object = static_cast<BasicThread *>(context) ;
   NOXCHECK(thread_object) ;

   void *result = NULL ;

   if (!thread_object->_stop)
   {
      TRACEPX(PCOMN_Threads, DBGL_ALWAYS, "Thread " << *thread_object << " just has started") ;

      thread_object->_state = BasicThread::SRunning ; // Don't forget to change the state!
      thread_object->_retval = thread_object->exec() ;

      TRACEPX(PCOMN_Threads, DBGL_LOWLEV, *thread_object << " finished execution") ;

      result = (void *)thread_object->_retval ;
   }

   if (thread_object->_join_mode == BasicThread::JoinDetached)
   {
      thread_object->_state = BasicThread::SAutoDestroyed ;
      delete thread_object ;
   }
   else
      thread_object->_state = BasicThread::SCompleted ;

   return result ;
}

void BasicThread::create()
{
   NOXCHECK(!is_created()) ;

   TRACEPX(PCOMN_Threads, DBGL_ALWAYS, "Creating new thread. Thread object address is " << this) ;

   create_thread() ;
   _state = SCreated ;

   TRACEPX(PCOMN_Threads, DBGL_ALWAYS, "Object thread " << *this << " has been succesfully created.") ;
}

void BasicThread::start(StartMode mode)
{
   TRACEPX(PCOMN_Threads, DBGL_ALWAYS, "Starting thread " << *this << (mode == StartRunning ? "" : " in suspended state")) ;

   PCOMN_THROW_MSG_IF(state() > SCreated, std::logic_error, "Attempt to start an already started thread.") ;

   if (!is_created())
      create() ;
   if (mode == StartRunning)
      resume() ;
}

bool BasicThread::suspend()
{
   PCOMN_THROW_MSG_IF(is_current(),
                      std::logic_error, "Cannot suspend other thread: a thread is only allowed to suspend itself.") ;
   if (!suspend_self())
      throw pcomn::system_error(pcomn::system_error::platform_specific) ;
   return true ;
}

void BasicThread::resume()
{
   PCOMN_THROW_MSG_IF(!is_created(),
                      std::logic_error, "Attempt to resume a thread object while the real thread is not yet created") ;
   // Unlike the suspend(), calling resume() after thread completed IS an error
   PCOMN_THROW_MSG_IF(is_completed(), std::logic_error, "Attempt to resume an already completed thread.") ;

   const State oldstate = state() ;

   if (oldstate == SCreated)
      _state = SStarting ;

   if (!resume_thread())
   {
      if (oldstate == SCreated)
         _state = SCreated ;
      throw pcomn::system_error(pcomn::system_error::platform_specific) ;
   }
}

intptr_t BasicThread::join()
{
   PCOMN_THROW_MSG_IF(!is_created(), std::logic_error, "Thread is not created yet") ;
   // Check for deadlock. It is obvious that <thread_object>.join() being
   // called from inside the thread of the <thread_object> never returns.
   PCOMN_THROW_MSG_IF(is_current(), std::logic_error, "Deadlock condition: attempt to join to itself.") ;
   if (!join_thread())
      throw pcomn::system_error(pcomn::system_error::platform_specific) ;
   _handle = 0 ;
   return _retval ;
}

void BasicThread::priority(Priority new_priority)
{
   // It is perfectly legal to call priority() for a thread not created yet or
   // for an already completed one. In the former case we just set _priority member
   // so it can be used later during thread starting; while in the latter case we just
   // ignore the call.
   // The only possible cause of exception throw can be some error on setting priority for
   // a running thread.
   Guard guard (get_lock()) ;

   if (!is_created())
      _priority = new_priority ;
   else if (!is_completed())
      if (set_priority(new_priority))
         _priority = new_priority ;
      else
         throw pcomn::system_error(pcomn::system_error::platform_specific) ;
}

std::ostream &BasicThread::debug_print(std::ostream &os) const
{
   os << this << ' ' << id() << (handle() ? ":alive " : ":closed ") ;
   #define STATE_CASE(s) case S##s: return os << #s
   switch (state())
   {
      STATE_CASE(New) ;
      STATE_CASE(Created) ;
      STATE_CASE(Starting) ;
      STATE_CASE(Running) ;
      STATE_CASE(Completed) ;
      STATE_CASE(AutoDestroyed) ;
   }
   #undef STATE_CASE
   return os << "UNKNOWN(" << state() << ')' ;
}

} // end of namespace pcomn

#include PCOMN_PLATFORM_HEADER(pcomn_thread.cc)
