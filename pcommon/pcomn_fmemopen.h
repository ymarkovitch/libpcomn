/*-*- mode:c;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel" -*-*/
#ifndef __PCOMN_FMEMOPEN_H
#define __PCOMN_FMEMOPEN_H
/*******************************************************************************
 FILE         :   pcomn_fmemopen.h
 COPYRIGHT    :   Yakov Markovitch, 2010-2017. All rights reserved.

 DESCRIPTION  :   A C memory stream (FILE *) for Linux and FreeBSD

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   12 Oct 2010
*******************************************************************************/
#include <pcomn_platform.h>

#if defined(PCOMN_PL_FREEBSD)
#include <stdio.h>

extern
#ifdef __cplusplus
"C"
#endif
FILE *pcomn_fmemopen(void *buf, ssize_t len, const char *mode) ;

#define fmemopen(buf, len, mode) (pcomn_fmemopen((buf), (len), (mode)))

#elif defined(PCOMN_PL_LINUX)

#ifndef _GNU_SOURCE
#error _GNU_SOURCE is not defined, cannot use fmemopen()
#endif

#include <stdio.h>

#else
#error fmemopen() is only supported for GNU LIBC and FreeBSD
#endif

#endif /* __PCOMN_FMEMOPEN_H */
