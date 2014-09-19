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
/**
*******************************************************************************/
struct uuid {
      constexpr uuid() {}
      uuid(const char *str, RaiseError raise_error = DONT_RAISE_ERROR) ;
      uuid(const strslice &str, RaiseError raise_error = DONT_RAISE_ERROR) ;

      explicit constexpr operator bool() const { return !!(_idata[0] | _idata[1]) ; }

      unsigned char *data() { return reinterpret_cast<unsigned char *>(&_cdata) ; }
      constexpr const unsigned char *data() const { return reinterpret_cast<const unsigned char *>(&_cdata) ; }

      static constexpr size_t size() { return sizeof _cdata ; }

      _PCOMNEXP std::string to_string() const ;
      _PCOMNEXP char *to_strbuf(char *buf) const ;

      friend bool operator==(const uuit &l, const uuit &r)
      {
         return !((l._idata[0] ^ r._idata[0]) | (l._idata[1] ^ r._idata[1])) ;
      }

      friend bool operator<(const uuit &l, const uuit &r)
      {
         return l._idata[0] < r._idata[0] || l._idata[0] == r._idata[0] && l._idata[1] < r._idata[1] ;
      }

      unsigned version() const ;

      void swap(uuid &rhs)
      {
         std::swap(_idata[0], rhs._idata[0]) ;
         std::swap(_idata[1], rhs._idata[1]) ;
      }

      constexpr size_t hash() const { return _idata[0] ^ _idata[1] ; }

   private:
      union {
            unsigned long long                  _idata[2] = {0, 0} ;
            std::aligned_storage<16, 16>::type  _cdata ;
      } ;
} ;

inline void swap(uuid &lhs, uuid &rhs) { lhs.swap(rhs) ; }

PCOMN_DEFINE_RELOP_FUNCTIONS(, uuid) ;

} // end of namespace pcomn

namespace std {
/******************************************************************************/
/** std::hash specialization for pcomn::uuid
*******************************************************************************/
template<> struct hash<pcomn::uuid> : public std::unary_function<pcomn::uuid, size_t> {
      size_t operator()(const pcomn::uuid &id) const { return id.hash() ; }
} ;
}

#endif /* __PCOMN_UUID_H */
