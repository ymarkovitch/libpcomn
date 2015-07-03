//------------------------------------------------------------------------
// ^FILE: patchlevel.c - version specific information
//
// ^DESCRIPTION:
//    This file contains all the version specific information of this
//    particular release of the CmdLine library.  It also implements
//    the static member functions of a CmdLine that return version
//    specific information.
//
// ^HISTORY:
//    04/03/92	Brad Appleton	<bradapp@enteract.com>	Created
//
//    03/03/93	Brad Appleton	<bradapp@enteract.com>
//    - Modified for patch 1
//
//    08/19/93	Brad Appleton	<bradapp@enteract.com>
//    - Modified for patch 2
//
//    10/08/93	Brad Appleton	<bradapp@enteract.com>
//    - Modified for patch 3
//
//    01/11/94	Brad Appleton	<bradapp@enteract.com>
//    - Modified for patch 4
//-^^---------------------------------------------------------------------

#include "cmdline.h"

   // Release and patchlevel information
#define  CMDLINE_RELEASE     1
#define  CMDLINE_PATCHLEVEL  4
#define  CMDLINE_IDENT       "@(#)CmdLine	1.04"

unsigned
CmdLine::release()  { return  CMDLINE_RELEASE; }

unsigned
CmdLine::patchlevel()  { return  CMDLINE_PATCHLEVEL; }

const char *
CmdLine::ident() {
   static const char Ident[] = CMDLINE_IDENT ;

   return  Ident;
}
