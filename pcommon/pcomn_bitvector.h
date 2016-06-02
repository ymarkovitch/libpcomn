/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_BITVECTOR_H
#define __PCOMN_BITVECTOR_H
/*******************************************************************************
 FILE         :   pcomn_bitvector.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Interpret an array of integral types as a bit vector.

 CREATION DATE:   25 May 2016
*******************************************************************************/
/** @file
 Template to interpret an array of integral types as a bit vector.
*******************************************************************************/
#include <pcomn_assert.h>
#include <pcomn_meta.h>
#include <pcomn_integer.h>
#include <pcomn_atomic.h>
#include <pcommon.h>

#include <iostream>
#include <algorithm>

namespace pcomn {

/******************************************************************************/
/** Pointer to an array of integral types interpreted as a bit vector.
*******************************************************************************/
template<typename E>
struct basic_bitvector ;

template<typename E>
struct basic_bitvector<const E> {
      /// The type of element of an array in which we store bits.
      typedef const E element_type ;

      // Must be unsigned and not bool
      PCOMN_STATIC_CHECK(pcomn::is_integer<element_type>::value && std::is_unsigned<element_type>::value) ;

      constexpr basic_bitvector() = default ;

      constexpr basic_bitvector(element_type *e, size_t n) : _elements(e), _nelements(n) {}

      template<size_t n>
      constexpr basic_bitvector(element_type (&e)[n]) : _elements(e), _nelements(n) {}

      /// Get the number of elements in of this vector.
      constexpr size_t nelements() const { return _nelements ; }

      /// Get the size of this vector in bits.
      constexpr size_t size() const { return nelements()*BITS_PER_ELEMENT ; }

      constexpr element_type *data() const { return _elements ; }
      constexpr element_type *cdata() { return _elements ; }

      /// Get the count of 1 or 0 bit in vector
      size_t count(bool bitval = true) const
      {
         const size_t c = bitop::bitcount(cdata(), nelements()) ;
         return bitval ? c : size() - c ;
      }

      /// Get the value of a bit at specified position.
      bool test(size_t pos) const
      {
         return !!(elem(pos) & bitmask(pos)) ;
      }

      /// Atomic test
      bool test(size_t pos, std::memory_order order) const
      {
         return !!(elem(pos, order) & bitmask(pos)) ;
      }

      /// Get the position of first nonzero bit between 'start' and 'finish'
      ///
      /// If there is no such bit, returns 'finish'
      template<bool b>
      size_t find_first_bit(size_t start = 0, size_t finish = -1) const
      {
         return bitop::find_first_bit(cdata(), start, std::min(size(), finish), b) ;
      }

      /// Given a bit position, get the position of an element containing specified bit
      static constexpr size_t cellndx(size_t pos) { return bitop::cellndx<element_type>(pos) ; }

      /// Given a bit position, get the index inside the appropriate chunk in bits[]
      /// such that 0 <= index < BITS_PER_ELEMENT.
      static constexpr size_t bitndx(size_t pos) { return bitop::bitndx<element_type>(pos) ; }

      static constexpr element_type bitmask(size_t pos) { return bitop::bitmask<element_type>(pos) ; }

      static constexpr element_type bitextend(bool bit) { return bitop::bitextend<element_type>(bit) ; }

      /************************************************************************/
      /** Random-access constant iterator over bits
      *************************************************************************/
      struct iterator : std::iterator<std::random_access_iterator_tag, bool, ptrdiff_t, void, void> {
            constexpr iterator() = default ;
            constexpr iterator(const basic_bitvector &v, size_t pos) :
               _elements(v._elements),
               _pos(pos)
            {}

            bool operator*() const { return !!(_elements[cellndx(_pos)] & bitmask(_pos)) ; }

            iterator &operator+=(ptrdiff_t diff)
            {
               _pos += diff ;
               return *this ;
            }
            iterator &operator-=(ptrdiff_t diff) { return *this += -diff ; }

            iterator &operator++() { ++_pos ; return *this ; }
            iterator &operator--() { --_pos ; return *this ; }

            PCOMN_DEFINE_POSTCREMENT_METHODS(iterator) ;
            PCOMN_DEFINE_NONASSOC_ADDOP_METHODS(iterator, ptrdiff_t) ;

            friend ptrdiff_t operator-(const iterator &x, const iterator &y) { return x.pos() - y.pos() ; }

            friend bool operator==(const iterator &x, const iterator &y) { return x.pos() == y.pos() ; }
            friend bool operator<(const iterator &x, const iterator &y) { return x.pos() < y.pos() ; }

            PCOMN_DEFINE_RELOP_FUNCTIONS(friend, iterator) ;

         private:
            const element_type *_elements ;
            ptrdiff_t           _pos ;

            ptrdiff_t pos() const { return _pos ; }
      } ;

      /************************************************************************/
      /** A constant iterator over a bit set, which traverses bit @em positions
       instead of bit @em values.

       The iterator successively returns positions of 1-bits in a vector.
      *************************************************************************/
      struct positional_iterator : std::iterator<std::forward_iterator_tag, ptrdiff_t, ptrdiff_t> {

            constexpr positional_iterator() = default ;
            constexpr positional_iterator(const basic_bitvector &v, size_t pos) :
               _vec(&v),
               _pos(v.find_first_bit<1>(pos))
            {}

            ptrdiff_t operator*() const { return _pos ; }

            positional_iterator &operator++()
            {
               _pos = _vec->find_first_bit<1>(_pos + 1) ;
               return *this ;
            }
            PCOMN_DEFINE_POSTCREMENT(positional_iterator, ++) ;

            bool operator==(const positional_iterator &rhs) const
            {
               NOXCHECK(_vec == rhs._vec) ;
               return _pos == rhs._pos ;
            }
            bool operator!=(const positional_iterator &rhs) const { return !(*this == rhs) ; }

         private:
            const basic_bitvector * _vec ;
            ptrdiff_t               _pos ;
      } ;

      typedef iterator const_iterator ;

      iterator begin() const { return iterator(*this, 0) ; }
      iterator end() const { return iterator(*this, size()) ; }
      const_iterator cbegin() const { return begin() ; }
      const_iterator cend() const { return end() ; }

      positional_iterator begin_positional() const { return positional_iterator(*this, 0) ; }
      positional_iterator end_positional() const { return positional_iterator(*this, size()) ; }

   protected:
      /// Bit count per storage element
      static constexpr const size_t BITS_PER_ELEMENT = CHAR_BIT*sizeof(element_type) ;

      element_type &elem(size_t bitpos) const
      {
         NOXCHECK(bitpos < size()) ;
         return *(data() + cellndx(bitpos)) ;
      }

      element_type elem(size_t bitpos, std::memory_order order) const
      {
         return atomic_op::load(&elem(bitpos), order) ;
      }

   private:
      element_type * _elements  = nullptr ;
      size_t         _nelements = 0 ;
} ;

/******************************************************************************/
/** Pointer to an array of integral types interpreted as a bit vector.
*******************************************************************************/
template<typename E>
struct basic_bitvector : basic_bitvector<const E> {
   private:
      typedef basic_bitvector<const E> ancestor ;
   public:
      /// The type of element of an array in which we store bits.
      typedef E element_type ;

      using ancestor::cellndx ;
      using ancestor::bitndx ;
      using ancestor::bitmask ;
      using ancestor::bitextend ;
      using ancestor::nelements ;

      constexpr basic_bitvector() = default ;
      constexpr basic_bitvector(element_type *e, size_t n) : ancestor(e, n) {}
      template<size_t n>
      constexpr basic_bitvector(element_type (&e)[n]) : ancestor(e) {}

      constexpr element_type *data() const
      {
         return const_cast<element_type *>(ancestor::data()) ;
      }

      /// Set bit value at the given position.
      ///
      /// @return Old value of the bit at @a pos.
      bool set(size_t pos, bool val = true) const
      {
         element_type &data = elem(pos) ;
         const element_type mask = bitmask(pos) ;
         const bool old = data & mask ;
         set_flags(data, val, mask) ;
         return old ;
      }

      /// Atomically set a bit at the given position to specified value.
      bool set(size_t pos, bool val, std::memory_order order) const
      {
         const element_type value = bitextend(val) ;
         const element_type mask = bitmask(pos) ;

         return
            atomic_op::fetch_and_F(&elem(pos), [=](element_type oldval)
            {
               return bitop::set_bits_masked(oldval, value, mask) ;
            },
            order)
            & mask ;
      }

      /// Atomically set a bit at the given position to 1
      bool set(size_t pos, std::memory_order order) const
      {
         return set(pos, true, order) ;
      }

      /// Atomically compare and swap single bit in the array.
      /// @return The result of the comparison: true if bit at @a pos was equal to
      /// @a expected, false otherwise.
      bool cas(size_t pos, bool expected, bool desired, std::memory_order order = std::memory_order_acq_rel) const
      {
         return atomic_op::bit_cas(&elem(pos), bitextend(expected), bitextend(desired), bitmask(pos), order) ;
      }

      /// Invert all bits in this vector.
      void flip() const ;

      /// Invert a bit at the specified position.
      ///
      /// @return The @em new bit value.
      bool flip(size_t pos) const
      {
         return !!((elem(pos) ^= bitmask(pos)) & bitmask(pos)) ;
      }

      /// Atomically invert a bit at the specified position.
      ///
      /// @return The @em new bit value.
      bool flip(size_t pos, std::memory_order order) const
      {
         const element_type mask = bitmask(pos) ;
         return !(atomic_op::bit_xor(&elem(pos), mask, order) & mask) ;
      }

   private:
      element_type &elem(size_t bitpos) const
      {
         return const_cast<element_type &>(ancestor::elem(bitpos)) ;
      }

      element_type elem(size_t bitpos, std::memory_order order) const
      {
         return ancestor::elem(bitpos) ;
      }
} ;

/*******************************************************************************
 bitvector
*******************************************************************************/
template<typename E>
void basic_bitvector<E>::flip() const
{
   const size_t n = nelements() ;
   if (!n)
      return ;

   element_type *e = data() ;
   for (element_type * const end = e + n ; e != end ; ++e)
      *e ^= int_traits<element_type>::ones ;
}

template<typename E>
inline basic_bitvector<E> make_bitvector(E *data, size_t sz)
{
   return {data, sz} ;
}

template<typename E, size_t n>
inline basic_bitvector<E> make_bitvector(E (&data)[n])
{
   return {data} ;
}

/*******************************************************************************
 Stream output
*******************************************************************************/
template<typename E>
inline std::ostream &operator<<(std::ostream &os, const basic_bitvector<E> &data)
{
    std::copy(data.begin(), data.end(), std::ostream_iterator<int>(os)) ;
    return os ;
}

} // end of namespace pcomn

#endif /* __PCOMN_BITVECTOR_H */
