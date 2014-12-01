#include "argtypes.h"

#include <iostream>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//-----------------------------------------------------------------------------
// ^FUNCTION: operator() - handle an argument from the command-line
//
// ^SYNOPSIS:
//    int  operator()(arg, cmd);
//
// ^PARAMETERS:
//    const char * & arg;
//    -- the prospective value for this command argument.
//       upon returning this value should be updated to point to the first
//       character of "arg" that was NOT used as part of the value for this
//       argument (set "arg" to NULL if all of it was used).
//
//    CmdLine & cmd;
//    -- the command that matched this argument on its command-line
//
// ^DESCRIPTION:
//    These member functions are responsible for taking whatever action
//    is appropriate when its corresponding command argument is matched
//    on the command-line.
//
//    All the argument-types in this file correspond to argument of some
//    kind of shell script, hence all of the command-argument objects are
//    not just derived from CmdArg but from ShellCmdArg as well.  All we
//    need to do is check the syntax of the argument (without compiling it)
//    and if it is valid then use the argument as the value for the
//    corresponding shell variable.
//
// ^REQUIREMENTS:
//    The "arg_flags" data member of this command-argument must have been
//    set appropriately (by "cmd") to indicate to us exactly how "arg" was
//    specified on the command-line for this (and only this) occurrence of
//    "arg".
//
// ^SIDE-EFFECTS:
//    - If (cmd.flags() & QUIET) is NOT TRUE and FAILURE is to be returned,
//      then error messages should be printed using cmd.error().
//
//    - arg is modified to be NULL of to point to the unused portion of itself.
//
//    - If we need the value of "arg" to stick around, then storage is allocated
//      in order to make  a copy of "arg" (and the command-argument is responsible
//      for de-allocating this storage).
//
// ^RETURN-VALUE:
//    FAILURE (non-zero)  If something went wrong when performing the
//                        desired actions for this command-argument.
//                        A common problem would be that "arg" is
//                        syntactically incorrect.
//
//    SUCCESS (zero)  If "arg" is NULL and/or we were able to succesfully
//                    perform all desired actions for this command argument.
//-^^--------------------------------------------------------------------------


//-------------------------------------------------------------- ShellCmdArgInt

ShellCmdArgInt::~ShellCmdArgInt()
{
}

int
ShellCmdArgInt::operator()(const char * & arg, CmdLine & cmd)
{
   CmdArgInt  int_arg(*this);
   const char * save_arg = arg;
   int  badval = int_arg(arg, cmd);
   if (save_arg && !badval)  set(save_arg);
   return  badval;
}

//------------------------------------------------------------ ShellCmdArgFloat

ShellCmdArgFloat::~ShellCmdArgFloat()
{
}

int
ShellCmdArgFloat::operator()(const char * & arg, CmdLine & cmd)
{
   CmdArgFloat  float_arg(*this);
   const char * save_arg = arg;
   int  badval = float_arg(arg, cmd);
   if (save_arg && !badval)  set(save_arg);
   return  badval;
}

//------------------------------------------------------------- ShellCmdArgChar

ShellCmdArgChar::~ShellCmdArgChar()
{
}

int
ShellCmdArgChar::operator()(const char * & arg, CmdLine & cmd)
{
   CmdArgChar  char_arg(*this);
   const char * save_arg = arg;
   int  badval = char_arg(arg, cmd);
   if (save_arg && !badval)  set(save_arg);
   return  badval;
}

//-------------------------------------------------------------- ShellCmdArgStr

ShellCmdArgStr::~ShellCmdArgStr()
{
}

int
ShellCmdArgStr::operator()(const char * & arg, CmdLine & cmd)
{
   CmdArgStr  str_arg(*this);
   const char * save_arg = arg;
   int  badval = str_arg(arg, cmd);
   if (save_arg && !badval)  set(save_arg);
   return  badval;
}

//------------------------------------------------------------- ShellCmdArgBool

const char * ShellCmdArgBool::true_string  = "TRUE" ;
const char * ShellCmdArgBool::false_string = "" ;

ShellCmdArgBool::~ShellCmdArgBool()
{
}

int
ShellCmdArgBool::operator()(const char * & arg, CmdLine & cmd)
{
   CmdArgBool  bool_arg(*this);
   int  badval = bool_arg(arg, cmd);
   if (! badval) {
      set((bool_arg) ? True() : False());
   }
   return  badval;
}

//------------------------------------------------------------ ShellCmdArgClear

ShellCmdArgClear::~ShellCmdArgClear()
{
}

int
ShellCmdArgClear::operator()(const char * & arg, CmdLine & cmd)
{
   CmdArgClear  bool_arg(*this);
   int  badval = bool_arg(arg, cmd);
   if (! badval) {
      set((bool_arg) ? True() : False());
   }
   return  badval;
}

//----------------------------------------------------------- ShellCmdArgToggle

ShellCmdArgToggle::~ShellCmdArgToggle()
{
}

int
ShellCmdArgToggle::operator()(const char * & arg, CmdLine & cmd)
{
   CmdArgToggle  bool_arg(*this);
   int  badval = bool_arg(arg, cmd);
   if (! badval) {
      set((bool_arg) ? True() : False());
   }
   return  badval;
}
