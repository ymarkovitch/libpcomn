/*-*- mode: c; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((inclass . ++)) -*-*/
#ifndef __PCSTRING_H
#define __PCSTRING_H
/*******************************************************************************
 FILE         :   pcstring.h
 COPYRIGHT    :   Yakov Markovitch, 1998-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Memory- and string-processing functions, unhappily missed in
                  string.h

 CREATION DATE:   24 Jul 1998
*******************************************************************************/
#include <pcomn_platform.h>
#include <string.h>

#if defined(PCOMN_COMPILER_MS)
#define strnicmp(lhs, rhs, count) (_strnicmp((lhs), (rhs), (count)))
#define stricmp(lhs, rhs) (_stricmp((lhs), (rhs)))
#define strtok_r(str, delim, context) (strtok_s((str), (delim), (context)))
#elif defined(PCOMN_COMPILER_GNU)
#define strnicmp(lhs, rhs, count) (strncasecmp((lhs), (rhs), (count)))
#define stricmp(lhs, rhs) (strcasecmp((lhs), (rhs)))
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PCOMN_NO_LOCALE_CONV

   char *_lstrlwr(char *s) ;
   char *_lstrupr(char *s) ;
   int _ltolower(int ch) ;
   int _ltoupper(int ch) ;

#endif


#ifdef PCOMN_NO_STRCASE_CONV

   __inline char *strlwr(char *s) ;
   __inline char *strupr(char *s) ;

#endif

/*
** memnotchr   -  searches memory region for the first occurrence of a character that is
**                NOT equal to given.
** Parameters:
**    mem   -  beginning of the memory region
**    c     -  given character
**    sz    -  memory region length
**
** Returns: a pointer to the first character that is not equal to c. If there is no such
**          character, memnotchr returns NULL.
*/
__inline void *   memnotchr (const void *mem, char c, size_t sz) ;


__inline void *   memstripcpy(void *dest, const void *src, int c, size_t cnt) ;

__inline void *memupr(void *s, size_t cnt) ;
__inline void *memlwr(void *s, size_t cnt) ;
__inline void *memuprcpy(void *dest, const void *src, size_t cnt) ;
__inline void *memlwrcpy(void *dest, const void *src, size_t cnt) ;

#ifndef PCOMN_PL_WINDOWS
__inline int memicmp(const void *lhs, const void *rhs, size_t len) ;
#endif

/*
** strnotchr   -  searches the string for the first occurrence of a character that is
**                NOT equal to given.
** Parameters:
**    s     -  string
**    c     -  given character
**
** Returns: a pointer to the first character that is not equal to c. If there is no such
**          character, strnotchr returns NULL. The null-terminator is considered to be NOT
**          part of the string.
*/
__inline char *   strnotchr (const char *s, char c) ;

/*
** strnotchr   -  searches the string for the LAST occurrence of a character that is
**                NOT equal to given.
**
** Parameters:
**    s     -  string
**    c     -  given character
**
** Returns: a pointer to the LAST character that is not equal to c. If there is no such
**          character, strrnotchr returns NULL.
*/
__inline char *   strrnotchr(const char *s, int c) ;

__inline char *   strncpyz(char *dest, const char *src, size_t bufsz) ;

__inline size_t   strntrimlen(const char *s, int c, size_t cnt) ;
__inline char *   strstripcpy(char *dest, const char *src, int c) ;
__inline char *   strnstripcpyz(char *dest, const char *src, int c, size_t bufsz) ;
__inline char *   strnstripcpyzp(char *dest, const char *src, int c, size_t bufsz) ;

/*
*/
__inline char *   strncpyp(char *dest, const char *src, int pad, size_t bufsz) ;

/*
** strchrreplace  -  searches and raplaces all occurences of given character in given string.
**
** Parameters:
**    dest  -  destination string
**    src   -  source string
**    cfrom -  source character
**    cto   -  replacement character
**
** Returns: a pointer to the destination string.
**          dest and src can be the same.
**          If dest and src overlap, (src < dest) results are unpredictable
*/
__inline char *   strchrreplace(char *dest, const char *src, int cfrom, int cto) ;

char *strlwrcpy(char *targ, const char *src) ;
char *struprcpy(char *targ, const char *src) ;

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus
//
//  The next functions are C++ - only
//

/// Copy string into an array allocated by the operator new.
///
/// Allocates memory for a copy of string by calling the operator new.
/// @param src Pointer to the source string
/// @return A pointer to the newly allocated memory containing the copied string; if
/// @a src is NULL, strnew returns NULL.
///
inline char *strnew(const char *src)
{
   return src ?
      strcpy(new char[strlen(src)+1], src) :
      NULL ;
}

// strnewz() - copy string into the array allocated by the operator new
//
// strnewz allocates memory for a copy of string by calling operator new.
// If source char pointer is NULL, returns newly allocated empty string
// Parameters:
//    src   -  pointer to source string
// Return Value
//    strnew returns a pointer to the newly allocated memory containing the
//    copied string or empty string if source string is NULL.
//
inline char *strnewz(const char *src)
{
   if (!src)
      src = "" ;
   return strcpy(new char[strlen(src)+1], src) ;
}

#endif


#if !defined(__PCSTRING_CC)
#include <pcstring.cc>
#endif /* PCSTRING.CC */


#endif /* __PCSTRING_H */
