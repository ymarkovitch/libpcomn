/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_CALGORITHM_H
#define __PCOMN_CALGORITHM_H
/*******************************************************************************
 FILE         :   pcomn_calgorithm.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Algorithms defined on containers (instead of interators)

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   25 Apr 2010
*******************************************************************************/
/** @file
  STL-like algorithms defined on containers instead of interators.

  Type traits:
    container_iterator_t<C>:       Container::iterator
    container_const_iterator_t<C>: Container::const_iterator
    container_value_t<C>:          Container::value_type

  All traits automatically remove references from its parameters, so e.g. both
  container_iterator_t<std::vector<int>> and
  container_iterator_t<const std::vector<int>&>
  are OK.

  Algorithms:
    make_container
    append_container
    find_keyed_value
    get_keyed_value
    erase_keyed_value
    erase_by_iterator
    erase_first
    erase_first_if
    both_ends
    pbegin
    pend
    ensure_size
    extend_container
    truncate_container
    sort
    unique
    unique_sort
    clear_icontainer
    caccumulate

    has_item
    all_of
    any_of
    none_of
    equal_seq
*******************************************************************************/
#include <pcomn_algorithm.h>
#include <pcomn_iterator.h>
#include <pcomn_meta.h>

#include <vector>
#include <array>
#include <memory>
#include <string>

namespace pcomn {
template<typename C>
using container_iterator_t = typename std::remove_reference_t<C>::iterator ;
template<typename C>
using container_const_iterator_t = typename std::remove_reference_t<C>::const_iterator ;
template<typename C>
using container_value_t = typename std::remove_reference_t<C>::value_type ;

/***************************************************************************//**
 Append contents of a container to the end of another container.
*******************************************************************************/
/**@{*/
template<class C1, class C2>
inline disable_if_t<has_key_type<valtype_t<C1>>::value, C1 &&>
append_container(C1 &&c1, const C2 &c2)
{
   c1.insert(c1.end(), std::begin(c2), std::end(c2)) ;
   return std::forward<C1>(c1) ;
}

template<class C1, class C2, typename UnaryOperation>
inline disable_if_t<has_key_type<valtype_t<C1>>::value, C1 &&>
append_container(C1 &&c1, const C2 &c2, UnaryOperation &&xform)
{
   c1.insert(c1.end(),
             xform_iter(std::begin(c2), std::ref(xform)),
             xform_iter(std::end(c2), std::ref(xform))) ;
   return std::forward<C1>(c1) ;
}
/**@}*/

/// Insert the contents of a container into a keyed container (like map or hash table).
/**@{*/
template<class KeyedContainer, class Container>
inline std::enable_if_t<has_key_type<std::remove_reference_t<KeyedContainer>>::value, KeyedContainer &&>
append_container(KeyedContainer &&c1, const Container &c2)
{
   c1.insert(std::begin(c2), std::end(c2)) ;
   return std::forward<KeyedContainer>(c1) ;
}

template<class KeyedContainer, class Container, typename UnaryOperation>
inline std::enable_if_t<has_key_type<std::remove_reference_t<KeyedContainer>>::value, KeyedContainer &&>
append_container(KeyedContainer &&c1, const Container &c2, UnaryOperation &&xform)
{
   c1.insert(xform_iter(std::begin(c2), std::ref(xform)), xform_iter(std::end(c2), std::ref(xform))) ;
   return std::forward<KeyedContainer>(c1) ;
}
/**@}*/

/******************************************************************************/
/** Apply given function to iterator range items and construct a container from
 result
*******************************************************************************/
template<class Container, typename InputIterator, typename UnaryOperation>
inline Container make_container(InputIterator first, InputIterator last, UnaryOperation &&xform)
{
   return Container(xform_iter(first, std::ref(xform)), xform_iter(last, std::ref(xform))) ;
}

template<class Container, class Source, typename UnaryOperation>
inline Container make_container(Source &&source, UnaryOperation &&xform)
{
   return make_container<Container>(std::begin(std::forward<Source>(source)),
                                    std::end(std::forward<Source>(source)),
                                    std::forward<UnaryOperation>(xform)) ;
}

template<class Container, typename T, size_t N, typename UnaryOperation>
inline Container make_container(T (&source)[N], UnaryOperation &&xform)
{
   return make_container<Container>(source + 0, source + N, std::forward<UnaryOperation>(xform)) ;
}

template<class Container, typename Source>
inline Container make_container(Source &&source)
{
   return Container(std::begin(std::forward<Source>(source)),
                    std::end(std::forward<Source>(source))) ;
}

/*******************************************************************************
 Get a value from the keyed container
*******************************************************************************/
template<class Map, typename Key>
inline bool find_keyed_value(const Map &c, Key &&key, typename Map::mapped_type &result)
{
   const auto i (c.find(std::forward<Key>(key))) ;
   if (i == c.end())
      return false ;
   result = i->second ;
   return true ;
}

template<class Map, typename Key>
inline const typename Map::mapped_type &
get_keyed_value(const Map &c, Key &&key,
                const typename Map::mapped_type &defval = default_constructed<typename Map::mapped_type>::value)
{
   const auto i (c.find(std::forward<Key>(key))) ;
   return i == c.end() ? defval : i->second ;
}

template<class Set, typename Key,
         typename C = typename Set::key_compare, typename K = typename Set::key_type>
inline std::enable_if_t<std::is_same<K, container_value_t<Set>>::value, const K> &
get_keyed_value(const Set &c, Key &&key,
                const typename Set::key_type &defval = default_constructed<typename Set::key_type>::value)
{
   const auto i (c.find(std::forward<Key>(key))) ;
   return i == c.end() ? defval : *i ;
}

template<class Map, typename Key>
inline bool erase_keyed_value(Map &c, Key &&key, typename Map::mapped_type &result)
{
   const auto i (c.find(std::forward<Key>(key))) ;
   if (i == c.end())
      return false ;
   result = std::move(i->second) ;
   c.erase(i) ;
   return true ;
}

template<class Container, typename Iterator>
inline auto erase_by_iterator(Container &c, const Iterator &i) ->type_t<decltype(bool(i == c.end())),
                                                                        decltype(c.erase(i))>
{
   if (i == c.end())
      return false ;
   c.erase(i) ;
   return true ;
}

template<class Container, typename Key>
inline auto erase_first(Container &c, const Key &key) ->type_t<bool,
                                                               decltype(c.erase(c.begin()))>
{
   return erase_by_iterator(c, std::find(c.begin(), c.end(), key)) ;
}

template<class Container, typename UnaryPredicate>
inline auto erase_first_if(Container &c, UnaryPredicate &&pred) ->type_t<bool,
                                                                         decltype(c.erase(c.begin()))>
{
   return erase_by_iterator(c, std::find_if(c.begin(), c.end(), std::forward<UnaryPredicate>(pred))) ;
}

/*******************************************************************************

*******************************************************************************/
template<class Container, typename Value>
inline disable_if_t<has_key_type<std::remove_reference_t<Container>>::value, bool>
has_item(Container &&container, const Value &item)
{
   for (const auto &ci: std::forward<Container>(container))
      if (ci == item)
         return true ;
   return false ;
}

template<class KeyedContainer, typename Value>
inline std::enable_if_t<has_key_type<std::remove_reference_t<KeyedContainer>>::value, bool>
has_item(KeyedContainer &&container, const Value &item)
{
   return !!std::forward<KeyedContainer>(container).count(item) ;
}

template<class Container, class Test>
inline bool any_of(Container &&c, Test &&test)
{
   return std::any_of(std::begin(std::forward<Container>(c)),
                      std::end(std::forward<Container>(c)),
                      std::forward<Test>(test)) ;
}

template<class Container, class Test>
inline bool all_of(Container &&c, Test &&test)
{
   return std::all_of(std::begin(std::forward<Container>(c)),
                      std::end(std::forward<Container>(c)),
                      std::forward<Test>(test)) ;
}

template<class Container, class Test>
inline bool none_of(Container &&c, Test &&test)
{
   return std::none_of(std::begin(std::forward<Container>(c)),
                       std::end(std::forward<Container>(c)),
                       std::forward<Test>(test)) ;
}

/******************************************************************************/
/** Get both iterators of a container
*******************************************************************************/
template<typename C>
inline unipair<decltype(std::begin(std::declval<C>()))> both_ends(C &&container)
{
   return {std::begin(std::forward<C>(container)), std::end(std::forward<C>(container))} ;
}

template<typename C>
inline decltype(std::distance(std::begin(std::declval<C>()), std::end(std::declval<C>())))
range_size(C &&c)
{
   return std::distance(std::begin(std::forward<C>(c)), std::end(std::forward<C>(c))) ;
}

template<typename T, typename = instance_if_t<std::is_arithmetic<T>::value>>
inline auto range_size(const unipair<T> &c) -> decltype(c.second - c.first)
{
   return c.second - c.first ;
}

template<typename T, typename A>
inline auto pbegin(std::vector<T, A> &v) -> decltype(v.data())
{
   return v.data() ;
}

template<typename T, typename A>
inline auto pbegin(const std::vector<T, A> &v) -> decltype(v.data())
{
   return v.data() ;
}

template<typename T, typename A>
inline auto pend(std::vector<T, A> &v) -> decltype(v.data())
{
   return v.data() + v.size() ;
}

template<typename T, typename A>
inline auto pend(const std::vector<T, A> &v) -> decltype(v.data())
{
   return v.data() + v.size() ;
}

template<typename C, typename R, typename A>
inline const C *pbegin(const std::basic_string<C,R,A> &s) { return s.c_str() ; }

template<typename C, typename R, typename A>
inline const C *pend(const std::basic_string<C,R,A> &s) { return s.c_str() + s.size() ; }

template<typename T, size_t N>
inline constexpr T *pbegin(T (&a)[N]) { return a ; }

template<typename T, size_t N>
inline constexpr T *pend(T (&a)[N]) { return a + N ; }

PCOMN_DEFINE_PRANGE(std::array<P_PASS(T, N)>, template<typename T, size_t N>) ;

/***************************************************************************//**
 Clear a container containing pointers: delete all pointed values and clear the container
 itself.
 @note The name 'clear_icontainer' stands for 'clear indirect container'
*******************************************************************************/
template<typename C, typename=std::enable_if_t<std::is_pointer<container_value_t<C>>::value>>
inline C &clear_icontainer(C &container)
{
   std::for_each(container.begin(), container.end(),
                 std::default_delete<std::remove_pointer_t<container_value_t<C>>>()) ;
   container.clear() ;
   return container ;
}

/***************************************************************************//**
 Ensure the size of a container is at least as big as specified: resize
 the container to the specified size if its current size is less than specified.

 In contrast with common STL container resize() method, never shrinks the container.
*******************************************************************************/
template<typename C>
inline C &ensure_size(C &container, size_t sz)
{
   if (sz > container.size())
      container.resize(sz) ;
   return container ;
}

/// @overload
template<typename C>
inline C &ensure_size(C &container, size_t sz, const container_value_t<C> &value)
{
   if (sz > container.size())
      container.resize(sz, value) ;
   return container ;
}

template<typename C>
inline std::enable_if_t<is_iterator<container_iterator_t<C>, std::random_access_iterator_tag>::value, C &&>
truncate_container(C &&container, const container_iterator_t<C> &pos)
{
   container.resize(pos - container.begin()) ;
   return std::forward<C>(container) ;
}

template<typename C>
inline C &&extend_container(C &&container, size_t extra)
{
   container.resize(container.size() + extra) ;
   return std::forward<C>(container) ;
}

/*******************************************************************************
 Sort a container
*******************************************************************************/
template<typename C, bool=std::is_pointer<decltype(std::declval<C>().data())>::value>
inline C &&sort(C &&container)
{
   std::sort(std::forward<C>(container).begin(), std::forward<C>(container).end()) ;
   return std::forward<C>(container) ;
}

template<typename C, typename BinaryPredicate, bool=std::is_pointer<decltype(std::declval<C>().data())>::value>
inline C &&sort(C &&container, BinaryPredicate &&pred)
{
   std::sort(std::forward<C>(container).begin(), std::forward<C>(container).end(),
             std::forward<BinaryPredicate>(pred)) ;
   return std::forward<C>(container) ;
}

/*******************************************************************************
 Make a container to contain only unique items
*******************************************************************************/
/// Ensure the items of an already sorted vector are unique
///
/// More exactly: ensure there are no consecutive groups of duplicate elements in @a v
template<typename T>
inline std::vector<T> &unique(std::vector<T> &v)
{
   v.erase(std::unique(v.begin(), v.end()), v.end()) ;
   return v ;
}

/// @overload
template<typename T, typename BinaryPredicate>
inline std::vector<T> &unique(std::vector<T> &v, BinaryPredicate &&pred)
{
   v.erase(std::unique(v.begin(), v.end(), std::forward<BinaryPredicate>(pred)), v.end()) ;
   return v ;
}

template<typename T>
inline std::vector<T> &&unique(std::vector<T> &&v)
{
   return std::move<std::vector<T>>
      (unique(static_cast<std::vector<T> &>(v))) ;
}

template<typename T, typename BinaryPredicate>
inline std::vector<T> &&unique(std::vector<T> &&v, BinaryPredicate &&pred)
{
   return std::move<std::vector<T>>
      (unique(static_cast<std::vector<T> &>(v), std::forward<BinaryPredicate>(pred))) ;
}

/// Sort the items of a vector and ensure they are unique
template<typename T>
inline std::vector<T> &unique_sort(std::vector<T> &v)
{
   return unique(sort(v)) ;
}

/// @overload
template<typename T, typename BinaryPredicate>
inline std::vector<T> &unique_sort(std::vector<T> &v, BinaryPredicate &&pred)
{
   auto &&p = std::forward<BinaryPredicate>(pred) ;
   return unique(sort(v, p), [&](const auto &x, const auto &y) { return !p(x, y) ; }) ;
}

template<typename T>
inline std::vector<T> &&unique_sort(std::vector<T> &&v)
{
   return std::move(unique_sort(static_cast<std::vector<T> &>(v))) ;
}

template<typename T, typename BinaryPredicate>
inline std::vector<T> &&unique_sort(std::vector<T> &&v, BinaryPredicate &&pred)
{
   return std::move(unique_sort(static_cast<std::vector<T> &>(v), std::forward<BinaryPredicate>(pred))) ;
}

/***************************************************************************//**
 Compute the sum of the given value init and the elements in the container/range.

 The first version uses operator+ to sum up the elements, the second version uses
 the given binary function op.
*******************************************************************************/
/**@{*/
template<typename Container, typename T>
inline T caccumulate(Container &&range, T init)
{
   for (auto &v: std::forward<Container>(range))
      init += v ;
   return init ;
}

template<typename Container, typename T, typename BinaryOperation>
inline T caccumulate(Container &&range, T init, BinaryOperation &&op)
{
   for (auto &v: std::forward<Container>(range))
      init = std::forward<BinaryOperation>(op)(init, v) ;
   return init ;
}
/**@}*/

template<typename C, typename F>
inline C &&cfor_each(C &&container, F &&fn)
{
   for (auto &item: std::forward<C>(container))
      std::forward<F>(fn)(item) ;
   return std::forward<C>(container) ;
}

/***************************************************************************//**
 Test sequences/containers are equal.
*******************************************************************************/
/**@{*/
template<typename S1, typename S2>
inline bool equal_seq(S1 &&x, S2 &&y)
{
   using namespace std ;
   return
      size(std::forward<S1>(x)) == size(std::forward<S2>(y)) &&
      std::equal(begin(std::forward<S1>(x)), end(std::forward<S1>(x)), begin(std::forward<S2>(y))) ;
}

template<typename S1, typename S2, typename BinaryPredicate>
inline bool equal_seq(S1 &&x, S2 &&y, BinaryPredicate &&pred)
{
   using namespace std ;
   return
      size(std::forward<S1>(x)) == size(std::forward<S2>(y)) &&
      std::equal(begin(std::forward<S1>(x)), end(std::forward<S1>(x)), begin(std::forward<S2>(y)),
                 std::forward<BinaryPredicate>(pred)) ;
}
/**@}*/

/*******************************************************************************
 Permutations
*******************************************************************************/
/// Copy the source sequence `src` into the destination sequence `dest` according to the
/// permutation `p` so that @em p[i]-th item of `src` is copied into i-th item of `dest`.
///
/// The algorithm gathers items of the source sequence according to indices in
/// the specified permutation into the sequential items of the destination.
///
template<typename Container, typename Permutation>
inline const Container &gather_by_permutation(Container &dest, const Container &src, const Permutation &p)
{
    using namespace std ;
    std::copy(mapped_iter(src, begin(p)), mapped_iter(src, end(p)), begin(dest)) ;
    return dest ;
}

/// Copy the source sequence `src` into the destination sequence `dest` according to the
/// permutation `p` so that i-th item of `src` is copied into the p[i]-th item of `dest`.
///
/// The algorithm sequentially reads the source sequence and scatters its items into the
/// destination according to indices in the specified permutation.
///
template<typename Container, typename Permutation>
inline Container &scatter_by_permutation(Container &dest, const Container &src, const Permutation &p)
{
    using namespace std ;
    std::copy(begin(src), end(src), mapped_iter(dest, begin(p))) ;
    return dest ;
}

/// Invert the permutation, for 'src -> dest' permutation make 'dest -> src'.
/// E.g. {4,2,0,1,3} -> {2,3,1,4,0}
template<typename InputIterator, typename RandomIterator>
inline void invert_permutation(InputIterator src_begin, InputIterator src_end, RandomIterator dst)
{
    typedef pcomn::valtype_t<decltype(*dst)> dst_type ;
    PCOMN_STATIC_CHECK(std::is_integral<dst_type>() && !std::is_same<dst_type, bool>()) ;

    for (dst_type i = 0 ; src_begin != src_end ; ++src_begin)
        *(dst + *src_begin) = i++ ;
}

} // end of namespace pcomn

#endif /* __PCOMN_CALGORITHM_H */
