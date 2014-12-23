/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_META_H
#define __PCOMN_META_H
/*******************************************************************************
 FILE         :   pcomn_meta.h
 COPYRIGHT    :   Yakov Markovitch, 2006-2014. All rights reserved.
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
#include <type_traits>

/*******************************************************************************
 C++14 definitions for C++11 compiler
*******************************************************************************/
#if __cplusplus <= 201103

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

template<bool B, typename T>
using enable_if_t = typename enable_if<B, T>::type ;
template<bool B, typename T, typename F>
using conditional_t = typename conditional<B, T, F>::type ;

template<typename T>
using result_of_t = typename result_of<T>::type ;

}

#endif /* __cplusplus > 201103 */

namespace pcomn {

template<bool v>
using bool_constant = std::integral_constant<bool, v> ;

template<int v>
using int_constant = std::integral_constant<int, v> ;

/******************************************************************************/
/** disable_if is a complement to std::enable_if
*******************************************************************************/
template<bool disabled, typename T>
using disable_if = std::enable_if<!disabled, T> ;

template<bool disabled, typename T>
using disable_if_t = typename disable_if<disabled, T>::type ;

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
const T default_constructed<T>::value ;

/*******************************************************************************
 Type testers
*******************************************************************************/
template<typename Base, typename Derived>
struct is_base_of_strict :
         bool_constant<!(std::is_same<Base, Derived>::value ||
                         std::is_same<const volatile Base*, const volatile void*>::value) &&
                         std::is_convertible<const volatile Derived*, const volatile Base*>::value> {} ;

/*******************************************************************************
 Parameter type
*******************************************************************************/
template<typename T>
using lvref_t = typename std::add_lvalue_reference<T>::type ;

template<typename T>
using clvref_t = lvref_t<typename std::add_const<T>::type> ;

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

/*******************************************************************************
                     union max_align
*******************************************************************************/
union max_align
{
      void (*_function_ptr)() ;
      int (max_align::*_member_ptr) ;
      int (max_align::*_member_function_ptr)() ;
      char        _char ;
      short       _short ;
      int         _int ;
      long        _long ;
      longlong_t  _long_long ;
      float       _float ;
      double      _double ;
      long double _ldouble ;
      void *      _ptr ;
} ;

const size_t max_alignment = std::alignment_of<max_align>::value ;

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
   using has_##type = decltype(detail::has_##type##_test<T>(0))

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
#if PCOMN_WORKAROUND(__GNUC_VER__, < 500)
namespace std {
template<typename T, typename U>
struct has_trivial_copy_assign<std::pair<T, U> > :
         pcomn::ct_and<std::has_trivial_copy_assign<T>, std::has_trivial_copy_assign<U> > {} ;

template<typename T>
struct is_trivially_copyable :
         pcomn::ct_and<std::has_trivial_copy_constructor<T>, std::has_trivial_copy_assign<T> > {} ;
}
#endif // PCOMN_WORKAROUND(__GNUC_VER__, < 500)

#endif /* __PCOMN_META_H */
