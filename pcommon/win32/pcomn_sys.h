/*-*- mode: c++; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_SYS_WIN32_H
#define __PCOMN_SYS_WIN32_H
/*******************************************************************************
 FILE         :   pcomn_sys.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2016. All rights reserved.
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
inline fileoff_t filesize(int fd)
{
   return _filelengthi64(fd) ;
}

inline fileoff_t filesize(const char *name)
{
   FILE * const f = fopen(name, "r") ;
   if (!f)
      return -1 ;
   const fileoff_t result = _filelengthi64(fileno(f)) ;
   fclose(f) ;
   return result ;
}

inline unsigned long long thread_id()
{
   return GetCurrentThreadId() ;
}

inline size_t pagesize()
{
   static SYSTEM_INFO info ;
   static SYSTEM_INFO *pinfo ;
   if (!pinfo)
   {
      GetSystemInfo(&info) ;
      pinfo = &info ;
   }
   return info.dwAllocationGranularity ;
}

} // end of namespace pcomn::sys
} // end of namespace pcomn

#endif /* __PCOMN_SYS_WIN32_H */
