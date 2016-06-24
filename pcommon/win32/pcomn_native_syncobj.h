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
#include <pcomn_unistd.h>

#ifndef PCOMN_PL_WINDOWS
#error This header supports only Windows
#endif

#include "pcomn_sys.h"
#include "pcomn_w32util.h"

#include <windows.h>
#include <limits.h>

namespace pcomn {
namespace sys {

/******************************************************************************/
/** Simple binary Dijkstra semaphore; nonrecursive mutex that allow both self-locking
 and unlocking by another thread (not only by the thread that had acquired the lock).
*******************************************************************************/
class NativeThreadLock final {
      PCOMN_NONCOPYABLE(NativeThreadLock) ;
      PCOMN_NONASSIGNABLE(NativeThreadLock) ;

   public:
      constexpr NativeThreadLock() : _lock(SRWLOCK_INIT) {}

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

#define PCOMN_HAS_NATIVE_RWMUTEX 1
/******************************************************************************/
/** Read-write mutex on  POSIX Threads.
*******************************************************************************/
class native_rw_mutex {
      PCOMN_NONCOPYABLE(native_rw_mutex) ;
      PCOMN_NONASSIGNABLE(native_rw_mutex) ;
   public:
      constexpr native_rw_mutex() : _lock(SRWLOCK_INIT) {}

      void lock() { AcquireSRWLockExclusive(&_lock) ; }
      bool try_lock() { return TryAcquireSRWLockExclusive(&_lock) ; }
      bool unlock()
      {
         ReleaseSRWLockExclusive(&_lock) ;
         return true ;
      }

      void lock_shared()     { AcquireSRWLockShared(&_lock) ; }
      bool try_lock_shared() { return TryAcquireSRWLockShared(&_lock) ; }
      bool unlock_shared()
      {
         ReleaseSRWLockShared(&_lock) ;
         return true ;
      }

   private:
      SRWLOCK _lock ;
} ;

/******************************************************************************/
/** File lock; provides read-write mutex logic
*******************************************************************************/
class native_file_mutex {
      PCOMN_NONASSIGNABLE(native_file_mutex) ;
   public:
      native_file_mutex(int fd, bool owned) :
         _fh(get_oshandle(fd)),
         _fd(owned ? fd : (fd | ((unsigned)INT_MAX + 1)))
      {
         _locksz = 0 ;
      }

      explicit native_file_mutex(const char *filename, int flags = O_CREAT|O_RDONLY, int mode = 0600) :
         native_file_mutex(openfile(PCOMN_ENSURE_ARG(filename), flags, mode), true)
      {}

      explicit native_file_mutex(const std::string &filename, int flags = O_CREAT|O_RDONLY, int mode = 0600) :
         native_file_mutex(filename.c_str(), flags, mode)
      {}

      ~native_file_mutex()
      {
         NOXVERIFY(!owned() || close(fd()) >= 0) ;
      }

      int fd() const { return _fd & ~((unsigned)INT_MAX + 1) ; }
      bool owned() const { return !(_fd & ((unsigned)INT_MAX + 1)) ; }

      void lock() { NOXVERIFY(acquire_lock(false, true)) ; }
      bool try_lock() { return acquire_lock(true, true) ; }

      bool unlock()
      {
         NOXCHECK(_locksz) ;

         // Release must never _ever_ throw exceptions, it is very likely be called
         // from destructors.
         OVERLAPPED dummy = OVERLAPPED() ;
         const bool result = UnlockFileEx(_fh, 0, _locksz_lh[0], _locksz_lh[1], &dummy) ;
         if (result)
            _locksz = 0 ;
         NOXCHECKX(result, sys_error_text(GetLastError()).c_str()) ;
         return result ;
      }

      void lock_shared() { NOXVERIFY(acquire_lock(false, false)) ; }
      bool try_lock_shared() { return acquire_lock(true, false) ; }
      bool unlock_shared() { return unlock() ; }

   private:
      const HANDLE   _fh ;
      const int      _fd ;
      union {
            DWORD _locksz_lh[2] ;
            DWORD64 _locksz ;
      } ;

   private:
      static HANDLE get_oshandle(int fd)
      {
         PCOMN_ASSERT_ARG(fd >= 0) ;
         const HANDLE native_file_handle = (HANDLE)_get_osfhandle(fd) ;
         PCOMN_ASSERT_ARG(native_file_handle != INVALID_HANDLE_VALUE) ;
         return native_file_handle ;
      }

      bool acquire_lock(bool nonblocking, bool exclusive)
      {
         const fileoff_t sz = std::max<fileoff_t>(PCOMN_ENSURE_POSIX(sys::filesize(fd()), "filesize"), 1) ;
         OVERLAPPED dummy = OVERLAPPED() ;
         const DWORD flags =
            (-(int)nonblocking &  LOCKFILE_FAIL_IMMEDIATELY) |
            (-(int)exclusive   &  LOCKFILE_EXCLUSIVE_LOCK) ;

         DWORD size_lower = (DWORD)sz ;
         DWORD size_upper = ((DWORD64)sz >> 32) & 0x0FFFFFFFF ;

         if (LockFileEx(_fh, flags, 0, size_lower, size_upper, &dummy))
         {
            _locksz_lh[0] = size_lower ;
            _locksz_lh[1] = size_upper ;
            return true ;
         }

         ensure<pcomn::system_error>(nonblocking && GetLastError() == ERROR_LOCK_VIOLATION,
                                     system_error::platform_specific) ;

         return false ;
      }

      static int openfile(const char *name, int flags, int mode)
      {
         const int d = ::open(name, flags, mode) ;
         PCOMN_CHECK_POSIX(d, "native_file_mutex cannot open '%s' for locking", name) ;
         return d ;
      }
} ;

} // end of namespace pcomn::sys
} // end of namespace pcomn

#endif /* __PCOMN_NATIVE_SYNCOBJ_H */
