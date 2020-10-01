/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   test_trace.cpp
 COPYRIGHT    :   Yakov Markovitch, 1998-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   TRACEPX/WARNPX tests

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   1 Jun 1998
*******************************************************************************/
#include <pcomn_trace.h>

#include <typeinfo>
#include <iostream>

#include <stdio.h>
#include <syslog.h>

DEFINE_DIAG_GROUP(TTST_FirstGroup, 0, 0, P_EMPTY_ARG) ;
DEFINE_DIAG_GROUP(TTST_SecondGroup, 0, 0, P_EMPTY_ARG) ;

DEFINE_DIAG_GROUP(TST0_Group1, 0, 0, P_EMPTY_ARG) ;
DEFINE_DIAG_GROUP(TST0_Group2, 0, 0, P_EMPTY_ARG) ;

DEFINE_DIAG_GROUP(STOBJ_Group1, true, DBGL_MAXLEVEL, P_EMPTY_ARG) ;
DEFINE_DIAG_GROUP(STOBJ_Group2, true, DBGL_MAXLEVEL, P_EMPTY_ARG) ;

DEFINE_TRACEFIXME(TTST) ;

#define TTST_FIXME(TEXT) TRACEFIXME(TTST, TEXT)

#define TEST_TRACE(group, level)                                        \
   TRACEPX(group, level, "Group " << #group << ". From lvl " << level   \
           << ". Current lvl " << DIAG_GETLEVEL(group))

#define TEST_WARN(group, cond, level)                                   \
   WARNPX(group, (cond), level, "Group " << #group << ". From lvl " << level \
          << ". Current lvl " << DIAG_GETLEVEL(group))

const char * const DEFAULT_PROFILE = "test_syslog.trace.ini" ;


int main(int argc, char *argv[])
{
   std::cout << "Using trace profile '" << DEFAULT_PROFILE << "'" << std::endl ;
   std::cout << "LOG_DEBUG=" << LOG_DEBUG << " LOG_CRIT=" << LOG_CRIT << std::endl ;

   openlog("test_syslog", LOG_PERROR|LOG_PID, LOG_USER) ;

   diag_inittrace(DEFAULT_PROFILE) ;

   try
   {
      TTST_FIXME("We should somehow issue a compiler warning!") ;

      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      LOGPXTRACE(TTST_SecondGroup, DBGL_MIDLEV, "LOGDBG") ;
      LOGPXINFO(TTST_FirstGroup, "LOGINFO") ;
      LOGPXWARN(TTST_SecondGroup, "LOGWARN") ;
      LOGPXERR(TTST_SecondGroup, "LOGERR") ;
      LOGPXALERT(TTST_FirstGroup, "LOGALERT") ;

      std::cout << std::endl ;
      try {
         LOGPXERR_CALL(pcomn::throw_exception<std::logic_error>,
                       TTST_FirstGroup, "Throw error") ;
      } catch (const std::exception &x) {
         std::cout << STDEXCEPTOUT(x) << std::endl ;
      }

      std::cout << std::endl ;
      try {
         LOGPXALERT_CALL(pcomn::throw_exception<std::domain_error>,
                         TTST_FirstGroup, "Throw alert") ;
      } catch (const std::exception &x) {
         std::cout << STDEXCEPTOUT(x) << std::endl ;
      }
   }
   catch (const std::exception &x)
   {
      std::cout << STDEXCEPTOUT(x) << std::endl ;
   }

   std::cout << "Press ENTER to end program..." << std::flush ;
   getchar() ;

   return 0 ;
}
