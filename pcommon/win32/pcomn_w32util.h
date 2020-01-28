/*-*- mode: c++; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_W32UTIL_H
#define __PCOMN_W32UTIL_H
/*******************************************************************************
 FILE         :   pcomn_w32util.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Win32-related utility functions

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   14 Feb 2000
*******************************************************************************/
#include <pcommon.h>
#include <string>

#include <windows.h>

namespace pcomn {

_PCOMNEXP int message_box_va(HWND owner, unsigned style, const char *title, const char *fmt, va_list argptr) ;
_PCOMNEXP int message_box(HWND owner, unsigned style, const char *title, const char *fmt, ...) ;

_PCOMNEXP char *sys_error_text(int err, char *buf, size_t size) ;

inline std::string sys_error_text(long err)
{
   char buf[512] ;
   return sys_error_text(err, buf, sizeof buf) ;
}

} // end of namespace pcomn

#endif /* __PCOMN_W32UTIL_H */
