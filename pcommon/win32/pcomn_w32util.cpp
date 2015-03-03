/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_w32util.cpp
 COPYRIGHT    :   Yakov Markovitch, 1996-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Extended message boxes and error string by code for Win32

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   26 Oct 1996
*******************************************************************************/
#include <win32/pcomn_w32util.h>
#include <pcstring.h>

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

namespace pcomn {

int message_box_va(HWND owner, bigflag_t style, const char *title, const char *fmt, va_list argptr)
{
   char *text = (char *)memset (new char[2048], 0, 2048) ;
   vsprintf (text, fmt, argptr) ;
   int result = MessageBoxA(owner, text, title, style) ;
   delete [] text ;
   return result ;
}

int message_box(HWND owner, bigflag_t style, const char *title, const char *fmt, ...)
{
   va_list argptr ;
   int result ;

   va_start (argptr, fmt) ;
   result = message_box_va(owner, style, title, fmt, argptr) ;
   va_end (argptr) ;
   return result ;
}

char *sys_error_text(int err, char *buf, size_t size)
{
   if (size > 0)
   {
      *buf = buf[size-1] = '\0' ;
      if (!FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err,
                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                          buf, size-1, NULL))
         snprintf(buf, size, "System error code 0x%X. No error text found.", (unsigned)err) ;
   }
   return buf ;
}

} // end of namespace pcomn
