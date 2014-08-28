/*-*- mode: c++; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_SYS_WIN32_H
#define __PCOMN_SYS_WIN32_H
/*******************************************************************************
 FILE         :   pcomn_sys.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   System routines for Windows

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   18 Nov 2008
*******************************************************************************/
/** @file
 System routines for Windows
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcomn_unistd.h>

#include <windows.h>

namespace pcomn {
namespace sys {

/******************************************************************************/
/** Portable filesize function.
 @return The current file size, or -1 on error.
*******************************************************************************/
inline off_t filesize(int fd)
{
   return _filelengthi64(fd) ;
}

inline off_t filesize(const char *name)
{
   FILE * const f = fopen(name, "r") ;
   if (!f)
      return -1 ;
   const off_t result = _filelengthi64(fileno(f)) ;
   fclose(f) ;
   return result ;
}

} // end of namespace pcomn::sys
} // end of namespace pcomn

#endif /* __PCOMN_SYS_WIN32_H */
