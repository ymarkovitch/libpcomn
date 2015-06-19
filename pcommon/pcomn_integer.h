/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_INTEGER_H
#define __PCOMN_INTEGER_H
/*******************************************************************************
 FILE         :   pcomn_integer.h
 COPYRIGHT    :   Yakov Markovitch, 2006-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Integral types traits.
                  Bit operations.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   27 Nov 2006
*******************************************************************************/
#include <pcomn_meta.h>
#include <pcomn_macros.h>

#include <functional>
#include <iterator>
#include <limits>
#include <cstdlib>
#include <limits.h>

namespace pcomn {

/// @cond
namespace detail {

template<unsigned v, unsigned s, bool> struct _ct_shl ;
template<unsigned v, unsigned s> struct _ct_shl<v, s, true> :
         public std::integral_constant<unsigned, (v << s)> {} ;
template<unsigned v, unsigned s> struct _ct_shl<v, s, false> :
         public std::integral_constant<unsigned, 0U> {} ;

} // end of namespace pcomn::detail
/// @endcond

/// Calculate at compile-time the number of bits in a type or value.
#define bitsizeof(t) (sizeof(t) * CHAR_BIT)

/*******************************************************************************
                     template<typename T>
                     struct int_traits
*******************************************************************************/
template<typename T>
struct int_traits {
      PCOMN_STATIC_CHECK(std::is_integral<T>::value) ;

      typedef typename std::make_signed<T>::type    stype ;
      typedef typename std::make_unsigned<T>::type  utype ;

      static constexpr const bool     is_signed = std::numeric_limits<T>::is_signed ;
      static constexpr const unsigned bitsize = bitsizeof(T) ;
      static constexpr const T        ones = (T)~(T)0 ;
      static constexpr const T        signbit = (T)((T)1 << (bitsize - 1)) ;
} ;

template<typename T>
constexpr const bool int_traits<T>::is_signed ;
template<typename T>
constexpr const unsigned int_traits<T>::bitsize ;
template<typename T>
constexpr const T int_traits<T>::ones ;
template<typename T>
constexpr const T int_traits<T>::signbit ;

/*******************************************************************************
                  template<typename T, typename R = T> struct if_integer
 Overload enabler, a la enable_if<>.
 If T is an integer type, returns (as internal typedef) R as 'type'
*******************************************************************************/
template<typename T, typename R = T> struct
if_integer : std::enable_if<std::numeric_limits<T>::is_integer, R> {} ;

template<typename T, typename R = T> struct
if_not_integer : disable_if<std::numeric_limits<T>::is_integer, R> {} ;

template<typename T, typename R = T> struct
if_signed_int : std::enable_if<(std::numeric_limits<T>::is_integer && std::numeric_limits<T>::is_signed), R> {} ;

template<typename T, typename R = T> struct
if_unsigned_int : std::enable_if<(std::numeric_limits<T>::is_integer && !std::numeric_limits<T>::is_signed), R> {} ;

template<typename T, typename R = T>
using if_integer_t = typename if_integer<T, R>::type ;
template<typename T, typename R = T>
using if_not_integer_t = typename if_not_integer<T, R>::type ;
template<typename T, typename R = T>
using if_signed_int_t = typename if_signed_int<T, R>::type ;
template<typename T, typename R = T>
using if_unsigned_int_t = typename if_unsigned_int<T, R>::type ;

template<typename T>
inline constexpr if_signed_int_t<T> sign_bit(T value)
{
   return value & int_traits<T>::signbit ;
}

/*******************************************************************************
 iabs
*******************************************************************************/
template<typename T>
inline typename if_signed_int<T, T>::type iabs(T v)
{
   return std::abs(v) ;
}

#ifdef PCOMN_PL_WIN32

template<>
inline int64_t iabs(int64_t v)
{
   return _abs64(v) ;
}

#endif

template<typename T>
inline typename if_unsigned_int<T, T>::type iabs(T v)
{
   return v ;
}

/*******************************************************************************
                     template<int nbits>
                     struct bit_traits
 bit_traits<n> describes bit operations on integers of size n bits.
*******************************************************************************/
template<int nbits>
struct bit_traits ;

template<>
struct bit_traits<8> {

   typedef int8_t  stype ;
   typedef uint8_t utype ;

   template<typename I>
   static unsigned bitcount(I value)
   {
      unsigned result = static_cast<utype>(value) ;
      result = (0x55U & result) + (0x55U & (result >> 1U)) ;
      result = (0x33U & result) + (0x33U & (result >> 2U)) ;
      result = (result + (result >> 4U)) & 0x0fU ;
      return result ;
   }

   template<typename I>
   static int log2floor(I value)
   {
      // At the end, x will have the same most significant 1 as value and all '1's below
      unsigned x = static_cast<utype>(value) ;
      x |= (x >> 1) ;
      x |= (x >> 2) ;
      x |= (x >> 4) ;
      return (int)bitcount((I)x) - 1 ;
   }

   template<typename I>
   static int log2ceil(I value)
   {
      const unsigned correction = static_cast<utype>(value) ;
      return log2floor(value) + !!(correction & (correction - 1)) ;
   }
} ;

template<> struct bit_traits<16> {

   typedef int16  stype ;
   typedef uint16 utype ;

   template<typename I>
   static unsigned bitcount(I value)
   {
      unsigned result = static_cast<utype>(value) ;
      result = (0x5555U & result) + (0x5555U & (result >> 1U)) ;
      result = (0x3333U & result) + (0x3333U & (result >> 2U)) ;
      result = (result + (result >> 4U)) & 0x0f0fU ;
      result += result >> 8U ;
      return result & 0x0000001fU ;
   }

   template<typename I>
   static int log2floor(I value)
   {
      unsigned x = static_cast<utype>(value) ;
      x |= (x >> 1) ;
      x |= (x >> 2) ;
      x |= (x >> 4) ;
      x |= (x >> 8) ;
      return (int)bitcount((I)x) - 1 ;
   }

   template<typename I>
   static int log2ceil(I value)
   {
      const unsigned correction = static_cast<utype>(value) ;
      return log2floor(value) + !!(correction & (correction - 1)) ;
   }
} ;

template<> struct bit_traits<32> {

   typedef int       stype ;
   typedef unsigned  utype ;

   template<typename I>
   static unsigned bitcount(I value)
   {
      unsigned result = static_cast<utype>(value) ;
      result = (0x55555555U & result) + (0x55555555U & (result >> 1U)) ;
      result = (0x33333333U & result) + (0x33333333U & (result >> 2U)) ;
      result = (result + (result >> 4U)) & 0x0f0f0f0fU ;
      result += result >> 8U ;
      result += result >> 16U ;
      return result & 0x0000003fU ;
   }

   template<typename I>
   static int log2floor(I value)
   {
      unsigned x = static_cast<utype>(value) ;
      x |= (x >> 1) ;
      x |= (x >> 2) ;
      x |= (x >> 4) ;
      x |= (x >> 8) ;
      x |= (x >> 16) ;
      return (int)bitcount((I)x) - 1 ;
   }

   template<typename I>
   static int log2ceil(I value)
   {
      const unsigned correction = static_cast<utype>(value) ;
      return log2floor(value) + !!(correction & (correction - 1)) ;
   }
} ;

template<> struct bit_traits<64> {

   typedef int64  stype ;
   typedef uint64 utype ;

   template<typename I>
   static unsigned bitcount(I value)
   {
      utype r = static_cast<utype>(value) ;
      r = (0x5555555555555555ULL & r) + (0x5555555555555555ULL & ((utype)r >> 1U)) ;
      r = (0x3333333333333333ULL & r) + (0x3333333333333333ULL & ((utype)r >>  2U)) ;
      r = (r + (r >> 4U)) & 0x0f0f0f0f0f0f0f0fULL ;
      r += r >> 8U ;
      r += r >> 16U ;
      r += r >> 32U ;
      return
         static_cast<unsigned>(r) & 0x0000007fU ;
   }

   template<typename I>
   static int log2floor(I value)
   {
      utype x = value ;
      x |= (x >> 1) ;
      x |= (x >> 2) ;
      x |= (x >> 4) ;
      x |= (x >> 8) ;
      x |= (x >> 16) ;
      x |= (x >> 32) ;
      return (int)bitcount((I)x) - 1 ;
   }

   template<typename I>
   static int log2ceil(I value)
   {
      const utype correction = value ;
      return log2floor(value) + !!(correction & (correction - 1)) ;
   }
} ;

/*******************************************************************************
 namespace pcomn::bitop
 Bit operations (like bit counts, etc.)
*******************************************************************************/
namespace bitop {

/// Count 1s in a value of some integral type
template<typename I>
inline unsigned bitcount(I i)
{
   return bit_traits<int_traits<I>::bitsize>::bitcount(i) ;
}

template<typename I>
inline int log2floor(I i)
{
   return bit_traits<int_traits<I>::bitsize>::log2floor(i) ;
}

template<typename I>
inline int log2ceil(I i)
{
   return bit_traits<int_traits<I>::bitsize>::log2ceil(i) ;
}

/// Clear Rightmost Non-Zero Bit.
/// 00001010 -> 00001000
template<typename I>
constexpr inline if_integer_t<I> clrrnzb(I x)
{
   return static_cast<I>(x & (x - 1)) ;
}

/// Get Rightmost Non-Zero Bit.
/// 00001010 -> 00000010
/// If there is no such bit, returns 0
template<typename I>
constexpr inline if_integer_t<I> getrnzb(I x)
{
   return static_cast<I>(x & -x) ;
}

/// Get Rightmost Zero Bit.
/// 01001111 -> 00010000
/// If there is no such bit, returns 0
template<typename I>
constexpr inline if_integer_t<I> getrzb(I x)
{
   return static_cast<I>(~x & (x + 1)) ;
}

/// Get Rightmost Zero Bit Sequence.
/// 00101000 -> 00000111
/// If there is no such bit, returns 0
template<typename I>
constexpr inline if_integer_t<I> getrzbseq(I x)
{
   return static_cast<I>(~(-getrnzb(x))) ;
}

/*******************************************************************************
                     template<typename I>
                     class nzbit_iterator
 Iterates over nonzero bits of an integer, from LSB to MSB.
 operator *() returns the currently selected nonsero bit.
 E.g.
 for (nzbit_iterator<unsigned> foo_iter (0x20005), foo_end ; foo_iter != foo_end ; ++foo_iter)
   cout << hex << *foo_iter << std::endl ;
 will print:
 1
 4
 20000
*******************************************************************************/
template<typename I>
struct nzbit_iterator : std::iterator<std::forward_iterator_tag, I> {

      explicit constexpr nzbit_iterator(if_integer_t<I> value) : _data(value) {}

      // Construct the end iterator
      // Please note that the end iterator _can_ be dereferenced and, by design,
      // returns 0
      constexpr nzbit_iterator() : _data() {}

      constexpr I operator*() const { return getrnzb(_data) ; }

      nzbit_iterator &operator++()
      {
         _data = clrrnzb(_data) ;
         return *this ;
      }

      nzbit_iterator operator++(int)
      {
         nzbit_iterator tmp(*this) ;
         ++*this ;
         return tmp ;
      }

      bool operator==(const nzbit_iterator &rhs) const { return _data == rhs._data ; }
      bool operator!=(const nzbit_iterator &rhs) const { return !(rhs == *this) ; }

   private:
      I _data ;
} ;

// make_nzbit_iterator()
/// Construct an object of type nzbit_iterator, where the iterable types is based
/// on the data type passed as its parameter.
template<typename I>
inline nzbit_iterator<if_integer_t<I> > make_nzbit_iterator(I value)
{
   return nzbit_iterator<I>(value) ;
}

/******************************************************************************/
/** Nonzero-bit positions iterator traverses bit @em positions instead of bit values

 It successively returns positions of nonzero bits.
 E.g.
 for (nzbitpos_iterator<unsigned> foo_iter (0x20005), foo_end ; foo_iter != foo_end ; ++foo_iter)
   cout << *foo_iter << std::endl ;
 will print:
 0
 2
 17
*************************************************************************/
template<typename I, typename V = int>
class nzbitpos_iterator : public std::iterator<std::forward_iterator_tag, V> {
      typedef typename int_traits<I>::utype datatype ;
      typedef std::iterator<std::forward_iterator_tag, V> ancestor ;
   public:
      using typename ancestor::value_type ;

      constexpr nzbitpos_iterator() : _data(), _pos(bitsizeof(I)) {}
      explicit nzbitpos_iterator(I value) : _data((datatype)value), _pos(-1)
      {
         advance() ;
      }

      nzbitpos_iterator &operator++()
      {
         advance() ;
         return *this ;
      }
      nzbitpos_iterator operator++(int)
      {
         nzbitpos_iterator<I> tmp(*this) ;
         advance() ;
         return tmp ;
      }

      constexpr bool operator==(const nzbitpos_iterator &rhs)
      {
         return _pos == rhs._pos ;
      }
      constexpr bool operator!= (const nzbitpos_iterator &rhs)
      {
         return !(*this == rhs) ;
      }

      constexpr value_type operator*() { return static_cast<value_type>(_pos) ; }

   private:
      datatype _data ;
      int      _pos ;

      void advance()
      {
         NOXCHECK(_pos < (int)bitsizeof(I)) ;
         if (!_data)
         {
            _pos = bitsizeof(I) ;
            return ;
         }
         for ( ; ++_pos, !(_data & 1) ; _data >>= 1) ;
         _data >>= 1 ;
      }
} ;

/*******************************************************************************
 Compile-time calculations
*******************************************************************************/
/// Get the rightmost nonzero bit at compile-time.
template<unsigned x>
struct ct_getrnzb : public std::integral_constant<unsigned, x & -x> {} ;

/// Clear the rightmost nonzero bit at compile-time.
template<unsigned x>
struct ct_clrrnzb  : public std::integral_constant<unsigned, x & (x - 1)> {} ;

namespace detail {
template<unsigned bit, unsigned v> struct ibc ;

template<unsigned v> struct ibc<1, v> : public std::integral_constant
<unsigned, (0x55555555U & v) + (0x55555555U & (v >> 1U))> {} ;

template<unsigned v> struct ibc<2, v> : public std::integral_constant
<unsigned, (0x33333333U & ibc<1, v>::value) + (0x33333333U & (ibc<1, v>::value >> 2U))> {} ;

template<unsigned v> struct ibc<4, v> : public std::integral_constant
<unsigned, (ibc<2, v>::value + (ibc<2, v>::value >> 4U)) & 0x0f0f0f0fU> {} ;

template<unsigned v> struct ibc<8, v> : public std::integral_constant
<unsigned, (ibc<4, v>::value + (ibc<4, v>::value >> 8U))> {} ;
}

/// Count nonzero bits in unsigned N at compile-time (the same as bitop::bitcount(),
/// but at compile-time).
template<unsigned x>
struct ct_bitcount : std::integral_constant
<unsigned, (detail::ibc<8, x>::value + (detail::ibc<8, x>::value >> 16U)) & 0x0000003fU> {} ;

/// Get a position of the rightmost nonzero bit at compile-time.
template<unsigned x>
struct ct_rnzbpos : std::integral_constant<int, (int)ct_bitcount<~(-(x & -x))>::value - 1> {} ;

template<typename U, U i>
struct ct_lnzbpos_value ;

template<uint8_t i>
struct ct_lnzbpos_value<uint8_t, i> :
         std::integral_constant
<int,
 (i > 127 ? 7 :
  i > 63  ? 6 :
  i > 31  ? 5 :
  i > 15  ? 4 :
  i > 7   ? 3 :
  i > 3   ? 2 :
  i > 1)> {} ;

template<uint16_t i>
struct ct_lnzbpos_value<uint16_t, i> : std::integral_constant
<int, ct_lnzbpos_value<uint8_t, (i >= 0x100U ? (i >> 8U) : i)>::value + (i >= 0x100U ? 8 : 0)>
{} ;

template<uint32_t i>
struct ct_lnzbpos_value<uint32_t, i> : std::integral_constant
<int, ct_lnzbpos_value<uint16_t, (i >= 0x10000U ? (i >> 16U) : i)>::value + (i >= 0x10000U ? 16 : 0)>
{} ;

/// Get a position of the leftmost nonzero bit at compile-time.
template<uint64_t i>
struct ct_lnzbpos  : std::integral_constant
<int, ct_lnzbpos_value<uint32_t, (i >= 0x100000000ULL ? (i >> 32ULL) : i)>::value + (i >= 0x100000000ULL ? 32 : 0)>
{} ;

template<> struct ct_lnzbpos<0>  : std::integral_constant<int, -1> {} ;

template<uint64_t i>
struct ct_log2ceil : std::integral_constant
<int, ct_lnzbpos<i>::value + (ct_getrnzb<i>::value != i) > {} ;

template<uint64_t i>
using ct_log2floor = ct_lnzbpos<i> ;

} // pcomn::bitop

template<unsigned v, unsigned s>
struct ct_shl : public detail::_ct_shl<v, s, (s < bitsizeof(unsigned)) > {} ;

template<unsigned v1,
         unsigned v2 = (unsigned)-1, unsigned v3 = (unsigned)-1, unsigned v4 = (unsigned)-1,
         unsigned v5 = (unsigned)-1, unsigned v6 = (unsigned)-1, unsigned v7 = (unsigned)-1,
         unsigned v8 = (unsigned)-1>
struct one_of {
      static const unsigned msz = bitsizeof(unsigned) ;
      static const unsigned mask =
         ct_shl<1U, v1>::value | ct_shl<1U, v2>::value | ct_shl<1U, v3>::value |
         ct_shl<1U, v4>::value | ct_shl<1U, v5>::value | ct_shl<1U, v6>::value |
         ct_shl<1U, v7>::value | ct_shl<1U, v8>::value ;
      PCOMN_STATIC_CHECK(v1 < msz && v2+1 <= msz && v3+1 <= msz && v4+1 <= msz &&
                         v5+1 <= msz && v6+1 <= msz && v7+1 <= msz && v8+1 <= msz) ;
      static constexpr bool is(unsigned value) { return !!(mask & (1U<<value)) ; }
} ;

} // end of namespace pcomn

#endif /* __PCOMN_INTEGER_H */
