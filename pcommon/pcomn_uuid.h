/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_UUID_H
#define __PCOMN_UUID_H
/*******************************************************************************
 FILE         :   pcomn_uuid.h
 COPYRIGHT    :   Yakov Markovitch, 2014-2017

 DESCRIPTION  :   UUID data type

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   15 Sep 2014
*******************************************************************************/
/** @flie
 UUID and MAC (Network Media Access Address) data types.
*******************************************************************************/
#include <pcomn_hash.h>
#include <pcomn_strslice.h>

namespace pcomn {

/***************************************************************************//**
 Standard UUID
*******************************************************************************/
struct uuid : binary128_t {

      enum : size_t {
         SZ_BIN = 16, /**< Binary representation length */
         SZ_STR = 36  /**< String representation length (RFC4122, Section 3)   */
      } ;

      /// Create a "null" UUID
      ///
      /// Null UUID has all its octets set to 0; operator bool() returns false
      /// for such UUID.
      ///
      constexpr uuid() = default ;
      constexpr uuid(uint16_t h1, uint16_t h2, uint16_t h3, uint16_t h4,
                     uint16_t h5, uint16_t h6, uint16_t h7, uint16_t h8) :
         binary128_t(h1, h2, h3, h4, h5, h6, h7, h8)
      {}

      constexpr uuid(uint64_t h1, uint64_t h2) : binary128_t(h1, h2) {}
      constexpr uuid(const binary128_t &bin) : binary128_t(bin) {}

      /// Create a UUID from "standard" string representation
      ///
      /// @param str Canonical form of UUID string representation, like
      /// e.g. "123e4567-e89b-12d3-a456-426655440000", or an empty string or nullptr,
      ///
      _PCOMNEXP
      uuid(const strslice &str, RaiseError raise_error) ;

      uuid(const strslice &str) : uuid (str, RAISE_ERROR) {}

      uuid(const char *str, RaiseError raise_error) :
         uuid(str ? strslice(str) : strslice(), raise_error)
      {}

      uuid(const char *str) : uuid(str, RAISE_ERROR) {}

      /// Get the pointer to 16-octets-sequence representing UUID in "most significant
      /// byte first" order, as stated in RFC 4122
      constexpr const unsigned char *data() const { return _cdata ; }
      /// @overload
      unsigned char *data() { return _cdata ; }

      /// Get the nth octet of the UUID (RFC states "MSB-first order")
      constexpr unsigned octet(size_t n) const { return _cdata[n] ; }

      constexpr unsigned version() const { return (octet(12) & 0xf0U) >> 4 ; }

      /// Get the length of canonical string representation of UUID (36 chars)
      static constexpr size_t slen() { return 2*size() + 4 ; }

      /// Convert UUID to a zero-terminated C string and place into a character buffer
      ///
      /// @param buf Pointer to character buffer of size at least uuid::slen() + 1
      /// (buffer must provide enough space for the terminating zero)
      ///
      /// @return @a buf
      ///
      _PCOMNEXP char *to_strbuf(char *buf) const ;

      std::string to_string() const
      {
         char buf[slen() + 1] ;
         return std::string(to_strbuf(buf)) ;
      }
} ;

PCOMN_STATIC_CHECK(sizeof(uuid) == 16) ;

/***************************************************************************//**
 Network Media Access Address (MAC)
*******************************************************************************/
struct MAC {
      constexpr MAC() : _idata{0} {}

      /// Explicit conversion from 64-bit integer.
      constexpr MAC(uint8_t o1, uint8_t o2, uint8_t o3, uint8_t o4, uint8_t o5, uint8_t o6) :
         _data{o6, o5, o4, o3, o2, o1, 0, 0} {}

      MAC(const char *str, RaiseError raise_error = RAISE_ERROR) :
         MAC(str ? strslice(str) : strslice(), raise_error)
      {}

      _PCOMNEXP
      MAC(const strslice &str, RaiseError raise_error = RAISE_ERROR) ;

      template<typename I, typename=std::enable_if_t<(sizeof(I) == 8 && std::is_unsigned<I>::value)>>
      explicit constexpr MAC(I data) : _idata{value_to_little_endian(data & 0x00FFFFFFFFFFFFFFULL)} {}

      explicit constexpr operator bool() const { return !!_idata ; }
      explicit constexpr operator uint64_t() const { return value_from_little_endian(_idata) ; }

      /// Get the count of MAC octets (6)
      static constexpr size_t size() { return 6 ; }
      /// Get the canonical string representation length of a MAC (17 chars)
      static constexpr size_t slen() { return 3*size() - 1 ; }

      /// Get the nth octet of the MAC (MSB-first order)
      constexpr unsigned octet(size_t n) const
      {
         return (_idata >> (n * 8)) & 0xFFU  ;
      }

      /// Convert MAC to string of the form "XX:XX:XX:XX:XX:XX"
      _PCOMNEXP std::string to_string() const ;

      /// Put MAC to a character buffer as a zero-terminated string of the form "XX:XX:XX:XX:XX:XX"
      ///
      /// @param buf Pointer to character buffer of size at least MAC::slen() + 1
      /// (18 bytes, including terminating zero)
      ///
      /// @return @a buf
      ///
      _PCOMNEXP char *to_strbuf(char *buf) const ;

      friend bool operator==(const MAC &x, const MAC &y) { return x._idata == y._idata ; }
      friend bool operator<(const MAC &x, const MAC &y)
      {
         return value_from_little_endian(x._idata) < value_from_little_endian(y._idata) ;
      }

      size_t hash() const { return valhash(_idata) ; }

   private:
      union {
            unsigned char  _data[8] ;
            uint64_t       _idata ;
      } ;
} ;

PCOMN_DEFINE_RELOP_FUNCTIONS(, MAC) ;

/*******************************************************************************
 Debug output
*******************************************************************************/
inline std::ostream &operator<<(std::ostream &os, const uuid &v)
{
   char buf[uuid::slen() + 1] ;
   return os << v.to_strbuf(buf) ;
}

inline std::ostream &operator<<(std::ostream &os, const MAC &v)
{
   char buf[MAC::slen() + 1] ;
   return os << v.to_strbuf(buf) ;
}

} // end of namespace pcomn

namespace std {
/***************************************************************************//**
 std::hash specialization for pcomn::uuid
*******************************************************************************/
template<> struct hash<pcomn::uuid> : pcomn::hash_fn_member<pcomn::uuid> {} ;
/***************************************************************************//**
 std::hash specialization for pcomn::MAC
*******************************************************************************/
template<> struct hash<pcomn::MAC> : pcomn::hash_fn_member<pcomn::MAC> {} ;
}

#endif /* __PCOMN_UUID_H */
