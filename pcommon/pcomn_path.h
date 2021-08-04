/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_PATH_H
#define __PCOMN_PATH_H
/*******************************************************************************
 FILE         :   pcomn_path.h
 COPYRIGHT    :   Yakov Markovitch, 2009-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Filesystem path functions

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   24 Jan 2009
*******************************************************************************/
/** @file
 Filesystem path functions.

    - path_dots:   if the path starts with "." or "..", return 1 or 2
    - is_pathname_separator: is char a pathname separator
    - is_absolute: is the path absolute
    - is_root_of:  is one path is the root of the other
    - split:       split a pathname into a dirname and basename
    - basename:    strip directory from filename
    - dirname:     strip last component from file name
    - splitext:    split filename into dir/basename and extension
    - joinpath:    join the two parts of a path intelligently
    - normpath:    normalize the pathname removing redundant path components
    - abspath:     convert a path to the normalized absolute path
    - realpath:    convert a path to absolute path with resolved symlinks
    - mkdirpath:   ensure the path has final '/'
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
 Path properties detection
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

inline bool is_absolute(const char *path)
{
   NOXCHECK(path) ;
   return is_pathname_separator(*path) ;
}

inline bool is_absolute(const strslice &path)
{
   return path && is_pathname_separator(*path.begin()) ;
}

inline bool is_root_of(const strslice &basedir, const strslice &path)
{
   return basedir
      && str::startswith(path, basedir)
      && (basedir.size() == path.size()
          || is_pathname_separator(path[basedir.size()])
          || is_pathname_separator(path[basedir.size() - 1])) ;
}

/*******************************************************************************
 Splitting and joining paths
*******************************************************************************/
/// Split a pathname into a dirname and basename.
/// @return pair "(dirname, basename)" where "basename" is everything after the final
/// slash. Either part may be empty.
inline unipair<strslice> split(const strslice &path)
{
   if (path.empty())
      return {} ;

   const char *s = path.end() ;
   const char * const b = path.begin() + path_dots(path.begin(), path.end()) ;
   while (s-- != b && !is_pathname_separator(*s)) ;

   return {{path.begin(), s + (s <= b)}, {s + 1, path.end()}} ;
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
inline unipair<strslice> splitext(const strslice &path)
{
   if (const strslice &base = basename(path))
      for (const char *s = base.end() ; --s != base.begin() ;)
         if (*s == '.')
            return {{path.begin(), s}, {s, path.end()}} ;

   return {path, {}} ;
}

/// Join the two parts of a path intelligently and return the string length of result.
size_t joinpath(const strslice &p1, const strslice &p2, char *result, size_t bufsize) ;

/// Indicate if the name is a valid basename, non-path, (i.e., nonempty, does not contain
/// path separators, not equal to "." and does not start with "..").
inline bool is_basename(const strslice &name)
{
   return
      name && !split(name).first && (*name.begin() != '.' || name.size() > 1 && name[1] != '.') ;
}

/*******************************************************************************
 Path normalization
*******************************************************************************/

/// Normalize the pathname removing redundant components such as "foo/../", "./" and
/// trailing "/.".
size_t normpath(const char *name, char *result, size_t bufsize) ;

/// Change a relative path to an absolute path and then normalize it (see normpath()).
size_t abspath(const char *name, char *result, size_t bufsize) ;

/// Convert a path to absolute path with resolved symlinks.
///
/// If @a name is a symlink, follows the link chain and returns the contents of the last
/// link converted with abspath(); otherwise, equivalent of abspath().
ssize_t realpath(const char *name, char *result, size_t bufsize) ;

/*******************************************************************************
*******************************************************************************/

} // end of namespace pcomn::path::posix

#if defined(PCOMN_PL_POSIX)

using posix::split ;
using posix::joinpath ;
using posix::abspath ;
using posix::normpath ;
using posix::realpath ;
using posix::path_dots ;
using posix::is_absolute ;
using posix::is_pathname_separator ;
using posix::is_root_of ;
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

typedef strslice_buffer<PATH_MAX + 1> path_buffer ;

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
   return abspath<R>(path_buffer(path).c_str()) ;
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
   return normpath<R>(path_buffer(path).c_str()) ;
}

/// Convert a path to absolute path with resolved symlinks.
///
/// If @a path is a symlink, follows the link chain and returns the contents of the last
/// link converted with abspath(); otherwise, equivalent of abspath().
/**@{*/
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
   return realpath<R>(path_buffer(path).c_str()) ;
}
/**@}*/

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
/// joinpath("a", "b/c") == "a/b/c" ;
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

template<typename R = std::string>
std::enable_if_t<is_strchar<R, char>::value, R>
joinpath(const strslice &p1, const strslice &p2, const strslice &p3, const strslice &p4, const strslice &p5, const strslice &p6)
{
   char buf1[PATH_MAX + 1] ;
   char buf2[sizeof buf1] ;
   joinpath(p1, p2, buf1, sizeof buf1) ;
   joinpath(p3, p4, buf2, sizeof buf2) ;
   return joinpath<R>(buf1, buf2, p5, p6) ;
}

template<typename R = std::string, typename S>
std::enable_if_t<is_compatible_strings<R, std::remove_reference_t<S>>::value, R>
mkdirpath(S &&path)
{
   return joinpath<R>(std::forward<S>(path), "") ;
}

template<typename R = std::string>
inline std::enable_if_t<is_strchar<R, char>::value, R>
mkdirpath(const strslice &p)
{
   return joinpath<R>(p, "") ;
}

} // end of namespace pcomn::path
} // end of namespace pcomn

#endif /* __PCOMN_PATH_H */
