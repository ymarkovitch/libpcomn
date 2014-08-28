/*-*- tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets: ((inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   cchktst.c
 COPYRIGHT    :   Yakov Markovitch, 1998-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   CHECKX/PRECONDITIONX tests

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   23 Jun 1998
*******************************************************************************/
#define __PCOMN_DEBUG 2

#include <pcomn_assert.h>

#include <stdio.h>

void main (int argc, char *argv[])
{
   if (argc != 2)
   {
      printf("Usage: checktst <0|1|2|3>\n") ;
      return ;
   }
   switch(atoi(argv[1]))
   {
      case 0 :
         printf("Test for PPRECONDITION\n") ;
         PPRECONDITION(argv[0] == NULL) ;

      case 1 :
         printf("Test for NOXPRECONDITION\n") ;
         NOXPRECONDITION(argv[0] == NULL) ;

      case 2 :
         printf("Test for PPRECONDITIONX\n") ;
         PPRECONDITIONX(argv[0] == NULL, "argv[0] must be NULL") ;

      case 3 :
         printf("Test for NOXPRECONDITIONX\n") ;
         NOXPRECONDITIONX(argv[0] == NULL, "argv[0] must be NULL") ;

      default:
         printf("Illegal parameter. Must be 0, 1, 2 or 3") ;
         return ;

   }

}
