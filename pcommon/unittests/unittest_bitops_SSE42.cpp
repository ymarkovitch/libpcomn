/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   unittest_bitops_SSE42.cpp
 COPYRIGHT    :   Yakov Markovitch, 2020
 DESCRIPTION  :   Shim source file to separately compile unittest_bitops for SSE 4.2
                  instructions set.
 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   3 Sep 2020
*******************************************************************************/
#include <pcomn_platform.h>

#if !defined (PCOMN_PL_SIMD_SSE42) || defined (PCOMN_PL_SIMD_AVX2) || defined (PCOMN_PL_SIMD_AVX)
#error This test must be compiled for SSE 4.2 instruction set (and AVX/AVX2 shall not be enabled).
#endif

#include "unittest_bitops.cpp"
