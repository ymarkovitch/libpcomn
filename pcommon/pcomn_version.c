/*-*- mode: c; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel" -*-*/
/*******************************************************************************
 FILE         :   pcomn_version.c
 COPYRIGHT    :   Yakov Markovitch, 2001-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Version identification data

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   12 Oct 2001
*******************************************************************************/
#include <pcomn_version.h>

const PCOMN_version_info_t *pcomn_version_info()
{
   static PCOMN_version_info_t _version_info =
   {
      "pcomn",
      PCOMN_VERSION_HEX,
      PCOMN_VERSION,
      "PCommon Library"
   } ;
   return &_version_info ;
}
