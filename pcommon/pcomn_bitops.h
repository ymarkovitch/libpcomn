/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_BITOPS_H
#define __PCOMN_BITOPS_H
/*******************************************************************************
 FILE         :   pcomn_bitops.h
 COPYRIGHT    :   Yakov Markovitch, 2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Basic bit operations, both using common C integer ariphmetic
                  and using CPU-specifics instructions/intrinsics

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   22 Mar 2016
*******************************************************************************/
/** @file
 Basic operations over bits of integral data types, both using common C integer arithmetic
 and CPU-specific instructions/intrinsics.
*******************************************************************************/
#include <pcomn_meta.h>
#include <pcomn_macros.h>
#include <pcomn_platform.h>

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

/******************************************************************************/
/** bit_traits<n> describes bit operations on integers of size n bits.
*******************************************************************************/
template<int nbits>
struct bit_traits ;

/******************************************************************************/
/** bit_traits specialization for 64-bit integers
*******************************************************************************/
template<> struct bit_traits<64> {

   typedef int64_t  stype ;
   typedef uint64_t utype ;

   static unsigned bitcount(utype value)
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

   static int log2floor(utype value)
   {
      utype x = value ;
      x |= (x >> 1) ;
      x |= (x >> 2) ;
      x |= (x >> 4) ;
      x |= (x >> 8) ;
      x |= (x >> 16) ;
      x |= (x >> 32) ;
      return (int)bitcount(x) - 1 ;
   }

   static int log2ceil(utype value)
   {
      const utype correction = value ;
      return log2floor(value) + !!(correction & (correction - 1)) ;
   }
} ;


/******************************************************************************/
/** bit_traits specialization for 32-bit integers
*******************************************************************************/
template<> struct bit_traits<32> {

   typedef int       stype ;
   typedef unsigned  utype ;

   static unsigned bitcount(utype value)
   {
      unsigned r = value ;
      r = (0x55555555U & r) + (0x55555555U & (r >> 1U)) ;
      r = (0x33333333U & r) + (0x33333333U & (r >> 2U)) ;
      r = (r + (r >> 4U)) & 0x0f0f0f0fU ;
      return (r + (r >> 8U) + (r >> 16U) + (r >> 24U)) & 0x3fU ;
   }

   static int log2floor(utype value)
   {
      unsigned x = value ;
      x |= (x >> 1) ;
      x |= (x >> 2) ;
      x |= (x >> 4) ;
      x |= (x >> 8) ;
      x |= (x >> 16) ;
      return (int)bitcount((utype)x) - 1 ;
   }

   static int log2ceil(utype value)
   {
      const unsigned correction = value ;
      return log2floor(value) + !!(correction & (correction - 1)) ;
   }
} ;

/******************************************************************************/
/** bit_traits specialization for 16-bit integers
*******************************************************************************/
template<> struct bit_traits<16> {

   typedef int16_t  stype ;
   typedef uint16_t utype ;

   static unsigned bitcount(utype value)
   {
      unsigned r = value ;
      r = (0x5555U & r) + (0x5555U & (r >> 1U)) ;
      r = (0x3333U & r) + (0x3333U & (r >> 2U)) ;
      r = (r + (r >> 4U)) & 0x0f0fU ;
      return (r + (r >> 8U)) & 0x1fU ;
   }

   static int log2floor(utype value)
   {
      unsigned x = static_cast<utype>(value) ;
      x |= (x >> 1) ;
      x |= (x >> 2) ;
      x |= (x >> 4) ;
      x |= (x >> 8) ;
      return (int)bitcount((utype)x) - 1 ;
   }

   static int log2ceil(utype value)
   {
      const unsigned correction = value ;
      return log2floor(value) + !!(correction & (correction - 1)) ;
   }
} ;

/******************************************************************************/
/** bit_traits specialization for 8-bit integers
*******************************************************************************/
template<> struct bit_traits<8> {

   typedef int8_t  stype ;
   typedef uint8_t utype ;

   static unsigned bitcount(utype value)
   {
      unsigned r = value ;
      r = (0x55U & r) + (0x55U & (r >> 1U)) ;
      r = (0x33U & r) + (0x33U & (r >> 2U)) ;
      return (r + (r >> 4U)) & 0xfU ;
   }

   static int log2floor(utype value)
   {
      // At the end, x will have the same most significant 1 as value and all '1's below
      unsigned x = value ;
      x |= (x >> 1) ;
      x |= (x >> 2) ;
      x |= (x >> 4) ;
      return (int)bitcount((utype)x) - 1 ;
   }

   static int log2ceil(utype value)
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
template<typename I>
inline size_t native_bitcount(I v, sse42_isa_tag)
{
   size_t result ;
   const uintmax_t source = (std::make_unsigned_t<I>)v ;
   // POPCNT sets ZF, thus "cc" constraint
   asm(
      "popcnt %[source], %[result]"
      : [result]"=r"(result)
      : [source]"rm"(source) : "cc"
      ) ;
   return result ;
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

} // end of namespace pcomn

#endif /* __PCOMN_BITOPS_H */
