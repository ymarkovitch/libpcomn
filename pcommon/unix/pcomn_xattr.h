/*-*- mode: c++; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_XATTR_H
#define __PCOMN_XATTR_H
/*******************************************************************************
 FILE         :   pcomn_xattr.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2018. All rights reserved.

 DESCRIPTION  :   Linux filesystem extended attributes

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   15 Sep 2008
*******************************************************************************/
/** @file
  Filesystem extended attributes

  At the moment, only Linux is supported; in the future, at least Solaris will be.
*******************************************************************************/
#include <pcomn_string.h>
#include <pcomn_meta.h>

#include <sys/types.h>
#include <attr/xattr.h>
#include <errno.h>
#include <stdlib.h>

namespace pcomn {

/// Hardcoded limit of extended attribute size
const size_t FATTRSIZE_MAX = 8192 ;

enum XAttrSetMode {
   XA_SET      = 0,
   XA_CREATE   = XATTR_CREATE,
   XA_REPLACE  = XATTR_REPLACE
} ;

#define PCOMN_CHECK_XA_SUPPORTED(attrfn, arg)         \
   if (::attrfn((arg), "user.foobar", NULL, 0) < 0)   \
      switch (errno)                                  \
      {                                               \
         case ENOTSUP:                                \
            return false ;                            \
         case ENOATTR:                                \
         case ERANGE:                                 \
            break ;                                   \
         default:                                     \
            PCOMN_THROW_SYSERROR(#attrfn) ;           \
      }                                               \
   return true

/// Check if file extended attributes is supported for given path
template<typename S>
enable_if_strchar_t<S, char, bool>
xattr_supported(const S &filename)
{
   PCOMN_CHECK_XA_SUPPORTED(getxattr, pcomn::str::cstr(filename)) ;
}

/// Check if file extended attributes is supported for given descriptor
inline bool xattr_supported(int fd)
{
   PCOMN_CHECK_XA_SUPPORTED(fgetxattr, fd) ;
}

#undef PCOMN_CHECK_XA_SUPPORTED

template<typename S1, typename S2>
inline std::enable_if_t<is_strchar<S1, char>::value && is_strchar<S2, char>::value, ssize_t>
xattr_get(const S1 &path, const S2 &name, void *value, size_t size)
{
   return ::getxattr(pcomn::str::cstr(path), pcomn::str::cstr(name), value, size) ;
}

template<typename S>
enable_if_strchar_t<S, char, ssize_t>
xattr_get(int fd, const S &name, void *value, size_t size)
{
   return ::fgetxattr(fd, pcomn::str::cstr(name), value, size) ;
}

template<typename S1, typename S2>
std::string xattr_get(const S1 &file, const S2 &name)
{
   // Don't make this function inline to prevent stack from uncontrolled grow
   // The limit is hardcoded
   char buf[FATTRSIZE_MAX] ;
   const ssize_t sz = PCOMN_ENSURE_POSIX(xattr_get(file, name, buf, sizeof buf), "getxattr") ;

   return std::string(buf + 0, buf + sz) ;

}

template<typename S1, typename S2>
std::string xattr_get(const S1 &file, const S2 &name, const std::string &defval)
{
   // Don't make this function inline to prevent stack from uncontrolled grow
   // The limit is hardcoded
   char buf[FATTRSIZE_MAX] ;
   const ssize_t sz = xattr_get(file, name, buf, sizeof buf) ;
   if (sz >= 0)
      return std::string(buf + 0, buf + sz) ;
   if (errno != ENOATTR)
      PCOMN_THROW_SYSERROR("getxattr") ;
   return defval ;
}

template<typename S1, typename S2>
inline ssize_t xattr_size(const S1 &file, const S2 &name)
{
   const ssize_t sz = xattr_get(file, name, NULL, 0) ;
   if (sz < 0 && errno != ENOATTR)
      PCOMN_THROW_SYSERROR("getxattr") ;
   return sz ;
}

template<typename S1, typename S2>
inline bool has_xattr(const S1 &file, const S2 &name)
{
   return xattr_size(file, name) >= 0 ;
}

template<typename S1, typename S2>
inline std::enable_if_t<is_strchar<S1, char>::value && is_strchar<S2, char>::value, bool>
xattr_set(XAttrSetMode mode, const S1 &path, const S2 &name, const void *value, size_t size)
{
   if (::setxattr(pcomn::str::cstr(path), pcomn::str::cstr(name), value, size, mode))
      switch (errno)
      {
         case ENOATTR:
         case EEXIST:
            return false ;
         default:
            PCOMN_THROW_SYSERROR("setxattr") ;
      }
   return true ;
}

template<typename S>
enable_if_strchar_t<S, char, bool>
xattr_set(XAttrSetMode mode, int fd, const S &name, const void *value, size_t size)
{
   if (::fsetxattr(fd, pcomn::str::cstr(name), value, size, mode))
      switch (errno)
      {
         case ENOATTR:
         case EEXIST:
            return false ;
         default:
            PCOMN_THROW_SYSERROR("setxattr") ;
      }
   return true ;
}

template<typename F, typename N, typename V>
inline std::enable_if_t<(is_strchar<N, char>::value && is_strchar<V, char>::value), bool>
xattr_set(XAttrSetMode mode, const F &file, const N &name, const V &value)
{
   return xattr_set(mode, file, name, pcomn::str::cstr(value), pcomn::str::len(value)) ;
}

template<typename S1, typename S2>
inline std::enable_if_t<(is_strchar<S1, char>::value && is_strchar<S2, char>::value), bool>
xattr_del(const S1 &path, const S2 &name)
{
   if (!::removexattr(pcomn::str::cstr(path), pcomn::str::cstr(name)))
      return true ;
   if (errno != ENOATTR)
      PCOMN_THROW_SYSERROR("removexattr") ;
   return false ;
}

template<typename S>
enable_if_strchar_t<S, char, bool>
xattr_del(int fd, const S &name)
{
   if (!::fremovexattr(fd, pcomn::str::cstr(name)))
      return true ;
   if (errno != ENOATTR)
      PCOMN_THROW_SYSERROR("fremovexattr") ;
   return false ;
}

} // end of namespace pcomn


#endif /* __PCOMN_XATTR_H */
