/*-*- mode: c++; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_NATIVE_SYNCOBJ_H
#define __PCOMN_NATIVE_SYNCOBJ_H
/*******************************************************************************
 FILE         :   pcomn_native_syncobj.h
 COPYRIGHT    :   Yakov Markovitch, 2015. All rights reserved.

 DESCRIPTION  :   Native synchronization objects for Windows

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   26 Jun 2015
*******************************************************************************/
#include <pcommon.h>
#include <pcomn_assert.h>
#include <pcomn_except.h>
#include <pcomn_meta.h>
#include <algorithm>
#ifndef PCOMN_PL_WINDOWS
#error This header supports only Windows
#endif

#include "pcomn_sys.h"

#include <windows.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>

namespace pcomn {

/******************************************************************************/
/** Simple binary Dijkstra semaphore; nonrecursive mutex that allow both self-locking
 and unlocking by another thread (not only by the thread that had acquired the lock).
*******************************************************************************/
class NativeThreadLock final {
      PCOMN_NONCOPYABLE(NativeThreadLock) ;
      PCOMN_NONASSIGNABLE(NativeThreadLock) ;

   public:
      constexpr NativeThreadLock() : _lock(SRWLOCK_INIT) {}

      ~NativeThreadLock() { ReleaseSRWLockExclusive(&_lock) ; }

      void lock() { AcquireSRWLockExclusive(&_lock) ; }

      bool try_lock() { return TryAcquireSRWLockExclusive(&_lock) ; }

      bool unlock()
      {
         ReleaseSRWLockExclusive(&_lock) ;
         return true ;
      }
   private:
      SRWLOCK _lock ;
} ;

typedef NativeThreadLock NativeNonrecursiveMutex ;

/******************************************************************************/
/** File lock; provides read-write mutex logic
*******************************************************************************/
class NativeFileMutex {
      PCOMN_NONASSIGNABLE(NativeFileMutex) ;
   public:
      NativeFileMutex(int fd, bool owned) :
         _fh(get_oshandle(fd)),
         _fd(owned ? fd : (fd | ((unsigned)INT_MAX + 1))),
      {}

      explicit NativeFileMutex(const char *filename, int flags = O_CREAT|O_RDONLY, int mode = 0600) :
         NativeFileMutex(PCOMN_ENSURE_ARG(filename), true)
      {}

      explicit NativeFileMutex(const std::string &filename, int flags = O_CREAT|O_RDONLY, int mode = 0600) :
         NativeFileMutex(filename.c_str(), flags, mode)
      {}

      NativeFileMutex(const NativeFileMutex &other, int flags) :
         _fd(reopenfile(other.fd(), flags))
      {}

      ~NativeFileMutex()
      {
         NOXVERIFY(!owned() || close(fd()) >= 0) ;
      }

      int fd() const { return _fd & ~((unsigned)INT_MAX + 1) ; }
      bool owned() const { return !(_fd & ((unsigned)INT_MAX + 1)) ; }

      void lock() { NOXVERIFY(acquire_lock(LOCK_EX)) ; }
      bool try_lock() { return acquire_lock(LOCK_EX|LOCK_NB) ; }
      bool unlock()
      {
         // Release should never _ever_ throw exceptions, it is very likely be called
         // from destructors.
         int err ;
         while((err = posix_errno(flock(fd(), LOCK_UN))) == EINTR) ;
         NOXCHECKX(!err, strerror(err)) ;
         return !err ;
      }

      void lock_shared() { NOXVERIFY(acquire_lock(LOCK_SH)) ; }
      bool try_lock_shared() { return acquire_lock(LOCK_SH|LOCK_NB) ; }
      bool unlock_shared() { return unlock() ; }

   private:
      const HANDLE   _fh ;
      const int      _fd ;

      static HANDLE get_oshandle(int fd)
      {
         PCOMN_ASSERT_ARG(fd >= 0) ;
         const HANDLE native_file_handle = (HANDLE)_get_osfhandle(fd) ;
         PCOMN_ASSERT_ARG(native_file_handle != INVALID_HANDLE_VALUE) ;
         return native_file_handle ;
      }

      bool acquire_lock(bool nonblocking, bool exclusive) const
      {
         DWORD size_lower, size_upper;

         const off_t sz = std::max<off_t>(PCOMN_ENSURE_POSIX(filesize(fd()), "filesize"), 1) ;
         OVERLAPPED dummy = OVERLAPPED() ;
         const DWORD flags =
            (-(int)nonblocking &  LOCKFILE_FAIL_IMMEDIATELY) |
            (-(int)exclusive   &  LOCKFILE_EXCLUSIVE_LOCK) ;

         if (LockFileEx(_fh, flags, 0, , , &dummy))
            return true ;

         const DWORD err = GetLastError() ;
         if (err == ERROR_LOCK_VIOLATION && nonblocking)
            return false ;
      }

      static int openfile(const char *name, int flags, int mode)
      {
         const int d = ::open(name, flags, mode) ;
         PCOMN_CHECK_POSIX(d, "NativeFileMutex cannot open '%s' for locking", name) ;
         return d ;
      }
} ;

} // end of namespace pcomn

#endif /* __PCOMN_NATIVE_SYNCOBJ_H */
