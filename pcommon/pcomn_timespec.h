/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_TIMESPEC_H
#define __PCOMN_TIMESPEC_H
/*******************************************************************************
 FILE         :   pcomn_timespec.h
 COPYRIGHT    :   Yakov Markovitch, 2010-2017. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Time point and time interval classes for simple time manipulation.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   9 Jun 2009
*******************************************************************************/
/** Supplies time point and time interval classes for simple time manipulation.
*******************************************************************************/
#include <pcommon.h>
#include <pcstring.h>
#include <pcomn_assert.h>

#include <algorithm>

#if !defined(PCOMN_RTL_MS)
/*******************************************************************************
 POSIX runtime
*******************************************************************************/
#include <sys/time.h>

inline timeval pcomn_gettimeofday()
{
   struct timeval tv ;
   gettimeofday(&tv, NULL) ;
   return tv ;
}
#else
/*******************************************************************************
 Microsoft runtime
*******************************************************************************/
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <winsock2.h>

#define localtime_r(struct_tm, time_t_time) (localtime_s((time_t_time), (struct_tm)))
#define gmtime_r(struct_tm, time_t_time)    (gmtime_s((time_t_time), (struct_tm)))

inline timeval pcomn_gettimeofday()
{
   __timeb64 t {} ;
   _ftime64(&t) ;
   struct timeval tv = { (long)t.time, (long)t.millitm * 1000L } ;
   return tv ;
}
#endif

#include <stdio.h>

#include <limits>
#include <string>
#include <iostream>
#include <string.h>

namespace pcomn {

typedef int64_t usectime_t ;
typedef int64_t usecinterval_t ;

const usecinterval_t Tus   = 1 ;
const usecinterval_t Tms   = 1000*Tus ;
const usecinterval_t Ts    = 1000*Tms ;
const usecinterval_t Tmin  = 60*Ts ;
const usecinterval_t Thr   = 60*Tmin ;
const usecinterval_t Tday  = 24*Thr ;
const usecinterval_t Tweek = 7*Tday ;

/******************************************************************************/
/** Time interval with microsecond resolution.
*******************************************************************************/
class time_interval {
   public:
      /// Create zero interval.
      time_interval() : _value(0) {}

      explicit time_interval(usecinterval_t usec) : _value(usec) {}

      /// Interval value in seconds (may have fractional part).
      double seconds() const { return 1e-6 * _value ;	}

      /// Interval value in microseconds.
      usecinterval_t useconds() const { return _value ; }

      time_interval &operator+=(const time_interval &rhs)
      {
         _value += rhs._value ;
         return *this ;
      }
      time_interval &operator-=(const time_interval &rhs)
      {
         _value -= rhs._value ;
         return *this ;
      }
      PCOMN_DEFINE_ADDOP_METHODS(time_interval) ;

      bool operator==(const time_interval &rhs) const { return _value == rhs._value ; }
      bool operator<(const time_interval &rhs) const { return _value < rhs._value ; }
      PCOMN_DEFINE_RELOP_METHODS(time_interval) ;

      static time_interval max_interval()
      {
         return time_interval(std::numeric_limits<usecinterval_t>::max()) ;
      }

   private:
      usecinterval_t _value ;
} ;

/******************************************************************************/
/** UNIX epoch-based point in time with microsecond precision.
*******************************************************************************/
class time_point {
   public:
      enum Zone {
         LOCAL,
         GMT
      } ;

      /// Create a time point from the UNIX epoch date expressed in useconds
      explicit time_point(usectime_t usectime) : _value(usectime) {}

      time_point() : _value(std::numeric_limits<usectime_t>::min()) {}

      /// Create a time point from the UNIX epoch date
      time_point(const struct timeval &tv) : _value(tv.tv_sec * Ts + tv.tv_usec * Tus) {}

      /// Create a time point from the broken-down time, either local or UTC/GMT
      /// @param t      Broken-down time
      /// @param zone   Time zone, LOCAL or GMT
      time_point(const struct tm &t, Zone zone = LOCAL)
      {
         init_from_tm(t, zone) ;
      }

      time_point(Zone zone,
                 unsigned year, unsigned month, unsigned day,
                 unsigned hour, unsigned minute, unsigned sec = 0,
                 unsigned usec = 0)
      {
         ::tm t = ::tm() ;
         t.tm_sec = sec + usec/Ts ; t.tm_min = minute ; t.tm_hour = hour ;
         t.tm_mday = day ; t.tm_mon = month ; t.tm_year = year - 1900 ;
         init_from_tm(t, zone) ;
         _value += usec % Ts ;
      }

      static time_point from_time(time_t t) { return time_point(t * Ts) ; }

      static time_point now()
      {
         return time_point(pcomn_gettimeofday()) ;
      }

      explicit operator bool() const { return _value != time_point()._value ; }

      time_point &operator+=(const time_interval &rhs)
      {
         NOXCHECK(*this) ;
         _value += rhs.useconds() ;
         return *this ;
      }
      time_point &operator-=(const time_interval &rhs)
      {
         NOXCHECK(*this) ;
         _value -= rhs.useconds() ;
         return *this ;
      }

      time_interval operator-(const time_point &rhs) const
      {
         NOXCHECK(*this && rhs) ;
         return time_interval(_value - rhs._value) ;
      }

      time_point operator+(const time_interval &rhs) const
      {
         time_point result (*this) ;
         return result += rhs ;
      }

      time_point operator-(const time_interval &rhs) const
      {
         time_point result (*this) ;
         return result -= rhs ;
      }

      struct timeval as_timeval() const
      {
         NOXCHECK(*this) ;
         const struct timeval result = { (long)(_value/Ts), (long)(_value%Ts) } ;
         return result ;
      }

      time_t as_time() const { return as_timeval().tv_sec ; }

      usectime_t useconds() const { return _value ; }

      usectime_t as_useconds() const { return useconds() ; }

      struct tm as_tm(Zone zone) const
      {
         const time_t rt = as_time() ;
         struct tm result ;
         zone == LOCAL ? localtime_r(&rt, &result) : gmtime_r(&rt, &result) ;
         return result ;
      }

      char *string(char *buf, size_t size, Zone zone = LOCAL) const
      {
         if (!buf || !size)
            return NULL ;
         if (!*this)
            return &(*buf = 0) ;

         char result[64] ;
         const struct tm &t = as_tm(zone) ;

         strftime(result, sizeof result, "%F %T", &t) ;
         const size_t sz = strlen(result) ;
         snprintf(result + sz, sizeof result - sz, ".%03u", (unsigned)(_value%Ts/Tms)) ;
         strncpy(buf, result, std::min(strlen(result) + 1, size - 1)) ;
         buf[size - 1] = 0 ;
         return buf ;
      }

      template<size_t n>
      char *string(char (&buf)[n], Zone zone = LOCAL) const
      {
         return string(buf, n, zone) ;
      }

      std::string string(Zone zone = LOCAL) const
      {
         char buf[64] ;
         return std::string(string(buf, zone)) ;
      }

      // rfc1123 format string, used in HTTP, like Sun, 06 Nov 1994 08:49:37 GMT
      char *http_string(char *buf, size_t size) const
      {
         if (!buf || !size)
            return NULL ;
         if (!*this)
            return &(*buf = 0) ;

         static const char wkday[7][4] =
            {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"} ;
         static const char month[12][4] =
            {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"} ;

         char result[64] ;
         const struct tm &t = as_tm(GMT) ;
         sprintf(result, "%s, %02d %s %04d %02d:%02d:%02d GMT",
                 wkday[std::min((unsigned)t.tm_wday, 6U)], t.tm_mday, month[std::min((unsigned)t.tm_mon, 11U)], 1900+t.tm_year,
                 t.tm_hour, t.tm_min, t.tm_sec) ;

         strncpyz(buf, result, size) ;
         return buf ;
      }

      template<size_t n>
      char *http_string(char (&buf)[n]) const { return http_string(buf, n) ; }

      std::string http_string() const
      {
         char buf[64] ;
         return std::string(http_string(buf)) ;
      }

   private:
      usectime_t _value ;

   private:
      void init_from_tm(struct tm t, Zone zone)
      {
         t.tm_isdst = -1 ;
         const time_t time_plus_offs = safe_mktime(t) ;
         if (zone == LOCAL)
            *this = from_time(time_plus_offs) ;
         else
         {
            struct tm tm_plus_offs ;
            gmtime_r(&time_plus_offs, &tm_plus_offs) ;
            const time_t offs = safe_mktime(tm_plus_offs) - time_plus_offs ;
            *this = from_time(time_plus_offs - offs) ;
         }
      }

      static time_t safe_mktime(struct tm &t)
      {
         const time_t time = mktime(&t) ;
         if (time != -1)
            return time ;

         --t.tm_hour ;
         const time_t off1hr = mktime(&t) ;
         return off1hr == -1 ? off1hr : off1hr + 3600 ;
      }
} ;

inline bool operator==(const time_point &left, const time_point &right)
{
   return left.as_useconds() == right.as_useconds() ;
}
inline bool operator<(const time_point &left, const time_point &right)
{
   return left.as_useconds() < right.as_useconds() ;
}

PCOMN_DEFINE_RELOP_FUNCTIONS(,time_point) ;

/*******************************************************************************
 Coarse-grained wall time clock
*******************************************************************************/
/// Get wall time in tenth of seconds
usectime_t time_coarse(unsigned precision) ;

inline usectime_t time_coarse(unsigned precision)
{
   if (!precision)
      return time(NULL) * Ts ;

   // FIXME: time_coarse stub
   const timeval t = pcomn_gettimeofday() ;
   static const int64_t usec_div[] = { 100000, 10000, 1000, 100, 10 } ;
   const int64_t d = precision >= 6 ? 1 : usec_div[precision - 1] ;
   return t.tv_sec * Ts + t.tv_usec / d * d ;
}

/*******************************************************************************
 Debug output
*******************************************************************************/
inline std::ostream &operator<<(std::ostream &os, const time_interval &t)
{
   char buf[64] ;
   snprintf(buf, sizeof buf, "%u.%06u", (unsigned)(t.useconds()/Ts), (unsigned)(t.useconds()%Ts)) ;
   return os << buf ;
}

inline std::ostream &operator<<(std::ostream &os, const time_point &t)
{
   char buf[64] ;
   return os << t.string(buf) ;
}
} // end of namespace pcomn

#endif /* __PCOMN_TIMESPEC_H */
