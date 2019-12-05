/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_BITOPS_H
#define __PCOMN_BITOPS_H
/*******************************************************************************
 FILE         :   pcomn_bitops.h
 COPYRIGHT    :   Yakov Markovitch, 2016-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Basic bit operations, both using common C integer ariphmetic
                  and using CPU-specifics instructions/intrinsics

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   22 Mar 2016
*******************************************************************************/
/** @file
 Basic operations over bits of integral data types, both using common C integer arithmetic
 and CPU-specific instructions/intrinsics.

 bitop::bitcount
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
*******************************************************************************/

#include <pcomn_meta.h>
#include <pcomn_assert.h>
#include <pcommon.h>

#include <functional>
#include <iterator>
#include <limits>
#include <cstdlib>
#include <limits.h>

#ifdef PCOMN_COMPILER_GNU
#  if defined(PCOMN_PL_X86)
#     include <x86intrin.h>
#  endif
#endif

namespace pcomn {

/// Calculate at compile-time the number of bits in a type or value.
#define bitsizeof(t) (sizeof(t) * CHAR_BIT)

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

   static constexpr unsigned bitcount(utype value)
   {
      utype r = value ;
      r = (0x5555555555555555ULL & r) + (0x5555555555555555ULL & (r >> 1U)) ;
      r = (0x3333333333333333ULL & r) + (0x3333333333333333ULL & (r >>  2U)) ;
      r = (r + (r >> 4U)) & 0x0f0f0f0f0f0f0f0fULL ;
      r += r >> 8U ;
      // Attempt to use a bit more of the out-of-order parallelism
      return
         ((unsigned)(r >> 48U) + (unsigned)(r >> 32U) + (unsigned)(r >> 16U) + (unsigned)r) & 0x7fU ;
   }

   static constexpr int log2floor(utype value)
   {
      #if defined(PCOMN_COMPILER_GNU) && defined(__BMI2__)
      return 63 - __builtin_clzll(value) ;
      #else

      utype x = value ;
      x |= (x >> 1) ;
      x |= (x >> 2) ;
      x |= (x >> 4) ;
      x |= (x >> 8) ;
      x |= (x >> 16) ;
      x |= (x >> 32) ;
      return (int)bitcount(x) - 1 ;

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

   static constexpr unsigned bitcount(utype value)
   {
      unsigned r = value ;
      r = (0x55555555U & r) + (0x55555555U & (r >> 1U)) ;
      r = (0x33333333U & r) + (0x33333333U & (r >> 2U)) ;
      r = (r + (r >> 4U)) & 0x0f0f0f0fU ;
      return (r + (r >> 8U) + (r >> 16U) + (r >> 24U)) & 0x3fU ;
   }

   static constexpr int log2floor(utype value)
   {
      #if defined(PCOMN_COMPILER_GNU) && defined(__BMI2__)
      return 31 - __builtin_clz(value) ;
      #else

      unsigned x = value ;
      x |= (x >> 1) ;
      x |= (x >> 2) ;
      x |= (x >> 4) ;
      x |= (x >> 8) ;
      x |= (x >> 16) ;
      return (int)bitcount((utype)x) - 1 ;

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

   static constexpr unsigned bitcount(utype value)
   {
      unsigned r = value ;
      r = (0x5555U & r) + (0x5555U & (r >> 1U)) ;
      r = (0x3333U & r) + (0x3333U & (r >> 2U)) ;
      r = (r + (r >> 4U)) & 0x0f0fU ;
      return (r + (r >> 8U)) & 0x1fU ;
   }

   static constexpr int log2floor(utype value)
   {
      #if defined(PCOMN_COMPILER_GNU) && defined(__BMI2__)
      return 31 - __builtin_clz(value) ;
      #else

      unsigned x = static_cast<utype>(value) ;
      x |= (x >> 1) ;
      x |= (x >> 2) ;
      x |= (x >> 4) ;
      x |= (x >> 8) ;
      return (int)bitcount((utype)x) - 1 ;

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

   static constexpr unsigned bitcount(utype value)
   {
      unsigned r = value ;
      r = (0x55U & r) + (0x55U & (r >> 1U)) ;
      r = (0x33U & r) + (0x33U & (r >> 2U)) ;
      return (r + (r >> 4U)) & 0xfU ;
   }

   static constexpr int log2floor(utype value)
   {
      #if defined(PCOMN_COMPILER_GNU) && defined(__BMI2__)
      return 31 - __builtin_clz(value) ;
      #else

      // At the end, x will have the same most significant 1 as value and all '1's below
      unsigned x = value ;
      x |= (x >> 1) ;
      x |= (x >> 2) ;
      x |= (x >> 4) ;
      return (int)bitcount((utype)x) - 1 ;

      #endif
   }

   static constexpr int log2ceil(utype value)
   {
      const unsigned correction = value ;
      return log2floor(value) + !!(correction & (correction - 1)) ;
   }
} ;

/*******************************************************************************
 Generic bitcount
*******************************************************************************/
template<typename I>
inline size_t native_bitcount(I v, generic_isa_tag)
{
   return bit_traits<sizeof(I)*8>::bitcount(v) ;
}

template<typename I>
inline size_t native_rzcnt(I v, generic_isa_tag)
{
   // Get rightmost non-zero bit
   // 00001010 -> 00000010
   const I rnzb = static_cast<I>(v & (0 - v)) ;
   // Convert rigthmost 0-sequence to 1-sequence and count bits
   return native_bitcount(static_cast<I>(rnzb - 1), native_isa_tag()) ;
}

/*******************************************************************************
 x86_64
*******************************************************************************/
#ifdef PCOMN_PL_X86
#if defined(PCOMN_COMPILER_GNU)
__forceinline size_t builtin_popcount(unsigned long long v) { return __builtin_popcountll(v) ; }
__forceinline size_t builtin_popcount(unsigned long v) { return __builtin_popcountl(v) ; }
__forceinline size_t builtin_popcount(unsigned int v) { return __builtin_popcount(v) ; }
__forceinline size_t builtin_popcount(unsigned short v) { return __builtin_popcount(v) ; }
__forceinline size_t builtin_popcount(unsigned char v) { return __builtin_popcount(v) ; }

template<typename I>
inline size_t native_bitcount(I v, sse42_isa_tag)
{
   return builtin_popcount((std::make_unsigned_t<I>)v) ;
}

template<typename I>
inline size_t native_rzcnt(I v, x86_64_isa_tag)
{
   size_t result ;
   const uintmax_t source = (std::make_unsigned_t<I>)v ;
   const uintmax_t bitsize = bitsizeof(I) ;
   asm(
      "bsf   %[source],  %[result]\n\t"
      "cmovz %[bitsize], %[result]"
      : [result]"=r"(result)
      : [source]"rm"(source),
        [bitsize]"r"(bitsize)
      : "cc") ;
   return result ;
}

#endif
#endif

/// @cond
namespace detail {

template<unsigned v, unsigned s, bool> struct _ct_shl ;
template<unsigned v, unsigned s> struct _ct_shl<v, s, true> :
         public std::integral_constant<unsigned, (v << s)> {} ;
template<unsigned v, unsigned s> struct _ct_shl<v, s, false> :
         public std::integral_constant<unsigned, 0U> {} ;

} // end of namespace pcomn::detail
/// @endcond

/******************************************************************************/
/** A traits class template that abstracts properties for a given integral type.

The defined property set is such that to allow to implement generic
bit-manipulation algorithms.
*******************************************************************************/
template<typename T>
struct int_traits {
      PCOMN_STATIC_CHECK(std::is_integral<T>::value) ;

      typedef T type ;
      typedef typename std::make_signed_t<type>    stype ;
      typedef typename std::make_unsigned_t<type>  utype ;

      static constexpr const bool     is_signed = std::numeric_limits<type>::is_signed ;
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

/******************************************************************************/
/** Type trait checks whether T is an integral type and @em not bool.
*******************************************************************************/
template<typename T>
struct is_integer : std::bool_constant<std::is_integral<T>::value && !std::is_same<T, bool>::value> {} ;

template<typename T>
struct is_numeric : std::bool_constant<std::is_arithmetic<T>::value && !std::is_same<T, bool>::value> {} ;

/******************************************************************************/
/** Overload enabler, a la enable_if<>.
 If T is an integer type, returns (as internal typedef) R as 'type'
*******************************************************************************/
template<typename T, typename R = T> struct
if_integer : std::enable_if<is_integer<T>::value, R> {} ;

template<typename T, typename R = T> struct
if_not_integer : disable_if<is_integer<T>::value, R> {} ;

template<typename T, typename R = T> struct
if_signed_int : std::enable_if<(is_integer<T>::value && std::numeric_limits<T>::is_signed), R> {} ;

template<typename T, typename R = T> struct
if_unsigned_int : std::enable_if<(is_integer<T>::value && !std::numeric_limits<T>::is_signed), R> {} ;

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
inline unsigned bitcount(I i)
{
   return native_bitcount(i, native_isa_tag()) ;
}

/// Count 1s in a bit vector
template<typename InputIterator>
size_t bitcount(InputIterator data, size_t nelements)
{
   size_t cnt = 0 ;
   for (; nelements-- ; ++data)
      cnt += bitcount(*data) ;
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
template<typename I>
constexpr inline if_integer_t<I, size_t> rzcnt(I x)
{
   return native_rzcnt(x, native_isa_tag()) ;
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

/// Given a bit position, get the position of a cell containing specified bit in the
/// array of integral-type items.
template<typename I>
constexpr inline if_integer_t<I, size_t> cellndx(size_t pos)
{
   return pos / int_traits<I>::bitsize ;
}

/// Given a bit position, get the bit index inside the corresponding cell.
/// @return 0 <= bitndx(pos) < bisizeof(I)
template<typename I>
constexpr inline if_integer_t<I, size_t> bitndx(size_t pos)
{
   return pos & (int_traits<I>::bitsize - 1) ;
}

template<typename I>
constexpr inline if_integer_t<I> bitmask(size_t pos)
{
   return std::integral_constant<I, 1>::value << bitndx<I>(pos) ;
}

template<typename I>
constexpr inline if_integer_t<I> tailmask(size_t bitcnt)
{
   return ~(~I(1) << bitndx<I>(bitcnt - 1)) ;
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

/// Set bits in the @a target selected by the @a mask to corresponding bits from the
/// second arguments (@a bits).
///
template<typename T>
constexpr inline std::enable_if_t<ct_and<std::is_integral<T>, ct_not<is_same_unqualified<T, bool>>>::value, T>
set_bits_masked(T target, T bits, T mask)
{
   return target &~ mask | bits & mask ;
}

/******************************************************************************/
/** Iterate over nonzero bits of an integer, from LSB to MSB.

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

/// Count nonzero bits in unsigned N at compile-time (the same as bitop::bitcount(),
/// but at compile-time).
template<unsigned x>
struct ct_bitcount : std::integral_constant
<unsigned, (detail::ibc<8, x>::value + (detail::ibc<8, x>::value >> 16U)) & 0x0000003fU> {} ;

/// Get a position of the rightmost nonzero bit at compile-time.
template<unsigned x>
struct ct_rnzbpos : std::integral_constant<int, (int)ct_bitcount<~(-(int)(x & -(int)x))>::value - 1> {} ;

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
      ((1ULL << underlying_int(m1)), (1ULL << underlying_int(ms))...) & (1ULL << underlying_int(v)) ;
}

} // end of namespace pcomn

#endif /* __PCOMN_BITOPS_H */
