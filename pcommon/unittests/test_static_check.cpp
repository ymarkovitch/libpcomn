/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   test_static_check.cpp
 COPYRIGHT    :   Yakov Markovitch, 2006-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Test compile-time checks.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   15 Nov 2006
*******************************************************************************/
#include <pcomn_assert.h>

// This should compile OK
PCOMN_STATIC_CHECK(sizeof(int) >= sizeof(char)) ;

// This should fail
PCOMN_STATIC_CHECK(sizeof(int) < sizeof(char)) ;
