/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   unittest_bitvector_AVX2.cpp
 COPYRIGHT    :   Yakov Markovitch, 2020
 DESCRIPTION  :   Shim source file to separately compile unittest_bitvector for AVX2
                  instructions set.
 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   24 Jan 2020
*******************************************************************************/
#include <pcomn_platform.h>

#ifndef PCOMN_PL_SIMD_AVX2
#error This test must be compiled for AVX2 instruction set.
#endif

#include "unittest_bitvector.cpp"
