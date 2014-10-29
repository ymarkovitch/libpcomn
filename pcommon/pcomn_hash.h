/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_HASH_H
#define __PCOMN_HASH_H
/*******************************************************************************
 FILE         :   pcomn_hash.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2014. All rights reserved.
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

#include <wchar.h>

#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

/* hashPJW
    *
    *  An adaptation of Peter Weinberger's (PJW) generic hashing
    *  algorithm based on Allen Holub's version.
    */
_PCOMNEXP unsigned int hashPJWstr(const char *datum) ;
_PCOMNEXP unsigned int hashPJWmem(const void *datum, size_t size) ;

/* hashELF
 *
 * The hash algorithm used in the UNIX ELF format for object files.
 */
_PCOMNEXP unsigned long hashELFstr(const char *name) ;
_PCOMNEXP unsigned long hashELFmem(const void *mem, size_t size) ;

_PCOMNEXP uint32_t calc_crc32(uint32_t crc, const void *buf, size_t len) ;

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include <string>
#include <iostream>
#include <type_traits>
#include <functional>
#include <utility>

namespace pcomn {

/*******************************************************************************
 Overloaded generic hash algorithms
*******************************************************************************/
inline unsigned int hashPJW(const char *str)
{
   return hashPJWstr(str) ;
}
inline unsigned int hashPJW(const wchar_t *str)
{
   return hashPJWmem(str, wcslen(str) * sizeof(wchar_t)) ;
}
inline unsigned int hashPJW(const void *datum, size_t size)
{
   return hashPJWmem(datum, size) ;
}
inline unsigned long hashELF(const char *str)
{
   return hashELFstr(str) ;
}
inline unsigned long hashELF(const wchar_t *str)
{
   return hashELFmem(str, wcslen(str) * sizeof(wchar_t)) ;
}
inline unsigned long hashELF(const void *mem, size_t size)
{
   return hashELFmem(mem, size) ;
}

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
  return x ;
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

/******************************************************************************/
/** Hashing 32-bit integer to size_t
*******************************************************************************/
inline size_t hash_32(uint32_t x)
{
   return hash_32(x, std::integral_constant<size_t, sizeof(size_t)>()) ;
}

/******************************************************************************/
/** Hashing 64-bit integer to size_t
*******************************************************************************/
inline size_t hash_64(uint64_t x)
{
   return hash_64(x, std::integral_constant<size_t, sizeof(size_t)>()) ;
}

/*******************************************************************************

*******************************************************************************/
namespace detail {
template<typename T>
inline size_t hash_fundamental(T value, std::false_type)
{
   return hash_32((uint32_t)value) ;
}
template<typename T>
inline size_t hash_fundamental(T value, std::true_type)
{
   return hash_64((uint64_t)value) ;
}
}

/*******************************************************************************
 Hasher functions for fundamental types.
 By default, template functor hash_fn<T> calls hasher(T)

 Note that according to dependent name lookup rules hasher() overloads for fundamental
 types must be declared _before_ hash_fn<> definition; for all other types, hasher()
 may be declared before hash_fn<> _instantiation_ (since ADL rules apply)
*******************************************************************************/
inline size_t hasher(const char *mem)
{
   return !mem ? 0 : hashFNV(mem, strlen(mem)) ;
}

#define PCOMN_HASH_IDENT_FN(type) \
   constexpr inline size_t hasher(type val) { return (size_t)val ; }

PCOMN_HASH_IDENT_FN(bool) ;
PCOMN_HASH_IDENT_FN(char) ;
PCOMN_HASH_IDENT_FN(unsigned char) ;
PCOMN_HASH_IDENT_FN(signed char) ;

#define PCOMN_HASH_INT_FN(type) \
   inline size_t hasher(type value) { return detail::hash_fundamental(value, std::integral_constant<bool, (sizeof value > 4)>())  ; }

PCOMN_HASH_INT_FN(short) ;
PCOMN_HASH_INT_FN(unsigned short) ;
PCOMN_HASH_INT_FN(int) ;
PCOMN_HASH_INT_FN(unsigned) ;
PCOMN_HASH_INT_FN(long) ;
PCOMN_HASH_INT_FN(unsigned long) ;
PCOMN_HASH_INT_FN(ulonglong_t) ;
PCOMN_HASH_INT_FN(longlong_t) ;
PCOMN_HASH_INT_FN(const void *) ;

inline size_t hasher(double value)
{
   return hash_64(*reinterpret_cast<const uint64_t *>(&value)) ;
}

inline size_t hasher(float value) { return hasher((double)value) ; }

inline size_t hasher(long double value) { return hasher((double)value) ; }

template<class T1, class T2>
size_t hasher(const std::pair<T1, T2> &) ;

/*******************************************************************************
 Hasher functors
*******************************************************************************/
/** Functor returning its parameter unchanged (but converted to size_t).
*******************************************************************************/
template<class T>
struct hash_identity : public std::unary_function<T, size_t> {
      size_t operator() (const T &val) const
      {
         return (size_t)val ;
      }
} ;

/*******************************************************************************
 Forward declarations of hash functor templates
*******************************************************************************/
template<class T> struct hash_fn ;
template<class T> struct hash_fn_raw ;
template<class T, template<typename> class> struct hash_fn_seq ;

/******************************************************************************/
/** Hash accumulator: "reduces" a sequence of hash value to single hash value.
 Useful for hashing structures and sequences (i.e. complex objects).
 The algorith is from Python's tuplehash.
*******************************************************************************/
class Hash {
   public:
      Hash() :
         _accumulator(0x345678L),
         _count(0)
      {}

      explicit Hash(uint64_t hash) :
         _accumulator((uint64_t)0x31E9D059168ULL ^ hash),
         _count(1)
      {}

      template<typename InputIterator>
      Hash(const InputIterator &b, const InputIterator &e) :
         _accumulator(0x345678L),
         _count(0)
      {
         append_data(b, e) ;
      }

      uint64_t value() const { return _accumulator ^ _count ; }
      operator uint64_t() const { return value() ; }
      Hash &append(uint64_t hash)
      {
         _accumulator = (1000003 * _accumulator) ^ hash ;
         ++_count ;
         return *this ;
      }

      template<class T>
      Hash &append_data(const T &data) { return append(hasher(data)) ; }

      template<typename InputIterator>
      Hash &append_data(InputIterator b, InputIterator e)
      {
         for (; b != e ; *this += *b, ++b) ;
         return *this ;
      }

      template<class T>
      Hash &operator +=(const T &data) { return append_data(data) ; }

   private:
      uint64_t _accumulator ;
      uint64_t _count ;
} ;

/******************************************************************************/
/** Cryptographic non-POD hash (has constructors)
*******************************************************************************/
template<typename T>
struct crypthash : T {
      /// Creates a hash filled with zeros
      crypthash() { T::init() ; }
      /// Creates a hash from a hex string representation
      explicit crypthash(const char *hashstr) { T::init(hashstr) ; }
} ;

/******************************************************************************/
/** 128-bit aligned binary big-endian POD data
*******************************************************************************/
struct binary128_t {
      /// Check helper
      explicit constexpr operator bool() const { return !!(_idata[0] | _idata[1]) ; }

      unsigned char *data() { return reinterpret_cast<unsigned char *>(&_cdata) ; }
      constexpr const unsigned char *data() const { return reinterpret_cast<const unsigned char *>(&_cdata) ; }

      static constexpr size_t size() { return sizeof _cdata ; }

      _PCOMNEXP std::string to_string() const ;
      _PCOMNEXP char *to_strbuf(char *buf) const ;

      friend std::ostream &operator<<(std::ostream &os, const binary128_t &v)
      {
         char buf[64] ;
         return os << v.to_strbuf(buf) ;
      }

      friend bool operator==(const binary128_t &l, const binary128_t &r)
      {
         return !((l._idata[0] ^ r._idata[0]) | (l._idata[1] ^ r._idata[1])) ;
      }

      friend bool operator<(const binary128_t &l, const binary128_t &r)
      {
         return
            value_from_big_endian(l._idata[0]) < value_from_big_endian(r._idata[0]) ||
            l._idata[0] == r._idata[0] && value_from_big_endian(l._idata[1]) < value_from_big_endian( r._idata[1]) ;
      }

      constexpr size_t hash() const { return _idata[0] ^ _idata[1] ; }

   protected:
      /// Create a hash filled with zeros.
      void init() { _idata[1] = _idata[0] = 0 ; }
      void init(const char *hexstr)
      {
         init() ;
         if (!hexstr) return ;
         for (const char *c = hexstr, *e = c + 2 * sizeof _cdata ; c != e ;)
            if (!isxdigit(*c++)) return ;

         sscanf(hexstr, "%16Lx%16Lx", _idata + 0, _idata + 1) ;
         to_big_endian(_idata[0]) ;
         to_big_endian(_idata[1]) ;
      }

   private:
      union {
            std::aligned_storage<16, 16>::type  _cdata ;
            unsigned long long                  _idata[2] ;
      } ;
} ;

// Define !=, >, <=, >= for binary128_t
PCOMN_DEFINE_RELOP_FUNCTIONS(, binary128_t) ;

/******************************************************************************/
/** MD5 hash POD type (no constructors, no destructors)
*******************************************************************************/
struct md5hash_pod_t : binary128_t {} ;

/******************************************************************************/
/** SHA1 hash POD type (no constructors, no destructors)
*******************************************************************************/
struct sha1hash_pod_t {

      /// Check helper.
      explicit constexpr operator bool() const
      {
         return _idata[0] || _idata[1] || _idata[2] || _idata[3] || _idata[4] ;
      }

      unsigned char *data() { return _cdata ; }
      const unsigned char *data() const { return _cdata ; }

      static size_t size() { return sizeof _cdata ; }

      _PCOMNEXP std::string to_string() const ;

      friend std::ostream &operator<<(std::ostream &os, const sha1hash_pod_t &v)
      {
         return os << v.to_string() ;
      }

      friend bool operator==(const sha1hash_pod_t &l, const sha1hash_pod_t &r)
      {
         return !memcmp(l._cdata, r._cdata, sizeof l._cdata) ;
      }

      friend bool operator<(const sha1hash_pod_t &l, const sha1hash_pod_t &r)
      {
         return memcmp(l._cdata, r._cdata, sizeof l._cdata) < 0 ;
      }

      size_t hash() const
      {
         return ((unsigned long long)_idata[3] << 32) | (unsigned long long)_idata[4] ;
      }

   protected:
      void init() { _idata[4] = _idata[3] = _idata[2] = _idata[1] = _idata[0] = 0 ; }
      void init(const char *hashstr)
      {
         init() ;
         if (!hashstr) return ;
         for (const char *c = hashstr, *e = c + 2 * sizeof _cdata ; c != e ;)
            if (!isxdigit(*c++)) return ;

         sscanf(hashstr, "%8x%8x%8x%8x%8x", _idata + 0, _idata + 1, _idata + 2, _idata + 3, _idata + 4) ;
         to_big_endian(_idata[0]) ;
         to_big_endian(_idata[1]) ;
         to_big_endian(_idata[2]) ;
         to_big_endian(_idata[3]) ;
         to_big_endian(_idata[4]) ;
      }

   private:
      union {
            unsigned char  _cdata[20] ;
            unsigned       _idata[5] ;
      } ;
} ;

// Define !=, >, <=, >= for sha1hash_pod_t
PCOMN_DEFINE_RELOP_FUNCTIONS(, sha1hash_pod_t) ;

/******************************************************************************/
/** MD5 hash (has constructors)
*******************************************************************************/
typedef crypthash<md5hash_pod_t> md5hash_t ;

/******************************************************************************/
/** SHA1 hash non-POD (has constructors)
*******************************************************************************/
typedef crypthash<sha1hash_pod_t> sha1hash_t ;

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
/// @note For convenient overloadings of this function see pcomn_strslice.h (md5 for any
/// string) and pcomn_buffer.h (md5 for any buffer)
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
/// @note For convenient overloadings of this function see pcomn_strslice.h (md5 for any
/// string) and pcomn_buffer.h (md5 for any buffer)
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

/******************************************************************************/
/** Generic hashing functor.

 Uses pcomn::hasher() function for hashing values.
*******************************************************************************/
template<class T>
struct hash_fn : public std::unary_function<T, size_t> {
      size_t operator() (const T &val) const { return hasher(val) ; }
} ;

/******************************************************************************/
/** Hashing functor, which uses pcomn::hash_fn<> for hashing non-integral values, and
 pcomn::hash_identity<> for hashing integral values.

 Such hashing requires the number of hashtable buckets to be a primary number.
*******************************************************************************/
template<class T>
struct hash_fn_raw : hash_fn<T> {} ;

/******************************************************************************/
/** Hashing functor for any sequence
*******************************************************************************/
template<class T, template<typename> class HashFunctor = hash_fn>
struct hash_fn_seq : HashFunctor<T> {
      template<class S>
      size_t operator() (const S &sequence) const
      {
         return (*this)(sequence.begin(), sequence.end()) ;
      }

      template<class InputIterator>
      size_t operator() (const InputIterator &b, const InputIterator &e) const
      {
         Hash accu ;
         for (InputIterator i (b) ; i != e ; ++i)
            accu.append(HashFunctor<T>::operator()(*i)) ;
         return accu ;
      }
} ;

#define PCOMN_HASH_RAW_FN(type) template<> struct hash_fn_raw<type > : hash_identity<type > {}

PCOMN_HASH_RAW_FN(short) ;
PCOMN_HASH_RAW_FN(unsigned short) ;
PCOMN_HASH_RAW_FN(int) ;
PCOMN_HASH_RAW_FN(unsigned) ;
PCOMN_HASH_RAW_FN(long) ;
PCOMN_HASH_RAW_FN(unsigned long) ;
PCOMN_HASH_RAW_FN(ulonglong_t) ;
PCOMN_HASH_RAW_FN(longlong_t) ;

#undef PCOMN_HASH_RAW_FN

template<typename T>
struct hash_fn_member : public std::unary_function<T, size_t> {
      size_t operator()(const T &instance) const { return instance.hash() ; }
} ;

/*******************************************************************************
 Hasher functions
*******************************************************************************/
template<class T1, class T2>
inline size_t hasher(const std::pair<T1, T2> &data)
{
   Hash hash(hash_fn<T1>()(*(T1 *)&data.first)) ;
   return hash += *(T2 *)&data.second ;
}

inline size_t hasher(const binary128_t &data) { return data.hash() ; }

inline size_t hasher(const md5hash_pod_t &data) { return data.hash() ; }

inline size_t hasher(const sha1hash_pod_t &data) { return data.hash() ; }

} // end of namespace pcomn

namespace std {
/*******************************************************************************
 std::hash specializations for cryptohashes
*******************************************************************************/
template<> struct hash<pcomn::binary128_t>    : pcomn::hash_fn_member<pcomn::binary128_t> {} ;
template<> struct hash<pcomn::md5hash_pod_t>  : pcomn::hash_fn_member<pcomn::md5hash_pod_t> {} ;
template<> struct hash<pcomn::sha1hash_pod_t> : pcomn::hash_fn_member<pcomn::sha1hash_pod_t> {} ;

template<typename T>
struct hash<pcomn::crypthash<T> > : public hash<T> {} ;
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
inline typename enable_if_buffer<B, uint32_t>::type
calc_crc32(uint32_t crc, const B &buffer)
{
   return calc_crc32(crc, buf::cptr(buffer), buf::size(buffer)) ;
}

template<typename B>
inline typename enable_if_buffer<B, uint32_t>::type
calc_crc32(uint32_t crc, size_t ignore_tail, const B &buffer)
{
   const size_t bufsize = buf::size(buffer) ;
   return
      ignore_tail >= bufsize ? crc : calc_crc32(crc, buf::cptr(buffer), bufsize - ignore_tail) ;
}

} // end of namespace pcomn
#endif // __cplusplus && __PCOMN_BUFFER_H
