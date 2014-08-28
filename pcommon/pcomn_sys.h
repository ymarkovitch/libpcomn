/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_SYS_H
#define __PCOMN_SYS_H
/*******************************************************************************
 FILE         :   pcomn_sys.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   System (platform) functions

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   18 Nov 2008
*******************************************************************************/
/** @file
 System (platform) functions
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcomn_def.h>
#include PCOMN_PLATFORM_HEADER(pcomn_sys.h)

namespace pcomn {
namespace sys {

enum Access {
   ACC_EXISTS,
   ACC_NOEXIST,
   ACC_DENIED,
   ACC_ERROR
} ;

/// Get the memory page size for the platform.
_PCOMNEXP size_t pagesize() ;

/// Portable filesize function.
/// @return The file size, or -1 on error.
inline off_t filesize(int fd) ;

/// @overload
inline off_t filesize(const char *name) ;

inline Access fileaccess(const char *name, int mode = 0)
{
   if (::access(name, mode) == 0)
      return ACC_EXISTS ;
   switch (errno)
   {
      case ENOENT:   return ACC_NOEXIST ;
      #ifdef PCOMN_PL_UNIX
      case EROFS:
      #endif
      case EACCES:   return ACC_DENIED ;
   }
   return ACC_ERROR ;
}

/// Get CPU cores count on the system.
/// @return The total count of @em actual cores on all physical CPUs in the system @em
/// not counting hyperthreads.
/// @param[out] phys_sockets  The function places here a count of physical CPUs (sockets).
/// @param[out] ht_count      The function places here a count of cores counting also
/// hyperthreads.
_PCOMNEXP unsigned cpu_core_count(unsigned *phys_sockets = NULL, unsigned *ht_count = NULL) ;

inline unsigned hw_threads_count()
{
   unsigned result = 0 ;
   cpu_core_count(NULL, &result) ;
   return result ;
}

} // end of namespace pcomn::sys
} // end of namespace pcomn

#endif /* __PCOMN_SYS_H */
