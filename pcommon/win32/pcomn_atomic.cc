/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_ATOMIC_CC
#define __PCOMN_ATOMIC_CC
/*******************************************************************************
 FILE         :   pcomn_atomic.cc
 COPYRIGHT    :   Yakov Markovitch, 2000-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Atomic operations support for Win32

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   22 Nov 2000
*******************************************************************************/
#include <pcomn_def.h>
#include <atomic>

namespace pcomn {
namespace atomic_op {

template<typename T, bool>
struct implementor {
      static T inc(T *value) { return ++reinterpret(value) ; }
      static T dec(T *value) { return --reinterpret(value) ; }
      template<typename U>
      static T add(T *value, U addend) { return reinterpret(value) += addend ; }
      template<typename U>
      static T sub(T *value, U subtrahend) { return reinterpret(value) -= subtrahend ; ; }

      static T get(T *value) { return reinterpret(value).fetch_add(0) ; }

      static T xchg(T *value, T new_value) { return reinterpret(value).exchange(new_value) ; }
      static bool cas(T *value, T expected_value, T new_value)
      {
         return reinterpret(value).compare_exchange_strong(expected_value, new_value) ;
      }

   private:
      static std::atomic<T> &reinterpret(T *p) { return *reinterpret_cast<std::atomic<T> *>(p) ; }
      PCOMN_STATIC_CHECK(sizeof(T) == sizeof(std::atomic<T>)) ;
} ;

template<bool b>
struct implementor<void *, b> {
   private:
      typedef implementor<char *, false> ancestor ;
      static char **reinterpret(void **p) { return reinterpret_cast<char **>(p) ; }
   public:
      static void *inc(void **value) { return ancestor::inc(reinterpret(value)) ; }
      static void *dec(void **value) { return ancestor::dec(reinterpret(value)) ; }
      static void *add(void **value, ptrdiff_t addend) { return ancestor::add(reinterpret(value), addend) ; }
      static void *sub(void **value, ptrdiff_t subtrahend) { return ancestor::sub(reinterpret(value), subtrahend) ; }
      static void *get(void *value) { return ancestor::get(reinterpret(value)) ; }
      static void *xchg(void **value, void *new_value) { return ancestor::xchg(reinterpret(value), (char *)new_value) ; }
      static bool cas(void **value, void *expected_value, void *new_value)
      {
         return ancestor::cas(reinterpret(value), (char *)expected_value, (char *)new_value) ;
      }
} ;

} // end of namespace pcomn::atomic_op
} // end of namespace pcomn

#endif /* __PCOMN_ATOMIC_CC */
