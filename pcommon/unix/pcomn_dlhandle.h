/*-*- mode: c++; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_DLHANDLE_H
#define __PCOMN_DLHANDLE_H
/*******************************************************************************
 FILE         :   pcomn_dlhandle.h
 COPYRIGHT    :   Yakov Markovitch, 2012-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   UNIX dlopen()/dlclose() "handle" traits for safe_handle<>

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   22 Feb 2012
*******************************************************************************/
/** @file
 UNIX dlopen()/dlclose() "handle" traits for safe_handle<>
*******************************************************************************/
#include <dlfcn.h>
#include <pcomn_handle.h>

namespace pcomn {

/******************************************************************************/
/** dlopen() handle tag
*******************************************************************************/
struct dlopen_handle_tag { typedef void *handle_type ; } ;

template<>
inline bool handle_traits<dlopen_handle_tag>::close(handle_type h) { return !dlclose(h) ; }

template<>
inline bool handle_traits<dlopen_handle_tag>::is_valid(handle_type h) { return !!h ; }

template<>
inline constexpr void *handle_traits<dlopen_handle_tag>::invalid_handle() { return NULL ; }

typedef safe_handle<dlopen_handle_tag>   dlopen_safehandle ;

} // end of namespace pcomn

#endif /* __PCOMN_DLHANDLE_H */
