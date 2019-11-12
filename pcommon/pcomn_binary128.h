/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_BINARY128_H
#define __PCOMN_BINARY128_H
/*******************************************************************************
 FILE         :   pcomn_binary128.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Large fixed binary types (128- and 256-bit)
 CREATION DATE:   10 Jan 2000
*******************************************************************************/
/** @file
 Large fixed binary types (128- and 256-bit).
*******************************************************************************/
#include <pcomn_meta.h>
#include <pcomn_bitops.h>

#include <string>
#include <iosfwd>
#include <utility>

namespace pcomn {

struct b128_t ;
struct binary128_t ;
struct binary256_t ;

std::ostream &operator<<(std::ostream &, const b128_t &) ;
std::ostream &operator<<(std::ostream &, const binary128_t &) ;
std::ostream &operator<<(std::ostream &, const binary256_t &) ;

/// @cond
namespace detail {
/*******************************************************************************
 Bit operations on big binary types implementer.
*******************************************************************************/
struct combine_bigbinary_bits {
      template<typename T, typename F>
      __forceinline
      static constexpr T eval(const T &x, const T &y, const F &combine)
      {
         T result ;
         const uint64_t * const xd = x.idata() ;
         const uint64_t * const yd = y.idata() ;
         uint64_t * const rd = result.idata() ;

         for (size_t i = 0 ; i < sizeof(T)/sizeof(uint64_t) ; ++i)
         {
            rd[i] = combine(xd[i], yd[i]) ;
         }
         return result ;
      }

      template<typename T>
      __forceinline
      static constexpr T invert(const T &x)
      {
         T result ;
         const uint64_t * const xd = x.idata() ;
         uint64_t * const rd = result.idata() ;

         for (size_t i = 0 ; i < sizeof(T)/sizeof(uint64_t) ; rd[i] = ~xd[i], ++i) ;

         return result ;
      }
} ;

/*******************************************************************************
 Ported from Leonid Yuriev's t1ha2
*******************************************************************************/
// 'magic' primes
constexpr uint64_t t1ha_prime_0 = 0xEC99BF0D8372CAABull ;
constexpr uint64_t t1ha_prime_1 = 0x82434FE90EDCEF39ull ;
constexpr uint64_t t1ha_prime_2 = 0xD4F06DB99D67BE4Bull ;
constexpr uint64_t t1ha_prime_4 = 0x9C06FAF4D023E3ABull ;
constexpr uint64_t t1ha_prime_5 = 0xC060724A8424F345ull ;
constexpr uint64_t t1ha_prime_6 = 0xCB5AF53AE3AAAC31ull ;

/// Modern compilers convert this to rotr64 assembler instruction.
__forceinline uint64_t t1ha_rotr64(uint64_t v, unsigned s)
{
   return (v >> s) | (v << (64 - s)) ;
}

__forceinline uint64_t t1ha_mul_64x64_128(uint64_t a, uint64_t b, uint64_t * __restrict h)
{
   __uint128_t r = (__uint128_t)a * (__uint128_t)b ;
   // Modern GCC/Clang nicely optimizes this
   *h = (uint64_t)(r >> 64) ;
   return (uint64_t)r ;
}

/// XOR high and low parts of the full 128-bit product
__forceinline uint64_t t1ha_mux64(uint64_t v, uint64_t prime)
{
   uint64_t l, h ;
   l = t1ha_mul_64x64_128(v, prime, &h) ;
   return l ^ h ;
}

/// xor-mul-xor mixer
__forceinline uint64_t t1ha_mix64(uint64_t v, uint64_t p)
{
   v *= p ;
   return v ^ t1ha_rotr64(v, 41) ;
}

__forceinline void t1ha2_mixup64(uint64_t *__restrict a,
                                 uint64_t *__restrict b, uint64_t v,
                                 uint64_t prime)
{
   uint64_t h ;
   *a ^= t1ha_mul_64x64_128(*b + v, prime, &h) ;
   *b += h ;
}

__forceinline uint64_t t1ha2_final64(uint64_t a, uint64_t b)
{
   const uint64_t x = (a + t1ha_rotr64(b, 41)) * t1ha_prime_0 ;
   const uint64_t y = (t1ha_rotr64(a, 23) + b) * t1ha_prime_6 ;
   return t1ha_mux64(x ^ y, t1ha_prime_5) ;
}

} // end of namespace pcomn::detail
/// @endcond

/***************************************************************************//**
 Inlined T1HA2 specialization for 128-bit binary data.
*******************************************************************************/
/**@{*/
inline uint64_t t1ha2_bin128(uint64_t lo, uint64_t hi, uint64_t seed)
{
   uint64_t a = seed, b = 16 ;
   detail::t1ha2_mixup64(&a, &b, lo, detail::t1ha_prime_2) ;
   detail::t1ha2_mixup64(&b, &a, hi, detail::t1ha_prime_1) ;
   return detail::t1ha2_final64(a, b) ;
}

inline uint64_t t1ha2_bin128(uint64_t lo, uint64_t hi)
{
   return t1ha2_bin128(lo, hi, 0) ;
}
/**@}*/

/***************************************************************************//**
 Inlined T1HA0 specialization for 128-bit binary data.
*******************************************************************************/
/**@{*/
inline uint64_t t1ha0_bin128(uint64_t lo, uint64_t hi, uint64_t seed)
{
   constexpr uint64_t len = 16 ;
   const uint64_t b = len  + detail::t1ha_mux64(lo, detail::t1ha_prime_2) ;
   const uint64_t a = seed + detail::t1ha_mux64(hi, detail::t1ha_prime_1) ;
   // final_weak_avalanche
   return
      detail::t1ha_mux64(detail::t1ha_rotr64(a + b, 17), detail::t1ha_prime_4) +
      detail::t1ha_mix64(a ^ b, detail::t1ha_prime_0) ;
}

inline uint64_t t1ha0_bin128(uint64_t lo, uint64_t hi)
{
   return t1ha0_bin128(lo, hi, 0) ;
}
/**@}*/

/***************************************************************************//**
 128-bit binary union with by-byte, by-word, by-dword, by-qword access.

 64-bit aligned.
*******************************************************************************/
struct b128_t {
      union {
            uint64_t       _idata[2] ;
            uint32_t       _wdata[4] ;
            uint16_t       _hdata[8] ;
            unsigned char  _cdata[16] ;
      } ;

   public:
      constexpr b128_t() : _idata() {}

      constexpr b128_t(uint64_t i0, uint64_t i1) : _idata{i0, i1} {}

      constexpr b128_t(uint32_t w0, uint32_t w1, uint32_t w2, uint32_t w3) :
         _wdata{w0, w1, w2, w3}
      {}

      constexpr b128_t(uint16_t h0, uint16_t h1, uint16_t h2, uint16_t h3,
                          uint16_t h4, uint16_t h5, uint16_t h6, uint16_t h7) :
         _hdata{h0, h1, h2, h3, h4, h5, h6, h7}
      {}

      constexpr b128_t(uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3,
                          uint8_t c4, uint8_t c5, uint8_t c6, uint8_t c7,
                          uint8_t c8, uint8_t c9, uint8_t ca, uint8_t cb,
                          uint8_t cc, uint8_t cd, uint8_t ce, uint8_t cf) :
         _cdata{c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, ca, cb, cc, cd, ce, cf}
      {}

      /// Check helper
      explicit constexpr operator bool() const { return !!(_idata[0] | _idata[1]) ; }

      unsigned char *data() { return _cdata ; }
      constexpr const unsigned char *data() const { return _cdata ; }

      /// Get the count of octets (16)
      static constexpr size_t size() { return sizeof _idata ; }
      /// Get the length of string representation (32 chars)
      static constexpr size_t slen() { return 2*size() ; }

      unsigned bitcount() const
      {
         return
            bitop::bitcount(_idata[0]) +
            bitop::bitcount(_idata[1]) ;
      }

      size_t hash() const { return t1ha0_bin128(_idata[0], _idata[1]) ; }

      _PCOMNEXP std::string to_string() const ;
      _PCOMNEXP char *to_strbuf(char *buf) const ;

      uint64_t *idata() { return _idata ; }
      constexpr const uint64_t *idata() const { return _idata ; }

      template<typename T>
      static constexpr auto be(T value) -> decltype(value_to_big_endian(value))
      {
         return value_to_big_endian(value) ;
      }
} ;

PCOMN_STATIC_CHECK(sizeof(b128_t) == 16) ;
PCOMN_STATIC_CHECK(alignof(b128_t) == 8) ;

/***************************************************************************//**
 Indicate if the specified type can be interpreted as 128-bit literal.

 The type must be trivially copyable, have size 16 bytes and be at least
 8-bytes aligned.
*******************************************************************************/
template<typename T>
struct is_literal128 :
   std::bool_constant<sizeof(T) == sizeof(b128_t) &&
                      alignof(T) >= alignof(b128_t) &&
                      std::is_trivially_copyable<T>::value>
{} ;

template<typename T, typename R>
using if_literal128_t = std::enable_if_t<is_literal128<T>::value, R> ;

/***************************************************************************//**
 b128_t equality and inequality.
*******************************************************************************/
constexpr inline bool operator==(const b128_t &l, const b128_t &r)
{
   return !((l._idata[0] ^ r._idata[0]) | (l._idata[1] ^ r._idata[1])) ;
}

constexpr inline bool operator!=(const b128_t &l, const b128_t &r)
{
   return !(l == r) ;
}

/***************************************************************************//**
 b128_t bit operations (~,&,|,^).
*******************************************************************************/
/**@{*/
constexpr inline b128_t operator&(const b128_t &x, const b128_t &y)
{
   return detail::combine_bigbinary_bits::eval(x, y, std::bit_and<>()) ;
}

constexpr inline b128_t operator|(const b128_t &x, const b128_t &y)
{
   return detail::combine_bigbinary_bits::eval(x, y, std::bit_or<>()) ;
}

constexpr inline b128_t operator^(const b128_t &x, const b128_t &y)
{
   return detail::combine_bigbinary_bits::eval(x, y, std::bit_xor<>()) ;
}

constexpr inline b128_t operator~(const b128_t &x)
{
   return detail::combine_bigbinary_bits::invert(x) ;
}
/**@}*/

/***************************************************************************//**
 128-bit binary big-endian POD data, aligned to 64-bit boundary.
*******************************************************************************/
struct binary128_t : protected b128_t {
   private:
      typedef b128_t ancestor ;
      friend detail::combine_bigbinary_bits ;

   public:
      constexpr binary128_t() = default ;
      constexpr binary128_t(uint64_t hi, uint64_t lo) : ancestor(be(hi), be(lo)) {}

      constexpr binary128_t(uint16_t h1, uint16_t h2, uint16_t h3, uint16_t h4,
                            uint16_t h5, uint16_t h6, uint16_t h7, uint16_t h8) :
         ancestor(be(h1), be(h2), be(h3), be(h4), be(h5), be(h6), be(h7), be(h8))
      {}

      constexpr binary128_t(uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3,
                            uint8_t c4, uint8_t c5, uint8_t c6, uint8_t c7,
                            uint8_t c8, uint8_t c9, uint8_t ca, uint8_t cb,
                            uint8_t cc, uint8_t cd, uint8_t ce, uint8_t cf) :
         ancestor(c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, ca, cb, cc, cd, ce, cf)
      {}

      /// Create value from a hex string representation.
      /// @note @a hexstr need not be null-terminated, the constructor will scan at most
      /// 32 characters, or until '\0' encountered, whatever comes first.
      explicit binary128_t(const char *hexstr)
      {
         if (!hextob(_idata, sizeof _idata, hexstr))
            _idata[1] = _idata[0] = 0 ;
      }

      using ancestor::operator bool ;
      using ancestor::data ;
      using ancestor::size ;
      using ancestor::slen ;
      using ancestor::bitcount ;
      using ancestor::hash ;
      using ancestor::to_string ;
      using ancestor::to_strbuf ;

      constexpr uint64_t hi() const { return value_from_big_endian(_idata[0]) ; }
      constexpr uint64_t lo() const { return value_from_big_endian(_idata[1]) ; }

      /// Get the nth octet MSB-first order.
      constexpr unsigned octet(size_t n) const { return _cdata[n] ; }

      /// Get the nth hextet, MSW-first order.
      /// Every hextet is returned as host-order word.
      constexpr uint16_t hextet(size_t n) const
      {
         return value_from_big_endian(_hdata[n]) ;
      }

      /*********************************************************************//**
       Equality (==), ordering (<)
      *************************************************************************/
      friend constexpr bool operator==(const binary128_t &l, const binary128_t &r)
      {
         return *l.bdata() == *r.bdata() ;
      }

      friend constexpr bool operator<(const binary128_t &l, const binary128_t &r)
      {
         return (l.hi() < r.hi()) | (l._idata[0] == r._idata[0]) & (l.lo() < r.lo()) ;
      }

      friend std::ostream &operator<<(std::ostream &os, const binary128_t &v)
      {
         return os << *v.bdata() ;
      }

   protected:
      b128_t *bdata() { return this ; }
      constexpr const b128_t *bdata() const { return this ; }
} ;

PCOMN_STATIC_CHECK(sizeof(binary128_t) == sizeof(b128_t)) ;
PCOMN_STATIC_CHECK(alignof(binary128_t) == alignof(b128_t)) ;

// Define !=, >, <=, >= for binary128_t
PCOMN_DEFINE_RELOP_FUNCTIONS(, binary128_t) ;

/***************************************************************************//**
 binary128_t bit operations (~,&,|,^).
*******************************************************************************/
/**@{*/
constexpr inline binary128_t operator&(const binary128_t &x, const binary128_t &y)
{
   return detail::combine_bigbinary_bits::eval(x, y, std::bit_and<>()) ;
}

constexpr inline binary128_t operator|(const binary128_t &x, const binary128_t &y)
{
   return detail::combine_bigbinary_bits::eval(x, y, std::bit_or<>()) ;
}

constexpr inline binary128_t operator^(const binary128_t &x, const binary128_t &y)
{
   return detail::combine_bigbinary_bits::eval(x, y, std::bit_xor<>()) ;
}

constexpr inline binary128_t operator~(const binary128_t &x)
{
   return detail::combine_bigbinary_bits::invert(x) ;
}
/**@}*/

/***************************************************************************//**
 256-bit binary
*******************************************************************************/
struct binary256_t {

      constexpr binary256_t() : _idata() {}
      constexpr binary256_t(uint64_t q0, uint64_t q1, uint64_t q2, uint64_t q3) :
         _idata{q0, q1, q2, q3}
      {}

      /// Create value from a hex string representation.
      /// @note @a hexstr need not be null-terminated, the constructor will scan at most
      /// 64 characters, or until '\0' encountered, whatever comes first.
      explicit binary256_t(const char *hexstr)
      {
         if (!hextob(_idata, sizeof _idata, hexstr))
            *this = {} ;
         else
            flip_endianness() ;
      }

      /// Check helper
      explicit constexpr operator bool() const { return !!((_idata[0] | _idata[1]) | (_idata[2] | _idata[3])) ; }

      unsigned char *data() { return _cdata ; }
      constexpr const unsigned char *data() const { return _cdata ; }

      uint64_t *idata() { return _idata ; }
      constexpr const uint64_t *idata() const { return _idata ; }

      unsigned bitcount() const
      {
         return
            bitop::bitcount(_idata[0]) +
            bitop::bitcount(_idata[1]) +
            bitop::bitcount(_idata[2]) +
            bitop::bitcount(_idata[3]) ;
      }

      /// Get the count of octets (16)
      static constexpr size_t size() { return sizeof _idata ; }
      /// Get the length of string representation (32 chars)
      static constexpr size_t slen() { return 2*size() ; }

      size_t hash() const
      {
         return t1ha0_bin128(_idata[0], _idata[1],
                             t1ha0_bin128(_idata[3], _idata[4])) ;
      }

      _PCOMNEXP std::string to_string() const ;
      char *to_strbuf(char *buf) const
      {
         PCOMN_STATIC_CHECK(slen() >= 2*binary128_t::slen()) ;
         binary128_t(*(idata() + 3), *(idata() + 2)).to_strbuf(buf) ;
         binary128_t(*(idata() + 1), *(idata() + 0)).to_strbuf(buf + binary128_t::slen()) ;

         return buf ;
      }

      binary256_t &flip_endianness()
      {
         const uint64_t q0 = be(_idata[3]) ;
         const uint64_t q1 = be(_idata[2]) ;
         const uint64_t q2 = be(_idata[1]) ;
         const uint64_t q3 = be(_idata[0]) ;
         _idata[0] = q0 ;
         _idata[1] = q1 ;
         _idata[2] = q2 ;
         _idata[3] = q3 ;
         return *this ;
      }

      /*********************************************************************//**
       Bit operations (~,&,|,^).
      *************************************************************************/
      friend constexpr binary256_t operator&(const binary256_t &x, const binary256_t &y)
      {
         return detail::combine_bigbinary_bits::eval(x, y, std::bit_and<>()) ;
      }

      friend constexpr binary256_t operator|(const binary256_t &x, const binary256_t &y)
      {
         return detail::combine_bigbinary_bits::eval(x, y, std::bit_or<>()) ;
      }

      friend constexpr binary256_t operator^(const binary256_t &x, const binary256_t &y)
      {
         return detail::combine_bigbinary_bits::eval(x, y, std::bit_xor<>()) ;
      }

      friend constexpr binary256_t operator~(const binary256_t &x)
      {
         return detail::combine_bigbinary_bits::invert(x) ;
      }

      /*********************************************************************//**
       Equality (==), ordering (<)
      *************************************************************************/
      friend constexpr bool operator==(const binary256_t &x, const binary256_t &y)
      {
         return !(((x._idata[0] ^ y._idata[0]) | (x._idata[1] ^ y._idata[1])) |
                  ((x._idata[2] ^ y._idata[2]) | (x._idata[3] ^ y._idata[3]))) ;
      }

      friend bool operator<(const binary256_t &x, const binary256_t &y)
      {
         const uint64_t xv[4] {be(x._idata[3]), be(x._idata[2]), be(x._idata[1]), be(x._idata[0])} ;
         const uint64_t yv[4] {be(y._idata[3]), be(y._idata[2]), be(y._idata[1]), be(y._idata[0])} ;
         return memcmp(&xv, &yv, sizeof xv) < 0 ;
      }

   protected:
      union {
            uint64_t       _idata[4] ;
            unsigned char  _cdata[32] ;
      } ;

   protected:
      template<typename T>
      static constexpr T be(T value) { return value_to_big_endian(value) ; }
} ;

// Define !=, >, <=, >= for binary256_t
PCOMN_DEFINE_RELOP_FUNCTIONS(, binary256_t) ;

/***************************************************************************//**
 Cast 128-bit literal
*******************************************************************************/
/**@{*/
template<typename T>
constexpr inline if_literal128_t<T,const T*> cast128(const b128_t *v)
{
   return reinterpret_cast<const T *>(v) ;
}

template<typename T>
constexpr inline if_literal128_t<T,T*> cast128(b128_t *v)
{
   return reinterpret_cast<T *>(v) ;
}

template<typename T>
constexpr inline if_literal128_t<T,const T*> cast128(const binary128_t *v)
{
   return reinterpret_cast<const T *>(v) ;
}

template<typename T>
constexpr inline if_literal128_t<T,T*> cast128(binary128_t *v)
{
   return reinterpret_cast<T *>(v) ;
}

template<typename T>
constexpr inline if_literal128_t<T,const T&> cast128(const b128_t &v)
{
   return *cast128<T>(&v) ;
}

template<typename T>
constexpr inline if_literal128_t<T,T&> cast128(b128_t &v)
{
   return *cast128<T>(&v) ;
}

template<typename T>
constexpr inline if_literal128_t<T,T&&> cast128(b128_t &&v)
{
   return std::move(*cast128<T>(&v)) ;
}

template<typename T>
constexpr inline if_literal128_t<T,const T&> cast128(const binary128_t &v)
{
   return *cast128<T>(&v) ;
}

template<typename T>
constexpr inline if_literal128_t<T,T&> cast128(binary128_t &v)
{
   return *cast128<T>(&v) ;
}

template<typename T>
constexpr inline if_literal128_t<T,T&&> cast128(binary128_t &&v)
{
   return std::move(*cast128<T>(&v)) ;
}

/**@}*/

} // end of namespace pcomn

namespace std {
/***************************************************************************//**
 std::hash specializations for large binary types (binary128_t, binary256_t)
*******************************************************************************/
/**@{*/
template<> struct hash<pcomn::b128_t> {
      size_t operator()(const pcomn::b128_t &v) const { return v.hash() ; }
} ;
template<> struct hash<pcomn::binary128_t> {
      size_t operator()(const pcomn::binary128_t &v) const { return v.hash() ; }
} ;
template<> struct hash<pcomn::binary256_t> {
      size_t operator()(const pcomn::binary256_t &v) const { return v.hash() ; }
} ;
/**@}*/
}

#endif /* __PCOMN_BINARY128_H */
