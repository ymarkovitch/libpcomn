/*-*- mode: c++; tab-width: 4; c-file-style: "stroustrup"; c-file-offsets:((innamespace . 0) (inline-open . 0) (case-label . +)) -*-*/
/*******************************************************************************
 FILE         :   http_diag.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Diagnostics groups definition for HTTP protocol implementation.

 CREATION DATE:   3 Mar 2008
*******************************************************************************/
#ifndef __PCOMN_TRACE
#  define __PCOMN_TRACE
#endif
#include <pcomn_trace.h>
#include <pcomn_http/http_def.h>

DEFINE_DIAG_GROUP(CHTTP_Message, 0, 0, _CHTTPEXP) ;
DEFINE_DIAG_GROUP(CHTTP_Connection, 0, 0, _CHTTPEXP) ;
