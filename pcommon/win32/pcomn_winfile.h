/*-*- mode: c++; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_WINFILE_H
#define __PCOMN_WINFILE_H
/*******************************************************************************
 FILE         :   pcomn_winfile.h
 COPYRIGHT    :   Yakov Markovitch, 2001-2018. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Windows-specific file classes and utilities

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   15 Jul 2001
*******************************************************************************/
#include <win32/pcomn_w32handle.h>
#include <pcomn_except.h>
#include <stdio.h>

namespace pcomn {

/*******************************************************************************
                     class PWin32TempFile
 Temporary file object.
*******************************************************************************/
class PWin32TempFile {
   public:
      explicit PWin32TempFile(const char *prefix = "~comn", unsigned flags = 0)
      {
         _handle = CreateFile(malloc_ptr<char[]>(tempnam(NULL, const_cast<char *>(prefix))).get(),
                              GENERIC_READ | GENERIC_WRITE,
                              0,
                              NULL,
                              CREATE_NEW,
                              FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE |
                              flags & (FILE_FLAG_WRITE_THROUGH | FILE_FLAG_OVERLAPPED |
                                       FILE_FLAG_SEQUENTIAL_SCAN | FILE_FLAG_RANDOM_ACCESS),
                              NULL) ;
         if (_handle.bad())
            throw system_error(pcomn::system_error::platform_specific) ;
      }

      HANDLE handle() const { return _handle.handle() ; }
      operator HANDLE() const { return handle() ; }

      HANDLE zero() { return _handle.zero() ; }

   private:
      win32_os_handle _handle ;
} ;

} // end of namespace pcomn

#endif /* __PCOMN_WINFILE_H */
