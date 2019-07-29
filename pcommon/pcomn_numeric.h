/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_NUMERIC_H
#define __PCOMN_NUMERIC_H
/*******************************************************************************
 FILE         :   pcomn_numeric.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2019. All rights reserved.
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

namespace pcomn {

/***************************************************************************//**
 XAccumulate: eXtract and Accumulate - compute the sum of the given initial value
 and the values extracted from the elements in the range [first, last)
*******************************************************************************/
/**@{*/
template<typename InputIterator,
         typename T, typename UnaryOperation>
inline T xaccumulate(InputIterator first, InputIterator last, T init, UnaryOperation extract)
{
    for (; first != last ; ++first)
       init = std::move(init) + extract(*first) ;

    return init ;
}

template<typename InputIterator,
         typename T, typename UnaryOperation, typename BinaryOperation>
inline T xaccumulate(InputIterator first, InputIterator last, T init, UnaryOperation extract, BinaryOperation op)
{
    for (; first != last ; ++first)
        init = op(std::move(init), extract(*first)) ;

    return init ;
}
/**@}*/

} // end of namespace pcomn

#endif /* __PCOMN_NUMERIC_H */
