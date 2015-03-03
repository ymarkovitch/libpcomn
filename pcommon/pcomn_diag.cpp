/*-*- tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_diag.cpp
 COPYRIGHT    :   Yakov Markovitch, 1995-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   PCommon library diagnostics groups

 CREATION DATE:   17 Jan 1995
*******************************************************************************/
#ifndef __PCOMN_TRACE
#  define __PCOMN_TRACE
#endif

#include <pcomn_trace.h>

DEFINE_DIAG_GROUP(PCOMN_Common, 0, 0, _PCOMNEXP) ;
DEFINE_DIAG_GROUP(PCOMN_Test, 1, DBGL_VERBOSE, _PCOMNEXP) ;

DEFINE_DIAG_GROUP(PCOMN_Smartptr, 0, 0, _PCOMNEXP) ;
DEFINE_DIAG_GROUP(PCOMN_BinaryStream, 0, 0, _PCOMNEXP) ;
DEFINE_DIAG_GROUP(PCOMN_Hashtbl, 0, 0, _PCOMNEXP) ;

DEFINE_DIAG_GROUP(PCOMN_Threads, 0, 0, _PCOMNEXP) ;
DEFINE_DIAG_GROUP(PCOMN_ThreadPool, 0, 0, _PCOMNEXP) ;
DEFINE_DIAG_GROUP(PCOMN_Fibers, 0, 0, _PCOMNEXP) ;
DEFINE_DIAG_GROUP(PCOMN_Scheduler, 0, 0, _PCOMNEXP) ;

DEFINE_DIAG_GROUP(PCOMN_Journal, 0, 0, _PCOMNEXP) ;
DEFINE_DIAG_GROUP(PCOMN_Journmmap, 0, 0, _PCOMNEXP) ;

DEFINE_DIAG_GROUP(PCOMN_Exec, 0, 0, _PCOMNEXP) ;
