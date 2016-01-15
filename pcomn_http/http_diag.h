/*-*- mode: c++; tab-width: 4; c-file-style: "stroustrup"; c-file-offsets:((innamespace . 0) (inline-open . 0) (case-label . +)) -*-*/
#ifndef __HTTP_DIAG_H
#define __HTTP_DIAG_H
/*******************************************************************************
 FILE         :   commsvr_diag.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Diagnostics groups declaration for HTTP protocol implementation.

 CREATION DATE:   3 Mar 2008
*******************************************************************************/
#include <pcomn_trace.h>
#include <pcomn_http/http_def.h>

DECLARE_DIAG_GROUP(CHTTP_Message, _CHTTPEXP) ;
DECLARE_DIAG_GROUP(CHTTP_Connection, _CHTTPEXP) ;

#endif /* __HTTP_DIAG_H */
