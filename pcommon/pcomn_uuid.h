/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_UUID_H
#define __PCOMN_UUID_H
/*******************************************************************************
 FILE         :   pcomn_uuid.h
 COPYRIGHT    :   Yakov Markovitch, 2014

 DESCRIPTION  :   UUID data type

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   15 Sep 2014
*******************************************************************************/
#include <pcomn_hash.h>
#include <pcomn_strslice.h>

namespace pcomn {

/******************************************************************************/
/** Standard UUID
*******************************************************************************/
struct uuid {
      /// Create a "null" UUID
      ///
      /// Null UUID has all its octets set to 0; operator bool() returns false
      /// for such UUID.
      ///
      constexpr uuid() : _idata{0, 0} {}

      constexpr uuid(uint16_t h1, uint16_t h2, uint16_t h3, uint16_t h4,
                     uint16_t h5, uint16_t h6, uint16_t h7, uint16_t h8) :
      _hdata{be(h1), be(h2), be(h3), be(h4), be(h5), be(h6), be(h7), be(h8)}
      {}

      /// Create a UUID from "standard" string representation
      ///
      /// @param str Canonical form of UUID string representation, like
      /// e.g. "123e4567-e89b-12d3-a456-426655440000", or an empty string or nullptr,
      ///
      uuid(const char *str, RaiseError raise_error = RAISE_ERROR) :
         uuid(str ? strslice(str) : strslice(), raise_error)
      {}

      _PCOMNEXP
      uuid(const strslice &str, RaiseError raise_error = RAISE_ERROR) ;

      explicit constexpr operator bool() const { return !!(_idata[0] | _idata[1]) ; }

      /// Get the pointer to 16-octets-sequence representing UUID in "most significant
      /// byte first" order, as stated in RFC 4122
      constexpr const unsigned char *data() const { return _cdata ; }
      /// @overload
      unsigned char *data() { return _cdata ; }

      /// Get the nth octet of the UUID (RFC states "MSB-first order")
      constexpr const unsigned octet(size_t n) const { return _cdata[n] ; }

      constexpr unsigned version() const { return (octet(12) & 0xf0U) >> 4 ; }

      /// Get the count of UUID octets (16)
      static constexpr size_t size() { return sizeof _cdata ; }
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

      friend bool operator==(const uuid &x, const uuid &y)
      {
         return !((x._idata[0] ^ y._idata[0]) | (x._idata[1] ^ y._idata[1])) ;
      }

      friend bool operator<(const uuid &x, const uuid &y)
      {
         return
            (value_from_big_endian(x._idata[0]) < value_from_big_endian(y._idata[0])) ||
            (x._idata[0] == y._idata[0] && value_from_big_endian(x._idata[1]) < value_from_big_endian(y._idata[1])) ;
      }

      void swap(uuid &rhs)
      {
         std::swap(_idata[0], rhs._idata[0]) ;
         std::swap(_idata[1], rhs._idata[1]) ;
      }

      size_t hash() const { return pcomn::hasher(std::make_pair(_idata[0], _idata[1])) ; }

   private:
      union {
            uint64_t       _idata[2] ;
            uint16_t       _hdata[8] ;
            unsigned char  _cdata[16] ;
      } ;

      static constexpr uint16_t be(uint16_t value)
      {
         #ifdef PCOMN_CPU_BIG_ENDIAN
         return value ;
         #else
         return (value >> 8) | (value << 8) ;
         #endif
      }
} ;

inline void swap(uuid &lhs, uuid &rhs) { lhs.swap(rhs) ; }

PCOMN_DEFINE_RELOP_FUNCTIONS(, uuid) ;

/******************************************************************************/
/** Network Media Access Address (MAC)
*******************************************************************************/
struct MAC {
      constexpr MAC() : _idata{0} {}

      constexpr MAC(uint8_t o1, uint8_t o2, uint8_t o3, uint8_t o4, uint8_t o5, uint8_t o6) :
      _data{o1, o2, o3, o4, o5, o6} {}

      MAC(const char *str, RaiseError raise_error = RAISE_ERROR) :
         MAC(str ? strslice(str) : strslice(), raise_error)
      {}

      _PCOMNEXP
      MAC(const strslice &str, RaiseError raise_error = RAISE_ERROR) ;

      explicit constexpr operator bool() const { return !!_idata ; }

      /// Get the count of MAC octets (6)
      static constexpr size_t size() { return sizeof _data ; }
      /// Get the canonical string representation length of a MAC (17 chars)
      static constexpr size_t slen() { return 3*size() - 1 ; }

      /// Get direct access to MAC octets
      constexpr const unsigned char *data() const { return _data ; }
      /// Get the nth octet of the MAC (MSB-first order)
      constexpr const unsigned octet(size_t n) const { return _data[n] ; }

      /// Convert MAC to string of the form "XX:XX:XX:XX:XX:XX"
      std::string to_string() const
      {
         char buf[slen() + 1] ;
         return std::string(to_strbuf(buf)) ;
      }
      /// Put MAC to a character buffer as a zero-terminated string of the form "XX:XX:XX:XX:XX:XX"
      ///
      /// @param buf Pointer to character buffer of size at least MAC::slen() + 1
      /// (18 bytes, including terminating zero)
      ///
      /// @return @a buf
      ///
      _PCOMNEXP char *to_strbuf(char *buf) const ;

      friend constexpr bool operator==(const MAC &x, const MAC &y) { return x._idata == y._idata ; }
      friend constexpr bool operator<(const MAC &x, const MAC &y)
      {
         return value_from_big_endian(x._idata) < value_from_big_endian(y._idata) ;
      }

      size_t hash() const { return hasher(_idata) ; }

   private:
      union {
            unsigned char  _data[6] ;
            uint64_t       _idata ;
      } ;
} ;

PCOMN_DEFINE_RELOP_FUNCTIONS(, MAC) ;

template<> struct hash_fn<uuid> : hash_fn_member<uuid> {} ;
template<> struct hash_fn<MAC> : hash_fn_member<MAC> {} ;

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
/******************************************************************************/
/** std::hash specialization for pcomn::uuid
*******************************************************************************/
template<> struct hash<pcomn::uuid> : pcomn::hash_fn<pcomn::uuid> {} ;
/******************************************************************************/
/** std::hash specialization for pcomn::MAC
*******************************************************************************/
template<> struct hash<pcomn::MAC> : pcomn::hash_fn<pcomn::MAC> {} ;
}

#endif /* __PCOMN_UUID_H */
