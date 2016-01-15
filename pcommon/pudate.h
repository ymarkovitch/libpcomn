/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PUDATE_H
#define __PUDATE_H
/*******************************************************************************
 FILE         :   pudate.h
 COPYRIGHT    :   Yakov Markovitch, 1999-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   64-bit date, time and timestamp type(s)
                  Universal timestamp is presented as number of microseconds
                  elapsed since the 31st December 1600, 00:00:00, shifted left (biased)
                  by 8 bits.
                  Timestamp have to be interpreted as UTC (Universal Time Coordinated).

 CREATION DATE:   15 Nov 1999
*******************************************************************************/
#include <pcommon.h>

GCC_DIAGNOSTIC_PUSH()
GCC_IGNORE_WARNING(reorder)

typedef struct bdate
{
#ifndef PCOMN_CPU_BIG_ENDIAN
      uint8_t day ;
      uint8_t mon ;
      uint16_t year ;
#else
      uint16_t year ;
      uint8_t mon ;
      uint8_t day ;
#endif
#ifdef __cplusplus
      constexpr bdate(uint16_t y, uint8_t m, uint8_t d = 1) :
         year(y), mon(m), day(d)
      {}
      explicit constexpr bdate(uint16_t y) : bdate(y, 1) {}
      explicit constexpr bdate() : bdate(1) {}

      explicit constexpr operator uint32_t() const
      {
         return *reinterpret_cast<const uint32_t *>(this) ;
      }
      explicit constexpr operator bool() const
      {
         return static_cast<uint32_t>(*this) ;
      }
#endif
} bdate_t ;

typedef struct btime
{
#ifndef PCOMN_CPU_BIG_ENDIAN
   uint8_t hundr ;
   uint8_t sec ;
   uint8_t min ;
   uint8_t hour ;
#else
   uint8_t hour ;
   uint8_t min ;
   uint8_t sec ;
   uint8_t hundr ;
#endif
#ifdef __cplusplus
      constexpr btime(uint8_t hr, uint8_t m, uint8_t s = 0, uint8_t hs = 0) :
         hour(hr), min(m), sec(s), hundr(hs)
      {}
      constexpr explicit btime(uint8_t hr) : btime(hr, 0) {}
      constexpr btime() : btime(0) {}

      explicit constexpr operator uint32_t() const
      {
         return *reinterpret_cast<const uint32_t *>(this) ;
      }
      explicit constexpr operator bool() const
      {
         return static_cast<uint32_t>(*this) ;
      }
#endif
} btime_t ;


#ifndef PCOMN_CPU_BIG_ENDIAN
/*
   Macros for bdate & btime static initialisation
*/
#define PCOMN_SCTR_BDATE(y,m,d) {(d),(m),(y)}
#define PCOMN_SCTR_BTIME(h,m,s,hs) {(hs),(s),(m),(h)}

#else // PCOMN_CPU_BIG_ENDIAN
/*
   Macros for bdate & btime static initialisation
*/
#define PCOMN_SCTR_BDATE(y,m,d) {(y),(m),(d)}
#define PCOMN_SCTR_BTIME(h,m,s,hs) {(h),(m),(s),(hs)}

#endif // PCOMN_CPU_BIG_ENDIAN

#define PCOMN_BDATE_AS_ULONG(bd) (*(const uint32_t *)(&(bd)))
#define PCOMN_BTIME_AS_ULONG(bt) (*(const uint32_t *)(&(bt)))

/*
   Checking bdate & btime for zero values
*/
#define PCOMN_IS_NULL_BDATE(bd) (!PCOMN_BDATE_AS_ULONG(bd))
#define PCOMN_IS_NULL_BTIME(bt) (!PCOMN_BTIME_AS_ULONG(bt))

typedef struct btimestamp {

      bdate_t date ;
      btime_t time ;

#ifdef __cplusplus
      constexpr btimestamp() = default ;
      constexpr btimestamp(bdate d, btime t = {}) : date(d), time(t) {}
#endif
} btimestamp_t ;

PCOMN_CFUNC _PCOMNEXP int32_t  pu_date2days(const bdate_t *date) ;
PCOMN_CFUNC _PCOMNEXP bdate_t *pu_days2date(int32_t days, bdate_t *date) ;

#ifdef __cplusplus

#include <pcomn_math.h>
#include <iostream>
#include <time.h>
#include <stdio.h>

#define PCOMN_MQSEC_BIAS         8
#define PCOMN_SEC_PER_DAY        ((int64_t)86400)
#define PCOMN_MQSEC_TO_TS(usec)  ((int64_t)((int64_t)(usec) << PCOMN_MQSEC_BIAS))
#define PCOMN_TS_TO_MQSEC(ts)    ((int64_t)((int64_t)(ts) >> PCOMN_MQSEC_BIAS))
#define PCOMN_TS_1MSEC           PCOMN_MQSEC_TO_TS(1000)
#define PCOMN_TS_1SEC            PCOMN_MQSEC_TO_TS(1000000)
#define PCOMN_TS_1DAY            PCOMN_MQSEC_TO_TS(PCOMN_SEC_PER_DAY*1000000)
#define PCOMN_UNIX_BEGINNING_OF_TIME ((int64_t)134775 * PCOMN_SEC_PER_DAY)

// The type of timestamp. As far as widespreaded compilers we are using have
// 64-bit integer, we don't care about portability.
typedef int64_t putimestamp_t ;

inline std::ostream &operator<<(std::ostream &os, const bdate_t &d)
{
   char buf[11] ;
   sprintf(buf, "%04hu-%02hu-%02hu", (short)d.year, (short)d.mon, (short)d.day) ;
   return os << buf ;
}

inline std::ostream &operator<<(std::ostream &os, const btime_t &t)
{
   char buf[12] ;
   sprintf(buf, "%02hu.%02hu.%02hu.%02hu", (short)t.hour, (short)t.min, (short)t.sec, (short)t.hundr) ;
   return os << buf ;
}

inline std::ostream &operator<<(std::ostream &os, const btimestamp_t &ts)
{
   return os << ts.date << '-' << ts.time ;
}

inline constexpr bool operator==(const bdate_t &d1, const bdate_t &d2)
{
   return PCOMN_BDATE_AS_ULONG(d1) == PCOMN_BDATE_AS_ULONG(d2) ;
}

inline constexpr bool operator<(const bdate_t &d1, const bdate_t &d2)
{
   return PCOMN_BDATE_AS_ULONG(d1) < PCOMN_BDATE_AS_ULONG(d2) ;
}

PCOMN_DEFINE_RELOP_FUNCTIONS(constexpr, bdate) ;

// pu_tsdaytime() -  extract the time of day part from the timestamp
//                   (i.e. cut off all but incomplete day).
// Result is always less than 24 hours.
// Parameters:
//    ts -  timestamp.
//
inline putimestamp_t pu_tsdaytime(putimestamp_t ts)
{
   return pcomn::idivmod(ts, PCOMN_TS_1DAY) ;
}

// pu_tsdate() - get timestamp equal to full days contained in parameter.
// Result is always positioned to the 00:00:00 time of day.
// Parameters:
//    ts -  timestamp.
//
inline putimestamp_t pu_tsdate(putimestamp_t ts)
{
   return pcomn::idiv(ts, PCOMN_TS_1DAY) * PCOMN_TS_1DAY ;
}

// pu_tsdays() - get full days from the beginning of the epoch contained in parameter.
// Parameters:
//    ts -  timestamp.
//
inline int32_t pu_tsdays(putimestamp_t ts)
{
   return static_cast<int32_t>(pcomn::idiv(ts, PCOMN_TS_1DAY)) ;
}

inline putimestamp_t pu_days2ts(int32_t days)
{
   return static_cast<putimestamp_t>(days) * PCOMN_TS_1DAY ;
}

// pu_usec2ts()  -  convert number of microseconds into timestamp
// Parameters:
//    usec -  number of microseconds.
//
inline putimestamp_t pu_usec2ts(int64_t usec)
{
   return PCOMN_MQSEC_TO_TS(usec) ;
}

// pu_ts2usec()  -  convert timestamp into the number of microseconds elapsed from
//                   the beginning of time.
// Parameters:
//    ts -  timestamp.
//
inline int64_t pu_ts2usec(putimestamp_t ts)
{
   return PCOMN_TS_TO_MQSEC(ts) ;
}

// pu_ts2bts() -  convert timestamp into btimestamp.
// Parameters:
//    ts -  timestamp.
//
inline btimestamp pu_ts2bts(putimestamp_t ts)
{
   btimestamp result ;
   int32_t puredays = pu_tsdays(ts) ;
   pu_days2date(puredays, &result.date) ;
   int32_t hundredths = (int32_t)(pu_ts2usec(ts - puredays*PCOMN_TS_1DAY)/10000) ;
   result.time.hundr = hundredths % 100 ;
   hundredths /= 100 ;
   result.time.sec = hundredths % 60 ;
   hundredths /= 60 ;
   result.time.min = hundredths % 60 ;
   result.time.hour = hundredths / 60 ;
   return result ;
}

inline putimestamp_t pu_bts2ts(btimestamp ts)
{
   return (int64_t)pu_date2days(&ts.date) * PCOMN_TS_1DAY +
      (ts.time.hour * 360000 + ts.time.min * 6000 + ts.time.sec * 100 + ts.time.hundr) *
      (PCOMN_TS_1SEC/100) ;
}

// pu_time2ts() -  convert UNIX time (the number of seconds elapsed since 1 1 1970) into timestamp.
// Parameters:
//    xtime -  UNIX time
//
inline putimestamp_t pu_time2ts(time_t xtime)
{
   return pu_usec2ts((PCOMN_UNIX_BEGINNING_OF_TIME + xtime) * 1000000) ;
}

// pu_time2ts() -  convert timestamp into UNIX time.
// Parameters:
//    ts -  timestamp. Mustn't designate time before 1 1 1970, 00:00:00
//
inline time_t pu_ts2time(putimestamp_t ts)
{
   return (time_t)(pu_ts2usec(ts)/1000000 - PCOMN_UNIX_BEGINNING_OF_TIME) ;
}

#endif /* __cplusplus */

GCC_DIAGNOSTIC_POP()

#endif /* __PUDATE_H */
