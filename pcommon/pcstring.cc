/*-*- mode:c;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((inclass . ++)) -*-*/
#ifndef __PCSTRING_CC
#define __PCSTRING_CC
/*******************************************************************************
 FILE         :   pcstring.cc
 COPYRIGHT    :   Yakov Markovitch, 1998-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Implementation of memory and string functions from pcstring.h

 CREATION DATE:   24 Jul 1998
*******************************************************************************/
#include <ctype.h>
#include <pcomn_platform.h>

/*
   Turning off boring Borland's messages about inlining and
   signed/unsigned digits
*/
#ifdef __BORLANDC__
#  pragma option -w-inl -w-csu -w-sig
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PCOMN_NO_STRCASE_CONV

__inline char *strlwr(char *s)
{
   if (s)
   {
      char *s1 ;
      for (s1 = s ; *s1 ; ++s1)
         *s1 = tolower((unsigned char)(*s1)) ;
   }
   return s ;
}

__inline char *strupr(char *s)
{
   if (s)
   {
      char *s1 ;
      for (s1 = s ; *s1 ; ++s1)
         *s1 = toupper((unsigned char)(*s1)) ;
   }
   return s ;
}

#endif

#ifndef PCOMN_PL_WINDOWS
__inline int memicmp(const void *lhs, const void *rhs, size_t len)
{
   const char *lhc = (const char *)lhs ;
   const char *rhc = (const char *)rhs ;
   const char *lhe = lhc + len ;
   int result = 0 ;
   while (lhc != lhe && (result = (int)(unsigned char)toupper(*lhc) - (int)(unsigned char)toupper(*rhc)) == 0)
      ++lhc, ++rhc ;
   return result ;
}
#endif

__inline void *memupr(void *s, size_t cnt)
{
   char * str ;
   for(str = (char *)s ; cnt-- ; ++str)
      *str = toupper(*str) ;
   return s ;
}

__inline void *memlwr(void *s, size_t cnt)
{
   char * str ;
   for(str = (char *)s ; cnt-- ; ++str)
      *str = tolower(*str) ;
   return s ;
}

__inline void *memuprcpy(void *dest, const void *src, size_t cnt)
{
   size_t cs ;
   for(cs = 0 ; cs < cnt ; ++cs)
      ((char *)dest)[cs] = toupper(((const char *)src)[cs]) ;
   return dest ;
}

__inline void *memlwrcpy(void *dest, const void *src, size_t cnt)
{
   size_t cs ;
   for(cs = 0 ; cs < cnt ; ++cs)
      ((char *)dest)[cs] = tolower(((const char *)src)[cs]) ;
   return dest ;
}

__inline void *memnotchr (const void *mem, char c, size_t sz)
{
   const char *s ;
   for (s = (const char *)mem ; sz && *s == c ; --sz, ++s) ;
   return sz ? (void *)s : NULL ;
}

__inline char *strnotchr (const char *s, char c)
{
   while (*s && *s == c)
      ++s ;

   return *s != c ? (char *)s : NULL ;
}

__inline char *strrnotchr(const char *s, int c)
{

   char *r = NULL ;
   int k ;
   while((k = *s) != 0)
      if (k != c)
         r = (char *)s ;
   return r ;
}

__inline char *strncpyz(char *dest, const char *src, size_t bufsz)
{
   size_t zndx = bufsz ;
   if (zndx--)
   {
      const char * const end = (const char *)memchr(src, 0, zndx) ;
      if (end)
         zndx = end - src ;
      memcpy(dest, src, zndx) ;
      dest[zndx] = 0 ;
   }
   return dest ;
}

__inline void *memstripcpy(void *dest, const void *src, int c, size_t cnt)
{
   size_t len = 0 ;

   for(size_t count = 0 ; count < cnt ;)
      if (((const char *)src)[count++] != c)
         len = count ;

   return (char *)memcpy(dest, src, len) + len ;
}

__inline char *strstripcpy(char *dest, const char *src, int c)
{
   const char *e = strrnotchr(src, c) ;
   if (e)
   {
      size_t sz = e - src + 1 ;
      *((char *)memmove(dest, src, sz) + sz) = 0 ;
   }
   else
      *dest = 0 ;

   return dest ;
}

__inline size_t strntrimlen(const char *s, int c, size_t cnt)
{
   size_t count ;
   size_t len = 0 ;

   for(count = 0 ; *s && count++ < cnt ;)
      if (*s++ != c)
         len = count ;

   return len ;
}

__inline char *strnstripcpyz(char *dest, const char *src, int c, size_t bufsz)
{
   if (bufsz--)
   {
      memmove(dest, src, bufsz = strntrimlen(src, c, bufsz)) ;
      dest[bufsz] = 0 ;
   }
   return dest ;
}

__inline char *strnstripcpyzp(char *dest, const char *src, int c, size_t bufsz)
{
   if (bufsz)
   {
      size_t len = strntrimlen(src, c, bufsz-1) ;
      memset ((char *)memmove(dest, src, len), 0, bufsz-len) ;
   }

   return dest ;
}

__inline char *strncpyp(char *dest, const char *src, int pad, size_t bufsz)
{
   if (bufsz)
   {
      size_t sc = 0 ;
      char c ;
      for (c = *src ; c ; c = src[sc])
      {
         dest[sc] = c ;
         if (++sc >= bufsz)
            return dest ;
      }
      while(sc < bufsz)
         dest[sc++] = pad ;
   }
   return dest ;
}

__inline char * strchrreplace(char *dest, const char *src, int cfrom, int cto)
{
   int c ;
   char *d ;

   if (cfrom == cto)
      return strcpy(dest, src) ;
   for (d = dest ; (c = *src) != 0 ; ++src)
      *d++ = c == cfrom ? cto : c ;
   *d = 0 ;
   return dest ;
}

#ifdef __cplusplus
}
#endif

#ifdef __BORLANDC__
#  pragma option -w.inl -w.csu -w.sig
#endif

#endif /* __PCSTRING_CC */
