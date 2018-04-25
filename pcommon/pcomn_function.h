/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_FUNCTION_H
#define __PCOMN_FUNCTION_H
/*******************************************************************************
 FILE         :   pcomn_function.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2017. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   STL-like additional functors

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   10 May 2000
*******************************************************************************/
#include <pcommon.h>
#include <pcomn_meta.h>
#include <pcomn_macros.h>

#include <functional>
#include <tuple>

namespace pcomn {

/***************************************************************************//**
 Functor to pass through its argument; effectively calls std::forward<T>.
*******************************************************************************/
struct identity {
      template<typename T>
      T &&operator() (T &&t) const { return std::forward<T>(t) ; }
} ;

/***************************************************************************//**
 Converter functor
*******************************************************************************/
template<typename T>
struct cast_to {
      template<typename S>
      T operator() (S &&source) const { return static_cast<T>(std::forward<S>(source)) ; }
} ;

template<size_t n>
struct select {
      template<typename T>
      auto operator()(T &&x) const -> decltype(std::get<n>(std::forward<T>(x)))
      {
         return std::get<n>(std::forward<T>(x)) ;
      }
      template<typename T>
      auto operator()(T *x) const -> decltype(std::get<n>(*x))
      {
         return std::get<n>(*x) ;
      }
} ;

typedef select<0> select1st ;
typedef select<1> select2nd ;

/*******************************************************************************
 Predicate functors.
*******************************************************************************/
template<typename A1>
using unary_predicate = std::unary_function<A1, bool> ;
template<typename A1, typename A2 = A1>
using binary_predicate = std::binary_function<A1, A2, bool> ;

/// Predicate adaptor, the opposite to std::unary_negate.
/// Reevaluates the result of Predicate (which must not necessary return bool value)
/// as a boolean value.
template<typename Predicate>
struct unary_affirm : unary_predicate<typename Predicate::argument> {
      explicit unary_affirm(const Predicate& x) : pred(x) {}
      bool operator() (const typename Predicate::argument_type& x) const { return !!pred(x) ; }

   protected:
      Predicate pred ;
} ;

/// Predicate adaptor, the opposite to std::binary_negate.
/// Reevaluates the result of Predicate (which must not necessary return bool value)
/// as a boolean value.
template<typename Predicate>
struct binary_affirm : binary_predicate<typename Predicate::first_argument_type,
                                        typename Predicate::second_argument_type>
{
      explicit binary_affirm(const Predicate& x) : pred(x) {}

      bool operator() (const typename Predicate::first_argument_type& a1,
                       const typename Predicate::second_argument_type& a2) const
      {
         return !!pred(a1, a2) ;
      }

   protected:
      Predicate pred ;
} ;

template <class Predicate>
inline unary_affirm<Predicate> yes1(const Predicate& pred)
{
    return unary_affirm<Predicate>(pred) ;
}

template <class Predicate>
inline binary_affirm<Predicate> yes2(const Predicate& pred)
{
    return binary_affirm<Predicate>(pred) ;
}

/*******************************************************************************
 Functors, destroying objects.
*******************************************************************************/
template<class T>
struct destroy_object : std::unary_function<T &, T *>
{
      T *operator() (T &object) const
      {
         (&object)->~T() ;
         return &object ;
      }
} ;

/*******************************************************************************
 Dereference
*******************************************************************************/
template<typename T> struct deref_traits ;

template<typename T>
struct is_dereferenceable : has_type_type<deref_traits<T> > {} ;

namespace detail {
template<typename T, bool = has_element_type<T>::value> struct deref_traits_{} ;
template<typename T> struct deref_traits_<T, true> { typedef typename T::element_type type ; } ;

template<typename T, bool = is_dereferenceable<T>::value>
struct deref_ : std::unary_function<T, typename deref_traits<T>::type &> {
      typename deref_traits<T>::type &operator()(const T &p) const { return *p ; }
} ;
template<typename T>
struct deref_<T, false> : std::unary_function<T, T &> {
      T &operator()(T &r) const { return r ; }
} ;
}

template<typename T> struct deref_traits : detail::deref_traits_<T> {} ;
template<typename T> struct deref_traits<T *> { typedef T type ; } ;

/******************************************************************************/
/** Functor that dereferences pointers.

 Convenient to use in algorithms working on sequences of pointers (both plain-
 and smartpointers).
*******************************************************************************/
template<typename T>
struct deref : detail::deref_<T> {} ;

template<typename T>
inline typename deref_traits<T>::type &dereference(T &v)
{
   return deref<T>()(v) ;
}

template<typename T>
inline disable_if_t<is_dereferenceable<T>::value, T &> &dereference(T &v)
{
   return deref<T>()(v) ;
}

/*******************************************************************************

*******************************************************************************/
template<typename T>
using cref_wrapper = std::reference_wrapper<const T> ;

template<typename T>
using ref_wrapper = std::reference_wrapper<std::remove_const_t<T> > ;

/// Checks if the passed type is an instance of std::reference_wrapper template.
///
/// If T is std::reference_wrapper<U>, possibly cv-qualified, provides the member
/// constant value equal true and member typedef element_type equal to U.
/// For any other type, value is false and element_type is not defined.
template<typename>
struct is_reference_wrapper : std::false_type {} ;

template<typename U>
struct is_reference_wrapper<std::reference_wrapper<U>> : std::true_type {
      typedef U element_type ;
} ;

/*******************************************************************************
 container_item
*******************************************************************************/
template<class Container, typename Key>
struct container_item_base :
   public std::unary_function<Key,
                              typename std::conditional<std::is_const<Container>::value,
                                                        typename Container::const_reference,
                                                        typename Container::reference>::type>
{} ;

template<class Container, typename Key>
struct container_item : public container_item_base<Container, Key> {
      container_item(Container &container) : _container(&container) {}

      typename container_item_base<Container, Key>::result_type
      operator()(const Key &key) const
      {
         return (*_container)[key] ;
      }
   private:
      Container *_container ;
} ;

/******************************************************************************/
/** Functor which inserts its call argument into the container passed to the
 functor's constructor.

*******************************************************************************/
template<typename Container>
struct container_inserter {
      container_inserter(Container &container) :
         _contents(NULL),
         _container(container)
      {}

      container_inserter(Container *container = NULL) :
         _contents(container ? container : new Container),
         _container(*_contents)
      {}

      ~container_inserter() { delete _contents ; }

      Container &container() const { return _container ; }

      template<typename T>
      const container_inserter &operator()(T &&item) const
      {
         container().insert(container().end(), std::forward<T>(item)) ;
         return *this ;
      }

      const container_inserter &operator()() const
      {
         container().insert(container().end(), typename Container::value_type()) ;
         return *this ;
      }

   private:
      Container *_contents ;
      Container &_container ;
} ;

/*******************************************************************************
 clone
*******************************************************************************/
template<typename T>
inline T *clone_object(const T *obj)
{
   return obj ? new T(*obj) : nullptr ;
}

struct clone {
      template<typename T>
      T *operator()(const T *source) const
      {
         return clone_object(source) ;
      }
} ;

/*******************************************************************************
 mem_data, mem_data_ref
*******************************************************************************/
template<typename R, typename T>
struct mem_data_ref_t : public std::unary_function<T, R> {
      mem_data_ref_t(R T::*p) : _p(p) {}
      R operator() (const T &t) const { return t.*_p ; }
   private:
      R T::*_p ;
} ;

template<typename R, typename T>
struct mem_data_ptr_t : public std::unary_function<T *, R> {
      mem_data_ptr_t(R T::*p) : _p(p) {}
      R operator() (const T *t) const { return !t ? R() : t->*_p ; }
   private:
      R T::*_p ;
} ;

template<typename T, typename R>
mem_data_ref_t<R, T> mem_data(R T::*p) { return mem_data_ref_t<R, T>(p) ; }

template<typename T, typename R>
mem_data_ptr_t<R, T *> mem_data_ptr(R T::*p) { return mem_data_ptr_t<R, T *>(p) ; }

/*******************************************************************************
 make_function
 bind_thisptr
*******************************************************************************/
template<typename R, typename T>
std::function<R()> bind_thisptr(R (T::*memfn)(), T *thisptr)
{
   return {std::bind(memfn, thisptr)} ;
}
template<typename R, typename T>
std::function<R()> bind_thisptr(R (T::*memfn)() const, const T *thisptr)
{
   return {std::bind(memfn, thisptr)} ;
}

template<typename R, typename T>
std::function<R()> bind_thisptr(R (T::element_type::*memfn)(), T thisptr)
{
   return {std::bind(memfn, thisptr)} ;
}

template<typename R, typename T>
std::function<R()> bind_thisptr(R (T::element_type::*memfn)() const, T thisptr)
{
   return {std::bind(memfn, thisptr)} ;
}

#define P_PLACEHOLDER_(num, ...)  std::placeholders::_##num
#define P_PLACELIST_(nargs) P_FOR(nargs, P_PLACEHOLDER_)
#define P_BIND_THISPTR_(argc)                                           \
   template<typename R, typename C, typename T, P_TARGLIST(argc, typename)> \
   std::enable_if_t<std::is_base_of<C, T>::value, std::function<R(P_TARGLIST(argc,))> > \
   bind_thisptr(R (C::*memfn)(P_TARGLIST(argc,)), T *thisptr)           \
   {                                                                    \
      return {std::bind(memfn, thisptr, P_PLACELIST_(argc))} ;          \
   }                                                                    \
   template<typename R, typename C, typename T, P_TARGLIST(argc, typename)> \
   std::enable_if_t<std::is_base_of<C, T>::value, std::function<R(P_TARGLIST(argc,))> > \
   bind_thisptr(R (C::*memfn)(P_TARGLIST(argc,)) const, const T *thisptr) \
   {                                                                    \
      return {std::bind(memfn, thisptr, P_PLACELIST_(argc))} ;          \
   }                                                                    \
   template<typename R, typename T, P_TARGLIST(argc, typename)>         \
   std::function<R(P_TARGLIST(argc,))>                                  \
   bind_thisptr(R (T::element_type::*memfn)(P_TARGLIST(argc,)), T thisptr) \
   {                                                                    \
      return {std::bind(memfn, std::move(thisptr), P_PLACELIST_(argc))} ; \
   }                                                                    \
   template<typename R, typename T, P_TARGLIST(argc, typename)>         \
   std::function<R(P_TARGLIST(argc,))>                                  \
   bind_thisptr(R (T::element_type::*memfn)(P_TARGLIST(argc,)) const, T thisptr) \
   {                                                                    \
      return {std::bind(memfn, std::move(thisptr), P_PLACELIST_(argc))} ; \
   }

P_BIND_THISPTR_(1) ;
P_BIND_THISPTR_(2) ;
P_BIND_THISPTR_(3) ;
P_BIND_THISPTR_(4) ;
P_BIND_THISPTR_(5) ;
P_BIND_THISPTR_(6) ;
P_BIND_THISPTR_(7) ;
P_BIND_THISPTR_(8) ;

#undef P_PLACEHOLDER_
#undef P_PLACELIST_
#undef P_BIND_THISPTR_

template<typename> struct function_type ;

template<typename Ret, typename Class, typename... Args>
struct function_type<Ret(Class::*)(Args...) const>
{
    typedef std::function<Ret(Args...)> type ;
} ;

template<typename Ret, typename Class, typename... Args>
struct function_type<Ret(Class::*)(Args...)>
{
    typedef std::function<Ret(Args...)> type ;
} ;

template<typename Callable>
using function_type_t = typename function_type<decltype(&Callable::operator())>::type ;

/// Make std::function from a function pointer.
template<typename R, typename... Args>
std::function<R(Args...)> make_function(R (*fn)(Args...))
{
   return {fn} ;
}

/// Make std::function from lambda or any object with `operator()`.
/// @note Does not work with objects with several overloaded `operator()`.
template<typename Callable>
function_type_t<Callable> make_function(Callable &&fn)
{
   return {std::forward<Callable>(fn)} ;
}

template<typename, typename>
struct is_callable : bool_constant<false> {} ;

template<typename T, typename R, typename...Args>
struct is_callable<T, R(Args...)> :
         bool_constant<(!is_same_unqualified<T, nullptr_t>::value &&
                        std::is_constructible<std::function<R(Args...)>, T>::value)> {} ;

} // end of namespace pcomn


#ifndef PCOMN_STL_CXX17

namespace std {

template<class Predicate>
inline constexpr unary_negate<Predicate> not_fn(Predicate &&pred)
{
   return not1(forward<Predicate>(pred)) ;
}

} // end of namespace std

#endif /* PCOMN_STL_CXX17 */

#endif /* __PCOMN_FUNCTION_H */
