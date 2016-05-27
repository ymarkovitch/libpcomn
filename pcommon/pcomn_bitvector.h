/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_BITVECTOR_H
#define __PCOMN_BITVECTOR_H
/*******************************************************************************
 FILE         :   pcomn_bitvector.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Bit mask of arbitrary lenght.

 CREATION DATE:   25 May 2016
*******************************************************************************/
/** @file
    Bit mask of arbitrary lenght.
*******************************************************************************/
#include <pcomn_assert.h>
#include <pcomn_integer.h>
#include <pcomn_meta.h>
#include <pcomn_atomic.h>
#include <pcomn_buffer.h>

namespace pcomn {

/*******************************************************************************

*******************************************************************************/


/******************************************************************************/
/** Bit mask of arbitrary length
*******************************************************************************/
template<typename E, typename Derived>
struct basic_bitvector {
      /// The type of element of an array in which we store bits.
      typedef E element_type ;

      // Must be unsigned and not bool
      PCOMN_STATIC_CHECK(std::is_integral<element_type>::value &&
                         std::is_unsigned<element_type>::value &&
                         !std::is_same<bool, std::remove_cv_t<E>>::value) ;

      /// Get the size of vector in bits
      size_t size() const { return _size ; }

      /// Get the count of 1 or 0 bit in vector
      size_t count(bool bitval = true) const
      {
         size_t c = Derived::cached_count() ;
         if (c == ~0UL)
            Derived::cache_count(c = count_ones()) ;
         return bitval ? c : size() - c ;
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

      void reset()
      {
         memset(mbits(), 0, buf::size(_elements)) ;
         _count = 0 ;
      }

      void set()
      {
         if (const size_t itemcount = nelements())
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

      /// Get the position of first nonzero bit between 'start' and 'finish'
      ///
      /// If there is no such bit, returns 'finish'
      size_t find_first_bit(size_t start, size_t finish) const ;

   protected:
      /// Bit count per storage element
      static constexpr const size_t BITS_PER_ELEMENT = CHAR_BIT*sizeof(element_type) ;

      constexpr bitvector() : _count(0) {}

      bitvector(bitvector &&other) :
         _elements(std::move(other._elements))
      {}

      ~bitvector() = default ;

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

      size_t nelements() const { return buf::size(_elements)/sizeof(element_type) ; }

      size_t count_ones() const ;

   private:
      buffer_type _elements ;

   private:
      static constexpr size_t cached_count() { return ~(size_t()) ; }
      static void cache_count(size_t) {}

      element_type *mbits()
      {
         Derived::cache_count(~(size_t())) ;
         return static_cast<element_type *>(buf::data(_elements)) ;
      }

      const element_type *cbits() const
      {
         return static_cast<const element_type *>(buf::cdata(_elements)) ;
      }

      element_type &mutable_elem(size_t bitpos) { return mbits()[elemndx(bitpos)] ; }
      const element_type &const_elem(size_t bitpos) const { return cbits()[elemndx(bitpos)] ; }
} ;

/******************************************************************************/
/** Reference to a POD array of integers or pointers to interpret its memory
 as a bit vector.
*******************************************************************************/
template<typename E>
struct bitvector_reference : bitvector<E, std::pair<void *, size_t>> {
} ;

/*******************************************************************************
 bitvector
*******************************************************************************/
template<typename E, typename B>
void bitvector<E, B>::flip()
{
   const size_t n = nelements() ;
   if (!n)
      return ;

   element_type *data = mbits() ;
   for (element_type * const end = data + n ; data != end ; ++data)
      *data ^= ~element_type() ;
}

} // end of namespace pcomn

#endif /* __PCOMN_BITVECTOR_H */
