/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_FUNCTION_H
#define __PCOMN_FUNCTION_H
/*******************************************************************************
 FILE         :   pcomn_function.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   STL-like additional functors

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   10 May 2000
*******************************************************************************/
#include <pcommon.h>
#include <pcomn_meta.h>
#include <pcomn_macros.h>

#include <stdexcept>
#include <functional>
#include <functional>

#if defined(PCOMN_COMPILER_GNU)

#include <ext/functional>

namespace pcomn {

using __gnu_cxx::identity ;
using __gnu_cxx::select1st ;
using __gnu_cxx::select2nd ;

} // end of namespace pcomn

#else

namespace pcomn {

template<class T>
struct identity : public unary_function<T, T> {
   const T &operator () (const T &t) const { return t ; }
} ;

template<class Pair>
struct select1st : public unary_function<Pair, typename Pair::first_type> {
  const typename Pair::first_type &operator()(const Pair& x) const
  {
    return x.first ;
  }
} ;

template<class Pair>
struct select2nd : public unary_function<Pair, typename Pair::second_type> {
  const typename Pair::second_type &operator()(const Pair& x) const
  {
    return x.second;
  }
} ;

} // end of namespace pcomn

#endif // PCOMN_COMPILER_GNU

namespace pcomn {

/// Get integer value of flags() method of the class.
template<class T>
struct get_flags : std::unary_function<T, bigflag_t>
{
   bigflag_t operator() (const T &t) const { return t.flags() ; }
} ;

/*******************************************************************************

*******************************************************************************/

template<class T1, class T2, class Cvt, class Cmp>
class symm_comp {
   public:
      bool operator () (const T1 &t1, const T2 &t2) const
      {
         return Cmp() (Cvt()(t1), t2) ;
      }

      bool operator () (const T2 &t1, const T1 &t2) const
      {
         return Cmp() (t1, Cvt()(t2)) ;
      }
} ;

/*******************************************************************************
 Predicate functors.
*******************************************************************************/
/// Logical "yes": I've always missed this in STL!
template<class T>
struct logical_yes : std::unary_function<T, bool>
{
   bool operator() (const T &object) const
   {
      return !!object ;
   }
} ;

/// Predicate adaptor, the opposite to std::unary_negate.
/// Reevaluates the result of Predicate (which must not necessary return bool value)
/// as a boolean value.
template<class Predicate>
struct unary_affirm : std::unary_function<typename Predicate::argument_type, bool>
{
      explicit unary_affirm(const Predicate& x) :
         pred(x)
      {}

      bool operator() (const typename Predicate::argument_type& x) const
      {
         return !!pred(x) ;
      }

   protected:
      Predicate pred ;
} ;

/// Predicate adaptor, the opposite to std::binary_negate.
/// Reevaluates the result of Predicate (which must not necessary return bool value)
/// as a boolean value.
template<class Predicate>
struct binary_affirm : std::binary_function<typename Predicate::first_argument_type,
                                            typename Predicate::second_argument_type,
                                            bool>
{
      explicit binary_affirm(const Predicate& x) :
         pred(x)
      {}

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
 Converter functors
*******************************************************************************/
template<typename S, typename T>
struct convert_object : std::unary_function<S, T>
{
      T operator() (S &source) const { return T(source) ; }
} ;

/*******************************************************************************

*******************************************************************************/
/// Get the identity conversion.
template<class T>
inline identity<T> make_identity(T *) { return identity<T>() ; }

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
inline typename disable_if<is_dereferenceable<T>::value, T &>::type &dereference(T &v)
{
   return deref<T>()(v) ;
}

/*******************************************************************************
                     template<typename Seed, typename Op>
                     struct generator
*******************************************************************************/
template<typename Seed, typename Op>
struct generator {
      typedef Seed   result_type ;
      typedef Op     op_type ;

      generator(const op_type &op) : _functor(op), _seed() {}
      generator(const op_type &op, const result_type &seed) : _functor(op), _seed(seed) {}

      result_type &operator ()() { return _seed = _functor(_seed) ; }

   private:
      op_type     _functor ;
      result_type _seed ;
} ;

template<typename T, typename Diff = T>
struct incr : std::unary_function<T, T>
{
   incr(const Diff &diff) : _diff(diff) {}
   T &operator() (T &value) const { return value += _diff ; }

   private:
      Diff _diff ;
} ;

template<class T>
inline int threeway_compare(const T &t1, const T &t2)
{
   return t1 < t2 ? -1 : t2 < t1 ;
}

template<class T>
struct threeway_cmp : std::binary_function<const T, const T, int>
{
   int operator() (const T &t1, const T &t2) const
   {
      return threeway_compare(t1, t2) ;
   }
} ;

/******************************************************************************/
/** A function object that tests whether some value lies in some (open) range
*******************************************************************************/
template<typename T>
struct is_xinrange : std::unary_function<const T, bool> {
      is_xinrange() : _range() {}
      is_xinrange(const T &begin, const T &end) : _range(begin, end) {}

      bool operator()(const T &v) const
      {
         return !(v < _range.first) && v < _range.second ;
      }
private:
    std::pair<T, T> _range ;
} ;

/*******************************************************************************
                     template<class Container, typename Key>
                     struct container_item
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

/*******************************************************************************

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
      const container_inserter &operator()(const T &item) const
      {
         container().insert(container().end(), item) ;
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
                     struct clone_any
*******************************************************************************/
struct clone_any {
      template<typename T>
      T *operator()(const T *source) const
      {
         return clone_object(source) ;
      }
} ;

template<typename T>
struct clone : public std::unary_function<const T *, T *> {
      T *operator()(const T *source) const
      {
         return clone_object(source) ;
      }
} ;

template<typename T>
struct is_empty : public std::unary_function<const T, bool> {
      bool operator()(const T &value) const
      {
         return value.empty(value) ;
      }
} ;

struct apply_fn {
      template<typename F>
      typename std::result_of<F()>::type
      operator() (const F &fn) const { return fn() ; }

      template<typename F, typename P1>
      typename std::result_of<F(P1)>::type
      operator() (const F &fn, const P1 &p1) const { return fn(p1) ; }
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
 rbinder1st
 rbinder2nd
*******************************************************************************/
template<class Op>
struct rbinder1st : std::unary_function<typename Op::second_argument_type, typename Op::result_type> {

      rbinder1st(const Op &o, const typename Op::first_argument_type &a) : op(o), arg1(a) {}

      typename Op::result_type
      operator() (clvref_t<typename Op::second_argument_type> arg2) const
      {
         return op(arg1, arg2) ;
      }

   protected:
      Op op ;
      typename Op::first_argument_type arg1 ;
} ;

template<class Op>
struct rbinder2nd : std::unary_function<typename Op::first_argument_type, typename Op::result_type> {

      rbinder2nd(const Op &o, const typename Op::second_argument_type &a) : op(o), arg2(a) {}

      typename Op::result_type
      operator() (clvref_t<typename Op::first_argument_type> arg1) const
      {
         return op(arg1, arg2) ;
      }

   protected:
      Op op ;
      typename Op::second_argument_type arg2 ;
} ;

template<class Op, class Arg>
inline rbinder1st<Op> rbind1st(const Op &op, const Arg &arg)
{
   return rbinder1st<Op>(op, typename Op::first_argument_type(arg)) ;
}

template<class Op, class Arg>
inline rbinder2nd<Op> rbind2nd(const Op &op, const Arg &arg)
{
   return rbinder2nd<Op>(op, typename Op::second_argument_type(arg)) ;
}

/*******************************************************************************
 make_function
 bind_thisptr
*******************************************************************************/
template<typename R, typename T>
std::function<R()> bind_thisptr(R (T::*memfn)(), T *thisptr)
{
   return std::function<R()>(std::bind(memfn, thisptr)) ;
}
template<typename R, typename T>
std::function<R()> bind_thisptr(R (T::*memfn)() const, const T *thisptr)
{
   return std::function<R()>(std::bind(memfn, thisptr)) ;
}

template<typename R, typename T>
std::function<R()> bind_thisptr(R (T::element_type::*memfn)(), T thisptr)
{
   return std::function<R()>(std::bind(memfn, thisptr)) ;
}

template<typename R, typename T>
std::function<R()> bind_thisptr(R (T::element_type::*memfn)() const, T thisptr)
{
   return std::function<R()>(std::bind(memfn, thisptr)) ;
}

#define P_PLACEHOLDER_(num, dummy)  _::_##num
#define P_PLACELIST_(nargs) P_FOR(nargs, P_PLACEHOLDER_)
#define P_BIND_THISPTR_(argc)                                           \
   template<typename R, typename C, typename T, P_TARGLIST(argc, typename)> \
   typename std::enable_if<std::is_base_of<C, T>::value, std::function<R(P_TARGLIST(argc))> >::type \
   bind_thisptr(R (C::*memfn)(P_TARGLIST(argc)), T *thisptr)            \
   {                                                                    \
      namespace _ = std::placeholders ;                                 \
      typedef std::function<R(P_TARGLIST(argc))> function_type ;        \
      return function_type(std::bind(memfn, thisptr, P_PLACELIST_(argc))) ; \
   }                                                                    \
   template<typename R, typename C, typename T, P_TARGLIST(argc, typename)> \
   typename std::enable_if<std::is_base_of<C, T>::value, std::function<R(P_TARGLIST(argc))> >::type \
   bind_thisptr(R (C::*memfn)(P_TARGLIST(argc)) const, const T *thisptr) \
   {                                                                    \
      namespace _ = std::placeholders ;                                 \
      typedef std::function<R(P_TARGLIST(argc))> function_type ;        \
      return function_type(std::bind(memfn, thisptr, P_PLACELIST_(argc))) ; \
   }                                                                    \
   template<typename R, typename T, P_TARGLIST(argc, typename)>         \
   std::function<R(P_TARGLIST(argc))>                                   \
   bind_thisptr(R (T::element_type::*memfn)(P_TARGLIST(argc)), T thisptr) \
   {                                                                    \
      namespace _ = std::placeholders ;                                 \
      return std::function<R(P_TARGLIST(argc))>(std::bind(memfn, thisptr, P_PLACELIST_(argc))) ; \
   }                                                                    \
   template<typename R, typename T, P_TARGLIST(argc, typename)>         \
   std::function<R(P_TARGLIST(argc))>                                   \
   bind_thisptr(R (T::element_type::*memfn)(P_TARGLIST(argc)) const, T thisptr) \
   {                                                                    \
      namespace _ = std::placeholders ;                                 \
      return std::function<R(P_TARGLIST(argc))>(std::bind(memfn, thisptr, P_PLACELIST_(argc))) ; \
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

template<typename R>
std::function<R()> make_function(R (*fn)())
{
   return std::function<R()>(fn) ;
}

#define P_MAKE_FUNCTION_(argc)                        \
   template<typename R, P_TARGLIST(argc, typename)>   \
   std::function<R(P_TARGLIST(argc))>                 \
   make_function(R (*fn)(P_TARGLIST(argc)))           \
   {                                                  \
      return std::function<R(P_TARGLIST(argc))>(fn) ; \
   }

P_MAKE_FUNCTION_(1) ;
P_MAKE_FUNCTION_(2) ;
P_MAKE_FUNCTION_(3) ;
P_MAKE_FUNCTION_(4) ;
P_MAKE_FUNCTION_(5) ;
P_MAKE_FUNCTION_(6) ;
P_MAKE_FUNCTION_(7) ;
P_MAKE_FUNCTION_(8) ;
#undef P_MAKE_FUNCTION_

} // end of namespace pcomn

#endif /* __PCOMN_FUNCTION_H */
