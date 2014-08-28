/*-*- mode: c; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((inclass . ++)) -*-*/
#ifndef ___PREGEX_H
#define ___PREGEX_H
/*******************************************************************************
 FILE         :   _pregex.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Private header

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   18 Jan 2000
*******************************************************************************/
#ifndef __PBREGEX_H
#include <pbregex.h>
#endif /* PBREGEX.H */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
      PRegError       lasterror ;
      regexp_handler  errhandler ;
} _regex_errproc ;

_regex_errproc *regex_errproc(void) ;

#ifdef __cplusplus
} ;
#endif

#endif /* ___PREGEX_H */
