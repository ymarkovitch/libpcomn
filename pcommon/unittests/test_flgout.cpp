/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   test_flgout.cpp
 COPYRIGHT    :   Yakov Markovitch, 1998-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   std::ostream flags output test

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   1 Jul 1998
*******************************************************************************/
#include <pcomn_flgout.h>
#include <stdlib.h>

enum DB4FileFlags
{
   DB2FLogical       =  0x00000001,
   DB2FSrcFile       =  0x00000002,
   DB2FHasKey        =  0x00000004,
   DB2FSelectOmit    =  0x00000008,
   DB2FMultiMember   =  0x00000010,
   DB2FMultiRecFmt   =  0x00000020,

   DB2FAllowRead     =  0x00000100,
   DB2FAllowWrite    =  0x00000200,
   DB2FAllowUpdate   =  0x00000400,
   DB2FAllowDelete   =  0x00000800
} ;

static pcomn::flag_name flgdesc[] =
{
   {DB2FLogical,     "Lgl"},
   {DB2FSrcFile,     "Src"},
   {DB2FHasKey,      "Key"},
   {DB2FSelectOmit,  "SelOmt"},
   {DB2FMultiMember, "MltMbr"},
   {DB2FMultiRecFmt, "MltFmt"},

   {DB2FAllowRead,   "AlwR"},
   {DB2FAllowWrite,  "AlwW"},
   {DB2FAllowUpdate, "AlwU"},
   {DB2FAllowDelete, "AlwD"},
   {0}
} ;

int main(int argc, char **argv)
{
   if (argc != 2)
   {
      std::cout << "Usage : " << argv[0] << " <flagval> " << std::endl ;
      return 1 ;
   }

   bigflag_t f = strtoul(argv[1], NULL, 0) ;

   std::cout << std::endl << "Flag print test." << std::endl << "Flags: " << HEXOUT(f) << std::endl
             << pcomn::flgout(f, flgdesc) << std::endl ;

   return 0 ;
}
