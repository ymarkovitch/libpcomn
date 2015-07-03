/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)) -*-*/
//------------------------------------------------------------------------
// ^FILE: private.cpp - private/protected functions used by the CmdLine library
//
// ^DESCRIPTION:
//  This file implements functions that are for the exclusive use of
//  the CmdLine library.  The following functions are implemented:
//
//      ck_need_val()  --  see if we left an argument without a value
//      handle_arg()   --  compile the string value of an argument
//      syntax()       --  find out the desired syntax for usage messages
//      missing_args() --  check for missing required arguments
//      opt_match()    --  match an option
//      kwd_match()    --  match a keyword
//      pos_match()    --  match a positional parameter
//
// ^HISTORY:
//    01/09/92	Brad Appleton	<bradapp@enteract.com>	Created
//
//    03/03/93	Brad Appleton	<bradapp@enteract.com>
//    - Added exit_handler() and quit() member-functions to CmdLine
//-^^---------------------------------------------------------------------

#include <iostream>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

extern "C" {
  int  isatty(int fd);

#ifdef GNU_READLINE
# include <readline.h>
#endif

}

#include "cmdline.h"
#include "states.h"
#include "fifolist.h"

#ifdef vms
#  define  getenv  getsym
   extern  const char * getsym(const char *);
#endif

//-------
// ^FUNCTION: CmdLine::handle_arg - compile the string value of an argument
//
// ^SYNOPSIS:
//    extern int CmdLine::handle_arg(cmdarg, arg);
//
// ^PARAMETERS:
//    CmdArg * cmdarg;
//    -- the matched argument whose value is to be "handled"
//
//    const char * & arg;
//    -- the string value for the argument (from the command-line).
//       upon exit, this will be NULL (if all of "arg" was used) or will
//       point to the first character or "arg" that was not used by the
//       argument's "compile" function.
//
// ^DESCRIPTION:
//    After we have matched an argument on the command-line to an argument
//    in the "cmd" object, we need to "handle" the value supplied for that
//    argument. This entails updating the state of the argument and calling
//    its "compile" function as well as updating the state of the command.
//
// ^REQUIREMENTS:
//    None that weren't covered in the PARAMETERS section.
//
// ^SIDE-EFFECTS:
//    - modifies the value pointed to by "arg"
//    - prints a message on stderr if "arg" is invalid and QUIET is NOT set.
//    - modifies the state of "cmd".
//    - modifies the "value" and "flags" of "cmdarg".
//
// ^RETURN-VALUE:
//    The value returned by calling the "compile" function associated
//    with the argument "cmdarg".
//
// ^ALGORITHM:
//    - if this is a cmdargUsage argument then print usage and call quit()
//    - call the operator() of "cmdarg" and save the result.
//    - if the above call returned SUCCESS then set the GIVEN and VALGIVEN
//      flags of the argument.
//    - update the parse_state of "cmd" if we were waiting for this value.
//    - if "cmdarg" corresponds to a LIST then set things up so that succeeding
//      arguments will be values for this "cmdarg"'s list.
//-^^----
int
CmdLine::handle_arg(CmdArg * cmdarg, const char * & arg)
{
   ++cmd_nargs_parsed;  // update the number of parsed args

   // call the argument compiler
   const char * save_arg = arg ;  // just in case someone forgets to set it
   int  bad_val = (*cmdarg)(arg, *this);
   if (! bad_val) {
      cmdarg->set_flags(CmdArg::GIVEN) ;
      cmdarg->sequence(cmd_nargs_parsed) ;
      if (arg != save_arg) {
         cmdarg->set_flags(CmdArg::VALGIVEN) ;
      }
   }

   // if we were waiting for a value - we just got it
   if (arg != save_arg) {
      if (cmdarg == cmd_matched_arg)  cmd_parse_state = cmd_START_STATE ;
   }

   // if this is a positional list argument - optional values may follow the one given
   if ((cmdarg->syntax() & (CmdArg::isLIST | CmdArg::isPOS)) == (CmdArg::isLIST | CmdArg::isPOS) && (arg != save_arg)) {
      cmd_matched_arg = cmdarg ;
      cmd_parse_state = cmd_WANT_VAL ;
   }

   return  bad_val ;
}


//-------
// ^FUNCTION: CmdLine::ck_need_val - See if an argument needs a value
//
// ^SYNOPSIS:
//    extern void CmdLine::ck_needval()
//
// ^PARAMETERS:
//    NONE.
//
// ^DESCRIPTION:
//    We parse command-lines using something akin to a deterministic
//    finite state machine. Each argv[] element on the command-line is
//    considered a single input to the machine and we keep track of an
//    associated machine-state that tells us what to do next for a given
//    input.
//
//    In this function, we are merely trying to query the "state" of the
//    machine by asking it if it is expecting to see a value for an
//    argument that was matched in a previous argv[] element.
//
//    It is assumed that this function is called only after it has already
//    been determined that the current argv[] element is NOT an argument
//    value.
//
// ^REQUIREMENTS:
//
// ^SIDE-EFFECTS:
//    - updates the "state" of the command.
//    - updates the "status" of the command.
//    - modifies the last matched argument if it takes an optional value.
//    - prints a message on stderr if cmd_QUIET is NOT set and we were
//      expecting a required value.
//
// ^RETURN-VALUE:
//    None.
//
// ^ALGORITHM:
//    If we were expecting an optional value then
//    - set the GIVEN flag of the last arg we matched (DO NOT set VALGIVEN).
//    - call the compiler-function of the last-matched arg using NULL
//      as the argument (unless the arg is a LIST and VALGIVEN is set).
//    - reset the command-state
//    Else if we were expecting a required value then
//    - print a an error message if cmd_QUIET is not set
//    - set the command-status to VAL_MISSING
//    - reset the command-state
//    Endif
//-^^----
void
CmdLine::ck_need_val()
{
   const char * null_str = NULL;
   if (cmd_parse_state == cmd_WANT_VAL) {
      // argument was given but optional value was not
      cmd_matched_arg->set_flags(CmdArg::GIVEN) ;
      if ((! (cmd_matched_arg->syntax() & CmdArg::isLIST)) ||
          (! (cmd_matched_arg->flags()  & CmdArg::VALGIVEN))) {
         (void) handle_arg(cmd_matched_arg, null_str) ;
      }
      cmd_parse_state = cmd_START_STATE ;
   } else if (cmd_parse_state == cmd_NEED_VAL) {
      // argument was given but required value was not
      if (! (cmd_flags & QUIET)) {
         arg_error("value required for", cmd_matched_arg) << "." << std::endl ;
      }
      cmd_status |= VAL_MISSING ;
      cmd_parse_state = cmd_START_STATE ;
   }
}


#ifndef GNU_READLINE
//
// readline() -- indigent person's version of the GNU readline() function
//

#define PROMPT_BUFSIZE 256

static char *
readline(const char * prompt)
{
   char * buf = (char *) ::malloc(PROMPT_BUFSIZE);
   if (buf == NULL)  return  NULL ;
   *buf = '\0';

   // prompt the user and collect input
   std::cerr << prompt << std::flush ;
   std::cin.getline(buf, PROMPT_BUFSIZE);

   return  buf ;
}
#endif // ! GNU_READLINE


//-------
// ^FUNCTION: CmdLine::prompt_user - prompt the user for a missing argument
//
// ^SYNOPSIS:
//    unsigned CmdLine::prompt_user(cmdarg);
//
// ^PARAMETERS:
//    CmdArg * cmdarg;
//    -- the argument that we need to prompt for
//
// ^DESCRIPTION:
//    If cin is connected to a terminal, then we will prompt the user
//    for an argument corresponding to "cmdarg" and attempt to "compile" it
//    into the desired internal format. The user only has one chance
//    to get the "argument" right; we do not continue prompting if the
//    value that was entered is invalid.
//
// ^REQUIREMENTS:
//    "cmdarg" should be a REQUIRED argument that has already been determined
//    to be missing from the command-line.
//
// ^SIDE-EFFECTS:
//    - modifies the status of the command.
//    - modifies "cmdarg".
//    - prints a prompt on std::cerr and reads from cin
//
// ^RETURN-VALUE:
//    0 if the argument was succesfully entered by the user,
//    ARG_MISSING otherwise.
//
// ^ALGORITHM:
//    - if cin is not a terminal return ARG_MISSING.
//    - if "cmdarg" is a LIST, make sure we prompt the use once for each
//      possible value in the list.
//    - prompt the user for an argument and read the result.
//    - if the user just typed <RETURN> return ARG_MISSING.
//    - "handle" the value that was entered.
//    - continue prompting if we are a LIST and a valid, non-empty,
//      value was given
//    - if an invalid value was given return ARG_MISSING
//    - else return 0
//-^^----
unsigned
CmdLine::prompt_user(CmdArg * cmdarg)
{
   // dont prompt if cin or std::cerr is not interactive
   if (!::isatty(0) || !::isatty(2))  return  ARG_MISSING ;

   // if we have a list, need to prompt repeatedly
   if (cmdarg->syntax() & CmdArg::isLIST) {
      std::cerr << "Enter one " << cmdarg->value_name() << " per line "
           << "(enter a blank-line to stop)." << std::endl ;
   }
   char * buf = NULL;
   char prompt[256];
   snprintf(prompt, sizeof prompt, "\rEnter %s: ", cmdarg->value_name()) ;
   int  errs = 0, first = 1;
   do {  // need repeated prompting for a LIST
      if (buf)  ::free(buf);
      buf = ::readline(prompt) ;
      if (buf == NULL)  return  ARG_MISSING ;

      // make sure we read something!
      if (! *buf) {
         if (first) {
            error() << "error - no " << cmdarg->value_name()
                    << " given!" << std::endl ;
            ++errs;
         }
         continue;
      }

#ifdef GNU_READLINE
      // add this line to the history list
      ::add_history(buf);
#endif

      // try to handle the value we read (remember - buf is temporary)
      if (! errs) {
         const char * arg = buf;
         unsigned  save_cmd_flags = cmd_flags;
         errs = handle_arg(cmdarg, arg);
         if (errs) {
            arg_error("bad value for", cmdarg) << "." << std::endl ;
         }
         cmd_flags = save_cmd_flags;
      }

      first = 0;
   } while (!errs && (cmdarg->syntax() & CmdArg::isLIST) &&  *buf);

   if (! errs)  cmdarg->set_flags(CmdArg::VALSEP);

   if (buf)  ::free(buf);
   return  (errs) ? ARG_MISSING : NO_ERROR ;
}

//-------
// ^FUNCTION: CmdLine::syntax - determine usage message syntax
//
// ^SYNOPSIS:
//    CmdLine::CmdLineSyntax CmdLine::usage_syntax();
//
// ^PARAMETERS:
//
// ^DESCRIPTION:
//
// ^REQUIREMENTS:
//
// ^SIDE-EFFECTS:
//    None.
//
// ^RETURN-VALUE:
//    The desired usage message syntax to use.
//
// ^ALGORITHM:
//    Trivial.
//-^^----
CmdLine::CmdLineSyntax
CmdLine::usage_syntax() const
{
   switch (cmd_flags & (KWDS_ONLY | OPTS_ONLY))
   {
      case KWDS_ONLY: return cmd_KWDS_ONLY ;
      case OPTS_ONLY: return cmd_OPTS_ONLY ;
   }
   return cmd_BOTH ;
}

//-------
// ^FUNCTION: CmdLine::missing_args - check for missing required arguments
//
// ^SYNOPSIS:
//    unsigned CmdLine::missing_args();
//
// ^PARAMETERS:
//
// ^DESCRIPTION:
//    This function checks to see if there is a required argument in the
//    CmdLine object that was NOT specified on the command. If this is
//    the case and PROMPT_USER is set (or $PROMPT_USER exists and is
//    non-empty) then we attempt to prompt the user for the missing argument.
//
// ^REQUIREMENTS:
//
// ^SIDE-EFFECTS:
//    - modifies the status of "cmd".
//    - terminates execution by calling quit() if cmd_NOABORT is NOT
//      set and a required argument (that was not properly supplied by
//      the user) is not given.
//    - prints on stderr if an argument is missing and cmd_QUIET is NOT set.
//    - also has the side-effects of prompt_user() if we need to prompt
//      the user for input.
//
// ^RETURN-VALUE:
//    The current value of the (possibly modified) command status. This is a
//    combination of bitmasks of type cmdline_flags_t defined in <cmdline.h>
//
// ^ALGORITHM:
//    Foreach argument in cmd
//       if argument is required and was not given
//          if required, prompt for the missing argument
//             if prompting was unsuccesful add ARG_MISSING to cmd-status
//             endif
//          else add ARG_MISSING to cmd-status
//          endif
//       endif
//    endfor
//    return the current cmd-status
//-^^----
unsigned
CmdLine::missing_args()
{
   char buf[256];

   CmdArgListListIter  list_iter(cmd_args);
   for (CmdArgList * alist = list_iter() ; alist ; alist = list_iter()) {
      CmdArgListIter  iter(alist);
      for (CmdArg * cmdarg = iter() ; cmdarg ; cmdarg = iter()) {
         if (cmdarg->is_dummy())  continue;
         if ((cmdarg->syntax() & CmdArg::isREQ) &&
             (! (cmdarg->flags() & CmdArg::GIVEN)))
         {
            if (! (cmd_flags & QUIET)) {
               fmt_arg(cmdarg,  buf, sizeof(buf), usage_syntax(), TERSE_USAGE);
               error() << buf << " required." << std::endl ;
            }
            if (cmd_status & ARG_MISSING) {
               // user didnt supply the missing argument
               return  cmd_status ;
            } else if ((! (cmd_flags & NO_ABORT)) && cmd_status) {
               // other problems
               return  cmd_status ;
            } else if (cmd_flags & PROMPT_USER) {
               cmd_status |= prompt_user(cmdarg);
            } else {
               char * env = ::getenv("PROMPT_USER");
               if (env && *env) {
                  cmd_status |= prompt_user(cmdarg);
               } else {
                  cmd_status |= ARG_MISSING ;
               }
            }
         } //if
      } //for iter
   } //for list_iter

   return  cmd_status ;
}


//-------
// ^FUNCTION: CmdLine::opt_match - attempt to match on option
//
// ^SYNOPSIS:
//    CmdArg * CmdLine::opt_match(optchar);
//
// ^PARAMETERS:
//    char optchar;
//    -- a possible option for "cmd"
//
// ^DESCRIPTION:
//    If "cmd" has an argument that has "optchar" as a single-character
//    option then this function will find and return that argument.
//
// ^REQUIREMENTS:
//
// ^SIDE-EFFECTS:
//    None.
//
// ^RETURN-VALUE:
//    If we find a match, then we return a pointer to its argdesc,
//    otherwise we return NULL.
//
// ^ALGORITHM:
//    Trivial.
//-^^----
CmdArg *
CmdLine::opt_match(char optchar) const
{
   CmdArgListListIter  list_iter(cmd_args);
   for (CmdArgList * alist = list_iter() ; alist ; alist = list_iter()) {
      CmdArgListIter  iter(alist);
      for (CmdArg * cmdarg = iter() ; cmdarg ; cmdarg = iter()) {
         if (cmdarg->is_dummy())  continue;
         if (optchar == cmdarg->char_name()) {
            // exact match
            return  cmdarg ;
         } else if (cmd_flags & ANY_CASE_OPTS) {
            // case-insensitive match
            if (tolower(optchar) == tolower(cmdarg->char_name())) {
               return  cmdarg ;
            }
         }
      } //for iter
   } //for list_iter
   return  NULL ;
}

//-------
// ^FUNCTION: CmdLine::kwd_match - purpose
//
// ^SYNOPSIS:
//    extern CmdArg * CmdLine::kwd_match(kwd, len, is_ambiguous, match_value);
//
// ^PARAMETERS:
//    const char * kwd;
//    -- a possible kewyord of "cmd"
//
//    int len;
//    -- the number of character of "kwd" to consider (< 0 if all characters
//       of "kwd" should be used).
//
//    int & is_ambiguous;
//    -- upon return, the value pointed to is set to 1 if the keyword
//       matches more than 1 keyword in "cmd"; Otherwise it is set to 0.
//
//    int match_value;
//    -- if this is non-zero, then if a keyword_name is NULL use the
//       value_name instead.
//
// ^DESCRIPTION:
//    If "cmd" has an argument that matches "kwd" as a kewyord
//    then this function will find and return that argument.
//
// ^REQUIREMENTS:
//
// ^SIDE-EFFECTS:
//    None.
//
// ^RETURN-VALUE:
//    If we find a match, then we return a pointer to its argdesc,
//    otherwise we return NULL.
//
// ^ALGORITHM:
//    Set is_ambigous to 0.
//    For each argument in cmd
//       if argument's keyword-name matches kwd then
//          if this was an exact match then return this argument
//          else if we had a previous partial match of this argument then
//             if argument is a default argument return the previous match
//             else set is_ambiguous to 1 and return NULL
//          else remember we had a partial match here and keep trying
//          endif
//       endif
//    end for
//    if we has a partial match and we get to here then it is NOT ambiguous do
//       go ahead and return the argument we matched.
//-^^----
CmdArg *
CmdLine::kwd_match(const char * kwd,
                   int          len,
                   int &        is_ambiguous,
                   int          match_value) const
{
   CmdArg * matched = NULL;

   is_ambiguous = 0 ;

   CmdArgListListIter  list_iter(cmd_args);
   for (CmdArgList * alist = list_iter() ; alist ; alist = list_iter()) {
      CmdArgListIter  iter(alist);
      for (CmdArg * cmdarg = iter() ; cmdarg ; cmdarg = iter()) {
         if (cmdarg->is_dummy())  continue;

         // attempt to match this keyword
         strmatch_t  result  = str_NONE ;
         const char * const source = cmdarg->keyword_name();
         if (source && *source)
            result = strmatch(source, kwd, len) ;
         else if (match_value)
            result = strmatch(cmdarg->value_name(), kwd, len) ;

         switch (result)
         {
            case str_EXACT: return cmdarg ;

            case str_PARTIAL:
               if (matched)
                  is_ambiguous = 1 ;
               else
                  matched = cmdarg ;
            default: break ;
         }
      } //for iter
   } //for list_iter

   if (is_ambiguous)
      // ambiguous keyword
      return NULL ;

   if (!matched && (cmd_flags & SKIP_UNKNWN))
      matched = get_unknown_arg() ;
   return  matched ;
}

//-------
// ^FUNCTION: CmdLine::pos_match - match a positional argument
//
// ^SYNOPSIS:
//    CmdArg * CmdLine::pos_match()
//
// ^PARAMETERS:
//
// ^DESCRIPTION:
//    If "cmd" has an positional argument that has not yet been given,
//    or that corresponds to a list, then this function will find and
//    return the first such argument.
//
// ^REQUIREMENTS:
//
// ^SIDE-EFFECTS:
//    None.
//
// ^RETURN-VALUE:
//    If we find a match, then we return a pointer to its argument,
//    otherwise we return NULL.
//
// ^ALGORITHM:
//    First look for the first unmatched positional argument!!
//    If there aren't any, then look for the LAST positional list!
//-^^----
CmdArg *
CmdLine::pos_match() const
{
   CmdArg * last_pos_list = NULL;
   CmdArgListListIter  list_iter(cmd_args);
   for (CmdArgList * alist = list_iter() ; alist ; alist = list_iter()) {
      CmdArgListIter  iter(alist);
      for (CmdArg * cmdarg = iter() ; cmdarg ; cmdarg = iter()) {
         if (cmdarg->is_dummy())  continue;
         if (cmdarg->syntax() & CmdArg::isPOS) {
            if (! (cmdarg->flags() & CmdArg::GIVEN)) {
               return  cmdarg ;
            } else if (cmdarg->flags() & CmdArg::isLIST) {
               last_pos_list = cmdarg;
            }
         }
      } //for iter
   } //for list_iter
   return  last_pos_list ;
}
