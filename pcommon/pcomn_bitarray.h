/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_BITARRAY_H
#define __PCOMN_BITARRAY_H
/*******************************************************************************
 FILE         :   pcomn_bitarray.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2014. All rights reserved.
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

#include <algorithm>
#include <iterator>
#include <iostream>

#include <limits.h>
#include <stddef.h>

#ifdef __BORLANDC__
#pragma warn -inl
#endif

namespace pcomn {

/*******************************************************************************
                     template<typename Element>
                     class bitarray_base
*******************************************************************************/
template<typename Element = unsigned long>
class bitarray_base {
   public:
      // count()  -  get number of bits in the array
      //
      size_t count(bool bits = true) const ;
      size_t size() const { return _size ; }

      bool test(size_t pos) const
      {
         NOXCHECK(pos < size()) ;
         return (_bits()[pos / BITS_PER_CHUNK] & ((Element)1 << index(pos))) != 0 ;
      }

      bool any() const ;

      // find_first_bit()  -  get the position of first nonzero bit
      //                      between 'start' and 'finish'
      // If there is no such bit, returns 'finish'
      //
      int find_first_bit(int start, int finish) const ;

      // mem() -  marshall the array of bits into the external platform-independent representation
      // Parameters:
      //    memento  -  the buffer where to save the array. If this parameter is NULL,
      //                this function simply returns the required size of buffer (in bytes)
      // Returns:
      //    The size of buffer required to save the array.
      //
      size_t mem(char *memento = NULL) const ;

   protected:
      // The type of element of an array in which we store bits.
      typedef Element element_type ;

      enum {
         BITS_PER_CHUNK    = CHAR_BIT*sizeof(element_type), // Number of bits in an array element.
      } ;

      // Constructor.
      // Parameters:
      //    sz       -  the size of array (i.e. the number of bits).
      //    initval  -  the initial value of all array items.
      //
      explicit bitarray_base(size_t sz = 0, bool initval = false) :
         _size(sz),
         _elements(sz ? (1 + ((sz - 1) / BITS_PER_CHUNK)) : 0)
      {
         initval ? set() : reset() ;
      }

      // Constructor. Restores the bit array previously saved by mem()
      // Parameters:
      //    memento  -  the external buffer to restore the array from. It is supposed the array
      //                was previously saved into this buffer through bitarray::mem() call
      //    size     -  the bit array size (in bits, NOT in bytes)
      //
      bitarray_base(const char *memento, size_t sz) :
         _size(sz),
         _elements(sz ? (1 + ((sz - 1) / BITS_PER_CHUNK)) : 0)
      {
         if (sz)
            memcpy(memset(_bits(), 0, _elements.size()), memento, mem()) ;
      }

      void assign(const bitarray_base<element_type> &source) ;
      void and_assign(const bitarray_base<element_type> &source) ;
      void or_assign(const bitarray_base<element_type> &source) ;
      void xor_assign(const bitarray_base<element_type> &source) ;

      void shift_left(int pos) ;
      void shift_right(int pos) ;

      void mask(const bitarray_base<element_type> &source) ;

      void set()
      {
         int full = _size / BITS_PER_CHUNK ;
         element_type *data = _bits() ;
         std::fill_n(data, full, ~0) ;
         if (full < _nelements())
            data[full] = ~(~0 << (_size % BITS_PER_CHUNK)) ;
      }

      void set(size_t pos, bool val = true)
      {
         NOXCHECK(pos < size()) ;
         element_type &data = _bits()[pos / BITS_PER_CHUNK] ;
         element_type mask = 1U << index(pos) ;
         if (val)
            data |= mask ;
         else
            data &= ~mask ;
      }

      void reset() { memset(_bits(), 0, _elements.size()) ; }

      void flip() ;
      void flip(size_t pos)
      {
         NOXCHECK(pos < size()) ;
         _bits()[pos / BITS_PER_CHUNK] ^= (1U << index(pos)) ;
      }

      bool equal(const bitarray_base<element_type> &other) const
      {
         return
            _size == other._size &&
            std::equal(_bits(), _bits() + _nelements(), other._bits()) ;
      }

      // valid_position()  -  is pos a valid bitarray position?
      //
      bool valid_position(size_t pos) { return pos < (size_t)_nelements() ; }

      // Given a bit position `pos', returns the index into the appropriate
      // chunk in bits[] such that 0 <= index < BITS_PER_CHUNK.
      //
      unsigned long index(size_t pos) const
      {
         return (sizeof(element_type)*8-1) & pos ;
      }

      void swap(bitarray_base<element_type> &other)
      {
         std::swap(other._size, _size) ;
         _elements.swap(other._elements) ;
      }

   private:
      size_t                              _size ;
      PTBuffer<element_type, PCowBuffer>  _elements ;

   private:
      int _nelements() const { return _elements.nitems() ; }
      element_type *_bits() { return _elements.get() ; }
      const element_type *_bits() const { return _elements ; }

} ;


/*******************************************************************************
                     class bitarray
*******************************************************************************/
class bitarray : private bitarray_base<unsigned long> {
      typedef bitarray_base<unsigned long> ancestor ;
   public:
      typedef bool value_type ;
      class        iterator ;
      class        const_iterator ;

      // bit reference
      //
      class bit_reference {
            friend class bitarray ;
            friend class iterator ;
            friend class const_iterator ;
         public:
            bit_reference &operator=(bool val)
            {
               _ref->set(_pos, val) ;
               return *this ;
            }

            bit_reference &operator=(const bit_reference &source)
            {
               _ref->set(_pos, source._ref->test(source._pos)) ;
               return *this ;
            }

            bool operator~() const { return !_ref->test(_pos) ; }

            operator bool() const { return _ref->test(_pos) ; }

            bit_reference &flip()
            {
               _ref->flip(_pos) ;
               return *this ;
            }

         protected:
            bitarray * _ref ;
            size_t     _pos ;

            bit_reference(bitarray &r, size_t p) :
               _ref(&r),
               _pos(p)
            {}

            void copy(const bit_reference &rhs)
            {
               _pos = rhs._pos ;
               _ref = rhs._ref ;
            }
      } ;

      typedef bit_reference reference ;

      // Constructor.
      // Parameters:
      //    sz       -  the size of array (i.e. the number of bits).
      //    initval  -  the initial value of all array items.
      //
      explicit bitarray(size_t sz = 0, bool initval = false) :
         ancestor(sz, initval)
      {}

      // Constructor. Restores the bit array previously saved by mem()
      // Parameters:
      //    memento  -  the external buffer to restore the array from. It is supposed the array
      //                was previously saved into this buffer through bitarray::mem() call
      //    size     -  the bit array size (in bits, NOT in bytes)
      //
      bitarray(const char *memento, size_t sz) :
         ancestor(memento, sz)
      {}

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

      // Inherited public members
      //
      using ancestor::count ;
      using ancestor::size ;
      using ancestor::test ;
      using ancestor::any ;
      using ancestor::find_first_bit ;
      using ancestor::mem ;

      bitarray &mask(const bitarray &source)
      {
         ancestor::mask(source) ;
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

      bitarray &flip(size_t pos)
      {
         ancestor::flip(pos) ;
         return *this ;
      }

      // element access
      //
      reference operator[](size_t pos) { return reference(*this, pos) ; }
      bool operator[](size_t pos) const { return test(pos) ; }

      bool operator==(const bitarray &other) const { return equal(other) ; }
      bool operator!=(const bitarray &other) const { return !equal(other) ; }

      bool none () const { return !any() ; }

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

      /*************************************************************************
                           Iterators
      *************************************************************************/
      class iterator :
         public std::iterator<std::random_access_iterator_tag, bool, ptrdiff_t> {
            friend class const_iterator ;
         public:
            iterator(bitarray &arr, int pos = 0) :
               _ref(arr, pos)
            {}

            iterator &operator=(const iterator &rhs)
            {
               _ref.copy(rhs._ref) ;
               return *this ;
            }

            iterator &operator+=(int diff)
            {
               _ref._pos += diff ;
               return *this ;
            }
            iterator &operator-=(int diff) { return *this += -diff ; }

            iterator &operator++() { ++_ref._pos ; return *this ; }
            iterator &operator--() { --_ref._pos ; return *this ; }
            iterator operator++(int)
            {
               iterator tmp(*this) ;
               operator++() ;
               return tmp ;
            }
            iterator operator--(int)
            {
               iterator tmp(*this) ;
               operator--() ;
               return tmp ;
            }
            iterator operator+ (int diff) const { return iterator(*this) += diff ; }
            iterator operator- (int diff) const { return iterator(*this) -= diff ; }
            int operator- (const iterator &iter) const { return _ref._pos - iter._ref._pos ; }
            bool operator== (const iterator &i2) const { return _ref._pos == i2._ref._pos ; }
            bool operator!= (const iterator &i2) const { return _ref._pos != i2._ref._pos ; }
            bool operator< (const iterator &i2) const { return _ref._pos < i2._ref._pos ; }
            bool operator<= (const iterator& i2) const { return ( !(i2 < *this)) ; }
            bool operator> (const iterator& i2) const { return i2 < *this ; }
            bool operator>= (const iterator& i2) const { return !(*this < i2) ; }

            bit_reference &operator*() const { return const_cast<iterator *>(this)->_ref ; }

         private:
            bit_reference _ref ;
      } ;

      class const_iterator :
         public std::iterator<std::random_access_iterator_tag, bool, ptrdiff_t> {

         public:
            const_iterator(const bitarray &arr, int pos = 0) :
               _ref(const_cast<bitarray &>(arr), pos)
            {}
            const_iterator(const bitarray::iterator &source) :
               _ref(source._ref)
            {}

            const_iterator &operator=(const const_iterator &rhs)
            {
               _ref.copy(rhs._ref) ;
               return *this ;
            }

            const_iterator &operator=(const bitarray::iterator &rhs)
            {
               _ref.copy(rhs._ref) ;
               return *this ;
            }

            const_iterator &operator+=(int diff)
            {
               _ref._pos += diff ;
               return *this ;
            }
            const_iterator &operator-=(int diff) { return *this += -diff ; }

            const_iterator &operator++() { ++_ref._pos ; return *this ; }
            const_iterator &operator--() { --_ref._pos ; return *this ; }
            const_iterator operator++(int)
            {
               const_iterator tmp(*this) ;
               operator++() ;
               return tmp ;
            }
            const_iterator operator--(int)
            {
               const_iterator tmp(*this) ;
               operator--() ;
               return tmp ;
            }
            const_iterator operator+ (int diff) const { return const_iterator(*this) += diff ; }
            const_iterator operator- (int diff) const { return const_iterator(*this) -= diff ; }
            int operator- (const const_iterator &iter) const { return _ref._pos - iter._ref._pos ; }
            bool operator== (const const_iterator &i2) const { return _ref._pos == i2._ref._pos ; }
            bool operator!= (const const_iterator &i2) const { return _ref._pos != i2._ref._pos ; }
            bool operator< (const const_iterator &i2) const { return _ref._pos < i2._ref._pos ; }
            bool operator<= (const const_iterator& i2) const { return ( !(i2 < *this)) ; }
            bool operator> (const const_iterator& i2) const { return i2 < *this ; }
            bool operator>= (const const_iterator& i2) const { return !(*this < i2) ; }

            bool operator*() const { return _ref ; }

         private:
            bit_reference _ref ;
      } ;

      /*************************************************************************
                           class positional_iterator
       This iterators traverses bit positions instead of bit values.
       I.e. it successively returns positions of a bitarray where bits are
       set to 'true'.
      *************************************************************************/
      class positional_iterator : public std::iterator<std::forward_iterator_tag, int, ptrdiff_t> {
         public:
            positional_iterator(const bitarray &array, int pos = 0) :
               _ref(&array),
               _pos(array.find_first_bit(pos, array.size()))
            {}

            positional_iterator &operator++()
            {
               _pos = _ref->find_first_bit(_pos + 1, _ref->size()) ;
               return *this ;
            }
            positional_iterator operator++(int)
            {
               positional_iterator tmp(*this) ;
               operator++() ;
               return tmp ;
            }

            bool operator==(const positional_iterator &rhs) const
            {
               NOXCHECK(_ref == rhs._ref) ;
               return _pos == rhs._pos ;
            }
            bool operator!= (const positional_iterator &rhs) const { return !(*this == rhs) ; }

            int operator*() const { return _pos ; }

         private:
            const bitarray *_ref ;
            int             _pos ;
      } ;

      //
      // STL-like access
      //
      iterator begin() { return iterator(*this, 0) ; }
      iterator end() { return iterator(*this, size()) ; }
      const_iterator begin() const { return const_iterator(*this, 0) ; }
      const_iterator end() const { return const_iterator(*this, size()) ; }

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
bitarray tmp(*l) ;                              \
return tmp.op(*r) ;

inline bitarray operator&(const bitarray &left, const bitarray &right)
{
   PCOMN_BIN_BITOP(operator&=, left, right) ;
}

inline bitarray operator|(const bitarray &left, const bitarray &right)
{
   PCOMN_BIN_BITOP(operator|=, left, right) ;
}

inline bitarray operator^(const bitarray &left, const bitarray &right)
{
   PCOMN_BIN_BITOP(operator^=, left, right) ;
}

inline bitarray mask(const bitarray &left, const bitarray &right)
{
   bitarray tmp (left) ;
   return tmp.mask(right) ;
}

#undef PCOMN_BIN_BITOP

/*******************************************************************************
 bitarray_base<Element>
*******************************************************************************/
template<typename Element>
void bitarray_base<Element>::flip()
{
   int full = _size / BITS_PER_CHUNK ;
   for (int i = 0 ; i < full ; i++)
      _bits()[i] = ~_bits()[i] ;
   if (full < _nelements())
      _bits()[full] ^= ~(~0 << (_size % BITS_PER_CHUNK)) ;
}

template<typename Element>
size_t bitarray_base<Element>::count(bool bits) const
{
   size_t cnt = 0 ;
   const element_type *data = _bits() ;
   for (int i = _nelements() ; i-- ; cnt += bitop::bitcount(data[i])) ;
   if (!bits)
      cnt = _size - cnt ;
   return cnt ;
}

template<typename Element>
bool bitarray_base<Element>::any() const
{
   const element_type *data = _bits() ;
   const element_type *dend = data + _nelements() ;
   while(data != dend)
      if (*data++)
         return true ;
   return false ;
}

template<typename Element>
void bitarray_base<Element>::and_assign(const bitarray_base<Element> &source)
{
   int minsize = std::min(_size, source._size) ;
   int full = minsize / BITS_PER_CHUNK ;
   int elem = std::min(_nelements(), source._nelements()) ;
   element_type *data = _bits() ;
   const element_type *source_data = source._bits() ;
   for (int i = 0 ; i < full ; ++i)
      data[i] &= source_data[i] ;
   if (full < elem)
      data[full] &= source_data[full] | (~0 << (minsize % BITS_PER_CHUNK)) ;
}

template<typename Element>
void bitarray_base<Element>::or_assign(const bitarray_base<Element> &source)
{
   int minsize = std::min(_size, source._size) ;
   int full = minsize / BITS_PER_CHUNK ;
   int elem = std::min(_nelements(), source._nelements()) ;
   element_type *data = _bits() ;
   const element_type *source_data = source._bits() ;
   for (int i = 0 ; i < full ; ++i)
      data[i] |= source_data[i] ;
   if (full < elem)
      data[full] |= source_data[full] & ~(~0 << (minsize % BITS_PER_CHUNK)) ;
}

template<typename Element>
void bitarray_base<Element>::xor_assign(const bitarray_base<Element> &source)
{
   int minsize = std::min(_size, source._size) ;
   int full = minsize / BITS_PER_CHUNK ;
   int elem = std::min(_nelements(), source._nelements()) ;
   element_type *data = _bits() ;
   const element_type *source_data = source._bits() ;
   for (int i = 0 ; i < full ; ++i)
      data[i] ^= source_data[i] ;
   if (full < elem)
      data[full] ^= source_data[full] & ~(~0 << (minsize % BITS_PER_CHUNK)) ;
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
void bitarray_base<Element>::mask(const bitarray_base<Element> &source)
{
   int minsize = std::min(_size, source._size) ;
   int full = minsize / BITS_PER_CHUNK ;
   int elem = std::min(_nelements(), source._nelements()) ;
   element_type *data = _bits() ;
   const element_type *source_data = source._bits() ;
   for (int i = 0 ; i < full ; ++i)
      data[i] &= ~source_data[i] ;
   if (full < elem)
      data[full] &= ~(source_data[full] & ~(~0 << (minsize % BITS_PER_CHUNK))) ;
}

template<typename Element>
size_t bitarray_base<Element>::mem(char *memento) const
{
   size_t memsize = (size() + 7) / 8 ;
   if (memento)
   {
      const element_type *data = _bits() ;
#ifndef PCOMN_CPU_BIG_ENDIAN
      memcpy(memento, data, memsize) ;
#else
      for (size_t i = 0 ; i < memsize ; ++i)
      {
         const char *item = reinterpret_cast<const char *>(data + i / sizeof(element_type)) ;
         memento[i] = item[sizeof(element_type) - 1 - i % sizeof(element_type)] ;
      }
#endif
   }
   return memsize ;
}

template<typename Element>
int bitarray_base<Element>::find_first_bit(int start, int finish) const
{
   NOXPRECONDITION(start >= 0 && (size_t)finish <= size()) ;

   if (start >= finish)
      return finish ;

   int pos = start / BITS_PER_CHUNK ;
   const element_type *bits = _bits() + pos ;
   element_type el = *bits >> index(start) ;
   if (!el)
   {
      int to = finish / BITS_PER_CHUNK ;
      do {
         if (++pos > to)
            return finish ;
         el = *++bits ;
      } while (!el) ;
      start = pos * BITS_PER_CHUNK ;
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

#ifdef __BORLANDC__
#pragma warn .inl
#endif

#endif /* __PCOMN_BITARRAY_H */
