/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_COUNTER_H
#define __PCOMN_COUNTER_H
/*******************************************************************************
 FILE         :   pcomn_counter.h
 COPYRIGHT    :   Yakov Markovitch, 1999-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Counter classes/templates

 CREATION DATE:   7 Oct 1999
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcomn_atomic.h>
#include <pcomn_assert.h>

#include <utility>

namespace pcomn {

template<typename C>
struct active_counter_base {
      typedef C count_type ;

      constexpr active_counter_base(count_type init = count_type()) noexcept : _counter(init) {}

      constexpr count_type count() const noexcept { return _counter ; }

      count_type reset(count_type new_value = count_type()) noexcept
      {
         count_type old = std::move(_counter) ;
         _counter = std::move(new_value) ;
         return std::move(old) ;
      }

      count_type inc_passive() noexcept { return ++_counter ; }
      count_type dec_passive() noexcept { return --_counter ; }

   private:
      count_type _counter ;
} ;

template<typename T>
struct active_counter_base<std::atomic<T>> {
      typedef T count_type ;

      constexpr active_counter_base(count_type init = count_type()) noexcept : _counter(init) {}

      count_type count() const noexcept { return _counter.load(std::memory_order_consume) ; }

      count_type reset(count_type new_value = count_type()) noexcept
      {
         return _counter.exchange(new_value, std::memory_order_acq_rel) ;
      }

      count_type inc_passive() noexcept { return _counter.fetch_add(1, std::memory_order_acq_rel) + 1 ; }
      count_type dec_passive() noexcept { return _counter.fetch_sub(1, std::memory_order_acq_rel) - 1 ; }

   private:
      std::atomic<count_type> _counter ;
} ;

/******************************************************************************/
/** Counter, possibly atomic, which automatically calls an overloadable action
 when its value becomes equal to a specified threshold as a result of increment
 or/and decrement.
*******************************************************************************/
template<typename C = std::atomic<int>, typename active_counter_base<C>::count_type init = 0>
class active_counter : public active_counter_base<C> {
      typedef active_counter_base<C> ancestor ;
   public:
      using typename ancestor::count_type ;

      active_counter(count_type initval = init) noexcept : ancestor(initval) {}

      /// Do-nothing destructor, just for the sake of polymorphism.
      virtual ~active_counter() = default ;

      /// Increment internal counter and call threshold action.
      /// Increment internal counter and, if it equal to threshold value (parameter),
      /// call inc_action.
      /// @param  threshold Threshold value to call threshold action.
      /// @return Threshold value given as @a threshold.
      count_type inc(count_type threshold = 0) noexcept
      {
         const count_type result = this->inc_passive() ;
         return result == threshold ?
            inc_action(threshold) : result ;
      }

      /// Decrement internal counter and call threshold action.
      /// Decrements internal counter and, if it equal to threshold value (parameter),
      /// calls dec_action().
      /// @param threshold Threshold value to call threshold action.
      /// @return @a threshold parameter value.
      count_type dec(count_type threshold = 0) noexcept
      {
         const count_type result = this->dec_passive() ;
         return result == threshold ?
            dec_action(threshold) : result ;
      }

   protected:
      virtual count_type inc_action(count_type threshold) noexcept = 0 ;
      virtual count_type dec_action(count_type threshold) noexcept = 0 ;
} ;

/******************************************************************************/
/** Unique instance ID
*******************************************************************************/
template<typename InstanceType, typename Atomic = uintptr_t>
struct instance_id {
      PCOMN_STATIC_CHECK(is_atomic_arithmetic<Atomic>::value) ;

      typedef Atomic       type ;
      typedef InstanceType instance_type ;
      friend instance_type ;

      operator type() const { return _value ; }

   private:
      type _value ;

      static type _counter ;

      instance_id() : _value(atomic_op::preinc(&_counter)) {}
} ;

template<typename InstanceType, typename Atomic>
alignas(64) typename instance_id<InstanceType, Atomic>::type instance_id<InstanceType, Atomic>::_counter ;

} // end of namespace pcomn

#endif /* __PCOMN_COUNTER_H */
