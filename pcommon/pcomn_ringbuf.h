/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_RINGBUF_H
#define __PCOMN_RINGBUF_H
/*******************************************************************************
 FILE         :   pcomn_ringbuf.h
 COPYRIGHT    :   Yakov Markovitch, 2009-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   "Contiguous" circular buffer, based on virtual memory

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   27 Jul 2009
*******************************************************************************/
#include <pcomn_buffer.h>

namespace pcomn {

/******************************************************************************/
/**
*******************************************************************************/
class ContRingBuffer {
   public:
      ContRingBuffer() ;
      explicit ContRingBuffer(size_t size) ;

      memvec_t reserve(size_t size) ;
      void reserve(size_t size) ;

   private:
      size_t _size ;
      char * _start ;
} ;

} // end of namespace pcomn

#endif /* __PCOMN_RINGBUF_H */
