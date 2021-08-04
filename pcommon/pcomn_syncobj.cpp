/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_syncobj.cpp
 COPYRIGHT    :   Yakov Markovitch, 2020. All rights reserved.

 DESCRIPTION  :   Synchronisation primitives

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   8 Feb 2020
*******************************************************************************/
#include "pcomn_syncobj.h"

namespace pcomn {

using namespace std::chrono ;
using namespace sys ;

/*******************************************************************************
 promise_lock
*******************************************************************************/
void promise_lock::wait()
{
   int32_t prev_value = 1 ;
   // NOTE: no transition from _locked==0, i.e. once 0 is forever 0.
   if (_locked.compare_exchange_strong(prev_value, 2, std::memory_order_acq_rel) /* 1->2 */
       || prev_value /* already was 2: !=1 (or else CAS would succeed) and !=0 */)
   {
      int err ;
      while ((err = posix_errno(futex_wait(&_locked, 2))) == EINTR) ;
      if (err != EAGAIN)
         PCOMN_ENSURE_ENOERR(err, "FUTEX_WAIT") ;
      // The only possible transition from 2 is to 0 (unlocked)
   }
}

bool promise_lock::wait_with_timeout(TimeoutMode mode, std::chrono::nanoseconds timeout)
{
   if (mode == TimeoutMode::None)
   {
      wait() ;
      return true ;
   }

   int32_t prev_value = 1 ;

   if (!_locked.compare_exchange_strong(prev_value, 2, std::memory_order_acq_rel) &&
       (!prev_value || timeout == nanoseconds()))
   {
      // Either unlocked and success or the timeout is 0 and failure
      return !prev_value ;
   }

   // Always use absolute timeout to compensate for EINTR possibility.
   const FutexWait wait_mode = FutexWait::AbsTime |
      ((mode == TimeoutMode::SteadyClock) ? FutexWait::SteadyClock : FutexWait::SystemClock) ;

   // Convert relative timeout to absolute: calculate the end of timeout period.
   struct timespec timeout_point = timeout_timespec(mode, timeout) ;

   int err ;
   // Wait actually
   while ((err = posix_errno(futex_wait_with_timeout(&_locked, 2, wait_mode, timeout_point))) == EINTR) ;

   const bool timed_out = err == ETIMEDOUT ;

   // err == EAGAIN means _locked is not more equal 2 (while has been before),
   // and the only possible transition from 2 is to 0 (unlocked).
   timed_out || err == EAGAIN || PCOMN_ENSURE_ENOERR(err, "FUTEX_WAIT") ;

   return !timed_out ;
}

void promise_lock::unlock()
{
   int32_t locked = _locked.load(std::memory_order_acquire) ;

   if (!locked ||
       locked == 1 && _locked.compare_exchange_strong(locked, 0, std::memory_order_acq_rel) ||
       !locked)
      return ;

   NOXCHECK(locked == 2) ;

   if (_locked.compare_exchange_strong(locked, 0, std::memory_order_acq_rel))
      futex_wake_all(&_locked) ;
}

} // end of namespace pcomn
