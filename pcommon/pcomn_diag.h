/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_DIAG_H
#define __PCOMN_DIAG_H
/*******************************************************************************
 FILE         :   pcomn_diag.h
 COPYRIGHT    :   Yakov Markovitch, 1995-2017. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   PCommon diagnostics groups declaration.

 CREATION DATE:   17 Jan 1995
*******************************************************************************/
#include <pcomn_trace.h>

DECLARE_DIAG_GROUP(PCOMN_Common, _PCOMNEXP) ;
DECLARE_DIAG_GROUP(PCOMN_Test, _PCOMNEXP) ;

DECLARE_DIAG_GROUP(PCOMN_Smartptr, _PCOMNEXP) ;
DECLARE_DIAG_GROUP(PCOMN_BinaryStream, _PCOMNEXP) ;
DECLARE_DIAG_GROUP(PCOMN_Hashtbl, _PCOMNEXP) ;

DECLARE_DIAG_GROUP(PCOMN_Threads, _PCOMNEXP) ;
DECLARE_DIAG_GROUP(PCOMN_ThreadPool, _PCOMNEXP) ;
DECLARE_DIAG_GROUP(PCOMN_Fibers, _PCOMNEXP) ;
DECLARE_DIAG_GROUP(PCOMN_Scheduler, _PCOMNEXP) ;

DECLARE_DIAG_GROUP(PCOMN_Journal, _PCOMNEXP) ;
DECLARE_DIAG_GROUP(PCOMN_Journmmap, _PCOMNEXP) ;

DECLARE_DIAG_GROUP(PCOMN_Exec, _PCOMNEXP) ;

#endif /* PCOMN_DIAG_H */
