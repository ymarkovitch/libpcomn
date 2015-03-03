/*-*- mode: c; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((inclass . ++)) -*-*/
#ifndef __PCOMN_DEF_H
#define __PCOMN_DEF_H
/*******************************************************************************
 FILE         :   pcomn_def.h
 COPYRIGHT    :   Yakov Markovitch, 1995-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Import/export definitions for PCommon library.
                  Includes pcommon.h so could be used as universal base header.

 CREATION DATE:   21 Jan 1995
*******************************************************************************/
#include <pcomn_platform.h>

/*******************************************************************************
 Export/import macros
*******************************************************************************/
#if defined(__PCOMMON_BUILD)
#  define _PCOMNEXP   __EXPORT
#elif defined(__PCOMMON_DLL)
#  define _PCOMNEXP   __IMPORT
#else
#  define _PCOMNEXP
#endif

#endif /* __PCOMN_DEF_H */
