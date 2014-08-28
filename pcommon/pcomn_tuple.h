/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_TUPLE_H
#define __PCOMN_TUPLE_H
/*******************************************************************************
 FILE         :   pcomn_tuple.h
 COPYRIGHT    :   Yakov Markovitch, 2006-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   std::tuple manipulation routines.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   8 Dec 2006
*******************************************************************************/
#include <pcomn_meta.h>
#include <pcomn_macros.h>

#include <tuple>
#include <utility>
#include <functional>

// GNU C++ 4.3 and higher uses variadic templates for tuples definition
#if PCOMN_WORKAROUND(__GNUC_VER__, >= 430)
#define PCOMN_TUPLE_ARGS  typename... __Ts
#define PCOMN_TUPLE_PARAMS __Ts...
#else
#define PCOMN_TUPLE_ARGS   P_TARGLIST(10, typename)
#define PCOMN_TUPLE_PARAMS P_TARGLIST(10)
#endif

namespace pcomn {

/******************************************************************************/
/** Visit every item of any object compatible with std::get<n> function (this
 includes at least std::tuple, std::pair, and std::array).
*******************************************************************************/
template<unsigned tuplesz>
struct for_each_visit {
      template<typename T, typename F>
      static void visit_lvalue(T &value, F &visitor)
      {
         for_each_visit<tuplesz-1>::visit_lvalue(value, visitor) ;
         visitor(std::get<tuplesz-1>(value)) ;
      }

      template<typename T, typename F>
      static void visit(T &value, const F &visitor)
      {
         visit_lvalue(value, visitor) ;
      }
} ;

/******************************************************************************/
/** @cond detail
 Explicit specialization of for_each_visit for size 0; serves as a sentry in for
 recursive for_each_visit definition.
*******************************************************************************/
template<> struct for_each_visit<0> {
      template<typename T, typename F>
      static void visit_lvalue(T &, F &) {}
      template<typename T, typename F>
      static void visit(T &, const F &) {}
} ;
/// @endcond

/******************************************************************************/
/** Statically apply a functor object to every item of any object compatible with
 std::get<n> function and std::tuple_size<T> class (this includes at least
 std::tuple, std::pair, and std::array).
*******************************************************************************/
template<class Tuple>
struct tuple_for_each : for_each_visit<std::tuple_size<typename std::remove_const<Tuple>::type>::value> {
      template<typename F>
      static void apply(Tuple &value, const F &visitor)
      {
         applier::visit_lvalue(value, visitor) ;
      }
      template<typename F>
      static void apply_lvalue(Tuple &value, F &visitor)
      {
         applier::visit_lvalue(value, visitor) ;
      }
   private:
      typedef for_each_visit<std::tuple_size<typename std::remove_const<Tuple>::type>::value> applier ;
} ;

// Prevent compiler from emitting a warning about return type having a const qualifier:
// we actually need such qualifier for valid template instantiation while decaying pointers
GCC_IGNORE_WARNING(ignored-qualifiers)

template<typename T>
inline T &decay(T &value) { return value ; }

template<typename T, size_t n>
inline T * const decay(T (&value)[n]) { return value + 0 ; }

template<typename T>
inline T &decay(const std::reference_wrapper<T> &value) { return value ; }

GCC_RESTORE_WARNING(ignored-qualifiers)

template<typename T>
struct decay_argtype { typedef T &type ; } ;

template<typename T, size_t n>
struct decay_argtype<T[n]> { typedef T *type ; } ;

template<typename T>
struct decay_argtype<const std::reference_wrapper<T> > { typedef T &type ; } ;

/*******************************************************************************
 const_tie
*******************************************************************************/
inline std::tuple<> const_tie() { return std::tuple<>() ; }

/// @internal
#define DEFINE_CONST_TIE(count)                             \
   template<P_TARGLIST(count, typename)>                    \
   inline std::tuple<P_TPARMLIST(count, _P_DECAY_ARG_TYPE)> \
   const_tie(P_REF_ARGLIST(count, const))                   \
   { return std::tie(P_PARMLIST(count, pcomn::decay)) ; }

/// @internal
#define _P_DECAY_ARG_TYPE(P) typename pcomn::decay_argtype<const P >::type

DEFINE_CONST_TIE(1) ;
DEFINE_CONST_TIE(2) ;
DEFINE_CONST_TIE(3) ;
DEFINE_CONST_TIE(4) ;
DEFINE_CONST_TIE(5) ;
DEFINE_CONST_TIE(6) ;
DEFINE_CONST_TIE(7) ;
DEFINE_CONST_TIE(8) ;
DEFINE_CONST_TIE(9) ;
DEFINE_CONST_TIE(10) ;

#undef _P_DECAY_ARG_TYPE
#undef DEFINE_CONST_TIE

} // end of namespace pcomn


#endif /* __PCOMN_TUPLE_H */
