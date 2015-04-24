/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_BITARRAY_H
#define __PCOMN_BITARRAY_H
/*******************************************************************************
 FILE         :   pcomn_bitarray.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Bit array. A kind of combination of std::bitset and std::vector<bool>
                  The size of class objects can be specified during construction,
                  like for the std::vector<bool> and unlike std::bitset<N>, where size is to
                  be specified at compile-time. Also like std::vector<bool>, the class has
                  iterators.
                  On the other hand, objects cannot be resized after construction (much like
                  std::bitset) and also the class has bit operators (~, &, |, <<, >>).
                  Class uses reference counting and copy-on-write logic, hence copy/assign
                  operations are cheap.

 CREATION DATE:   27 Jul 2000
*******************************************************************************/
/** @file
    Bit array: a kind of combination of std::bitset and std::vector<bool>.

 The length of bitarray can be specified during construction, unlike std::bitset<N>,
 where size must be specified at compile-time.

 Provides both iterators, like std::vector<bool>, and bit operators (~, &, |, <<,>>),
 like std::bitset). Uses reference counting and copy-on-write logic, so copy/assign
 operations are cheap.
*******************************************************************************/
#include <pcomn_assert.h>
#include <pcomn_buffer.h>
#include <pcomn_integer.h>
#include <pcomn_function.h>

#include <algorithm>
#include <iterator>
#include <iostream>

#include <limits.h>
#include <stdint.h>
#include <stddef.h>

namespace pcomn {

/*******************************************************************************
                     template<typename Element>
                     class bitarray_base
*******************************************************************************/
template<typename Element = unsigned long>
struct bitarray_base {

      /// Get the size of array (in bits)
      size_t size() const { return _size ; }

      /// Get the count of 1 or 0 bit in the array
      size_t count(bool bitval = true) const
      {
         if (_count == ~0UL)
            _count = count_ones() ;
         return bitval ? _count : _size - _count ;
      }

      bool test(size_t pos) const
      {
         NOXCHECK(pos < size()) ;
         return !!(const_elem(pos) & bitmask(pos)) ;
      }

      /// Indicate if there exists '1' bit in this array
      bool any() const
      {
         const element_type * const data = cbits() ;
         return std::any_of(data, data + nelements(), identity()) ;
      }

      /// Indicate if all bits in this array are '1'
      bool all() const
      {
         if (const size_t n = nelements())
         {
            const element_type * const data = cbits() ;
            return
               std::all_of(data, data + (n-1), [](element_type e){ return e == ~element_type() ; }) &&
               (data[n-1] & tailmask()) == tailmask() ;
         }
         return true ;
      }

      /// Get the position of first nonzero bit between 'start' and 'finish'
      ///
      /// If there is no such bit, returns 'finish'
      size_t find_first_bit(size_t start, size_t finish) const ;

      /// Marshall the array of bits into the external platform-independent
      /// representation
      ///
      /// @param memento The buffer where to save the array; if NULL, the function does
      /// not copy anything and returns required storage size
      ///
      /// @return        Required size of buffer (in bytes)
      ///
      size_t mem(void *memento = nullptr) const ;

   protected:
      // The type of element of an array in which we store bits.
      typedef Element element_type ;

      PCOMN_STATIC_CHECK(std::is_integral<element_type>::value && std::is_unsigned<element_type>::value) ;

      /// Bit count per storage element
      static constexpr const size_t BITS_PER_ELEMENT = CHAR_BIT*sizeof(element_type) ;

      constexpr bitarray_base() : _count(0) {}

      // Constructor.
      // Parameters:
      //    sz       -  the size of array (i.e. the number of bits).
      //    initval  -  the initial value of all array items.
      //
      explicit bitarray_base(size_t sz, bool initval = false) : bitarray_base(sz, nullptr)
      {
         initval ? set() : reset() ;
      }

      /// Constructor restores the bit array previously saved by mem()
      ///
      /// @param  memento  The external buffer to restore the array from.
      /// @param  size     The bit array size (in @em bits, @em not in bytes)
      ///
      /// It is supposed the source bitarryr array was previously saved into @a memento
      /// through bitarray::mem() call
      ///
      bitarray_base(const void *memento, size_t sz) : bitarray_base(sz, nullptr)
      {
         if (sz)
            memcpy(memset(mbits(), 0, _elements.size()), memento, mem()) ;
      }

      template<typename InputIterator>
      bitarray_base(InputIterator start, std::enable_if_t<is_iterator<InputIterator>::value, InputIterator> finish) :
         bitarray_base(start, finish, is_iterator<InputIterator, std::random_access_iterator_tag>())
      {}

      bitarray_base(const bitarray_base &) = default ;

      bitarray_base(bitarray_base &&other) :
         _size(other._size),
         _count(other._count),
         _elements(std::move(other._elements))
      {
         other._count = other._size = 0 ;
      }

      ~bitarray_base() = default ;

      bitarray_base &operator=(const bitarray_base &other) = default ;

      bitarray_base &operator=(bitarray_base &&other)
      {
         if (&other != this)
         {
            _size = other._size ;
            _count = other._count ;
            other._count = other._size = 0 ;
            _elements = std::move(other._elements) ;
         }
         return *this ;
      }

      void and_assign(const bitarray_base &source) ;
      void or_assign(const bitarray_base &source) ;
      void xor_assign(const bitarray_base &source) ;
      void andnot_assign(const bitarray_base &source) ;

      void shift_left(int pos) ;
      void shift_right(int pos) ;

      void reset()
      {
         memset(mbits(), 0, _elements.size()) ;
         _count = 0 ;
      }

      void set()
      {
         if (const size_t itemcount = _elements.nitems())
         {
            element_type * const data = mbits() ;
            memset(data, -1, itemcount * sizeof data) ;
            data[itemcount - 1] &= tailmask() ;
            _count = size() ;
         }
      }

      void set(size_t pos, bool val = true)
      {
         NOXCHECK(pos < size()) ;
         element_type &data = mutable_elem(pos) ;
         const element_type mask = bitmask(pos) ;
         val ? (data |= mask) : (data &= ~mask) ;
      }

      void flip() ;
      bool flip(size_t pos)
      {
         NOXCHECK(pos < size()) ;
         return !!((mutable_elem(pos) ^= bitmask(pos)) & bitmask(pos)) ;
      }

      bool equal(const bitarray_base &other) const
      {
         return
            _size == other._size &&
            std::equal(cbits(), cbits() + nelements(), other.cbits()) ;
      }

      /// Indicate if given position is a valid bitarray index
      bool valid_position(size_t pos) { return pos < nelements() ; }

      /// Given a bit position, get the position of an element containing specified bit
      ///
      static constexpr size_t elemndx(size_t pos)
      {
         return pos / BITS_PER_ELEMENT ;
      }

      /// Given a bit position, get the index inside the appropriate chunk in bits[]
      /// such that 0 <= index < BITS_PER_ELEMENT.
      ///
      static constexpr size_t bitndx(size_t pos)
      {
         return pos & (BITS_PER_ELEMENT - 1) ;
      }

      static constexpr element_type bitmask(size_t pos)
      {
         return std::integral_constant<element_type, 1>::value << bitndx(pos) ;
      }

      constexpr element_type tailmask() const
      {
         return ~(~element_type() << (bitndx(_size - 1) + 1)) ;
      }

      void swap(bitarray_base &other) noexcept
      {
         std::swap(other._size, _size) ;
         std::swap(other._count, _count) ;
         _elements.swap(other._elements) ;
      }

   private:
      size_t                                 _size  = 0UL ;
      mutable size_t                         _count ;
      typed_buffer<element_type, cow_buffer> _elements ;

   private:
      bitarray_base(size_t sz, nullptr_t) :
         _size(sz),
         _count(0),
         _elements((sz + (BITS_PER_ELEMENT - 1))/BITS_PER_ELEMENT)
      {}

      size_t nelements() const { return _elements.nitems() ; }
      size_t count_ones() const ;

      element_type *mbits()
      {
         _count = ~0UL ;
         return _elements.get() ;
      }
      const element_type *cbits() const { return _elements ; }

      element_type &mutable_elem(size_t bitpos) { return mbits()[elemndx(bitpos)] ; }
      const element_type &const_elem(size_t bitpos) const { return cbits()[elemndx(bitpos)] ; }

      template<typename InputIterator>
      bitarray_base(InputIterator &start, InputIterator &finish, std::false_type) :
         bitarray_base(std::vector<bool>(start, finish), Instance)
      {}

      template<typename RandomAccessIterator>
      bitarray_base(RandomAccessIterator &start, RandomAccessIterator &finish, std::true_type) :
         bitarray_base(std::distance(start, finish))
      {
         if (size())
         {
            element_type * const data = mbits() ;
            for (size_t pos = 0 ; pos < size() ; ++pos, ++start)
               if (*start)
               {
                  data[elemndx(pos)] |= bitmask(pos) ;
                  ++_count ;
               }
         }
      }

      bitarray_base(std::vector<bool> &&bits, Instantiate) :
         bitarray_base(bits.begin(), bits.end())
      {}
} ;


/******************************************************************************/
/** Like std::bitset, but has its size specified at runtime paramete
*******************************************************************************/
class bitarray : private bitarray_base<unsigned long> {
      typedef bitarray_base<unsigned long> ancestor ;
   public:
      class                      bit_reference ;
      template<typename> class   bit_iterator ;

      typedef bit_iterator<const bit_reference &>  iterator ;
      typedef bit_iterator<bool>                   const_iterator ;

      typedef bool            value_type ;
      typedef bit_reference   reference ;

      /************************************************************************/
      /** Proxy class representing a reference to a bit in pcomn::bitarray
      *************************************************************************/
      class bit_reference {
         public:
            const bit_reference &operator=(bool val) const
            {
               _ref->set(_pos, val) ;
               return *this ;
            }

            const bit_reference &operator=(const bit_reference &source) const
            {
               return *this = source._ref->test(source._pos) ;
            }

            bool operator~() const { return !_ref->test(_pos) ; }
            operator bool() const { return _ref->test(_pos) ; }

            const bit_reference &flip() const
            {
               _ref->flip(_pos) ;
               return *this ;
            }

         private:
            bitarray * _ref ;
            ptrdiff_t  _pos ;

         private:
            friend bitarray ;
            template<typename> friend class bit_iterator ;

            bit_reference() : _ref(), _pos() {}
            bit_reference(bitarray *r, ptrdiff_t p) : _ref(r), _pos(p) {}
            bit_reference(const bit_reference &) = default ;
      } ;

      /************************************************************************/
      /** Implementor of random-access iterator over bitarray bits
      *************************************************************************/
      template<typename R>
      struct bit_iterator : std::iterator<std::random_access_iterator_tag, R, ptrdiff_t, void, void> {
            bit_iterator() = default ;
            bit_iterator(const bit_iterator &) = default ;

            bit_iterator(std::conditional_t<std::is_same<R, bool>::value, const bitarray, bitarray> &arr,
                         ptrdiff_t pos = 0) :
               _ref(const_cast<bitarray *>(&arr), pos)
            {}

            R operator*() const { return _ref ; }

            bit_iterator &operator=(const bit_iterator &src)
            {
               _ref._ref = src._ref._ref ;
               _ref._pos = src._ref._pos ;
               return *this ;
            }

            bit_iterator &operator+=(ptrdiff_t diff)
            {
               _ref._pos += diff ;
               return *this ;
            }
            bit_iterator &operator-=(ptrdiff_t diff) { return *this += -diff ; }

            bit_iterator &operator++() { ++_ref._pos ; return *this ; }
            bit_iterator &operator--() { --_ref._pos ; return *this ; }

            PCOMN_DEFINE_POSTCREMENT_METHODS(bit_iterator) ;
            PCOMN_DEFINE_NONASSOC_ADDOP_METHODS(bit_iterator, ptrdiff_t) ;

            friend ptrdiff_t operator-(const bit_iterator &x, const bit_iterator &y)
            {
               return x.pos() - y.pos() ;
            }

            friend bool operator==(const bit_iterator &x, const bit_iterator &y)
            {
               return x.pos() == y.pos() ;
            }
            friend bool operator<(const bit_iterator &x, const bit_iterator &y)
            {
               return x.pos() < y.pos() ;
            }

            PCOMN_DEFINE_RELOP_FUNCTIONS(friend, bit_iterator) ;

         private:
            bit_reference _ref ;

            ptrdiff_t pos() const { return _ref._pos ; }
      } ;

      /************************************************************************/
      /** A constant iterator over a bit set, which traverses bit @em positions
       instead of bit @em values.

       That is, positional_iterator successively returns positions of a bitarray
       where bits are set to 'true'.
      *************************************************************************/
      struct positional_iterator : std::iterator<std::forward_iterator_tag, ptrdiff_t, ptrdiff_t> {

            positional_iterator(const bitarray &array, ptrdiff_t pos = 0) :
               _ref(&array),
               _pos(array.find_first_bit(pos, array.size()))
            {}

            ptrdiff_t operator*() const { return _pos ; }

            positional_iterator &operator++()
            {
               _pos = _ref->find_first_bit(_pos + 1, _ref->size()) ;
               return *this ;
            }
            PCOMN_DEFINE_POSTCREMENT(positional_iterator, ++) ;

            bool operator==(const positional_iterator &rhs) const
            {
               NOXCHECK(_ref == rhs._ref) ;
               return _pos == rhs._pos ;
            }
            bool operator!=(const positional_iterator &rhs) const { return !(*this == rhs) ; }

         private:
            const bitarray *_ref ;
            ptrdiff_t       _pos ;
      } ;

   public:
      // Inherited public members
      //
      using ancestor::count ;
      using ancestor::size ;
      using ancestor::test ;
      using ancestor::any ;
      using ancestor::all ;
      using ancestor::find_first_bit ;
      using ancestor::mem ;

      constexpr bitarray() = default ;

      // Constructor.
      // Parameters:
      //    sz       -  the size of array (i.e. the number of bits).
      //    initval  -  the initial value of all array items.
      //
      explicit bitarray(size_t sz, bool initval = false) : ancestor(sz, initval) {}

      /// Constructor restores the bit array previously saved by mem()
      ///
      /// @param  memento  The external buffer to restore the array from.
      /// @param  size     The bit array size (in @em bits, @em not in bytes)
      ///
      /// It is supposed the source bitarryr array was previously saved into @a memento
      /// through bitarray::mem() call
      ///
      bitarray(void *memento, size_t sz) : ancestor(memento, sz) {}

      template<typename InputIterator>
      bitarray(InputIterator start, std::enable_if_t<is_iterator<InputIterator>::value, InputIterator> finish) :
         ancestor(start, finish)
      {}

      bitarray(const bitarray &) = default ;
      bitarray(bitarray &&) = default ;

      bitarray &operator=(const bitarray &) = default ;
      bitarray &operator=(bitarray &&) = default ;

      bitarray &operator&=(const bitarray &source)
      {
         and_assign(source) ;
         return *this ;
      }

      bitarray &operator|=(const bitarray &source)
      {
         or_assign(source) ;
         return *this ;
      }

      bitarray &operator^=(const bitarray &source)
      {
         xor_assign(source) ;
         return *this ;
      }

      bitarray &operator-=(const bitarray &source)
      {
         andnot_assign(source) ;
         return *this ;
      }

      bitarray &operator<<=(int pos)
      {
         shift_left(pos) ;
         return *this ;
      }

      bitarray &operator>>=(int pos)
      {
         shift_right(pos) ;
         return *this ;
      }

      bitarray &mask(const bitarray &source)
      {
         ancestor::andnot_assign(source) ;
         return *this ;
      }

      bitarray &set()
      {
         ancestor::set() ;
         return *this ;
      }

      bitarray &set(size_t pos, bool val = true)
      {
         ancestor::set(pos, val) ;
         return *this ;
      }

      bitarray &reset()
      {
         ancestor::reset() ;
         return *this ;
      }

      bitarray &reset(size_t pos) { return set(pos, false) ; }

      bitarray operator~() const
      {
         bitarray tmp(*this) ;
         return tmp.flip() ;
      }

      bitarray &flip()
      {
         ancestor::flip() ;
         return *this ;
      }

      bool flip(size_t pos) { return ancestor::flip(pos) ; }

      // element access
      //
      reference operator[](size_t pos) { return reference(this, pos) ; }
      bool operator[](size_t pos) const { return test(pos) ; }

      bool operator==(const bitarray &other) const { return equal(other) ; }
      bool operator!=(const bitarray &other) const { return !equal(other) ; }

      /// Indicate if all the bits in this array are '0'
      bool none() const { return !any() ; }

      bitarray operator<<(int pos) const
      {
         bitarray tmp(*this) ;
         tmp <<= pos ;
         return tmp ;
      }

      bitarray operator>>(int pos) const
      {
         bitarray tmp(*this) ;
         tmp >>= pos ;
         return tmp ;
      }

      void swap(bitarray &other)
      {
         ancestor::swap(other) ;
      }

      //
      // STL-like access
      //
      iterator begin() { return iterator(*this, 0) ; }
      iterator end() { return iterator(*this, size()) ; }
      const_iterator begin() const { return const_iterator(*this, 0) ; }
      const_iterator end() const { return const_iterator(*this, size()) ; }
      const_iterator cbegin() const { return begin() ; }
      const_iterator cend() const { return end() ; }

      positional_iterator begin_positional() const { return positional_iterator(*this) ; }
      positional_iterator end_positional() const { return positional_iterator(*this, size()) ; }
} ;

#define PCOMN_BIN_BITOP(op,lhs,rhs)             \
const bitarray *l ;                             \
const bitarray *r ;                             \
if (lhs.size() >= rhs.size())                   \
{                                               \
   l = &lhs ;                                   \
   r = &rhs ;                                   \
}                                               \
else                                            \
{                                               \
   l = &rhs ;                                   \
   r = &lhs ;                                   \
}                                               \
return std::move(bitarray(*l).operator op(*r)) ;

inline bitarray operator&(const bitarray &left, const bitarray &right)
{
   PCOMN_BIN_BITOP(&=, left, right) ;
}

inline bitarray operator|(const bitarray &left, const bitarray &right)
{
   PCOMN_BIN_BITOP(|=, left, right) ;
}

inline bitarray operator^(const bitarray &left, const bitarray &right)
{
   PCOMN_BIN_BITOP(^=, left, right) ;
}

inline bitarray operator-(const bitarray &left, const bitarray &right)
{
   return std::move(bitarray(left) -= right) ;
}

#undef PCOMN_BIN_BITOP

/*******************************************************************************
 bitarray_base<Element>
*******************************************************************************/
template<typename Element>
void bitarray_base<Element>::flip()
{
   const size_t n = nelements() ;
   if (!n)
      return ;

   element_type *data = mbits() ;
   for (element_type * const end = data + n ; data != end ; ++data)
      *data ^= ~element_type() ;

   *--data &= tailmask() ;
}

template<typename Element>
size_t bitarray_base<Element>::count_ones() const
{
   size_t cnt = 0 ;
   for (const element_type *data = cbits(), *end = data + nelements() ; data != end ; ++data)
      cnt += bitop::bitcount(*data) ;

   return cnt ;
}

template<typename Element>
void bitarray_base<Element>::and_assign(const bitarray_base<Element> &source)
{
   const element_type *source_data = source.cbits() ;
   if (cbits() == source_data)
      return ;

   const int minsize = std::min(_size, source._size) ;
   const int full = minsize / BITS_PER_ELEMENT ;
   const int elem = std::min(nelements(), source.nelements()) ;

   element_type *data = mbits() ;
   for (int i = 0 ; i < full ; ++i)
      data[i] &= source_data[i] ;
   if (full < elem)
      data[full] &= source_data[full] | (~0 << (minsize % BITS_PER_ELEMENT)) ;
}

template<typename Element>
void bitarray_base<Element>::or_assign(const bitarray_base<Element> &source)
{
   const element_type *source_data = source.cbits() ;
   if (cbits() == source_data)
      return ;

   const int minsize = std::min(_size, source._size) ;
   const int full = minsize / BITS_PER_ELEMENT ;
   const int elem = std::min(nelements(), source.nelements()) ;

   element_type *data = mbits() ;
   for (int i = 0 ; i < full ; ++i)
      data[i] |= source_data[i] ;
   if (full < elem)
      data[full] |= source_data[full] & ~(~0 << (minsize % BITS_PER_ELEMENT)) ;
}

template<typename Element>
void bitarray_base<Element>::xor_assign(const bitarray_base<Element> &source)
{
   int minsize = std::min(_size, source._size) ;
   int full = minsize / BITS_PER_ELEMENT ;
   int elem = std::min(nelements(), source.nelements()) ;
   element_type *data = mbits() ;
   const element_type *source_data = source.cbits() ;
   for (int i = 0 ; i < full ; ++i)
      data[i] ^= source_data[i] ;
   if (full < elem)
      data[full] ^= source_data[full] & ~(~0 << (minsize % BITS_PER_ELEMENT)) ;
}

template<typename Element>
void bitarray_base<Element>::andnot_assign(const bitarray_base<Element> &source)
{
   const int minsize = std::min(_size, source._size) ;
   const int full = minsize / BITS_PER_ELEMENT ;
   const int elem = std::min(nelements(), source.nelements()) ;

   element_type *data = mbits() ;
   const element_type *source_data = source.cbits() ;
   for (int i = 0 ; i < full ; ++i)
      data[i] &= ~source_data[i] ;
   if (full < elem)
      data[full] &= ~(source_data[full] & ~(~0 << (minsize % BITS_PER_ELEMENT))) ;
}

template<typename Element>
void bitarray_base<Element>::shift_left(int pos)
{
   if (!pos)
      return ;
   long i ;
   for (i = _size - 1 ; i >= pos ; --i)
      set(i, test(i - pos)) ;
   while(i)
      set(i--, false) ;
}

template<typename Element>
void bitarray_base<Element>::shift_right(int pos)
{
   if (!pos)
      return ;
   long i ;
   for (i = 0 ; i < _size - pos ; ++i)
      set(i, test(i + pos)) ;
   while(i < _size)
      set(i++, false) ;
}

template<typename Element>
size_t bitarray_base<Element>::mem(void *memento) const
{
   const size_t memsize = (size() + 7) / 8 ;
   if (char * const out = static_cast<char *>(memento))
   {
      const element_type * const data = cbits() ;
#ifndef PCOMN_CPU_BIG_ENDIAN
      memcpy(out, data, memsize) ;
#else
      for (size_t i = 0 ; i < memsize ; ++i)
      {
         const char *item = reinterpret_cast<const char *>(data + i / sizeof(element_type)) ;
         out[i] = item[sizeof(element_type) - 1 - i % sizeof(element_type)] ;
      }
#endif
   }
   return memsize ;
}

template<typename Element>
size_t bitarray_base<Element>::find_first_bit(size_t start, size_t finish) const
{
   NOXCHECK(finish <= size()) ;

   if (start >= finish)
      return finish ;

   size_t pos = elemndx(start) ;
   const element_type *bits = cbits() + pos ;
   element_type el = *bits >> bitndx(start) ;
   if (!el)
   {
      size_t to = elemndx(finish) ;
      do {
         if (++pos > to)
            return finish ;
         el = *++bits ;
      } while (!el) ;
      start = pos * BITS_PER_ELEMENT ;
   }
   while (!(el & 1) && start < finish)
   {
      el >>= 1 ;
      ++start ;
   }
   return start ;
}

/******************************************************************************/
/** swap specialization for pcomn::bitarray.
*******************************************************************************/
PCOMN_DEFINE_SWAP(bitarray) ;

/*******************************************************************************
 Stream output
*******************************************************************************/
inline std::ostream &operator<<(std::ostream &os, const bitarray &data)
{
    std::copy(data.begin(), data.end(), std::ostream_iterator<int>(os)) ;
    return os ;
}

} // end of namespace pcomn

#endif /* __PCOMN_BITARRAY_H */
