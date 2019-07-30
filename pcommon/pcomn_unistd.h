/*-*- mode:c;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel"; c-file-offsets:((inclass . ++)) -*-*/
#ifndef __PCOMN_UNISTD_H
#define __PCOMN_UNISTD_H
/*******************************************************************************
 FILE         :   pcomn_unistd.h
 COPYRIGHT    :   Yakov Markovitch, 1998-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Platform and compiler-independent UNIX I/O functions header.
                  The problem is in UNIX I/O header name, that is io.h
                  in Windows compilers (Microsoft, Borland) and unistd.h in all others.
                  Include this header instead of io.h or unistd.h

 CREATION DATE:   26 Jun 1998
*******************************************************************************/
#include <pcomn_platform.h>

#ifdef PCOMN_PL_WINDOWS
/*******************************************************************************
 PCOMN_PL_WINDOWS
*******************************************************************************/

#  include <io.h>
#  include <fcntl.h>
#  include <share.h>
#  include <sys/stat.h>
#  include <stdlib.h>
#  include <stdio.h>
#  include <process.h>
#  include <direct.h>
#  include <windows.h>

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

#  ifndef O_ACCMODE
#     define O_ACCMODE (O_RDONLY|O_WRONLY|O_RDWR)
#  endif

#  ifndef PATH_MAX
#     define PATH_MAX _MAX_PATH
#  endif

#  ifndef WEXITSTATUS
#     define WEXITSTATUS(status) ((status))
#  endif

#  ifndef WIFEXITED
#     define WIFEXITED(status) ((1))
#  endif

/// ftruncate quasi for Windows.
__inline int ftruncate(int fd, fileoff_t newsize) { return _chsize_s(fd, newsize) ; }

__inline void msleep(unsigned msec) { Sleep(msec) ; }

__inline errno_t setenv(const char *name, const char *value, int overwrite)
{
   return (overwrite || !getenv(name)) ? _putenv_s(name, value ? value : "") : 0 ;
}

__inline errno_t unsetenv(const char *name) { return setenv(name, NULL, 1) ; }

#define rand_r(seedp) ((*(seedp)) = rand())

#define STDERR_FILENO (fileno(stderr))
#define STDOUT_FILENO (fileno(stdout))
#define STDIN_FILENO  (fileno(stdin))

#define fseek_i(stream, offset, whence) (_fseeki64((stream), (offset), (whence)))

#define popen(cmd, mode) (_popen((cmd), (mode)))
#define pclose(stream)   (_pclose((stream)))

#else /* end of PCOMN_PL_WINDOWS */
/*******************************************************************************
 PCOMN_PL_UNIX
*******************************************************************************/

#define fseek_i(stream, offset, whence) (fseek((stream), (offset), (whence)))

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

#ifdef __cplusplus
inline void msleep(unsigned msec) { usleep(msec*1000) ; }
#else
#  define msleep(msec) ((usleep((msec)*1000)))
#endif

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
