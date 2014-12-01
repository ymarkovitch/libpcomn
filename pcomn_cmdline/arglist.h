//------------------------------------------------------------------------
// ^FILE: arglist.h - lists to hold CmdArgs
//
// ^DESCRIPTION:
//   This file declares the types that are used by a CmdLine to hold the
//   arguments associated with it.  What we do is keep a list of lists
//   of CmdArgs. The first list is the arguments that were specified
//   by the user/programmer, The second list is the list of default
//   arguments for this command. Only two lists are used at present.
//
// ^HISTORY:
//    03/21/92	Brad Appleton	<bradapp@enteract.com>	Created
//-^^---------------------------------------------------------------------

#ifndef _arglist_h
#define _arglist_h

#include "fifolist.h"

class CmdArg;

DECLARE_FIFO_LIST(CmdArgList, CmdArg);

DECLARE_FIFO_LIST(CmdArgListList, CmdArgList);

#endif /* _arglist_h */

