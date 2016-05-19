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

 Provides "classic" atomic operations on "raw" variables, like e.g. atomic_op::inc(int*)
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
      typedef std::atomic<typename std::remove_cv<C>::type> type ;
      typedef C value_type ;
} ;

template<typename C>
struct atomic_type<std::atomic<C>> : atomic_type<C> {} ;
template<typename C>
struct atomic_type<const std::atomic<C>> : atomic_type<const C> {} ;
template<typename C>
struct atomic_type<const volatile std::atomic<C>> : atomic_type<const volatile C> {} ;
template<typename C>
struct atomic_type<volatile std::atomic<C>> : atomic_type<volatile C> {} ;

template<typename C>
using atomic_type_t = typename atomic_type<C>::type ;
template<typename C>
using atomic_value_t = typename atomic_type<C>::value_type ;

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

/// Atomic preincrement
template<typename T>
inline enable_if_atomic_arithmetic_t<atomic_value_t<T>>
inc(T *value)
{
   return ++*reinterpret_cast<atomic_type_t<T> *>(value) ;
}

/// Atomic predecrement
template<typename T>
inline enable_if_atomic_arithmetic_t<atomic_value_t<T>>
dec(T *value)
{
   return --*reinterpret_cast<atomic_type_t<T> *>(value) ;
}

/// Atomic add
template<typename T>
inline enable_if_atomic_arithmetic_t<atomic_value_t<T>>
add(T *value, ptrdiff_t addend)
{
   return *reinterpret_cast<atomic_type_t<T> *>(value) += addend ;
}

/// Atomic subtract
template<typename T>
inline enable_if_atomic_arithmetic_t<atomic_value_t<T>>
sub(T *value, ptrdiff_t subtrahend)
{
   return *reinterpret_cast<atomic_type_t<T> *>(value) -= subtrahend ;
}

/// Atomic load value
template<typename T>
inline enable_if_atomic_t<atomic_value_t<T>>
load(T *value, std::memory_order order = std::memory_order_seq_cst)
{
   return reinterpret_cast<const atomic_type_t<T> *>(value)
      ->load(order) ;
}

/// Atomic store value
template<typename T>
inline enable_if_atomic_t<T, void>
store(T *value, atomic_value_t<T> new_value, std::memory_order order = std::memory_order_seq_cst)
{
   reinterpret_cast<atomic_type_t<T> *>(value)->store(new_value, order) ;
}

/// Atomic set value and return previous
template<typename T>
inline enable_if_atomic_t<atomic_value_t<T>>
xchg(T *value, atomic_value_t<T> new_value, std::memory_order order = std::memory_order_seq_cst)
{
   return reinterpret_cast<atomic_type_t<T> *>(value)
      ->exchange(new_value, order) ;
}

/// Atomic compare-and-swap operation
template<typename T>
inline enable_if_atomic_t<T, bool>
cas(T *value, atomic_value_t<T> old_value, atomic_value_t<T> new_value,
    std::memory_order order = std::memory_order_seq_cst)
{
   return reinterpret_cast<atomic_type_t<T> *>(value)
      ->compare_exchange_strong(old_value, new_value, order) ;
}

/// Atomic set bound
/// @return true, is a value was set; false, if value was already changed
template<typename T, typename Predicate>
inline enable_if_atomic_t<T, bool>
set_bound(T *value, atomic_value_t<T> bound, Predicate pred)
{
   bool result = false ;
   for(T current ; !pred((current = load(value)), bound) && !(result|=cas(value, current, bound)) ;) ;
   return result ;
}

template<typename T>
inline enable_if_atomic_t<atomic_value_t<T>>
set_flags(T *value, atomic_value_t<T> flags)
{
   T cf ;
   while ((((cf = load(value)) & flags) != flags) && !cas(value, cf, cf | flags)) ;
   return cf | flags ;
}

template<typename T>
inline enable_if_atomic_t<atomic_value_t<T>>
clear_flags(T *value, atomic_value_t<T> flags)
{
   T cf ;
   while (((cf = load(value)) & flags) && !cas(value, cf, cf &~ flags)) ;
   return cf &~ flags ;
}

} // end of namespace pcomn::atomic_op
} // end of namespace pcomn

#endif // __PCOMN_ATOMIC
