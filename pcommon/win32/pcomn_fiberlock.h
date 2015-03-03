#ifndef __PCOMN_FIBERLOCK_H
#define __PCOMN_FIBERLOCK_H
/*******************************************************************************
 FILE         :   pcomn_fiberlock.h
 COPYRIGHT    :   Yakov Markovitch, 2001-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Base synchronization objects for inter-fiber synchronisation

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   15 Jul 2001
*******************************************************************************/
#include <win32/pcomn_fiber.h>
#include <win32/pcomn_winfile.h>
#include <pcomn_except.h>

#ifdef __BORLANDC__
// Damn! Borland IS right: there is ULONG_PTR referred to in MSDN, not UINT_PTR!!!
typedef ULONG_PTR completion_key_t ;
#else
typedef UINT_PTR completion_key_t ;
#endif

/*******************************************************************************
                     class PFiberPrimitiveLock
 This is a synchronization object to synchronize between sheduled fibers
 pertaining to one I/O completion port.
*******************************************************************************/
class PFiberPrimitiveLock : private PWin32TempFile {
   public:
      struct info : public OVERLAPPED {
            PFiber *suspended ;
      } ;

      PFiberPrimitiveLock(HANDLE completion_port, completion_key_t key) :
         PWin32TempFile("~flck", FILE_FLAG_OVERLAPPED)
      {
         if (!CreateIoCompletionPort(handle(), completion_port, key, 0))
            throw pcomn::system_error(pcomn::system_error::platform_specific) ;
      }

      void lock()
      {
         overlapped_info info ;
         memset(&info, 0, sizeof info) ;
         PFiber *current = PFiber::current_fiber() ;
         info.suspended = current ;
         _check(LockFileEx(handle(), LOCKFILE_EXCLUSIVE_LOCK, 0, 1, 0, &info)) ;
         current->yield() ;
         NOXCHECK(HasOverlappedIoCompleted(&info)) ;
      }

      void unlock()
      {
         NOXVERIFY(UnlockFile(handle(), 0, 0, 1, 0)) ;
      }

      bool try_lock()
      {
         BOOL result = LockFile(handle(), 0, 0, 1, 0) ;
         _check(result || GetLastError() == ERROR_LOCK_VIOLATION) ;
         return !!result ;
      }

   private:
      static void _check(BOOL result)
      {
         if (!result)
            throw pcomn::system_error(pcomn::system_error::platform_specific) ;
      }

      PCOMN_NONCOPYABLE(PFiberPrimitiveLock) ;
      PCOMN_NONASSIGNABLE(PFiberPrimitiveLock) ;
} ;

/*******************************************************************************
                     class PFiberLock
*******************************************************************************/
class PFiberLock : private PFiberPrimitiveLock {
      typedef PFiberPrimitiveLock ancestor ;
   public:
      PFiberLock(HANDLE completion_port, completion_key_t key) :
         ancestor(completion_port, key),
         _acquired(-1),
         _owner(NULL)
      {}

      void lock() { _acquire(true) ; }
      bool try_lock() { return _acquire(false) ; }
      void unlock()
      {
         _owner = NULL ;
         if (InterlockedDecrement(&_acquired) >= 0)
            ancestor::unlock() ; /* Other fibers are waiting, wake one on them up */
      }

   private:
      bool _acquire(bool wait)
      {
         bool result ;
         if (!wait)
            if (InterlockedCompareExchange((PVOID *)&_acquired, (PVOID)0, (PVOID)-1) != (PVOID)-1)
               return false ;
            else
               result = true ;
         else
            /* InterlockedIncrement(&_acquired) == 0 means that no fiber currently owns the lock */
            /* Or else someone already acquired the lock, let's wait... */
            result = !InterlockedIncrement(&_acquired) || ancestor::lock() ;
         _owner = PFiber::current_fiber() ; /* Just for the sake of debugging */
         return ret ;
      }

      long     _acquired ;  /* <0 - free, >=0 - acquired (>0 - number of waiting fibers)  */
      PFiber * _owner ;     /* Fiber which acquired lock. For debug purposes. */
} ;

#endif /* __PCOMN_FIBERLOCK_H */
