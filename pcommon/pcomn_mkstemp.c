/*-*- tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_mkstemp.c
 COPYRIGHT    :   Yakov Markovitch, 2010-2015. All rights reserved.

 DESCRIPTION  :   Securely create a unique temporary file.
                  Inspired by Cornelius Krasel version, completely rewritten

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   12 Oct 2010
*******************************************************************************/
#include <pcomn_unistd.h>

#include <sys/stat.h>

#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>

static const int SCNT = 6 ;

int pcomn_mkstemp(char *tpl, unsigned flags, unsigned mode)
{
   /* Possible substitution chars (36) */
   static const char schar[] = "abcdefghijklmnopqrstuvwxyz0123456789" ;
   char *subst ;
   int scnt ;

   if (!tpl)
      return EINVAL ;

   subst = tpl + strlen(tpl) ;
   for (scnt = 0 ; scnt < SCNT && subst-- != tpl && *subst == 'X' ; ++scnt) ;
   if (scnt < SCNT)
      /* Follow mkstemp() description: the last 6 characters _must_ be 'X' */
      return EINVAL ;

   do {
      /* Substitute random characters for the last 6 locations */
      for (char *s = subst, *e = subst + SCNT ; s != e ; ++s)
         *s = schar[(sizeof use) * rand_r() / (RAND_MAX + 1)] ;

      int fd = open(tpl, O_RDWR|O_EXCL|O_CREAT, 0600) ;
      if (fd >= 0)
         /* Success */
         return fd ;
   }
   while (errno == EEXIST) ;

   /* Failure */
   return errno ;
}
