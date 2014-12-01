//------------------------------------------------------------------------
// ^FILE: usage.c - functions to print the usage of a CmdLine
//
// ^DESCRIPTION:
//     This file contains the functions that are used to print the
//  command-line usage of a command that is represented by a CmdLine
//  object.
//
// ^HISTORY:
//    01/09/92	Brad Appleton	<bradapp@enteract.com>	Created
//
//    03/01/93	Brad Appleton	<bradapp@enteract.com>
//    - Added cmd_description field to CmdLine
//-^^---------------------------------------------------------------------

#include <iostream>
#include <algorithm>
#include <stdlib.h>
#include <string.h>

#include "cmdline.h"
#include "states.h"
#include "arglist.h"

#ifdef vms
#  define  getenv  getsym
   extern  const char * getsym(const char *);
#endif

static const int PRINT_LMARGIN = 2 ;
static const int PRINT_MAXCOLS = 79 ;

static int term_columns()
{
  return PRINT_MAXCOLS ;
}

//-------
// ^FUNCTION: CmdLine::get_usage_level
//
// ^SYNOPSIS:
//    CmdLine::CmdUsageLevel CmdLine::get_usage_level()
//
// ^PARAMETERS:
//    NONE.
//
// ^DESCRIPTION:
//    Gets the usage_level that tells us how "verbose" we should be
//    when printing usage-messages. This usage_level is recorded in
//    the environment variable $USAGE_LEVEL. This variable may have the
//    following values:
//
//       0 : Dont print usage at all.
//       1 : Print a terse usage message (command-line syntax only).
//       2 : Print a verbose usage message (include argument descriptions).
//
//    If $USAGE_LEVEL is not defined or is empty, then the default
//    usage_level is 2.
//
// ^REQUIREMENTS:
//
// ^SIDE-EFFECTS:
//    None.
//
// ^RETURN-VALUE:
//    The usage level to use.
//
// ^ALGORITHM:
//    Read the usage_level from the environment and return it.
//-^^----
CmdLine::CmdUsageLevel
CmdLine::get_usage_level() const
{
   if (cmd_usage_level != DEFAULT_USAGE)
      return cmd_usage_level ;

   long level;
   char * end_scan, * level_str = ::getenv("USAGE_LEVEL");

   if (level_str == NULL)  return  VERBOSE_USAGE ;
   if (*level_str == '\0') return  NO_USAGE ;

   level = ::strtol(level_str, &end_scan, 0);
   if (end_scan == level_str)  return  VERBOSE_USAGE ;

   switch(level) {
      case 0 :  return NO_USAGE ;
      case 1 :  return TERSE_USAGE ;
      default:  return VERBOSE_USAGE ;
   }
}

//-------
// ^FUNCTION: CmdLine::print_synopsis
//
// ^SYNOPSIS:
//    unsigned CmdLine::print_synopsis(syntax, os, cols)
//
// ^PARAMETERS:
//    CmdLine::CmdLineSyntax syntax;
//    -- the syntax to use (long-option, short-option, or both)
//       when printing the synopsis.
//
//    std::ostream & os;
//    -- where to print.
//
//    int cols;
//    -- the maximum width of a line.
//
// ^DESCRIPTION:
//    Print a command-line synopsis (the command-line syntax).
//    The synopsis should be printed to "os" using the desired syntax,
//    in lines that are no more than "cols" characters wide.
//
// ^REQUIREMENTS:
//
// ^SIDE-EFFECTS:
//     Prints on "os".
//
// ^RETURN-VALUE:
//     The length of the longest argument-buf that was printed.
//
// ^ALGORITHM:
//     It's kind of complicated so follow along!
//-^^----
unsigned
CmdLine::print_synopsis(CmdLine::CmdLineSyntax  syntax,
                        std::ostream    & os,
                        int          cols) const
{
#ifdef vms_style
   static  char  usg_prefix[] = "Format: ";
#else
   static  char  usg_prefix[] = "Usage: ";
#endif

   unsigned  ll, positionals, longest = 0;

   // first print the command name
   os << usg_prefix << cmd_name ;
   ll = (cmd_name ? ::strlen(cmd_name) : 0) + (sizeof(usg_prefix) - 1);

   // set margin so that we always start printing arguments in a column
   // that is *past* the command name.
   //
   unsigned  margin = ll + 1;

   // print option-syntax followed by positional parameters
   int  first;
   char buf[256] ;
   for (positionals = 0 ; positionals < 2 ; positionals++) {
      first = 1;
      CmdArgListListIter  list_iter(cmd_args);
      for (CmdArgList * alist = list_iter() ; alist ; alist = list_iter()) {
         CmdArgListIter  iter(alist);
         for (CmdArg * cmdarg = iter() ; cmdarg ; cmdarg = iter()) {
            unsigned  len, pl;

               // don't display hidden arguments
            if (cmdarg->syntax() & CmdArg::isHIDDEN)  continue;
            if (!positionals  &&  (cmdarg->syntax() & CmdArg::isPOS))  continue;
            if (positionals  &&  !(cmdarg->syntax() & CmdArg::isPOS))  continue;

               // figure out how wide this parameter is (for printing)
            pl = len = fmt_arg(cmdarg, buf, sizeof(buf), syntax, VERBOSE_USAGE);
            if (! len)  continue;

            if (cmdarg->syntax() & CmdArg::isLIST)  pl -= 4 ;  // " ..."
            if (! (cmdarg->syntax() & CmdArg::isREQ))  pl -= 2 ;   // "[]"
            if (pl > longest)  longest = pl;

            //  Will this fit?
            if ((long)(ll + len + 1) > (cols - first)) {
               os << char('\n') ;
               os.width(margin);
               os << "" ;  // No - start a new line;
               ll = margin;
            } else {
               os << char(' ') ;  // Yes - just throw in a space
               ++ll;
            }
            ll += len;
            os << buf;

            first = 0;
         } //for each cmdarg
      } //for each arg-list
   } //for each parm-type
   os << std::endl ;

   return  longest ;
}


//-------
// ^FUNCTION: CmdLine::print_descriptions
//
// ^SYNOPSIS:
//    unsigned CmdLine::print_descriptions(syntax, os, cols, longest)
//
// ^PARAMETERS:
//    CmdLine::CmdLineSyntax syntax;
//    -- the syntax to use (long-option, short-option, or both)
//       when printing the synopsis.
//
//    std::ostream & os;
//    -- where to print.
//
//    int cols;
//    -- the maximum width of a line.
//
//    unsigned longest;
//    -- value returned by print_synopsis.
//
// ^DESCRIPTION:
//    Print a command argument descriptions (using the command-line syntax).
//    The descriptions should be printed to "os" using the desired syntax,
//    in lines that are no more than "cols" characters wide.
//
// ^REQUIREMENTS:
//    "longest" should correspond to a value returned by print_synopsis
//    that used the same "cmd" and syntax.
//
// ^SIDE-EFFECTS:
//     Prints on "os".
//
// ^RETURN-VALUE:
//     None.
//
// ^ALGORITHM:
//     Print the description for each argument.
//-^^----
void
CmdLine::print_descriptions(CmdLine::CmdLineSyntax   syntax,
                            std::ostream                & os,
                            int                      cols,
                            unsigned                 longest) const
{
  static const unsigned argarray_size = 128 ;
  typedef const CmdArg *argarray[argarray_size] ;

  struct local {
    static int copy_args(CmdArgListList *arglist, bool positional, argarray &argv)
    {
      unsigned argc = 0 ;
      CmdArgListListIter list_iter(arglist) ;
      for (CmdArgList * alist = list_iter() ; argc < argarray_size && alist ; alist = list_iter()) {
         CmdArgListIter  iter(alist) ;

         for (CmdArg * cmdarg = iter() ; cmdarg ; cmdarg = iter()) {
               // don't display hidden arguments
            if ((cmdarg->syntax() & CmdArg::isHIDDEN) ||
                !positional != !(cmdarg->syntax() & CmdArg::isPOS))  continue ;

            const char * const description = cmdarg->description() ;
            if (description && *description)
              argv[argc++] = cmdarg ;
         } //for each cmdarg
      } //for each arg-list
      return argc ;
    }
    static bool less_args(const CmdArg *left, const CmdArg *right)
    {
      return left->char_name() == right->char_name()
        ?
        left->keyword_name() != right->keyword_name() &&
        (!left->keyword_name() || (right->keyword_name() && strcmp(left->keyword_name(), right->keyword_name()) < 0))
        :
        (unsigned char)(left->char_name() - 1) < (unsigned char)(right->char_name() - 1) ;
    }
  } ;

  static const char arghead[] =
#ifdef vms_style
    "Qualifiers/Parameters:\n"
#else
    "Options/Arguments:\n" ;
#endif

  int has_args = 0 ;

  for (int is_positional = 0 ; is_positional <= 1 ; ++is_positional) {
    argarray argv ;
    const int argcnt = local::copy_args(cmd_args, is_positional, argv) ;
    std::sort(argv + 0, argv + argcnt, local::less_args) ;

    char title_buf[256] ;
    char defval_str[512] = "[ default: " ;
    static const char defval_end[] = " ]" ;

    char * const defval_buf = strchr(defval_str, 0) ;
    const size_t defval_sz = sizeof defval_str - (defval_buf - defval_str) - (sizeof defval_end - 1) ;
    const unsigned indent = longest + 2 ;

    for (int n = 0 ; n < argcnt ; ++n)
      if (fmt_arg(argv[n], title_buf, sizeof title_buf, syntax, TERSE_USAGE)) {
        if (!has_args++)
          os << arghead ;
        strindent(os, cols, PRINT_LMARGIN, title_buf, indent, argv[n]->description()) ;
        // Output default value, if nonzero
        if (argv[n]->valstr(defval_buf, defval_sz, CmdArg::VALSTR_DEFNOZERO)) {
          strcat(defval_buf, defval_end) ;
          strindent(os, cols, PRINT_LMARGIN, NULL, indent, defval_str) ;
        }
      }
  }
}

//-------
// ^FUNCTION: CmdLine::usage - print command-usage
//
// ^SYNOPSIS:
//    void CmdLine::usage(os, usage_level);
//
// ^PARAMETERS:
//    std::ostream & os;
//    -- where to print.
//
//    CmdLine::CmdUsageLevel  usage_level;
//    -- verboseness to use.
//
// ^DESCRIPTION:
//    Print the usage for the given CmdLine object on "os".
//
// ^REQUIREMENTS:
//
// ^SIDE-EFFECTS:
//    Prints on "os".
//
// ^RETURN-VALUE:
//    None.
//
// ^ALGORITHM:
//    - get the usage level.
//    - determine which syntax to use
//    - get the max-columns for "os".
//    - print synopsis if required.
//    - print descriptions if required.
//-^^----
std::ostream &CmdLine::usage(std::ostream & os, CmdUsageLevel usage_level) const
{
   // get user-specified usage-level
   //   (if status is zero this must be an explicit request so force verbose)
   //
   if (usage_level == DEFAULT_USAGE)  usage_level = get_usage_level();
   if (usage_level == NO_USAGE)  return  os;

   // determine syntax to use
   const CmdLineSyntax cmd_syntax = usage_syntax() ;

   // get screen size (dont know how to do this yet)
   const int max_cols = term_columns() - 1 ;

   // print command-line synopsis
   unsigned  longest = print_synopsis(cmd_syntax, os, max_cols) ;
   if (usage_level == TERSE_USAGE)  return  os;

   // now print the short command description, if there is one
   if (*description())
      strindent(os, max_cols, 0, "", 0, description());

   // now print argument descriptions
   print_descriptions(cmd_syntax, os << '\n', max_cols, longest) ;

   // now print the full description, if there is one
   if (*full_description())
     strindent(os << '\n', max_cols, 0, "", 0, full_description());

   return  os;
}

std::ostream &CmdLine::usage(CmdUsageLevel usage_level) const
{
   return  usage(*cmd_err, usage_level);
}
