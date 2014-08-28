/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   test_trace.cpp
 COPYRIGHT    :   Yakov Markovitch, 1998-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   TRACEPX/WARNPX tests

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   1 Jun 1998
*******************************************************************************/
#include <pcomn_trace.h>

#include <typeinfo>
#include <iostream>

#include <stdio.h>

DEFINE_DIAG_GROUP(TTST_FirstGroup, 0, 0, P_EMPTY_ARG) ;
DEFINE_DIAG_GROUP(TTST_SecondGroup, 0, 0, P_EMPTY_ARG) ;

DEFINE_DIAG_GROUP(TST0_Group1, 0, 0, P_EMPTY_ARG) ;
DEFINE_DIAG_GROUP(TST0_Group2, 0, 0, P_EMPTY_ARG) ;

DEFINE_DIAG_GROUP(STOBJ_Group1, true, DBGL_MAXLEVEL, P_EMPTY_ARG) ;
DEFINE_DIAG_GROUP(STOBJ_Group2, true, DBGL_MAXLEVEL, P_EMPTY_ARG) ;

DEFINE_TRACEFIXME(TTST) ;

#define TTST_FIXME(TEXT) TRACEFIXME(TTST, TEXT)

class TraceTester {

   public:
      TraceTester()
      {
         const std::type_info &t (typeid(*this)) ;
         t.name() ;
         TRACEPX(STOBJ_Group1, DBGL_HIGHLEV, "Object of class " << PCOMN_TYPENAME(*this) <<
            " constructed. this = " << (void *)this) ;
      }

      TraceTester(const TraceTester &src)
      {
         TRACEPX(STOBJ_Group1, DBGL_HIGHLEV, "Object of class " << PCOMN_TYPENAME(*this) <<
            " constructed by copy from " << (void *)&src << " to "  << (void *)this) ;
      }

      virtual ~TraceTester()
      {
         TRACEPX(STOBJ_Group1, DBGL_HIGHLEV, "Destructor called for " << PCOMN_TYPENAME(*this) <<
            " this = " << (void *)this) ;
      }

      TraceTester &operator =(const TraceTester &src)
      {
         TRACEPX(STOBJ_Group2, DBGL_HIGHLEV, PCOMN_TYPENAME(*this) << " (" << (void *)this <<
            ") = " << PCOMN_TYPENAME(src)  << " (" << (void *)&src << ')') ;
         return *this ;
      }

} ;

#define TEST_TRACE(group, level)                                        \
   TRACEPX(group, level, "Group " << #group << ". From lvl " << level   \
           << ". Current lvl " << DIAG_GETLEVEL(group))

#define TEST_WARN(group, cond, level)                                   \
   WARNPX(group, (cond), level, "Group " << #group << ". From lvl " << level \
          << ". Current lvl " << DIAG_GETLEVEL(group))

const char * const DEFAULT_PROFILE = "test_trace.trace.ini" ;

struct FooStruct {
      void output(int a, double b) const
      {
         std::cout << SCOPEMEMFNOUT(diag::endargs) << std::endl ;
         std::cout << SCOPEMEMFNOUT(a << b) << std::endl ;
         std::cout << MEMFNOUT("OUTPUT", diag::endargs) << std::endl ;
         std::cout << MEMFNOUT("OUTPUT", a << b) << std::endl ;
      }
} ;

void test_funcout(int argc, char *argv[])
{
   std::cout << diag::ofncall("foobar") << 1 << 2 << "ThirdArg" << diag::endargs << std::endl ;
   std::cout << diag::ofncall("quux") << diag::endargs << std::endl ;
   std::cout << SCOPEFUNCOUT(diag::endargs) << std::endl ;
   std::cout << SCOPEFUNCOUT(argc << argv) << std::endl ;
   std::cout << FUNCOUT("hello", diag::endargs) << std::endl ;
   std::cout << FUNCOUT("hello", argc << argv) << std::endl ;

   FooStruct foo ;
   foo.output(777, 0.25) ;
}

int main(int argc, char *argv[])
{
   const char * const profile_name = argc < 2 ? DEFAULT_PROFILE : argv[1] ;

   std::cout << "Using trace profile '" << profile_name << "'" << std::endl ;

   DIAG_INITTRACE(profile_name) ;

   try
   {
      TTST_FIXME("We should somehow issue a compiler warning!") ;

      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
      TEST_TRACE(TTST_FirstGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;

      TEST_TRACE(TTST_SecondGroup, DBGL_ALWAYS) ;
   }
   catch (const std::exception &x)
   {
      std::cout << STDEXCEPTOUT(x) << std::endl ;
   }

   std::cout << "Press ENTER to end program..." << std::flush ;
   getchar() ;

   TEST_TRACE(TST0_Group2, DBGL_MIDLEV) ;
   TEST_WARN(TST0_Group2, false, DBGL_MIDLEV) ;
   TEST_WARN(TST0_Group2, true, DBGL_LOWLEV) ;

   return 0 ;
}
