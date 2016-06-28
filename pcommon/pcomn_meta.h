/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_META_H
#define __PCOMN_META_H
/*******************************************************************************
 FILE         :   pcomn_meta.h
 COPYRIGHT    :   Yakov Markovitch, 2006-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Basic template metaprogramming support.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   14 Nov 2006
*******************************************************************************/
/** @file Basic template metaprogramming support

 The template metaprogramming support in e.g. Boost is _indisputably_ much more
 extensive, but PCommon library should not depend on Boost.

 Besides, pcommon @em requires C++11 support to the extent of at least Visual Studio 2013
 compiler in order to keep its own code as simple and clean as possible, in contrast to
 Boost and other "industrial-grade" libraries that are much more portable at the cost of
 cryptic workarounds and bloated portability layers.
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcomn_macros.h>
#include <pcomn_assert.h>

#include <limits>
#include <utility>
#include <stdlib.h>
#include <stddef.h>
#include <type_traits>

/*******************************************************************************
 C++14 definitions for C++11 compiler
*******************************************************************************/
#ifndef PCOMN_STL_CXX14

namespace std {

template<typename T>
using remove_cv_t       = typename remove_cv<T>::type ;
template<typename T>
using remove_const_t    = typename remove_const<T>::type ;
template<typename T>
using remove_volatile_t = typename remove_volatile<T>::type ;

template<typename T>
using remove_pointer_t = typename remove_pointer<T>::type ;
template<typename T>
using remove_reference_t = typename remove_reference<T>::type ;

template<typename T>
using add_cv_t       = typename add_cv<T>::type ;
template<typename T>
using add_const_t    = typename add_const<T>::type ;
template<typename T>
using add_volatile_t = typename add_volatile<T>::type ;

template<typename T>
using add_lvalue_reference_t = typename add_lvalue_reference<T>::type ;
template<typename T>
using add_rvalue_reference_t = typename add_rvalue_reference<T>::type ;

template<typename T>
using add_pointer_t = typename add_pointer<T>::type ;

template<bool B, typename T>
using enable_if_t = typename enable_if<B, T>::type ;
template<bool B, typename T, typename F>
using conditional_t = typename conditional<B, T, F>::type ;

template<typename T>
using result_of_t = typename result_of<T>::type ;

template<typename T>
using underlying_type_t = typename underlying_type<T>::type ;

template<size_t sz, size_t align = alignof(max_align_t)>
using aligned_storage_t = typename aligned_storage<sz, align>::type ;

template<typename T>
using make_unsigned_t = typename make_unsigned<T>::type ;
template<typename T>
using make_signed_t = typename make_signed<T>::type ;

}

#endif /* PCOMN_STL_CXX14 */

#ifndef PCOMN_STL_CXX17

namespace std {

template<class C>
constexpr auto size(const C &container) -> decltype(container.size()) { return container.size() ; }

template<typename T, size_t N>
constexpr size_t size(const T (&)[N]) noexcept { return N ; }

template<class C>
constexpr auto empty(const C &container) -> decltype(container.empty()) { return container.empty() ; }

template<typename T, size_t N>
constexpr bool empty(const T (&)[N]) noexcept { return false ; }

template<typename E>
constexpr bool empty(std::initializer_list<E> v) noexcept { return v.size() == 0 ; }

template<bool v>
using bool_constant = std::integral_constant<bool, v> ;

}

#endif /* PCOMN_STL_CXX17 */

namespace pcomn {

using std::bool_constant ;

template<int v>
using int_constant = std::integral_constant<int, v> ;

template<unsigned v>
using uint_constant = std::integral_constant<unsigned, v> ;

template<long v>
using long_constant = std::integral_constant<long, v> ;

template<unsigned long v>
using ulong_constant = std::integral_constant<unsigned long, v> ;

template<long long v>
using longlong_constant = std::integral_constant<long long, v> ;

template<unsigned long long v>
using ulonglong_constant = std::integral_constant<unsigned long long, v> ;

/******************************************************************************/
/** Function for getting T value in unevaluated context for e.g. passsing
 to functions in SFINAE context without the need to go through constructors

 Differs from std::declval in that it does not converts T to rvalue reference
*******************************************************************************/
template<typename T>
T autoval() ;

/******************************************************************************/
/** disable_if is a complement to std::enable_if
*******************************************************************************/
template<bool disabled, typename T>
using disable_if = std::enable_if<!disabled, T> ;

template<bool disabled, typename T>
using disable_if_t = typename disable_if<disabled, T>::type ;

template<bool enabled>
using instance_if_t = std::enable_if_t<enabled, Instantiate> ;

/*******************************************************************************
 Template compile-time logic operations.
*******************************************************************************/
#define PCOMN_CTVALUE(type, name, expr) static const type name = (expr)

template<class L, class R>
using ct_and = bool_constant<L::value && R::value> ;
template<class L, class R>
using ct_or = bool_constant<L::value || R::value> ;
template<class L, class R>
using ct_xor = bool_constant<!L::value != !R::value> ;
template<class L>
using ct_not = bool_constant<!L::value> ;
// While we can express ct_nand, ct_nor and ct_nxor in terms of already
// defined templates (using public inheritance), I prefer to define their
// 'value' member directly (to take some burden from the compiler)
template<class L, class R>
using ct_nand = bool_constant<!(L::value && R::value)> ;
template<class L, class R>
using ct_nor = bool_constant<!(L::value || R::value)> ;
template<class L, class R>
using ct_nxor = bool_constant<!L::value == !R::value> ;

/*******************************************************************************
 Compile-time min/max.
*******************************************************************************/
template<typename T, T...> struct ct_min ;
template<typename T, T...> struct ct_max ;

template<typename T, T x> struct ct_min<T, x> :
         std::integral_constant<T, x> {} ;
template<typename T, T x, T...y> struct ct_min<T, x, y...> :
         std::integral_constant<T, (x < ct_min<T, y...>::value ? x : ct_min<T, y...>::value)> {} ;

template<typename T, T x> struct ct_max<T, x> :
         std::integral_constant<T, x> {} ;
template<typename T, T x, T...y> struct ct_max<T, x, y...> :
         std::integral_constant<T, (ct_max<T, y...>::value < x  ? x : ct_max<T, y...>::value)> {} ;

/******************************************************************************/
/** Creates unique type from another type.

 This allows to completely separate otherwise compatible types (e.g. pointer to derived
 and pointer to base, etc.)
*******************************************************************************/
template<typename T> struct identity_type { typedef T type ; } ;

/******************************************************************************/
/** Provides globally placed default-constructed value of its parameter type.
*******************************************************************************/
template<typename T>
struct default_constructed {
      typedef T type ;
      static const type value ;
} ;

template<typename T>
const T default_constructed<T>::value = {} ;

/*******************************************************************************
 Type testers
*******************************************************************************/
template<typename Base, typename Derived>
struct is_base_of_strict :
         bool_constant<!(std::is_same<Base, Derived>::value ||
                         std::is_same<const volatile Base*, const volatile void*>::value) &&
                         std::is_convertible<const volatile Derived*, const volatile Base*>::value> {} ;

template<typename T, typename U>
using is_same_unqualified = std::is_same<std::remove_cv_t<T>, std::remove_cv_t<U> > ;

/*******************************************************************************
 Parameter type
*******************************************************************************/
template<typename T>
using lvref_t = typename std::add_lvalue_reference<T>::type ;

template<typename T>
using clvref_t = lvref_t<typename std::add_const<T>::type> ;

template<typename T>
using parmtype_t = typename std::conditional<(std::is_arithmetic<T>::value || std::is_pointer<T>::value), T, clvref_t<T> >::type ;

/******************************************************************************/
/** Deduce the return type of a function call expression at compile time @em and,
 if the deduced type is a reference type, provides the member typedef type which
 is the type referred to, otherwise deduced type itself.
*******************************************************************************/
template<typename> struct noref_result_of ;

template<typename F, typename... ArgTypes>
struct noref_result_of<F(ArgTypes...)> :
         std::remove_reference<std::result_of_t<F(ArgTypes...)> > {} ;

template<typename T>
using noref_result_of_t = typename noref_result_of<T>::type ;

template<typename T>
using valtype_t = std::remove_cv_t<std::remove_reference_t<T> > ;

template<typename T, typename... A>
struct rebind___t {
      template<template<typename...> class Template, typename A1, typename... Args>
      static Template<T, A...> eval(Template<A1, Args...> *) ;
} ;

template<typename C, typename T, typename... A>
using rebind_t = decltype(rebind___t<T, A...>::eval(autoval<C *>())) ;

/******************************************************************************/
/** Macro to define member type metatesters
*******************************************************************************/
#define PCOMN_DEFINE_TYPE_MEMBER_TEST(type)                             \
   namespace detail {                                                   \
   template<typename T>                                                 \
   std::false_type has_##type##_test(...) ;                             \
   template<typename T>                                                 \
   std::true_type has_##type##_test(typename T::type const volatile *) ; \
   }                                                                    \
                                                                        \
   template<typename T>                                                 \
   struct has_##type : decltype(detail::has_##type##_test<T>(0)) {}

/*******************************************************************************
 Define member type metatests for std:: member typedefs
*******************************************************************************/
PCOMN_DEFINE_TYPE_MEMBER_TEST(key_type) ;
PCOMN_DEFINE_TYPE_MEMBER_TEST(mapped_type) ;
PCOMN_DEFINE_TYPE_MEMBER_TEST(value_type) ;
PCOMN_DEFINE_TYPE_MEMBER_TEST(element_type) ;
PCOMN_DEFINE_TYPE_MEMBER_TEST(iterator) ;
PCOMN_DEFINE_TYPE_MEMBER_TEST(const_iterator) ;
PCOMN_DEFINE_TYPE_MEMBER_TEST(pointer) ;

/// @cond
// Intentionally create an inconsistent name pcomn::has_type_type
// has_type is too generic a name, even for pcomn namespace.
namespace detail { PCOMN_DEFINE_TYPE_MEMBER_TEST(type) ; }
template<typename T>
struct has_type_type : detail::has_type<T> {} ;

/*******************************************************************************
 Check whether the derived class overloads base class member or not
*******************************************************************************/
namespace detail {
template<typename C, typename R, typename... Args>
std::integral_constant<bool, true> is_derived_mem_fn_(R (C::*)(Args...) const) ;
template<typename C, typename R, typename... Args>
std::integral_constant<bool, true> is_derived_mem_fn_(R (C::*)(Args...)) ;
template<typename C>
std::integral_constant<bool, false> is_derived_mem_fn_(...) ;

template<typename C, typename T>
std::integral_constant<bool, true> is_derived_mem_(T C::*) ;
template<typename C>
std::integral_constant<bool, false> is_derived_mem_(...) ;
}

/******************************************************************************/
/** Metafunction: check if the member function of a derived class is derived from
 the base class or overloaded in the derived class.

 @return std::integral_constant<bool,true> if derived, std::integral_constant<bool,false>
 if overloaded.
*******************************************************************************/
template<typename Base, typename Member>
constexpr decltype(detail::is_derived_mem_fn_<Base>(std::declval<Member>()))
is_derived_mem_fn(Member &&) { return {} ; }

/******************************************************************************/
/** Metafunction: check if the member of a derived class is derived from
 the base class or overloaded in the derived class.

 @return std::integral_constant<bool,true> if derived, std::integral_constant<bool,false>
 if overloaded.
*******************************************************************************/
template<typename Base, typename Member>
constexpr decltype(detail::is_derived_mem_<Base>(std::declval<Member>()))
is_derived_mem(Member &&) { return {} ; }

/******************************************************************************/
/** Uniform pair (i.e. pair<T,T>)
*******************************************************************************/
template<typename T>
using unipair = std::pair<T, T> ;

} // end of namespace pcomn
/// @endcond

/*******************************************************************************
 Ersatz C++11 type taits: even GCC 4.9 does not provide is_trivially_something
 traits, and we desperately need is_trivially_copyable.

 Note this contrived trait class is not a complete equivalent of the standard
 one: it does not consider move constructor/assignment.
*******************************************************************************/
#if PCOMN_WORKAROUND(__GNUC_VER__, < 600)
namespace std {
#if PCOMN_WORKAROUND(__GNUC_VER__, < 500)
template<typename T, typename U>
struct has_trivial_copy_assign<pair<T, U> > :
         pcomn::ct_and<has_trivial_copy_assign<T>, has_trivial_copy_assign<U> > {} ;

template<typename T>
struct is_trivially_copyable :
         pcomn::ct_and<has_trivial_copy_constructor<T>, has_trivial_copy_assign<T> > {} ;
#else
template<typename T, typename U>
struct is_trivially_copyable<pair<T, U> > :
         pcomn::ct_and<is_trivially_copyable<T>, is_trivially_copyable<U> > {} ;
#endif
}
#endif // PCOMN_WORKAROUND(__GNUC_VER__, < 500)

#endif /* __PCOMN_META_H */
