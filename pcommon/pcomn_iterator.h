/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_ITERATOR_H
#define __PCOMN_ITERATOR_H
/*******************************************************************************
 FILE         :   pcomn_iterator.h
 COPYRIGHT    :   Yakov Markovitch, 2006-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Iterators:
                  collection_iterator<RandomAccessContainer>
                  mapped_iterator<Container, Iterator, Reference>
                  xform_iterator<Iterator, Converter, Value>

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   19 Dec 2006
*******************************************************************************/
#include <pcomn_meta.h>
#include <pcomn_function.h>

#include <utility>
#include <iterator>
#include <tuple>
#include <functional>

namespace pcomn {

/*******************************************************************************
 *
 * is_iterator<Iterator, Category = std::input_iterator_tag>
 *
 * Type trait to check if the type is an iterator of at least specified category
 *
 *******************************************************************************/
namespace detail {
template<typename T, typename = std::remove_pointer_t<std::remove_cv_t<T> > >
struct icategory : std::iterator_traits<T> {} ;
template<typename T> struct icategory<T, void> {} ;

template<typename, typename>
std::false_type test_icategory(...) ;

template<typename T, typename C>
std::is_base_of<C, typename icategory<T>::iterator_category>
test_icategory(typename icategory<T>::iterator_category const volatile *) ;

}

/// type trait to check if the type is an iterator of at least specified category
///
template<typename T, typename C = std::input_iterator_tag>
struct is_iterator : decltype(detail::test_icategory<T, C>(0)) {} ;

/*******************************************************************************
 Convenience typedefs for various iterator traits
*******************************************************************************/
template<typename T>
using iterator_difference_t = typename std::iterator_traits<T>::difference_type ;

template<typename T>
using iterator_value_t = typename std::iterator_traits<T>::value_type ;

template<typename T>
using iterator_pointer_t = typename std::iterator_traits<T>::pointer_type ;

template<typename T>
using iterator_reference_t = typename std::iterator_traits<T>::reference ;

template<typename T>
using iterator_category_t = typename std::iterator_traits<T>::iterator_category ;

/******************************************************************************/
/**
*******************************************************************************/
template<typename Category = std::random_access_iterator_tag, typename Iter = int *>
iterator_difference_t<Iter> estimated_distance(const Iter &first, const Iter &last,
                                               iterator_difference_t<Iter> mindist = 0) ;

namespace detail {
template<typename I>
inline iterator_difference_t<I> estimate_distance(const I &first, const I &last, std::true_type)
{
   return std::distance(first, last) ;
}

template<typename I>
inline iterator_difference_t<I> estimate_distance(const I &, const I &, std::false_type)
{
   return 0 ;
}
}

template<typename C, typename I>
inline iterator_difference_t<I> estimated_distance(const I &first, const I &last,
                                                   iterator_difference_t<I> mindist)
{
   return std::max(detail::estimate_distance(first, last, is_iterator<I, C>()), mindist) ;
}

/*******************************************************************************

*******************************************************************************/
template<class RandomAccessContainer>
struct collection_iterator_tag :
         public std::iterator<std::random_access_iterator_tag,
                              typename RandomAccessContainer::value_type, int,
                              typename RandomAccessContainer::value_type *,
                              typename std::conditional
                              <std::is_const<RandomAccessContainer>::value,
                               typename RandomAccessContainer::const_reference,
                               typename RandomAccessContainer::reference>::type> {

      typedef RandomAccessContainer container_type ;
      typedef typename std::conditional
      <std::is_const<container_type>::value,
       typename container_type::const_reference,
       typename container_type::reference>::type reference ;
} ;

/******************************************************************************/
/** An iterator over a random-access container.
    Container requirements:
      - there should be types value_type, reference, const_reference;
      - an expression a[n], where a is a containter and n is of an integral
        type should be valid and return reference (const_reference).
*******************************************************************************/
template<class RandomAccessContainer>
struct collection_iterator : collection_iterator_tag<RandomAccessContainer> {

      typedef RandomAccessContainer container_type ;
      typedef typename collection_iterator_tag<RandomAccessContainer>::reference       reference ;
      typedef typename collection_iterator_tag<RandomAccessContainer>::difference_type difference_type ;

      collection_iterator() :
         _container(),
         _index()
      {}

      explicit collection_iterator(container_type &collection, difference_type ndx = 0) :
         _container(&collection),
         _index(ndx)
      {}

      reference operator*() const
      {
         NOXCHECK(_container) ;
         return (*_container)[_index] ;
      }
      reference operator->() const { return **this ; }

      reference operator[](difference_type ndx) const
      {
         NOXCHECK(_container) ;
         return (*_container)[_index + ndx] ;
      }

      collection_iterator &operator++()
      {
         NOXCHECK(_container) ;
         ++_index ;
         return *this ;
      }
      collection_iterator &operator--()
      {
         NOXCHECK(_container) ;
         --_index ;
         return *this ;
      }

      // Post(inc|dec)rement
      PCOMN_DEFINE_POSTCREMENT_METHODS(collection_iterator) ;

      collection_iterator &operator+=(difference_type diff)
      {
         NOXCHECK(_container || !diff) ;
         _index += diff ;
         return *this ;
      }

      friend bool operator==(const collection_iterator &lhs, const collection_iterator &rhs)
      {
         NOXCHECK(lhs._container == rhs._container) ;
         return lhs._index == rhs._index ;
      }

      friend bool operator<(const collection_iterator &lhs, const collection_iterator &rhs)
      {
         NOXCHECK(lhs._container == rhs._container) ;
         return lhs._index < rhs._index ;
      }

      PCOMN_DEFINE_RELOP_FUNCTIONS(friend, collection_iterator) ;

      collection_iterator &operator-=(difference_type diff) { return *this += -diff ; }
      collection_iterator operator+ (difference_type diff) const { return collection_iterator(*this) += diff ; }
      collection_iterator operator- (difference_type diff) const { return collection_iterator(*this) -= diff ; }
      difference_type operator-(const collection_iterator &rhs) const { return _index - rhs._index ; }

   private:
      container_type *_container ;
      difference_type _index ;
} ;

/******************************************************************************/
/** An iterator which counts how far it is advanced (i.e. how many times
 operator++() is called).
*******************************************************************************/
template<typename Counter = size_t>
struct count_iterator : std::iterator<std::random_access_iterator_tag, Counter, ptrdiff_t, void, Counter> {

      constexpr count_iterator() : _count() {}
      explicit constexpr count_iterator(Counter init_count) : _count(init_count) {}

      constexpr Counter count() const { return _count ; }
      constexpr Counter operator*() const { return count() ; }

      count_iterator &operator++()
      {
         ++_count ;
         return *this ;
      }
      count_iterator &operator--()
      {
         --_count ;
         return *this ;
      }
      count_iterator &operator+=(ptrdiff_t diff)
      {
         _count += diff ;
         return *this ;
      }
      count_iterator &operator-=(ptrdiff_t diff)
      {
         _count -= diff ;
         return *this ;
      }

      count_iterator &operator=(Counter value)
      {
         _count = value ;
         return *this ;
      }

      PCOMN_DEFINE_POSTCREMENT_METHODS(count_iterator) ;
      PCOMN_DEFINE_NONASSOC_ADDOP_METHODS(count_iterator, ptrdiff_t) ;

      friend ptrdiff_t operator-(const count_iterator &x, const count_iterator &y)
      {
         return x._count - y._count ;
      }
      friend bool operator==(const count_iterator &x, const count_iterator &y)
      {
         return x._count == y._count ;
      }
      friend bool operator<(const count_iterator &x, const count_iterator &y)
      {
         return x._count < y._count ;
      }
   private:
      Counter _count ;
} ;

PCOMN_DEFINE_RELOP_FUNCTIONS(template<typename T>, count_iterator<T>) ;

/******************************************************************************/
/** An output iterator which counts how far it is advanced (how many times
 operator=(Value &) is called).
*******************************************************************************/
template<typename Counter = size_t>
struct output_count_iterator :
         std::iterator<std::output_iterator_tag, Counter, ptrdiff_t, void, Counter> {

      explicit output_count_iterator(Counter init_count = Counter()) : _count(init_count) {}

      Counter count() const { return _count ; }

      template<typename T>
      output_count_iterator &operator=(const T &)
      {
         ++_count ;
         return *this ;
      }
      output_count_iterator &operator*() { return *this ; }
      output_count_iterator &operator++() { return *this ; }
      output_count_iterator &operator++(int) { return *this ; }
   private:
      Counter _count ;
} ;

/******************************************************************************/
/** Append iterator is an output iterator that turns assignment into appending
 to any container.

 Output iterator, constructed from a container. Assigning a value to such
 iterator appends it to the container, much like std::back_insert_iterator do;
 but, in contrast to std::back_insert_iterator it works with any container type,
 including std::map, std::set, and std:: unordered containers.

 Use the appender() function to create append_iterator.
*******************************************************************************/
template<typename Container>
struct append_iterator : std::iterator<std::output_iterator_tag, void, void, void, void> {

      /// A nested typedef for the type of whatever container you used.
      typedef Container container_type ;

      append_iterator(container_type &c) : _container(&c) {}

      template<typename T>
      append_iterator &operator=(T &&value)
      {
         put(has_key_type<container_type>(), std::forward<T>(value)) ;
         return *this ;
      }
      append_iterator &operator*() { return *this ; }
      append_iterator &operator++() { return *this ; }
      append_iterator &operator++(int) { return *this ; }

      container_type &container() { return *_container ; }

   private:
      container_type *_container ;

      template<typename T>
      void put(std::true_type, T &&value) { container().insert(std::forward<T>(value)) ; }
      template<typename T>
      void put(std::false_type, T &&value) { container().push_back(std::forward<T>(value)) ; }
} ;

/// Create append_iterator instance for a container.
/// @param  container  A container of arbitrary type.
/// @return An instance of append_iterator working on @a container.
template<typename Container>
inline append_iterator<Container> appender(Container &container)
{
   return append_iterator<Container>(container) ;
}

/******************************************************************************/
/** Output iterator that turns assignment into calling a functor.

 Output iterator, constructed from a functor. Assigning a value to it calls the functor,
 passing it a value

 Use the calliter() function to create call_iterator.
*******************************************************************************/
template<typename V>
class call_iterator : public std::iterator<std::output_iterator_tag, void, void, void, void> {
   public:
      typedef V value_type ;
      typedef std::function<void(const value_type &)> call_type ;

      call_iterator(const call_type &c) : _call(c) {}
      call_iterator(call_type &&c) : _call(std::move(c)) {}
      call_iterator(call_iterator &&v) : _call(std::move(v._call)) {}

      call_iterator &operator=(const value_type &value)
      {
         _call(value) ;
         return *this ;
      }
      call_iterator &operator*() { return *this ; }
      call_iterator &operator++() { return *this ; }
      call_iterator &operator++(int) { return *this ; }

   private:
      call_type _call ;
} ;

/// Create call_iterator instance for a functor.
template<typename V>
inline call_iterator<V> calliter(const std::function<void(const V &)> &callback)
{
   return call_iterator<V>(callback) ;
}

/// @overload
template<typename V>
inline call_iterator<V> calliter(std::function<void(const V &)> &&callback)
{
   return call_iterator<V>(std::move(callback)) ;
}

/******************************************************************************/
/** Mapped iterators allow traversing indexed container (e.g. vectors, maps)
 by external indices.
*******************************************************************************/
/// @cond
namespace detail {
template<typename C,
         int = ((int)std::is_pointer<C>::value + 2*(int)has_mapped_type<C>::value)>
struct container_reference ;

template<typename C> struct container_reference<C, (-1)> {
      typedef C &arg_type ;
      typedef C *member_type ;
      static member_type member_from_arg(arg_type c) { return &c ; }
      static arg_type arg_from_member(member_type m) { return *m ; }
} ;

// Pointer
template<typename C> struct container_reference<C, 1> :
         std::remove_pointer<C>
{
      typedef std::remove_cv_t<C>      arg_type ;
      typedef arg_type                 member_type ;
      static member_type member_from_arg(arg_type c) { return c ; }
      static arg_type arg_from_member(member_type m) { return m ; }
} ;

// Vector-like
template<typename C> struct container_reference<C, 0> : container_reference<C, (-1)>,
   std::conditional<std::is_const<C>::value, typename C::const_reference, typename C::reference> {} ;

// Map-like
template<typename C> struct container_reference<C, 2> : container_reference<C, (-1)>,
   std::conditional<std::is_const<C>::value, typename C::mapped_type const &, typename C::mapped_type &> {} ;
}
/// @endcond

template<typename Container, typename Iterator,
         typename Reference = typename detail::container_reference<Container>::type>
class mapped_iterator :
         public std::iterator<typename std::iterator_traits<Iterator>::iterator_category,
                              std::remove_reference_t<Reference>,
                              typename std::iterator_traits<Iterator>::difference_type,
                              void, Reference> {

      typedef std::iterator<typename std::iterator_traits<Iterator>::iterator_category,
                            std::remove_reference_t<Reference>,
                            typename std::iterator_traits<Iterator>::difference_type,
                            void, Reference> ancestor ;

      typedef detail::container_reference<Container> container_reference ;

   public:
      using typename ancestor::value_type ;
      using typename ancestor::reference ;
      using typename ancestor::difference_type ;

      mapped_iterator() : _container(), _iter() {}
      mapped_iterator(typename container_reference::arg_type c, const Iterator &i) :
         _container(container_reference::member_from_arg(c)), _iter(i) {}

      reference operator*() const { return container_reference::arg_from_member(_container)[*_iter] ; }

      mapped_iterator &operator++() { ++_iter ; return *this ; }
      mapped_iterator &operator--() { --_iter ; return *this ; }

      // Post(inc|dec)rement
      PCOMN_DEFINE_POSTCREMENT_METHODS(mapped_iterator) ;

      mapped_iterator &operator+=(difference_type diff) { _iter += diff ; return *this ; }
      mapped_iterator &operator-=(difference_type diff) { _iter -= diff ; return *this ; }

      mapped_iterator operator+(difference_type diff) const
      {
         return mapped_iterator(*this) += diff ;
      }
      mapped_iterator operator-(difference_type diff) const
      {
         return mapped_iterator(*this) -= diff ;
      }

      difference_type operator-(const mapped_iterator &other) const
      {
         return _iter - other._iter ;
      }

      friend bool operator==(const mapped_iterator &left, const mapped_iterator &right)
      {
         return left._iter == right._iter ;
      }
      friend bool operator<(const mapped_iterator &left, const mapped_iterator &right)
      {
         return left._iter < right._iter ;
      }

      PCOMN_DEFINE_RELOP_FUNCTIONS(friend, mapped_iterator) ;

   private:
      typename container_reference::member_type _container ;
      Iterator _iter ;
} ;

/******************************************************************************/
/** Wrapper over any iterator converting dereferenced value
*******************************************************************************/
template<typename Iterator, typename Converter>
class xform_iterator :
         public std::iterator<typename std::iterator_traits<Iterator>::iterator_category,
                              std::result_of_t<Converter(decltype(*std::declval<Iterator>()))>,
                              typename std::iterator_traits<Iterator>::difference_type,
                              void, void> {

      typedef std::iterator<typename std::iterator_traits<Iterator>::iterator_category,
                            std::result_of_t<Converter(decltype(*std::declval<Iterator>()))>,
                            typename std::iterator_traits<Iterator>::difference_type,
                            void, void> ancestor ;
   public:
      typedef typename ancestor::value_type value_type ;
      typedef typename ancestor::difference_type difference_type ;

      xform_iterator() = default ;
      xform_iterator(const Iterator &i, const Converter &c = Converter()) : _data(i, c) {}

      xform_iterator &operator++() { ++iter() ; return *this ; }
      xform_iterator &operator--() { --iter() ; return *this ; }

      // Post(inc|dec)rement
      PCOMN_DEFINE_POSTCREMENT_METHODS(xform_iterator) ;

      xform_iterator &operator+=(difference_type diff) { iter() += diff ; return *this ; }
      xform_iterator &operator-=(difference_type diff) { iter() -= diff ; return *this ; }

      xform_iterator operator+(difference_type diff) const
      {
         return xform_iterator(*this) += diff ;
      }
      xform_iterator operator-(difference_type diff) const
      {
         return xform_iterator(*this) -= diff ;
      }
      difference_type operator-(const xform_iterator &other) const
      {
         return iter() - other.iter() ;
      }

      value_type operator*() const { return converter()(*iter()) ; }

      friend bool operator==(const xform_iterator &left, const xform_iterator &right)
      {
         return left.iter() == right.iter() ;
      }
      friend bool operator<(const xform_iterator &left, const xform_iterator &right)
      {
         return left.iter() < right.iter() ;
      }

      PCOMN_DEFINE_RELOP_FUNCTIONS(friend, xform_iterator) ;

   private:
      std::tuple<Iterator, Converter> _data ; /* Gain advantage of (possible) empty base
                                               * class optimization for stateless
                                               * Converter */

      Iterator &iter() { return std::get<0>(_data) ; }
      const Iterator &iter() const { return std::get<0>(_data) ; }
      const Converter &converter() const { return std::get<1>(_data) ; }
} ;

/******************************************************************************/
/** Wrapper over any iterator converting dereferenced value
*******************************************************************************/
template<typename Iterator, typename Type>
class xform_iterator<Iterator, identity_type<Type> > :
         public std::iterator<typename std::iterator_traits<Iterator>::iterator_category,
                              Type,
                              typename std::iterator_traits<Iterator>::difference_type,
                              void, void> {

      typedef std::iterator<typename std::iterator_traits<Iterator>::iterator_category,
                            Type,
                            typename std::iterator_traits<Iterator>::difference_type,
                            void, void> ancestor ;
   public:
      typedef typename ancestor::value_type value_type ;
      typedef typename ancestor::difference_type difference_type ;

      xform_iterator() : _iter() {}
      xform_iterator(const Iterator &i) : _iter(i) {}

      xform_iterator &operator++() { ++_iter ; return *this ; }
      xform_iterator &operator--() { --_iter ; return *this ; }

      // Post(inc|dec)rement
      PCOMN_DEFINE_POSTCREMENT_METHODS(xform_iterator) ;

      xform_iterator &operator+=(difference_type diff) { _iter += diff ; return *this ; }
      xform_iterator &operator-=(difference_type diff) { _iter -= diff ; return *this ; }

      xform_iterator operator+(difference_type diff) const
      {
         return xform_iterator(*this) += diff ;
      }
      xform_iterator operator-(difference_type diff) const
      {
         return xform_iterator(*this) -= diff ;
      }
      difference_type operator-(const xform_iterator &other) const
      {
         return _iter - other._iter ;
      }

      value_type operator*() const { return value_type(*_iter) ; }

      friend bool operator==(const xform_iterator &left, const xform_iterator &right)
      {
         return left._iter == right._iter ;
      }
      friend bool operator<(const xform_iterator &left, const xform_iterator &right)
      {
         return left._iter < right._iter ;
      }

      PCOMN_DEFINE_RELOP_FUNCTIONS(friend, xform_iterator) ;

   private:
      Iterator _iter ;
} ;

/******************************************************************************/
/** Block buffer iterator traits
*******************************************************************************/
template<typename Buffer>
struct buffer_iterator_traits {
      /// Get the block interval of a buffer starting from @a offset.
      /// @param buffer
      /// @param offset Offset from the start of the whole buffer
      /// @return Pair (ptr at @a offset, available from @a offset)
      static std::pair<const void *, size_t> next_block(const Buffer &buffer, size_t offset)
      {
         return buffer.next_block(offset) ;
      }

      /// Get the whole buffer size (in bytes)
      static size_t size(const Buffer &buffer) { return buffer.size() ; }
} ;

/******************************************************************************/
/** Forward iterator over a memory buffer
*******************************************************************************/
template<typename Buffer>
class const_buffer_iterator :
         public std::iterator<std::forward_iterator_tag, char, ptrdiff_t, const char *, const char &> {
   public:
      typedef Buffer                         buffer_type ;
      typedef buffer_iterator_traits<Buffer> traits_type ;

      const_buffer_iterator() : _buf(), _ptr(), _offset(0), _endblock(0) {}

      /// Create a begin iterator over a buffer
      explicit const_buffer_iterator(const buffer_type &b) :
         _buf(&b), _ptr(), _offset(0), _endblock(0)
      {
         next_block() ;
      }
      /// Create an end iterator over a buffer
      const_buffer_iterator(const buffer_type &b, ptrdiff_t) :
         _buf(&b), _ptr(), _offset(traits_type::size(b)), _endblock(_offset)
      {}

      char operator*() const { NOXCHECK(_ptr) ; return *_ptr ; }

      const_buffer_iterator &operator++()
      {
         ++_offset ; ++_ptr ;
         if (_offset >= _endblock)
         {
            next_block() ;
            // Prevent to increment the iterator past the end of the buffer
            PCOMN_VERIFY(_offset <= _endblock) ;
         }
         return *this ;
      }

      PCOMN_DEFINE_POSTCREMENT(const_buffer_iterator, ++) ;

      friend bool operator==(const const_buffer_iterator &x, const const_buffer_iterator &y)
      {
         return x._offset == y._offset && x._buf == y._buf ;
      }
      friend bool operator!=(const const_buffer_iterator &x, const const_buffer_iterator &y)
      {
         return !(x == y) ;
      }
      friend ptrdiff_t operator-(const const_buffer_iterator &x, const const_buffer_iterator &y)
      {
         return x._offset - y._offset ;
      }

   private:
      const buffer_type *  _buf ;
      const char *         _ptr ;
      size_t               _offset ;
      size_t               _endblock ;

      const buffer_type &buf() { PCOMN_VERIFY(_buf) ; return *_buf ; }
      void next_block() ;
} ;

template<typename Buffer>
void const_buffer_iterator<Buffer>::next_block()
{
   const std::pair<const void *, size_t> &block = traits_type::next_block(buf(), _offset) ;
   if (block.first && block.second)
   {
      _ptr = static_cast<const char *>(block.first) ;
      _endblock += block.second ;
   }
}

/******************************************************************************/
/** Abstract block buffer interface, iterable by const_buffer_iterator
*******************************************************************************/
struct block_buffer {
      virtual ~block_buffer() {}
      /// Get the whole buffer size (in bytes)
      virtual size_t size() const = 0 ;
      /// Get the block interval of a buffer starting from @a offset.
      /// @param offset Offset from the start of the whole buffer
      /// @return Pair (ptr at @a offset, available from @a offset)
      virtual std::pair<const void *, size_t> next_block(size_t offset) const = 0 ;
} ;

typedef const_buffer_iterator<block_buffer> const_block_buffer_iterator ;

/*******************************************************************************
 Helper functions that construct adapted iterators on-the-fly
*******************************************************************************/
template<typename Container, typename Iter>
inline mapped_iterator<Container, Iter>
mapped_iter(Container &c, const Iter &i)
{
   return mapped_iterator<Container, Iter>(c, i) ;
}

template<typename Container, typename Iter>
inline mapped_iterator<const Container, Iter>
const_mapped_iter(const Container &c, const Iter &i)
{
   return mapped_iterator<const Container, Iter>(c, i) ;
}

template<typename Converter, typename Iterator>
inline xform_iterator<Iterator, Converter>
xform_iter(const Iterator &i, Converter &&c)
{
   return {i, std::forward<Converter>(c)} ;
}

template<typename Type, typename Iterator>
inline xform_iterator<Iterator, identity_type<Type> >
xform_iter(const Iterator &i)
{
   return xform_iterator<Iterator, identity_type<Type> >(i) ;
}

template<typename Converter, typename Container>
inline auto xform_range(const Container &cont, Converter &&cvt) ->
   decltype(std::make_pair(xform_iter(std::begin(cont), std::forward<Converter>(cvt)),
                           xform_iter(std::end(cont), std::forward<Converter>(cvt))))
{
   return std::make_pair(xform_iter(std::begin(cont), std::forward<Converter>(cvt)),
                         xform_iter(std::end(cont), std::forward<Converter>(cvt))) ;
}

template<typename Iterator>
inline xform_iterator<Iterator, select1st> mapkey_iter(const Iterator &i)
{
   return {i} ;
}

template<typename Counter>
inline count_iterator<Counter> count_iter(const Counter &c)
{
   return count_iterator<Counter>(c) ;
}

} // end of namespace pcomn

namespace std {
/*******************************************************************************
 Allow to use range-based for std::pair<Iterator, Iterator>
*******************************************************************************/
template<typename I>
inline enable_if_t<pcomn::is_iterator<I>::value, I> begin(const pair<I, I> &range)
{
   return range.first ;
}

template<typename I>
inline enable_if_t<pcomn::is_iterator<I>::value, I> end(const pair<I, I> &range)
{
   return range.second ;
}

template<typename I>
inline auto size(const pair<I, I> &range) -> decltype(range.second - range.first)
{
   return range.second - range.first ;
}
} // end of namespace std

#endif /* __PCOMN_ITERATOR_H */
