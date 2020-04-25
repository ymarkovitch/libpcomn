/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_META_H
#define __PCOMN_META_H
/*******************************************************************************
 FILE         :   pcomn_meta.h
 COPYRIGHT    :   Yakov Markovitch, 2006-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Basic template metaprogramming support.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   14 Nov 2006
*******************************************************************************/
/** @file Basic template metaprogramming support

 The template metaprogramming support in e.g. Boost is _indisputably_ much more
 extensive, but PCommon library should not depend on Boost.

 Besides, pcommon @em requires C++14 support to the extent of at least Visual Studio 2015
 compiler in order to keep its own code as simple and clean as possible, in contrast to
 Boost and other "industrial-grade" libraries that are much more portable at the cost of
 cryptic workarounds and bloated portability layers.
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcomn_macros.h>
#include <pcomn_assert.h>

#include <limits>
#include <utility>
#include <type_traits>

#ifndef PCOMN_STL_CXX17
#include <experimental/type_traits>
#endif /* PCOMN_STL_CXX17 */

// Avoid including the whole <string>
#if defined(__GLIBCXX__)
#include <bits/stringfwd.h>
#endif

#include <stdlib.h>
#include <stddef.h>

#ifndef PCOMN_STL_CXX17

namespace std {

template<class C>
inline constexpr auto size(const C &container) -> decltype(container.size()) { return container.size() ; }

template<typename T, size_t N>
inline constexpr size_t size(const T (&)[N]) noexcept { return N ; }

template<class C>
inline constexpr auto empty(const C &container) -> decltype(container.empty()) { return container.empty() ; }

template<typename T, size_t N>
inline constexpr bool empty(const T (&)[N]) noexcept { return false ; }

template<typename E>
inline constexpr bool empty(std::initializer_list<E> v) noexcept { return v.size() == 0 ; }

template<typename C, size_t n>
inline constexpr C *data(C (&arr)[n]) { return arr ; }

template<typename C, typename R, typename A>
inline constexpr C *data(std::basic_string<C, R, A> &s)
{
   return const_cast<C *>(s.data()) ;
}

template<typename T>
inline constexpr auto data(T &&container) -> decltype(std::forward<T>(container).data())
{
   return std::forward<T>(container).data() ;
}

template<bool v>
using bool_constant = std::integral_constant<bool, v> ;

template<typename...> using void_t = void ;

using std::experimental::is_void_v ;
using std::experimental::is_null_pointer_v ;
using std::experimental::is_integral_v ;
using std::experimental::is_floating_point_v ;
using std::experimental::is_array_v ;
using std::experimental::is_pointer_v ;
using std::experimental::is_lvalue_reference_v ;
using std::experimental::is_rvalue_reference_v ;
using std::experimental::is_member_object_pointer_v ;
using std::experimental::is_member_function_pointer_v ;
using std::experimental::is_enum_v ;
using std::experimental::is_union_v ;
using std::experimental::is_class_v ;
using std::experimental::is_function_v ;
using std::experimental::is_reference_v ;
using std::experimental::is_arithmetic_v ;
using std::experimental::is_fundamental_v ;
using std::experimental::is_object_v ;
using std::experimental::is_scalar_v ;
using std::experimental::is_compound_v ;
using std::experimental::is_member_pointer_v ;
using std::experimental::is_const_v ;
using std::experimental::is_volatile_v ;
using std::experimental::is_trivial_v ;
using std::experimental::is_trivially_copyable_v ;
using std::experimental::is_standard_layout_v ;
using std::experimental::is_pod_v ;
using std::experimental::is_literal_type_v ;
using std::experimental::is_empty_v ;
using std::experimental::is_polymorphic_v ;
using std::experimental::is_abstract_v ;
using std::experimental::is_final_v ;
using std::experimental::is_signed_v ;
using std::experimental::is_unsigned_v ;
using std::experimental::is_constructible_v ;
using std::experimental::is_trivially_constructible_v ;
using std::experimental::is_nothrow_constructible_v ;
using std::experimental::is_default_constructible_v ;
using std::experimental::is_trivially_default_constructible_v ;
using std::experimental::is_nothrow_default_constructible_v ;
using std::experimental::is_copy_constructible_v ;
using std::experimental::is_trivially_copy_constructible_v ;
using std::experimental::is_nothrow_copy_constructible_v ;
using std::experimental::is_move_constructible_v ;
using std::experimental::is_trivially_move_constructible_v ;
using std::experimental::is_nothrow_move_constructible_v ;
using std::experimental::is_assignable_v ;
using std::experimental::is_trivially_assignable_v ;
using std::experimental::is_nothrow_assignable_v ;
using std::experimental::is_copy_assignable_v ;
using std::experimental::is_trivially_copy_assignable_v ;
using std::experimental::is_nothrow_copy_assignable_v ;
using std::experimental::is_move_assignable_v ;
using std::experimental::is_trivially_move_assignable_v ;
using std::experimental::is_nothrow_move_assignable_v ;
using std::experimental::is_destructible_v ;
using std::experimental::is_trivially_destructible_v ;
using std::experimental::is_nothrow_destructible_v ;
using std::experimental::has_virtual_destructor_v ;
using std::experimental::alignment_of_v ;
using std::experimental::rank_v;
using std::experimental::extent_v ;
using std::experimental::is_same_v ;
using std::experimental::is_base_of_v ;
using std::experimental::is_convertible_v ;

} // end of namespace std

#endif /* PCOMN_STL_CXX17 */

#ifndef PCOMN_STL_CXX20

namespace std {

template<typename T>
using remove_cvref = std::remove_cv<std::remove_reference_t<T>> ;

template<typename T>
using remove_cvref_t = typename remove_cvref<T>::type ;

} // end of namespace std

#endif /* PCOMN_STL_CXX20 */

namespace pcomn {

inline namespace traits {

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

template<size_t v>
using size_constant = std::integral_constant<size_t, v> ;

/***************************************************************************//**
 @name TypeTraits
 Type properties checks that are evaluated to std::bool_constant<>.

 While many (most?) implementations of STL implement type traits classes by deriving
 from std::integral_constant<bool, ...>, it is not mandated by the Standard.
 So, if protable code needs to get bool_constant<> as a result of type trait check,
 it has to use `typename is_xxx<T>::type`.
 So we provide corresponding template typedefs to reduce such verbosity.
*******************************************************************************/
/**@{*/
template<typename T> using is_abstract_t     = typename std::is_abstract<T>::type ;
template<typename T> using is_arithmetic_t   = typename std::is_arithmetic<T>::type ;
template<typename T> using is_array_t        = typename std::is_array<T>::type ;
template<typename T> using is_class_t        = typename std::is_class<T>::type ;
template<typename T> using is_compound_t     = typename std::is_compound<T>::type ;
template<typename T> using is_const_t        = typename std::is_const<T>::type ;
template<typename T> using is_empty_t        = typename std::is_empty<T>::type ;
template<typename T> using is_enum_t         = typename std::is_enum<T>::type ;
template<typename T> using is_floating_point_t = typename std::is_floating_point<T>::type ;
template<typename T> using is_function_t     = typename std::is_function<T>::type ;
template<typename T> using is_fundamental_t  = typename std::is_fundamental<T>::type ;
template<typename T> using is_integral_t     = typename std::is_integral<T>::type ;
template<typename T> using is_lvalue_reference_t = typename std::is_lvalue_reference<T>::type ;
template<typename T> using is_member_function_pointer_t = typename std::is_member_function_pointer<T>::type ;
template<typename T> using is_member_object_pointer_t = typename std::is_member_object_pointer<T>::type ;
template<typename T> using is_member_pointer_t = typename std::is_member_pointer<T>::type ;
template<typename T> using is_object_t       = typename std::is_object<T>::type ;
template<typename T> using is_pod_t          = typename std::is_pod<T>::type ;
template<typename T> using is_pointer_t      = typename std::is_pointer<T>::type ;
template<typename T> using is_polymorphic_t  = typename std::is_polymorphic<T>::type ;
template<typename T> using is_reference_t    = typename std::is_reference<T>::type ;
template<typename T> using is_rvalue_reference_t = typename std::is_rvalue_reference<T>::type ;
template<typename T> using is_scalar_t       = typename std::is_scalar<T>::type ;
template<typename T> using is_signed_t       = typename std::is_signed<T>::type ;
template<typename T> using is_standard_layout_t = typename std::is_standard_layout<T>::type ;
template<typename T> using is_trivial_t      = typename std::is_trivial<T>::type ;
template<typename T> using is_trivially_copyable_t = typename std::is_trivially_copyable<T>::type ;
template<typename T> using is_trivially_destructible_t = typename std::is_trivially_destructible<T>::type ;
template<typename T> using is_union_t        = typename std::is_union<T>::type ;
template<typename T> using is_unsigned_t     = typename std::is_unsigned<T>::type ;
template<typename T> using is_void_t         = typename std::is_void<T>::type ;
template<typename T> using is_volatile_t     = typename std::is_volatile<T>::type ;
/**@}*/

} // end of inline namespace pcomn::traits

template<typename To, typename From>
constexpr __forceinline To bit_cast(const From &from) noexcept
{
   PCOMN_STATIC_CHECK(sizeof(To) == sizeof(From)) ;
   PCOMN_STATIC_CHECK(alignof(To) <= alignof(From)) ;
   PCOMN_STATIC_CHECK(std::is_trivially_copyable_v<From> && std::is_trivially_copyable_v<To>) ;

   const union { const From *src ; const To *dst ; } result = {&from} ;
   return *result.dst ;
}

/***************************************************************************//**
 Function for getting T value in unevaluated context for e.g. passsing
 to functions in SFINAE context without the need to go through constructors

 Differs from std::declval in that it does not converts T to rvalue reference
*******************************************************************************/
template<typename T>
T autoval() ;

/***************************************************************************//**
 disable_if is a complement to std::enable_if
*******************************************************************************/
template<bool disabled, typename T = void>
using disable_if = std::enable_if<!disabled, T> ;

template<bool disabled, typename T = void>
using disable_if_t = typename disable_if<disabled, T>::type ;

template<bool enabled>
using instance_if_t = std::enable_if_t<enabled, Instantiate> ;

/***************************************************************************//**
 Utility metafunction that maps a sequence of any types to the type specified
 as its first argument.

 This metafunction is like void_t used in template metaprogramming to detect
 ill-formed types in SFINAE context. It differs from std::void_t in that it maps
 to some specified type intead of void, facilitating specification of function
 argument and/or type types.
*******************************************************************************/
template<typename T, typename...> using type_t = T ;

/*******************************************************************************
 Template compile-time logic operations.
*******************************************************************************/
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

/*******************************************************************************
 Folding, until C++17 is the baseline standard
*******************************************************************************/
template<typename F, typename T>
constexpr inline T fold_left(F &&, T a1) { return a1 ; }

template<typename F, typename T, typename... TN>
constexpr inline T fold_left(F &&monoid, T a1, T a2,  TN ...aN)
{
   return fold_left<F, T>(std::forward<F>(monoid), std::forward<F>(monoid)(a1, a2), aN...) ;
}

template<typename T>
constexpr inline T fold_bitor(T a1) { return a1 ; }

template<typename T, typename... TN>
constexpr inline T fold_bitor(T a1, T a2, TN ...aN)
{
   return fold_bitor<T>(a1 | a2, aN...) ;
}

/***************************************************************************//**
 Creates unique type from another type.

 This allows to completely separate otherwise compatible types (e.g. pointer to derived
 and pointer to base, etc.)
*******************************************************************************/
template<typename T> struct identity_type { typedef T type ; } ;

/***************************************************************************//**
 Transfer cv-qualifiers of the source type to the target type.
 So, e.g.
     - `transfer_cv_t<const int, double>` is `const double`
     - `transfer_cv_t<const int, volatile double>` is `const volatile double`
     - `transfer_cv_t<const volatile int, double>` is `const volatile double`
     - `transfer_cv_t<const volatile int, volatile double>` is `const volatile double`

 @Note `transfer_cv_t<const int, double*>` is `double* const`, _not_ 'const double*'
*******************************************************************************/
/**@{*/
template<typename S, typename T>
struct transfer_cv { typedef T type ; } ;

template<typename S, typename T>
struct transfer_cv<const S, T> : transfer_cv<S, const T> {} ;

template<typename S, typename T>
struct transfer_cv<volatile S, T> : transfer_cv<S, volatile T> {} ;

template<typename S, typename T>
struct transfer_cv<const volatile S, T> : transfer_cv<S, const volatile T> {} ;
/**@}*/

template<typename S, typename T>
using transfer_cv_t = typename transfer_cv<S,T>::type ;

/***************************************************************************//**
 Provides globally placed default-constructed value of its parameter type.
*******************************************************************************/
template<typename T>
struct default_constructed {
      typedef T type ;
      static const type value ;
} ;

template<typename T>
const T default_constructed<T>::value = {} ;

/***************************************************************************//**
 Callable object to construct an object of its template parameter type.
*******************************************************************************/
template<typename T>
struct make {
      typedef T type ;

      template<typename... Args>
      type operator() (Args && ...args) const
      {
         return type(std::forward<Args>(args)...) ;
      }
} ;

/*******************************************************************************
 Type testers
*******************************************************************************/
template<typename Base, typename Derived>
struct is_base_of_strict :
         bool_constant<!(std::is_same_v<Base, Derived> ||
                         std::is_same_v<const volatile Base*, const volatile void*>) &&
                         std::is_convertible_v<const volatile Derived*, const volatile Base*>> {} ;

template<typename T, typename U>
using is_same_unqualified = std::is_same<std::remove_cv_t<T>, std::remove_cv_t<U>> ;

template<typename To, typename... From>
struct is_all_convertible : std::true_type {} ;

template<typename To, typename F1, typename... Fn>
struct is_all_convertible<To, F1, Fn...> :
   std::bool_constant<(std::is_convertible<F1, To>::value && is_all_convertible<To, Fn...>::value)>
{} ;

template<typename T, typename To, typename... From>
struct if_all_convertible : std::enable_if<is_all_convertible<To, From...>::value, T> {} ;

template<typename T, typename To, typename... From>
using if_all_convertible_t = typename if_all_convertible<T, To, From...>::type ;

template<typename T, typename... Types>
struct is_one_of : std::true_type {} ;

template<typename T, typename T1, typename... Tn>
struct is_one_of<T, T1, Tn...> :
   std::bool_constant<(std::is_same_v<T, T1> || is_one_of<T, Tn...>::value)>
{} ;

template<typename T, typename... Types>
constexpr bool is_one_of_v = is_one_of<T, Types...>::value ;

template<typename To, typename... From>
constexpr bool is_all_convertible_v = is_all_convertible<To, From...>::value ;

/***************************************************************************//**
 Check if the type can be trivially swapped, i.e. by simply swapping raw memory
 contents.
*******************************************************************************/
template<typename T>
struct is_trivially_swappable :
         std::is_trivially_copyable<T>
{} ;

template<typename T1, typename T2>
struct is_trivially_swappable<std::pair<T1, T2>> :
         ct_and<is_trivially_swappable<T1>, is_trivially_swappable<T2>>
{} ;

/***************************************************************************//**
 Type trait for types that allow memcpy()/memmove() construction and assignment.

 This is the equivalent of `std::is_trivially_copyable` - and, by default,
 is implemented as `std::is_trivially_copyable` - except that it can be overloaded
 to return `true` for types that _are_, in fact, trivially copyable, but for which
 `std::is_trivially_copyable` returns `false` due to collateral restrictions,
 e.g. `std::pair<int,unsigned>`
*******************************************************************************/
template<typename T>
struct is_memmovable : std::bool_constant<std::is_trivially_copyable<T>::value> {} ;

template<typename T, typename U>
struct is_memmovable<std::pair<T, U>> : ct_and<is_memmovable<T>, is_memmovable<U> > {} ;

template<typename T>
constexpr bool is_trivially_swappable_v = is_trivially_swappable<T>::value ;

template<typename T>
constexpr bool is_memmovable_v = is_memmovable<T>::value ;

/*******************************************************************************
 Parameter type
*******************************************************************************/
template<typename T>
using lvref_t = typename std::add_lvalue_reference<T>::type ;

template<typename T>
using clvref_t = lvref_t<typename std::add_const<T>::type> ;

template<typename T>
using parmtype_t = std::conditional_t<std::is_scalar<std::decay_t<T>>::value,
                                      std::decay_t<T>, clvref_t<T>> ;

template<typename T>
inline std::conditional_t<std::is_scalar<std::decay_t<T>>::value,
                          std::decay_t<const T>,
                          std::reference_wrapper<const T>>
inparm(const T &v) { return v ; }

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
using valtype_t = std::remove_cvref_t<T> ;

template<typename T, typename... A>
struct rebind___t {
      template<template<typename...> class Template, typename A1, typename... Args>
      static Template<T, A...> eval(Template<A1, Args...> *) ;
} ;

template<typename C, typename T, typename... A>
using rebind_t = decltype(rebind___t<T, A...>::eval(autoval<C *>())) ;

template<typename Functor, typename Argtype, template<class> class Default>
struct select_functor {
      typedef Functor type ;
} ;

template<typename Argtype, template<class> class Default>
struct select_functor<void, Argtype, Default> {
      typedef Default<Argtype> type ;
} ;

template<typename Functor, typename Argtype, template<class> class Default>
using select_functor_t = typename select_functor<Functor, Argtype, Default>::type ;

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

/***************************************************************************//**
 Metafunction: is the member of a derived class is derived from the base class
 or overloaded in the derived class.

 @return std::integral_constant<bool,true> if derived, std::integral_constant<bool,false>
 if overloaded.
*******************************************************************************/
template<typename Base, typename Member>
constexpr decltype(detail::is_derived_mem_<Base>(std::declval<Member>()))
is_derived_mem(Member &&) { return {} ; }

/***************************************************************************//**
 Uniform pair (i.e. pair<T,T>)
*******************************************************************************/
template<typename T>
using unipair = std::pair<T, T> ;

/***************************************************************************//**
 Count types that satisfy a meta-predicate in the arguments pack
*******************************************************************************/
template<template<typename> class Predicate, typename... Types>
struct count_types_if ;

template<template<typename> class F>
struct count_types_if<F> : std::integral_constant<int, 0> {} ;

template<template<typename> class F, typename H, typename... T>
struct count_types_if<F, H, T...> :
         std::integral_constant<int, ((int)!!F<H>::value + count_types_if<F, T...>::value)> {} ;

/****************************************************************************//**
 Convert an enum value into the value of underlying integral type, or pass
 through the paramter if it is already of interal type.
*******************************************************************************/
template<typename T, int mode = (int)std::is_enum<T>::value - (int)std::is_integral<T>::value>
struct underlying_integral ;

template<typename E>
struct underlying_integral<E, 1> : std::underlying_type<E> {} ;

template<typename I>
struct underlying_integral<I, -1> { typedef I type ; } ;

template<typename T>
using underlying_integral_t = typename underlying_integral<T>::type ;

template<typename E>
constexpr inline std::enable_if_t<std::is_enum<E>::value, underlying_integral_t<E>>
underlying_int(E value)
{
   return static_cast<std::underlying_type_t<E>>(value) ;
}

template<typename I>
constexpr inline std::enable_if_t<std::is_integral<I>::value, I>
underlying_int(I value)
{
   return value ;
}

} // end of namespace pcomn

#endif /* __PCOMN_META_H */
