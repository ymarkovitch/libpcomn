/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_ATOMIC_CC
#define __PCOMN_ATOMIC_CC
/*******************************************************************************
 FILE         :   pcomn_atomic.cc
 COPYRIGHT    :   Yakov Markovitch, 2000-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Atomic operations support for Win32

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   22 Nov 2000
*******************************************************************************/
#include <windows.h>
#include <pcomn_def.h>

namespace pcomn {

namespace atomic_op {

inline atomic_t inc(atomic_t *value)
{
   return InterlockedIncrement(value) ;
}

inline atomic_t dec(atomic_t *value)
{
   return InterlockedDecrement(value) ;
}

inline atomic_t get(atomic_t *value)
{
   return InterlockedExchangeAdd(value, 0) ;
}

inline atomic_t xchg(atomic_t *value, atomic_t new_value)
{
   return InterlockedExchange(value, new_value) ;
}

} // end of namespace pcomn::atomic_op
} // end of namespace pcomn

#endif /* __PCOMN_ATOMIC_CC */
