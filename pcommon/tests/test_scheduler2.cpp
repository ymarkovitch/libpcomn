/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   test_scheduler2.cpp
 COPYRIGHT    :   Yakov Markovitch, 2009-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   The test of an asynchronous scheduler class.

 CREATION DATE:   11 Jun 2009
*******************************************************************************/
#include <pcomn_scheduler.h>
#include <pcomn_timespec.h>
#include <pcomn_unistd.h>

#include <functional>

#include <stdio.h>

using namespace pcomn ;

static bool stop_all ;

static void worker_fn(const std::string &name, unsigned sleep_usec)
{
   const int sout = fileno(stdout) ;

   const std::string &start_text = name + " started at " + time_point::now().string() + "\n" ;
   ::write(sout, start_text.c_str(), start_text.size()) ;
   usleep(sleep_usec) ;
   const std::string &end_text = name + " ended at " + time_point::now().string() + "\n" ;
   ::write(sout, end_text.c_str(), end_text.size()) ;
}

int main(int argc, char *argv[])
{
   DIAG_INITTRACE("pcomn.scheduler.trace.ini") ;

   try
   {
      AsyncScheduler scheduler (0, 128*1024) ;

      std::cout << "The asynchronous scheduler has been created." << std::endl ;

      scheduler.schedule(std::bind(worker_fn, "First", 500000), 1000000, 2000000, 0) ;
      scheduler.schedule(std::bind(worker_fn, "Second", 200000), 0, 1000000, 0) ;

      std::cout << "All tasks has been sent." << std::endl ;
      std::cerr << "Please hit <AnyKey><ENTER> to stop scheduler." << std::endl ;
      char c ;
      std::cin >> c ;
      std::cout << "Stopping scheduler" << std::endl ;
      stop_all = true ;
   }
   catch(const std::exception &x)
   {
      std::cout << "Exception: " << x.what() << std::endl ;
      return 1 ;
   }
   std::cout << "Stopped" << std::endl ;

   std::cerr << "Please hit <AnyKey><ENTER> to exit." << std::endl ;
   char c ;
   std::cin >> c ;
   std::cout << "Finished" << std::endl ;

   return 0 ;
}
