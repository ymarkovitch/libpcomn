/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_ALGORITHM_H
#define __PCOMN_ALGORITHM_H
/*******************************************************************************
 FILE         :   pcomn_algorithm.h
 COPYRIGHT    :   Yakov Markovitch, 1998-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Some necessary and useful algorithms that absent in STL.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   10 Apr 1998
*******************************************************************************/
#include <pcomn_function.h>
#include <pcomn_iterator.h>

#include <algorithm>
#include <utility>
#include <string>
#include <type_traits>
#include <new>

#include <string.h>

namespace pcomn {

template<class InputIterator, class OutputIterator>
inline std::pair<InputIterator, OutputIterator> bound_copy
(
   InputIterator ifrom, InputIterator ito,
   OutputIterator ofrom, OutputIterator oto
)
{
   while (ifrom != ito && ofrom != oto)
   {
     *ofrom = *ifrom ;
      ++ifrom ;
      ++ofrom ;
   }
   return {ifrom, ofrom} ;
}

template<class InputIterator, class OutputIterator, class Transform>
std::pair<InputIterator, OutputIterator> bound_transform
(
   InputIterator ifrom, InputIterator ito,
   OutputIterator ofrom, OutputIterator oto, Transform trs
)
{
   while (ifrom != ito && ofrom != oto)
   {
     *ofrom = trs (*ifrom) ;
      ++ifrom ;
      ++ofrom ;
   }
   return {ifrom, ofrom} ;
}

template<class InputIterator, class OutputIterator, class Predicate>
std::pair<InputIterator, OutputIterator> bound_copy_if
(
   InputIterator ifrom, InputIterator ito,
   OutputIterator ofrom, OutputIterator oto, Predicate p
)
{
   while (ifrom != ito && ofrom != oto)
   {
      if (p(*ifrom))
      {
        *ofrom = *ifrom ;
         ++ofrom ;
      }
      ++ifrom ;
   }
   return {ifrom, ofrom} ;
}

/*******************************************************************************
 Member extractors
*******************************************************************************/
#define PCOMN_MEMBER_EXTRACTOR(member)                                  \
   template<typename T>                                                 \
   struct extract_##member {                                            \
      decltype(declval<T>().member()) operator() (const T &t) const { return t.member() ; } \
   } ;                                                                  \
                                                                        \
   template<typename T>                                                 \
   inline extract_##member<T> member##_extractor(const T &) { return {} ; }  \
                                                                        \
   template<typename T>                                                 \
   struct extract_##member<T *>  {                                      \
      decltype(declval<const T *>()->member()) operator() (const T *t) const \
      {                                                                 \
         return t ? t->member() : pcomn::default_constructed<pcomn::valtype_t<R> >::value ; \
      }                                                                 \
   }

PCOMN_MEMBER_EXTRACTOR(name) ;
PCOMN_MEMBER_EXTRACTOR(key) ;
PCOMN_MEMBER_EXTRACTOR(code) ;
PCOMN_MEMBER_EXTRACTOR(id) ;
PCOMN_MEMBER_EXTRACTOR(hash) ;
PCOMN_MEMBER_EXTRACTOR(size) ;

template<typename T, typename V, template<typename, typename> class X>
struct less_by : std::binary_function<T, T, bool> {
      bool operator() (const T &left, const T &right) const
      {
         return _xtract(left) < _xtract(right) ;
      }
   private:
      X<T, V> _xtract ;
} ;


/******************************************************************************/
/** Function object for performing comparison of keys by keyed values in a container
*******************************************************************************/
template<typename Container, typename Compare = std::less<typename Container::value_type> >
struct before_by_key : Compare {

      explicit before_by_key(const Container &container) :
         Compare(), _container(container) {}

      before_by_key(const Container &container, const Compare &comp) :
         Compare(comp), _container(container) {}

      template<typename Key>
      bool operator()(const Key &x, const Key &y) const
      {
         return (*static_cast<const Compare *>(this))(_container[x], _container[y]) ;
      }
   private:
      Container _container ;
} ;

template<typename Container>
inline before_by_key<Container>
make_before_by_key(Container &&container)
{
   return before_by_key<Container>(std::forward<Container>(container)) ;
}

template<typename Container, typename Compare>
inline before_by_key<Container, Compare>
make_before_by_key(Container &&container, Compare &&comp)
{
   return {std::forward<Container>(container), std::forward<Compare>(comp)} ;
}

/******************************************************************************/
/** Function object for performing comparison of indices by indexes values
*******************************************************************************/
template<typename RandomAccessIter,
         typename Compare = std::less<iterator_value_t<RandomAccessIter> > >
struct before_by_ndx : Compare {

      explicit before_by_ndx(RandomAccessIter iter) :
         Compare(), _iter(iter) {}

      before_by_ndx(RandomAccessIter iter, const Compare &comp) :
         Compare(comp), _iter(iter) {}

      template<typename Index>
      bool operator()(const Index &x, const Index &y) const
      {
         return (*static_cast<const Compare *>(this))(*(_iter + x), *(_iter + y)) ;
      }
   private:
      RandomAccessIter _iter ;
} ;

template<typename RandomAccessIter>
inline before_by_ndx<RandomAccessIter>
make_before_by_ndx(RandomAccessIter &&iter)
{
   return before_by_ndx<RandomAccessIter>(std::forward<RandomAccessIter>(iter)) ;
}

template<typename RandomAccessIter, typename Compare>
inline before_by_ndx<RandomAccessIter, Compare>
make_before_by_ndx(RandomAccessIter &&iter, Compare &&comp)
{
   return {std::forward<RandomAccessIter>(iter), std::forward<Compare>(comp)} ;
}

/*******************************************************************************
 Algorithms
*******************************************************************************/
/// Indicate whether some value is contained in a sequence.
/// If an equal to @a value is contained in a sequence defined by
/// @a begin and @a end iterators, returns 'true', otherwise 'false'.
/// @note For equality test uses operator==
template<class InputIterator, class T>
inline bool exists(InputIterator begin, InputIterator end, const T &value)
{
   for (; begin != end ; ++begin)
      if (*begin == value)
         return true ;
   return false ;
}

template<class T>
inline T *destroy(T *ptr)
{
   if (ptr)
      ptr->~T() ;
   return ptr ;
}

template<typename InputIterator>
inline InputIterator destroy_range(InputIterator begin, InputIterator end)
{
   for ( ; begin != end ; ++begin)
      destroy(&*begin) ;
   return begin ;
}

template<typename T>
inline T &placement_construct(T &mem) { return *new (&mem) T ; }

template<typename InputIterator>
inline InputIterator construct_range(InputIterator begin, InputIterator end)
{
   for ( ; begin != end ; ++begin)
      placement_construct(*begin) ;
   return begin ;
}

/// Clear-by-swap: implements widely occuring C++ idiom of clearing an object by swapping
/// it with a default constructed temporary.
template<class T>
inline T &swap_clear(T &value)
{
   T().swap(value) ;
   return value ;
}

template<class T>
inline T &assign_clear(T &value)
{
   return value = T() ;
}

template<class Iter, class Val>
inline std::pair<Iter, bool> search(Iter begin, Iter end, Val val)
{
   std::pair<Iter, bool> result (std::find(begin, end, val), false) ;
   result.second = result.first != end ;
   return result ;
}

template<class Iter, class Pred>
inline std::pair<Iter, bool> search_if(Iter begin, Iter end, Pred pred)
{
   std::pair<Iter, bool> result (std::find_if(begin, end, pred), false) ;
   result.second = result.first != end ;
   return result ;
}

template<class InputIterator, class Value>
inline InputIterator find_n(InputIterator begin, size_t maxitems, Value val)
{
   while(maxitems-- && !(*begin == val))
      ++begin ;
   return begin ;
}

template<class InputIterator, class Pred>
inline InputIterator find_n_if(InputIterator begin, size_t maxitems, Pred pred)
{
   while(maxitems-- && !pred(*begin))
      ++begin ;
   return begin ;
}

template<typename InputIterator, typename ForwardIterator>
InputIterator find_first_not_of(InputIterator begin1, InputIterator end1,
                                ForwardIterator begin2, ForwardIterator end2)
{
   while (begin1 != end1 && exists(begin2, end2, *begin1))
      ++begin1 ;
   return begin1 ;
}

template<typename InputIterator, typename ForwardIterator, typename BinaryPredicate>
InputIterator find_first_not_of(InputIterator begin1, InputIterator end1,
                                ForwardIterator begin2, ForwardIterator end2,
                                BinaryPredicate pred)
{
   while (begin1 != end1 && std::any_of(begin2, end2, [&](decltype(*begin2) &c){ return pred(*begin1, c) ; }))
      ++begin1 ;
   return begin1 ;
}

template<class InputIterator1, class InputIterator2>
bool lexicographical_compare(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2)
{
   while(first1 != last1)
   {
      if (!(*first1 == *first2))
         return *first1 < *first2 ;
      ++first1 ;
      ++first2 ;
   }
   return false ;
}

template <class InputIterator1, class InputIterator2>
int lexicographical_compare_3way(InputIterator1 first1, InputIterator1 last1,
                                 InputIterator2 first2, InputIterator2 last2)
{
   for(;;)
   {
      if (first2 == last2)
         return !(first1 == last1) ;
      if (first1 == last1)
         return -1 ;
      if (*first1 < *first2)
         return -1 ;
      if (*first2 < *first1)
         return 1 ;
      ++first1 ;
      ++first2 ;
   }
}

template <class ForwardIterator, class Function>
void adjacent_for_each(ForwardIterator begin, ForwardIterator end, Function f)
{
   if (begin != end)
   {
      ForwardIterator next (begin) ;
      while(++next != end)
      {
         f(*begin, *next) ;
         ++begin ;
      }
   }
}

template<typename ForwardIterator, typename BinaryPredicate, typename BinaryOperation>
inline ForwardIterator adjacent_coalesce(ForwardIterator first, ForwardIterator last,
                                         BinaryPredicate pred, BinaryOperation coalesce)
{
    first = std::adjacent_find(first, last, pred) ;
    if (first == last)
        return last ;
    ForwardIterator result = first ;
    ++first ;
    *result = coalesce(*result, *first) ;

    for (++first ; first != last ; ++first)
       if (pred(*result, *first))
          *result = coalesce(*result, *first) ;
       else
       {
          ++result ;
          *result = std::move(*first) ;
       }
    ++result ;
    return result ;
}

template <typename T>
inline std::enable_if_t<std::is_trivially_copyable<T>::value, T *>
raw_copy(const T *first, const T *last, T *dest)
{
   NOXCHECK(dest || first == last) ;
   const ptrdiff_t length = last - first ;
   memmove(dest, first, length*sizeof(T)) ;
   return dest + length ;
}

template <typename T>
inline std::enable_if_t<(std::is_scalar<T>::value && sizeof(T) == sizeof(char)), void>
raw_fill(T *first, T *last, T value)
{
   NOXCHECK(first && last && first <= last || first == last) ;
   memset(first, value, last-first) ;
}

template <typename T>
inline std::enable_if_t<(std::is_scalar<T>::value && sizeof(T) == sizeof(wchar_t)), void>
raw_fill(T *first, T *last, T value)
{
   NOXCHECK(first && last && first <= last || first == last) ;
   wmemset((wchar_t *)first, value, last-first) ;
}

template <typename T>
inline std::enable_if_t<(std::is_scalar<T>::value && sizeof(T) != sizeof(char) && sizeof(T) != sizeof(wchar_t)), void>
raw_fill(T *first, T *last, const T &value)
{
   NOXCHECK(first && last && first <= last || first == last) ;
   std::fill(first, last, value) ;
}

template <typename T, size_t n>
inline void raw_fill(T (&buf)[n], const T &value)
{
   raw_fill(buf + 0, buf + n, value) ;
}

} // end of namespace pcomn

#endif /* __PCOMN_ALGORITHM_H */
