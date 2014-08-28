/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_ATOMIC_CC
#define __PCOMN_ATOMIC_CC
/*******************************************************************************
 FILE         :   pcomn_atomic.cc
 COPYRIGHT    :   Yakov Markovitch, 2007-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Atomic operations support for GNU C platform

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   18 May 2007
*******************************************************************************/
#ifndef PCOMN_COMPILER_GNU
#  error This file provides atomic operations only for GNU C/C++. Current platform is PCOMN_COMPILER.
#endif

#include <cstddef>


namespace pcomn {
namespace atomic_op {

template<typename T, bool is_integer> struct implementor ;

template<typename T>
struct implementor<T, true> {
      static T inc(T *value) { return __sync_add_and_fetch(value, 1) ; }
      static T dec(T *value) { return __sync_sub_and_fetch(value, 1) ; }
      static T add(T *value, T addend) { return __sync_add_and_fetch(value, addend) ; }
      static T sub(T *value, T subtrahend) { return __sync_sub_and_fetch(value, subtrahend) ; }

      static T get(T *value) { return __sync_add_and_fetch(value, 0) ; }

      static T xchg(T *value, T new_value)
      {
         return __sync_lock_test_and_set(value, new_value) ;
      }
      static bool cas(T *value, T old_value, T new_value)
      {
         return __sync_bool_compare_and_swap(value, old_value, new_value) ;
      }
} ;

template<typename T>
struct implementor<T *, false> : implementor<T *, true> {
      typedef T *value_type ;

      static value_type inc(value_type *value)
      {
         return __sync_add_and_fetch(value, sizeof(T)) ;
      }
      static value_type dec(value_type *value)
      {
         return __sync_sub_and_fetch(value, sizeof(T)) ;
      }
      static value_type add(value_type *value, ptrdiff_t addend)
      {
         return __sync_add_and_fetch(value, addend * sizeof(T)) ;
      }
      static value_type sub(value_type *value, ptrdiff_t subtrahend)
      {
         return __sync_sub_and_fetch(value, subtrahend * sizeof(T)) ;
      }
} ;

template<> struct implementor<void *, false> : implementor<void *, true> {} ;

} // end of namespace pcomn::atomic_op

} // end of namespace pcomn

#endif /* __PCOMN_ATOMIC_CC */
