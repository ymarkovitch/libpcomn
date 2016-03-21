/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_COUNTER_H
#define __PCOMN_COUNTER_H
/*******************************************************************************
 FILE         :   pcomn_counter.h
 COPYRIGHT    :   Yakov Markovitch, 1999-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Counter classes/templates

 CREATION DATE:   7 Oct 1999
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcomn_atomic.h>

namespace pcomn {

/*******************************************************************************
                     class PTActiveCounter
*******************************************************************************/
template<atomic_t init = 0, typename ThreadPolicy = atomic_op::MultiThreaded>
class PTActiveCounter {
   public:
      typedef ThreadPolicy thread_policy ;

      PTActiveCounter(atomic_t initval = init) :
         _counter(initval)
      {}

      /// Do-nothing destructor, just for the sake of polymorphism.
      virtual ~PTActiveCounter() {}

      atomic_t count() const { return _counter ; }

      atomic_t inc_passive() { return thread_policy::inc(_counter) ; }

      atomic_t dec_passive() { return thread_policy::dec(_counter) ; }

      atomic_t reset(atomic_t new_value) { return thread_policy::xchg(_counter, new_value) ; }

      /// Increment internal counter and call threshold action.
      /// Increment internal counter and, if it equal to threshold value (parameter),
      /// call inc_action.
      /// @param  threshold Threshold value to call threshold action.
      /// @return Threshold value given as @a threshold.
      atomic_t inc(atomic_t threshold = 0)
      {
         atomic_t result = inc_passive() ;
         return result == threshold ?
            inc_action(threshold) : result ;
      }

      /// Decrement internal counter and call threshold action.
      /// Decrements internal counter and, if it equal to threshold value (parameter),
      /// calls dec_action().
      /// @param threshold Threshold value to call threshold action.
      /// @return @a threshold parameter value.
      atomic_t dec(atomic_t threshold = 0)
      {
         atomic_t result = dec_passive() ;
         return result == threshold ?
            dec_action(threshold) : result ;
      }

   protected:
      virtual atomic_t inc_action(atomic_t threshold) = 0 ;
      virtual atomic_t dec_action(atomic_t threshold) = 0 ;

   private:
      atomic_t _counter ;
} ;

/******************************************************************************/
/** Automatic scope decrementor
*******************************************************************************/
template<typename Atomic, typename ThreadPolicy = atomic_op::MultiThreaded>
struct auto_decrementer {
      explicit auto_decrementer(Atomic &counter) : _counter(counter) {}
      ~auto_decrementer() { ThreadPolicy::dec(_counter) ; }
   private:
      Atomic &_counter ;
} ;

/******************************************************************************/
/** Unique instance ID
*******************************************************************************/
template<typename InstanceType, typename Atomic = uatomic64_t>
struct instance_id {
      PCOMN_STATIC_CHECK(is_atomic<Atomic>::value) ;

      typedef Atomic       type ;
      typedef InstanceType instance_type ;
      friend instance_type ;

      operator uatomic64_t() const { return _value ; }

   private:
      type _value ;

      static type _counter ;

      instance_id() : _value(atomic_op::inc(&_counter)) {}
} ;

template<typename InstanceType, typename Atomic>
typename instance_id<InstanceType, Atomic>::type instance_id<InstanceType, Atomic>::_counter ;

} // end of namespace pcomn

#endif /* __PCOMN_COUNTER_H */
