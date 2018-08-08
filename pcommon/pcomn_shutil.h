/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_SHUTIL_H
#define __PCOMN_SHUTIL_H
/*******************************************************************************
 FILE         :   pcomn_shutil.h
 COPYRIGHT    :   Yakov Markovitch, 2011-2018. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   High-level operations on files and collections of files.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   1 Jun 2011
*******************************************************************************/
/** @file
 High-level operations on files and collections of files, in particular, file copying.
*******************************************************************************/
#include <pcomn_strslice.h>

namespace pcomn {
namespace sys {

/***************************************************************************//**
 File/directory copying flags.
*******************************************************************************/
enum CopyFlags {
   CP_IGNORE_ERRORS     = 0x01,  /**< On error, don't throw exception, just return false */
   CP_DONT_PRESERVE     = 0x02,  /**< Don't preserve attributes */
   CP_FOLLOW_SRC_LINKS  = 0x04,  /**< If the source argument is a symbolic link, dereference it. */
   CP_FOLLOW_ALL_LINKS  = 0x08,  /**< Dereference all symlinks (when copytree() is called or CP_SRC_ALLOW_DIR is set) */
   CP_SRC_ALLOW_DIR     = 0x10,  /**< The source argument allowed to be a directory, in which case it is recursively copied. */
   CP_DST_REQUIRE_DIR   = 0x20   /**< The destination argument must be an existent directory */
} ;

PCOMN_DEFINE_FLAG_ENUM(CopyFlags) ;

/// Copy the contents of a source file to a destination file or a directory.
/// @param source Source file name; must not refer to a directory.
/// @param dest   Destination file name; if refers to a file, the file is replaced with
/// @a source, if refers to a directory, @a source is copied to that directory.
/// @param flags  ORed CopyFlags
///
_PCOMNEXP bool copyfile(const pcomn::strslice &source, const pcomn::strslice &dest, unsigned flags = 0) ;

_PCOMNEXP bool copytree(const pcomn::strslice &sourcedir, const pcomn::strslice &destdir, unsigned flags = 0) ;

/***************************************************************************//**
 Remove flags.

 @note By default, pcomn::sys::rm is more or less "foolprof", disallowing by default
 the most dangerous behaviours. When needed, these behaviours may be allowed explicitly
 using RM_ALLOW_RELPATH, RM_ALLOW_DEPTH1, and RM_ALLOW_ROOT flags.
*******************************************************************************/
enum RmFlags {
   RM_IGNORE_ERRORS  = 0x01,  /**< On error, don't throw exception, just return false */
   RM_IGNORE_NEXIST  = 0x02,  /**< Ignore nonexistent files (and return true) */
   RM_RECURSIVE      = 0x04,  /**< Remove directories */
   RM_ALLOW_RELPATH  = 0x08,  /**< Allow to specify relative paths */
   RM_ALLOW_ROOTDIR  = 0x10   /**< Allow to remove immediately from "/" */
} ;

PCOMN_DEFINE_FLAG_ENUM(RmFlags) ;

_PCOMNEXP bool rm(const pcomn::strslice &path, unsigned flags = 0) ;

} // end of namespace pcomn::sys
} // end of namespace pcomn

#endif /* __PCOMN_SHUTIL_H */
