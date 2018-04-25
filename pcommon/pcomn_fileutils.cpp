/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_fileutils.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2017. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Assortment of file routines: readfile, etc.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   21 Nov 2008
*******************************************************************************/
#include <pcomn_fileutils.h>
#include <pcomn_unistd.h>

#include <limits>

#include <stdlib.h>

namespace pcomn {

ssize_t readfile(int fd, void *buf, size_t size, void **allocbuf)
{
   if (allocbuf)
      *allocbuf = NULL ;

   if (unlikely(!size))
      return 0 ;

   if (size > std::numeric_limits<long>::max())
   {
      errno = E2BIG ;
      return -1 ;
   }

   char *actual_buf ;

   if (buf)
      actual_buf = (char *)buf ;

   else if (unlikely(!allocbuf))
   {
      errno = EINVAL ;
      return -1 ;
   }
   else if ((*allocbuf = malloc(size + 1)) != NULL)
      actual_buf = (char *)*allocbuf ;

   else
   {
      errno = ENOMEM ;
      return -1 ;
   }

   long bufsize = (long)size ;
   long readcount = 0 ;
   long lastcount = 0 ;

   char *data ;
   do {
      data = actual_buf ;

      while (readcount < bufsize && (lastcount = read(fd, data + readcount, bufsize - readcount)) > 0)
         readcount += lastcount ;

      // The file has been read OK
      if (!lastcount)
         return readcount ;

      if (lastcount < 0)
      {
         if (allocbuf)
         {
            free(*allocbuf) ;
            *allocbuf = NULL ;
         }
         return -1 ;
      }
      else if (!allocbuf)
      {
         errno = E2BIG ;
         return -1 ;
      }
   }
   while ((*allocbuf = actual_buf = (char *)realloc(*allocbuf, (bufsize *= 2) + 1)) != NULL) ;

   free(data) ;
   errno = ENOMEM ;

   return -1 ;
}

} // end of namespace pcomn
