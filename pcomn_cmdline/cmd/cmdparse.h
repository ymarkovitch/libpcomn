//------------------------------------------------------------------------
// ^FILE: cmdparse.h - define the interface to cmdparse(1)
//
// ^DESCRIPTION:
//    This file defines a class that carries out the services provided
//    by cmdparse(1).
//
// ^HISTORY:
//    05/01/92	Brad Appleton	<bradapp@enteract.com>	Created
//
//    03/01/93	Brad Appleton	<bradapp@enteract.com>
//    - Added ALLOW_PLUS to list of CmdLine configuration flags
//-^^---------------------------------------------------------------------

#ifndef _cmdparse_h
#define _cmdparse_h

#include <cmdargs.h>
#include <iostream>

   // Exit values used
enum  ExitValues {
   e_SUCCESS   = 0,      // no errors
   e_USAGE     = 1,      // no errors - usage printed
   e_VERSION   = 1,      // no errors - version printed
   e_CMDSYNTAX = 2,      // command-line syntax error
   e_BADSHELL  = 3,      // invalid shell specified
   e_BADDECLS  = 4,      // invalid declaration(s) given
} ;


   // CmdArgVers is a class that simply prints (on cerr) the version
   // information for this command.
   //
class  CmdArgVers : public CmdArg {
public:
   CmdArgVers(char optchar, const char * keyword, const char * description);

   virtual ~CmdArgVers();

   virtual  int
   operator()(const char * & arg, CmdLine & cmd);
} ;


class  ArgSyntax ;
class  UnixShell ;
class  CmdParseCommand : public CmdLine {
public:

   CmdParseCommand(const char * name);

   virtual  ~CmdParseCommand();

      // Do whatever it is we need to do!
   int
   operator()(CmdLineArgIter & iter) ;

private:
      // Dont allow copying or assignment
   CmdParseCommand(const CmdParseCommand &);
   CmdParseCommand & operator=(const CmdParseCommand &);

      // Set the users arguments
   void
   set_args(UnixShell * shell);

      // Parse the users argument declarations
   int
   parse_declarations();

   int
   parse_declarations(const char * input);

   int
   parse_declarations(std::istream & input);

      // Add a parsed declaration to the user's argument list
   int
   usr_append(const char * type,
              const char * varname,
              ArgSyntax  & arg,
              const char * description);

   //------------------------------------------------ arguments to cmdparse(1)

   CmdArgBool     anywhere;         // clear OPTS_FIRST
   CmdArgBool     anycase;          // set ANY_CASE_OPTS
   CmdArgBool     no_abort;         // set NO_ABORT
   CmdArgBool     guess;            // set GUESS
   CmdArgBool     prompt;           // set PROMPT_USER
   CmdArgBool     plus;             // set ALLOW_PLUS
   CmdArgBool     opts_only;        // set OPTS_ONLY
   CmdArgBool     kwds_only;        // set KWDS_ONLY
   CmdArgBool     quiet;            // set QUIET

   CmdArgVers     version;          // print version and exit
   CmdArgBool     usage;            // print usage and exit

   CmdArgBool     array_variant;    // use alternate array syntax
   CmdArgStr      true_str;         // TRUE for booleans
   CmdArgStr      false_str;        // FALSE for booleans
   CmdArgStr      suffix_str;       // suffix for missing optional-values
   CmdArgStr      usr_shell;        // the shell (command interpreter)

   CmdArgStr      input_file;       // read declarations from file
   CmdArgStr      input_var;        // read declarations environment variable
   CmdArgStr      input_str;        // read declarations from string

   CmdArgDummy    dummy_arg;        // "--"

   CmdArgStr      usr_prog;         // program name
   CmdArgStrList  usr_args;         // program arguments

   CmdLine        usr_cmd;          // the user's CmdLine object
} ;

#endif  /* _cmdparse_h */
