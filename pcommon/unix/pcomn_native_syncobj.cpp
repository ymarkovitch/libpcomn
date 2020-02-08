/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_native_syncobj.cpp
 COPYRIGHT    :   Yakov Markovitch, 2020. All rights reserved.

 DESCRIPTION  :   Native synchronization objects for Unices

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   8 Feb 2020
*******************************************************************************/
#include "pcomn_native_syncobj.h"
#ifndef PCOMN_PL_LINUX
#error This file supports only Linux.
#endif

namespace pcomn {
namespace sys {

/*******************************************************************************
 Futex
*******************************************************************************/
static int futex_wait_for_period(int32_t *self, int32_t expected_value,
                                 struct timespec &period, bool interruptible)
{
   if (interruptible)
      return futex(self, FUTEX_WAIT_PRIVATE, expected_value, &period, NULL, 0) ;

   // Cannot just call futex(FUTEX_WAIT_PRIVATE) for uninterruptible RelTime wait:
   // must account for restartable wait.
   struct timespec period_end = nsec_to_timespec((std::chrono::steady_clock::now() + timespec_to_nsec(period))
                                                 .time_since_epoch()) ;

   int err = futex(self, FUTEX_WAIT_PRIVATE, expected_value, &period, NULL, 0) ;

   if (err == EINTR)
      // Interrupted, make a recursive call, wait until the end of the period
      err = futex_wait(self, expected_value, FutexWait::SystemClock|FutexWait::AbsTime, period_end) ;

   return err ;
}

int futex_wait(int32_t *self, int32_t expected_value, FutexWait flags, struct timespec timeout)
{
   if (!(flags & FutexWait::AbsTime))
      return futex_wait_for_period(self, expected_value, timeout, !!(flags & FutexWait::Interruptible)) ;

   // To wait until the specified point in time (i.e. to absolute time), use
   // FUTEX_WAIT_BITSET operation: FUTEX_WAIT is for _relative_ time (i.e., period).
   const int32_t op = FUTEX_WAIT_BITSET_PRIVATE | flags_if(FUTEX_CLOCK_REALTIME, !!(flags & FutexWait::SystemClock)) ;

   int err ;
   do err = futex(self, op, expected_value, &timeout, NULL, FUTEX_BITSET_MATCH_ANY) ;

   while (err == EINTR && !(flags & FutexWait::Interruptible)) ;

   return err ;
}

} // end of namespace pcomn::sys
} // end of namespace pcomn
