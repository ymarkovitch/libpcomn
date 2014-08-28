/*-*- tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   test_scheduler3.cpp
 COPYRIGHT    :   Yakov Markovitch, 2009-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   The test of a trivial scheduler class.

 CREATION DATE:   9 Nov 2009
*******************************************************************************/
#include <pcomn_scheduler.h>
#include <pcomn_timespec.h>
#include <pcomn_fileutils.h>
#include <pcomn_unistd.h>

#include <functional>

#include <stdio.h>

using namespace pcomn ;

static Scheduler::taskid_t stop_id ;

static void worker_fn(const std::string &name, unsigned sleep_usec, Scheduler *sched)
{
   const int sout = fileno(stdout) ;

   pcomn::fdprintf<32>(sout, "%p\n", &sleep_usec) ;

   pcomn::fdprintf<128>(sout, "%s started at %s\n", name.c_str(), time_point::now().string().c_str()) ;
   usleep(sleep_usec) ;
   pcomn::fdprintf<128>(sout, "%s ended at %s\n", name.c_str(), time_point::now().string().c_str()) ;
   if (stop_id && sched)
   {
      pcomn::fdprintf<128>(sout, "%s is self-cancelling at %s\n", name.c_str(), time_point::now().string().c_str()) ;
      sched->cancel(stop_id, true) ;
      pcomn::fdprintf<128>(sout, "%s has been self-canceled at %s\n", name.c_str(), time_point::now().string().c_str()) ;
   }

}

int main(int argc, char *argv[])
{
   DIAG_INITTRACE("pcomn.scheduler.trace.ini") ;

   try
   {
      Scheduler scheduler (0, 4096) ;

      std::cout << "The synchronous scheduler has been created." << std::endl ;

      scheduler.schedule(std::bind(worker_fn, "First", 0, (Scheduler *)NULL), 1000000, 2000000, 0) ;
      const Scheduler::taskid_t Second =
         scheduler.schedule(std::bind(worker_fn, "Second", 100000, &scheduler), 0, 1000000, 0) ;

      std::cout << "All tasks has been sent." << std::endl ;
      char c ;

      std::cerr << "Please hit <AnyKey><ENTER> to cancel 'Second' scheduler." << std::endl ; std::cin >> c ;
      std::cout << "Cancelling 'Second'" << std::endl ;
      stop_id = Second ;

      std::cerr << "Please hit <AnyKey><ENTER> to stop scheduler." << std::endl ; std::cin >> c ;
      std::cout << "Stopping scheduler" << std::endl ;
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
