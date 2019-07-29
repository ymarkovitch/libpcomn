/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_MAGICLBL_H
#define __PCOMN_MAGICLBL_H
/*******************************************************************************
 FILE         :   pcomn_magiclbl.h
 COPYRIGHT    :   Yakov Markovitch, 2018-2019. All rights reserved.

 DESCRIPTION  :   Magic label structures.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   26 Feb 2018
*******************************************************************************/
/** @file
    Magic label structures: 4- or 8-bytes long, fixed-size string, represented
    as uint32_t or uint64_t, respectively.
*******************************************************************************/
#include <pcomn_strslice.h>

namespace pcomn {

/***************************************************************************//**
 Magic label structures.

 4- or 8-bytes long, fixed-size string, represented as uint32_t or uint64_t,
 respectively.

 Both magiclbl<uint32_t> and magiclbl<uint64_t> are implicitly convertable to
 uint32_t and uint64_t, respectively; the results of such conversion is ordered
 the same as the corresponding string representation (ASCII, lexicographically),
 independently on endianness.
*******************************************************************************/
/**@{*/
template<typename I> struct magiclbl ;

typedef magiclbl<uint32_t> magic32 ;
typedef magiclbl<uint64_t> magic64 ;

template<> struct magiclbl<uint32_t> {

      constexpr magiclbl(char c1, char c2 = 0, char c3 = 0, char c4 = 0) : _c{c1, c2, c3, c4} {}
      constexpr magiclbl() = default ;

      constexpr operator uint32_t() const
      {
         return
            ((uint32_t)_c[3])       | ((uint32_t)_c[2] << 8) |
            ((uint32_t)_c[1] << 16) | ((uint32_t)_c[0] << 24) ;
      }

      explicit operator strslice() const
      {
         return _c[std::size(_c)-1] ? strslice(std::begin(_c), std::end(_c)) : strslice(_c) ;
      }

   private:
      char _c[4] {} ;
} ;

template<> struct magiclbl<uint64_t> {
      explicit constexpr magiclbl(char c1,     char c2 = 0, char c3 = 0, char c4 = 0,
                                  char c5 = 0, char c6 = 0, char c7 = 0, char c8 = 0) :
         _c{c1, c2, c3, c4, c5, c6, c7, c8}
      {}
      constexpr magiclbl() = default ;

      constexpr operator uint64_t() const
      {
         return
            ((uint64_t)_c[7])       | ((uint64_t)_c[6] << 8)  |
            ((uint64_t)_c[5] << 16) | ((uint64_t)_c[4] << 24) |
            ((uint64_t)_c[3] << 32) | ((uint64_t)_c[2] << 40) |
            ((uint64_t)_c[1] << 48) | ((uint64_t)_c[0] << 56) ;
      }

      explicit operator strslice() const
      {
         return unlikely(_c[std::size(_c)-1]) ? strslice(std::begin(_c), std::end(_c)) : strslice(_c) ;
      }

   private:
      char _c[8] {} ;
} ;
/**@}*/

namespace detail {
template<typename I, typename Char, char...c>
constexpr inline magiclbl<I> make_magiclbl()
{
   static_assert(std::is_same<Char, char>(), "Only single-byte characters allowed for _magic literals") ;
   static_assert(sizeof...(c) <= sizeof(I), "_magic literal is too long") ;
   static_assert(!pcomn::fold_bitor(false, (!c)...), "null character is not allowed in _magic literals") ;
   return magiclbl<I>(c...) ;
}
} // end of namespace pcomn::detail

inline namespace literals {

/***************************************************************************//**
 User-defined literal for magic label structures.
*******************************************************************************/
/**@{*/
template<typename C, C...c>
constexpr inline magic32 operator"" _magic32() { return detail::make_magiclbl<uint32_t, C, c...>() ; }

template<typename C, C...c>
constexpr inline magic64 operator"" _magic64() { return detail::make_magiclbl<uint64_t, C, c...>() ; }
/**@}*/

} // end of pcomn::literals

/*******************************************************************************
 eqi(), lti() specializations.
*******************************************************************************/
template<typename I>
inline bool eqi(const strslice &x, magiclbl<I> y) { return eqi(x, strslice(y)) ; }

template<typename I>
inline bool eqi(magiclbl<I> x, const strslice &y) { return eqi(y, x) ; }

template<typename I>
inline bool lti(const strslice &x, magiclbl<I> y) { return lti(x, strslice(y)) ; }

template<typename I>
inline bool lti(magiclbl<I> x, const strslice &y) { return lti(strslice(x), y) ; }

/*******************************************************************************
 Print magic label.
*******************************************************************************/
template<typename I>
inline std::ostream &operator<<(std::ostream &os, const magiclbl<I> &v)
{
   return os << (strslice)v ;
}

} // end of namespace pcomn

#endif /* __PCOMN_MAGICLBL_H */
