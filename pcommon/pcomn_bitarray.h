/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_BITARRAY_H
#define __PCOMN_BITARRAY_H
/*******************************************************************************
 FILE         :   pcomn_bitarray.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2019. All rights reserved.
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
#include <pcomn_iterator.h>

#include <algorithm>
#include <iterator>
#include <vector>
#include <iostream>

#include <limits.h>
#include <stdint.h>
#include <stddef.h>

namespace pcomn {

/******************************************************************************/
/** Implementation of pcomn::bitarray.
*******************************************************************************/
template<typename Element = uintptr_t>
struct bitarray_base {

      /// Get the size of array (in bits)
      size_t size() const { return _size ; }

      /// Get the count of 1 or 0 bit in the array
      size_t count(bool bitval = true) const
      {
         if (!size())
            return 0 ;
         auto &ones = cb(cdata())->cached_popcount() ;
         if (ones == ~0UL)
            ones = bitop::popcount(cbits(), nelements()) ;
         return bitval ? ones : _size - ones ;
      }

      bool test(size_t pos) const
      {
         NOXCHECK(pos < size()) ;
         return !!(const_elem(pos) & bitmask(pos)) ;
      }

      /// Indicate if there exists '1' bit in this array
      bool any() const
      {
         const item_type * const data = cdata() ;
         const element_type * const start_nzmap = nzmap(data) ;
         const element_type * const end_nzmap = bits(data) ;

         return std::any_of(start_nzmap, end_nzmap, identity()) ;
      }

      /// Indicate if all bits in this array are '1'
      bool all() const
      {
         const element_type * const b = cbits() ;
         GCC_DIAGNOSTIC_PUSH_IGNORE(implicit-fallthrough)
         switch (const size_t n = nelements())
         {
            default:
               if (!std::all_of(b, b + (n-1), [](element_type e){ return e == ~element_type() ; }))
                  return false ;

            case 1: return (b[n-1] & tailmask(size())) == tailmask(size()) ;
            case 0: break ;
         }
         GCC_DIAGNOSTIC_POP()
         return true ;
      }

      /// Get the position of first nonzero bit between 'start' and 'finish'.
      /// If there is no such bit, returns 'finish'
      size_t find_first_bit(size_t start = 0, size_t finish = -1) const ;

   protected:
      // The type of element of an array in which we store bits.
      typedef Element element_type ;

      PCOMN_STATIC_CHECK(std::is_integral<element_type>::value && std::is_unsigned<element_type>::value) ;

      /*************************************************************************
       Constants for bit counts and sizes
      *************************************************************************/
      /// Bit count per storage element
      static constexpr const size_t BITS_PER_ELEMENT = bitsizeof(element_type) ;
      /*************************************************************************/

      constexpr bitarray_base() = default ;

      // Constructor.
      // Parameters:
      //    sz       -  the size of array (i.e. the number of bits).
      //    initval  -  the initial value of all array items.
      //
      explicit bitarray_base(size_t sz, bool initval = false) : bitarray_base(sz, nullptr)
      {
         initval ? set() : reset() ;
      }

      template<typename InputIterator>
      bitarray_base(InputIterator start, std::enable_if_t<is_iterator<InputIterator>::value, InputIterator> finish) :
         bitarray_base(start, finish, is_iterator<InputIterator, std::random_access_iterator_tag>())
      {}

      bitarray_base(const bitarray_base &) = default ;

      bitarray_base(bitarray_base &&other) :
         _size(other._size), _elements(std::move(other._elements))
      {
         other._size = 0 ;
      }

      ~bitarray_base() = default ;

      bitarray_base &operator=(const bitarray_base &other) = default ;

      bitarray_base &operator=(bitarray_base &&other)
      {
         if (&other != this)
         {
            _size = other._size ;
            other._size = 0 ;
            _elements = std::move(other._elements) ;
         }
         return *this ;
      }


      template<typename Operator>
      void op_assign(const bitarray_base &source, Operator op) ;

      void reset() { size() && memset(mdata(), 0, _elements.size()) ; }

      void set()
      {
         const size_t elemcount = nelements() ;
         if (!elemcount)
            return ;

         item_type * const data = mdata() ;
         memset(data, -1, _elements.size()) ;
         // Fix the tail of bitdata
         *(bits(data) + (elemcount - 1)) &= tailmask(size()) ;
         // Fix the tail of the nonzero map
         *(nzmap(data) + cellndx(elemcount - 1)) &= tailmask(elemcount) ;

         cb(data)->cached_popcount() = size() ;
      }

      void set(size_t pos, bool val = true) ;

      /// Flip (invert) all the bits in the array.
      void flip() ;

      /// Flip (invert) the bit at @a pos.
      /// @return New (i.e. flipped) value of the nit at @a pos.
      bool flip(size_t pos) ;

      bool equal(const bitarray_base &other) const
      {
         const item_type * const data1 = cdata() ;
         const item_type * const data2 = other.cdata() ;
         return
            data1 == data2 ||
            _size == other._size && !memcmp(data1 + 1, data2 + 1, _elements.size() - sizeof *data1) ;
      }

      void swap(bitarray_base &other) noexcept
      {
         std::swap(other._size, _size) ;
         _elements.swap(other._elements) ;
      }

      /// Given a bit position, get the position of an element containing specified bit
      static constexpr size_t cellndx(size_t pos) { return bitop::cellndx<element_type>(pos) ; }

      /// Given a bit position, get the index inside the appropriate chunk in bits[]
      /// such that 0 <= index < BITS_PER_ELEMENT.
      static constexpr size_t bitndx(size_t pos) { return bitop::bitndx<element_type>(pos) ; }

      static constexpr element_type bitmask(size_t pos) { return bitop::bitmask<element_type>(pos) ; }

      static constexpr element_type tailmask(size_t bitcnt) { return bitop::tailmask<element_type>(bitcnt) ; }

   private:
      /*************************************************************************
       Data layout (every single item is item_type):
          control_block (AKA bcb, bits control block)
              cached_popcount
              nonzero_map[]
          elements[]
      *************************************************************************/
      union item_type {
            element_type   _element ;
            mutable size_t _popcount ; /* The cached count of 1s in the bitset */
      } ;

      struct control_block {
            item_type _cached_popcount ;
            item_type _nonzero_map[] ; /* Map of nonzero elements */

            size_t &cached_popcount() const { return _cached_popcount._popcount ; }

            // The size of the control block expressed in item_type items
            static constexpr size_t size(size_t nelements)
            {
               // Add 1 to make allowance for _cached_popcount
               return (nelements + (BITS_PER_ELEMENT - 1))/BITS_PER_ELEMENT + 1 ;
            }
      } ;

      static_assert(sizeof(element_type) >= sizeof(size_t),
                    "The Element type argument to pcomn::bitarray_base must be at least sizeof(size_t)") ;
      PCOMN_STATIC_CHECK(sizeof(item_type) ==  sizeof(element_type)) ;

   private:
      size_t      _size  = 0UL ;
      cow_buffer  _elements ;

      // Fake control block data for zero-sized bitarrays
      static const item_type _empty_data[control_block::size(1)] ;

   private:
      static constexpr size_t nelements(size_t sz) { return cellndx(sz + (BITS_PER_ELEMENT - 1)) ; }
      size_t nelements() const { return nelements(_size) ; }

      static constexpr size_t nzmapcellndx(size_t pos) { return cellndx(cellndx(pos)) ; }
      static constexpr size_t nzmapbitndx(size_t pos) { return bitndx(cellndx(pos)) ; }
      static constexpr element_type nzmapbitmask(size_t pos) { return bitmask(nzmapbitndx(pos)) ; }

      // Get bits by data
      element_type *bits(item_type *data)
      {
         return &((data + control_block::size(nelements()))->_element) ;
      }
      const element_type *bits(const item_type *data) const
      {
         return &((data + control_block::size(nelements()))->_element) ;
      }

      // Get a control block by data
      control_block *cb(item_type *data)
      {
         control_block * const __attribute__((__may_alias__)) r =
            reinterpret_cast<control_block *>(data) ;
         return r ;
      }
      const control_block *cb(const item_type *data) const
      {
         const control_block * const __attribute__((__may_alias__)) r =
            reinterpret_cast<const control_block *>(data) ;
         return r ;
      }

      const element_type *nzmap(const item_type *data) const
      {
         return &cb(data)->_nonzero_map[0]._element ;
      }
      element_type *nzmap(item_type *data)
      {
         return &cb(data)->_nonzero_map[0]._element ;
      }

      // Ensure COW, reset cache, get mutable data
      // Don't COW, don't touch cache
      const item_type *cdata() const
      {
         const item_type * const d = static_cast<const item_type *>(_elements.get()) ;
         return d ? d : _empty_data ;
      }
      const element_type *cbits() const { return bits(cdata()) ; }

      item_type *mdata()
      {
         NOXCHECK(_size) ;
         item_type * const data = static_cast<item_type *>(_elements.get()) ;
         cb(data)->cached_popcount() = ~0UL ;
         return data ;
      }

      const element_type &const_elem(size_t bitpos) const { return cbits()[cellndx(bitpos)] ; }

   private:
      /*************************************************************************
       Constructors
      *************************************************************************/
      bitarray_base(size_t sz, nullptr_t) :
         _size(sz), _elements(sz ? sizeof(item_type) * (nelements() + control_block::size(nelements())) : 0)
      {}

      template<typename InputIterator>
      bitarray_base(InputIterator &start, InputIterator &finish, std::false_type) :
         bitarray_base(std::vector<bool>(start, finish), Instantiate())
      {}

      template<typename RandomAccessIterator>
      bitarray_base(RandomAccessIterator &start, RandomAccessIterator &finish, std::true_type) ;

      /// Constructor to allow derived classes for construction from non-random-access
      /// iterators.
      bitarray_base(std::vector<bool> &&bits, Instantiate) :
         bitarray_base(bits.begin(), bits.end())
      {}

      /*************************************************************************
       Generic mutator
       Returns _new_ value of bits(data)[elndx]
      *************************************************************************/
      template<typename Operator>
      element_type update_element(item_type *data, size_t elndx, element_type operand, Operator op)
      {
         element_type &nzmap_cell = *(nzmap(data) + cellndx(elndx)) ;
         element_type &element = *(bits(data) + elndx) ;
         const element_type input = element ;
         const element_type output = op(input, operand) ;
         element = output ;
         // If the "nonzero state" of the element has been changed (i.e. zero->nonzero or
         // vice versa), flip the corresponding bit in the nonzero map
         nzmap_cell ^= element_type(!input ^ !output) << bitndx(elndx) ;
         return output ;
      }

      void fix_tail(item_type *data)
      {
         update_element(data, nelements() - 1, tailmask(size()), std::bit_and<element_type>()) ;
      }
} ;

/******************************************************************************/
/** Like std::bitset, but has its size specified at runtime.
 Implemented with copy-on-write, has O(1) copy and assignment.
*******************************************************************************/
class bitarray : private bitarray_base<uintptr_t> {
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
               _pos(array.find_first_bit(pos))
            {}

            ptrdiff_t operator*() const { return _pos ; }

            positional_iterator &operator++()
            {
               _pos = _ref->find_first_bit(_pos + 1) ;
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

      constexpr bitarray() {}

      /// Create bitarray of specified size.
      /// @param sz  The size of array (bit count).
      ///
      explicit bitarray(size_t sz) : bitarray(sz, false) {}

      /// Create bitarray of specified size filled with specified bit.
      /// @param sz        The size of array (bit count)
      /// @param initval   Initial value of all bits
      ///
      bitarray(size_t sz, bool initval) : ancestor(sz, initval) {}

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
         op_assign(source, std::bit_and<element_type>()) ;
         return *this ;
      }

      bitarray &operator|=(const bitarray &source)
      {
         op_assign(source, std::bit_or<element_type>()) ;
         return *this ;
      }

      bitarray &operator^=(const bitarray &source)
      {
         op_assign(source, std::bit_xor<element_type>()) ;
         return *this ;
      }

      bitarray &operator-=(const bitarray &source)
      {
         size() >= source.size()
            ? op_assign(source, [](element_type x, element_type y) { return x &~ y ; })
            : op_assign(source, [](element_type x, element_type y) { return y &~ x ; }) ;
         return *this ;
      }

      bitarray &mask(const bitarray &source) { return *this -= source ; }

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

      void swap(bitarray &other) { ancestor::swap(other) ; }

      //
      // STL-like access
      //
      iterator begin() { return iterator(*this, 0) ; }
      iterator end() { return iterator(*this, size()) ; }
      const_iterator begin() const { return const_iterator(*this, 0) ; }
      const_iterator end() const { return const_iterator(*this, size()) ; }
      const_iterator cbegin() const { return begin() ; }
      const_iterator cend() const { return end() ; }
      bool front() const { return test(0) ; }
      bool back() const { return test(size() - 1) ; }

      positional_iterator begin_positional() const { return positional_iterator(*this) ; }
      positional_iterator end_positional() const { return positional_iterator(*this, size()) ; }
} ;

/*******************************************************************************
 bitarray
*******************************************************************************/
inline bitarray operator&(const bitarray &left, const bitarray &right)
{
   return std::move(bitarray(left) &= right) ;
}

inline bitarray operator|(const bitarray &left, const bitarray &right)
{
   return std::move(bitarray(left) |= right) ;
}

inline bitarray operator^(const bitarray &left, const bitarray &right)
{
   return std::move(bitarray(left) ^= right) ;
}

inline bitarray operator-(const bitarray &left, const bitarray &right)
{
   return std::move(bitarray(left) -= right) ;
}

inline bitarray operator~(bitarray v)
{
   return std::move(v.flip()) ;
}

MS_PUSH_IGNORE_WARNING(4267)

/*******************************************************************************
 bitarray_base<Element>
*******************************************************************************/
template<typename Element>
const typename bitarray_base<Element>::item_type bitarray_base<Element>::_empty_data[] = {} ;

template<typename Element>
template<typename RandomAccessIterator>
bitarray_base<Element>::bitarray_base(RandomAccessIterator &start, RandomAccessIterator &finish, std::true_type) :
   bitarray_base(std::distance(start, finish))
{
   if (!size())
      return ;

   item_type * const data = mdata() ;
   element_type * const bitdata = bits(data) ;
   element_type * const nzmapdata = nzmap(data) ;
   auto &cached_count = cb(data)->cached_popcount() ;

   for (size_t pos = 0 ; pos < size() ; ++pos, ++start)
   {
      if (!*start)
         continue ;
      bitdata[cellndx(pos)] |= bitmask(pos) ;
      nzmapdata[nzmapcellndx(pos)] |= nzmapbitmask(pos) ;
      ++cached_count ;
   }
}

template<typename Element>
inline void bitarray_base<Element>::set(size_t pos, bool val)
{
   NOXCHECK(pos < size()) ;
   update_element(mdata(), cellndx(pos), 0LL - (long long)val,
                  [=](element_type data, element_type value)
                  { return bitop::set_bits_masked(data, value, bitmask(pos)) ; }) ;
}

template<typename Element>
inline bool bitarray_base<Element>::flip(size_t pos)
{
   NOXCHECK(pos < size()) ;
   const element_type mask = bitmask(pos) ;
   return
      update_element(mdata(), cellndx(pos), mask, std::bit_xor<element_type>()) & mask ;
}

template<typename Element>
void bitarray_base<Element>::flip()
{
   const size_t n = nelements() ;
   if (!n)
      return ;

   item_type * const data = mdata() ;
   for (size_t ndx = 0 ; ndx < n ; ++ndx)
      update_element(data, ndx, ~element_type(), std::bit_xor<element_type>()) ;

   fix_tail(data) ;
}

template<typename Element>
template<typename Operator>
void bitarray_base<Element>::op_assign(const bitarray_base &source, Operator op)
{
   const element_type *source_bits = source.cbits() ;
   if (cbits() == source_bits)
      return ;

   bitarray_base input (source) ;
   if (source.size() > size())
   {
      swap(input) ;
      source_bits = input.cbits() ;
   }

   const size_t ninput = input.nelements() ;
   const size_t noutput = nelements() ;

   item_type * const outdata = mdata() ;

   size_t ndx ;
   for (ndx = 0 ; ndx < ninput ; ++ndx)
      update_element(outdata, ndx, source_bits[ndx], op) ;

   for (; ndx < noutput ; ++ndx)
      update_element(outdata, ndx, 0, op) ;

   fix_tail(outdata) ;
}

template<typename Element>
size_t bitarray_base<Element>::find_first_bit(size_t start, size_t finish) const
{
   finish = std::min(finish, size()) ;

   if (start >= finish)
      return finish ;

   const item_type * const self = cdata() ;
   const element_type * const bit_cells = bits(self) ;

   size_t current_elemndx = cellndx(start) ;
   element_type element = bit_cells[current_elemndx] >> bitndx(start) ;

   if (!element)
   {
      start = ++current_elemndx * BITS_PER_ELEMENT ;
      if (start >= finish)
         return finish ;

      // Search for the first nonzero element after the current using the nonzero map
      const element_type * const nzmap_cells = nzmap(self) ;

      size_t current_nzcellndx = cellndx(current_elemndx) ;
      element_type nzcell = nzmap_cells[current_nzcellndx] & (~element_type() << bitndx(current_elemndx)) ;
      for ( ; !nzcell ; nzcell = nzmap_cells[current_nzcellndx])
      {
         ++current_nzcellndx ;
         start = current_nzcellndx * (BITS_PER_ELEMENT * BITS_PER_ELEMENT) ;
         if (start >= finish)
            return finish ;
      }
      current_elemndx = current_nzcellndx * BITS_PER_ELEMENT + bitop::rzcnt(nzcell) ;
      NOXCHECK(current_elemndx < nelements()) ;

      element = bit_cells[current_elemndx] ;
      NOXCHECK(element) ;

      start = current_elemndx * BITS_PER_ELEMENT ;
   }
   return std::min(start + bitop::rzcnt(element), finish) ;
}

MS_DIAGNOSTIC_POP()

/******************************************************************************/
/** swap specialization for pcomn::bitarray.
*******************************************************************************/
PCOMN_DEFINE_SWAP(bitarray) ;

/******************************************************************************/
/** User-defined literal for bitarrays (e.g. 101_bits, 011111_bits)
*******************************************************************************/
inline bitarray operator "" _bit(const char *r)
{
   static const auto chartobit = [](char c)
   {
      ensure<std::invalid_argument>
      (c == '0' || c == '1', "Invalid bitarray literal: only 0s and 1s are allowed") ;
      return c == '1' ;
   } ;
   return bitarray(xform_iter(r, chartobit), xform_iter(r + strlen(r), chartobit)) ;
}

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
