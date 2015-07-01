/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_THREAD_H
#define __PCOMN_THREAD_H
/*******************************************************************************
 FILE         :   pcomn_thread.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Thread objects.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   17 Nov 2000
*******************************************************************************/
/** @file
    Runnable objects: threads and tasks.
 Implementation exists for Win32/64 threads API and for Posix Threads API.
*******************************************************************************/
#include <pcomn_def.h>
#include <pcomn_syncobj.h>
#include <pcomn_threadid.h>
#include <pcomn_smartptr.h>
#include <pcomn_meta.h>

#include <iostream>

#if defined(PCOMN_PL_UNIX)
#include <pthread.h>
namespace pcomn {
typedef pthread_t thread_handle_t ;
typedef pthread_t thread_id_t ;
}
#elif defined(PCOMN_PL_WINDOWS)
#include <windows.h>
namespace pcomn {
typedef HANDLE    thread_handle_t ;
typedef unsigned  thread_id_t ;
}
#else
#error This platform is not supported by pcomn_thread.
#endif
#include <functional>

namespace pcomn {

/******************************************************************************/
/** The base abstract class for all runnable objects: threads, microthreads (i.e. fibers),
 maybe even processes.

 Can be used to represent any independent piece of control flow.
*******************************************************************************/
class _PCOMNEXP Runnable {
   public:
      virtual ~Runnable() {}

   protected:
      /// Call run().
      /// Could be used for pre- and post- processing.
      virtual int exec() { return run() ; }

      /// Do actual workload.
      /// Intended do do some useful job
      virtual int run() = 0 ;
} ;

/******************************************************************************/
/** The task abstraction, base unit a worker can do.
 @note This class is abstract, derived classes should implement method run.
*******************************************************************************/
class _PCOMNEXP Task : public virtual Runnable, public virtual PRefCount {
   public:
      int accomplish() { return exec() ; }
} ;

/******************************************************************************/
/** Shared pointer to a task object.
*******************************************************************************/
typedef shared_intrusive_ptr<Task> TaskPtr ;

/******************************************************************************/
/** A concrete runnable Task, specified through a functor.
*******************************************************************************/
template<typename Fn>
class Job : public Task {
   public:
      explicit Job(const Fn &functor) :
         _fn(functor)
      {}

   protected:
      int run() { return call_functor((functor_result *)NULL) ; }

   private:
      Fn _fn ;

      typedef typename std::result_of<Fn()>   functor_result ;

      template<typename T>
      std::enable_if_t<std::is_convertible<T, int>::value, int>
      call_functor(T *) { return _fn() ; }

      template<typename T>
      disable_if_t<std::is_convertible<T, int>::value, int>
      call_functor(T *) { _fn() ; return 1 ; }
} ;

/******************************************************************************/
/** A template helper function used to construct objects of type pcomn::Job,
 where job functor type is based on the type passed as a parameter.
 @return Task smart pointer, which can be used as a parameter to pcomn::TaskThread
 constructor.
*******************************************************************************/
template<typename Fn>
inline TaskPtr make_job(const Fn &fn)
{
   return TaskPtr(new Job<Fn>(fn)) ;
}

/******************************************************************************/
/** This class provides a generic and platform-independent interface to threads.
*******************************************************************************/
class _PCOMNEXP BasicThread : public virtual Runnable {
      // Please note, the basic thread _is_ copiable
      PCOMN_NONASSIGNABLE(BasicThread) ;
   public:
      /// How to start the underlying OS thread
      enum StartMode {
         StartRunning,
         StartSuspended
      } ;

      enum JoinMode {
         JoinManually,  /**< The running thread must be explicitly joined and the
                        corresponding thread object must be destructed "from outside" */
         JoinDetached,  /**< The thread object is destroyed automagically by the thread
                         procedure */
         JoinAuto
      } ;

      enum Priority {
         PtyIdle,
         PtyLowest,
         PtyBelowNormal,
         PtyNormal,
         PtyAboveNormal,
         PtyHighest,
         PtyRealTime
      } ;

      virtual ~BasicThread() ;

      /// Start the thread either immediately or in suspended state
      void start(StartMode mode = StartRunning) ;

      bool suspend() ;
      void resume() ;

      /// Wait for thread to terminate normally.
      /// Returns the result of run().
      intptr_t join() ;

      /// Immediately terminate the thread.
      /// @note This call is VERY dangerous!
      void terminate() ;

      /// Get the current priority of the thread.
      int priority() const { return is_created() ? get_priority() : _priority ; }

      /// Set the current priority of the thread.
      void priority(Priority new_priority) ;

      bool is_created() const { return !!id() ; }
      bool is_completed() const { return state() >= SCompleted ; }
      bool is_current() const ;

      thread_handle_t handle() const { return _handle ; }
      thread_id_t id() const { return _id ; }

      virtual std::ostream &debug_print(std::ostream &os) const ;

      static void yield(unsigned long milliseconds = 0) ;

      friend std::ostream &operator<<(std::ostream &os, const BasicThread &object_thread)
      {
         return object_thread.debug_print(os << '<') << '>' ;
      }

   protected:
      enum State {
         SNew,             /**< A thread object is newly constructed, not yet created a real thread */
         SCreated,         /**< A real thread created but haven't started yet*/
         SStarting,
         SRunning,         /**< A thread is running*/
         SCompleted,       /**< A thread has completed*/
         SAutoDestroyed    /**< A thread is being autodestroyed */
      } ;

      explicit BasicThread(JoinMode jmode = JoinManually, size_t stack_size = 0, Priority pty = PtyNormal) ;

      // One cannot copy _running_ thread, but it is clearly possible
      // to copy thread whose "OS thread" is not yet created!
      BasicThread(const BasicThread &source) ;

      State state() const { return _state ; }

      void destroy() { destroy_thread(true) ; }

   private:
      State             _state ;
      const JoinMode    _join_mode ;
      bool              _stop ;
      int               _priority ;

      thread_handle_t   _handle ;
      thread_id_t       _id ;
      const size_t      _stack_size ;
      intptr_t          _retval ;
      std::recursive_mutex   _lock ;

      void create() ;

      // Platform-specific
      void create_thread() ;
      bool close_thread() ;
      bool join_thread() ;
      int  get_priority() const ;
      bool set_priority(int new_priority) ;

      // On Windows, a thread can be started in suspended state, while on Unix cannot.
      // Since we need a thread initally suspended, we call maybe_suspend() immediately
      // at start of thread_proc. On Windows, maybe_suspend() is dummy.
      static bool maybe_suspend() ;
      bool suspend_self() ;
      bool resume_thread() ;
      void destroy_thread(bool derived) throw() ;

      std::recursive_mutex &get_lock() const { return const_cast<std::recursive_mutex &>(_lock) ; }

      friend void *pcomn_thread_proc(void *context) ;
} ;


/******************************************************************************/
/** Concrete thread class, accepts in the constractor a smartpointer to other a "task"
 object and runs (accomplishes) it.

 This class is concrete, as opposite to BasicThread, BasicThread::run member is abstract.

 Using pcomn::Job template class and pcomn::make_job function together with this class
 allows to create "ad-hoc" threads.
*******************************************************************************/
class _PCOMNEXP TaskThread : public BasicThread {
      typedef BasicThread ancestor ;
   public:
      /// Create new thread object and specify the task the new thread should perform;
      /// does @em not create/start an actual OS thread.
      ///
      /// This constructor creates a thread object, but doesn't start or even create an
      /// "actual" underlying OS thread. To create/start an actual thread one should call
      /// start().
      TaskThread(const TaskPtr &task, JoinMode mode = JoinManually, size_t stack_size = 0, Priority pty = PtyNormal) :
         ancestor(mode, stack_size, pty),
         _task(PCOMN_ENSURE_ARG(task))
      {}

      ~TaskThread() { destroy() ; }

   protected:
      int run() { return _task->accomplish() ; }
   private:
      TaskPtr _task ;
} ;

} // end of namespace pcomn

#endif /* __PCOMN_THREAD_H */
