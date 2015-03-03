/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_RANGE_H
#define __PCOMN_RANGE_H
/*******************************************************************************
 FILE         :   pcomn_range.h
 COPYRIGHT    :   Yakov Markovitch, 2006-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Rangelib mini-library made after the STLSoft rangelib

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   25 Dec 2006
*******************************************************************************/
#include <pcomn_assert.h>

#include <iterator>
#include <type_traits>
#include <algorithm>

#include <stddef.h>

/// Check this macro in headers that provide range algorithms to find out whether
/// rangelib is present.
#define PCOMN_USE_RANGES 1

namespace pcomn {

/******************************************************************************
 Range tags
*******************************************************************************/
/// Denotes a deriving class as being a Notional Range
///
/// All ranges must be derived from notional_range_tag directly or indirectly
///
struct notional_range_tag {} ;

/// Denotes a deriving class as being an Iterable Range
struct iterable_range_tag : notional_range_tag {} ;

/// Range tag for ranges ofer "collections", i.e. random-access sequences with defined
/// operator[](int_type) and no iterators
struct collection_range_tag : public notional_range_tag {} ;

/******************************************************************************/
/** Unary type trait that checks whether the given type is Rangelib-like range
*******************************************************************************/
template<typename T>
struct is_range : std::is_base_of<notional_range_tag, T> {} ;

} // end of namespace pcomn

/******************************************************************************
 Range increment operators
*******************************************************************************/
#define RETURN_IF_RANGE(range_type, return_type)                        \
   typename std::enable_if<pcomn::is_range<range_type>::value &&        \
                           std::is_convertible<decltype((void)std::declval<range_type>().advance()), void>::value, \
                           return_type>::type

/// Advance the current position in the range
template<typename R>
inline RETURN_IF_RANGE(R, R &) operator++(R &r)
{
   r.advance() ;
   return r ;
}

/// Advance the current position in the range
template<typename R>
inline RETURN_IF_RANGE(R, R &&) operator++(R &&r)
{
   r.advance() ;
   return std::move(r) ;
}

template<typename R>
inline RETURN_IF_RANGE(R, R) operator++(R &r, int)
{
   R old (r) ;
   r.advance() ;
   return old ;
}

template<typename R>
inline RETURN_IF_RANGE(R, R) operator++(R &&r, int)
{
   R old (r) ;
   r.advance() ;
   return old ;
}

#undef RETURN_IF_RANGE

namespace pcomn {

/******************************************************************************
 Iterator range
*******************************************************************************/
template<typename I>
struct iterator_range_traits {
      typedef I iterator_type ;

      typedef typename std::iterator_traits<iterator_type>::reference   reference ;
      typedef typename std::iterator_traits<iterator_type>::value_type  value_type ;
} ;

template<typename I, typename T = iterator_range_traits<I> >
struct iterator_range : iterable_range_tag {
   public:
      typedef T                    traits_type ;
      typedef iterable_range_tag   range_tag_type;

      typedef I                                iterator ;
      typedef typename traits_type::value_type value_type;
      typedef typename traits_type::reference  reference;

      template<typename Iter>
      iterator_range(Iter b, Iter e) : _pos(b), _last(e) {}

      explicit operator bool() const { return _pos != _last ; }

      reference operator *() const
      {
         NOXCHECK(*this) ;
         return *_pos ;
      }

      void advance() { ++_pos ; }

      iterator begin() const { return _pos ; }
      iterator end() const { return _last ; }

   private:
      iterator  _pos ;
      iterator  _last ;
} ;

/*******************************************************************************
                     template<typename Collection>
                     class collection_range_base
*******************************************************************************/
template<typename C>
struct collection_range_base : collection_range_tag {

      /// The sequence type
      typedef C                                          collection_type ;
      /// The range category tag type
      typedef collection_range_tag                       range_tag_type ;
      /// The value type
      typedef typename collection_type::value_type       value_type ;

      /// The reference type.
      /// Whether it is mutating ot non-mutating (const), depends on collection's constness.
      typedef typename std::conditional<std::is_const<collection_type>::value,
                                        typename collection_type::const_reference,
                                        typename collection_type::reference>::type reference ;

      /// The difference type
      typedef typename collection_type::difference_type  difference_type ;
      /// The size type
      typedef typename collection_type::size_type        size_type ;

      /// Copy assignment operator.
      collection_range_base &operator=(const collection_range_base &rhs)
      {
         _collection = rhs._collection ;
         _lower_ndx = rhs._lower_ndx ;
         _upper_ndx = rhs._upper_ndx ;
         return *this ;
      }

      /// Indicates whether the range is open.
      explicit operator bool() const { return _lower_ndx != _upper_ndx ; }

      /// Get the number of a collection items in the range.
      size_type size() const { return upper_ndx() - lower_ndx() ; }

      size_type lower_ndx() const { return _lower_ndx ; }
      size_type upper_ndx() const { return _upper_ndx ; }
      collection_type &collection() const { return *_collection ; }

   protected:
      constexpr collection_range_base() : _collection(), _lower_ndx(), _upper_ndx() {}

      collection_range_base(collection_type &collection,
                            difference_type lowerndx, difference_type upperndx) :
         _collection(&collection),
         _lower_ndx(adjusted_pos(lowerndx)),
         _upper_ndx(adjusted_pos(upperndx))
      {
         NOXCHECK(_lower_ndx <= _upper_ndx && _upper_ndx <= collection.size()) ;
      }

      /// Copy constructor.
      collection_range_base(const collection_range_base &rhs) :
         _collection(rhs._collection),
         _lower_ndx(rhs._lower_ndx),
         _upper_ndx(rhs._upper_ndx)
      {}

      template<class OtherCollection>
      collection_range_base(const collection_range_base<OtherCollection> &rhs) :
         _collection(&rhs.collection()),
         _lower_ndx(rhs.lower_ndx()),
         _upper_ndx(rhs.upper_ndx())
      {}

      template<class OtherCollection>
      collection_range_base &operator=(const collection_range_base<OtherCollection> &rhs)
      {
         _collection = &rhs.collection() ;
         _lower_ndx = rhs.lower_ndx() ;
         _upper_ndx = rhs.upper_ndx() ;
         return *this ;
      }

      /// Move the current position in the range one item forward.
      void forward()
      {
         NOXCHECKX(*this, "Attempt to increment the collection range past its end point.") ;
         ++_lower_ndx ;
      }

      /// Move the current position in the range one item backward.
      void backward()
      {
         NOXCHECKX(*this, "Attempt to decrement the collection range past its end point.") ;
         --_upper_ndx ;
      }
   private:
      collection_type * _collection ;
      size_type         _lower_ndx ;
      size_type         _upper_ndx ;

      size_type adjusted_pos(difference_type wraparound_pos) const
      {
         return wraparound_pos < 0
            // Not '+ wraparound_pos' due to integer conversion
            ? collection().size() - (-wraparound_pos)
            : static_cast<size_type>(wraparound_pos) ;
      }
} ;

/*******************************************************************************
                     template<typename Collection>
                     class collection_range
*******************************************************************************/
template<typename C>
class collection_range : public collection_range_base<C> {
      typedef collection_range_base<C> ancestor ;
   public:
      using typename ancestor::collection_type ;
      using typename ancestor::difference_type ;
      using typename ancestor::reference ;

      /// The conversion constructor from the sequence.
      /// The constructor defines an implicit conversion from the collection.
      collection_range(collection_type &collection, difference_type start = 0) :
         ancestor(collection, start, collection.size())
      {}

      collection_range(collection_type &collection,
                       difference_type start,
                       difference_type finish) :
         ancestor(collection, start, finish)
      {}

      /// The conversion constructor from another range type.
      /// The constructor defines an implicit conversion from a colection range
      /// over any compatible collection.
      template<class OtherCollection>
      collection_range(const collection_range<OtherCollection> &rhs) : ancestor(rhs) {}

      /// Copy constructor.
      collection_range(const collection_range &rhs) : ancestor(rhs) {}

      /// Copy assignment operator.
      template<class OtherCollection>
      collection_range &operator=(const collection_range<OtherCollection> &rhs)
      {
         ancestor::operator=(rhs) ;
         return *this ;
      }

      /// Get the current value in the range.
      reference operator*() const
      {
         NOXCHECK(*this) ;
         return this->collection()[this->lower_ndx()] ;
      }

      /// Advance the current position in the range.
      void advance() { this->forward() ; }
} ;

/*******************************************************************************
                     template<typename Collection>
                     class collection_rrange
*******************************************************************************/
template<typename C>
class collection_rrange : public collection_range_base<C> {
      typedef collection_range_base<C> ancestor ;
   public:
      using typename ancestor::collection_type ;
      using typename ancestor::difference_type ;
      using typename ancestor::reference  ;

      collection_rrange(collection_type &collection) : ancestor(collection, 0, collection.size()) {}

      collection_rrange(collection_type &collection,
                        difference_type start, difference_type finish) :
         ancestor(collection, finish, start)
      {}

      /// The conversion constructor from another range type.
      /// The constructor defines an implicit conversion from a colection range
      /// over any compatible collection.
      template<class OtherCollection>
      collection_rrange(const collection_rrange<OtherCollection> &rhs) : ancestor(rhs) {}

      /// Copy constructor.
      collection_rrange(const collection_rrange &rhs) : ancestor(rhs) {}

      /// Copy assignment operator.
      template<class OtherCollection>
      collection_rrange &operator=(const collection_rrange<OtherCollection> &rhs)
      {
         ancestor::operator=(rhs) ;
         return *this ;
      }

      /// Get the current value in the range.
      reference operator*() const
      {
         NOXCHECK(*this) ;
         return this->collection()[this->upper_ndx() - 1] ;
      }

      /// Advance the current position in the range.
      /// From the collection POV, this means moving one item @em backward.
      void advance() { this->backward() ; }
} ;

/*******************************************************************************
 Factory functions
*******************************************************************************/
/// Create a pcomn::collection_range object for a collection.
template<class C>
inline collection_range<C>
make_collection_range(C &collection, typename collection_range<C>::difference_type start = 0)
{
   return collection_range<C>(collection, start) ;
}

/// Create a pcomn::collection_range object for a collection.
template<class C>
inline collection_range<C>
make_collection_range(C &collection,
                      typename collection_range<C>::difference_type start,
                      typename collection_range<C>::difference_type finish)
{
   return collection_range<C>(collection, start, finish) ;
}

/// Create a pcomn::collection_range object for a collection.
/// crange is a short alias for make_collection_range
template<class C>
inline collection_range<C> crange(C &collection, typename collection_range<C>::difference_type start = 0)
{
   return collection_range<C>(collection, start) ;
}

/// @overload
template<class C>
inline collection_range<C> crange(C &collection,
                                  typename collection_range<C>::difference_type start,
                                  typename collection_range<C>::difference_type finish)
{
   return collection_range<C>(collection, start, finish) ;
}

/// Create a reverse collection range object for a collection.
template<class C>
inline collection_rrange<C> make_collection_rrange(C &collection)
{
   return collection_rrange<C>(collection) ;
}

/// Create a reverse collection range object for a collection.
template<class C>
inline collection_rrange<C>
make_collection_rrange(C &collection,
                       typename collection_range<C>::difference_type start,
                       typename collection_range<C>::difference_type finish)
{
   return collection_rrange<C>(collection, start, finish) ;
}

/// Create a reverse collection range object for a collection.
/// @note crrange is a short alias for make_collection_rrange
template<class C>
inline collection_rrange<C> crrange(C &collection)
{
   return collection_rrange<C>(collection) ;
}

/// @overload
template<class C>
inline collection_rrange<C> crrange(C &collection,
                                    typename collection_range<C>::difference_type start,
                                    typename collection_range<C>::difference_type finish)
{
   return collection_rrange<C>(collection, start, finish) ;
}

template<typename Range>
inline typename Range::reference range_current(const Range &range,
                                               typename Range::reference defvalue)
{
   return range ? *range : defvalue ;
}

/******************************************************************************/
/** Range adaptor that runs open until the specified predicate becomes true.

 As an example of such range may serve a range over C string (char pointer), where
 the predicate is logical_not<char>
*******************************************************************************/
template<typename R, typename P = std::logical_not<R> >
class terminated_range : public notional_range_tag {
      typedef R adapted_range_type ;
      typedef P predicate_type ;
   public:
      typedef typename adapted_range_type::reference  reference ;
      typedef typename adapted_range_type::value_type value_type ;

      /// Create a terminated range from from a range and a predicate
      ///
      /// @param r The range which will be terminated
      /// @param p The predicate which will be used to terminate the range
      explicit terminated_range(const adapted_range_type &r, const predicate_type &p = {}) :
         _adapted(r, p)
      {}

      /// Indicates whether the range is open
      explicit operator bool() const { return _adapted.r && !_adapted(*_adapted.r) ; }

      /// Returns the current value in the range
      reference operator *() const
      {
         NOXCHECK(*this) ;
         return *_adapted.r ;
      }

      /// Advances the current position in the range
      void advance()
      {
         NOXCHECK(*this) ;
         ++_adapted.r ;
      }

   private:
      // Use empty base class optimization
      struct ar : predicate_type {
            ar(const adapted_range_type &rn, const predicate_type &p) : predicate_type(p), r(rn) {}
            adapted_range_type r ;
      }
      _adapted ;
} ;

/*******************************************************************************
 terminated_range creator functions
*******************************************************************************/
template<typename R, typename P>
inline terminated_range<R, P> make_terminated_range(R &&r, P &&p)
{
   return terminated_range<R, P>(std::forward<R>(r), std::forward<P>(p)) ;
}

template<typename R>
inline terminated_range<R> make_terminated_range(R &&r)
{
   return terminated_range<R>(std::forward<R>(r)) ;
}

template<typename R, typename P>
inline terminated_range<R, P> trange(R &&r, P &&p)
{
   return terminated_range<R, P>(std::forward<R>(r), std::forward<P>(p)) ;
}

template<typename R>
inline terminated_range<R> trange(R &&r)
{
   return terminated_range<R>(std::forward<R>(r)) ;
}

/*******************************************************************************
 Range versions of STL algorithms
*******************************************************************************/

/*******************************************************************************
 r_copy_n
*******************************************************************************/
template<typename R, typename O>
inline O r_copy_impl(R r, O o, const notional_range_tag &)
{
   for(; r; ++r, ++o)
      *o = *r ;
   return o ;
}

template<typename R, typename O>
inline O r_copy_impl(R r, O o, const iterable_range_tag &)
{
   return std::copy(r.begin(), r.end(), o) ;
}

template<typename R, typename O>
inline O r_copy(R r, O o)
{
   return r_copy_impl(r, o, r) ;
}

/*******************************************************************************
 r_copy_if
*******************************************************************************/
template<typename R, typename O, typename P>
inline O r_copy_if_impl(R r, O o, P pred, const notional_range_tag &)
{
   for(; r ; ++r)
   {
      if(!pred(*r))
         continue ;
      *o = *r ;
      ++o ;
   }
   return o;
}

template<typename R, typename O, typename P>
inline O r_copy_if_impl(R &&r, O o, P pred, const iterable_range_tag &)
{
   return std::copy_if(begin(std::forward<R>(r)), end(std::forward<R>(r)), o) ;
}

template<typename R, typename O, typename P>
inline O r_copy_if(R &&r, O o, P pred)
{
   return r_copy_if_impl(std::forward<R>(r), o, pred, r);
}

/*******************************************************************************
 r_copy_n
*******************************************************************************/
template<typename R, typename O>
inline O r_copy_n_impl(R &r, size_t n, O &out, const notional_range_tag &)
{
   for(; n && r ; ++r, ++out, --n)
      *out = *r ;
   return out ;
}

template<typename R, typename O, typename T>
inline O r_copy_n_impl(R &r, size_t n, O &out, T *)
{
   if (r)
      for(; n ; ++r, ++out, --n)
         *out = *r ;
   return out ;
}

template<typename R, typename O>
inline O r_copy_n(R r, size_t n, O out)
{
   return r_copy_n_impl(r, n, out, r) ;
}

/*******************************************************************************
 r_count
*******************************************************************************/
template<typename R, typename T>
inline size_t r_count_impl(R r, const T &val, const notional_range_tag &)
{
   size_t n ;
   for (n = 0 ; r ; ++r)
      if(val == *r)
         ++n ;
   return n ;
}

template<typename R, typename T>
inline size_t r_count_impl(R &&r, const T &val, const iterable_range_tag &)
{
   using namespace std ;
   return std::count(begin(std::forward<R>(r)), end(std::forward<R>(r)), val) ;
}

template<typename R, typename T>
inline size_t r_count(R &&r, const T &val)
{
   return r_count_impl(std::forward<R>(r), val, r) ;
}

/*******************************************************************************
 r_count_if
*******************************************************************************/
template<typename R, typename P>
inline size_t r_count_if_impl(R r, P pred, const notional_range_tag &)
{
   size_t n ;
   for (n = 0 ; r ; ++r)
      if (pred(*r))
         ++n;
   return n ;
}

template<typename R, typename P>
inline size_t r_count_if_impl(R &&r, P pred, const iterable_range_tag &)
{
   using namespace std ;
   return std::count_if(begin(std::forward<R>(r)), end(std::forward<R>(r)), pred) ;
}

template<typename R, typename P>
inline size_t r_count_if(R &&r, P pred)
{
    return r_count_if_impl(std::forward<R>(r), pred, r) ;
}

/*******************************************************************************
 r_distance
*******************************************************************************/
template<typename R>
inline ptrdiff_t r_distance_impl(R r, const notional_range_tag &)
{
   ptrdiff_t d = 0 ;
   while (r) ++r, ++d ;
   return d ;
}

template<typename R>
inline ptrdiff_t r_distance_impl(R &&r, const iterable_range_tag &)
{
   using namespace std ;
   return std::distance(begin(std::forward<R>(r)), end(std::forward<R>(r))) ;
}

template <typename R>
inline ptrdiff_t r_distance(R &&r)
{
   return r_distance_impl(std::forward<R>(r), r) ;
}
/*******************************************************************************
 r_transform
*******************************************************************************/
template<typename R, typename O, typename UnaryFunction>
inline O r_transform_impl(R &r, O &out, UnaryFunction &fn, const notional_range_tag &)
{
   for(; r ; ++r, ++out)
      *out = fn(*r) ;
   return out ;
}

template<typename R, typename O, typename UnaryFunction>
inline O r_transform_impl(R &&r, O &out, UnaryFunction &fn, const iterable_range_tag &)
{
   using namespace std ;
   return std::transform(begin(std::forward<R>(r)), end(std::forward<R>(r)), out, fn) ;
}

template<typename R, typename O, typename UnaryFunction>
inline O r_transform(R &&r, O out, UnaryFunction fn)
{
   return r_transform_impl(std::forward<R>(r), out, fn, r) ;
}

} // end of namespace pcomn

#endif /* __PCOMN_RANGE_H */
