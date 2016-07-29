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
#include <utility>
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

template<typename T, typename R = T>
using enable_if_atomic_ptr_t =
   typename std::enable_if<is_atomic<atomic_value_t<T>>::value && std::is_pointer<atomic_value_t<T>>::value, R>::type ;

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
/// Atomically replace the value pointed to by @a target with the value of @a newvalue
/// and return the value @a target held previously.
///
template<typename T>
inline enable_if_atomic_t<atomic_value_t<T>>
xchg(T *target, atomic_value_t<T> newvalue, std::memory_order order = std::memory_order_acq_rel)
{
   return reinterpret_cast<atomic_type_t<T> *>(target)
      ->exchange(newvalue, order) ;
}

/// Atomically compare the value of target object with argument and perform atomic
/// exchange if those are equal or atomic load if not.
///
/// Atomically compare the value pointed to by @a target with *expected_value,
/// and if those are bitwise-equal, replace *target with @a new_value
/// (read-modify-write). Otherwise, load *target into *expected.
///
/// @return The result of the comparison: true if *target was equal to *expected_value
/// (and thus *target is replaced), false otherwise.
///
template<typename T>
inline enable_if_atomic_t<T, bool>
cas(T *target, atomic_value_t<T> *expected_value, atomic_value_t<T> new_value,
    std::memory_order order = std::memory_order_acq_rel)
{
   return reinterpret_cast<atomic_type_t<T> *>(target)
      ->compare_exchange_strong(*expected_value, new_value, order) ;
}

/// @overload
template<typename T>
inline enable_if_atomic_t<T, bool>
cas(T *target, atomic_value_t<T> expected_value, atomic_value_t<T> new_value,
    std::memory_order order = std::memory_order_acq_rel)
{
   return cas(target, &expected_value, new_value, order) ;
}

/// Atomically compare the value of target object of size 2*sizeof(void*) with argument
/// and perform atomic exchange if those are equal or atomic load if not.
///
/// Works only on platforms with either native double-width cas support (x86/x86_64)
/// or with double-width LL/SC support (arm64
///
/// @return The result of the comparison: true if *target was equal to *expected_value
/// (and thus *target is replaced), false otherwise.
///
template<typename T>
inline enable_if_atomic_t<T, bool>
cas2(T *target, atomic_value_t<T> *expected_value, atomic_value_t<T> new_value,
    std::memory_order order = std::memory_order_acq_rel)
{
   return reinterpret_cast<atomic_type_t<T> *>(target)
      ->compare_exchange_strong(*expected_value, new_value, order) ;
}

/*******************************************************************************
 Fetch-And-Function
 Compare-And-Function
*******************************************************************************/
/// Atomic fetch_and_F.
/// @return Old value
template<typename T, typename F>
inline enable_if_atomic_t<atomic_value_t<T>>
fetch_and_F(T *value, F &&fn, std::memory_order order = std::memory_order_acq_rel)
{
   for (atomic_value_t<T> oldval = *value ;;)
   {
      const atomic_value_t<T> newval = std::forward<F>(fn)(oldval) ;
      if (cas(value, &oldval, newval, order))
         return oldval ;
   }
}

/// Atomic check_and_F
template<typename T, typename F, typename C>
inline enable_if_atomic_t<T, std::pair<bool, atomic_value_t<T>>>
check_and_F(T *value, C &&check, F &&fn, std::memory_order order = std::memory_order_acq_rel)
{
   atomic_value_t<T> oldval = *value ;
   while(std::forward<C>(check)(oldval))
      if (cas(value, &oldval, std::forward<F>(fn)(oldval), order))
         return {true, oldval} ;
   return {false, oldval} ;
}

template<typename T, typename C>
inline enable_if_atomic_t<T, std::pair<bool, atomic_value_t<T>>>
check_and_swap(T *target, C &&check, atomic_value_t<T> new_value,
               std::memory_order order = std::memory_order_acq_rel)
{
   return
      check_and_F(target, std::forward<C>(check), [=](atomic_value_t<T>){ return new_value ; }, order) ;
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
bit_and(T *value, atomic_value_t<T> bits, std::memory_order order = std::memory_order_acq_rel)
{
   return reinterpret_cast<atomic_type_t<T> *>(value)->fetch_and(bits, order) ;
}

template<typename T>
inline enable_if_atomic_arithmetic_t<atomic_value_t<T>>
bit_or(T *value, atomic_value_t<T> bits, std::memory_order order = std::memory_order_acq_rel)
{
   return reinterpret_cast<atomic_type_t<T> *>(value)->fetch_or(bits, order) ;
}

template<typename T>
inline enable_if_atomic_arithmetic_t<atomic_value_t<T>>
bit_xor(T *value, atomic_value_t<T> bits, std::memory_order order = std::memory_order_acq_rel)
{
   return reinterpret_cast<atomic_type_t<T> *>(value)->fetch_xor(bits, order) ;
}

template<typename T>
inline enable_if_atomic_arithmetic_t<atomic_value_t<T>>
bit_set(T *value, atomic_value_t<T> bits, atomic_value_t<T> mask, std::memory_order order = std::memory_order_acq_rel)
{
   bits &= mask ;
   return fetch_and_F(value, [=](atomic_value_t<T> v){ return v &~ mask | bits ; }, order) ;
}

/// Atomically compare selected set of bits of target object with the same set of bits of
/// the expected value and perform atomic set of that set of bits from desired value if
/// those are equal.
///
template<typename T>
inline enable_if_atomic_arithmetic_t<T, bool>
bit_cas(T *target, atomic_value_t<T> expected_bits, atomic_value_t<T> new_bits, atomic_value_t<T> mask,
        std::memory_order order = std::memory_order_acq_rel)
{
   typedef atomic_value_t<T> type ;

   expected_bits &= mask ;
   new_bits &= mask ;
   return check_and_F(target,
                      [=](type v){ return (v & mask) == expected_bits ; },
                      [=](type v){ return v &~ mask | new_bits ; },
                      order).first ;
}

/*******************************************************************************
 Tagged pointers
*******************************************************************************/
/// Set the least significant bit of a pointer to 1.
/// The pointed-to type must have alignment at least 2; it is checked at compile time.
template<typename T>
inline enable_if_atomic_ptr_t<atomic_value_t<T>>
tag_ptr(T *pptr, std::memory_order order)
{
   const uintptr_t tag = 1 ;
   uintptr_t * const data_ptr = reinterpret_cast<uintptr_t *>(pptr) ;
   return reinterpret_cast<atomic_value_t<T>>
      (bit_or(data_ptr, tag, order)) ;
}

/// Set the least significant log2(alignment) bits of a pointer to 0.
/// The pointed-to type must have alignment at least 2; it is checked at compile time.
template<typename T>
inline enable_if_atomic_ptr_t<atomic_value_t<T>>
untag_ptr(T *pptr, std::memory_order order)
{
   const uintptr_t mask = alignof(T) - 1 ;
   uintptr_t * const data_ptr = reinterpret_cast<uintptr_t *>(pptr) ;
   return reinterpret_cast<atomic_value_t<T>>
      (bit_and(data_ptr, mask, order)) ;
}

/// Invert the least significant bit of a pointer.
/// The pointed-to type must have alignment at least 2; it is checked at compile time.
template<typename T>
inline enable_if_atomic_ptr_t<atomic_value_t<T>>
fliptag_ptr(T *pptr, std::memory_order order)
{
   const uintptr_t tag = 1 ;
   uintptr_t * const data_ptr = reinterpret_cast<uintptr_t *>(pptr) ;
   return reinterpret_cast<atomic_value_t<T>>
      (bit_xor(data_ptr, tag, order)) ;
}

/******************************************************************************/
/** Implementation of LoadLinked/StoreConditional pointer variables.

 On platforms without native LL/SC (x86) uses CAS2 and generation counter,
 so it is 2*sizeof(void*) on such platforms and sizeof(void*) on LL/SC platforms
 (such as arm64, POWER8, etc.).
*******************************************************************************/
template<typename T>
class llsc_ptr {
      PCOMN_NONCOPYABLE(llsc_ptr) ;
      PCOMN_NONASSIGNABLE(llsc_ptr) ;

   public:
      typedef T element_type ;

      llsc_ptr(llsc_ptr &&other)

      element_type load_linked() ;

      bool store_condidtional(element_type value) ;

   private:
} ;

} // end of namespace pcomn::atomic_op
} // end of namespace pcomn

#endif // __PCOMN_ATOMIC
