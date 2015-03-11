/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_TUPLE_H
#define __PCOMN_TUPLE_H
/*******************************************************************************
 FILE         :   pcomn_tuple.h
 COPYRIGHT    :   Yakov Markovitch, 2006-2015. All rights reserved.
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

/*******************************************************************************
 C++14 definitions for C++11 compiler
*******************************************************************************/
#if __cplusplus <= 201103

namespace std {
template<std::size_t I, class Tuple>
using tuple_element_t = typename tuple_element<I, Tuple>::type ;
}

#endif /* __cplusplus > 201103 */

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
struct tuple_for_each : for_each_visit<std::tuple_size<std::remove_const_t<Tuple> >::value> {
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
      typedef for_each_visit<std::tuple_size<std::remove_const_t<Tuple> >::value> applier ;
} ;

/******************************************************************************/
/** Visit every item of any object compatible with std::get<n> function (this
 includes at least std::tuple, std::pair, and std::array).
*******************************************************************************/
template<unsigned tuplesz>
struct for_each_apply {
      template<typename F, typename... S>
      static void apply(F &&visitor, S &&...seq)
      {
         for_each_apply<tuplesz-1>::apply(std::forward<F>(visitor), std::forward<S>(seq)...) ;
         std::forward<F>(visitor)(std::get<tuplesz-1>(std::forward<S>(seq))...) ;
      }
} ;

/******************************************************************************/
/** @cond detail
 Explicit specialization of for_each_apply for size 0; serves as a sentry in for
 recursive for_each_apply definition.
*******************************************************************************/
template<> struct for_each_apply<0> {
      template<typename F, typename... S>
      static void apply(F &&, S &&...) {}
} ;
/// @endcond

/******************************************************************************/
/** Apply a functor object to every item of a sequence of tuples, where the
 i-th tuple contains the i-th element from each of the argument sequences.

 The number of vizited tuples is the min(tuple_size<sequence>...) (i.e. the shortest
 from @a sequences)
*******************************************************************************/
template<typename F, typename... Tuples>
void tuple_zip(F &&visitor, Tuples &&...sequences)
{
   PCOMN_STATIC_CHECK(sizeof...(Tuples) > 0) ;
   typedef for_each_apply<ct_min<size_t, std::tuple_size<valtype_t<Tuples> >::value...>::value> applier ;
   applier::apply(std::forward<F>(visitor), std::forward<Tuples>(sequences)...) ;
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

template<typename T>
using decay_argtype_t = typename decay_argtype<T>::type ;

/*******************************************************************************
 const_tie
*******************************************************************************/
inline std::tuple<> const_tie() { return std::tuple<>() ; }

template<typename... A>
inline std::tuple<decay_argtype_t<const A> ...>
const_tie(A&& ...args)
{
   return std::tie(pcomn::decay(std::forward<A>(args))...) ;
}

/*******************************************************************************

*******************************************************************************/
namespace detail {
template<typename T, bool> struct tuplesize_ : std::tuple_size<std::remove_cv_t<T> > {} ;
template<typename T> struct tuplesize_<T, false> : std::integral_constant<ssize_t, -1> {} ;
}

template<typename T>
inline constexpr ssize_t tuplesize()
{
   return detail::tuplesize_<T, std::is_destructible<std::tuple_size<T> >::value>::value ;
}

template<typename T>
inline constexpr ssize_t tuplesize(const T &) { return tuplesize<T>() ; }

} // end of namespace pcomn

// We need std namespace to ensure proper Koenig lookup
namespace std {

namespace detail {
struct out_element {
      template<typename T>
      void operator()(const T &v)
      {
         if (next)
            os << ',' ;
         else
            next = true ;
         os << v ;
      }

      std::ostream &os ;
      bool next ;
} ;
}

template<typename... T>
std::ostream &operator<<(std::ostream &os, const std::tuple<T...> &v)
{
   detail::out_element out_element = { os << '{', false } ;
   pcomn::tuple_for_each<const std::tuple<T...> >::apply_lvalue(v, out_element) ;

   return os << '}' ;
}
} // end of namespace std

#endif /* __PCOMN_TUPLE_H */
