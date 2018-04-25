/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_BUILD_H
#define __PCOMN_BUILD_H
/*******************************************************************************
 FILE          :  pcomn_build.h
 COPYRIGHT    :   Yakov Markovitch, 2009-2017. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION   :  Build configuration constants.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE :  13 Jul 2009
*******************************************************************************/
/** @file
 Build configuration constants: SVN version, host, buildname, etc.
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcomn_macros.h>

#ifdef SVN_VERSION
#define P_SVN_VERSION P_STRINGIFY_I(SVN_VERSION)
#else
#error SVN_VERSION is not defined
#endif

#ifdef SVN_PROJPATH
#define P_SVN_PROJPATH P_STRINGIFY_I(SVN_PROJPATH)
#else
#error SVN_PROJPATH is not defined
#endif

#ifdef BUILD_HOST
#define P_BUILD_HOST P_STRINGIFY_I(BUILD_HOST)
#else
#error BUILD_HOST is not defined
#endif

#define P_VERSION_OUTPUT(appname, cmdname) (appname) << " (" << (cmdname) \
    << (") (r"P_SVN_VERSION" "P_SVN_PROJPATH", "__DATE__", "__TIME__")\n["PCOMN_COMPILER_NAME" "__VERSION__"] on "P_BUILD_HOST)

#define P_VERSION_FORMAT(appname) \
    (appname " (%s) (r"P_SVN_VERSION" "P_SVN_PROJPATH", "__DATE__", "__TIME__")\n["PCOMN_COMPILER_NAME" "__VERSION__"] on "P_BUILD_HOST)

#endif /* __PCOMN_BUILD_H */
