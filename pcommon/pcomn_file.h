/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_FILE_H
#define __PCOMN_FILE_H
/*******************************************************************************
 FILE         :   pcomn_file.h
 COPYRIGHT    :   Yakov Markovitch, 2001-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Various file objects and utilities

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   5 Feb 2001
*******************************************************************************/
#include <pcomn_handle.h>
#include <pcomn_except.h>
#include <pcomn_safeptr.h>
#include <pcomn_meta.h>

#include <iterator>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>

namespace pcomn {

/******************************************************************************/
/**
*******************************************************************************/
struct auto_unlink {
      auto_unlink(auto_unlink &&other) { swap(other) ; }
      explicit auto_unlink(const char *path) : _path(path && *path ? strdup(path) : NULL) {}

      ~auto_unlink() { path() && unlink(path()) ; }

      const char *path() const { return _path.get() ; }

      void release() { _path.reset() ; }

      void reset()
      {
         if (!path())
         {
            unlink(path()) ;
            release() ;
         }
      }

      void swap(auto_unlink &other) { other._path.swap(_path) ; }

   private:
      std::unique_ptr<char[], malloc_delete> _path ;
} ;

inline void swap(auto_unlink &left, auto_unlink &right) { left.swap(right) ; }

#if PCOMN_PL_UNIX
/******************************************************************************/
/**
*******************************************************************************/
struct auto_unlinkat : auto_unlink {
      auto_unlinkat(int dfd, const char *path) :
         auto_unlink(handle_traits<fd_handle_tag>::is_valid(dfd) ? path : NULL),
         _dirfd(dfd)
      {}

      ~auto_unlinkat() { reset() ; }

      int dirfd() const { return _dirfd ; }

      void reset()
      {
         if (path())
         {
            unlinkat(_dirfd, path(), 0) ;
            release() ;
         }
      }

      void swap(auto_unlinkat &other)
      {
         auto_unlink::swap(other) ;
         std::swap(other._dirfd, _dirfd) ;
      }

   private:
      int _dirfd ;
} ;

inline void swap(auto_unlinkat &left, auto_unlinkat &right) { left.swap(right) ; }

#endif

/******************************************************************************/
/** A platform-independent temporary file object
*******************************************************************************/
class tempfile {
   public:
      tempfile() : _handle(create()) {}
      ~tempfile() = default ;

      int handle() const { return _handle.second.handle() ; }
      operator int() const { return handle() ; }
      const char *path() const { return _handle.first.path() ; }

      int release() { return _handle.second.release() ; }

   private:
      std::pair<auto_unlink, fd_safehandle> _handle ;

      __noinline static std::pair<auto_unlink, fd_safehandle> create()
      {
         static constexpr char name_template[] = "/ptmpXXXXXX" ;
         auto direxists = [](const char *dir)
         {
            struct stat buf ;
            return stat(dir, &buf) == 0 && S_ISDIR(buf.st_mode) ;
         } ;

         const char *d = getenv("TMPDIR") ;
         if ((!d || !direxists(d)) && !direxists(d = "/tmp"))
            throw_exception<system_error>("/tmp does not exist or is not accessible", ENOENT) ;

         std::unique_ptr<char[]> path_template
            (strcat(strcpy(new char[sizeof name_template + strlen(d)], d), name_template)) ;

         fd_safehandle result (mkstemp(path_template.get())) ;
         if (result.bad())
            throw_exception<system_error>() ;
         return { auto_unlink(path_template.get()), std::move(result) } ;
      }
} ;

/******************************************************************************/
/** Autoclosed stdio temporary file
*******************************************************************************/
struct tmpFILE : FILE_safehandle {
      tmpFILE() : FILE_safehandle(tmpfile()) { if (bad()) PCOMN_THROW_SYSERROR("tmpfile") ; }
} ;

/*******************************************************************************
 File iterators
*******************************************************************************/
/// @cond
namespace detail {
template<size_t itemsz>
class FILE_iterator_base : public std::iterator<std::output_iterator_tag, void, void, void, void> {
   protected:
      FILE_iterator_base(FILE *stream = NULL) : _stream(stream) {}
      void put(const void *data) { putdata(data, bool_constant<itemsz == 1>()) ; }
   private:
      FILE *_stream ;

      void putdata(const void *data, std::false_type) { fwrite(data, itemsz, 1, _stream) ; }
      void putdata(const void *data, std::true_type) { fputc(*(const unsigned char *)data, _stream) ; }
} ;
} ;
/// @endcond

/******************************************************************************/
/** Output iterator over stdio FILE
*******************************************************************************/
template<typename V>
class FILE_iterator : public detail::FILE_iterator_base<sizeof(V)> {
      typedef detail::FILE_iterator_base<sizeof(V)> ancestor ;
   public:
      FILE_iterator(FILE *file) : ancestor(PCOMN_ENSURE_ARG(file)) {}
      FILE_iterator() {}

      FILE_iterator &operator=(const V &value)
      {
         this->put(&value) ;
         return *this ;
      }
      FILE_iterator &operator*() { return *this ; }
      FILE_iterator &operator++() { return *this ; }
      FILE_iterator &operator++(int) { return *this ; }
} ;

} // end of namespace pcomn

#endif /* __PCOMN_FILE_H */
