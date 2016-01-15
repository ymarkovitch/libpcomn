/*-*- mode: c++; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_W32HANDLE_H
#define __PCOMN_W32HANDLE_H
/*******************************************************************************
 FILE         :   pcomn_w32handle.h
 COPYRIGHT    :   Yakov Markovitch, 2005-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Win32 HANDLE and HMODULE handle traits for safe_handle<>

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   27 Jul 2005
*******************************************************************************/
#include <windows.h>
#include <pcomn_handle.h>

namespace pcomn {

/*******************************************************************************
 win_handle_tag
*******************************************************************************/
struct win32_handle_tag { typedef HANDLE handle_type ; } ;

template<>
inline bool handle_traits<win32_handle_tag>::close(HANDLE h)
{
   return !!CloseHandle(h) ;
}
template<>
inline bool handle_traits<win32_handle_tag>::is_valid(HANDLE h)
{
   return h && h != INVALID_HANDLE_VALUE ;
}
template<>
inline constexpr HANDLE handle_traits<win32_handle_tag>::invalid_handle()
{
   return INVALID_HANDLE_VALUE ;
}

/*******************************************************************************
 win32dll_handle_traits
*******************************************************************************/
struct win32dll_handle_tag { typedef HMODULE handle_type ; } ;

template<>
inline bool handle_traits<win32dll_handle_tag>::close(HMODULE h)
{
   return !!::FreeLibrary(h) ;
}

template<>
inline bool handle_traits<win32dll_handle_tag>::is_valid(HMODULE h)
{
   return !!h ;
}

template<>
inline constexpr HMODULE handle_traits<win32dll_handle_tag>::invalid_handle()
{
   return NULL ;
}

/*******************************************************************************

*******************************************************************************/
typedef safe_handle<win32_handle_tag>    win32os_safehandle ;
typedef safe_handle<win32dll_handle_tag> win32dll_safehandle ;

} // end of namespace pcomn

#endif /* __PCOMN_W32HANDLE_H */
