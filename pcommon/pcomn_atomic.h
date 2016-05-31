/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_ATOMIC
#define __PCOMN_ATOMIC
/*******************************************************************************
 FILE         :   pcomn_atomic.h
 COPYRIGHT    :   Yakov Markovitch, 2001-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Atomic operations support for all platforms

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   16 Apr 2001
*******************************************************************************/
/** @file
  Global functions implementing atomic operations on "raw" types using their
  std::atomic counterparts.

 Provides "classic" atomic operations on "raw" variables, like e.g. atomic_op::preinc(int*)
 (vs. std::atomic<int>).

 Sometimes "explicit atomic operation on a suitable standard layout type" is
 better way than using std::atomic<>.
 Sometimes it is the @em only way, for e.g. interop with pre-existing code.
*******************************************************************************/
#include <pcomn_platform.h>

#include <atomic>
#include <type_traits>

#include <stddef.h>

namespace pcomn {

template<typename C>
struct atomic_type {
      typedef typename std::remove_cv<C>::type  value_type ;
      typedef std::atomic<value_type>           type ;
} ;

template<typename C>
struct atomic_type<std::atomic<C>> : atomic_type<C> {} ;
template<typename C>
struct atomic_type<const std::atomic<C>> : atomic_type<C> {} ;
template<typename C>
struct atomic_type<const volatile std::atomic<C>> : atomic_type<C> {} ;
template<typename C>
struct atomic_type<volatile std::atomic<C>> : atomic_type<C> {} ;

template<typename C>
using atomic_type_t = typename atomic_type<C>::type ;
template<typename C>
using atomic_value_t = typename atomic_type<C>::value_type ;

/******************************************************************************/
/** Type trait equals true if this atomic type is always lock-free.
*******************************************************************************/
template<typename T>
using is_atomic_placement =
   std::integral_constant<bool, sizeof(T) >= 4 && sizeof(T) <= sizeof(void *) && alignof(T) >= sizeof(T)> ;

/******************************************************************************/
/** If the type is eligible for both atomic load/store and integral or pointer
 arithmetic operations, provides the member constant @a value equal to true;
 otherwise, false.
*******************************************************************************/
template<typename T>
struct is_atomic_arithmetic : std::integral_constant
<bool,
 std::is_pointer<T>::value ||
 std::is_integral<T>::value && is_atomic_placement<T>::value>
{} ;

/******************************************************************************/
/** If the type is eligible for atomic store, load, and exchange, provides the member
 constant @a value equal to true; otherwise, false.
*******************************************************************************/
template<typename T>
struct is_atomic : std::integral_constant
<bool,
 is_atomic_arithmetic<T>::value ||
 std::is_trivially_copyable<T>::value && is_atomic_placement<T>::value>
{} ;

template<typename T, typename R = T>
using enable_if_atomic_t =
   typename std::enable_if<is_atomic<atomic_value_t<T>>::value, R>::type ;

template<typename T, typename R = T>
using enable_if_atomic_arithmetic_t =
   typename std::enable_if<is_atomic_arithmetic<atomic_value_t<T>>::value, R>::type ;

/******************************************************************************/
/** @namespace pcomn::atomic_op
 Atomic operations
*******************************************************************************/
namespace atomic_op {

/*******************************************************************************
 Ordered load
 Ordered store
*******************************************************************************/
/// Ordered load value
template<typename T>
inline enable_if_atomic_t<atomic_value_t<T>>
load(T *value, std::memory_order order = std::memory_order_acq_rel)
{
   return reinterpret_cast<const atomic_type_t<T> *>(value)
      ->load(order) ;
}

/// Ordered store value
template<typename T>
inline enable_if_atomic_t<T, void>
store(T *value, atomic_value_t<T> new_value, std::memory_order order = std::memory_order_acq_rel)
{
   reinterpret_cast<atomic_type_t<T> *>(value)->store(new_value, order) ;
}

/*******************************************************************************
 Compare-And-Swap
 Atomic exchange
*******************************************************************************/
/// Atomic set value and return previous
template<typename T>
inline enable_if_atomic_t<atomic_value_t<T>>
xchg(T *value, atomic_value_t<T> new_value, std::memory_order order = std::memory_order_acq_rel)
{
   return reinterpret_cast<atomic_type_t<T> *>(value)
      ->exchange(new_value, order) ;
}

/// Atomic compare-and-swap operation
template<typename T>
inline enable_if_atomic_t<T, bool>
cas(T *value, atomic_value_t<T> old_value, atomic_value_t<T> new_value,
    std::memory_order order = std::memory_order_acq_rel)
{
   return reinterpret_cast<atomic_type_t<T> *>(value)
      ->compare_exchange_strong(old_value, new_value, order) ;
}

/*******************************************************************************
 Fetch-And-Function
 Compare-And-Function
*******************************************************************************/
/// Atomic fetch_and_F
template<typename T, typename F>
inline enable_if_atomic_t<atomic_value_t<T>>
fetch_and_F(T *value, F fn, std::memory_order order = std::memory_order_acq_rel)
{
   for(;;)
   {
      atomic_value_t<T> oldval = *value ;
      atomic_value_t<T> newval = fn(oldval) ;
      if (cas(value, oldval, newval, order))
         return oldval ;
   }
}

/// Atomic compare_and_F
template<typename T, typename F, typename C>
inline enable_if_atomic_t<T, bool>
compare_and_F(T *value, C comparator, F fn, std::memory_order order = std::memory_order_acq_rel)
{
   for(atomic_value_t<T> oldval ; comparator(oldval = *value) ; )
      if (cas(value, oldval, fn(oldval), order))
         return true ;
   return false ;
}

/*******************************************************************************
 Atomic arithmetic and bit operations
*******************************************************************************/
/// Atomic add
template<typename T>
inline enable_if_atomic_arithmetic_t<atomic_value_t<T>>
add(T *value, ptrdiff_t addend, std::memory_order order = std::memory_order_acq_rel)
{
   return reinterpret_cast<atomic_type_t<T> *>(value)->fetch_add(addend, order) + addend ;
}

/// Atomic subtract
template<typename T>
inline enable_if_atomic_arithmetic_t<atomic_value_t<T>>
sub(T *value, ptrdiff_t subtrahend, std::memory_order order = std::memory_order_acq_rel)
{
   return reinterpret_cast<atomic_type_t<T> *>(value)->fetch_sub(subtrahend, order) - subtrahend ;
}

/// Atomic preincrement
template<typename T>
inline enable_if_atomic_arithmetic_t<atomic_value_t<T>>
preinc(T *value, std::memory_order order = std::memory_order_acq_rel)
{
   return add(value, 1, order) ;
}

/// Atomic predecrement
template<typename T>
inline enable_if_atomic_arithmetic_t<atomic_value_t<T>>
predec(T *value, std::memory_order order = std::memory_order_acq_rel)
{
   return sub(value, 1, order) ;
}

/// Atomic postincrement
template<typename T>
inline enable_if_atomic_arithmetic_t<atomic_value_t<T>>
postinc(T *value, std::memory_order order = std::memory_order_acq_rel)
{
   return reinterpret_cast<atomic_type_t<T> *>(value)->fetch_add(1, order) ;
}

/// Atomic postdecrement
template<typename T>
inline enable_if_atomic_arithmetic_t<atomic_value_t<T>>
postdec(T *value, std::memory_order order = std::memory_order_acq_rel)
{
   return reinterpret_cast<atomic_type_t<T> *>(value)->fetch_sub(1, order) ;
}

template<typename T>
inline enable_if_atomic_arithmetic_t<atomic_value_t<T>>
bit_and(T *value, atomic_value_t<T> operand, std::memory_order order = std::memory_order_acq_rel)
{
   return reinterpret_cast<atomic_type_t<T> *>(value)->fetch_and(operand, order) ;
}

template<typename T>
inline enable_if_atomic_arithmetic_t<atomic_value_t<T>>
bit_or(T *value, atomic_value_t<T> operand, std::memory_order order = std::memory_order_acq_rel)
{
   return reinterpret_cast<atomic_type_t<T> *>(value)->fetch_or(operand, order) ;
}

template<typename T>
inline enable_if_atomic_arithmetic_t<atomic_value_t<T>>
bit_xor(T *value, atomic_value_t<T> operand, std::memory_order order = std::memory_order_acq_rel)
{
   return reinterpret_cast<atomic_type_t<T> *>(value)->fetch_xor(operand, order) ;
}

template<typename T>
inline enable_if_atomic_arithmetic_t<T, bool>
cas_mask(T *value, atomic_value_t<T> oldvalue, atomic_value_t<T> newvalue, atomic_value_t<T> mask,
         std::memory_order order = std::memory_order_acq_rel)
{
   typedef atomic_value_t<T> type ;

   oldvalue &= mask ;
   newvalue &= mask ;
   return compare_and_F(value,
                        [=](type v){ return v & mask == oldvalue ; },
                        [=](type v){ return v &~ mask | newvalue ; },
                        order) ;
}

template<typename T>
inline enable_if_atomic_arithmetic_t<T>
fetch_and_mask(T *value, atomic_value_t<T> newvalue, atomic_value_t<T> mask,
               std::memory_order order = std::memory_order_acq_rel)
{
   typedef atomic_value_t<T> type ;

   newvalue &= mask ;
   return fetch_and_F(value, [=](type v){ return v &~ mask | newvalue ; }, order) ;
}

} // end of namespace pcomn::atomic_op
} // end of namespace pcomn

#endif // __PCOMN_ATOMIC
