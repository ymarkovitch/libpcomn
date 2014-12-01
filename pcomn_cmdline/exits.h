//------------------------------------------------------------------------
// ^FILE: exits.h - define exit codes used by the CmdLine library
//
// ^DESCRIPTION:
//    When we call exit(3C), we want to use a value that is appropriate
//    for the operating system that we are on.  Here are the values
//    that we need to define:
//
//       e_SUCCESS  -- no errors everything is fine
//       e_USAGE    -- no errors but we didnt parse the command-line
//                     because we saw a CmdArgUsage argument
//       e_SYNTAX   -- a syntax error occurred
//       e_INTERNAL -- an internal error occurred
//
// ^HISTORY:
//    04/13/92	Brad Appleton	<bradapp@enteract.com>	Created
//-^^---------------------------------------------------------------------

#ifndef e_SUCCESS

#ifdef vms
# include <ssdef.h>
# define  e_SUCCESS   SS$_NORMAL
# define  e_USAGE     SS$_NOTMODIFIED
# define  e_SYNTAX    SS$_BADPRAM
# define  e_INTERNAL  SS$_CANCEL
#else
# define  e_SUCCESS   0
# define  e_USAGE     1
# define  e_SYNTAX    2
# define  e_INTERNAL  127
#endif

#endif
