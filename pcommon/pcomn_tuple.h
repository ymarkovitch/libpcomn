/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_TUPLE_H
#define __PCOMN_TUPLE_H
/*******************************************************************************
 FILE         :   pcomn_tuple.h
 COPYRIGHT    :   Yakov Markovitch, 2006-2018. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   std::tuple manipulation routines.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   8 Dec 2006
*******************************************************************************/
/** @file
  Tuple and typelist manipulation routines.
*******************************************************************************/
#include <pcomn_meta.h>
#include <pcomn_macros.h>

#include <tuple>
#include <utility>
#include <functional>

namespace pcomn {

/***************************************************************************//**
 Template typedefs for 0-,1-,3-,4-item tuples.
*******************************************************************************/
/**@{*/
using nulltuple = std::tuple<> ;

template<typename T>
using single = std::tuple<T> ;

template<typename A, typename B = A, typename C = B>
using triple = std::tuple<A, B, C> ;

template<typename A, typename B = A, typename C = B, typename D = C>
using quad = std::tuple<A, B, C, D> ;
/**@}*/

/***************************************************************************//**
 Typelists: type sequence tags.
*******************************************************************************/
template<typename... Types>
using tlist = std::tuple<Types...>* ;

using tnull = tlist<> ;

template<typename T>
using tsingle = tlist<T> ;

template<typename T1, typename T2 = T1>
using tpair = tlist<T1, T2> ;

template<typename... TLists>
using tlists_cat = std::add_pointer_t<decltype(std::tuple_cat(*std::declval<TLists>()...))> ;

/******************************************************************************/
/** Visit every item of any object compatible with std::get<n> function (this
 includes at least std::tuple, std::pair, and std::array).
*******************************************************************************/
template<unsigned tuplesz>
struct for_each_visit {
      template<typename T, typename F>
      static void visit(T &value, F &&visitor)
      {
         for_each_visit<tuplesz-1>::visit(value, std::forward<F>(visitor)) ;
         std::forward<F>(visitor)(std::get<tuplesz-1>(value)) ;
      }
} ;

/******************************************************************************/
/** @cond detail
 Explicit specialization of for_each_visit for size 0; serves as a sentry in for
 recursive for_each_visit definition.
*******************************************************************************/
template<> struct for_each_visit<0> {
      template<typename T, typename F>
      static void visit(T &, F &&) {}
} ;
/// @endcond

/***************************************************************************//**
 Statically apply a functor object to every item of any object compatible with
 std::get<n> function and std::tuple_size<T> class (this includes at least
 std::tuple, std::pair, and std::array).
*******************************************************************************/
template<typename Tuple>
struct tuple_for_each : for_each_visit<std::tuple_size<valtype_t<Tuple>>::value> {
      template<typename F>
      static void apply(Tuple &value, F &&visitor)
      {
         applier::visit(value, std::forward<F>(visitor)) ;
      }

   private:
      typedef for_each_visit<std::tuple_size<std::remove_const_t<Tuple>>::value> applier ;

   public:
      template<typename F>
      __deprecated("Please use tuple_for_each::apply")
      static void apply_lvalue(Tuple &value, F &&visitor)
      {
         apply(value, std::forward<F>(visitor)) ;
      }
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

GCC_DIAGNOSTIC_PUSH()
// Prevent compiler from emitting a warning about return type having a const qualifier:
// we actually need such qualifier for valid template instantiation while decaying pointers
GCC_IGNORE_WARNING(ignored-qualifiers)

template<typename T>
inline T &decay(T &value) { return value ; }

template<typename T, size_t n>
inline T * const decay(T (&value)[n]) { return value + 0 ; }

template<typename T>
inline T &decay(const std::reference_wrapper<T> &value) { return value ; }

GCC_DIAGNOSTIC_POP()

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
 Now mostly supeseeded with std::forward_as_tuple
*******************************************************************************/
inline std::tuple<> const_tie() { return std::tuple<>() ; }

template<typename... A>
inline std::tuple<decay_argtype_t<const A> ...>
const_tie(A&& ...args)
{
   return std::tie(pcomn::decay(args)...) ;
}

/***************************************************************************//**
 Checks whether this tuple precedes other in lexicographical order where tuple item
 comparison is specified by the ordering predicate.

 @note For empty tuples always returns `false'.
*******************************************************************************/
/**@{*/
template<typename Predicate, typename Tuple = void>
struct tuple_before {
      bool operator()(const Tuple &x, Tuple &y) const
      {
         return tuple_before<Predicate>()(x, y) ;
      }
} ;

template<typename Predicate>
struct tuple_before<Predicate, void> {

      template<typename T1, typename T2>
      bool operator()(const std::pair<T1,T2> &x, const std::pair<T1,T2> &y) const
      {
         constexpr const Predicate before ;
         return lt(x.first, y.first) || !lt(y.first, x.first) && lt(x.second, y.second) ;
      }

      template<typename...T>
      bool operator()(const std::tuple<T...> &x, const std::tuple<T...> &y) const
      {
         return tail_before(x, y, pcomn::size_constant<0>()) ;
      }

   private:
      template<typename...T>
      static constexpr bool tail_before(const std::tuple<T...> &,
                                        const std::tuple<T...> &,
                                        pcomn::size_constant<sizeof...(T)>)
      {
         return false ;
      }

      template<size_t i, typename...T>
      static bool tail_before(const std::tuple<T...> &x, const std::tuple<T...> &y,
                              pcomn::size_constant<i>)
      {
         constexpr const Predicate lt ;
         return
             lt(std::get<i>(x), std::get<i>(y)) ||
            !lt(std::get<i>(y), std::get<i>(x)) && tail_before(x, y, pcomn::size_constant<i+1>()) ;
      }
} ;
/**@}*/

/***************************************************************************//**
 Checks whether this tuple is equal to other in lexicographical order
 where tuple item comparison is specified by the ordering predicate.

 @note For empty tuples always returns `true'.
*******************************************************************************/
/**@{*/
template<typename Predicate, typename Tuple = void>
struct tuple_equal {
      bool operator()(const Tuple &x, Tuple &y) const
      {
         return tuple_equal<Predicate>()(x, y) ;
      }
} ;

template<typename Predicate>
struct tuple_equal<Predicate, void> {

      template<typename T1, typename T2>
      bool operator()(const std::pair<T1,T2> &x, const std::pair<T1,T2> &y) const
      {
         constexpr const Predicate eq ;
         return eq(x.first, y.first) && eq(x.second, y.second) ;
      }

   private:
      template<typename...T>
      static constexpr bool tail_equal(const std::tuple<T...> &, const std::tuple<T...> &,
                                       pcomn::size_constant<sizeof...(T)>)
      {
         return true ;
      }

      template<size_t i, typename...T>
      static bool tail_equal(const std::tuple<T...> &x, const std::tuple<T...> &y, pcomn::size_constant<i>)
      {
         return Predicate()(std::get<i>(x), std::get<i>(y)) && tail_equal(x, y, pcomn::size_constant<i+1>()) ;
      }
} ;
/**@}*/
/*******************************************************************************

*******************************************************************************/
namespace detail {
template<typename... Args>
inline constexpr ssize_t tuplesize_(std::tuple<Args...> *) { return std::tuple_size<std::tuple<Args...> >::value ; }
template<typename U, typename V>
inline constexpr ssize_t tuplesize_(std::pair<U, V> *) { return std::tuple_size<std::pair<U, V> >::value ; }
template<typename T, size_t n>
inline constexpr ssize_t tuplesize_(std::array<T, n> *) { return std::tuple_size<std::array<T, n> >::value ; }
inline constexpr ssize_t tuplesize_(void *) { return -1  ; }
}

/***************************************************************************//**
 tuplesize<T>() constexpr is like std::tuple_size<T>::value except for
 it is applicable to _any_ T and returns -1 for those T that std::tuple_size<T>
 is not applicable to.
*******************************************************************************/
template<typename T>
inline constexpr ssize_t tuplesize()
{
   return detail::tuplesize_((valtype_t<T> *)nullptr)  ;
}

template<typename T>
inline constexpr ssize_t tuplesize(T &&) { return tuplesize<T>() ; }

template<typename Pred, typename...T>
inline bool less_tuple(const std::tuple<T...> &x, const std::tuple<T...> &y, const Pred & = {})
{
   return std::less<std::tuple<T...>>()(x, y) ;
}

template<typename Pred, typename T1, typename T2>
inline bool less_tuple(const std::pair<T1,T2> &x, const std::pair<T1,T2> &y, const Pred & = {})
{
   return std::less<std::pair<T1,T2>>()(x, y) ;
}

template<typename Pred, typename...T>
inline bool equal_tuple(const std::tuple<T...> &x, const std::tuple<T...> &y, const Pred & = {})
{
   return std::equal_to<std::tuple<T...>>()(x, y) ;
}

template<typename Pred, typename T1, typename T2>
inline bool equal_tuple(const std::pair<T1,T2> &x, const std::pair<T1,T2> &y, const Pred & = {})
{
   return std::equal_to<std::pair<T1,T2>>()(x, y) ;
}

/*******************************************************************************

*******************************************************************************/
namespace detail {
struct out_element {
      template<typename T>
      void operator()(const T &v) { if (next) os << ',' ; else next = true ; os << v ; }
      std::ostream &os ;
      bool next ;
} ;
}

} // end of namespace pcomn

namespace std {

template<typename... T>
__noinline std::ostream &operator<<(std::ostream &os, const std::tuple<T...> &v)
{
   pcomn::tuple_for_each<const std::tuple<T...> >::apply(v, (pcomn::detail::out_element{ os << '{', false })) ;
   return os << '}' ;
}

} // end of namespace std

#endif /* __PCOMN_TUPLE_H */
