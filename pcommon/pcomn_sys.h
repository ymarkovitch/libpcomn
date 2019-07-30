/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)(inlambda . 0)) -*-*/
#ifndef __PCOMN_SYS_H
#define __PCOMN_SYS_H
/*******************************************************************************
 FILE         :   pcomn_sys.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   System (platform) functions

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   18 Nov 2008
*******************************************************************************/
/** @file
 System (platform) functions
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcomn_cstrptr.h>
#include <pcomn_except.h>
#include <pcomn_unistd.h>
#include <pcomn_path.h>
#include <pcomn_handle.h>

#include "platform/dirent.h"

namespace pcomn {
namespace sys {

enum Access {
   ACC_EXISTS,
   ACC_NOEXIST,
   ACC_DENIED,
   ACC_ERROR
} ;

/// Get the memory page size for the platform.
inline size_t pagesize() ;

inline size_t pagemask()
{
   return pagesize() - 1 ;
}

inline size_t pagemulsize(size_t sz)
{
   return (sz + pagemask()) & pagemask() ;
}

inline void *pagealloc() ;

inline void pagefree(void *) ;

inline void *alloc_aligned(size_t align, size_t sz)
{
   return
      #ifndef PCOMN_PL_MS
      aligned_alloc(align, sz)
      #else
      _aligned_malloc(sz, align)
      #endif
      ;
}

inline void free_aligned(void *data)
{
   #ifndef PCOMN_PL_MS
   free(data) ;
   #else
   _aligned_free(data) ;
   #endif
}

template<typename T>
inline T *alloc_aligned(size_t count)
{
   return count
      ? static_cast<T *>(alloc_aligned(alignof(T), count*sizeof(T)))
      : nullptr ;
}

template<typename T>
inline T *alloc_aligned()
{
   return alloc_aligned<T>(1) ;
}

/// Portable filesize function.
/// @return The file size, or -1 on error.
inline fileoff_t filesize(int fd) ;

/// @overload
inline fileoff_t filesize(const char *name) ;

inline fileoff_t filesize(const cstrptr &name)
{
   return filesize(name.c_str()) ;
}

inline Access fileaccess(const cstrptr &name, int mode = 0) noexcept
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

/// Get platform-dependent 64-bit thread ID
inline unsigned long long thread_id() ;

/*******************************************************************************
 Directory listing
*******************************************************************************/
const unsigned ODIR_SKIP_DOT     = 0x0001 ; /**< Skip '.' while writing filenames */
const unsigned ODIR_SKIP_DOTDOT  = 0x0002 ; /**< Skip '..' while writing filenames */
const unsigned ODIR_SKIP_DOTS    = 0x0003 ; /**< Skip both '.' and '..' */
const unsigned ODIR_CLOSE_DIR    = 0x0004 ; /**< Close directory descriptor on return */

/// @cond
namespace detail {
template<typename OutputIterator>
DIR *listdir(DIR *d, const char *dirname, unsigned flags, OutputIterator &filenames, RaiseError raise)
{
   if (!d)
   {
      PCOMN_CHECK_POSIX(-!!raise, *dirname ? "Cannot open directory '%s'" : "Cannot open directory", dirname) ;
      return NULL ;
   }

   errno = 0 ;
   for (const dirent *entry ; (entry = ::readdir(d)) != NULL ; errno = 0, ++filenames)
      if (!(path::posix::path_dots(entry->d_name) & flags))
         *filenames = entry->d_name ;

   const int err = errno ;
   if (err || (flags & ODIR_CLOSE_DIR))
   {
      ::closedir(d) ;
      errno = err ;
      if (!err)
         return NaP ;

      PCOMN_CHECK_POSIX(-!!raise, *dirname ? "Cannot open directory '%s'" : "Cannot open directory", dirname) ;
      return NULL ;
   }

   return d ;
}

template<typename OutputIterator>
inline DIR *listdir(const char *dirname, unsigned flags, OutputIterator &filenames, RaiseError raise)
{
   return listdir(::opendir(PCOMN_ENSURE_ARG(dirname)), dirname, flags, filenames, raise) ;
}

#ifdef PCOMN_PL_POSIX
template<typename OutputIterator>
inline DIR *listdir(int dirfd, unsigned flags, OutputIterator &filenames, RaiseError raise)
{
   fd_safehandle guard (dirfd) ;
   DIR * const dir = ::fdopendir(dirfd) ;
   if (dir || !(flags & ODIR_CLOSE_DIR))
      guard.release() ;

   return listdir(dir, "", flags, filenames, raise) ;
}

template<typename Dir, typename OutputIterator>
inline int listdirfd(const Dir &dir, unsigned flags, OutputIterator &filenames, RaiseError raise)
{
   if (DIR * const d = listdir(dir, flags &~ ODIR_CLOSE_DIR, filenames, raise))
   {
      const int result_fd = (flags & ODIR_CLOSE_DIR) ? 0 : dup(dirfd(d)) ;
      ::closedir(d) ;
      return result_fd ;
   }
   else
      return -1 ;
}
#endif
}
/// @endcond

/// Open and read a directory.
/// @return Pointer to DIR; if @a raise is DONT_RAISE_ERROR
/// and there happens an error while opening/reading a directory, returns NULL.
///
template<typename OutputIterator>
inline DIR *opendir(const cstrptr &dirname, unsigned flags, OutputIterator filenames, RaiseError raise = DONT_RAISE_ERROR)
{
   return detail::listdir(dirname, flags, filenames, raise) ;
}

/// Open, read, and close a directory.
template<typename OutputIterator>
inline OutputIterator ls(const cstrptr &dirname, unsigned flags, OutputIterator filenames, RaiseError raise = DONT_RAISE_ERROR)
{
   detail::listdir(dirname, flags | ODIR_CLOSE_DIR, filenames, raise) ;
   return filenames ;
}

#ifdef PCOMN_PL_POSIX
/// Open and read a directory.
/// @return A file descriptor of directory @a dirname; if @a raise is DONT_RAISE_ERROR
/// and there happens an error while opening/reading a directory, returns -1.
///
template<typename OutputIterator>
inline int opendirfd(const cstrptr &dirname, unsigned flags, OutputIterator filenames, RaiseError raise = DONT_RAISE_ERROR)
{
   return detail::listdirfd(dirname, flags, filenames, raise) ;
}

template<typename OutputIterator>
inline int opendirfd(int dirfd, unsigned flags, OutputIterator filenames, RaiseError raise = DONT_RAISE_ERROR)
{
   return detail::listdirfd(dirfd, flags, filenames, raise) ;
}

template<typename OutputIterator>
inline DIR *opendir(int dirfd, unsigned flags, OutputIterator filenames, RaiseError raise = DONT_RAISE_ERROR)
{
   return detail::listdir(dirfd, flags, filenames, raise) ;
}

template<typename OutputIterator>
inline OutputIterator ls(int dirfd, unsigned flags, OutputIterator filenames, RaiseError raise = DONT_RAISE_ERROR)
{
   detail::listdir(dirfd, flags | ODIR_CLOSE_DIR, filenames, raise) ;
   return filenames ;
}
#endif

/*******************************************************************************
 filestat
*******************************************************************************/
#define _PCOMN_SYS_FSTAT(statfn, path, raise)      \
   do {                                            \
      fsstat result ;                              \
      const int r = statfn((path), &(result)) ;    \
      if ((raise))                                 \
         PCOMN_ENSURE_POSIX(r, #statfn) ;          \
      else if (r == -1)                            \
         (result) = {} ;                           \
      return result ;                              \
   } while(false)

struct fsstat : stat {
      constexpr fsstat() : stat() {}
      constexpr fsstat(const stat &src) : stat(src) {}
      constexpr explicit operator bool() const { return st_nlink || st_dev || st_ino || st_mode ; }
} ;

inline fsstat filestat(const cstrptr &path, RaiseError raise = DONT_RAISE_ERROR)
{
   _PCOMN_SYS_FSTAT(::stat, path, raise) ;
}

inline fsstat filestat(int fd, RaiseError raise = DONT_RAISE_ERROR)
{
   _PCOMN_SYS_FSTAT(::fstat, fd, raise) ;
}

#ifdef PCOMN_PL_POSIX
inline fsstat linkstat(const cstrptr &path, RaiseError raise = DONT_RAISE_ERROR)
{
   _PCOMN_SYS_FSTAT(::lstat, path, raise) ;
}

inline fsstat linkstat(int dirfd, const cstrptr &name, RaiseError raise = DONT_RAISE_ERROR)
{
   const auto fstatat = [dirfd](const cstrptr &name, struct stat *buf)
   {
      return ::fstatat(dirfd, name, buf, AT_SYMLINK_NOFOLLOW) ;
   } ;
   _PCOMN_SYS_FSTAT(fstatat, name, raise) ;
}
#endif

#undef _PCOMN_SYS_FSTAT

} // end of namespace pcomn::sys
} // end of namespace pcomn

#include PCOMN_PLATFORM_HEADER(pcomn_sys.h)

#endif /* __PCOMN_SYS_H */
