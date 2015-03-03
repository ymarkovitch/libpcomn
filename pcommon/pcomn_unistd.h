/*-*- mode:c;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel"; c-file-offsets:((inclass . ++)) -*-*/
#ifndef __PCOMN_UNISTD_H
#define __PCOMN_UNISTD_H
/*******************************************************************************
 FILE         :   pcomn_unistd.h
 COPYRIGHT    :   Yakov Markovitch, 1998-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Platform and compiler-independent UNIX I/O functions header.
                  The problem is in UNIX I/O header name, that is io.h
                  in Windows compilers (Microsoft, Borland) and unistd.h in all others.
                  Include this header instead of io.h or unistd.h

 CREATION DATE:   26 Jun 1998
*******************************************************************************/
#ifndef __PCOMN_PLATFORM_H
#include <pcomn_platform.h>
#endif /* PCOMN_PLATFORM.H */

#ifdef PCOMN_PL_WINDOWS
/* PCOMN_PL_WINDOWS */

#  include <io.h>
#  include <fcntl.h>
#  include <share.h>
#  include <sys/stat.h>
#  include <stdlib.h>

#  ifndef O_SHARE_RDONLY
#     define O_SHARE_RDONLY   SH_DENYWR
#  endif

#  ifndef O_SHARE_WRONLY
#     define O_SHARE_WRONLY   SH_DENYRD
#  endif

#  ifndef O_SHARE_RDWR
#     define O_SHARE_RDWR     SH_DENYNO
#  endif

#  ifndef O_SHARE_NONE
#     define O_SHARE_NONE     SH_DENYRW
#  endif

#  ifndef S_IRUSR
#     define S_IRUSR _S_IREAD
#  endif

#  ifndef S_IWUSR
#     define S_IWUSR _S_IWRITE
#  endif

#  ifndef S_IRGRP
#     define S_IRGRP _S_IREAD
#  endif

#  ifndef S_IWGRP
#     define S_IWGRP _S_IWRITE
#  endif

#  ifndef S_IROTH
#     define S_IROTH _S_IREAD
#  endif

#  ifndef S_IWOTH
#     define S_IWOTH _S_IWRITE
#  endif

#ifdef __cplusplus
/// ftruncate quasi for Windows.
inline int ftruncate(int fd, long newsize) { return _chsize(fd, newsize) ; }
#else
#  define ftruncate(file, size) (_chsize((file), (size)))
#endif


/* MS doesn't define ssize_t */
#ifdef PCOMN_COMPILER_MS
#ifdef PCOMN_PL_WIN64
typedef int64_t ssize_t ;
#else
typedef int ssize_t ;
#endif
#endif

#else /* end of PCOMN_PL_WINDOWS */
/* PCOMN_PL_UNIX */

#  include <unistd.h>
#  include <fcntl.h>
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <sys/uio.h>

#  ifndef O_TEXT
#     define O_TEXT 0x00
#  endif

#  ifndef O_BINARY
#     define O_BINARY 0x00
#  endif

#  ifndef O_SHARE_RDONLY
#     define O_SHARE_RDONLY 0x00
#  endif

#  ifndef O_SHARE_WRONLY
#     define O_SHARE_WRONLY 0x00
#  endif

#  ifndef O_SHARE_RDWR
#     define O_SHARE_RDWR 0x00
#  endif

#  ifndef O_SHARE_NONE
#     define O_SHARE_NONE 0x00
#  endif

/*
   Access rights
*/

#  ifndef S_IREAD
#     define S_IREAD S_IRUSR
#  endif

#  ifndef S_IWRITE
#     define S_IWRITE S_IWUSR
#  endif


#endif /* end of PCOMN_PL_UNIX */

#ifndef SH_COMPAT
#  define SH_COMPAT      O_SHARE_RDWR    /* compatibility mode */
#endif

#ifndef SH_DENYRW
#  define SH_DENYRW      O_SHARE_NONE    /* deny read/write mode */
#endif

#ifndef SH_DENYWR
#  define SH_DENYWR      O_SHARE_RDONLY    /* deny write mode */
#endif

#ifndef SH_DENYRD
#  define SH_DENYRD      O_SHARE_WRONLY    /* deny read mode */
#endif

#ifndef SH_DENYNO
#  define SH_DENYNO      O_SHARE_RDWR     /* deny none mode */
#endif

#ifndef SH_DENYNONE
#  define SH_DENYNONE    SH_DENYNO
#endif

#ifndef O_BINARY
#  define O_BINARY       0x00
#endif

#include "platform/dirent.h"

#endif // __PCOMN_UNISTD_H
