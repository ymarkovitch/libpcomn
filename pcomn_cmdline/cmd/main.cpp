//------------------------------------------------------------------------
// ^FILE: main.c - main program for cmdparse(1)
//
// ^DESCRIPTION:
//     Instantiate a CmdParseCommand object and let it go do its stuff!
//
// ^HISTORY:
//    05/03/92	Brad Appleton	<bradapp@enteract.com>	Created
//-^^---------------------------------------------------------------------

#include <new>
#include <iostream>
#include <stdlib.h>

#include "cmdparse.h"

//
//  cmdparse_new_handler -- allocation error handler for cmdparse(1).
//
void  cmdparse_new_handler()
{
   std::cerr << "cmdparse: free-store allocation error." << std::endl ;
   ::exit(127);
}

int
main(int argc, const char * argv[])
{
   std::set_new_handler(cmdparse_new_handler);

   CmdParseCommand  cmdparse_cmd(*argv);
   CmdArgvIter  argv_iter(--argc, ++argv);

   return  cmdparse_cmd(argv_iter);
}
