/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_RANDOM_H
#define __PCOMN_RANDOM_H
/*******************************************************************************
 FILE         :   pcomn_random.h
 COPYRIGHT    :   Yakov Markovitch, 2016-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Super-fast pseudorandom generators

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   23 Aug 2016
*******************************************************************************/
/** @file
  Super-fast pseudorandom generators.
*******************************************************************************/
#include <pcomn_integer.h>
#include <pcomn_atomic.h>

namespace pcomn {

struct splitmix64_prng ;

template<typename>
struct xoroshiro_prng ;

typedef splitmix64_prng splitmix64 ;
template<typename I>
using fastrand = xoroshiro_prng<I> ;

/******************************************************************************/
/** Very fast trivially constructible/copyable 64bit-sized pseudorandom generator
object.

 A fixed-increment version of Java 8's SplittableRandom generator.

 This is a very fast generator passing BigCrush, it can be useful if you need
 64bit-sized object; otherwise, xoroshiro_prng has much better quality and
 2^128 bit period.

 The algorithm is by Sebastiano Vigna (vigna@acm.org), 2015
*******************************************************************************/
struct splitmix64_prng {
      /// The default constructor always starts the PRNG from the same fixed initial
      /// state, so the PRN sequences are repeatable.
      constexpr splitmix64_prng() = default ;
      /// The splitmix64 may be seeded with any seed, including 0.
      constexpr explicit splitmix64_prng(uint64_t seed) : _state(seed) {}

      uint64_t operator()()
      {
         uint64_t z = (_state += 0x9e3779b97f4a7c15ULL) ;
         z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL ;
         z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL ;
         return z ^ (z >> 31) ;
      }

      friend bool operator==(splitmix64_prng x, splitmix64_prng y) { return x._state == y._state ; }
      friend bool operator!=(splitmix64_prng x, splitmix64_prng y) { return !(x == y) ; }

   private:
      uint64_t _state = 0 ;
} ;

/******************************************************************************/
/** xoroshiro128+: a super-fast pseudorandom generator by David Blackman
 and Sebastiano Vigna, a variation on the theme of XORSHIFT by George Marsaglia.

 Of moderate quality but @em extremely fast, order of a milliard numbers per core
 per second on a modern CPU.
 Note that it has 2^128 period.
*******************************************************************************/
template<typename I>
class xoroshiro_prng {
      static_assert(is_integer<I>::value, "Invalid xoroshiro PRNG type argument, must me integral type") ;
   public:
      typedef I type ;

      /// The default constructor always starts the PRNG from the same fixed initial
      /// state, so the PRN sequences are repeatable.
      constexpr xoroshiro_prng() = default ;

      /// Set the seed for a new sequence of pseudo-random integers.
      explicit xoroshiro_prng(uint64_t s)
      {
         splitmix64 seeder {s} ;
         _s0 = seeder() ;
         _s1 = seeder() ;
      }

      type operator()()
      {
         const uint64_t result = _s0 + _s1 ;

         const uint64_t s0     = _s0 ;
         const uint64_t s1     = s0 ^ _s1 ;

         // a = 55, b = 14, c = 36
         _s0  = bitop::rotl(s0, 55) ^ s1 ^ (s1 << 14) ; // a, b
         _s1 = bitop::rotl(s1, 36) ; // c

         return result ;
      }

      /// Jump function is equivalent to 2^64 calls to operator().
      /// It can be used to generate 2^64 non-overlapping subsequences for parallel computations.
      xoroshiro_prng &jump()
      {
         uint64_t s0 = 0 ;
         uint64_t s1 = 0 ;

         auto jump_half =
            [&,this](uint64_t jumpos)
            {
               for (; jumpos ; jumpos >>= 1, (*this)())
                  if (jumpos & 1)
                  {
                     s0 ^= _s0 ;
                     s1 ^= _s1 ;
                  }
            } ;
         jump_half(0xbeac0467eba5facbULL) ;
         jump_half(0xd86b048b86aa9922ULL) ;

         _s0 = s0 ;
         _s1 = s1 ;

         return *this ;
      }

      friend bool operator==(xoroshiro_prng x, xoroshiro_prng y)
      {
         return x._s0 == y._s0 && x._s1 == y._s1 ;
      }
      friend bool operator!=(xoroshiro_prng x, xoroshiro_prng y) { return !(x == y) ; }

   private:
      // The initial generator state is two first values from splitmix64(0).
      uint64_t _s0 = 0xe220a8397b1dcdafULL ;
      uint64_t _s1 = 0x6e789e6aa1b965f4ULL ;
} ;

/******************************************************************************/
/** Specialization of xoroshiro PRNG for booleans.
*******************************************************************************/
template<>
class xoroshiro_prng<bool> {
      typedef bool type ;

      constexpr xoroshiro_prng() = default ;

      explicit xoroshiro_prng(uint64_t s) : _prng(s) {} ;

      bool operator()()
      {
         return _prng() & 1 ;
      }

      xoroshiro_prng<bool> &jump()
      {
         _prng.jump() ;
         return *this ;
      }

      friend bool operator==(xoroshiro_prng<bool> x, xoroshiro_prng<bool> y)
      {
         return x._prng == y._prng ;
      }
      friend bool operator!=(xoroshiro_prng x, xoroshiro_prng y) { return !(x == y) ; }

   private:
      xoroshiro_prng<unsigned> _prng ;
} ;

/******************************************************************************/
/** Version of xoroshiro PRNG that provides atomic next, jump, and atomic copy
 to plain xoroshiro_prng.

 Usually global object as such class used as a factory for producing thread-local
 xoroshiro_prng objects with non-overlapped random sequenced.
*******************************************************************************/
template<typename I>
class xoroshiro_prng<std::atomic<I>> {
      PCOMN_NONCOPYABLE(xoroshiro_prng) ;
      PCOMN_NONASSIGNABLE(xoroshiro_prng) ;
   public:
      typedef xoroshiro_prng<I>        prng_type ;
      typedef typename prng_type::type type ;

      xoroshiro_prng() = default ;
      explicit xoroshiro_prng(uint64_t seed) : _data{prng_type{seed}} {}
      xoroshiro_prng(const prng_type &prng) : _data{prng} {}

      xoroshiro_prng &operator=(prng_type other)
      {
         atomic_op::store(&_data, data_type{other}) ;
         return *this ;
      }

      operator prng_type() const
      {
         return atomic_op::load(&_data)->_prng ;
      }

      prng_type jump()
      {
         for(data_type result {_data}, expected {result} ;; result = expected)
         {
            result._prng.jump() ;
            if (atomic_op::cas2_strong(&_data, &expected, result))
               return result._prng ;
         }
      }

      type operator()()
      {
         for(data_type expected {_data}, next {expected} ;; next = expected)
         {
            const type result = next._prng() ;
            if (atomic_op::cas2_strong(&_data, &expected, next))
               return result ;
         }
      }

   private:
      mutable struct alignas(2*sizeof(void*)) data_type {
            prng_type _prng ;
      }
      _data ;
} ;

} // end of namespace pcomn

#endif /* __PCOMN_RANDOM_H */
