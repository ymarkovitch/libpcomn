/*-*- tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)) -*-*/
/*******************************************************************************
 FILE         :   perftest_mutex.cpp
 COPYRIGHT    :   Yakov Markovitch, 2009-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   7 Jan 2009
*******************************************************************************/
#include <pcomn_syncobj.h>
#include <pcomn_stopwatch.h>

#include <stdio.h>

int main(int argc, char *argv[])
{
   if (argc != 2)
      return 1 ;

   const unsigned count = atoi(argv[1]) ;

   pcomn::PCpuStopwatch sw ;

   {
      std::mutex nrmutex ;

      sw.start() ;

      for (unsigned i = 0 ; i < count ; ++i)
      {
         nrmutex.lock() ;
         nrmutex.unlock() ;
      }

      sw.stop() ;

      printf("\nNonrecursive mutex: count=%u, %fs, %lu locks/s, size=%u, alignment=%u\n",
             count, sw.elapsed(), (unsigned long)(((double)count)/sw),
             (unsigned)(sizeof nrmutex), (unsigned)std::alignment_of<std::mutex>::value) ;
   }

   {
      std::recursive_mutex rmutex ;

      sw.restart() ;

      for (unsigned i = 0 ; i < count ; ++i)
      {
         rmutex.lock() ;
         rmutex.unlock() ;
      }

      sw.stop() ;

      printf("\nRecursive mutex: count=%u, %fs, %lu locks/s, size=%u, alignment=%u\n",
             count, sw.elapsed(), (unsigned long)(((double)count)/sw),
             (unsigned)(sizeof rmutex), (unsigned)std::alignment_of<std::recursive_mutex>::value) ;

      sw.restart() ;

      for (unsigned i = 0 ; i < count ; ++i)
         rmutex.lock() ;

      sw.stop() ;

      printf("\nRecursive mutex: count=%u, %fs, %lu recursive-acquires/s\n",
             count, sw.elapsed(), (unsigned long)(((double)count)/sw)) ;

      sw.restart() ;

      for (unsigned i = 0 ; i < count ; ++i)
         rmutex.unlock() ;

      sw.stop() ;

      printf("\nRecursive mutex: count=%u, %fs, %lu recursive-releases/s\n",
             count, sw.elapsed(), (unsigned long)(((double)count)/sw)) ;
   }

   {
      pcomn::shared_mutex rwlock ;

      sw.restart() ;

      for (unsigned i = 0 ; i < count ; ++i)
      {
         rwlock.lock_shared() ;
         rwlock.unlock_shared() ;
      }

      sw.stop() ;

      printf("\nReader-writer mutex: count=%u, %fs, %lu rlocks/s, size=%u, alignment=%u\n",
             count, sw.elapsed(), (unsigned long)(((double)count)/sw),
             (unsigned)(sizeof rwlock), (unsigned)std::alignment_of<pcomn::shared_mutex>::value) ;

      sw.restart() ;

      for (unsigned i = 0 ; i < count ; ++i)
         rwlock.lock_shared() ;

      sw.stop() ;

      printf("\nReader-writer mutex: count=%u, %fs, %lu recursive-reader/s\n",
             count, sw.elapsed(), (unsigned long)(((double)count)/sw)) ;

      sw.restart() ;

      for (unsigned i = 0 ; i < count ; ++i)
         rwlock.unlock_shared() ;

      sw.stop() ;

      printf("\nReader-writer mutex: count=%u, %fs, %lu recursive-release/s\n",
             count, sw.elapsed(), (unsigned long)(((double)count)/sw)) ;

      sw.restart() ;

      for (unsigned i = 0 ; i < count ; ++i)
      {
         rwlock.lock() ;
         rwlock.unlock() ;
      }

      sw.stop() ;

      printf("\nReader-writer mutex: count=%u, %fs, %lu wlocks/s\n",
             count, sw.elapsed(), (unsigned long)(((double)count)/sw)) ;

   }

   return 0 ;
}
