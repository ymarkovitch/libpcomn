/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   test_threadpool1.cpp
 COPYRIGHT    :   Yakov Markovitch, 2001-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   The test of the thread pool class.
                  This program calculates the CRC32 for every file in the
                  command line.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   18 Jan 2001
*******************************************************************************/
#include <pcomn_threadpool.h>
#include <pcomn_hash.h>
#include <pcomn_unistd.h>

#include <iostream>
#include <vector>
#include <string>

#include <fcntl.h>
#include <stdio.h>

using namespace pcomn ;

class CRCTask : public Task {
   public:
      CRCTask(const std::string &file) :
         _file(file)
      {}
      virtual ~CRCTask()
      {
         TRACEP("Destructing task " << this << " for " << _file) ;
         StreamLock(std::cout).stream() << "Destructing task " << this << " for " << _file << std::endl ;
      }

   protected:
      virtual int run()
      {
         int f = open(_file.c_str(), O_RDONLY) ;
         if (f <= 0)
         {
            StreamLock(std::cout).stream() << this << ": cannot open file '" << _file << '\'' << std::endl ;
            return 0 ;
         }
         TRACEP("The task " << this << " is processing " << _file) ;
         char buf[2048] ;
         int sz ;
         uint32_t crc = 0 ;
         while((sz = read(f, buf, sizeof buf)) > 0)
            crc = calc_crc32(crc, (const byte_t *)buf, sz) ;
         close(f) ;
         snprintf(buf, sizeof buf, "%s:%8.8X\n", _file.c_str(), crc) ;
         write(STDOUT_FILENO, buf, strlen(buf)) ;
         buf[strlen(buf) - 1] = 0 ;
         TRACEP(buf) ;
         return 1 ;
      }

   private:
      std::string _file ;
} ;

class WatchingThread : public BasicThread {
   public:
      WatchingThread(ThreadPool &pool) :
         _pool(pool)
      {}
   protected:
      int run()
      {
         StreamLock(std::cerr).stream() << "Please hit <AnyKey><ENTER> to exit." << std::endl ;
         char c ;
         std::cin >> c ;
         StreamLock(std::cout).stream() << "Stopping pool " << (c == '0' ? "immediately" : "gracefully") << "..." << std::endl ;
         _pool.stop(c == '0' ? 0 : -1) ;
         StreamLock(std::cout).stream() << "Stopped." << std::endl ;
         return 1 ;
      }

   private:
      ThreadPool &_pool ;
} ;

static void usage()
{
   std::cout << "Usage: " << PCOMN_PROGRAM_SHORTNAME << " [-t worker_threads] [-c capacity] file ..." << std::endl ;
   exit(1) ;
}

int main(int argc, char *argv[])
{
   DIAG_INITTRACE("pcomntest.ini") ;

   int initsize =  5 ;
   int capacity = 50 ;
   int c ;

   while ((c = getopt (argc, argv, "t:c:")) != -1)
      switch (c)
      {
         case 't':
            initsize = atoi(optarg) ;
            break;

         case 'c':
            capacity = atoi(optarg) ;
            break;

         case '?':
         case ':':
            usage() ;
      }

   if (optind >= argc)
      usage() ;

   std::vector<std::string> files (argv + optind, argv + argc) ;
   std::sort(files.begin(), files.end()) ;
   std::cout << "Thread pool initsize:" << initsize << std::endl
             << "Thread pool capacity:" << capacity << std::endl
             << files.size() <<  " files to calculate CRC32." << std::endl ;

   PTSafePtr<WatchingThread> watchdog ;
   PTSafePtr<ThreadPool> pool ;
   try
   {
      pool = new ThreadPool(capacity) ;
      StreamLock(std::cout).stream() << "The pool has been created." << std::endl ;
      StreamLock(std::cout).stream() << "Starting pool..." << std::endl ;

      // Start the pool
      pool->start(initsize, 0, BasicThread::PtyBelowNormal) ;

      StreamLock(std::cout).stream() << "Pool has started" << std::endl ;
      watchdog = new WatchingThread(*pool) ;
      watchdog->start() ;
      sleep(2) ;

      for (std::vector<std::string>::const_iterator iter (files.begin()), end (files.end()) ; iter != end ; ++iter)
      {
         StreamLock(std::cout).stream() << "Sending task for " << *iter << " to the thread pool." << std::endl ;
         pool->push(TaskPtr(new CRCTask(*iter))) ;
      }
      StreamLock(std::cout).stream() << "All tasks has been sent." << std::endl ;
   }
   catch(const std::exception &x)
   {
      StreamLock(std::cout).stream() << "Exception: " << x.what() << std::endl ;
      watchdog && watchdog->join() ;
      return 1 ;
   }
   watchdog->join() ;
   return 0 ;
}
