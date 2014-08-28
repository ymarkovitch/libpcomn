/*-*- mode: c; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((inclass . ++)) -*-*/
#ifndef __PCOMN_FLOAT_H
#define __PCOMN_FLOAT_H
/*******************************************************************************
 FILE         :   pcomn_float.h
 COPYRIGHT    :   Yakov Markovitch, 2001-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Compatibility wrapper for float.h standard header.
                  Since some Windows compiler have some POSIX floating-point
                  functions named in nonstandard fashion (with leading underscore),
                  this file defines macros for them

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   20 Dec 2001
*******************************************************************************/
#include <pcommon.h>
#include <float.h>

#ifdef PCOMN_NONSTD_FP_PREDICATES

#ifndef __cplusplus
#  define finite(d) _finite(d)
#  define isnan(d) _isnan(d)
#else
inline int finite(double d) { return _finite(d) ; }
inline int isnan(double d) { return _isnan(d) ; }
#endif

#endif

#endif /* __PCOMN_FLOAT_H */
