/*-*- mode: c; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel" -*-*/
/*******************************************************************************
 FILE         :   packpshn.h
 COPYRIGHT    :   Yakov Markovitch, 1998-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Push Pack Native - set platform-native alignment for structures

 CREATION DATE:   12 Oct 1998
*******************************************************************************/
#ifndef __PCOMN_PLATFORM_H
#include <pcomn_platform.h>
#endif /* PCOMN_PLATFORM.H */

#if (PCOMN_STD_ALIGNMENT >= 4)
#  include <packpsh4.h>
#elif (PCOMN_STD_ALIGNMENT >= 2)
#  include <packpsh2.h>
#else
#  include <packpsh1.h>
#endif

#ifdef __BORLANDC__
#  pragma nopackwarning
#endif
