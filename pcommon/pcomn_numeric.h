/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_NUMERIC_H
#define __PCOMN_NUMERIC_H
/*******************************************************************************
 FILE         :   pcomn_numeric.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2018. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Add-on generalized numeric algorithms
 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   27 May 2000
*******************************************************************************/
#include <pcomn_platform.h>
#include <numeric>

#ifndef PCOMN_STL_CXX17
#include <experimental/numeric>

namespace std {

using std::experimental::gcd ;
using std::experimental::lcm ;

} // end of namespace std

#endif /* PCOMN_STL_CXX17 */

#endif /* __PCOMN_NUMERIC_H */
