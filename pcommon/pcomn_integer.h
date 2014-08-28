/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_INTEGER_H
#define __PCOMN_INTEGER_H
/*******************************************************************************
 FILE         :   pcomn_integer.h
 COPYRIGHT    :   Yakov Markovitch, 2006-2014. All rights reserved.
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
template<typename T, typename R = T>
struct if_integer : public std::enable_if<std::numeric_limits<T>::is_integer, R> {} ;

template<typename T, typename R = T>
struct if_not_integer : public disable_if<std::numeric_limits<T>::is_integer, R> {} ;

template<typename T, typename R = T>
struct if_signed_int : public std::enable_if<std::numeric_limits<T>::is_integer &&
                                             std::numeric_limits<T>::is_signed, R> {} ;

template<typename T, typename R = T>
struct if_unsigned_int : public std::enable_if<std::numeric_limits<T>::is_integer &&
                                               !std::numeric_limits<T>::is_signed, R> {} ;

template<typename T>
inline constexpr typename if_signed_int<T, T>::type sign_bit(T value)
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

   typedef int8   stype ;
   typedef uint8  utype ;

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
inline typename if_integer<I>::type clrrnzb(I x)
{
   return static_cast<I>(x & (x - 1)) ;
}

/// Get Rightmost Non-Zero Bit.
/// 00001010 -> 00000010
/// If there is no such bit, returns 0
template<typename I>
inline typename if_integer<I>::type getrnzb(I x)
{
   return static_cast<I>(x & -x) ;
}

/// Get Rightmost Zero Bit.
/// 01001111 -> 00010000
/// If there is no such bit, returns 0
template<typename I>
inline typename if_integer<I>::type getrzb(I x)
{
   return static_cast<I>(~x & (x + 1)) ;
}

/// Get Rightmost Zero Bit Sequence.
/// 00101000 -> 00000111
/// If there is no such bit, returns 0
template<typename I>
inline typename if_integer<I>::type getrzbseq(I x)
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
class nzbit_iterator : public std::iterator<std::forward_iterator_tag, I> {
   public:
      explicit nzbit_iterator(typename if_integer<I>::type value) :
         _data(value)
      {}

      // Construct the end iterator
      // Please note that the end iterator _can_ be dereferenced and, by design,
      // returns 0
      nzbit_iterator() :
         _data()
      {}

      I operator*() const { return getrnzb(_data) ; }

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
inline nzbit_iterator<typename if_integer<I>::type> make_nzbit_iterator(I value)
{
   return nzbit_iterator<I>(value) ;
}

/*******************************************************************************
                     template<typename I>
                     class nzbitpos_iterator
*******************************************************************************/
/** This iterators traverses bit positions instead of bit values.
 It successively returns positions of nonzero bits.
 E.g.
 for (nzbitpos_iterator<unsigned> foo_iter (0x20005), foo_end ; foo_iter != foo_end ; ++foo_iter)
   cout << *foo_iter << std::endl ;
 will print:
 0
 2
 17
*************************************************************************/
template<typename I>
class nzbitpos_iterator : public std::iterator<std::forward_iterator_tag, int> {
      typedef typename int_traits<I>::utype datatype ;
   public:
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

      constexpr int operator*() { return _pos ; }

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
 Bitwise functors
*******************************************************************************/
/// Calculate a | b.
template<class T>
struct bor : std::binary_function<T, T, T>
{
   T operator() (const T &t1, const T &t2) const
   {
      return t1 | t2 ;
   }
} ;

/// Calculate a & b.
template<class T>
struct band : std::binary_function<T, T, T>
{
   T operator() (const T &t1, const T &t2) const
   {
      return t1 & t2 ;
   }
} ;

/// Calculate a ^ b.
template<class T>
struct bxor : std::binary_function<T, T, T>
{
   T operator() (const T &t1, const T &t2) const
   {
      return t1 ^ t2 ;
   }
} ;

/// Calculate ~a.
template<class T>
struct bnot : std::unary_function<T, T>
{
   T operator() (const T &t) const
   {
      return ~t ;
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

/// Count nonzero bits in unsigned N at compile-time (the same as bitop::bitcount(),
/// but at compile-time).
template<unsigned i>
struct ct_bitcount {
   private:
      template<unsigned bit, class = void> struct _ibc {} ;

      template<class T> struct _ibc<1, T> : public std::integral_constant
      <unsigned, (0x55555555U & i) + (0x55555555U & (i >> 1U))> {} ;

      template<class T> struct _ibc<2, T> : public std::integral_constant
      <unsigned, (0x33333333U & _ibc<1>::value) + (0x33333333U & (_ibc<1>::value >> 2U))> {} ;

      template<class T> struct _ibc<4, T> : public std::integral_constant
      <unsigned, (_ibc<2>::value + (_ibc<2>::value >> 4U)) & 0x0f0f0f0fU> {} ;

      template<class T> struct _ibc<8, T> : public std::integral_constant
      <unsigned, (_ibc<4>::value + (_ibc<4>::value >> 8U))> {} ;

   public:
      static const unsigned value =
         (_ibc<8>::value + (_ibc<8>::value >> 16U)) & 0x0000003fU ;
} ;

template<unsigned i>
const unsigned ct_bitcount<i>::value ;

/// Get a position of the rightmost nonzero bit at compile-time.
template<unsigned x>
struct ct_rnzbpos { static const int value = (int)ct_bitcount<~(-(x & -x))>::value - 1 ; } ;

template<unsigned x>
const int ct_rnzbpos<x>::value ;

/// Get a position of the leftmost nonzero bit at compile-time.
template<uint32_t i>
class ct_lnzbpos {
#define PW_(shift) (((i << (shift)) & MB) ? (POW - (shift))
#define PWEND_ -1))))))))))))))))))))))))))))))))

      enum { MB = ((uint32_t)~(uint32_t)0 >> 1) + 1, POW = 31 } ;
   public:
      static const int value =
         (PW_(0) : PW_(1) : PW_(2) : PW_(3) : PW_(4) : PW_(5) : PW_(6) : PW_(7) :
          PW_(8) : PW_(9) : PW_(10) : PW_(11) : PW_(12) : PW_(13) : PW_(14) : PW_(15) :
          PW_(16) : PW_(17) : PW_(18) : PW_(19) : PW_(20) : PW_(21) : PW_(22) : PW_(23) :
          PW_(24) : PW_(25) : PW_(26) : PW_(27) : PW_(28) : PW_(29) : PW_(30) : PW_(31) :
          PWEND_) ;

#undef PW_
#undef PWEND_
} ;

template<uint32_t i>
const int ct_lnzbpos<i>::value ;

template<uint32_t i>
struct ct_log2ceil { static const int value = ct_lnzbpos<i>::value + (ct_getrnzb<i>::value != i) ; } ;

template<uint32_t i>
const int ct_log2ceil<i>::value ;

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
      static bool is(unsigned value) { return !!(mask & (1U<<value)) ; }
} ;

} // end of namespace pcomn

#endif /* __PCOMN_INTEGER_H */
