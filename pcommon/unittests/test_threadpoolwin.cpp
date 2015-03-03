/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)) -*-*/
/*******************************************************************************
 FILE         :   test_threadpoolwin.cpp
 COPYRIGHT    :   Yakov Markovitch, 2001-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   The test of the thread pool class.
                  This program calculates the CRC32 for every file in the
                  given directory.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   18 Jan 2001
*******************************************************************************/
#include <pcomn_threadpool.h>
#include <pcomn_hash.h>
#include <pcomn_unistd.h>

#include <iostream>
#include <list>
#include <string>

#include <fcntl.h>
#include <stdio.h>

template<bool dummy = true>
struct PTStreamLock {
      PTStreamLock(std::ostream &os) :
         _ostream(os),
         _guard(_lock)
      {}

      std::ostream &stream() { return _ostream ; }

   private:
      std::ostream &                _ostream ;
      std::lock_guard<std::recursive_mutex> _guard ;

   static std::recursive_mutex _lock ;
} ;

template<bool dummy>
std::recursive_mutex PTStreamLock<dummy>::_lock ;

typedef PTStreamLock<> StreamLock ;

template<class Container>
Container &directory(const std::string &dir, Container &container)
{
   _finddata_t find_data ;
   long h = _findfirst((char *)(dir + "\\*.*").c_str(), &find_data) ;
   if (h >= 0)
      try {
         for(int result = 0 ; !result ; result = _findnext(h, &find_data))
            if (!(find_data.attrib & (_A_HIDDEN | _A_SUBDIR)))
               container.push_back((char *)find_data.name) ;
         _findclose(h) ;
      }
      catch(...)
      {
         _findclose(h) ;
         throw ;
      }
   return container ;
}

class CRCTask : public PTask {
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
         // There is a special "bad" case: if the last character in the file
         // name is 'b', dereference NULL pointer. This is to demonstrate the
         // self-recovery feature of a thread pool.
         const char *ptr = NULL ;
         //if (*(_file.end()-1) != 'b')
            ptr = _file.c_str() ;
         _file[0] = *ptr ;

         int f = open(_file.c_str(), O_BINARY | O_RDONLY) ;
         if (f <= 0)
         {
            StreamLock(std::cout).stream() << this << ": cannot open file '" << _file << '\'' << std::endl ;
            return 0 ;
         }
         TRACEP("The task " << this << " is processing " << _file) ;
         char buf[2048] ;
         int sz ;
         uint32 crc = 0 ;
         while((sz = read(f, buf, sizeof buf)) > 0)
            crc = calc_crc32(crc, (const byte_t *)buf, sz) ;
         close(f) ;
         sprintf(buf, "%08.8X", crc) ;
         StreamLock(std::cout).stream() << _file << ':' << buf << std::endl ;
         TRACEP(_file << ':' << buf) ;
         return 1 ;
      }

   private:
      std::string _file ;
} ;

class WatchingThread : public PBaseThread {
   public:
      WatchingThread(ThreadPool &pool) :
         _pool(pool)
      {}
   protected:
      int run()
      {
         StreamLock(std::cout).stream() << "Please hit <AnyKey><ENTER> to exit." << std::endl ;
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

static void usage(const char *progname)
{
   const char *name = strrchr(progname, '\\') ;
   progname = name ? name + 1 : progname ;
   std::cout << "Usage: " << progname << " [-t worker_threads] [-c capacity] [directory]" << std::endl ;
}

void main(int argc, char *argv[])
{
   DIAG_INITTRACE("pcomntest.ini") ;

   std::string dir (".") ;
   int initsize =  5 ;
   int capacity = 50 ;
   int c ;

   while ((c = getopt (argc, argv, "?t:c:")) != -1)
      switch (c)
      {
         case 't':
            initsize = atoi(optarg) ;
            break;

         case 'c':
            capacity = atoi(optarg) ;
            break;

         case '?':
            usage(*argv) ;

         default:
            return ;
      }

   if (optind < argc)
      dir = argv[optind] ;

   std::cout << "dir=" << dir << "; initsize=" << initsize << "; " << "; capacity=" << capacity << std::endl ;
   PTClassPtr<WatchingThread> watch ;
   PTClassPtr<ThreadPool> pool ;
   try
   {
      pool = new ThreadPool(capacity) ;
      StreamLock(std::cout).stream() << "The pool has been created." << std::endl ;
      StreamLock(std::cout).stream() << "Starting pool..." << std::endl ;

      // Start the pool
      pool->start(initsize, PBaseThread::PtyBelowNormal) ;

      StreamLock(std::cout).stream() << "Pool has started" << std::endl ;
      watch = new WatchingThread(*pool) ;
      watch->start() ;
      Sleep(2000) ;

      std::list<std::string> dirlist ;
      directory(dir, dirlist) ;
      StreamLock(std::cout).stream() << dirlist.size() << " files in " << dir << std::endl ;
      dirlist.sort() ;
      std::list<std::string>::const_iterator iter = dirlist.begin() ;
      for (int sz = dirlist.size() ; sz-- ; ++iter)
      {
         StreamLock(std::cout).stream() << "Pushing " << *iter << std::endl ;
         pool->push(PTaskP (new CRCTask(dir + "\\" + *iter))) ;
      }
      StreamLock(std::cout).stream() << "All tasks has been sent." << std::endl ;
   }
   catch(const std::exception &x)
   {
      StreamLock(std::cout).stream() << "Exception: " << x.what() << std::endl ;
   }
   watch && watch->wait_for_result() ;
}
