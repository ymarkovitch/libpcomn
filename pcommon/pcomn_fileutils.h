/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_FILEUTILS_H
#define __PCOMN_FILEUTILS_H
/*******************************************************************************
 FILE         :   pcomn_fileutils.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Assortment of file routines: readfile, etc.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   21 Nov 2008
*******************************************************************************/
/** @file
 Assortment of file routines: readfile, etc.
*******************************************************************************/
#include <pcommon.h>
#include <pcomn_unistd.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

namespace pcomn {

/// Read a whole file into a memory buffer.
_PCOMNEXP ssize_t readfile(int fd, void *buf, size_t size, void **allocbuf) ;

template<unsigned short n>
int fdprintf(int fd, const char *format, ...) PCOMN_ATTR_PRINTF(2, 3) ;

template<unsigned short n>
int fdprintf(int fd, const char *format, ...)
{
   char buf[n] ;

   va_list parm ;
   va_start(parm, format) ;
   vsnprintf(buf + 0, n, format, parm) ;
   va_end(parm) ;

   return ::write(fd, buf, strlen(buf)) ;
}

} // end of namespace pcomn

#endif /* __PCOMN_FILEUTILS_H */
