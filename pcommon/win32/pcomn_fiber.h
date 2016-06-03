#ifndef __PCOMN_FIBER_H
#define __PCOMN_FIBER_H
/*******************************************************************************
 FILE         :   pcomn_fiber.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Fiber (microthread) classes

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   25 Nov 2000
*******************************************************************************/
#include <pcomn_thread.h>
#include <pcomn_smartptr.h>
#include <pcomn_except.h>

namespace pcomn {

/*******************************************************************************
                     class FiberPointer
*******************************************************************************/
class FiberPointer {

   public:
      static void switch_to(const FiberPointer &another_fiber)
      {
         NOXPRECONDITION(!another_fiber.is_running()) ;
         SwitchToFiber(another_fiber.fiber()) ;
      }

      static FiberPointer current() { return FiberPointer() ; }

      void activate() { switch_to(*this) ; }

      static void *data() { return GetFiberData() ; }

      // is_running() -  check whether this fiber is a currently running one
      bool is_running() const { return current() == *this ; }

      friend bool operator==(const FiberPointer &lhs, const FiberPointer &rhs)
      {
         return lhs.fiber() == rhs.fiber() ;
      }

      friend std::ostream &operator<<(std::ostream &os, const FiberPointer &fiber)
      {
         return os << fiber.fiber() ;
      }

   protected:
      FiberPointer(void *fiber)
      {
         set_fiber_ptr(fiber) ;
      }

      void *fiber() const { return _fiber ; }

      void remove()
      {
         WARNPX(PCOMN_Fibers, is_running(), DBGL_ALWAYS, "Deleting the current fiber " << *this) ;
         DeleteFiber(fiber()) ;
      }

      void set_fiber_ptr(void *fiber)
      {
         _fiber = fiber ;
         if (!_fiber)
            throw pcomn::system_error(pcomn::system_error::platform_specific) ;
      }

      void *release()
      {
         void *value = _fiber ;
         _fiber = NULL ;
         return value ;
      }

   private:
      FiberPointer() :
         _fiber(GetCurrentFiber())
      {}

      void * _fiber ;
} ;

inline bool operator!=(const FiberPointer &lhs, const FiberPointer &rhs)
{
   return !(lhs == rhs) ;
}

/*******************************************************************************
                     class Fiber
*******************************************************************************/
class Fiber : public virtual PRefCount, public virtual Runnable, protected FiberPointer {
      typedef FiberPointer ancestor ;
   public:
      using ancestor::activate ;
      using ancestor::is_running ;

      virtual ~Fiber()
      {
         TRACEPX(PCOMN_Fibers, DBGL_ALWAYS, "Deleting the fiber " << *this) ;
         remove() ;
      }

      void yield()
      {
         TRACEPX(PCOMN_Fibers, DBGL_LOWLEV, "Fiber " << *this << " yields to " << _scheduler) ;
         if (!_scheduler)
            throw std::logic_error("Cannot yield fiber which have never been scheduled") ;
         _scheduler->activate() ;
      }

      void schedule(Fiber &fiber)
      {
         NOXPRECONDITION(fiber != *this) ;
         TRACEPX(PCOMN_Fibers, DBGL_LOWLEV, "Scheduling:" << fiber << " Scheduler:" << *this) ;

         fiber._scheduler = this ;
         fiber.activate() ;
      }

      bool is_completed() const { return _completed ; }

      static Fiber *current_fiber()
      {
         Fiber *result = static_cast<Fiber *> (ancestor::data()) ;
         NOXCHECK(result->fiber() == GetCurrentFiber()) ;
         return result ;
      }

      friend std::ostream &operator<<(std::ostream &os, const Fiber &fiber_object)
      {
         return os << '<' << fiber_object.fiber() << ',' << (void *)fiber_object._scheduler << '>' ;
      }

   protected:
      explicit Fiber(unsigned stacksize = 0, void *data = NULL) :
         ancestor(CreateFiber(stacksize, _pcomn_fiber_proc, this)),
         _scheduler(NULL),
         _retval(0),
         _completed(false)
      {
         TRACEPX(PCOMN_Fibers, DBGL_ALWAYS, "Fiber " << *this << " has been created") ;
      }

      explicit Fiber(BasicThread &, void *data = NULL) :
         ancestor(this),
         _scheduler(NULL),
         _retval(0),
         _completed(false)
      {}

      bool is_initialized() const { return fiber() != (void *)this ; }

      void set_fiber(BasicThread &thread)
      {
         NOXPRECONDITION(thread.is_running()) ;
         NOXCHECK(!is_initialized()) ;

         set_fiber_ptr(ConvertThreadToFiber(this)) ;

         TRACEPX(PCOMN_Fibers, DBGL_ALWAYS, "Thread " << thread << " -> fiber " << *this) ;
      }

   private:
      Fiber *       _scheduler ;
      void *         _data ;
      int            _retval ;
      bool           _completed ;

      PCOMN_NONCOPYABLE(Fiber) ;
      PCOMN_NONASSIGNABLE(Fiber) ;

      friend void CALLBACK _pcomn_fiber_proc(void *context)
      {
         Fiber *fiber = static_cast<Fiber *>(context) ;

         TRACEPX(PCOMN_Fibers, DBGL_ALWAYS, "Fiber " << *fiber << " started") ;
         fiber->_retval = fiber->exec() ;
         TRACEPX(PCOMN_Fibers, DBGL_ALWAYS, "Fiber " << *fiber << " finished. result=" << fiber->_retval) ;

         fiber->_completed = true ;
         if (fiber->_scheduler)
         {
            TRACEPX(PCOMN_Fibers, DBGL_ALWAYS, "Fiber " << *fiber << " has a scheduler. Yelding...") ;
            fiber->yield() ;
            PCOMN_DEBUG_FAIL("We must never get here!") ;
         }
      }
} ;

typedef shared_intrusive_ptr<Fiber> FiberP ;

/*******************************************************************************
                     class PTFiberThread
*******************************************************************************/
template<class Thread>
class PTFiberThread : public Thread, protected Fiber {

   protected:
      PTFiberThread(unsigned flags = 0, int stack_size = 0, BasicThread::Priority pty = PtyNormal) :
         Thread(false, stack_size, pty),
         Fiber(*static_cast<Thread *>(this))
      {
         start(StartSuspended) ;
      }

      int exec()
      {
         set_fiber(*this) ;
         return Thread::exec() ;
      }
} ;

typedef PTFiberThread<BasicThread> FiberThread ;

} // end of namespace pcomn

#endif /* __PCOMN_FIBER_H */
