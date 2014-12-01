//------------------------------------------------------------------------
// ^FILE: shell_arg.c - implement a shell-script argument
//
// ^DESCRIPTION:
//    This file implements the base class that is used for all
//    shell-script command arguments.
//
// ^HISTORY:
//    04/22/92	Brad Appleton	<bradapp@enteract.com>	Created
//-^^---------------------------------------------------------------------

#include <iostream>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "shell_arg.h"

//----------------------------------------------------------------- initialize

void
ShellCmdArg::initialize(const char * variable_name)
{
   if (is_array()) {
      shell_array = new ShellArray(variable_name) ;
   } else {
      shell_variable = new ShellVariable(variable_name) ;
   }
}

//---------------------------------------------------------------- constructors

ShellCmdArg::ShellCmdArg(char    * variable_name,
                         char      optchar,
                         char    * keyword,
                         char    * value,
                         char    * description,
                         unsigned  syntax_flags)
   : CmdArg(optchar, keyword, value, description, syntax_flags),
     sca_name(variable_name), sca_keyword(keyword),
     sca_value(value), sca_description(description)
{
   initialize(variable_name);
}

ShellCmdArg::ShellCmdArg(char    * variable_name,
                         char      optchar,
                         char    * keyword,
                         char    * description,
                         unsigned  syntax_flags)
   : CmdArg(optchar, keyword, description, syntax_flags),
     sca_name(variable_name), sca_keyword(keyword),
     sca_value(NULL), sca_description(description)
{
   initialize(variable_name);
}

ShellCmdArg::ShellCmdArg(char    * variable_name,
                         char    * value,
                         char    * description,
                         unsigned  syntax_flags)
   : CmdArg(value, description, syntax_flags),
     sca_name(variable_name), sca_keyword(NULL),
     sca_value(value), sca_description(description)
{
   initialize(variable_name);
}

//------------------------------------------------------------------ destructor

ShellCmdArg::~ShellCmdArg()
{
   if (is_array()) {
      delete  shell_array ;
   } else {
      delete  shell_variable ;
   }
   delete [] sca_name ;
   delete [] sca_keyword ;
   delete [] sca_value ;
   delete [] sca_description ;
}

//-------------------------------------------------------------------- is_array

int
ShellCmdArg::is_array() const
{
   return  (syntax() & CmdArg::isLIST) ;
}

//------------------------------------------------------------------------- set

void
ShellCmdArg::set(const char * value)
{
   if (is_array()) {
      shell_array->append(value);
   } else {
      shell_variable->set(value);
   }
}
