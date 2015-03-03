/*-*- mode: c; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((inclass . ++)) -*-*/
#ifndef __PCOMN_VERSION_H
#define __PCOMN_VERSION_H
/*******************************************************************************
 FILE         :   pcomn_version.h
 COPYRIGHT    :   Yakov Markovitch, 2001-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   A version identification scheme.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   15 Oct 2001
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcomn_macros.h>
#include <pcomn_versioninfo.h>
#include <pcomn_def.h>

/* Version parsed out into numeric values */
#define PCOMN_MAJOR_VERSION    5
#define PCOMN_MINOR_VERSION    0
#define PCOMN_MICRO_VERSION    1

/* Version as a string */
#define PCOMN_VERSION		"5.0.01"

/* Version as a single 4-byte hex number, e.g. 0x01020200 == 1.2.02
   Use this for numeric comparisons, e.g. #if PCOMN_VERSION_HEX >= ... */
#define PCOMN_VERSION_HEX PCOMN_CONSTRUCT_VERSION_HEX(PCOMN_MAJOR_VERSION,  \
                                                      PCOMN_MINOR_VERSION,  \
                                                      PCOMN_MICRO_VERSION)

#define PCOMN_BUILD_STRING (P_STRINGIFY_I(PCOMN_COMPILER) " " __VERSION__ ", " __DATE__ " " __TIME__)

#ifdef __cplusplus
extern "C" {
#endif

_PCOMNEXP const PCOMN_version_info_t *pcomn_version_info(void) ;

#ifdef __cplusplus
}
#endif


#endif /* __PCOMN_VERSION_H */
