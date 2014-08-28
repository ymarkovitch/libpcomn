/*-*- mode: c; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((inclass . ++)) -*-*/
#ifndef __PCOMN_VERSIONINFO_H
#define __PCOMN_VERSIONINFO_H
/*******************************************************************************
 FILE         :   pcomn_versioninfo.h
 COPYRIGHT    :   Yakov Markovitch, 2001-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Version info structures and macros

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   15 Oct 2001
*******************************************************************************/

/* Version as a single 4-byte hex number, e.g. 0x01020200 == 1.2.02 */
#define PCOMN_CONSTRUCT_VERSION_HEX(major,minor,micro) \
   (((major) << 24) | ((minor) << 16) | ((micro) <<  8))

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
      const char *   feature ;
      unsigned long  version_number ;
      const char *   version_string ;
      const char *   comment ;
} PCOMN_version_info_t ;

#ifdef __cplusplus
}
#endif

#endif /* __PCOMN_VERSIONINFO_H */
