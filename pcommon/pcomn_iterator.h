/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_ITERATOR_H
#define __PCOMN_ITERATOR_H
/*******************************************************************************
 FILE         :   pcomn_iterator.h
 COPYRIGHT    :   Yakov Markovitch, 2006-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Iterators:
                  collection_iterator<RandomAccessContainer>
                  mapped_iterator<Container, Iterator, Reference>
                  const_mapped_iterator<Container, Iterator, Reference>
                  xform_iterator<Iterator, Converter, Value>

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   19 Dec 2006
*******************************************************************************/
#include <pcomn_meta.h>
#include <pcommon.h>

#include <utility>
#include <iterator>
#include <functional>

namespace pcomn {

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

/*******************************************************************************
                     template<struct RandomAccessContainer>
                     struct collection_iterator
*******************************************************************************/
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
/** A forward iterator which counts how far it is advanced (i.e. how many times
 operator++() is called).
*******************************************************************************/
template<typename Counter = size_t>
struct count_iterator :
         std::iterator<std::forward_iterator_tag, Counter, ptrdiff_t, void, Counter> {

      explicit count_iterator(Counter init_count = Counter()) : _count(init_count) {}

      Counter count() const { return _count ; }

      Counter operator*() const { return count() ; }

      count_iterator &operator++()
      {
         ++_count ;
         return *this ;
      }
      PCOMN_DEFINE_POSTCREMENT(count_iterator, ++) ;

      friend bool operator==(const count_iterator &lhs, const count_iterator &rhs)
      {
         return lhs._count == rhs._count ;
      }
      friend bool operator!=(const count_iterator &lhs, const count_iterator &rhs)
      {
         return !(lhs == rhs) ;
      }
   private:
      Counter _count ;
} ;

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

/*******************************************************************************
 append_iterator
*******************************************************************************/
template<typename Container, bool has_key = has_key_type<Container>::value>
struct append_iterator_base {
      Container &container() { return *_container ; }
   protected:
      explicit append_iterator_base(Container &c) : _container(&c) {}
      void put(typename Container::const_reference value) { _container->push_back(value) ; }
   private:
      Container *_container ;
} ;

template<typename Container>
struct append_iterator_base<Container, true> {
      Container &container() { return *_container ; }
   protected:
      explicit append_iterator_base(Container &c) : _container(&c) {}
      void put(typename Container::const_reference value) { _container->insert(value) ; }
   private:
      Container *_container ;
} ;

/******************************************************************************/
/** Output iterator that turns assignment into appending to any container.

 Output iterator, constructed from a container. Assigning a value to such
 iterator appends it to the container, much like std::back_insert_iterator do;
 but, in contrast to std::back_insert_iterator it works with any container type,
 including std::map, std::set, and std:: unordered containers.

 Use the appender() function to create append_iterator.
*******************************************************************************/
template<typename Container>
class append_iterator :
         public std::iterator<std::output_iterator_tag, void, void, void, void>,
         public append_iterator_base<Container> {

      typedef append_iterator_base<Container> ancestor ;
   public:
      /// A nested typedef for the type of whatever container you used.
      typedef Container container_type ;

      append_iterator(container_type &c) : ancestor(c) {}

      append_iterator &operator=(typename container_type::const_reference value)
      {
         ancestor::put(value) ;
         return *this ;
      }
      append_iterator &operator*() { return *this ; }
      append_iterator &operator++() { return *this ; }
      append_iterator &operator++(int) { return *this ; }
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

 Output iterator, constructed from a functor. Assigning a value to such
 iterator calls that functor, passing it a value

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
template<typename Container, typename Iterator, typename Reference>
class const_mapped_iterator ;

/// @cond
namespace detail {
template<typename C, bool, bool = has_mapped_type<C>::value> struct container_reference ;
template<typename C> struct container_reference<C, false, false> { typedef typename C::reference type ; } ;
template<typename C> struct container_reference<C, false, true>  { typedef typename C::mapped_type &type ; } ;
template<typename C> struct container_reference<C, true, false>  { typedef typename C::const_reference type ; } ;
template<typename C> struct container_reference<C, true, true>   { typedef typename C::mapped_type const &type ; } ;
}
/// @endcond

template<typename Container, typename Iterator,
         typename Reference = typename detail::container_reference<Container, false>::type>
class mapped_iterator :
         public std::iterator<typename std::iterator_traits<Iterator>::iterator_category,
                              typename Container::value_type,
                              typename std::iterator_traits<Iterator>::difference_type,
                              void, Reference> {

      friend class const_mapped_iterator<Container, Iterator, Reference> ;

      typedef std::iterator<typename std::iterator_traits<Iterator>::iterator_category,
                            typename Container::value_type,
                            typename std::iterator_traits<Iterator>::difference_type,
                            void, Reference> ancestor ;
   public:
      typedef typename ancestor::value_type        value_type ;
      typedef typename ancestor::difference_type   difference_type ;

      mapped_iterator() :
         _container(NULL)
      {}

      mapped_iterator(Container &c, const Iterator &i) :
         _container(&c),
         _iter(i)
      {}

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

      Reference operator*() const { return (*_container)[*_iter] ; }

   private:
      Container  *_container ;
      Iterator    _iter ;
} ;

/******************************************************************************/
/** See mapped_iterator
*******************************************************************************/
template<typename Container, typename Iterator,
         typename Reference = typename detail::container_reference<Container, true>::type>
class const_mapped_iterator :
         public std::iterator<typename std::iterator_traits<Iterator>::iterator_category,
                              typename Container::value_type,
                              typename std::iterator_traits<Iterator>::difference_type,
                              void, Reference> {


      typedef std::iterator<typename std::iterator_traits<Iterator>::iterator_category,
                            typename Container::value_type,
                            typename std::iterator_traits<Iterator>::difference_type,
                            void, Reference> ancestor ;

   public:
      typedef typename ancestor::value_type        value_type ;
      typedef typename ancestor::difference_type   difference_type ;

      const_mapped_iterator() :
         _container(NULL)
      {}

      const_mapped_iterator(const Container &c, const Iterator &i) :
         _container(&c),
         _iter(i)
      {}

      const_mapped_iterator(const mapped_iterator<Container, Iterator> &i) :
         _container(i._container),
         _iter(i._iter)
      {}

      const_mapped_iterator &operator++() { ++_iter ; return *this ; }
      const_mapped_iterator &operator--() { --_iter ; return *this ; }

      // Post(inc|dec)rement
      PCOMN_DEFINE_POSTCREMENT_METHODS(const_mapped_iterator) ;

      const_mapped_iterator &operator+=(difference_type diff) { _iter += diff ; return *this ; }
      const_mapped_iterator &operator-=(difference_type diff) { _iter -= diff ; return *this ; }

      const_mapped_iterator operator+(difference_type diff) const
      {
         return const_mapped_iterator(*this) += diff ;
      }
      const_mapped_iterator operator-(difference_type diff) const
      {
         return const_mapped_iterator(*this) -= diff ;
      }

      difference_type operator-(const const_mapped_iterator &other) const
      {
         return _iter - other._iter ;
      }

      friend bool operator==(const const_mapped_iterator &left,
                             const const_mapped_iterator &right)
      {
         return left._iter == right._iter ;
      }
      friend bool operator<(const const_mapped_iterator &left,
                            const const_mapped_iterator &right)
      {
         return left._iter < right._iter ;
      }

      PCOMN_DEFINE_RELOP_FUNCTIONS(friend, const_mapped_iterator) ;

      Reference operator*() const { return (*_container)[*_iter] ; }

   private:
      const Container  *_container ;
      Iterator          _iter ;
} ;


/******************************************************************************/
/** Wrapper over any iterator converting dereferenced value
*******************************************************************************/
template<typename Iterator, typename Converter, typename Value = typename Converter::result_type>
class xform_iterator :
         public std::iterator<typename std::iterator_traits<Iterator>::iterator_category,
                              Value,
                              typename std::iterator_traits<Iterator>::difference_type,
                              void, void> {

      typedef std::iterator<typename std::iterator_traits<Iterator>::iterator_category,
                            Value,
                            typename std::iterator_traits<Iterator>::difference_type,
                            void, void> ancestor ;
   public:
      typedef typename ancestor::value_type value_type ;
      typedef typename ancestor::difference_type difference_type ;

      xform_iterator() :
         _iter(),
         _converter()
      {}

      xform_iterator(const Iterator &i, const Converter &c = Converter()) :
         _iter(i),
         _converter(c)
      {}

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

      friend bool operator==(const xform_iterator &left, const xform_iterator &right)
      {
         return left._iter == right._iter ;
      }
      friend bool operator<(const xform_iterator &left, const xform_iterator &right)
      {
         return left._iter < right._iter ;
      }

      PCOMN_DEFINE_RELOP_FUNCTIONS(friend, xform_iterator) ;

      value_type operator*() const { return _converter(*_iter) ; }

   private:
      Iterator  _iter ;
      Converter _converter ;
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
 *
 * is_iterator<Iterator, Category = std::input_iterator_tag>
 *
 * Type trait to check if the type is an iterator of at least specified category
 *
 *******************************************************************************/
namespace detail {
template<typename>
std::false_type test_iterator_category(...) ;
template<typename T>
std::true_type test_iterator_category(typename std::iterator_traits<T>::iterator_category const volatile *) ;
}

/// type trait to check if the type is an iterator of at least specified category
///
template<typename T, typename C = std::input_iterator_tag>
using is_iterator = decltype(detail::test_iterator_category<T>(0)) ;

/*******************************************************************************
 Helper functions that construct adapted iterators on-the-fly
*******************************************************************************/
template<class Container, typename Iter>
inline mapped_iterator<Container, Iter> mapped_iter(Container &c, const Iter &i)
{
   return mapped_iterator<Container, Iter>(c, i) ;
}

template<class Container, typename Iter>
inline const_mapped_iterator<Container, Iter> const_mapped_iter(const Container &c, const Iter &i)
{
   return const_mapped_iterator<Container, Iter>(c, i) ;
}

template<typename Iterator, typename Converter>
inline xform_iterator<Iterator, Converter> xform_iter(const Iterator &i, const Converter &c)
{
   return xform_iterator<Iterator, Converter>(i, c) ;
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
template<typename Iterator>
inline Iterator begin(const pair<Iterator, Iterator> &range,
                      pcomn::identity_type<typename iterator_traits<Iterator>::iterator_category> = {})
{
   return range.first ;
}

template<typename Iterator>
inline Iterator end(const pair<Iterator, Iterator> &range,
                    pcomn::identity_type<typename iterator_traits<Iterator>::iterator_category> = {})
{
   return range.second ;
}
} // end of namespace std

#endif /* __PCOMN_ITERATOR_H */
