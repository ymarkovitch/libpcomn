/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_fileread.cpp
 COPYRIGHT    :   Yakov Markovitch, 2018. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Read the whole file into the string.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   16 May 2017
*******************************************************************************/
#include <pcomn_fileutils.h>
#include <pcomn_unistd.h>
#include <pcomn_sys.h>
#include <pcomn_handle.h>
#include <pcomn_strslice.h>

#include <stdlib.h>

namespace pcomn {

std::string readfile(int fd)
{
   if (fd < 0)
   {
      errno = EINVAL ;
      PCOMN_THROW_SYSERROR(__func__) ;
   }

   std::string result ;
   const ssize_t sz = sys::filesize(fd) ;
   if (!sz)
      return result ;

   if (sz > 0)
   {
      result.resize(sz) ;
      const ssize_t rsz = PCOMN_ENSURE_POSIX(read(fd, &result[0], sz), "read") ;
      if (rsz == sz)
         return result ;
      result.resize(rsz) ;
   }

   char buf[32*1024] ;

   ssize_t count = 0 ;
   while ((count = read(fd, buf, sizeof buf)) > 0)
      result.append(buf + 0, buf + count) ;
   PCOMN_ENSURE_POSIX(count, "read") ;
   return result ;
}

std::string readfile(const char *filename)
{
   const int fd = open(PCOMN_ENSURE_ARG(filename), O_RDONLY) ;
   PCOMN_CHECK_POSIX(fd, "Cannot open '%s' for reading", filename) ;

   return readfile(fd_safehandle(fd)) ;
}

std::string readfile(const strslice &filename)
{
   return readfile(path::path_buffer(filename).c_str()) ;
}

} // end of namespace pcomn
