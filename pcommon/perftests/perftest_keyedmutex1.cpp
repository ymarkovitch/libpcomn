/*-*- tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)) -*-*/
/*******************************************************************************
 FILE         :   perftest_keyedmutex1.cpp
 COPYRIGHT    :   Yakov Markovitch, 2009-2017. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   7 Jan 2009
*******************************************************************************/
#include <pcomn_keyedmutex.h>
#include <pcomn_numeric.h>
#include <pcomn_stopwatch.h>

#include <stdlib.h>

const unsigned NKeys = 50 ;

int main(int argc, char *argv[])
{
   if (argc != 2)
      return 1 ;

   const unsigned count = atoi(argv[1]) ;

   unsigned slots1[NKeys] ;
   unsigned slots2[NKeys] ;

   pcomn::iota(slots1 + 0, slots1 + NKeys, 1) ;
   pcomn::iota(slots2 + 0, slots2 + NKeys, 1) ;
   std::random_shuffle(slots1 + 0, slots1 + NKeys) ;
   std::random_shuffle(slots2 + 0, slots2 + NKeys) ;

   pcomn::PTKeyedMutex<unsigned> kmi (4, 4) ;

   pcomn::PCpuStopwatch sw ;
   sw.start() ;

   for (unsigned i = 0 ; i < count ; ++i)
      for (unsigned n = 0 ; n < NKeys ; ++n)
      {
         kmi.lock(slots1[n]) ;
         kmi.unlock(slots1[n]) ;
      }

   sw.stop() ;

   printf("\nCount=%u, %u key(s), %fs, %lu keys/s\n",
          count, NKeys, sw.elapsed(), (unsigned long)(((double)count * NKeys)/sw)) ;

   return 0 ;
}
