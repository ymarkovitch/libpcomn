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
int futex_wait(int32_t *self, int32_t expected_value, FutexWait flags, struct timespec timeout)
{
   if (!(flags & FutexWait::AbsTime))
      return futex(self, FUTEX_WAIT_PRIVATE, expected_value, &timeout, NULL, 0) ;

   // To wait until the specified point in time (i.e. to absolute time), use
   // FUTEX_WAIT_BITSET operation: FUTEX_WAIT is for _relative_ time (i.e., period).
   const int32_t op = FUTEX_WAIT_BITSET_PRIVATE | flags_if(FUTEX_CLOCK_REALTIME, !!(flags & FutexWait::SystemClock)) ;

   return futex(self, op, expected_value, &timeout, NULL, FUTEX_BITSET_MATCH_ANY) ;
}

} // end of namespace pcomn::sys
} // end of namespace pcomn
