/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_CALGORITHM_H
#define __PCOMN_CALGORITHM_H
/*******************************************************************************
 FILE         :   pcomn_calgorithm.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Algorithms defined on containers (instead of interators)

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   25 Apr 2010
*******************************************************************************/
/** Algorithms defined on containers instead of interators
*******************************************************************************/
#include <pcomn_algorithm.h>
#include <pcomn_iterator.h>
#include <pcomn_meta.h>

#include <vector>
#include <array>
#include <memory>

namespace pcomn {
template<typename C>
using container_iterator_t = typename std::remove_reference_t<C>::iterator ;
template<typename C>
using container_const_iterator_t = typename std::remove_reference_t<C>::const_iterator ;
template<typename C>
using container_value_t = typename std::remove_reference_t<C>::value_type ;

/******************************************************************************/
/** Append contents of a container to the end of another container.
*******************************************************************************/
template<class C1, class C2>
inline disable_if_t<has_key_type<std::remove_reference_t<C1> >::value, C1 &&>
append_container(C1 &&c1, const C2 &c2)
{
   c1.insert(c1.end(), std::begin(c2), std::end(c2)) ;
   return std::forward<C1>(c1) ;
}

/// Insert the contents of a container into a keyed container (like map or hash table).
template<class KeyedContainer, class Container>
inline std::enable_if_t<has_key_type<std::remove_reference_t<KeyedContainer> >::value, KeyedContainer &&>
append_container(KeyedContainer &&c1, const Container &c2)
{
   c1.insert(std::begin(c2), std::end(c2)) ;
   return std::forward<KeyedContainer>(c1) ;
}

/******************************************************************************/
/** Apply given function to iterator range items and construct a container from
 result
*******************************************************************************/
template<class Container, typename InputIterator, typename UnaryOperation>
inline Container make_container(InputIterator first, InputIterator last, UnaryOperation &&op)
{
   auto start = xform_iter(first, std::ref(op)) ;

   return Container(start, xform_iter(last, std::ref(op))) ;
}

/*******************************************************************************
 Get a value from the keyed container
*******************************************************************************/
template<class KeyedContainer, typename Key>
inline bool find_keyed_value(const KeyedContainer &c, Key &&key,
                             typename KeyedContainer::mapped_type &result)
{
   const typename KeyedContainer::const_iterator i (c.find(std::forward<Key>(key))) ;
   if (i == c.end())
      return false ;
   result = i->second ;
   return true ;
}

template<class KeyedContainer, typename Key>
inline const typename KeyedContainer::mapped_type &
get_keyed_value(const KeyedContainer &c, Key &&key,
                const typename KeyedContainer::mapped_type &defval = typename KeyedContainer::mapped_type())
{
   const typename KeyedContainer::const_iterator i (c.find(std::forward<Key>(key))) ;
   return i == c.end() ? defval : i->second ;
}

template<class Set, typename Key,
         typename C = typename Set::key_compare, typename = typename C::is_transparent>
inline const typename Set::key_type &
get_keyed_value(const Set &c, Key &&key,
                const typename Set::key_type &defval = typename Set::key_type())
{
   const typename Set::const_iterator i (c.find(std::forward<Key>(key))) ;
   return i == c.end() ? defval : *i ;
}

template<class KeyedContainer>
inline bool erase_keyed_value(KeyedContainer &c,
                              const typename KeyedContainer::key_type &key,
                              typename KeyedContainer::mapped_type &result)
{
   const typename KeyedContainer::iterator i (c.find(key)) ;
   if (i == c.end())
      return false ;
   result = std::move(i->second) ;
   c.erase(i) ;
   return true ;
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

template<typename T, typename = instance_if_t<std::is_arithmetic<T>::value> >
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

template<typename T, size_t N>
inline constexpr T *pbegin(T (&a)[N]) { return a ; }

template<typename T, size_t N>
inline constexpr T *pend(T (&a)[N]) { return a + N ; }

PCOMN_DEFINE_PRANGE(std::array<P_PASS(T, N)>, template<typename T, size_t N>) ;

/******************************************************************************/
/** Clear a container containing pointers: delete all pointed values and clear
 the container itself.
 @note The name 'clear_icontainer' stands for 'clear indirect container'
*******************************************************************************/
template<typename C>
inline C &clear_icontainer(C &container)
{
   std::for_each(container.begin(), container.end(),
                 std::default_delete<std::remove_pointer_t<typename C::value_type> >()) ;
   container.clear() ;
   return container ;
}

/******************************************************************************/
/** Ensure the size of a container is at least as big as specified; resize
 the container to the specified size if its current size is less

 In contrast with common STL container resize() method, never shrinks
 the container.
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
template<typename T>
inline std::vector<T> &sort(std::vector<T> &v)
{
   std::sort(v.begin(), v.end()) ;
   return v ;
}

template<typename T, typename BinaryPredicate>
inline std::vector<T> &sort(std::vector<T> &v, BinaryPredicate &&pred)
{
   std::sort(v.begin(), v.end(), std::forward<BinaryPredicate>(pred)) ;
   return v ;
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

/// Sort the items of a vector and ensure they are unique
template<typename T>
inline std::vector<T> &unique_sort(std::vector<T> &v)
{
   return unique(sort(v)) ;
}

/// @overload
template<typename T, typename BinaryPredicate>
inline std::vector<T> &unique_sort(std::vector<T> &v, BinaryPredicate pred)
{
   return unique(sort(v, pred),
                 #ifdef PCOMN_STL_CXX14
                 [&](const auto &x, const auto &y) { return !pred(x, y) ; }
                 #else
                 std::binary_negate<BinaryPredicate>(pred)
                 #endif
                 ) ;
}

/*******************************************************************************
 Test sequences/containers are equal
*******************************************************************************/
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

} // end of namespace pcomn

#endif /* __PCOMN_CALGORITHM_H */
