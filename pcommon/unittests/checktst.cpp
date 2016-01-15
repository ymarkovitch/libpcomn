/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   checktst.cpp
 COPYRIGHT    :   Yakov Markovitch, 1998-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   PCOMMON's assert macros tests

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   23 Jun 1998
*******************************************************************************/
#include <pcomn_assert.h>
#include <pcommon.h>
#include <stdlib.h>
#include <iostream>

int main (int argc, char *argv[])
{
   try
   {
      if (argc < 2)
      {
         std::cout << "Usage: checktst <0|1|2>" << std::endl ;
         return 1 ;
      }
      switch(atoi(argv[1]))
      {
         case 0 :
            if (argc < 3)
            {
               std::cout << "Test for NOXPRECONDITION"  << std::endl ;
               NOXPRECONDITION(argv[0] == NULL) ;
            }
            else
            {
               std::cout << "Test for NOXCHECK"  << std::endl ;
               NOXCHECK(argv[0] == NULL) ;
            }
            break ;

         case 1 :
            if (argc < 3)
            {
               std::cout << "Test for NOXPRECONDITIONXX"  << std::endl ;
               NOXPRECONDITIONX(argv[0] == NULL, "argv[0] must be NULL but actually is not") ;
            }
            else
            {
               std::cout << "Test for NOXCHECKXX"  << std::endl ;
               NOXCHECKX(argv[0] == NULL, "argv[0] must be NULL but actually is not") ;
            }
            break ;

         case 2 :
            if (argc < 3)
            {
               std::cout << "Test for PARANOID_NOXCHECK"  << std::endl ;
               PARANOID_NOXCHECK(argv[0] == NULL) ;
            }
            else
            {
               std::cout << "Test for PARANOID_NOXCHECKX"  << std::endl ;
               PARANOID_NOXCHECKX(argv[3] == NULL, "There must be no more than 3 arguments!") ;
            }
            break ;

         default:
            std::cerr << "Illegal parameter. Must be 0, 1, or 2" << std::endl ;
            return 1 ;

      }

   }
   catch (const std::exception &x)
   {
      std::cout << STDEXCEPTOUT(x) << std::endl ;
   }

   std::cerr << "No assertions has been triggered, sorry." << std::endl ;
   return 0 ;
}
