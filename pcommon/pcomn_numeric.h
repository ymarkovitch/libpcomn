/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_NUMERIC_H
#define __PCOMN_NUMERIC_H
/*******************************************************************************
 FILE         :   pcomn_numeric.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Add-on generalized numeric algorithms
 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   27 May 2000
*******************************************************************************/
#include <numeric>
#include <algorithm>
#include <functional>
#include <stdexcept>

#include <stdlib.h>

namespace pcomn {

using std::iota ;

const unsigned UPRIME_MIN = 3u ;
const unsigned UPRIME_MAX = 4294967291u ;

template<Instantiate = Instantiate{}>
struct doubling_primes {

      static unsigned lbound(unsigned num)
      {
         return
            *(std::find_if(primes + 1, primes + P_ARRAY_COUNT(primes),
                           std::bind1st(std::less<unsigned>(), num)) - 1) ;
      }
      static unsigned ubound(unsigned num)
      {
         return
            *std::find_if(primes + 0, primes + (P_ARRAY_COUNT(primes) - 1),
                          std::bind1st(std::less_equal<unsigned>(), num)) ;
      }

      static const unsigned primes[32] ;
} ;

template<Instantiate i>
const unsigned doubling_primes<i>::primes[32] =
{
   3u,            7u,            17u,           29u,
   53u,           97u,           193u,          389u,
   769u,          1543u,         3079u,         6151u,
   12289u,        24593u,        49157u,        98317u,
   196613u,       393241u,       786433u,       1572869u,
   3145739u,      6291469u,      12582917u,     25165843u,
   50331653u,     100663319u,    201326611u,    402653189u,
   805306457u,    1610612741u,   3221225473u,   4294967291u
} ;

inline unsigned dprime_lbound(unsigned num) { return doubling_primes<>::lbound(num) ; }

inline unsigned dprime_ubound(unsigned num) { return doubling_primes<>::ubound(num) ; }

} ; // end of namespace pcomn



#endif /* __PCOMN_NUMERIC_H */
