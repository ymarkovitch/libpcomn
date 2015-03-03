/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_CALGORITHM_H
#define __PCOMN_CALGORITHM_H
/*******************************************************************************
 FILE         :   pcomn_calgorithm.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Algorithms defined on containers (instead of interators)

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   25 Apr 2010
*******************************************************************************/
/** Algorithms defined on containers instead of interators
*******************************************************************************/
#include <pcomn_algorithm.h>
#include <pcomn_function.h>
#include <pcomn_meta.h>

#include <vector>
#include <memory>

namespace pcomn {

/******************************************************************************/
/** Append contents of a container to the end of another container.
*******************************************************************************/
template<class Container1, class Container2>
inline disable_if_t<has_key_type<std::remove_reference_t<Container1> >::value, Container1 &>
append_container(Container1 &&c1, const Container2 &c2)
{
   c1.insert(c1.end(), c2.begin(), c2.end()) ;
   return c1 ;
}

/// Insert the contents of a container into a keyed container (like map or hash table).
template<class KeyedContainer, class Container>
inline std::enable_if_t<has_key_type<std::remove_reference_t<KeyedContainer> >::value, KeyedContainer &>
append_container(KeyedContainer &&c1, const Container &c2)
{
   c1.insert(c2.begin(), c2.end()) ;
   return c1 ;
}

/*******************************************************************************
 Get a value from the keyed container
*******************************************************************************/
template<class KeyedContainer>
inline bool find_keyed_value(const KeyedContainer &c,
                             const typename KeyedContainer::key_type &key,
                             typename KeyedContainer::mapped_type &result)
{
   const typename KeyedContainer::const_iterator i (c.find(key)) ;
   if (i == c.end())
      return false ;
   result = i->second ;
   return true ;
}

template<class KeyedContainer>
inline const typename KeyedContainer::mapped_type &
get_keyed_value(const KeyedContainer &c, const typename KeyedContainer::key_type &key,
                const typename KeyedContainer::mapped_type &defval = typename KeyedContainer::mapped_type())
{
   const typename KeyedContainer::const_iterator i (c.find(key)) ;
   return i == c.end() ? defval : i->second ;
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
/** Clear a container containing pointers: delete all pointed values and clear
 the container itself.
 @note The name 'clear_icontainer' stands for 'clear indirect container'
*******************************************************************************/
template<typename Container>
inline Container &clear_icontainer(Container &container)
{
   std::for_each(container.begin(), container.end(),
                 std::default_delete<typename std::remove_pointer<typename Container::value_type>::type>()) ;
   container.clear() ;
   return container ;
}

/******************************************************************************/
/** Ensure the size of a container is at least as big as specified; resize
 the container to the specified size if its current size is less

 In contrast with common STL container resize() method, never shrinks
 the container.
*******************************************************************************/
template<typename Container>
inline Container &ensure_size(Container &container, size_t sz)
{
   if (sz > container.size())
      container.resize(sz) ;
   return container ;
}

/// @overload
template<typename Container>
inline Container &ensure_size(Container &container, size_t sz,
                              const typename Container::value_type &value)
{
   if (sz > container.size())
      container.resize(sz, value) ;
   return container ;
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
inline std::vector<T> &sort(std::vector<T> &v, BinaryPredicate predicate)
{
   std::sort(v.begin(), v.end(), predicate) ;
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
inline std::vector<T> &unique(std::vector<T> &v, BinaryPredicate predicate)
{
   v.erase(std::unique(v.begin(), v.end(), predicate), v.end()) ;
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
inline std::vector<T> &unique_sort(std::vector<T> &v, BinaryPredicate predicate)
{
   return unique(sort(v, predicate), std::binary_negate<BinaryPredicate>(predicate)) ;
}

} // end of namespace pcomn

#endif /* __PCOMN_CALGORITHM_H */
