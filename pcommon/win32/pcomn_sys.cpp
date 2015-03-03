/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_sys.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   System (platform) functions for Windows

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   18 Nov 2008
*******************************************************************************/
#include <pcomn_platform.h>
#ifdef PCOMN_PL_WINDOWS
#include "../pcomn_sys.h"
#else
#error This flie is only for Windows platforms!
#endif
#include <windows.h>

namespace pcomn {
namespace sys {

size_t pagesize()
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
