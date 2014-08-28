/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_ATOMIC
#define __PCOMN_ATOMIC
/*******************************************************************************
 FILE         :   pcomn_atomic.h
 COPYRIGHT    :   Yakov Markovitch, 2001-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Atomic operations support for all platforms

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   16 Apr 2001
*******************************************************************************/
/** @file
  Atomic operations (inc, dec, xchg) support for all platforms.
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcomn_meta.h>

#include <stddef.h>

namespace pcomn {

typedef long            atomic_t ;
typedef unsigned long   uatomic_t ;

typedef int32_t   atomic32_t ;
typedef uint32_t  uatomic32_t ;

typedef PCOMN_ALIGNED(8) int64_t    atomic64_t ;
typedef PCOMN_ALIGNED(8) uint64_t   uatomic64_t ;

} // end of namespace pcomn

#include PCOMN_PLATFORM_HEADER(pcomn_atomic.cc)

namespace pcomn {

template<typename T>
struct is_atomic : bool_constant<std::is_pointer<T>::value ||
                                 std::is_integral<T>::value &&
                                 std::alignment_of<T>::value >= 4 &&
                                 sizeof(T) >= 4 && sizeof(T) <= 8> {} ;

template<typename T, typename R> struct
enable_if_atomic : public std::enable_if<is_atomic<T>::value, R> {} ;

/******************************************************************************/
/** @namespace pcomn::atomic_op
 Atomic operations
*******************************************************************************/
namespace atomic_op {

template<size_t sz, bool sign = false> struct traits ;

template<> struct traits<4, true>    { typedef atomic32_t type ; } ;
template<> struct traits<4, false>   { typedef uatomic32_t type ; } ;
template<> struct traits<8, true>    { typedef atomic64_t type ; } ;
template<> struct traits<8, false>   { typedef uatomic64_t type ; } ;

#define PCOMN_ATOMIC_RESULT(result)                                     \
   typename std::enable_if<(sizeof(T)==4 || sizeof(T)==8) &&                 \
                           std::alignment_of<T>::value >= std::alignment_of<typename traits<sizeof(T)>::type>::value, \
      result>::type

/// Atomic preincrement
template<typename T>
inline typename enable_if_atomic<T, T>::type
inc(T *value)
{
   return implementor<T, std::is_integral<T>::value>::inc(value) ;
}

/// Atomic predecrement
template<typename T>
inline typename enable_if_atomic<T, T>::type
dec(T *value)
{
   return implementor<T, std::is_integral<T>::value>::dec(value) ;
}

/// Atomic add
template<typename T>
inline typename enable_if_atomic<T, T>::type
add(T *value, ptrdiff_t addend)
{
   return implementor<T, std::is_integral<T>::value>::add(value, addend) ;
}

/// Atomic subtract
template<typename T>
inline typename enable_if_atomic<T, T>::type
sub(T *value, ptrdiff_t subtrahend)
{
   return implementor<T, std::is_integral<T>::value>::sub(value, subtrahend) ;
}

/// Atomic get value
template<typename T>
inline PCOMN_ATOMIC_RESULT(T) get(T *value)
{
   typedef typename traits<sizeof(T)>::type atomic ;

   atomic result = implementor<atomic, true>::get((atomic *)(void *)value) ;
   return reinterpret_cast<T &&>(*(&result)) ;
}

/// Atomic set value and return previous
template<typename T>
inline PCOMN_ATOMIC_RESULT(T) xchg(T *value, T new_value)
{
   typedef typename traits<sizeof(T)>::type atomic ;
   atomic result = implementor<atomic, true>::xchg((atomic *)(void *)value,
                                                   *(atomic *)(void *)&new_value) ;
   return reinterpret_cast<T &&>(*(&result)) ;
}

/// Atomic compare-and-swap operation
template<typename T>
inline PCOMN_ATOMIC_RESULT(bool) cas(T *value, T old_value, T new_value)
{
   typedef typename traits<sizeof(T)>::type atomic ;

   return implementor<atomic, true>::cas((atomic *)(void *)value,
                                         *(atomic *)(void *)&old_value,
                                         *(atomic *)(void *)&new_value) ;
}

/// Atomic set bound
/// @return true, is a value was set; false, if value was already changed
template<typename T, typename Predicate>
inline PCOMN_ATOMIC_RESULT(bool) set_bound(T *value, T bound, Predicate pred)
{
   bool result = false ;
   for(T current ; !pred((current = get(value)), bound) && !(result|=cas(value, current, bound)) ;) ;
   return result ;
}

template<typename T>
inline PCOMN_ATOMIC_RESULT(T) set_flags(T *value, T flags)
{
   T cf ;
   while ((((cf = get(value)) & flags) != flags) && !cas(value, cf, cf | flags)) ;
   return cf | flags ;
}

template<typename T>
inline PCOMN_ATOMIC_RESULT(T) clear_flags(T *value, T flags)
{
   T cf ;
   while (((cf = get(value)) & flags) && !cas(value, cf, cf &~ flags)) ;
   return cf &~ flags ;
}

/*******************************************************************************
 Atomic operations threading policies
*******************************************************************************/
/******************************************************************************/
/** Single-threaded program policy.
*******************************************************************************/
struct SingleThreaded {

      template<typename T> static T inc(T &value) { return ++value ; }
      template<typename T> static T dec(T &value) { return --value ; }
      template<typename T> static T get(T &value)
      {
         PCOMN_STATIC_CHECK(std::is_trivially_copyable<T>::value) ;
         return value ;
      }
      template<typename T> static T xchg(T &value, const T &new_value)
      {
         PCOMN_STATIC_CHECK(std::is_trivially_copyable<T>::value) ;
         const T old = value ;
         value = new_value ;
         return old ;
      }

      template<typename T>
      static bool cas(T &value, const T &old_value, const T &new_value)
      {
         PCOMN_STATIC_CHECK(std::is_trivially_copyable<T>::value) ;
         if (old_value != value)
            return false ;
         value = new_value ;
         return true ;
      }

      template<typename T>
      static T reset(T &value) { SingleThreaded::xchg(value, T()) ; }
} ;

/******************************************************************************/
/** Multiple-threaded program policy.
*******************************************************************************/
struct MultiThreaded {
      template<typename T> static T inc(T &value) { return ::pcomn::atomic_op::inc(&value) ; }
      template<typename T> static T dec(T &value) { return ::pcomn::atomic_op::dec(&value) ; }

      template<typename T> static T get(T &value) { return ::pcomn::atomic_op::get(&value) ; }
      template<typename T> static T xchg(T &value, const T &new_value)
      {
         return ::pcomn::atomic_op::xchg(&value, new_value) ;
      }

      template<typename T>
      static T reset(T &value) { MultiThreaded::xchg(value, T()) ; }

      template<typename T>
      static bool cas(T &value, const T &old_value, const T &new_value)
      {
         return ::pcomn::atomic_op::cas(&value, old_value, new_value) ;
      }
} ;

} // end of namespace pcomn::atomic_op
} // end of namespace pcomn

#endif // __PCOMN_ATOMIC
