//------------------------------------------------------------------------
// ^FILE: shell_arg.h - define a shell-script command-line argument
//
// ^DESCRIPTION:
//    This file defines the base class for all shell-script command
//    line arguments.  In addition to inheriting from class CmdArg,
//    a ShellCmdArg must also contain a variable-name, and its value.
//
// ^HISTORY:
//    04/22/92	Brad Appleton	<bradapp@enteract.com>	Created
//-^^---------------------------------------------------------------------

#ifndef  _shell_arg_h
#define  _shell_arg_h

#include  <cmdargs.h>

#include  "shells.h"

   // All of our shell-script args will be ShellCmdArgs with two possible
   // exceptions:
   //   1) We may have a CmdArgUsage or
   //   2) CmdArg::is_dummy() may return TRUE
   //
   // Hence, before we can downcast a CmdArg * to a ShellCmdArg *
   // we must first make sure of the following:
   //   1) is_dummy returns FALSE and that
   //   2) CmdArg::GIVEN is set (if a CmdArgUsage was given then we would have
   //      already exited!).
   //

class  ShellCmdArg : public CmdArg {
public:
   ShellCmdArg(char    * variable_name,
               char      optchar,
               char    * keyword,
               char    * value,
               char    * description,
               unsigned  syntax_flags =CmdArg::isVALREQ);

   ShellCmdArg(char    * variable_name,
               char      optchar,
               char    * keyword,
               char    * description,
               unsigned  syntax_flags = 0);

   ShellCmdArg(char    * variable_name,
               char    * value,
               char    * description,
               unsigned  syntax_flags =CmdArg::isPOSVALREQ);

   virtual ~ShellCmdArg();

      // Return the name of this variable/array
   const char *
   name() const { return  sca_name; }

      // Are we an array or a variable?
   int
   is_array() const;

      // Return the variable portion
   ShellVariable &
   variable()  { return  *shell_variable; }

      // Return the array portion
   ShellArray &
   array()  { return  *shell_array; }

      // If we are a variable then the "set" member function sets the
      // value of the variable, otherwise it appends to the list of
      // values of the array
      //
   void
   set(const char * value);

   virtual  int
   operator()(const char * & arg, CmdLine & cmd) = 0;

private:
   ShellCmdArg(const ShellCmdArg & cp);

   ShellCmdArg &
   operator=(const ShellCmdArg & cp);

   void
   initialize(const char * name);

   union {
      ShellVariable *  shell_variable;
      ShellArray    *  shell_array;
   } ;

   char * sca_name;
   char * sca_keyword;
   char * sca_value;
   char * sca_description;
} ;

#endif  /* _shell_arg_h */
