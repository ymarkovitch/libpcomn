/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   test_keyedrwmutex.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for the keyed mutex.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   29 Aug 2008
*******************************************************************************/
#include <pcomn_unittest.h>
#include <pcomn_keyedmutex.h>
#include <pcomn_thread.h>
#include <pcomn_numeric.h>
#include <pcomn_stopwatch.h>
#include <pcomn_unistd.h>

#include <stdlib.h>

static bool stop_test = false ;
static const unsigned keycount = 23 ;
static const int reader_delay_us = 100000 ;
static const int writer_delay_us = 300000 ;

typedef pcomn::PTKeyedRWMutex<unsigned> ReaderWriterLock ;

static void msg(unsigned long long id, unsigned key, const char *name, const char *state)
{
   char buf[512] ;
   snprintf(buf, 512, "%s %18llu %s %u\n", name, id, state, key) ;
   write(fileno(stdout), buf, strlen(buf)) ;
}

class ReaderThread : public pcomn::BasicThread {
      typedef pcomn::BasicThread ancestor ;
   public:
      ReaderThread(ReaderWriterLock &mutex) :
         _mutex(mutex)
      {}

   protected:
      int run()
      {
         char buf[512] ;
         sprintf(buf, "Reader %18llu STARTED\n", (unsigned long long)id()) ;
         write(fileno(stdout), buf, strlen(buf)) ;

         unsigned seed = id() ;
         while (!stop_test)
         {
            const unsigned key = (int)(keycount * (rand_r(&seed) / (RAND_MAX + 1.0))) ;
            const unsigned delay = (int)(reader_delay_us * (rand_r(&seed) / (RAND_MAX + 1.0))) ;

            usleep(delay) ;
            msg(id(), key, "Reader", "->ENTERING") ;
            {
               shared_lock<ReaderWriterLock> reader_lock (_mutex, key) ;

               msg(id(), key, "Reader", "**ENTERED") ;
               usleep(delay) ;
               msg(id(), key, "Reader", "<-EXITING") ;
            }
            msg(id(), key, "Reader", "<>EXITED") ;
         }

         sprintf(buf, "Reader %18llu STOPPING\n", (unsigned long long)id()) ;
         write(fileno(stdout), buf, strlen(buf)) ;
         return 0 ;
      }
   private:
      ReaderWriterLock &_mutex ;
} ;

class WriterThread : public pcomn::BasicThread {
      typedef pcomn::BasicThread ancestor ;
   public:
      WriterThread(ReaderWriterLock &mutex) :
         _mutex(mutex)
      {}

   protected:
      int run()
      {
         char buf[512] ;
         sprintf(buf, "Writer %18llu STARTED\n", (unsigned long long)id()) ;
         write(fileno(stdout), buf, strlen(buf)) ;

         unsigned seed = id() ;
         while (!stop_test)
         {
            const unsigned key = (int)(keycount * (rand_r(&seed) / (RAND_MAX + 1.0))) ;
            const unsigned delay = (int)(writer_delay_us * (rand_r(&seed) / (RAND_MAX + 1.0))) ;

            usleep(delay) ;
            msg(id(), key, "Writer", "->ENTERING") ;
            {
               std::lock_guard<ReaderWriterLock> writer_lock (_mutex, key) ;

               msg(id(), key, "Writer", "**ENTERED") ;
               usleep(delay) ;
               msg(id(), key, "Writer", "<-EXITING") ;
            }
            msg(id(), key, "Writer", "<>EXITED") ;
         }

         sprintf(buf, "Writer %18llu STOPPING\n", (unsigned long long)id()) ;
         write(fileno(stdout), buf, strlen(buf)) ;
         return 0 ;
      }
   private:
      ReaderWriterLock &_mutex ;
} ;

static void usage()
{
    std::cout << "Usage: " << PCOMN_PROGRAM_SHORTNAME << " reader_count [writer_count]" << std::endl ;
    exit(1) ;
}

int main(int argc, char *argv[])
{
   if (!pcomn::inrange(argc, 2, 3))
      usage() ;

   const unsigned reader_count = atoi(argv[1]) ;
   const unsigned writer_count = argc < 3 ? 1 : atoi(argv[2]) ;

   ReaderWriterLock mutex (4, 4) ;

   try {
      pcomn::PTVSafePtr<pcomn::BasicThread *> readers (new pcomn::BasicThread *[reader_count]) ;
      pcomn::PTVSafePtr<pcomn::BasicThread *> writers (new pcomn::BasicThread *[writer_count]) ;

      for (unsigned i = 0 ; i < reader_count ; ++i)
         readers[i] = new ReaderThread(mutex) ;
      for (unsigned i = 0 ; i < writer_count ; ++i)
         writers[i] = new WriterThread(mutex) ;


      std::for_each(readers + 0, readers + reader_count,
                    std::bind2nd(std::mem_fun(&pcomn::BasicThread::start), pcomn::BasicThread::StartRunning)) ;
      std::for_each(writers + 0, writers + writer_count,
                    std::bind2nd(std::mem_fun(&pcomn::BasicThread::start), pcomn::BasicThread::StartRunning)) ;

      std::cerr << "Press any key to stop...\n" ;
      char dummy ;
      std::cin >> dummy ;

      stop_test = true ;

      std::for_each(readers + 0, readers + reader_count, std::mem_fun(&pcomn::BasicThread::join)) ;
      std::for_each(writers + 0, writers + writer_count, std::mem_fun(&pcomn::BasicThread::join)) ;


      std::for_each(readers + 0, readers + reader_count, pcomn::clear_delete<pcomn::BasicThread>) ;
      std::for_each(writers + 0, writers + writer_count, pcomn::clear_delete<pcomn::BasicThread>) ;
   }
   catch (const std::exception &x)
   {
      std::cerr << STDEXCEPTOUT(x) << std::endl ;
      return 1 ;
   }
   return 0 ;
}
