/*-*- mode: c++; tab-width: 4; c-file-style: "stroustrup"; c-file-offsets:((innamespace . 0) (inline-open . 0) (case-label . +)) -*-*/
#ifndef __HTTP_DEF_H
#define __HTTP_DEF_H
/*******************************************************************************
 FILE         :   http_def.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Export/import macro declarations for HTTP protocol library.

 CREATION DATE:   02 Mar 2008
*******************************************************************************/
#include <pcomn_platform.h>

#if (defined(__CHTTP_BUILD) || defined(__CHTTP_DLL)) && !defined(_RTLDLL)
#  error You must use shared RTL with shared communication server library!
#endif

#if defined(__CHTTP_BUILD)
#  define _CHTTPEXP __EXPORT
#elif defined(__CHTTP_DLL)
#  define _CHTTPEXP __IMPORT
#else
#  define _CHTTPEXP
#endif

#endif /* __HTTP_DEF_H */
