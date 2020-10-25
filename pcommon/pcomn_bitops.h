/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_BITOPS_H
#define __PCOMN_BITOPS_H
/*******************************************************************************
 FILE         :   pcomn_bitops.h
 COPYRIGHT    :   Yakov Markovitch, 2016-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Basic bit operations, both using common C integer ariphmetic
                  and using CPU-specifics instructions/intrinsics

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   22 Mar 2016
*******************************************************************************/
/** @file
 Basic operations over bits of integral data types, both using common C integer arithmetic
 and CPU-specific instructions/intrinsics.

 bitop::popcount
 bitop::log2floor
 bitop::log2ceil

 bitop::clrrnzb   - Clear Rightmost Non-Zero Bit      (00001010 -> 00001000)
 bitop::getrnzb   - Get Rightmost Non-Zero Bit        (00001010 -> 00000010).
 bitop::getrzb    - Get Rightmost Zero Bit            (01001111 -> 00010000)
 bitop::getrzbseq - Get Rightmost Zero Bit Sequence   (00101000 -> 00000111,
                                                      if all bits are 1s, returns 0)
 bitop::rzcnt     - Rightmost Zero Bit Count          (00101000 -> 3, 00101001 -> 0, 0 -> bitsizeof(I))

 bitop::tstpow2   - Test if Power of 2                (00000001 -> true, 00011000 -> false, 0 -> false)
 bitop::tstpow2z  - Test if Power of 2 or Zero        (00010000 -> true, 00011000 -> false, 0 -> true)

 bitop::round2z   - Round Up to Power of 2 or Zero    (0->0, 1->1, 2->2, 3->4, 5->8)

 bitop::rotl      - Rotate Left
 bitop::rotr      - Rotate Right

 bitop::bitextend - Get the integral value filled with the specified bit value.

 bitop::array_bools_to_bits<N> - Convert std::array<bool,N> to uintN_t
 bitop::bits_to_array_bools<N> - Convert uintN_t to std::array<bool,N>

 bitop::bits_extract - Extract bits from unsigned integer at the bit locations
                       specified by `mask` to contiguous low bits of the result
                       (see BMI2 PEXT instruction).
*******************************************************************************/

#include <pcomn_meta.h>
#include <pcomn_assert.h>
#include <pcommon.h>

#include <array>
#include <functional>
#include <iterator>
#include <limits>
#include <cstdlib>
#include <limits.h>

#ifdef PCOMN_GCC86_INTRINSICS
#  include <immintrin.h>
#endif

namespace pcomn {

/// Calculate at compile-time the number of bits in a type or value.
#define bitsizeof(t) (sizeof(t) * CHAR_BIT)

#if (__CLANG_VER__)
#define shuffle_simd_vector(v, ...) __builtin_shufflevector(v, v, __VA_ARGS__)
#elif (__GNUC_VER__)
#define shuffle_simd_vector(v, ...) __builtin_shuffle(v, decltype(v){__VA_ARGS__})
#endif

/*******************************************************************************
 Istruction Set Architecture variant tags
*******************************************************************************/
struct generic_isa_tag {} ;

struct x86_64_isa_tag   : generic_isa_tag {} ;

struct sse42_isa_tag    : x86_64_isa_tag {} ;

struct avx_isa_tag      : sse42_isa_tag {} ;

struct avx2_isa_tag     : avx_isa_tag {} ;

/******************************************************************************/
/** A tag for the ISA the object code is generated for.

 Note this is @em not the ISA the binary is currently executing on.
 This one is static and reflects compilation property (e.g. -march for GCC)
*******************************************************************************/
using native_isa_tag =
#if   defined(PCOMN_PL_SIMD_AVX2)
   avx2_isa_tag
#elif defined(PCOMN_PL_SIMD_AVX)
   avx_isa_tag
#elif defined(PCOMN_PL_SIMD_SSE42)
   sse42_isa_tag
#elif defined(PCOMN_PL_X86)
   x86_64_isa_tag
#else
   generic_isa_tag
#endif
   ;

/***************************************************************************//**
 bit_traits<n> describes bit operations on integers of size n bits.
*******************************************************************************/
template<int nbits>
struct bit_traits ;

template<int nbits>
using bit_stype_t = typename bit_traits<nbits>::stype ;

template<int nbits>
using bit_utype_t = typename bit_traits<nbits>::utype ;

/***************************************************************************//**
 bit_traits specialization for 64-bit integers
*******************************************************************************/
template<> struct bit_traits<64> {

   typedef int64_t  stype ;
   typedef uint64_t utype ;

   static constexpr unsigned popcount(utype value)
   {
      #if defined(PCOMN_GCC_INTRINSICS)
      return __builtin_popcountll(value) ;
      #else

      utype r = value ;
      r = (0x5555555555555555ULL & r) + (0x5555555555555555ULL & (r >> 1U)) ;
      r = (0x3333333333333333ULL & r) + (0x3333333333333333ULL & (r >>  2U)) ;
      r = (r + (r >> 4U)) & 0x0f0f0f0f0f0f0f0fULL ;
      r += r >> 8U ;
      // Attempt to use a bit more of the out-of-order parallelism
      return
         ((unsigned)(r >> 48U) + (unsigned)(r >> 32U) + (unsigned)(r >> 16U) + (unsigned)r) & 0x7fU ;
      #endif
   }

   static constexpr int log2floor(utype value)
   {
      #ifdef PCOMN_GCC_INTRINSICS

      const int result = 63 - __builtin_clzll(value) ;
      return
         #ifndef PCOMN_PL_BMI2
         !value ? -1 :
         #endif
         result ;

      #else

      utype x = value ;
      x |= (x >> 1) ;
      x |= (x >> 2) ;
      x |= (x >> 4) ;
      x |= (x >> 8) ;
      x |= (x >> 16) ;
      x |= (x >> 32) ;
      return (int)popcount(x) - 1 ;

      #endif
   }

   static constexpr int log2ceil(utype value)
   {
      const utype correction = value ;
      return log2floor(value) + !!(correction & (correction - 1)) ;
   }
} ;

/***************************************************************************//**
 bit_traits specialization for 32-bit integers
*******************************************************************************/
template<> struct bit_traits<32> {

   typedef int       stype ;
   typedef unsigned  utype ;

   static constexpr unsigned popcount(utype value)
   {
      #ifdef PCOMN_GCC_INTRINSICS
      return __builtin_popcount(value) ;
      #else

      unsigned r = value ;
      r = (0x55555555U & r) + (0x55555555U & (r >> 1U)) ;
      r = (0x33333333U & r) + (0x33333333U & (r >> 2U)) ;
      r = (r + (r >> 4U)) & 0x0f0f0f0fU ;
      return (r + (r >> 8U) + (r >> 16U) + (r >> 24U)) & 0x3fU ;

      #endif
   }

   static constexpr int log2floor(utype value)
   {
      #ifdef PCOMN_GCC_INTRINSICS

      const int result = 31 - __builtin_clz(value) ;
      return
         #ifndef PCOMN_PL_BMI2
         !value ? -1 :
         #endif
         result ;

      #else

      unsigned x = value ;
      x |= (x >> 1) ;
      x |= (x >> 2) ;
      x |= (x >> 4) ;
      x |= (x >> 8) ;
      x |= (x >> 16) ;
      return (int)popcount((utype)x) - 1 ;

      #endif
   }

   static constexpr int log2ceil(utype value)
   {
      const unsigned correction = value ;
      return log2floor(value) + !!(correction & (correction - 1)) ;
   }
} ;

/***************************************************************************//**
 bit_traits specialization for 16-bit integers
*******************************************************************************/
template<> struct bit_traits<16> {

   typedef int16_t  stype ;
   typedef uint16_t utype ;

   static constexpr unsigned popcount(utype value)
   {
      #ifdef PCOMN_GCC_INTRINSICS
      return __builtin_popcount(value) ;
      #else

      unsigned r = value ;
      r = (0x5555U & r) + (0x5555U & (r >> 1U)) ;
      r = (0x3333U & r) + (0x3333U & (r >> 2U)) ;
      r = (r + (r >> 4U)) & 0x0f0fU ;
      return (r + (r >> 8U)) & 0x1fU ;

      #endif
  }

   static constexpr int log2floor(utype value)
   {
      #ifdef PCOMN_GCC_INTRINSICS

      return bit_traits<32>::log2floor(value) ;

      #else

      unsigned x = static_cast<utype>(value) ;
      x |= (x >> 1) ;
      x |= (x >> 2) ;
      x |= (x >> 4) ;
      x |= (x >> 8) ;
      return (int)popcount((utype)x) - 1 ;

      #endif
   }

   static constexpr int log2ceil(utype value)
   {
      const unsigned correction = value ;
      return log2floor(value) + !!(correction & (correction - 1)) ;
   }
} ;

/***************************************************************************//**
 bit_traits specialization for 8-bit integers
*******************************************************************************/
template<> struct bit_traits<8> {

   typedef int8_t  stype ;
   typedef uint8_t utype ;

   static constexpr unsigned popcount(utype value)
   {
      #if defined(PCOMN_GCC_INTRINSICS)
      return __builtin_popcount(value) ;
      #else

      unsigned r = value ;
      r = (0x55U & r) + (0x55U & (r >> 1U)) ;
      r = (0x33U & r) + (0x33U & (r >> 2U)) ;
      return (r + (r >> 4U)) & 0xfU ;

      #endif
   }

   static constexpr int log2floor(utype value)
   {
      #ifdef PCOMN_GCC_INTRINSICS

      return bit_traits<32>::log2floor(value) ;

      #else

      // At the end, x will have the same most significant 1 as value and all '1's below
      unsigned x = value ;
      x |= (x >> 1) ;
      x |= (x >> 2) ;
      x |= (x >> 4) ;
      return (int)popcount((utype)x) - 1 ;

      #endif
   }

   static constexpr int log2ceil(utype value)
   {
      const unsigned correction = value ;
      return log2floor(value) + !!(correction & (correction - 1)) ;
   }
} ;

/// @cond
namespace detail {

template<unsigned v, unsigned s, bool> struct _ct_shl ;
template<unsigned v, unsigned s> struct _ct_shl<v, s, true> :
         public std::integral_constant<unsigned, (v << s)> {} ;
template<unsigned v, unsigned s> struct _ct_shl<v, s, false> :
         public std::integral_constant<unsigned, 0U> {} ;

} // end of namespace pcomn::detail
/// @endcond

/***************************************************************************//**
 A traits class template that abstracts properties for a given integral type.

The defined property set is such that to allow to implement generic
bit-manipulation algorithms.
*******************************************************************************/
template<typename T>
struct int_traits {
      PCOMN_STATIC_CHECK(std::is_integral<T>::value) ;

      typedef T type ;
      typedef typename std::make_signed_t<type>    stype ;
      typedef typename std::make_unsigned_t<type>  utype ;

      static constexpr const bool     is_signed = std::is_signed<type>::value ;
      static constexpr const unsigned bitsize = bitsizeof(type) ;
      static constexpr const type     ones = (type)~type() ;
      static constexpr const type     signbit = (type)((type)1 << (bitsize - 1)) ;
} ;

template<typename T>
constexpr const bool int_traits<T>::is_signed ;
template<typename T>
constexpr const unsigned int_traits<T>::bitsize ;
template<typename T>
constexpr const T int_traits<T>::ones ;
template<typename T>
constexpr const T int_traits<T>::signbit ;

/***************************************************************************//**
 Type trait checks whether T is an integral type and @em not bool.
*******************************************************************************/
template<typename T>
struct is_integer : std::bool_constant<std::is_integral<T>::value && !std::is_same<T, bool>::value> {} ;

template<typename T>
struct is_numeric : std::bool_constant<std::is_arithmetic<T>::value && !std::is_same<T, bool>::value> {} ;

/***************************************************************************//**
 Overload enabler, a la enable_if<>.
 If T is an integer type, returns (as internal typedef) R as 'type'
*******************************************************************************/
template<typename T, typename R = T> struct
if_integer : std::enable_if<is_integer<T>::value, R> {} ;

template<typename T, typename R = T> struct
if_not_integer : disable_if<is_integer<T>::value, R> {} ;

template<typename T, typename R = T> struct
if_signed_int : std::enable_if<(is_integer<T>::value && std::is_signed<T>::value), R> {} ;

template<typename T, typename R = T> struct
if_unsigned_int : std::enable_if<(is_integer<T>::value && !std::is_signed<T>::value), R> {} ;

template<typename T, typename R = T> struct
if_numeric : std::enable_if<is_numeric<T>::value, R> {} ;

template<typename T, typename R = T> struct
if_arithmetic : std::enable_if<std::is_arithmetic<T>::value, R> {} ;

template<typename T, typename R = T>
using if_integer_t = typename if_integer<T, R>::type ;
template<typename T, typename R = T>
using if_not_integer_t = typename if_not_integer<T, R>::type ;
template<typename T, typename R = T>
using if_signed_int_t = typename if_signed_int<T, R>::type ;
template<typename T, typename R = T>
using if_unsigned_int_t = typename if_unsigned_int<T, R>::type ;
template<typename T, typename R = T>
using if_numeric_t = typename if_numeric<T, R>::type ;
template<typename T, typename R = T>
using if_arithmetic_t = typename if_arithmetic<T, R>::type ;

template<typename T>
inline constexpr if_signed_int_t<T> sign_bit(T value)
{
   return value & int_traits<T>::signbit ;
}

/*******************************************************************************
 iabs
*******************************************************************************/
template<typename T>
inline typename if_signed_int<T, T>::type iabs(T v) { return std::abs(v) ; }

template<typename T>
inline typename if_unsigned_int<T, T>::type iabs(T v) { return v ; }
/*******************************************************************************
 namespace pcomn::bitop
 Bit operations (like bit counts, etc.)
*******************************************************************************/
namespace bitop {

/// Get the integral value filled with the specified bit value.
/// E.g.
///    - `bitextend<int>(true)` is `-1`
///    - `bitextend<unsigned>(false)` is `0`
///    - `bitextend<uint64_t>(true)` is `0xffff'ffff'ffff'ffff`
///
template<typename I>
constexpr inline if_integer_t<I> bitextend(bool bit)
{
   return I() - I(bit) ;
}

/// Count 1s in a value of some integral type
template<typename I>
constexpr inline unsigned popcount(I v)
{
   return bit_traits<sizeof(I)*8>::popcount(v) ;
}

/// Count 1s in a bit vector
template<typename InputIterator>
size_t popcount(InputIterator data, size_t nelements)
{
   size_t cnt = 0 ;
   for (; nelements-- ; ++data)
      cnt += popcount(*data) ;
   return cnt ;
}

template<typename I>
constexpr inline int log2floor(I i)
{
   return bit_traits<int_traits<I>::bitsize>::log2floor(i) ;
}

template<typename I>
constexpr inline int log2ceil(I i)
{
   return bit_traits<int_traits<I>::bitsize>::log2ceil(i) ;
}

template<typename U>
constexpr inline if_unsigned_int_t<U> round2z(U u)
{
   return (U(1) << log2ceil(u)) & bitextend<U>(u) ;
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
   return static_cast<I>(x & (0 - x)) ;
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
   return static_cast<I>(~(0 - getrnzb(x))) ;
}

/// Get Rightmost Zero Bit Count.
/// 00101000 -> 3
/// 00101001 -> 0
/// 0 -> bitsizeof(I)
/// @note Analog of TZCNT
template<typename I>
constexpr inline if_integer_t<I, unsigned> rzcnt(I v)
{
   #ifdef PCOMN_GCC_INTRINSICS

   const size_t count = bitsizeof(v) <= 32 ? __builtin_ctz(v) : __builtin_ctzl(v) ;
   return
      #ifndef PCOMN_PL_BMI2
      !v ? bitsizeof(v) :
      #endif

      bitsizeof(v) >= 32 ? count : std::min(count, bitsizeof(v)) ;

   #else

   // Get rightmost non-zero bit
   // 00001010 -> 00000010
   const I rnzb = static_cast<I>(v & (0 - v)) ;
   // Convert rigthmost 0-sequence to 1-sequence and count bits
   return popcount(static_cast<I>(rnzb - 1)) ;

   #endif
}

/// Test if Power of 2 or Zero.
template<typename I>
constexpr inline if_integer_t<I, bool> tstpow2z(I x)
{
   return !clrrnzb(x) ;
}

/// Test if Power of 2.
/// 00001000 -> true
/// 00101000 -> false
template<typename I>
constexpr inline if_integer_t<I, bool> tstpow2(I x)
{
   return tstpow2z(x) && x ;
}

/// Rotate Left.
/// @note Most contemporary compilers recognize such code sequence and replace it with
/// single native CPU command equivalent.
template<typename I>
constexpr inline if_unsigned_int_t<I> rotl(I x, int r)
{
   return (x << r) | (x >> (int_traits<I>::bitsize - r)) ;
}

/// Rotate Right.
/// @note Most contemporary compilers recognize such code sequence and replace it with
/// single native CPU command equivalent.
template<typename I>
constexpr inline if_unsigned_int_t<I> rotr(I x, int r)
{
   return (x >> r) | (x << (int_traits<I>::bitsize - r)) ;
}

/// Broadcast integral operand into integral value.
/// sizeof(result) must be >= sizeof(operand).
/// E.g.
///   - `broadcasti<uint16_t>('\xAB')` == `uint16_t(0xABAB)`
///   - `broadcasti<uint64_t>('\xAB')` == `uint64_t(0xABAB'ABAB'ABAB'ABAB)`
///   - `broadcasti<uint64_t>((uint32_t)0xF00DFEED)` == `uint64_t(0xF00DFEED'F00DFEED)`
///
template<typename R, typename I>
constexpr inline std::enable_if_t<(is_integer<R>() &&
                                   is_integer<I>() &&
                                   sizeof(R)>=sizeof(I)),
                                  R>
broadcasti(I value)
{
   return ((std::make_unsigned_t<R>)~0ULL/(std::make_unsigned_t<I>)~0ULL) * value ;
}

/// Set bits in the @a target selected by the @a mask to corresponding bits from the
/// second arguments (@a bits).
///
template<typename T>
constexpr inline std::enable_if_t<ct_and<std::is_integral<T>, ct_not<is_same_unqualified<T, bool>>>::value, T>
set_bits_masked(T target, T bits, T mask)
{
   return target &~ mask | bits & mask ;
}

template<typename I>
constexpr inline if_integer_t<I, bool> bit_test(I word, uint8_t pos)
{
   return (word >> pos) & 1 ;
}

template<typename I>
constexpr inline if_integer_t<I> bit_set(I word, uint8_t pos, bool bit)
{
   const I mask = I(1) << pos ;
   const I on = word | mask ;
   const I off = word &~ mask ;
   return bit ? on : off ;
}

/*******************************************************************************
 Bit manipulations for bitvectors
*******************************************************************************/
/// Given a bit position, get the position of a cell containing specified bit in the
/// array of integral-type items.
template<typename I>
constexpr inline if_integer_t<I, size_t> cellndx(size_t pos)
{
   return pos / int_traits<I>::bitsize ;
}

template<typename I>
constexpr inline if_integer_t<I, size_t> cellcount(size_t bitcount)
{
   return cellndx<I>(bitcount + int_traits<I>::bitsize - 1) ;
}

/// Given a bit position, get the bit index inside the corresponding cell.
/// @return 0 <= bitndx(pos) < bisizeof(I)
template<typename I>
constexpr inline if_integer_t<I, size_t> bitndx(size_t pos)
{
   return pos & (int_traits<I>::bitsize - 1) ;
}

/// Get the mask to select a bit from a bitvector cell.
///
/// @tparam I  The type of bitvector cell (e.g. uint64_t).
/// @param pos The position in the bitvector.
///
/// bitmask<uint64_t>(63)==0x80'00'00'00'00'00'00'00
/// bitmask<uint64_t>(67)==0b1000
///
template<typename I>
constexpr inline if_integer_t<I> bitmask(size_t pos)
{
   return std::integral_constant<I, 1>::value << bitndx<I>(pos) ;
}

/// Get the mask to select tail bits from the last cell of a bitvector.
/// @tparam I     The type of bitvector cell (e.g. uint64_t)
/// @param bitcnt Bitvector size
///
/// tailmask<uint64_t>(67)==0b111
///
template<typename I>
constexpr inline if_integer_t<I> tailmask(size_t bitcnt)
{
   return ~(~I(1) << bitndx<I>(bitcnt - 1)) ;
}

template<typename I>
constexpr inline if_integer_t<I> headmask(size_t bitcnt)
{
   return
      #ifdef PCOMN_PL_BMI2
      !__builtin_is_constant_evaluated() ? ~_bzhi_u64(-1, bitndx<I>(bitcnt)) :
      #endif

      ~tailmask<I>(bitcnt) | -!bitndx<I>(bitcnt) ;
}

/// Get the end of the range of equal bits starting from the given position of specified
/// word.
template<typename I>
inline if_integer_t<I, size_t> find_range_boundary(I word, size_t start_bit)
{
   const unsigned start = bitndx<I>(start_bit) ;
   const I startmask = int_traits<std::make_signed_t<I>>::signbit >> (start - 1) ;
   const I leading_ones = start ? startmask : 0 ;

   word >>= start ;
   word ^= -(word & 1) ;

   return start_bit + popcount(getrzbseq(I(word | leading_ones))) ;
}

/// Get the position of first nonzero bit between 'start' and 'finish'.
/// If there is no such bit, returns 'finish'
template<typename I>
if_integer_t<I, size_t> find_first_bit(const I *bits, size_t start, size_t finish, bool bitval = 1)
{
   typedef I cell_type ;

   if (start >= finish)
      return finish ;

   const cell_type invert = cell_type(bitval) - 1 ;
   size_t ndx = cellndx<cell_type>(start) ;
   bits += ndx ;
   cell_type cell = (*bits ^ invert) >> bitndx<cell_type>(start) ;

   if (!cell)
   {
      const size_t to = cellndx<cell_type>(finish) ;
      do {
         if (++ndx >= to)
            return finish ;
         cell = (*++bits ^ invert) ;
      } while (!cell) ;
      start = ndx * int_traits<cell_type>::bitsize ;
   }
   return std::min<size_t>(start + rzcnt(cell), finish) ;
}

/***************************************************************************//**
 Iterate over nonzero bits of an integer, from LSB to MSB.

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

      constexpr bool operator==(const nzbit_iterator &rhs) const { return _data == rhs._data ; }
      constexpr bool operator!=(const nzbit_iterator &rhs) const { return !(rhs == *this) ; }

   private:
      I _data ;
} ;

// make_nzbit_iterator()
/// Construct an object of type nzbit_iterator, where the iterable types is based
/// on the data type passed as its parameter.
template<typename I>
constexpr inline nzbit_iterator<if_integer_t<I> > make_nzbit_iterator(I value)
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
      typedef std::conditional_t<(sizeof(datatype) >= sizeof(int)), int, typename int_traits<I>::stype> postype ;

      typedef std::iterator<std::forward_iterator_tag, V> ancestor ;
   public:
      using typename ancestor::value_type ;

      constexpr nzbitpos_iterator() : _data(), _pos(bitsizeof(I)) {}
      explicit nzbitpos_iterator(I value) : _data(static_cast<datatype>(value)), _pos()
      {
         advance_pos() ;
      }

      nzbitpos_iterator &operator++()
      {
         advance_pos() ;
         return *this ;
      }
      nzbitpos_iterator operator++(int)
      {
         nzbitpos_iterator<I> tmp(*this) ;
         advance_pos() ;
         return tmp ;
      }

      constexpr bool operator==(const nzbitpos_iterator &rhs) const
      {
         return _pos == rhs._pos ;
      }
      constexpr bool operator!= (const nzbitpos_iterator &rhs) const
      {
         return !(*this == rhs) ;
      }

      value_type operator*() const { return static_cast<value_type>((int)_pos) ; }

   private:
      datatype _data ;
      postype  _pos ;

      void advance_pos()
      {
         NOXCHECK((int)_pos < (int)bitsizeof(I)) ;
         _pos = rzcnt(_data) ;
         _data = clrrnzb(_data) ;
      }
} ;

template<typename T>
inline if_integer_t<T, nzbitpos_iterator<T>> bitpos_begin(T value, bool v = true)
{
   return nzbitpos_iterator<T>(value ^ ((T)v - (T)1)) ;
}

template<typename T>
constexpr inline if_integer_t<T, nzbitpos_iterator<T>> bitpos_end(T)
{
   return nzbitpos_iterator<T>() ;
}

/***************************************************************************//**
 Convert array<bool> <-> bitword
*******************************************************************************/
/**@{*/
uint8_t  array_bools_to_bits(const std::array<bool, 8> &) ;
uint16_t array_bools_to_bits(const std::array<bool, 16> &) ;
uint32_t array_bools_to_bits(const std::array<bool, 32> &) ;
uint64_t array_bools_to_bits(const std::array<bool, 64> &) ;

std::array<bool, 8>  bits_to_array_bools(uint8_t) ;
std::array<bool, 16> bits_to_array_bools(uint16_t) ;
std::array<bool, 32> bits_to_array_bools(uint32_t) ;
std::array<bool, 64> bits_to_array_bools(uint64_t) ;

inline uint8_t array_bools_to_bits(const std::array<bool, 8> &ab)
{
   const union { std::array<bool, 8> a ; uint64_t data ; } _ = {ab} ;

   #ifndef PCOMN_PL_BMI2
   return _.data * 0x0102040810204080ULL >> 56 ;
   #else
   return _pext_u64(_.data, 0x0101010101010101ULL) ;
   #endif
}

inline std::array<bool, 8> bits_to_array_bools(uint8_t bits)
{
   #ifndef PCOMN_PL_BMI2
   const uint64_t rawdata = value_to_big_endian(bits * 0x8040201008040201ULL) ;
   const union { uint64_t data ; std::array<bool, 8> a ; } _ = {(rawdata & 0x8080808080808080ULL) >> 7} ;
   #else
   const union { uint64_t data ; std::array<bool, 8> a ; } _ = {_pdep_u64(bits, 0x0101010101010101ULL)} ;
   #endif
   return _.a ;
}

template<size_t bitsize>
inline bit_utype_t<bitsize> array_bools_to_bits_generic(const std::array<bool, bitsize> &ab)
{
   const union { std::array<bool, bitsize> a ; std::array<bool, bitsize/2>  data[2] ; } _ = {ab} ;
   return
      array_bools_to_bits(_.data[0]) |
      ((bit_utype_t<bitsize>)array_bools_to_bits(_.data[1]) << (bitsize/2)) ;
}

template<typename I>
inline std::array<bool, bitsizeof(I)> bits_to_array_bools_generic(I bits)
{
   PCOMN_STATIC_CHECK(is_integer<I>() && sizeof(I) >= 2) ;

   constexpr size_t bitsize = bitsizeof(I) ;
   constexpr size_t halfsize = bitsize/2 ;

   const union { std::array<bool, halfsize> data[2] ; std::array<bool, bitsize> a ; } _ =
   {{
         bits_to_array_bools(bit_utype_t<halfsize>(bits)),
         bits_to_array_bools(bit_utype_t<halfsize>(bits >> halfsize))
   }} ;

   return _.a ;
}

inline uint16_t array_bools_to_bits(const std::array<bool, 16> &ab)
{
   #if !defined(PCOMN_PL_SIMD_SSE42) || PCOMN_WORKAROUND(__clang_major__, <6)
   return array_bools_to_bits_generic(ab) ;

   #else
   typedef uint64_t v2x8  __attribute__ ((vector_size(16))) ;
   typedef char v1x16     __attribute__ ((vector_size(16))) ;
   const union { std::array<bool, 16> a ; v2x8 data ; } _ = {ab} ;

   return __builtin_ia32_pmovmskb128((v1x16)(_.data << 7)) ;

   #endif
}

inline std::array<bool, 16> bits_to_array_bools(uint16_t bits)
{
   #if !defined(PCOMN_PL_SIMD_AVX) || PCOMN_WORKAROUND(__clang_major__, <6)
   return bits_to_array_bools_generic(bits) ;

   #else
   typedef int8_t v16i8  __attribute__ ((vector_size(16))) ;
   union {
         uint16_t vv8u16 __attribute__ ((vector_size(16))) ;
         uint64_t vv2u64 __attribute__ ((vector_size(16))) ;
         v16i8    vv16i8 ;
         std::array<bool, 16> a ;
   } result = {bits} ;

   result.vv16i8 = shuffle_simd_vector(result.vv16i8, 0,0,0,0,0,0,0,0, 1,1,1,1,1,1,1,1) ;
   result.vv2u64 |= uint64_t(0b01111111'10111111'11011111'11101111'11110111'11111011'11111101'11111110) ;
   result.vv16i8 = (result.vv16i8 == -1) & 1 ;

   return result.a ;

   #endif
}

inline uint32_t array_bools_to_bits(const std::array<bool, 32> &ab)
{
   #if !defined(PCOMN_PL_SIMD_AVX2) || PCOMN_WORKAROUND(__clang_major__, <6)
   return array_bools_to_bits_generic(ab) ;

   #else
   typedef uint64_t v4u64  __attribute__ ((vector_size(32))) ;
   typedef char v1x32     __attribute__ ((vector_size(32))) ;
   const union { std::array<bool, 32> a ; v4u64 data ; } _ = {ab} ;

   return __builtin_ia32_pmovmskb256((v1x32)(_.data << 7)) ;

   #endif
}

inline std::array<bool, 32> bits_to_array_bools(uint32_t bits)
{
   #if !defined(PCOMN_PL_SIMD_AVX2) || PCOMN_WORKAROUND(__clang_major__, <6)
   return bits_to_array_bools_generic(bits) ;

   #else
   typedef int8_t v32i8  __attribute__ ((vector_size(32))) ;
   union {
         uint32_t vv8u32 __attribute__ ((vector_size(32))) ;
         uint64_t vv4u64 __attribute__ ((vector_size(32))) ;
         v32i8    vv32i8 ;
         std::array<bool, 32> a ;
   } result = {bits} ;

   result.vv32i8 = shuffle_simd_vector(result.vv32i8,
                                       0,0,0,0,0,0,0,0, 1,1,1,1,1,1,1,1, 2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3) ;
   result.vv4u64 |= uint64_t(0b01111111'10111111'11011111'11101111'11110111'11111011'11111101'11111110) ;
   result.vv32i8 = (result.vv32i8 == -1) & 1 ;

   return result.a ;

   #endif
}

inline uint64_t array_bools_to_bits(const std::array<bool, 64> &ab)
{
   return array_bools_to_bits_generic(ab) ;
}

inline std::array<bool, 64> bits_to_array_bools(uint64_t bits)
{
   return bits_to_array_bools_generic(bits) ;
}

/**@}*/

/***************************************************************************//**
 Extract bits from unsigned integer `source` at the corresponding bit locations
 specified by `mask` to contiguous low bits of the result; the remaining upper
 bits in the result are set to zero.

 Matches the logic of PEXT - Parallel Bits Extract from BMI2 ISA, and on platforms
 supporting BMI2 is implemented with this instruction.

 bits_extract<uint8_t>(0b11110010, 0b00100111) -> 0b1010
 bits_extract<uint8_t>(0b11110000, 0b10100000) -> 0b11 ;

 bits_extract<uint32_t>(0b11110000'00000000'00000000'10000010,
                        0b00100001'00000000'00000000'11111111) -> 0b1010000010
*******************************************************************************/
/**@{*/
#ifdef PCOMN_PL_BMI2
template<typename I>
inline if_unsigned_int_t<I> bits_extract(I source, I mask)
{
   return bitsizeof(I) <= 32 ? _pext_u32(source, mask) : _pext_u64(source, mask) ;
}
#else
template<typename I>
inline if_unsigned_int_t<I> bits_extract(I source, I mask)
{
   if (mask == bitextend<I>(1))
      return source ;

   I result = 0 ;
   for (unsigned result_bitpos = 0 ; mask ; ++result_bitpos)
   {
      const unsigned bitpos =
         #ifdef PCOMN_GCC_INTRINSICS
         // In absence of BMI, __builtin_ctz() is slightly faster than bitop::rzcnt();
         // with BMI there's no difference.
         bitsizeof(mask) <= 32 ? __builtin_ctz(mask) : __builtin_ctzl(mask)
         #else
         rzcnt(mask)
         #endif
         ;

      (mask >>= bitpos) >>= 1 ;
      result |= ((source >>= bitpos) & 1) << result_bitpos ;
      source >>= 1 ;
   }
   return result ;
}
#endif
/**@}*/

/*******************************************************************************
 Compile-time calculations
*******************************************************************************/
/// Get the rightmost nonzero bit at compile-time.
template<unsigned x>
struct ct_getrnzb : public std::integral_constant<unsigned, x & -(long)x> {} ;

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

/// Count nonzero bits in unsigned N at compile-time (the same as bitop::popcount(),
/// but at compile-time).
template<unsigned x>
struct ct_popcount : std::integral_constant
<unsigned, (detail::ibc<8, x>::value + (detail::ibc<8, x>::value >> 16U)) & 0x0000003fU> {} ;

/// Get a position of the rightmost nonzero bit at compile-time.
template<unsigned x>
struct ct_rnzbpos : std::integral_constant<int, (int)ct_popcount<~(-(int)(x & -(int)x))>::value - 1> {} ;

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

/***************************************************************************//**
 Backward compatibility
*******************************************************************************/
/**@{*/
template<unsigned x>
using ct_bitcount = ct_popcount<x> ;

template<typename... Args>
auto inline bitcount(Args && ...args) -> decltype(popcount(std::forward<Args>(args)...))
{
   return popcount(std::forward<Args>(args)...) ;
}
/**@}*/

} // pcomn::bitop

template<unsigned v, unsigned s>
struct ct_shl : public detail::_ct_shl<v, s, (s < bitsizeof(unsigned)) > {} ;

template<unsigned long long v1, unsigned long long...vN>
struct one_of {
      static_assert(fold_bitor(v1, vN...) < 64, "Some values to test against exceed allowed maximum (63)") ;
      static constexpr bool is(unsigned long long value)
      {
         return !!(fold_bitor((1ULL << v1), (1ULL << vN)...) & (1ULL << value) & -(value < 64)) ;
      }
} ;

template<typename T>
constexpr inline bool is_in(T) { return false ; }

template<typename T, T v1, T...vN>
constexpr inline bool is_in(T v)
{
   return one_of<underlying_int(v1), underlying_int(vN)...>::is(underlying_int(v)) ;
}

template<typename T, typename M1, typename... Ms>
constexpr inline bool is_in(T v, M1 m1, Ms...ms)
{
   return fold_bitor<unsigned long long>
      ((1ULL << underlying_int(m1)), (1ULL << underlying_int(ms))...) &
      (1ULL << underlying_int(v)) & -((uint64_t)underlying_int(v) < 64) ;
}

} // end of namespace pcomn

#endif /* __PCOMN_BITOPS_H */
