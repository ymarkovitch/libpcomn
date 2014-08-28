/*-*- tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)) -*-*/
/*******************************************************************************
 FILE         :   pudate.c
 COPYRIGHT    :   Yakov Markovitch, 1998-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Date processing (no Y2K problem!)

 PROGRAMMED BY:   Emil Dovidovitch, Yakov Markovitch
 CREATION DATE:   16 Oct 1998
*******************************************************************************/
#include <pudate.h>
#include <pcomn_assert.h>

#define PCOMN_BEGINNING_OF_TIME 1600

static int dayspermonth[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 } ;

int32_t pu_date2days(const bdate_t *date)
{
   int M = date->mon ;
   int Y = date->year - PCOMN_BEGINNING_OF_TIME ;

   int32_t E = Y - 1 ;
   int32_t D = date->day ;
   int32_t V = !(Y%4) && ((Y%100) || !(Y%400)) && (M > 2) ;

   NOXCHECK(date->year > PCOMN_BEGINNING_OF_TIME) ;

   return E*365L + E/4 - E/100 + E/400 + (int32_t)dayspermonth[M-1] + D + V ;
}

#define QQ ( 400L*365L + 97L )
#define Q  ( 4L*365L + 1L )

#define Century  ( 100L*365L + 24L )

bdate_t *pu_days2date(int32_t days, bdate_t *date)
{
   int i ;
   int32_t N400 = days / QQ,

      R400 = days % QQ,

      N100 = R400 / Century,
      R100 = R400 % Century,

      N4 = R100 / Q,
      R4 = R100 % Q,
      N1 = R4 / 365,
      R1 = R4 % 365,
      lV ;


   N1 += (R1 != 0) ;
   date->year = (unsigned short)(N400*400 + N100*100 + N4*4 + N1) ;

   lV = !(date->year%4) && ((date->year%100) || !(date->year%400)) ;

   if (!R1)
      R1 = 365+!N1*lV-!((days+1)%QQ) ;

   lV = lV && R1 > dayspermonth[2] ;

   for (i = 0; i < 12; i++)
      if (R1 - lV <= dayspermonth[i])
         break ;

   date->mon = i ;
   date->day = R1 - dayspermonth[i-1] - (lV && i > 2) ;
   date->year += PCOMN_BEGINNING_OF_TIME ;

   return date ;
}
