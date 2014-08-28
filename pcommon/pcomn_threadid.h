/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_THREADID_H
#define __PCOMN_THREADID_H
/*******************************************************************************
 FILE         :   pcomn_threadid.h
 COPYRIGHT    :   Yakov Markovitch, 1997-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   thread_id implementation

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   19 Feb 1997
*******************************************************************************/
#include <pcomn_def.h>

namespace pcomn {

/******************************************************************************/
/** Thread identifier.
*******************************************************************************/
class _PCOMNEXP thread_id {
   public:
      /// Current thread ID.
      thread_id() ;

      operator unsigned long() const { return _id ; }

      /// Process' main thread ID.
      static thread_id mainThread() ;

      static thread_id null() { return thread_id(0) ; }

   private:
      unsigned long _id ;

      explicit thread_id(unsigned long id) : _id(id) {}
} ;

} // end of namespace pcomn

#endif /* __PCOMN_THREADID_H */
