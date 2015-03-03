/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_PRIMENUM_H
#define __PCOMN_PRIMENUM_H
/*******************************************************************************
 FILE         :   pcomn_primenum.h
 COPYRIGHT    :   Yakov Markovitch, 2010-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Primary numbers
 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   8 May 2009
*******************************************************************************/
#include <pcomn_meta.h>

namespace pcomn {

template<unsigned pow2> struct pow2_primefloor ;
template<unsigned pow2> struct pow2_primeceil ;

#define PCOMN_PRIMEBOUNDS(power, floor, ceil) PCOMN_PRIMECEIL(power, ceil) PCOMN_PRIMEFLOOR(power, floor)

#define PCOMN_PRIMECEIL(power, prime)                           \
   template<> struct pow2_primeceil<power>  : std::integral_constant<unsigned, prime> {} ;
#define PCOMN_PRIMEFLOOR(power, prime)                           \
   template<> struct pow2_primefloor<power> : std::integral_constant<unsigned, prime> {}

PCOMN_PRIMEBOUNDS(0, 1, 1) ;
PCOMN_PRIMEBOUNDS(1, 2, 3) ;
PCOMN_PRIMEBOUNDS(2, 3, 5) ;
PCOMN_PRIMEBOUNDS(3, 7, 11) ;
PCOMN_PRIMEBOUNDS(4, 13, 17) ;
PCOMN_PRIMEBOUNDS(5, 31, 37) ;
PCOMN_PRIMEBOUNDS(6, 61, 67) ;
PCOMN_PRIMEBOUNDS(7, 127, 131) ;
PCOMN_PRIMEBOUNDS(8, 251, 257) ;
PCOMN_PRIMEBOUNDS(9, 509, 521) ;
PCOMN_PRIMEBOUNDS(10, 1021, 1031) ;
PCOMN_PRIMEBOUNDS(11, 2039, 2053) ;
PCOMN_PRIMEBOUNDS(12, 4093, 4099) ;
PCOMN_PRIMEBOUNDS(13, 8191, 8209) ;
PCOMN_PRIMEBOUNDS(14, 16381, 16411) ;
PCOMN_PRIMEBOUNDS(15, 32749, 32771) ;
PCOMN_PRIMEBOUNDS(16, 65521, 65537) ;
PCOMN_PRIMEBOUNDS(17, 131071, 131101) ;
PCOMN_PRIMEBOUNDS(18, 262139, 262147) ;
PCOMN_PRIMEBOUNDS(19, 524287, 524309) ;
PCOMN_PRIMEBOUNDS(20, 1048573, 1048583) ;
PCOMN_PRIMEBOUNDS(21, 2097143, 2097169) ;
PCOMN_PRIMEBOUNDS(22, 4194301, 4194319) ;
PCOMN_PRIMEBOUNDS(23, 8388593, 8388617) ;
PCOMN_PRIMEBOUNDS(24, 16777213, 16777259) ;
PCOMN_PRIMEBOUNDS(25, 33554393, 33554467) ;
PCOMN_PRIMEBOUNDS(26, 67108859, 67108879) ;
PCOMN_PRIMEBOUNDS(27, 134217689, 134217757) ;
PCOMN_PRIMEBOUNDS(28, 268435399, 268435459) ;
PCOMN_PRIMEBOUNDS(29, 536870909, 536870923) ;
PCOMN_PRIMEBOUNDS(30, 1073741789, 1073741827) ;
PCOMN_PRIMEBOUNDS(31, 2147483647, 2147483659UL) ;

PCOMN_PRIMEFLOOR(32, 4294967291UL) ;

#undef PCOMN_PRIMEFLOOR
#undef PCOMN_PRIMECEIL

} ; // end of namespace pcomn

#endif /* __PCOMN_PRIMENUM_H */
