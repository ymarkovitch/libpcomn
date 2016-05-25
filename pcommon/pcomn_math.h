/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_MATH_H
#define __PCOMN_MATH_H
/*******************************************************************************
 FILE         :   pcomn_math.h
 COPYRIGHT    :   Yakov Markovitch, 2005-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Math stuff, which is missing from standard C/C++ libraries

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   16 Oct 2005
*******************************************************************************/
#include <math.h>

namespace pcomn {

/*******************************************************************************
 fdiv, fdivmod
 idiv, idivmod
 The family of mathematical functions that implement "mathematically correct"
 both the integer division ('div' operator) and remainder calculation ('mod')
 In C, both integer operation / and % and floating-point fmod() return
 mathematically incorrect results when dividend and divisor have different signs.
*******************************************************************************/
inline double fdiv(double lhs, double rhs)
{
   double result ;
   const double mod = modf(lhs/rhs, &result) ;
   if (mod && (rhs < 0) != (mod < 0))
      result -= 1.0 ;
   return result ;
}

inline double fdivmod(double lhs, double rhs, double *div = NULL)
{
   double mod = fmod(lhs, rhs) ;
   if (div)
      modf(lhs/rhs, div) ;
   // fmod returns not a "mathematical" remainder, but the "C" remainder instead.
   // So ensure the remainder has the same sign as the denominator.
   if (mod && (rhs < 0) != (mod < 0))
   {
      mod += rhs ;
      if (div)
         *div -= 1.0 ;
   }
   return mod ;
}

template<typename T>
inline T idiv(T lhs, T rhs)
{
   const T result = lhs / rhs ;
   const T mod = lhs % rhs ;
   // Please note that (mod ^ rhs) < 0 means that mod and rhs have different
   // sign bits (i.e. tantamount to (rhs < 0) != (mod < 0))
   return (mod && (mod ^ rhs) < 0) ?
      result - 1 : result ;
}

template<typename T>
inline T idivmod(T lhs, T rhs)
{
   const T mod = lhs % rhs ;
   return (mod && (mod ^ rhs) < 0) ? mod + rhs : mod ;
}

template<typename T>
inline T idivmod(T lhs, T rhs, T *div)
{
   const T mod = lhs % rhs ;
   if (mod && (mod ^ rhs) < 0)
   {
      if (div) *div = lhs / rhs - 1 ;
      return mod + rhs ;
   }
   if (div) *div = lhs / rhs ;
   return mod ;
}

} // end of namespace pcomn

#endif /* __PCOMN_MATH_H */
