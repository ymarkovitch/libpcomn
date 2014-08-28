/*-*- mode: c++; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_TSS_H
#define __PCOMN_TSS_H
/*******************************************************************************
 FILE         :   pcomn_tss.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Win32 thread-specific storage.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   10 Feb 2008
*******************************************************************************/

namespace pcomn {
/*******************************************************************************
                     class PTSSBase
 The base class for Thread Local Storage. Implements base logic of the
 platform-independent TSS. Factors out code that would bloat derived
 (templatized) version of TSS.
*******************************************************************************/
class PTSSBase {
   public:
      // Constructor.
      PTSSBase() :
         _id(TlsAlloc())
      {
         NOXCHECK(_id >= 0) ;
      }

      ~PTSSBase()
      {
         if (_id >= 0)
            TlsFree(_id) ;
      }

      void *get_value() const { return TlsGetValue(_id) ; }

      void set_value(void *value) const { TlsSetValue(_id, value) ; }

   private:
      long _id ;
} ;
} // end of namespace pcomn

#endif /* __PCOMN_TSS_H */
