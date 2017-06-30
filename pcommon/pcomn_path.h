/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_PATH_H
#define __PCOMN_PATH_H
/*******************************************************************************
 FILE         :   pcomn_path.h
 COPYRIGHT    :   Yakov Markovitch, 2009-2017. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Filesystem path functions

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   24 Jan 2009
*******************************************************************************/
/** @file
 Filesystem path functions
*******************************************************************************/
#include <pcomn_string.h>
#include <pcomn_strslice.h>
#include <pcomn_unistd.h>

#include <memory>

#include <limits.h>

namespace pcomn {
namespace path {
namespace posix {

/*******************************************************************************
 Filesystem paths
*******************************************************************************/
inline size_t path_dots(const char *path)
{
   NOXCHECK(path) ;
   if (*path != '.')
      return 0 ;
   else if (!*++path || *path == '/')
      return 1 ;
   else if (*path != '.' || *++path && *path != '/')
      return 0 ;
   else
      return 2 ;
}

inline size_t path_dots(const char *begin, const char *end)
{
   NOXCHECK(begin == end || begin && end) ;
   if (begin == end || *begin != '.')
      return 0 ;

   const char *p = begin + 1 ;
   if (p == end || *p == '/')
      return 1 ;
   else if (*p != '.' || ++p != end && *p != '/')
      return 0 ;
   else
      return 2 ;
}

inline bool is_pathname_separator(char c)
{
   return c == '/' ;
}

inline bool is_rooted(const char *path)
{
   NOXCHECK(path) ;
   return *path == '/' ;
}

inline bool is_rooted(const strslice &path)
{
   return path && *path.begin() == '/' ;
}

inline bool is_absolute(const char *path)
{
   return is_rooted(path) ;
}

inline bool is_absolute(const strslice &path)
{
   return is_rooted(path) ;
}

/// Split a pathname into a dirname and basename.
/// @return pair "(dirname, basename)" where "basename" is everything after the final
/// slash. Either part may be empty.
inline strslice_pair split(const strslice &path)
{
   if (path.empty())
      return strslice_pair() ;

   const char *s = path.end() ;
   const char * const b = path.begin() + path_dots(path.begin(), path.end()) ;
   while (s-- != b && !is_pathname_separator(*s)) ;

   return strslice_pair(strslice(path.begin(), s + (s <= b)), strslice(s + 1, path.end())) ;
}

/// Get a pathname with any leading directory components removed.
inline strslice basename(const strslice &path)
{
   return split(path).second ;
}

/// Get a pathname with its last non-slash component and trailing slashes removed.
inline strslice dirname(const strslice &path)
{
   return split(path).first ;
}

/// Split the extension from a pathname.
///
/// Extension is everything from the last dot to the end, ignoring leading dots.
/// @return pair "(root, ext)"; ext may be empty.
/// @note A nonempty returned extension always includes dot (e.g. ".h" for "foo.h")
///
inline strslice_pair splitext(const strslice &path)
{
   if (const strslice &base = basename(path))
      for (const char *s = base.end() ; --s != base.begin() ;)
         if (*s == '.')
            return strslice_pair(strslice(path.begin(), s), strslice(s, path.end())) ;

   return strslice_pair(path, strslice()) ;
}

/// Join the two parts of a path intelligently and return the string length of result.
size_t joinpath(const strslice &p1, const strslice &p2, char *result, size_t bufsize) ;

inline bool is_basename(const strslice &name)
{
   return
      name && !split(name).first && (*name.begin() != '.' || name.size() > 1 && name[1] != '.') ;
}

/// Normalize the pathname removing redundant components such as "foo/../", "./" and
/// trailing "/.".
size_t normpath(const char *name, char *result, size_t bufsize) ;

/// Change a relative path to an absolute path and then normalize it (see normpath()).
size_t abspath(const char *name, char *result, size_t bufsize) ;

/// If @a name is a symlink, follows the link chain and returns the contents of the last
/// link converted with abspath(); otherwise, equivalent of abspath().
ssize_t realpath(const char *name, char *result, size_t bufsize) ;

} // end of namespace pcomn::path::posix

#if defined(PCOMN_PL_POSIX)

using posix::split ;
using posix::joinpath ;
using posix::abspath ;
using posix::normpath ;
using posix::realpath ;
using posix::is_rooted ;
using posix::is_absolute ;
using posix::is_pathname_separator ;
using posix::is_basename ;

#elif defined(PCOMN_PL_MS)

namespace windows {

size_t joinpath(const strslice &p1, const strslice &p2, char *result, size_t bufsize) ;

size_t abspath(const char *name, char *result, size_t bufsize) ;

inline bool is_absolute(const strslice &path)
{
   const char *p = path.begin() ;
   return path.size() >= 3
      && (inrange(*p, 'A', 'Z') || inrange(*p, 'a', 'z'))
      && *++p == ':' && (*++p == '\\' || *p == '/') ;
}

inline bool is_absolute(const char *path)
{
   return path[0] && path[1] && path[2] && is_absolute({path, path + 3}) ;
}
} // end of namespace pcomn::path::windows
using windows::abspath ;
using windows::joinpath ;
#endif

using posix::basename ;
using posix::dirname ;
using posix::splitext ;

/// @overload
template<typename R = std::string, typename S>
std::enable_if_t<is_compatible_strings<R, S>::value, R>
abspath(const S &path)
{
   char buf[PATH_MAX + 1] ;
   return abspath(str::cstr(path), buf, sizeof buf) ? R(buf) : R() ;
}

template<typename R = std::string>
enable_if_strchar_t<R, char, R>
abspath(const basic_strslice<char> &path)
{
   auto_buffer<PATH_MAX + 1> buf (path.size() + 1) ;
   return abspath<R>(strslicecpy(buf.get(), path, path.size() + 1)) ;
}

/// @overload
template<typename R = std::string, typename S>
std::enable_if_t<is_compatible_strings<R, S>::value, R>
normpath(const S &path)
{
   char buf[PATH_MAX + 1] ;
   return normpath(str::cstr(path), buf, sizeof buf) ? R(buf) : R() ;
}

template<typename R = std::string>
enable_if_strchar_t<R, char, R>
normpath(const basic_strslice<char> &path)
{
   auto_buffer<PATH_MAX + 1> buf (path.size() + 1) ;
   return normpath<R>(strslicecpy(buf.get(), path, path.size() + 1)) ;
}

/// @overload
template<typename R = std::string, typename S>
std::enable_if_t<is_compatible_strings<R, S>::value, R>
realpath(const S &path)
{
   char buf[PATH_MAX + 1] ;
   return realpath(str::cstr(path), buf, sizeof buf) > 0 ? R(buf) : R() ;
}

template<typename R = std::string>
enable_if_strchar_t<R, char, R>
realpath(const basic_strslice<char> &path)
{
   auto_buffer<PATH_MAX + 1> buf (path.size() + 1) ;
   return realpath<R>(strslicecpy(buf.get(), path, path.size() + 1)) ;
}

/// Join one or more path components intelligently.
///
/// The return value is the concatenation of _nonempty_ parameters with exactly _one_
/// directory separator following each _nonempty_ part except the last, meaning that the
/// result will only end in a separator if there are nonempty arguments and the last
/// argument is empty.
/// @note If a parameter is an absolute path, all previous parameters are thrown away and
/// joining continues from the absolute path parameter.
/// @code
/// joinpath("", "") == "" ;
/// joinpath(".", "") == "./" ;
/// joinpath("", "a", "", "b") == "a/b" ;
/// joinpath("", "a/", "", "b") == "a/b" ;
/// joinpath("", "a", "/", "b") == "/b" ;
/// joinpath("a", "", "c", "") == "a/c/" ;
/// @endcode
///
template<typename R = std::string>
std::enable_if_t<is_strchar<R, char>::value, R>
joinpath(const strslice &p1, const strslice &p2)
{
   char buf[PATH_MAX + 2] ;
   if (const size_t length = joinpath(p1, p2, buf, sizeof buf - 1))
   {
      if (!p2 && buf[length - 1] != PCOMN_PATH_NATIVE_DELIM)
      {
         buf[length] = PCOMN_PATH_NATIVE_DELIM ;
         buf[length + 1] = 0 ;
      }
      return R(buf) ;
   }
   return R() ;
}

/// @overload
template<typename R = std::string>
std::enable_if_t<is_strchar<R, char>::value, R>
joinpath(const strslice &p1, const strslice &p2, const strslice &p3)
{
   char buf1[PATH_MAX + 1] ;
   joinpath(p1, p2, buf1, sizeof buf1) ;
   return joinpath<R>(buf1, p3) ;
}

/// @overload
template<typename R = std::string>
std::enable_if_t<is_strchar<R, char>::value, R>
joinpath(const strslice &p1, const strslice &p2, const strslice &p3, const strslice &p4)
{
   char buf1[PATH_MAX + 1] ;
   char buf2[sizeof buf1] ;
   joinpath(p1, p2, buf1, sizeof buf1) ;
   joinpath(buf1, p3, buf2, sizeof buf2) ;
   return joinpath<R>(buf2, p4) ;
}

/// @overload
template<typename R = std::string>
std::enable_if_t<is_strchar<R, char>::value, R>
joinpath(const strslice &p1, const strslice &p2, const strslice &p3, const strslice &p4, const strslice &p5)
{
   char buf1[PATH_MAX + 1] ;
   joinpath(p1, p2, buf1, sizeof buf1) ;
   return joinpath<R>(buf1, p3, p4, p5) ;
}

template<typename R = std::string, typename S>
std::enable_if_t<is_compatible_strings<R, std::remove_reference_t<S>>::value, R>
mkdirpath(S &&path)
{
   static constexpr const char separator[2] = {PCOMN_PATH_NATIVE_DELIM, 0} ;

   return (!*str::cstr(std::forward<S>(path))
           || is_pathname_separator(*(str::cstr(std::forward<S>(path)) + (str::len(std::forward<S>(path)) - 1))))
      ? R(std::forward<S>(path))
      : R(std::forward<S>(path)) + separator ;
}

template<typename R = std::string>
inline std::enable_if_t<is_strchar<R, char>::value, R>
mkdirpath(const strslice &p)
{
   return mkdirpath(R(p)) ;
}

} // end of namespace pcomn::path
} // end of namespace pcomn

#endif /* __PCOMN_PATH_H */
