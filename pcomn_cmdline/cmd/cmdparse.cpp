//------------------------------------------------------------------------
// ^FILE: cmdparse.c - implementation of the CmdParseCommand
//
// ^DESCRIPTION:
//    This file implements the member functions of the CmdParseCommand
//    class.
//
// ^HISTORY:
//    04/26/92	Brad Appleton	<bradapp@enteract.com>	Created
//
//    03/01/93	Brad Appleton	<bradapp@enteract.com>
//    - Added ALLOW_PLUS to list of CmdLine configuration flags
//-^^---------------------------------------------------------------------

#include <iostream>
#include <fstream>
#include <sstream>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "argtypes.h"
#include "cmdparse.h"
#include "syntax.h"
#include "quoted.h"

enum { SUCCESS = 0, FAILURE = -1 } ;

enum { MAX_IDENT_LEN = 64, MAX_DESCRIPTION_LEN = 1024 } ;


extern "C" {
   int  isatty(int fd);
}

//------------------------------------------------------------------------ copy

//-------------------
// ^FUNCTION: copy - copy a string
//
// ^SYNOPSIS:
//    copy(dest, src)
//
// ^PARAMETERS:
//    char * & dest;
//    -- where to put the copy we make
//
//    const char * src;
//    -- the string to copy
//
// ^DESCRIPTION:
//    This function duplicates its second parameter to its first parameter.
//
// ^REQUIREMENTS:
//    None.
//
// ^SIDE-EFFECTS:
//    "dest" is modified to point to the newly allocated and copied result.
//
// ^RETURN-VALUE:
//    None.
//
// ^ALGORITHM:
//    Trivial.
//-^^----------------
static  void
copy(char * & dest, const char * src)
{
   if (src == NULL)  return ;
   dest = new char[::strlen(src) + 1] ;
   if (dest)  ::strcpy(dest, src);
}

//------------------------------------------------------------------ CmdArgVers

CmdArgVers::CmdArgVers(char opt, const char * kwd, const char * description)
   : CmdArg(opt, kwd, description, CmdArg::isOPT)
{
}

CmdArgVers::~CmdArgVers()
{
}

int
CmdArgVers::operator()(const char * & , CmdLine & cmd)
{
   std::cerr << cmd.name() << "\trelease " << cmd.release()
        << " at patchlevel " << cmd.patchlevel() << std::endl ;
   cmd.quit(e_VERSION);
   return  0;  // shutup the compiler about not returning a value
}

//------------------------------------------------------------- CmdParseCommand

//-------------------
// ^FUNCTION: CmdParseCommand::parse_declarations - parse user arguments
//
// ^SYNOPSIS:
//    CmdParseCommand::parse_declarations()
//
// ^PARAMETERS:
//    None.
//
// ^DESCRIPTION:
//    This routine will go through all the input sources that were supplied
//    on the command-line. Each of these "input sources" is something that
//    contains user argument declarations that need to be parsed.  If no
//    input sources were given and cin is not connected to a terminal, then
//    we try to read the declarations from cin.
//
//    If input sources were given, they are parsed in the following order:
//       1) from a string
//       2) from an environment variable
//       3) from a file
//
//    If more than one input source is specified then they are processed in
//    the above order and each argument is appended to the user's CmdLine
//    object in the order that it was seen.
//
// ^REQUIREMENTS:
//    This routine should be called by CmdParseCommand::operator() after
//    the command-line has been parsed.
//
// ^SIDE-EFFECTS:
//    If input comes from a file, then the file is read (until eof or an
//    error condition occurs).
//
//    Any arguments found are "compiled" and appended to "usr_cmd", the user's
//    CmdLine object.
//
// ^RETURN-VALUE:
//    0 upon success, non-zero upon failure.
//
// ^ALGORITHM:
//    Follow along - its pretty straightforward.
//-^^----------------
int
CmdParseCommand::parse_declarations()
{
   const char * str = input_str ;
   const char * varname = input_var ;
   const char * filename = input_file ;

      // If no "input sources" were specified, try std::cin.
   if ((str == NULL)  &&  (varname == NULL)  &&  (filename == NULL)) {
      // Make sure std::cin is NOT interactive!
      if (::isatty(0)) {
         error() << "Can't read argument declarations from a terminal."
                 << std::endl ;
         return  -1;
      }
      return  parse_declarations(std::cin);
   }

   int  rc = 0;

      // First - parse from the string
   if (str) {
      rc += parse_declarations(str);
   }

      // Second - parse from the environment variable
   if (varname) {
      const char * contents = ::getenv(varname);
      if (contents) {
         rc += parse_declarations(contents);
      } else {
         error() << varname << " is empty or is undefined." << std::endl ;
         return  -1;
      }
   }

      // Third - parse from the file. If the filename is "-" then it
      //         means that standard input should be used.
      //
   if (filename) {
      if (::strcmp(filename, "-") == 0) {
         rc += parse_declarations(std::cin);
      } else {
         std::ifstream ifs (filename) ;
         if (ifs) {
            rc += parse_declarations(ifs);
         } else {
            error() << "Unable to read from " << filename << '.' << std::endl ;
            return  -1;
         }
      }
   }

   return  rc;
}


//-------------------
// ^FUNCTION: CmdParseCommand::usr_append - add a user argument
//
// ^SYNOPSIS:
//    CmdParseCommand::usr_append(type, varname, arg, description)
//
// ^PARAMETERS:
//    const char * type;
//    -- the name of the desired argument type (which should correspond
//       to either CmdArgUsage, CmdArgDummy, or one of the types defined
//       in "argtypes.h").
//
//    const char * varname;
//    -- the name of the shell variable that will be receiving the value
//       that was supplied to this argument on the command-line.
//
//    ArgSyntax & arg;
//    -- the argument syntax corresponding to the "syntax-string" supplied
//       by the user.
//
//    const char * description;
//    -- the user's description of this argument.
//
// ^DESCRIPTION:
//    This member function will create a corresponding instance of a derived
//    class of CmdArg named by "type" and will append it to the user's CmdLine
//    object (named usr_cmd).
//
//    "type" may be one of the following (the leading "CmdArg" prefix may be
//    omitted):
//
//       CmdArgInt, CmdArgFloat, CmdArgChar, CmdArgStr, CmdArgBool,
//       CmdArgSet, CmdArgClear, CmdArgToggle, CmdArgUsage, CMdArgDummy
//
//    It is NOT necessary to use the name of a "List" CmdArg type in order
//    to indicate a list, that will have been inferred from the syntax string
//    and the appropriate ShellCmdArg<Type> object that we create will know
//    how to handle it.
//
// ^REQUIREMENTS:
//    This function should be called after an argument declaration has been
//    completely parsed.
//
// ^SIDE-EFFECTS:
//    If "type" is invalid - an error is printed on std::cerr, otherwise
//    we create an appopriate CmdArg<Type> object and append it to usr_cmd.
//
// ^RETURN-VALUE:
//    0 for success; non-zero for failure.
//
// ^ALGORITHM:
//    Trivial - there's just a lot of type-names to check for.
//-^^----------------
int
CmdParseCommand::usr_append(const char * type,
                            const char * varname,
                            ArgSyntax  & arg,
                            const char * description)
{
   char * name = NULL ;
   char * kwd  = NULL ;
   char * val  = NULL ;
   char * desc = NULL ;
   unsigned  flags = arg.syntax() ;
   char  opt = arg.optchar() ;

      // Need to make copies of some things because we cant assume they
      // will be sticking around.  We assume that ShellCmdArg::~ShellCmdArg
      // will deallocate this storage.
      //
   ::copy(name, varname);
   ::copy(kwd,  arg.keyword());
   ::copy(val,  arg.value());
   ::copy(desc, description);

       // Skip any leading "Cmd", "Arg", or "CmdArg" prefix in the type-name.
   if (CmdLine::strmatch("Cmd", type, 3) == CmdLine::str_EXACT)  type += 3;
   if (CmdLine::strmatch("Arg", type, 3) == CmdLine::str_EXACT)  type += 3;

       // Create an argument for the appropriate type and append it
       // to usr_cmd.
       //
   if (CmdLine::strmatch("Usage", type) == CmdLine::str_EXACT) {
      delete [] name ;
      usr_cmd.append(new CmdArgUsage(opt, kwd, desc)) ;
   }
   else if (CmdLine::strmatch("Dummy", type) == CmdLine::str_EXACT) {
      delete [] name ;
      usr_cmd.append(new CmdArgDummy(opt, kwd, val, desc, flags));
   }
   else if (CmdLine::strmatch("Set", type) == CmdLine::str_EXACT) {
      usr_cmd.append(new ShellCmdArgBool(name, opt, kwd, desc, flags));
   }
   else if (CmdLine::strmatch("Clear", type) == CmdLine::str_EXACT) {
      usr_cmd.append(new ShellCmdArgClear(name, opt, kwd, desc, flags));
   }
   else if (CmdLine::strmatch("Toggle", type) == CmdLine::str_EXACT) {
      usr_cmd.append(new ShellCmdArgToggle(name, opt, kwd, desc, flags));
   }
   else if (CmdLine::strmatch("Boolean", type) != CmdLine::str_NONE) {
      usr_cmd.append(new ShellCmdArgBool(name, opt, kwd, desc, flags));
   }
   else if (CmdLine::strmatch("Integer", type) != CmdLine::str_NONE) {
      usr_cmd.append(new ShellCmdArgInt(name, opt, kwd, val, desc, flags));
   }
   else if (CmdLine::strmatch("Float", type) != CmdLine::str_NONE) {
      usr_cmd.append(new ShellCmdArgFloat(name, opt, kwd, val, desc, flags));
   }
   else if (CmdLine::strmatch("Character", type) != CmdLine::str_NONE) {
      usr_cmd.append(new ShellCmdArgChar(name, opt, kwd, val, desc, flags));
   }
   else if (CmdLine::strmatch("String", type) != CmdLine::str_NONE) {
      usr_cmd.append(new ShellCmdArgStr(name, opt, kwd, val, desc, flags));
   }
   else {
      std::cerr << "Unknown argument type \"" << type << "\"." << std::endl ;
      delete [] kwd ;
      delete [] val ;
      delete [] desc ;
      return  FAILURE ;
   }

   return  SUCCESS ;
}


//-------------------
// ^FUNCTION: CmdParseCommand::parse_declarations - parse from a string
//
// ^SYNOPSIS:
//    CmdParseCommand::parse_declarations(str);
//
// ^PARAMETERS:
//    const char * str;
//    -- the string containing the argument declarations.
//
// ^DESCRIPTION:
//    Parse the user's argument declarations from an input string.
//
// ^REQUIREMENTS:
//    This member function should only be called by parse_declarations().
//
// ^SIDE-EFFECTS:
//    - modifies usr_cmd by appending to it any valid arguments that we parse.
//
// ^RETURN-VALUE:
//    0 for success; non-zero for failure.
//
// ^ALGORITHM:
//    Just turn the string into an instream and parse the instream.
//-^^----------------
int
CmdParseCommand::parse_declarations(const char * str)
{
   std::istringstream  iss(str);
   return parse_declarations(iss);
}

//-------------------
// ^FUNCTION: CmdParseCommand::parse_declarations - parse from an instream
//
// ^SYNOPSIS:
//    CmdParseCommand::parse_declarations(is);
//
// ^PARAMETERS:
//    std::istream & is;
//    -- the instream containing the argument declarations.
//
// ^DESCRIPTION:
//    Parse the user's argument declarations from an input steam.
//
// ^REQUIREMENTS:
//    This member function should only be called by parse_declarations().
//
// ^SIDE-EFFECTS:
//    - modifies usr_cmd by appending to it any valid arguments that we parse.
//
// ^RETURN-VALUE:
//    0 for success; non-zero for failure.
//
// ^ALGORITHM:
//    while not eof do
//       - read the type
//       - read the name
//       - read the syntax
//       - read the description
//       - convert (type, name, syntax, description) into something we can
//           append to usr_cmd.
//    done
//-^^----------------
int
CmdParseCommand::parse_declarations(std::istream & is)
{
      // Keep track of the number of declarations that we parse.
   unsigned  nargs = 0;

   if (is.eof())  return  SUCCESS;

   char  arg_type[MAX_IDENT_LEN];
   char  arg_name[MAX_IDENT_LEN];
   QuotedString  arg_description(MAX_DESCRIPTION_LEN);

   while (is) {
      ++nargs;

         // Skip all non-alpha-numerics
     int c = is.peek() ;
     while ((c != EOF) && (c != '_') && (! isalnum(c))) {
        (void) is.get();
        c = is.peek();
     }

         // First parse the argument type
      is.width(sizeof(arg_type) - 1);
      is >> arg_type ;
      if (! is) {
         if (is.eof()) {
            return  SUCCESS;  // end of args
         } else {
            error() << "Unable to extract type for argument #" << nargs
                    << '.' << std::endl ;
            return  FAILURE;
         }
      }

         // Now parse the argument name
      is.width(sizeof(arg_name) - 1);
      is >> arg_name ;
      if (! is) {
         if (is.eof()) {
            error() << "Premature end of input.\n"
                    << "\texpecting a name for argument #" << nargs
                    << '.' << std::endl ;
         } else {
            error() << "Unable to extract name of argument #" << nargs
                    << '.' << std::endl ;
         }
         return  FAILURE;
      }

         // Now parse the argument syntax
      ArgSyntax  arg;
      is >> arg;
      if (! is) {
         error() << "Unable to get syntax for \"" << arg_name << "\" argument."
                 << std::endl ;
         return  FAILURE ;
      }

         // Now parse the argument description
      is >> arg_description ;
      if (! is) {
         error() << "Unable to get description for \"" << arg_name
                 << "\" argument." << std::endl ;
         return  FAILURE ;
      }

      if (usr_append(arg_type, arg_name, arg, arg_description)) {
         error() << "Unable to append \"" << arg_name << "\" argument "
                 << "to the list." << std::endl ;
         return  FAILURE;
      }
   }
   return  SUCCESS;
}


//-------------------
// ^FUNCTION: CmdParseCommand::set_args - set the user's arguments.
//
// ^SYNOPSIS:
//    CmdParseCommand::set_args(shell)
//
// ^PARAMETERS:
//    UnixShell * shell;
//    -- the command-interpreter (shell) that we need to output
//       variable settings for.
//
// ^DESCRIPTION:
//    For each argument that was given on the user's command-line, we need to
//    output a variable setting using the sepcified shell's syntax to indicate
//    the value that was specified.
//
// ^REQUIREMENTS:
//    This member function should only be called by CmdParseCommand::operator()
//
// ^SIDE-EFFECTS:
//    All the variable settings or sent to std::cout (standard output), the invoking
//    user is responsible for evaluating what this member function outputs.
//
// ^RETURN-VALUE:
//    None.
//
// ^ALGORITHM:
//    For each of the user's command-argument objects
//       - if the argument is a dummy, then skip it
//       - if the argument corresponds to the positional parameters for
//           this shell but was not given a value on the command-line then
//           unset this shell's positional parameters.
//       - if the argument was given then
//           - if the argument took a value and no value was given, then
//               set the variable <argname>_FLAG to be TRUE.
//           - else set the corresponding shell variable.
//           endif
//       endif
//    endfor
//-^^----------------
void
CmdParseCommand::set_args(UnixShell * shell)
{
   unsigned  flags, syntax;
   CmdLineCmdArgIter  iter(usr_cmd);

   for (CmdArg * cmdarg = iter() ; cmdarg ; cmdarg = iter()) {
      flags  = cmdarg->flags();
      syntax = cmdarg->syntax();

      if (cmdarg->is_dummy())  continue;

      ShellCmdArg * sh_cmdarg = (ShellCmdArg *)cmdarg;

      if ((syntax & CmdArg::isPOS) && (! (flags & CmdArg::VALGIVEN))) {
         // if these are the positional-parameters then unset them!
         if (shell->is_positionals(sh_cmdarg->name())) {
            shell->unset_args(sh_cmdarg->name());
         }
      }

      if (! (flags & CmdArg::GIVEN))  continue;

      if ((syntax & CmdArg::isVALTAKEN) && (! (flags & CmdArg::VALGIVEN))) {
         // flag was given without its value - we need to record that
         char  var_name[256];
         (void) ::strcpy(var_name, sh_cmdarg->name());
         (void) ::strcat(var_name, suffix_str);
         ShellVariable  sh_var(var_name);
         sh_var.set(ShellCmdArgBool::True());
         shell->set(sh_var);
      } else {
         // output the value
         if (sh_cmdarg->is_array()) {
            shell->set(sh_cmdarg->array(), array_variant);
         } else {
            shell->set(sh_cmdarg->variable());
         }
      }
   } //for
}


//-------------------------------------------- CmdParseCommand::CmdParseCommand

CmdParseCommand::CmdParseCommand(const char * name)
   : CmdLine(name),
     anywhere('a', "anywhere",
        "Allow options (and keywords) to follow positional parameters."
     ),
     anycase('i', "ignore-case",
        "Ignore character case on options."
     ),
     no_abort('n', "noabort",
        "Dont exit if bad syntax; try to continue parsing."
     ),
     guess('g', "guess",
        "Try to \"guess\" for unmatched options/keywords."
     ),
     prompt('p', "prompt",
        "Prompt the user interactively for any missing required arguments."
     ),
     plus('+', "plus",
        "Allow the string \"+\" to be used as a long-option prefix."
     ),
     opts_only('o', "options-only",
        "Dont match keywords (long-options)."
     ),
     kwds_only('k', "keywords-only",
        "Dont match options."
     ),
     quiet('q', "quiet",
        "Dont print command-line syntax error messages."
     ),
     array_variant('A', "arrays",
        "Use alternative syntax for arrays."
     ),
     usage('u', "usage",
        "Print command-line usage and exit."
     ),
     version('v', "version",
        "Print version information and exit."
     ),
     true_str('T', "true", "string",
        "The string to use for boolean arguments that are turned ON \
(default=\"TRUE\")."
     ),
     false_str('F', "false", "string",
        "The string to use for boolean arguments that are turned OFF \
(default=\"\")."
     ),
     suffix_str('S', "suffix", "string",
        "The suffix to use for missing optional values. (default=\"_FLAG\")."
     ),
     usr_shell('s', "shell", "shellname",

        "Set program arguments using the syntax of the given shell \
(default=\"sh\")."
     ),
     input_file('f', "file", "filename",
        "The file from which program argument declarations are read."
     ),
     input_var('e', "env", "varname",
        "The environment variable containing the program argument declarations."
     ),
     input_str('d', "decls", "string",
        "The string that contains the program argument declarations."
     ),
     dummy_arg("--",
        "Indicates the end of options/keywords."
     ),
     usr_prog('N', "name", "program-name",
        "The name of the program whose arguments are to be parsed.",
        (CmdArg::isPOS | CmdArg::isREQ | CmdArg::isVALREQ)
     ),
     usr_args("[arguments ...]",
        "The program-arguments to be parsed"
     )
{
      // Append options.
   (*this) << anywhere << anycase << no_abort << guess << prompt << plus
           << opts_only << kwds_only << quiet << array_variant << usage
           << version << true_str << false_str << suffix_str << usr_shell
           << input_file << input_var << input_str ;

      // Append positional parameters.
   (*this) << usr_prog << dummy_arg << usr_args ;

   set(CmdLine::KWDS_ONLY);

      // Set up defaults
   usr_shell  = "sh" ;
   true_str   = "TRUE" ;
   false_str  = "" ;
   suffix_str = "_FLAG" ;
}

//------------------------------------------- CmdParseCommand::~CmdParseCommand

CmdParseCommand::~CmdParseCommand()
{
   CmdLineCmdArgIter  iter(usr_cmd);

   for (CmdArg * cmdarg = iter() ; cmdarg ; cmdarg = iter()) {
      delete  cmdarg ;
   }
}


//-------------------
// ^FUNCTION: CmdParseCommand::operator()
//
// ^SYNOPSIS:
//    CmdParseCommand::operator(iter)
//
// ^PARAMETERS:
//    CmdLineArgIter & iter;
//    -- an object to iterate over the arguments on the command-line.
//
// ^DESCRIPTION:
//    This member function is the "guts" of a CmdParseCommand object.
//    We perform all the actions necessary to read the user's argument
//    declaratins, read the user's command-line, and set the corresponding
//    shell variables.
//
// ^REQUIREMENTS:
//    None.
//
// ^SIDE-EFFECTS:
//    - Modifies all parts of the corresponding CmdParseCommand object.
//    - prints variable settings on std::cout
//    - prints usage/error messages on std::cerr
//
// ^RETURN-VALUE:
//    e_SUCCESS   --  no errors
//    e_USAGE     --  no errors - usage printed
//    e_VERSION   --  no errors - version printed
//    e_CMDSYNTAX --  command-line syntax error
//    e_BADSHELL  --  invalid shell specified
//    e_BADDECLS  --  invalid declaration(s) given
//
// ^ALGORITHM:
//    It gets complicated so follow along.
//-^^----------------
int
CmdParseCommand::operator()(CmdLineArgIter & iter)
{
      // Parse arguments
   parse(iter);

      // Use the specified shell
   UnixShell * shell = new UnixShell(usr_shell);
   if (! shell->is_valid()) {
      delete  shell ;
      error() << "\"" << usr_shell
              << "\" is not a known command interpreter." << std::endl ;
      return  e_BADSHELL ;
   }

      // Handle "-true" and "-false" options
   if (true_str.flags()  & CmdArg::GIVEN)  ShellCmdArgBool::True(true_str);
   if (false_str.flags() & CmdArg::GIVEN)  ShellCmdArgBool::False(false_str);

      // Intitialize user's command-line
   usr_cmd.name(usr_prog);
   if (parse_declarations())  return  e_BADDECLS ;

      // Set user parsing preferences
   if (anywhere)     usr_cmd.clear(CmdLine::OPTS_FIRST);
   if (anycase)      usr_cmd.set(CmdLine::ANY_CASE_OPTS);
   if (no_abort)     usr_cmd.set(CmdLine::NO_ABORT);
   if (guess)        usr_cmd.set(CmdLine::GUESS);
   if (prompt)       usr_cmd.set(CmdLine::PROMPT_USER);
   if (plus)         usr_cmd.set(CmdLine::ALLOW_PLUS);
   if (opts_only)    usr_cmd.set(CmdLine::OPTS_ONLY);
   if (kwds_only)    usr_cmd.set(CmdLine::KWDS_ONLY);
   if (quiet)        usr_cmd.set(CmdLine::QUIET);

      // Just print usage if thats all that is desired
   if (usage) {
      usr_cmd.usage(std::cout);
      return  e_USAGE ;
   }

      // Parse user's command-line
   usr_cmd.prologue();
   for (int i = 0 ; i < usr_args.count() ; i++) {
      usr_cmd.parse_arg(usr_args[i]);
   }
   usr_cmd.epilogue();

      // Set user's variables
   set_args(shell);

   delete  shell ;
   return  0;
}
