/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_METAFUNCTIONAL_H
#define __PCOMN_METAFUNCTIONAL_H
/*******************************************************************************
 FILE         :   pcomn_metafunctional.h
 COPYRIGHT    :   Yakov Markovitch, 2007-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Compile-time algorithms and functions.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   26 Jan 2007
*******************************************************************************/
/** @file
  Compile-time algorithms and functions.
*******************************************************************************/
#include <pcomn_integer.h>
#include <pcomn_meta.h>

namespace pcomn {

/*******************************************************************************
 Helper templates
*******************************************************************************/
/// @cond
namespace detail {
template<unsigned szpower2, int bitpattern>
struct static_memset2 {
      static void fill(void *mem)
      {
         static_memset2<szpower2/2, bitpattern>::fill(mem) ;
         static_memset2<szpower2/2, bitpattern>::fill((char *)mem + szpower2/2) ;
      }
} ;

template<unsigned sz, int bitpattern>
struct static_memset {
      static void fill(void *mem)
      {
         static_memset<bitop::ct_clrrnzb<sz>::value, bitpattern>::fill(mem) ;
         static_memset2<bitop::ct_getrnzb<sz>::value, bitpattern>::
            fill((char *)mem + bitop::ct_clrrnzb<sz>::value) ;
      }
} ;

template<int bitpattern>
struct static_memset<0, bitpattern> { static void fill(void *) {} } ;
template<int bitpattern>
struct static_memset2<0, bitpattern> { static void fill(void *) {} } ;

#define _PCOMN_BP_FILL(nbit)                                            \
   template<int bitpattern>                                             \
   struct static_memset2<sizeof(int##nbit##_t), bitpattern>             \
   { static void fill(void *mem) { *static_cast<int##nbit##_t *>(mem) = bitpattern ; } }

_PCOMN_BP_FILL(8) ; _PCOMN_BP_FILL(16) ; _PCOMN_BP_FILL(32) ; _PCOMN_BP_FILL(64) ;
#undef _PCOMN_BP_FILL

template<unsigned szpower2>
struct static_memcpy2 {
      static void cpy(void *dest, const void *src)
      {
         static_memcpy2<szpower2/2>::cpy(dest, src) ;
         static_memcpy2<szpower2/2>::cpy((char *)dest + szpower2/2,
                                         (const char *)src + szpower2/2) ;
      }
} ;

template<unsigned sz>
struct static_memcpy {
      static void cpy(void *dest, const void *src)
      {
         static_memcpy<bitop::ct_clrrnzb<sz>::value>::cpy(dest, src) ;
         static_memcpy2<bitop::ct_getrnzb<sz>::value>::
            cpy((char *)dest + bitop::ct_clrrnzb<sz>::value,
                (const char *)src + bitop::ct_clrrnzb<sz>::value) ;
      }
} ;

template<>
struct static_memcpy<0> { static void cpy(void *, const void *) {} } ;
template<>
struct static_memcpy2<0> { static void cpy(void *, const void *) {} } ;

#define _PCOMN_CPY(nbit)                                                \
   template<> struct static_memcpy2<sizeof(int##nbit##_t)>              \
   { static void cpy(void *dest, const void *src)                       \
      { *static_cast<int##nbit##_t *>(dest) = *static_cast<const int##nbit##_t *>(src) ; } }

_PCOMN_CPY(8) ; _PCOMN_CPY(16) ; _PCOMN_CPY(32) ; _PCOMN_CPY(64) ;
#undef _PCOMN_CPY
} // end of namespace pcomn::detail
/// @endcond

/*******************************************************************************
 Memory-filling functions
*******************************************************************************/
template<int bitpattern, unsigned sz>
inline void *static_memset(void *mem)
{
   detail::static_memset<sz, bitpattern>::fill(mem) ;
   return mem ;
}

template<int bitpattern, typename T>
inline std::enable_if_t<std::is_pointer<T>::value, T> static_fill(T mem)
{
   static_memset<bitpattern, sizeof *mem>(mem) ;
   return mem ;
}

template<int bitpattern, typename T, size_t n>
inline T *static_fill(T (&mem)[n])
{
   static_memset<bitpattern, sizeof mem>(mem) ;
   return mem ;
}

template<unsigned sz>
inline void *static_memcpy(void *dest, const void *src)
{
   if (dest != src)
      detail::static_memcpy<sz>::cpy(dest, src) ;
   return dest ;
}

template<typename T>
inline T *static_copy(T *dest, const T *src)
{
   static_memcpy<sizeof(T)>(dest, src) ;
   return dest ;
}

template<template<typename> class F, typename... Types>
struct count_types_if ;

template<template<typename> class F>
struct count_types_if<F> : std::integral_constant<int, 0> {} ;

template<template<typename> class F, typename Head, typename... Tail>
struct count_types_if<F, Head, Tail...> : std::integral_constant<int, ((int)!!F<Head>::value + count_types_if<F, Tail...>::value)> {} ;

} // end of namespace pcomn

#endif /* __PCOMN_METAFUNCTIONAL_H */
