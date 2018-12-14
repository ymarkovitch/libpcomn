/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_HASH_H
#define __PCOMN_HASH_H
/*******************************************************************************
 FILE         :   pcomn_hash.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2018. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Hash functions & functors.
                  MD5 hash.
                  CRC32 function(s)

 CREATION DATE:   10 Jan 2000
*******************************************************************************/
/** @file
 Hash functions and functors, MD5 hash, CRC32 calculations.
*******************************************************************************/
#include <pcommon.h>
#include <pcomn_assert.h>

#include "t1ha/t1ha.h"

#include <wchar.h>

#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

_PCOMNEXP uint32_t calc_crc32(uint32_t crc, const void *buf, size_t len) ;

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include <pcomn_meta.h>

#include <string>
#include <iosfwd>
#include <functional>
#include <utility>
#include <tuple>
#include <initializer_list>

namespace pcomn {

/*******************************************************************************
 Overloaded generic hash algorithms
*******************************************************************************/
inline uint32_t calc_crc32(uint32_t prev_crc, const char *str)
{
   return ::calc_crc32(prev_crc, (const uint8_t *)str, strlen(str)) ;
}

inline uint32_t calc_crc32(const char *str)
{
   return calc_crc32(0, str) ;
}

inline uint32_t calc_crc32(uint32_t prev_crc, const void *buf, size_t sz)
{
   return ::calc_crc32(prev_crc, buf, sz) ;
}

/*******************************************************************************/
/** Fowler/Noll/Vo (FNV) hash.
*******************************************************************************/
inline uint32_t hashFNV32(const void *data, size_t size, uint32_t init)
{
	uint32_t result = init ;
	for (const uint8_t *p = static_cast<const uint8_t *>(data), *end = p + size ; p != end ; ++p)
   {
      result ^= uint32_t(*p) ;
      result *= uint32_t(16777619UL) ;
   }
	return result ;
}

inline uint32_t hashFNV32(const void *data, size_t size)
{
   return hashFNV32(data, size, 2166136261UL) ;
}

inline uint64_t hashFNV64(const void *data, size_t size, uint64_t init)
{
	uint64_t result = init ;
	for (const uint8_t *p = static_cast<const uint8_t *>(data), *end = p + size ; p != end ; ++p)
   {
      result ^= uint64_t(*p) ;
      result *= uint64_t(1099511628211ULL) ;
   }
	return result ;
}

inline uint64_t hashFNV64(const void *data, size_t size)
{
   return hashFNV64(data, size, 14695981039346656037ULL) ;
}

inline uint32_t hashFNV(const void *data, size_t size, std::integral_constant<size_t, 4>)
{
   return hashFNV32(data, size) ;
}

inline uint32_t hashFNV(const void *data, size_t size, uint32_t init, std::integral_constant<size_t, 4>)
{
   return hashFNV32(data, size, init) ;
}

inline uint64_t hashFNV(const void *data, size_t size, std::integral_constant<size_t, 8>)
{
   return hashFNV64(data, size) ;
}

inline uint64_t hashFNV(const void *data, size_t size, uint64_t init, std::integral_constant<size_t, 8>)
{
   return hashFNV64(data, size, init) ;
}

inline size_t hashFNV(const void *data, size_t size)
{
   return hashFNV(data, size, std::integral_constant<size_t, sizeof(size_t)>()) ;
}

inline size_t hashFNV(const void *data, size_t size, size_t init)
{
   return hashFNV(data, size, init, std::integral_constant<size_t, sizeof(size_t)>()) ;
}

/******************************************************************************/
/** Robert Jenkins' hash function for hashing a 32-bit integer to a 32-bit integer
*******************************************************************************/
inline uint32_t jenkins_hash32to32(uint32_t x)
{
   x = (x+0x7ed55d16) + (x<<12) ;
   x = (x^0xc761c23c) ^ (x>>19) ;
   x = (x+0x165667b1) + (x<<5) ;
   x = (x+0xd3a2646c) ^ (x<<9) ;
   x = (x+0xfd7046c5) + (x<<3) ;
   x = (x^0xb55a4f09) ^ (x>>16) ;
   return x ;
}

/******************************************************************************/
/** Thomas Wang's hash function for hashing a 64-bit integer to a 64-bit integer
*******************************************************************************/
inline uint64_t wang_hash64to64(uint64_t x)
{
  x = ~x + (x << 21) ; // x = (x << 21) - x - 1
  x = x ^ (x >> 24) ;
  x = x + (x << 3) + (x << 8) ; // x * 265
  x = x ^ (x >> 14) ;
  x = x + (x << 2) + (x << 4) ; // x * 21
  x = x ^ (x >> 28) ;
  x = x + (x << 31) ;
  return x ;
}

/******************************************************************************/
/** Thomas Wang's hash function for hashing a 64-bit integer to a 32-bit integer
*******************************************************************************/
inline uint32_t wang_hash64to32(uint64_t x)
{
  x = ~x + (x << 18) ; // x = (x << 18) - x - 1
  x = x ^ (x >> 31) ;
  x = x + (x << 2) + (x << 4) ; // x * 21
  x = x ^ (x >> 11) ;
  x = x + (x << 6) ;
  x = x ^ (x >> 22) ;
  return (uint32_t)x ;
}

inline uint32_t hash_64(uint64_t x, std::integral_constant<size_t, 4>)
{
   return wang_hash64to32(x) ;
}

inline uint64_t hash_64(uint64_t x, std::integral_constant<size_t, 8>)
{
   return wang_hash64to64(x) ;
}

inline uint32_t hash_32(uint32_t x, std::integral_constant<size_t, 4>)
{
   return jenkins_hash32to32(x) ;
}

inline uint64_t hash_32(uint32_t x, std::integral_constant<size_t, 8>)
{
   return wang_hash64to64(x) ;
}

/***************************************************************************//**
 Hashing 32-bit integer to size_t
*******************************************************************************/
inline size_t hash_32(uint32_t x)
{
   return hash_32(x, std::integral_constant<size_t, sizeof(size_t)>()) ;
}

/***************************************************************************//**
 Hashing 64-bit integer to size_t
*******************************************************************************/
inline size_t hash_64(uint64_t x)
{
   return hash_64(x, std::integral_constant<size_t, sizeof(size_t)>()) ;
}

/***************************************************************************//**
 Hashing byte sequence to size_t.
 @note Always returns 0 for zero length.
*******************************************************************************/
inline size_t hash_bytes(const void *data, size_t length)
{
   return length ? t1ha0(data, length, 0) : 0 ;
}

/*******************************************************************************
 Hasher functors
*******************************************************************************/
/** Functor returning its parameter unchanged (but converted to size_t).
*******************************************************************************/
template<typename T>
struct hash_identity : public std::unary_function<T, size_t> {
      size_t operator() (const T &val) const { return static_cast<size_t>(val) ; }
} ;

/*******************************************************************************
 Forward declarations of hash functor templates
*******************************************************************************/
template<typename = void> struct hash_fn ;

// "Unwrap" type being hashed from const, volatile, and reference wrappers.
template<typename T> struct hash_fn<const T> : hash_fn<T> {} ;
template<typename T> struct hash_fn<volatile T> : hash_fn<T> {} ;
template<typename T> struct hash_fn<T &> : hash_fn<T> {} ;
template<typename T> struct hash_fn<T &&> : hash_fn<T> {} ;
template<typename T>
struct hash_fn<std::reference_wrapper<T>> : hash_fn<T> {} ;

/***************************************************************************//**
 Universal hashing functor.
*******************************************************************************/
template<> struct hash_fn<void> {
      template<typename T>
      auto operator()(T &&v) const -> decltype(std::declval<hash_fn<T>>()(std::forward<T>(v)))
      {
         hash_fn<T> h ;
         return h(std::forward<T>(v)) ;
      }
} ;

/***************************************************************************//**
 Universal hashing function.
*******************************************************************************/
template<typename T>
inline decltype(hash_fn<std::decay_t<T>>()(std::declval<T>())) valhash(T &&data)
{
   return hash_fn<std::decay_t<T>>()(std::forward<T>(data)) ;
}

/***************************************************************************//**
 Hash combinator: "reduces" a sequence of hash value to single hash value.

 Useful for hashing tuples, structures and sequences, i.e. complex objects.
 Uses a part of 64-bit MurmurHash2 to combine hash values.
*******************************************************************************/
struct hash_combinator {
      constexpr hash_combinator() = default ;

      explicit hash_combinator(uint64_t initial_hash) { append(initial_hash) ; }

      template<typename InputIterator>
      hash_combinator(const InputIterator &b, const InputIterator &e) : hash_combinator()
      {
         append_data(b, e) ;
      }

      constexpr uint64_t value() const { return _combined ; }
      constexpr operator uint64_t() const { return value() ; }

      hash_combinator &append(uint64_t hash_value)
      {
         constexpr const uint64_t mask = 0xc6a4a7935bd1e995ULL ;
         constexpr const int r = 47 ;

         hash_value *= mask ;
         hash_value ^= hash_value >> r ;
         hash_value *= mask ;

         _combined ^= hash_value ;
         _combined *= mask ;
         return *this ;
      }

      template<typename T>
      hash_combinator &append_data(T &&data) { return append(valhash(std::forward<T>(data))) ; }

      template<typename InputIterator>
      hash_combinator &append_data(InputIterator b, InputIterator e)
      {
         for (; b != e ; *this += *b, ++b) ;
         return *this ;
      }

      template<typename T>
      hash_combinator &operator+=(T &&data) { return append_data(std::forward<T>(data)) ; }

      template<typename...T>
      hash_combinator &append_as_tuple(const std::tuple<T...> &data)
      {
         append_item<sizeof...(T)>::append(*this, data) ;
         return *this ;
      }

   private:
      uint64_t _combined = 0 ;

      template<unsigned tuplesz, typename=void>
      struct append_item {
            template<typename T>
            static void append(hash_combinator &accumulator, T &&tuple_data)
            {
               accumulator.append_data(std::get<std::tuple_size<valtype_t<T>>::value - tuplesz>(std::forward<T>(tuple_data))) ;
               append_item<tuplesz-1>::append(accumulator, std::forward<T>(tuple_data)) ;
            }
      } ;
      template<typename D> struct append_item<0, D> {
            template<typename...Args> static void append(Args &&...) {}
      } ;
} ;

/***************************************************************************//**
 Backward compatibility definition.
*******************************************************************************/
typedef hash_combinator Hash ;

/***************************************************************************//**
 Function that hashes its arguments and returns combined hash value.
*******************************************************************************/
inline size_t tuplehash() { return 0 ; }

template<typename A1>
inline size_t tuplehash(A1 &&a1)
{
   return hash_combinator(valhash(std::forward<A1>(a1))) ;
}

template<typename A1, typename A2>
inline size_t tuplehash(A1 &&a1, A2 &&a2)
{
   return hash_combinator(valhash(std::forward<A1>(a1))).append_data(std::forward<A2>(a2)) ;
}

template<typename A1, typename A2, typename A3, typename... Args>
inline size_t tuplehash(A1 &&a1, A2 &&a2, A3 &&a3, Args &&...args)
{
   hash_combinator result (valhash(std::forward<A1>(a1))) ;
   result.append_data(std::forward<A2>(a2)).append_data(std::forward<A3>(a3)) ;

   const size_t h[] = { valhash(std::forward<Args>(args))...} ;
   for (size_t v: h) result.append(v) ;
   return result ;
}

/***************************************************************************//**
 128-bit binary big-endian POD data, aligned to 64-bit boundary.
*******************************************************************************/
struct binary128_t {

      constexpr binary128_t() : _idata() {}
      constexpr binary128_t(uint64_t hi, uint64_t lo) : _idata{be(hi), be(lo)} {}

      constexpr binary128_t(uint16_t h1, uint16_t h2, uint16_t h3, uint16_t h4,
                            uint16_t h5, uint16_t h6, uint16_t h7, uint16_t h8) :
         _hdata{be(h1), be(h2), be(h3), be(h4), be(h5), be(h6), be(h7), be(h8)}
      {}
      constexpr binary128_t(uint8_t c0, uint8_t c1, uint8_t c2, uint8_t c3,
                            uint8_t c4, uint8_t c5, uint8_t c6, uint8_t c7,
                            uint8_t c8, uint8_t c9, uint8_t ca, uint8_t cb,
                            uint8_t cc, uint8_t cd, uint8_t ce, uint8_t cf) :
         _cdata{c0, c1, c2, c3, c4, c5, c6, c7, c8, c9, ca, cb, cc, cd, ce, cf}
      {}

      /// Create value from a hex string representation.
      /// @note @a hexstr need not be null-terminated, the constructor will scan at most
      /// 32 characters, or until '\0' encountered, whatever comes first.
      explicit binary128_t(const char *hexstr)
      {
         if (!hextob(_idata, sizeof _idata, hexstr))
            _idata[1] = _idata[0] = 0 ;
      }

      /// Check helper
      explicit constexpr operator bool() const { return !!(_idata[0] | _idata[1]) ; }

      unsigned char *data() { return _cdata ; }
      constexpr const unsigned char *data() const { return _cdata ; }

      /// Get the count of octets (16)
      static constexpr size_t size() { return sizeof _idata ; }
      /// Get the length of string representation (32 chars)
      static constexpr size_t slen() { return 2*size() ; }

      constexpr uint64_t hi() const { return value_from_big_endian(_idata[0]) ; }
      constexpr uint64_t lo() const { return value_from_big_endian(_idata[1]) ; }

      _PCOMNEXP std::string to_string() const ;
      _PCOMNEXP char *to_strbuf(char *buf) const ;

      friend constexpr bool operator==(const binary128_t &l, const binary128_t &r)
      {
         return !((l._idata[0] ^ r._idata[0]) | (l._idata[1] ^ r._idata[1])) ;
      }

      friend constexpr bool operator<(const binary128_t &l, const binary128_t &r)
      {
         return l.hi() < r.hi() || l._idata[0] == r._idata[0] && l.lo() < r.lo() ;
      }

      size_t hash() const { return tuplehash(_idata[0], _idata[1]) ; }

   protected:
      union {
            uint64_t       _idata[2] ;
            uint32_t       _wdata[4] ;
            uint16_t       _hdata[8] ;
            unsigned char  _cdata[16] ;
      } ;

   protected:
      template<typename T>
      static constexpr T be(T value) { return value_to_big_endian(value) ; }
} ;

PCOMN_STATIC_CHECK(sizeof(binary128_t) == 16) ;
PCOMN_STATIC_CHECK(alignof(binary128_t) == 8) ;

// Define !=, >, <=, >= for binary128_t
PCOMN_DEFINE_RELOP_FUNCTIONS(, binary128_t) ;

template<typename T>
struct is_literal128 :
   std::bool_constant<sizeof(T) == sizeof(binary128_t) &&
                      alignof(T) == alignof(binary128_t) &&
                      std::is_trivially_copyable<T>::value>
{} ;

template<typename T>
inline std::enable_if_t<is_literal128<T>::value, T *> cast128(binary128_t *v)
{
   return reinterpret_cast<T *>(v) ;
}

template<typename T>
inline std::enable_if_t<is_literal128<T>::value, const T *> cast128(const binary128_t *v)
{
   return reinterpret_cast<const T *>(v) ;
}

template<typename T>
inline std::enable_if_t<is_literal128<T>::value, T &> cast128(binary128_t &v)
{
   return *cast128<T>(&v) ;
}

template<typename T>
inline std::enable_if_t<is_literal128<T>::value, const T &> cast128(const binary128_t &v)
{
   return *cast128<T>(&v) ;
}

template<typename T>
inline std::enable_if_t<is_literal128<T>::value, T &&> cast128(binary128_t &&v)
{
   return std::move(*reinterpret_cast<T *>(&v)) ;
}

/***************************************************************************//**
 MD5 hash
*******************************************************************************/
struct md5hash_t : binary128_t {

      constexpr md5hash_t() = default ;
      explicit constexpr md5hash_t(const binary128_t &src) : binary128_t(src) {}
      explicit md5hash_t(const char *hexstr) : binary128_t(hexstr) {}

      constexpr size_t hash() const { return _idata[0] ; }
} ;

/***************************************************************************//**
 SHA1 hash
*******************************************************************************/
struct sha1hash_t {

      constexpr sha1hash_t() : _idata() {}

      /// Create a hash from a hex string representation.
      /// @note @a hexstr need not be null-terminated, the constructor will scan at most
      /// 40 characters, or until '\0' encountered, whatever comes first.
      explicit sha1hash_t(const char *hexstr)
      {
         if (!hextob(_idata, sizeof _idata, hexstr))
            _idata[4] = _idata[3] = _idata[2] = _idata[1] = _idata[0] = 0 ;
      }

      /// Check helper.
      explicit constexpr operator bool() const
      {
         return _idata[0] || _idata[1] || _idata[2] || _idata[3] || _idata[4] ;
      }

      unsigned char *data() { return reinterpret_cast<unsigned char *>(_idata) ; }
      const unsigned char *data() const { return reinterpret_cast<const unsigned char *>(_idata) ; }

      static constexpr size_t size() { return sizeof _idata ; }

      _PCOMNEXP std::string to_string() const ;

      friend bool operator==(const sha1hash_t &l, const sha1hash_t &r)
      {
         return !memcmp(l._idata, r._idata, sizeof l._idata) ;
      }

      friend bool operator<(const sha1hash_t &l, const sha1hash_t &r)
      {
         return memcmp(l._idata, r._idata, sizeof l._idata) < 0 ;
      }

      constexpr size_t hash() const { return ((uint64_t)_idata[3] << 32) | (uint64_t)_idata[4] ; }

   private:
      uint32_t _idata[5] ;
} ;

PCOMN_STATIC_CHECK(sizeof(sha1hash_t) == 20) ;

// Define !=, >, <=, >= for sha1hash_t
PCOMN_DEFINE_RELOP_FUNCTIONS(, sha1hash_t) ;

/***************************************************************************//**
 t1ha2 128-bit hash
*******************************************************************************/
struct t1ha2hash_t : binary128_t {

      constexpr t1ha2hash_t() = default ;
      explicit constexpr t1ha2hash_t(const binary128_t &src) : binary128_t(src) {}
      explicit t1ha2hash_t(const char *hexstr) : binary128_t(hexstr) {}

      constexpr size_t hash() const { return _idata[0] ; }
} ;

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
         {
            const uint64_t q0 = be(_idata[3]) ;
            const uint64_t q1 = be(_idata[2]) ;
            const uint64_t q2 = be(_idata[1]) ;
            const uint64_t q3 = be(_idata[0]) ;
            _idata[0] = q0 ;
            _idata[1] = q1 ;
            _idata[2] = q2 ;
            _idata[3] = q3 ;
         }
      }

      /// Check helper
      explicit constexpr operator bool() const { return !!((_idata[0] | _idata[1]) | (_idata[2] | _idata[3])) ; }

      unsigned char *data() { return _cdata ; }
      constexpr const unsigned char *data() const { return _cdata ; }

      uint64_t *idata() { return _idata ; }
      constexpr const uint64_t *idata() const { return _idata ; }

      /// Get the count of octets (16)
      static constexpr size_t size() { return sizeof _idata ; }
      /// Get the length of string representation (32 chars)
      static constexpr size_t slen() { return 2*size() ; }

      size_t hash() const { return tuplehash(_idata[0], _idata[1], _idata[3], _idata[4]) ; }

      char *to_strbuf(char *buf) const
      {
         PCOMN_STATIC_CHECK(slen() >= 2*binary128_t::slen()) ;
         binary128_t(*(idata() + 3), *(idata() + 2)).to_strbuf(buf) ;
         binary128_t(*(idata() + 1), *(idata() + 0)).to_strbuf(buf + binary128_t::slen()) ;

         return buf ;
      }

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
 Backward compatibility typedefs
*******************************************************************************/
/**{@*/
typedef md5hash_t  md5hash_pod_t ;
typedef sha1hash_t sha1hash_pod_t ;
/**}@*/

/*******************************************************************************

*******************************************************************************/
/// @cond
namespace detail {
template<size_t n>
struct crypthash_state {
      size_t   _size ;
      uint64_t _statebuf[n] ;

      bool is_init() const { return *_statebuf || _size ; }
} ;

}
/// @endcond

/******************************************************************************/
/** MD5 hash accumulator: calculates MD5 incrementally.
*******************************************************************************/
class _PCOMNEXP MD5Hash {
   public:
      MD5Hash() { memset(&_state, 0, sizeof _state) ; }

      md5hash_t value() const ;
      operator md5hash_t() const { return value() ; }

      /// Data size appended so far
      size_t size() const { return _state._size ; }

      MD5Hash &append_data(const void *buf, size_t size) ;
      MD5Hash &append_file(const char *filename) ;
      MD5Hash &append_file(FILE *file) ;

   private:
      typedef detail::crypthash_state<24> state ;
      state _state ;
} ;

/// Create zero MD5.
inline md5hash_t md5hash() { return md5hash_t() ; }

/// Compute MD5 for a buffer.
///
/// @note There are convenient overloading of this function for any object that provides
/// pcomn::buf::cdata() and pcomn::buf::size() and strslice.
///
_PCOMNEXP md5hash_t md5hash(const void *buf, size_t size) ;
/// Compute MD5 for a file specified by a filename.
/// @param filename     The name of file to md5.
/// @param raise_error  Whether to throw pcomn::system_error exception on I/O error
/// (e.g., @a filename does not exist), if DONT_RAISE_ERROR then return zero md5.
/// @throw std::invalid_argument if @a filename is NULL.
_PCOMNEXP md5hash_t md5hash_file(const char *filename, size_t *size, RaiseError raise_error = DONT_RAISE_ERROR) ;
/// @overload
_PCOMNEXP md5hash_t md5hash_file(int fd, size_t *size, RaiseError raise_error = DONT_RAISE_ERROR) ;
/// @overload
inline md5hash_t md5hash_file(const char *filename, RaiseError raise_error = DONT_RAISE_ERROR)
{
   return md5hash_file(filename, NULL, raise_error) ;
}
/// @overload
inline md5hash_t md5hash_file(int fd, RaiseError raise_error = DONT_RAISE_ERROR)
{
   return md5hash_file(fd, NULL, raise_error) ;
}

/// Compute MD5 for a file opened for reading.
/// @note Doesn't rewind file before starting calculation.
inline md5hash_t md5hash_file(FILE *file, size_t *size = NULL)
{
   MD5Hash crypthasher ;
   crypthasher.append_file(file) ;
   if (size)
      *size = crypthasher.size() ;
   return crypthasher.value() ;
}

/******************************************************************************/
/** SHA1 hash accumulator: calculates SHA1 incrementally.
 *******************************************************************************/
class _PCOMNEXP SHA1Hash {
   public:
      SHA1Hash() { memset(&_state, 0, sizeof _state) ; }

      sha1hash_t value() const ;
      operator sha1hash_t() const { return value() ; }

      /// Data size appended so far
      size_t size() const { return _state._size ; }

      SHA1Hash &append_data(const void *buf, size_t size) ;
      SHA1Hash &append_file(const char *filename) ;
      SHA1Hash &append_file(FILE *file) ;

   private:
      typedef detail::crypthash_state<24> state ;
      state _state ;
} ;

/// Create all-zero SHA1.
inline sha1hash_t sha1hash() { return sha1hash_t() ; }

/// Compute SHA1 for a buffer.
///
/// @note There are convenient overloading of this function for any object that provides
/// pcomn::buf::cdata() and pcomn::buf::size().
///
_PCOMNEXP sha1hash_t sha1hash(const void *buf, size_t size) ;
/// Compute SHA1 for a file specified by a filename.
/// @param filename     The name of file to md5.
/// @param raise_error  Whether to throw pcomn::system_error exception on I/O error
/// (e.g., @a filename does not exist), if DONT_RAISE_ERROR then return zero md5.
/// @throw std::invalid_argument if @a filename is NULL.
_PCOMNEXP sha1hash_t sha1hash_file(const char *filename, size_t *size, RaiseError raise_error = DONT_RAISE_ERROR) ;
/// @overload
_PCOMNEXP sha1hash_t sha1hash_file(int fd, size_t *size, RaiseError raise_error = DONT_RAISE_ERROR) ;
/// @overload
inline sha1hash_t sha1hash_file(const char *filename, RaiseError raise_error = DONT_RAISE_ERROR)
{
   return sha1hash_file(filename, NULL, raise_error) ;
}
/// @overload
inline sha1hash_t sha1hash_file(int fd, RaiseError raise_error = DONT_RAISE_ERROR)
{
   return sha1hash_file(fd, NULL, raise_error) ;
}

/// Compute SHA1 for a file opened for reading.
/// @note Doesn't rewind file before starting calculation.
inline sha1hash_t sha1hash_file(FILE *file, size_t *size = NULL)
{
   SHA1Hash crypthasher ;
   crypthasher.append_file(file) ;
   if (size)
      *size = crypthasher.size() ;
   return crypthasher.value() ;
}

/*******************************************************************************
 Leo Yuriev's t1ha2 128-bit hashing
*******************************************************************************/
/// Create zero t1ha2.
inline t1ha2hash_t t1ha2hash() { return {} ; }

/// Compute 128-bit t1ha2 hash for a buffer.
///
/// @note There are convenient overloading of this function for any object that provides
/// pcomn::buf::cdata() and pcomn::buf::size() and for strslice.
///
inline t1ha2hash_t t1ha2hash(const void *data, size_t size)
{
   uint64_t hi = 0 ;
   const uint64_t lo = t1ha2_atonce128(&hi, data, size, 0) ;
   return t1ha2hash_t(binary128_t(hi, lo)) ;
}

/***************************************************************************//**
 Hashing functor using object's `hash()` member function.
*******************************************************************************/
template<typename T>
struct hash_fn_member : public std::unary_function<T, size_t> {
      size_t operator()(const T &instance) const { return instance.hash() ; }
} ;

template<typename T>
struct hash_fn_fundamental {
      PCOMN_STATIC_CHECK(std::is_fundamental<T>::value) ;

      size_t operator()(T value) const { return hash_64((uint64_t)value) ; }
} ;

template<typename T>
struct hash_fn_rawdata {
      size_t operator()(const T &v) const
      {
         using namespace std ;
         return hash_bytes(data(v), (sizeof *data(v)) * size(v)) ;
      }
} ;

/***************************************************************************//**
 Generic hashing functor.
*******************************************************************************/
template<typename T>
struct hash_fn : std::conditional_t<std::is_default_constructible<std::hash<T>>::value,
                                    std::hash<T>,
                                    hash_fn_member<T>>
{} ;

template<> struct hash_fn<bool> { size_t operator()(bool v) const { return v ; } } ;

template<> struct hash_fn<double> {
      size_t operator()(double value) const
      {
         union { double _ ; uint64_t value ; } _ {value} ;
         return hash_64(_.value) ;
      }
} ;

template<> struct hash_fn<void *> {
      size_t operator()(const void *value) const
      {
         union { const void * _ ; uint64_t value ; } _ {value} ;
         return hash_64(_.value) ;
      }
} ;

template<typename T> struct hash_fn<const T *> : hash_fn<T *> {} ;
template<typename T> struct hash_fn<T *> : hash_fn<void *> {} ;

template<> struct hash_fn<float> : hash_fn<double> {} ;
template<> struct hash_fn<long double> : hash_fn<double> {} ;

#define PCOMN_HASH_INT_FN(type) template<> struct hash_fn<type> : hash_fn_fundamental<type> {}

PCOMN_HASH_INT_FN(char) ;
PCOMN_HASH_INT_FN(unsigned char) ;
PCOMN_HASH_INT_FN(signed char) ;
PCOMN_HASH_INT_FN(short) ;
PCOMN_HASH_INT_FN(unsigned short) ;
PCOMN_HASH_INT_FN(int) ;
PCOMN_HASH_INT_FN(unsigned) ;
PCOMN_HASH_INT_FN(long) ;
PCOMN_HASH_INT_FN(unsigned long) ;
PCOMN_HASH_INT_FN(long long) ;
PCOMN_HASH_INT_FN(unsigned long long) ;

/***************************************************************************//**
 hash_combinator C string, return 0 for NULL and empty string
*******************************************************************************/
template<> struct hash_fn<char *> {
      size_t operator()(const char *s) const
      {
         return hash_bytes(s, s ? strlen(s) : 0) ;
      }
} ;

template<> struct hash_fn<const char *> : hash_fn<char *> {} ;

template<> struct hash_fn<std::string> : hash_fn_rawdata<std::string> {} ;

template<typename T1, typename T2>
struct hash_fn<std::pair<T1, T2>> {
      size_t operator()(const std::pair<T1, T2> &data) const
      {
         return tuplehash(data.first, data.second) ;
      }
} ;

template<>
struct hash_fn<std::tuple<>> {
      size_t operator()(const std::tuple<> &) const { return 0 ; }
} ;

template<typename T1, typename...T>
struct hash_fn<std::tuple<T1, T...>> {
      size_t operator()(const std::tuple<T1, T...> &data) const
      {
         return hash_combinator().append_as_tuple(data) ;
      }
} ;

/***************************************************************************//**
 Hashing functor for any sequence
*******************************************************************************/
template<typename S, typename ItemHasher = hash_fn<decltype(*std::begin(std::declval<S>()))>>
struct hash_fn_sequence : private ItemHasher {
      size_t operator()(const S &sequence) const
      {
         hash_combinator accumulator ;
         for (const auto &v: sequence)
            accumulator.append(ItemHasher::operator()(v)) ;
         return accumulator ;
      }
} ;

template<typename InputIterator>
inline size_t hash_sequence(InputIterator b, InputIterator e)
{
   hash_combinator accumulator ;
   for (; b != e ; ++b)
      accumulator.append_data(*b) ;
   return accumulator ;
}

template<typename InputIterator, typename ItemHasher>
inline size_t hash_sequence(InputIterator b, InputIterator e, ItemHasher &&h)
{
   hash_combinator accumulator ;
   for (; b != e ; ++b)
      accumulator.append(std::forward<ItemHasher>(h)(*b)) ;
   return accumulator ;
}

template<typename Container>
inline size_t hash_sequence(Container &&c)
{
   return hash_fn_sequence<Container>()(std::forward<Container>(c)) ;
}

template<typename T>
inline size_t hash_sequence(std::initializer_list<T> s) { return hash_sequence(s.begin(), s.end()) ; }

/*******************************************************************************
 ostream
*******************************************************************************/
std::ostream &operator<<(std::ostream &, const binary128_t &) ;
std::ostream &operator<<(std::ostream &, const sha1hash_t &) ;
std::ostream &operator<<(std::ostream &, const binary256_t &) ;

} // end of namespace pcomn

namespace std {
/*******************************************************************************
 std::hash specializations for cryptohashes
*******************************************************************************/
template<> struct hash<pcomn::binary128_t>: pcomn::hash_fn_member<pcomn::binary128_t> {} ;
template<> struct hash<pcomn::md5hash_t>  : pcomn::hash_fn_member<pcomn::md5hash_t> {} ;
template<> struct hash<pcomn::sha1hash_t> : pcomn::hash_fn_member<pcomn::sha1hash_t> {} ;
template<> struct hash<pcomn::t1ha2hash_t>: pcomn::hash_fn_member<pcomn::t1ha2hash_t> {} ;
}

#endif /* __cplusplus */
#endif /* __PCOMN_HASH_H */

/*******************************************************************************
 Hashing buffers
 For this to work, include pcomn_buffer.h before pcomn_hash.h
*******************************************************************************/
#if defined(__cplusplus) && defined(__PCOMN_BUFFER_H) && !defined(__PCOMN_BUFFER_HASH_H)
#define __PCOMN_BUFFER_HASH_H

namespace pcomn {

template<typename B>
inline enable_if_buffer_t<B, uint32_t> calc_crc32(uint32_t crc, const B &buffer)
{
   return calc_crc32(crc, buf::cdata(buffer), buf::size(buffer)) ;
}

template<typename B>
inline enable_if_buffer_t<B, uint32_t> calc_crc32(uint32_t crc, size_t ignore_tail, const B &buffer)
{
   const size_t bufsize = buf::size(buffer) ;
   return
      ignore_tail >= bufsize ? crc : calc_crc32(crc, buf::cdata(buffer), bufsize - ignore_tail) ;
}

template<typename B>
inline enable_if_buffer_t<B, md5hash_t>  md5hash(const B &b) { return md5hash(buf::cdata(b), buf::size(b)) ; }
template<typename B>
inline enable_if_buffer_t<B, sha1hash_t> sha1hash(const B &b) { return sha1hash(buf::cdata(b), buf::size(b)) ; }
template<typename B>
inline enable_if_buffer_t<B, t1ha2hash_t> t1ha2hash(const B &b) { return t1ha2hash(buf::cdata(b), buf::size(b)) ; }

} // end of namespace pcomn
#endif /* __cplusplus && __PCOMN_BUFFER_H */
