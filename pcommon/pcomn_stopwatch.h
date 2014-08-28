/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_STOPWATCH_H
#define __PCOMN_STOPWATCH_H
/*******************************************************************************
 FILE         :   pcomn_stopwatch.h
 COPYRIGHT    :   Yakov Markovitch, 1998-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Stopwatches - classes for both CPU and real time counting for different
                  OS platforms.

 CREATION DATE:   23 Feb 1998
*******************************************************************************/
#include <pcommon.h>
#include <time.h>

#if   defined(PCOMN_PL_WIN32)
#  include <windows.h>
#elif defined(PCOMN_PL_OS400)
#  include <ledate.h>
#elif defined(PCOMN_PL_UNIX)
#  include <sys/time.h>
#endif

namespace pcomn {

/*******************************************************************************
                     class PStopwatch
*******************************************************************************/
/** The base (abstract) class for both CPU and physical time stopwatches.
*******************************************************************************/
class PStopwatch {
   public:
      /// Get time resolution (ticks per seconds)
      double resolution() const { return _resolution ; }

      /// Get the time (in seconds) counted off by the stopwatch.
      double elapsed() const
      {
         return !is_running()
            ? _elapsed
            : _elapsed + (current() - _start) / resolution() ;
      }

      /// The same as elapsed()
      operator double() const { return elapsed() ; }

      /// Check whether the stopwatch is ticking: true, if ticking, false, if stopped.
      bool is_running() const { return _running ; }

      /// Start the stopwatch; if it is already ticking, no-op.
      double start()
      {
         const double curr = current() ;
         if (!is_running())
            _running = true ;
         else
            _elapsed += (curr - _start) / resolution() ;
         _start = curr ;
         return _elapsed ;
      }

      /// Stop the stopwatch and return the elapsed time.
      double stop()
      {
         _elapsed = elapsed() ;
         _running = false ;
         return _elapsed ;
      }

      /// Stop the stopwatch, get the elapsed time and reset the stopwatch to 0
      double reset()
      {
         const double old = stop() ;
         _elapsed = 0 ;
         return old ;
      }

      /// Stop the stopwatch, get the elapsed time, reset to 0 and start again
      double restart()
      {
         double old = reset() ;
         start() ;
         return old ;
      }

   protected:
      PStopwatch(double res) :
         _resolution (res),
         _start(0),
         _elapsed(0),
         _running(false)
      {}

      virtual double current() const = 0 ;

   private:
      double   _resolution ;  /* Resolution (tick per second) */
      double   _start ;       /* The initial tick count */
      double   _elapsed ;     /* Elapsed time (in ticks) */
      bool     _running ;     /* True, if ticking */
} ;

/*******************************************************************************
                     class PRealStopwatch
*******************************************************************************/
/** A stopwatch for counting periods ot the real (physical) time.
*******************************************************************************/
class PRealStopwatch : public PStopwatch {
      typedef PStopwatch ancestor ;
   public:
      PRealStopwatch() ;

   protected:
      virtual double current() const ;
} ;

/*******************************************************************************
                     class PCpuStopwatch
*******************************************************************************/
/** A stopwatch for counting CPU time, i.e. time that a process spends on CPU
*******************************************************************************/
class PCpuStopwatch : public PStopwatch {
      typedef PStopwatch ancestor ;
   public:
      PCpuStopwatch() ;

   protected:
      virtual double current() const ;
} ;

/*******************************************************************************
 PRealStopwatch
*******************************************************************************/
inline PRealStopwatch::PRealStopwatch() :
#if defined(PCOMN_PL_WIN32)
   ancestor (1000)
#else
   ancestor(1000000)
#endif
{}

inline double PRealStopwatch::current() const
{
#ifdef PCOMN_PL_OS400
   _INT4 d ;
   _FLOAT8 s ;
   _FEEDBACK fb ;
   CEEUTC(&d, &s, &fb) ;
   return s ;
#elif defined(PCOMN_PL_WIN32)
   return GetTickCount() ;
#else
   timeval t = {0, 0} ;
   gettimeofday(&t, NULL) ;
   return t.tv_sec * 1000000.0 + t.tv_usec ;
#endif
}

/*******************************************************************************
 PCpuStopwatch
*******************************************************************************/
#ifdef PCOMN_PL_WINDOWS

inline PCpuStopwatch::PCpuStopwatch() :
   ancestor(10000000)
{}

inline PStopwatch::double PCpuStopwatch::current() const
{
   int64_t creationTime ;
   int64_t exitTime ;
   int64_t kernelTime ;
   int64_t userTime ;
   GetProcessTimes(GetCurrentProcess(),
                   (LPFILETIME)&creationTime,
                   (LPFILETIME)&exitTime,
                   (LPFILETIME)&kernelTime,
                   (LPFILETIME)&userTime) ;
   return (double)(kernelTime + userTime) ;
}

#elif defined(_POSIX_TIMERS) // PCOMN_PL_WINDOWS
// Unix, POSIX timers

inline PCpuStopwatch::PCpuStopwatch() :
   ancestor(1e9)
{}

inline double PCpuStopwatch::current() const
{
   struct timespec ts = { 0, 0 } ;
   clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) ;
   return ts.tv_sec * 1e9 + ts.tv_nsec ;
}

#else
// Unix, no POSIX timers

inline PCpuStopwatch::PCpuStopwatch() :
   ancestor(CLOCKS_PER_SEC)
{}

inline double PCpuStopwatch::current() const
{
   return clock() ;
}

#endif

/*******************************************************************************
 Backward compatibility support
*******************************************************************************/
typedef PStopwatch      PBaseTimer ;
typedef PCpuStopwatch   PCPUTimer ;
typedef PRealStopwatch  PRealTimer ;

} // end of namespace pcomn

#endif /* __PCOMN_STOPWATCH_H */
