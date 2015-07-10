/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_path.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Pathname functions for POSIX

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   18 Nov 2008
*******************************************************************************/
#include <pcomn_path.h>
#include <pcomn_alloca.h>
#include <pcomn_unistd.h>
#include <pcstring.h>

#ifndef PCOMN_PL_POSIX
#define readlink(x, y, z) (((void)(x), (void)(y), (void)(z), (-1)))
#endif

namespace pcomn {
namespace path {
namespace posix {

/*******************************************************************************
 Filesystem
*******************************************************************************/
size_t abspath(const char *name, char *result, size_t bufsize)
{
   if (!name || !*name || !result && bufsize)
      return 0 ;

   if (is_absolute(name))
      return normpath(name, result, bufsize) ;

   const size_t namelen = strlen(name) ;
   const size_t path_max = PATH_MAX + namelen + 1 ;
   if (path_max > 2*PATH_MAX)
      return 0 ;

   char *pathbuf = P_ALLOCA(char, path_max) ;

   if (!getcwd(pathbuf, path_max))
      return 0 ;
   const size_t cwdsz = strlen(pathbuf) ;

   if (namelen == 1 && *name == '.')
      if (cwdsz >= bufsize)
         return 0 ;
      else
      {
         memcpy(result, pathbuf, cwdsz) ;
         result[cwdsz] = 0 ;
         return cwdsz ;
      }

   pathbuf[cwdsz] = '/' ;
   strcpy(pathbuf + cwdsz + 1, name) ;

   return normpath(pathbuf, result, bufsize) ;
}

size_t joinpath(const strslice &p1, const strslice &p2, char *result, size_t bufsize)
{
   if (!result || !bufsize)
      return 0 ;
   if (const strslice *sp = !p2 ? &p1 : (*p2.begin() == '/' || !p1 ? &p2 : NULL))
   {
      const size_t sz = sp->size() ;
      if (sz >= bufsize)
         return 0 ;
      memmove(result, sp->begin(), sz) ;
      result[sz] = 0 ;
      return sz ;
   }

   size_t p1sz = p1.size() ;
   const size_t p2sz = p2.size() ;
   const size_t fullsz = p1sz + p2sz + (*(p1.end() - 1) != '/') ;

   if (fullsz >= bufsize)
      return 0 ;

   memmove(result, p1.begin(), p1sz) ;
   if (p1sz + p2sz != fullsz)
      result[p1sz++] = '/' ;
   memmove(result + p1sz, p2.begin(), p2sz) ;
   result[fullsz] = 0 ;

   return fullsz ;
}

size_t normpath(const char *name, char *result, size_t bufsize)
{
   if (!name || !*name || !result && bufsize)
      return 0 ;

   const size_t path_max = PATH_MAX ;
   char *pathbuf_begin = P_ALLOCA(char, path_max + 1) ;

   char *dest = pathbuf_begin ;

   if (is_absolute(name))
      *dest++ = '/' ;
   *dest = 0 ;

   const char * const pathbuf_end = pathbuf_begin + path_max ;

   // Skip multiple path-separators
   for (const char *cbegin = name, *cend ; *(cbegin = strnotchr(cbegin, '/')) ; cbegin = cend)
   {
      // Find the end of a component
      for (cend = cbegin ; *cend && *cend != '/' ; ++cend) ;

      switch (path_dots(cbegin))
      {
         // ".."
         case 2:
            switch (dest - pathbuf_begin)
            {
               case 0:
                  *dest++ = '.' ; *dest++ = '.' ; break ;

               case 1:
                  switch (*pathbuf_begin)
                  {
                     case '/': break ;
                     case '.': *dest++ = '.' ; break ;
                     default:  *pathbuf_begin = '.' ; break ;
                  }
                  break ;

               case 2:
                  switch (pathbuf_begin[0])
                  {
                     case '/': --dest ; break ;

                     case '.':
                        if (pathbuf_begin[1] == '.')
                        {
                           *dest++ = '/' ; *dest++ = '.' ; *dest++ = '.' ;
                           break ;
                        }
                     default:
                        *(dest = pathbuf_begin)++ = '.' ; break ;
                  }
                  break ;

               default:
                  while(dest > pathbuf_begin && *--dest != '/') ;
                  if (dest == pathbuf_begin)
                  {
                     if (*dest != '/')
                        *dest = '.' ;
                     ++dest ;
                  }
                  break ;
            }
            break ;

         case 1:
            if (dest == pathbuf_begin) *dest++ = '.' ; break ;

         case 0:
         {
            const ptrdiff_t csize = cend - cbegin ;

            if (dest > pathbuf_begin + 1)
               *dest++ = '/' ;
            else if (dest == pathbuf_begin + 1)
               if (*pathbuf_begin == '.')
                  dest = pathbuf_begin ;
               else if (*pathbuf_begin != '/')
                  *dest++ = '/' ;

            if (dest + csize >= pathbuf_end)
               return 0 ;

            memcpy(dest, cbegin, csize) ;
            dest += csize ;
         }
      }
   }
   *dest = '\0' ;

   const size_t size = dest - pathbuf_begin ;

   if (size < bufsize)
      memcpy(result, pathbuf_begin, size + 1) ;

   return size ;
}

static const size_t rpath_max = PATH_MAX + 1 ;
static const int linkdepth_max = 32 ;

ssize_t realpath(const char *name, char *result, size_t bufsize)
{
   if (!name || !*name || !result && bufsize)
      return 0 ;

   struct local {
         static char *follow_links(const char *name, char *pathbuf1, char *pathbuf2, int depth)
         {
            const size_t psz = abspath(name, pathbuf1, rpath_max) ;
            if (psz > rpath_max)
            {
               // Since there cannot exist actual paths longer than PATH_MAX, there is no sense in
               // following links
               errno = ENAMETOOLONG ;
               return NULL ;
            }
            const ssize_t lsz = readlink(pathbuf1, pathbuf2, rpath_max) ;
            if (lsz <= 0)
               return pathbuf1 ;
            else if (depth >= linkdepth_max)
            {
               errno = ELOOP ;
               return NULL ;
            }
            joinpath(split(pcomn::strslice(pathbuf1, pathbuf1+psz)).first,
                     pcomn::strslice(pathbuf2, pathbuf2+lsz), pathbuf1,
                     rpath_max*2) ;
            return
               follow_links(pathbuf1, pathbuf2, pathbuf1, ++depth) ;
         }
   } ;

   char *pathbuf1 = P_ALLOCA(char, rpath_max*2) ;
   char *pathbuf2 = P_ALLOCA(char, rpath_max*2) ;
   int depth = 0 ;
   char *rpath = local::follow_links(name, pathbuf1, pathbuf2, depth) ;
   if (!rpath)
      return -1 ;
   const size_t rsz = strlen(rpath) ;
   if (rsz < bufsize)
      memcpy(result, rpath, rsz + 1) ;
   return rsz ;
}

} // end of namespace pcomn::path::posix

/*******************************************************************************
 Win32 paths
*******************************************************************************/
#ifdef PCOMN_PL_MS
namespace windows {
static inline char *normalize_separators(char *path_begin, char *path_end)
{
   std::replace(path_begin,
                path_end = std::unique(path_begin, path_end,
                                       [](char x, char y) { return y == '/' && (x == '/' || x == '\\') ; }),
                '/', '\\') ;
   return path_end ;
}

size_t abspath(const char *name, char *result, size_t bufsize)
{
   if (!name || !*name || !result && bufsize)
      return 0 ;

   const size_t nmsz = strlen(name) ;
   auto_buffer<PATH_MAX + 1> normalized (nmsz + 1) ;
   *normalize_separators(strcpy(normalized.get(), name), normalized.get() + nmsz) = 0 ;

   return _fullpath(result, normalized.get(), bufsize) ? strlen(result) : 0 ;
}
} // end of namespace pcomn::path::windows
#endif
/*******************************************************************************
 End of win32 paths
*******************************************************************************/

} // end of namespace pcomn::path
} // end of namespace pcomn
