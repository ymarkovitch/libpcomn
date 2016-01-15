/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
#ifndef __PCOMN_CPP11_H
#define __PCOMN_CPP11_H
/*******************************************************************************
 FILE         :   pcomn_cpp11.h
 COPYRIGHT    :   Yakov Markovitch, 2012-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Ensure the compilation unit is being compiled with C++ 11 compiler

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   13 Nov 2012
*******************************************************************************/
/** @file
 Ensure the compilation unit is being compiled with C++11 compiler
*******************************************************************************/

#if defined(__cplusplus) && __cplusplus <= 199711
#error C++11 compiler is required for compilation
#endif

#endif /* __PCOMN_CPP11_H */
