//------------------------------------------------------------------------
// ^FILE: unix.c - implement the unix-specific portions of CmdLine
//
// ^DESCRIPTION:
//     This file implements the public and private CmdLine library functions
//  that are specific to the native command-line syntax of Unix.
//
//  The following functions are implemented:
//
//     CmdLine::parse_option() -- parse an option
//     CmdLine::parse_keyword() -- parse a keyword
//     CmdLine::parse_value() -- parse a value
//     CmdLine::parse_arg() -- parse a single argument
//     CmdLine::arg_error() -- format an argument for error messages
//     CmdLine::fmt_arg()   -- format an argument for usage messages
//
// ^HISTORY:
//    01/09/92	Brad Appleton	<bradapp@enteract.com>	Created
//
//    03/01/93	Brad Appleton	<bradapp@enteract.com>
//    - Added ALLOW_PLUS to list of CmdLine configuration flags
//-^^---------------------------------------------------------------------

#include <iostream>
#include <sstream>
#include <algorithm>

#include <stdlib.h>
#include <string.h>

#include "exits.h"
#include "cmdline.h"
#include "states.h"

//
// Some Helper function for getting and recognizing prefixes
//

  // Function to tell us if an argument looks like an option
inline static int
isOPTION(const char * s)  {
   return  ((*(s) == '-') && ((*((s)+1) != '-')) && ((*((s)+1) != '\0'))) ;
}

   // Function to return the option-prefix
inline static const char *
OptionPrefix() { return  "-" ; }


   // Function to tell us if an argument looks like a long-option.
   //
   // NOTE: allowing "+" does not preclude the use of "--"
   //
inline static int
isKEYWORD(const char *s, int allow_plus) {
   return  (((*(s) == '-') && (*((s)+1) == '-')  && (*((s)+2) != '\0')) ||
            (allow_plus && (*(s) == '+') && ((*((s)+1) != '\0')))) ;
}

   // Function to return the long-option prefix
inline static const char *
KeywordPrefix(bool allow_plus) { return allow_plus ? "+" : "--" ; }

   // Need to know when an argument means "end-of-options"
inline static int
isENDOPTIONS(const char *s) {
   return  ((*(s) == '-') && (*((s)+1) == '-')  && (*((s)+2) == '\0')) ;
}

   // Function to return the "end-of-options" string
inline static const char *
EndOptions() { return "--" ; }


//-------
// ^FUNCTION: CmdLine::parse_option - parse a Unix option
//
// ^SYNOPSIS:
//    unsigned CmdLine::parse_option(arg);
//
// ^PARAMETERS:
//    const char * arg;
//    -- the command-line argument containing the prospective option
//
// ^DESCRIPTION:
//    This routine will attempt to "handle" all options specified in
//    the string "arg". For each option found, its compile-function
//    is called and the corresponding state of both the command
//    and of the matched option(s) is (are) updated.
//
// ^REQUIREMENTS:
//    "arg" should point past any leading option prefix (such as "-").
//
// ^SIDE-EFFECTS:
//    "cmd" is modified accordingly as each option is parsed (as are its
//    constituent arguments). If there are syntax errors then error messages
//    are printed if QUIET is NOT set.
//
// ^RETURN-VALUE:
//    CMDSTAT_OK is returned if no errors were encountered. Otherwise the
//    return value is a combination of bitmasks of type CmdLine::CmdStatus
//    (defined in <cmdline.h>) indicating all the problems that occurred.
//
// ^ALGORITHM:
//    see if we left an option dangling without parsing its value.
//    for each option bundled in "arg"
//       try to match the option
//       if no match issue a message (unless QUIET is set)
//       else
//          if the option takes NO argument than handle that
//          else if the option takes an argument
//             if the rest or arg is not empty then
//                call the option's compile-function using the rest of
//                                                    arg as the value.
//                skip past whatever portion of value was used
//             else
//                update the state of the command to show that we are expecting
//                       to see the value for this option as the next argument.
//             endif
//          endif
//          update the state of the argument.
//       endif
//    endfor
//-^^----
unsigned
CmdLine::parse_option(const char * arg)
{
   const  char * save_arg = arg;
   unsigned  save_flags = 0, rc = 0 ;
   CmdArg * cmdarg = NULL;
   int  bad_val;

   // see if we left an argument dangling without a value
   ck_need_val() ;

   cmd_matched_arg = NULL ;
   do {  // loop over bundled options
      cmdarg = opt_match(*arg);
      if (cmdarg == NULL) {
         // If we were in the middle of a guess - sorry no cigar, otherwise
         // guess that maybe this is a keyword and not an keyword.
         //
         if (cmd_state & cmd_GUESSING) {
            if (arg == save_arg)  return  BAD_OPTION;
         } else {
            if (cmd_flags & GUESS) {
               cmd_state |= cmd_GUESSING;
               rc = parse_keyword(arg);
               cmd_state &= ~cmd_GUESSING;
               if (rc != BAD_KEYWORD)  return  rc;
            }
         }
         if (! (cmd_flags & QUIET)) {
            error() << "unknown option \"" << OptionPrefix() << char(*arg)
                    << "\"." << std::endl ;
         }
         rc |= BAD_OPTION ;
         ++arg ;  // skip bad option
         continue ;
      }
      ++arg ;  // skip past option character
      save_flags = cmdarg->flags() ;
      cmdarg->clear_flags();
      cmdarg->set_flags(CmdArg::OPTION) ;
      if ((! *arg) && (cmdarg->syntax() & CmdArg::isVALTAKEN)) {
         // End of string -- value must be in next arg
         // Save this cmdarg-pointer for later and set the parse_state to
         // indicate that we are expecting a value.
         //

         if (cmdarg->syntax() & CmdArg::isVALSTICKY) {
            // If this argument is sticky we already missed our chance
            // at seeing a value.
            //
            if (cmdarg->syntax() & CmdArg::isVALREQ) {
               if (! (cmd_flags & QUIET)) {
                  error() << "value required in same argument for "
                          << OptionPrefix() << char(cmdarg->char_name())
                          << " option." << std::endl;
               }
               rc |= (VAL_MISSING | VAL_NOTSTICKY) ;
               cmdarg->flags(save_flags);
            } else {
               // The value is optional - set the GIVEN flag and call
               // handle_arg with NULL (and check the result).
               //
               const char * null_str = NULL;
               cmdarg->set_flags(CmdArg::GIVEN) ;
               cmd_parse_state = cmd_START_STATE ;
               bad_val = handle_arg(cmdarg, null_str);
               if (bad_val) {
                  if (! (cmd_flags & QUIET)) {
                     arg_error("bad value for", cmdarg) << "." << std::endl ;
                  }
                  rc |= BAD_VALUE ;
                  cmdarg->flags(save_flags);
               }
            }
         } else {
            // Wait for the value to show up next time around
            cmdarg->set_flags(CmdArg::GIVEN) ;
            cmd_matched_arg = cmdarg ;
            cmd_parse_state = cmd_WANT_VAL ;
            if (cmdarg->syntax() & CmdArg::isVALREQ) {
               cmd_parse_state += cmd_TOK_REQUIRED ;
            }
         }
         return  rc ;
      }

      // If this option is an isVALSEP and "arg" is not-empty then we
      // have an error.
      //
      if ((cmdarg->syntax() & CmdArg::isVALTAKEN) &&
          (cmdarg->syntax() & CmdArg::isVALSEP)) {
         if (! (cmd_flags & QUIET)) {
            error() << "value required in separate argument for "
                    << OptionPrefix() << char(cmdarg->char_name())
                    << " option." << std::endl;
         }
         rc |= (VAL_MISSING | VAL_NOTSEP) ;
         cmdarg->flags(save_flags);
         return  rc;
      } else {
         // handle the option
         const char * save_arg = arg;
         bad_val = handle_arg(cmdarg, arg);
         if (bad_val) {
            if (! (cmd_flags & QUIET)) {
               arg_error("bad value for", cmdarg) << "." << std::endl ;
            }
            rc |= BAD_VALUE ;
            cmdarg->flags(save_flags);
         }
         cmdarg->set_flags(CmdArg::GIVEN);
         if (arg != save_arg)  cmdarg->set_flags(CmdArg::VALGIVEN);
      }
   } while (arg && *arg) ;

   return  rc ;
}


//-------
// ^FUNCTION: CmdLine::parse_keyword - parse a Unix keyword
//
// ^SYNOPSIS:
//    unsigned CmdLine::parse_keyword(arg);
//
// ^PARAMETERS:
//    const char * arg;
//    -- the command-line argument containing the prospective keyword
//
// ^DESCRIPTION:
//    This routine will attempt to "handle" the keyword specified in
//    the string "arg". For any keyword found, its compile-function
//    is called and the corresponding state of both the command
//    and of the matched keyword(s) is (are) updated.
//
// ^REQUIREMENTS:
//    "arg" should point past any leading keyword prefix (such as "--").
//
// ^SIDE-EFFECTS:
//    "cmd" is modified accordingly as the keyword is parsed (as are its
//    constituent arguments). If there are syntax errors then error messages
//    are printed if QUIET is NOT set.
//
// ^RETURN-VALUE:
//    CMDSTAT_OK is returned if no errors were encountered. Otherwise the
//    return value is a combination of bitmasks of type CmdLine::CmdStatus
//    (defined in <cmdline.h>) indicating all the problems that occurred.
//
// ^ALGORITHM:
//    see if we left an option dangling without parsing its value.
//    look for a possible value for this keyword denoted by ':' or '='
//    try to match "arg" as a keyword
//    if no match issue a message (unless QUIET is set)
//    else
//       if the keyword takes NO argument than handle that
//       else if the keyword takes an argument
//          if a value was found "arg"
//             call the keyword's compile-function with the value found.
//          else
//             update the state of the command to show that we are expecting
//                    to see the value for this option as the next argument.
//          endif
//       endif
//       update the state of the argument.
//    endif
//-^^----
unsigned
CmdLine::parse_keyword(const char * arg)
{
   unsigned  save_flags = 0, rc = 0 ;
   CmdArg * cmdarg = NULL ;
   int  ambiguous = 0, len = -1, bad_val;
   const char * val = NULL ;

   int  plus = (cmd_flags & ALLOW_PLUS) ;  // Can we use "+"?

   // see if we left an argument dangling without a value
   ck_need_val() ;

   // If there is a value with this argument, get it now!
   val = ::strpbrk(arg, ":=") ;
   if (val) {
      len = val - arg ;
      ++val ;
   }

   cmd_matched_arg = NULL ;
   cmdarg = kwd_match(arg, len, ambiguous);
   if (cmdarg == NULL) {
      // If we were in the middle of a guess - sorry no cigar, otherwise
      // guess that maybe this is an option and not a keyword.
      //
      if (cmd_state & cmd_GUESSING) {
         return  BAD_KEYWORD;
      } else if ((! ambiguous) || (len == 1)) {
         if (cmd_flags & GUESS) {
            cmd_state |= cmd_GUESSING;
            rc = parse_option(arg);
            cmd_state &= ~cmd_GUESSING;
            if (rc != BAD_OPTION)  return  rc;
         }
      }
      if (! (cmd_flags & QUIET)) {
         error() << ((ambiguous) ? "ambiguous" : "unknown") << " option "
                 << "\"" << ((cmd_flags & KWDS_ONLY) ? OptionPrefix()
                                                     : KeywordPrefix(plus))
                 << arg << "\"." << std::endl ;
      }
      rc |= ((ambiguous) ? KWD_AMBIGUOUS : BAD_KEYWORD) ;
      return  rc ;
   }

   save_flags = cmdarg->flags() ;
   cmdarg->clear_flags();
   cmdarg->set_flags(CmdArg::KEYWORD) ;
   cmd_matched_arg = cmdarg ;
   if ((cmdarg->syntax() & CmdArg::isVALTAKEN) && (val == NULL)) {
      // Value must be in the next argument.
      // Save this cmdarg for later and indicate that we are
      // expecting a value.
      //
      if (cmdarg->syntax() & CmdArg::isVALSTICKY) {
         // If this argument is sticky we already missed our chance
         // at seeing a value.
         //
         if (cmdarg->syntax() & CmdArg::isVALREQ) {
            if (! (cmd_flags & QUIET)) {
               error() << "value required in same argument for "
                       << ((cmd_flags & KWDS_ONLY) ? OptionPrefix()
                                                   : KeywordPrefix(plus))
                       << cmdarg->keyword_name() << " option." << std::endl;
            }
            rc |= (VAL_MISSING | VAL_NOTSTICKY) ;
            cmdarg->flags(save_flags);
         } else {
            // The value is optional - set the GIVEN flag and call
            // handle_arg with NULL (and check the result).
            //
            const char * null_str = NULL;
            cmdarg->set_flags(CmdArg::GIVEN) ;
            cmd_parse_state = cmd_START_STATE ;
            bad_val = handle_arg(cmdarg, null_str);
            if (bad_val) {
               if (! (cmd_flags & QUIET)) {
                  arg_error("bad value for", cmdarg) << "." << std::endl ;
               }
               rc |= BAD_VALUE ;
               cmdarg->flags(save_flags);
            }
         }
      } else {
         // Wait for the value to show up next time around
         cmdarg->set_flags(CmdArg::GIVEN) ;
         cmd_parse_state = cmd_WANT_VAL ;
         if (cmdarg->syntax() & CmdArg::isVALREQ) {
            cmd_parse_state += cmd_TOK_REQUIRED ;
         }
      }
      return  rc ;
   }

   // If this option is an isVALSEP and "val" is not-NULL then we
   // have an error.
   //
   if (val  &&  (cmdarg->syntax() & CmdArg::isVALTAKEN) &&
       (cmdarg->syntax() & CmdArg::isVALSEP)) {
      if (! (cmd_flags & QUIET)) {
         error() << "value required in separate argument for "
                 << ((cmd_flags & KWDS_ONLY) ? OptionPrefix()
                                             : KeywordPrefix(plus))
                 << cmdarg->keyword_name() << " option." << std::endl;
      }
      rc |= (VAL_MISSING | VAL_NOTSEP) ;
      cmdarg->flags(save_flags);
      return  rc;
   }
   // handle the keyword
   bad_val = handle_arg(cmdarg, val);
   if (bad_val) {
      if (! (cmd_flags & QUIET)) {
         arg_error("bad value for", cmdarg) << "." << std::endl ;
      }
      rc |= BAD_VALUE ;
      cmdarg->flags(save_flags);
   }

   return  rc ;
}


//-------
// ^FUNCTION: CmdLine::parse_value - parse a Unix value
//
// ^SYNOPSIS:
//    unsigned CmdLine::parse_value(arg);
//
// ^PARAMETERS:
//    const char * arg;
//    -- the command-line argument containing the prospective value
//
// ^DESCRIPTION:
//    This routine will attempt to "handle" the value specified in
//    the string "arg". The compile-function of the corresponding
//    argument-value is called and the corresponding state of both
//    the command and of the matched option(s) is (are) updated.
//    If the value corresponds to a multi-valued argument, then that
//    is handled here.
//
// ^REQUIREMENTS:
//
// ^SIDE-EFFECTS:
//    "cmd" is modified accordingly for the value that is parsed (as are its
//    constituent arguments). If there are syntax errors then error messages
//    are printed if QUIET is NOT set.
//
// ^RETURN-VALUE:
//    CMDSTAT_OK is returned if no errors were encountered. Otherwise the
//    return value is a combination of bitmasks of type CmdLine::CmdStatus
//    (defined in <cmdline.h>) indicating all the problems that occurred.
//
// ^ALGORITHM:
//    If the command-state says we are waiting for the value of an option
//       find the option that we matched last
//    else
//       match the next positional parameter in "cmd"
//       if there isnt one issue a "too many args" message
//               (unless cmd_QUIETs is set) and return.
//    endif
//    handle the given value and update the argument and command states.
//-^^----
unsigned
CmdLine::parse_value(const char * arg)
{
   unsigned  save_flags = 0, rc = 0 ;
   int  bad_val;
   CmdArg * cmdarg = NULL;

   if (cmd_parse_state & cmd_WANT_VAL) {
      if (cmd_matched_arg == NULL) {
         std::cerr << "*** Internal error in class CmdLine.\n"
              << "\tparse-state is inconsistent with last-matched-arg."
              << std::endl ;
         ::exit(e_INTERNAL);
      }
      // get back the cmdarg that we saved for later
      // - here is the value it was expecting
      //
      cmdarg = cmd_matched_arg ;
      save_flags = cmdarg->flags() ;
   } else {
      // argument is positional - find out which one it is
      cmdarg = pos_match() ;
      if (cmdarg == NULL) {
         if (! (cmd_flags & QUIET)) {
            error() << "too many arguments given." << std::endl ;
         }
         rc |= TOO_MANY_ARGS ;
         return  rc ;
      }
      save_flags = cmdarg->flags() ;
      cmdarg->clear_flags();
      cmdarg->set_flags(CmdArg::POSITIONAL) ;
      if (cmd_flags & OPTS_FIRST) {
         cmd_state |= cmd_END_OF_OPTIONS ;
      }
   }

   // handle this value
   cmdarg->set_flags(CmdArg::VALSEP) ;
   bad_val = handle_arg(cmdarg, arg) ;
   if (bad_val) {
      if (! (cmd_flags & QUIET)) {
         arg_error("bad value for", cmdarg) << "." << std::endl ;
      }
      rc |= BAD_VALUE ;
      cmdarg->flags(save_flags);
      if (! (cmdarg->syntax() & CmdArg::isLIST)) {
         cmd_parse_state = cmd_START_STATE;
      }
   }

   // If the value was okay and we were requiring a value, then
   // a value is no longer required.
   //
   if ((! bad_val) && (cmdarg->syntax() & CmdArg::isLIST)) {
      cmd_parse_state &= ~cmd_TOK_REQUIRED ;
   }

   return  rc ;
}


//-------
// ^FUNCTION: CmdLine::parse_arg - parse an argv[] element unix-style
//
// ^SYNOPSIS:
//    unsigned CmdLine::parse_arg(arg)
//
// ^PARAMETERS:
//    const char * arg;
//    -- an argument string (argv[] element) from the command-line
//
// ^DESCRIPTION:
//    This routine will determine whether "arg" is an option, a long-option,
//    or a value and call the appropriate parse_xxxx function defined above.
//
// ^REQUIREMENTS:
//
// ^SIDE-EFFECTS:
//    "cmd" is modified accordingly for the string that is parsed (as are its
//    constituent arguments). If there are syntax errors then error messages
//    are printed if QUIET is NOT set.
//
// ^RETURN-VALUE:
//    CMDSTAT_OK is returned if no errors were encountered. Otherwise the
//    return value is a combination of bitmasks of type CmdLine::CmdStatus
//    (defined in <cmdline.h>) indicating all the problems that occurred.
//
// ^ALGORITHM:
//    if we are expecting a required value
//       call parse_value()
//    else if "arg" is an option
//       skip past the option prefix
//       call parse_option()
//    else if "arg" is a keyword
//       skip past the kewyord prefix
//       call parse_keyword()
//    else if "arg" is "--" (meaning end of options)
//       see that we didnt leave an option dangling without a value
//       indicate end-of-options in the command-state
//    else
//       call parse_value()
//    endif
//-^^----
unsigned
CmdLine::parse_arg(const char * arg)
{
   if (arg == NULL)  return  cmd_status ;

   int  plus = (cmd_flags & ALLOW_PLUS) ;  // Can we use "+"?

   if (cmd_parse_state & cmd_TOK_REQUIRED) {
      // If a required value is expected, then this argument MUST be
      // the value (even if it looks like an option
      //
      cmd_status |= parse_value(arg) ;
   } else if (isOPTION(arg) && (! (cmd_state & cmd_END_OF_OPTIONS))) {
      ++arg ;  // skip '-' option character
      if (cmd_flags & KWDS_ONLY) {
         cmd_state  |= cmd_KEYWORDS_USED ;
         cmd_status |=  parse_keyword(arg) ;
      } else {
         cmd_state  |= cmd_OPTIONS_USED ;
         cmd_status |=  parse_option(arg) ;
      }
   } else if ((! (cmd_flags & OPTS_ONLY))
              && isKEYWORD(arg, plus) && (! (cmd_state & cmd_END_OF_OPTIONS))) {
      cmd_state |= cmd_KEYWORDS_USED ;
      if (*arg == '+') {
         ++arg ;  // skip over '+' keyword prefix
      } else {
         arg += 2 ;  // skip over '--' keyword prefix
      }
      cmd_status |= parse_keyword(arg) ;
   } else if (isENDOPTIONS(arg) && (! (cmd_state & cmd_END_OF_OPTIONS))) {
      cmd_state |= cmd_END_OF_OPTIONS ;
      // see if we left an argument dangling without a value
      ck_need_val() ;
   } else {
      cmd_status |= parse_value(arg) ;
   }

   return  cmd_status ;
}

//-------
// ^FUNCTION: CmdLine::arg_error - format an argument for error messages
//
// ^SYNOPSIS:
//    std::ostream & arg_error(error_str, cmdarg);
//
// ^PARAMETERS:
//    const char * error_str;
//    -- the problem with the argument
//
//    const CmdArg * cmdarg;
//    -- the argument to be formatted
//
// ^DESCRIPTION:
//    This function will write to "os" the argument corresponding to
//    "cmdarg" as we would like it to appear in error messages that pertain
//    to this argument.
//
// ^REQUIREMENTS:
//    None.
//
// ^SIDE-EFFECTS:
//    writes to "os"
//
// ^RETURN-VALUE:
//    A reference to os.
//
// ^ALGORITHM:
//    Pretty straightforward, just print to os the way we
//    want the argument to appear in usage messages.
//-^^----
std::ostream &
CmdLine::arg_error(const char * error_str, const CmdArg * cmdarg) const
{
   const bool plus = (cmd_flags & ALLOW_PLUS) && (cmdarg->flags() & CmdArg::GIVEN) ;  // Can we use "+"?

   std::ostream & os = error() << error_str << char(' ') ;

   static const auto
     print_argname = [](std::ostream &os, const CmdArg *arg, bool only_kwds, bool allow_plus)
     {
       if (!arg->char_name() || (arg->flags() & CmdArg::KEYWORD))
         os << (only_kwds ? OptionPrefix() : KeywordPrefix(allow_plus)) << arg->keyword_name() ;
       else
         os << OptionPrefix() << (char)arg->char_name() << " option" ;
       os << " option" ;
     } ;

   if ((cmdarg->flags() & (CmdArg::KEYWORD | CmdArg::OPTION)) || !(cmdarg->syntax() & CmdArg::isPOS))
     print_argname(os, cmdarg, cmd_flags & KWDS_ONLY, plus) ;
   else
     os << cmdarg->value_name() << " argument" ;
   return  os;
}

//-------
// ^FUNCTION: CmdLine::fmt_arg - format an argument for usage messages
//
// ^SYNOPSIS:
//    unsigned CmdLine::fmt_arg(cmdarg, buf, bufsize, syntax, level);
//
// ^PARAMETERS:
//    const CmdArg * cmdarg;
//    -- the argument to be formatted
//
//    char * buf;
//    -- where to print the formatted result
//
//    unsigned bufsize;
//    -- number of bytes allocated for buf.
//
//    CmdLine::CmdLineSyntax syntax;
//    -- the syntax to use (option, long-option, or both).
//
//    CmdLine::CmdUsageLevel;
//    -- the usage-level corresponding to this portion of the
//       usage message.
//
// ^DESCRIPTION:
//    This function will write into "buf" the argument corresponding to
//    "cmdarg" as we would like it to appear in usage messages.
//
// ^REQUIREMENTS:
//    "buf" must be large enough to hold the result.
//
// ^SIDE-EFFECTS:
//    writes to "buf"
//
// ^RETURN-VALUE:
//    the length of the formatted result.
//
// ^ALGORITHM:
//    Its kind of tedious so follow along.
//-^^----
unsigned
CmdLine::fmt_arg(const CmdArg           * cmdarg,
                 char                   * buf,
                 unsigned                 bufsize,
                 CmdLine::CmdLineSyntax   syntax,
                 CmdLine::CmdUsageLevel   level) const
{
   std::ostringstream oss ;
   const size_t maxcnt = bufsize ? bufsize - 1 : 0 ;
   *buf = '\0';

   const bool plus = !!(cmd_flags & ALLOW_PLUS) ;  // Can we use "+"?
   const char optchar = cmdarg->char_name();
   const char * const keyword = cmdarg->keyword_name();

   // Need to adjust the syntax if optchar or keyword is empty
   if ((! (cmdarg->syntax() & CmdArg::isPOS)) &&
       ((! optchar) || (keyword == NULL))) {
      if (keyword == NULL) {
         if ((cmd_flags & KWDS_ONLY) && !(cmd_flags & GUESS)) {
            return  0;
         } else {
            syntax = cmd_OPTS_ONLY;
         }
      }
      if (! optchar) {
         if ((cmd_flags & OPTS_ONLY) && !(cmd_flags & GUESS)) {
            return  0;
         } else {
            syntax = cmd_KWDS_ONLY;
         }
      }
   }

   // If the argument is optional - print the leading '['
   if ((level == VERBOSE_USAGE) && (! (cmdarg->syntax() & CmdArg::isREQ))) {
      oss << char('[') ;
   }

   // If we have a sticky-argument and usage is cmd_BOTH then it gets
   // really hairy so we just treat this as a special case right here
   // and now.
   //
   if ((syntax == cmd_BOTH) &&
       (! (cmdarg->syntax() & CmdArg::isPOS)) &&
       (cmdarg->syntax() & CmdArg::isVALTAKEN) &&
       (cmdarg->syntax() & CmdArg::isVALSTICKY))
   {
      if (cmdarg->syntax() & CmdArg::isVALOPT) {
         oss << OptionPrefix() << char(optchar) << char('[')
             << cmdarg->value_name() << "]|" << KeywordPrefix(plus)
             << keyword << "[=" << cmdarg->value_name() << char(']') ;
      } else {
         oss << OptionPrefix() << optchar << cmdarg->value_name()
             << char('|') << KeywordPrefix(plus) << keyword << char('=')
             << cmdarg->value_name() ;
      }
      if ((level == VERBOSE_USAGE) && (cmdarg->syntax() & CmdArg::isLIST)) {
         oss << " ..." ;
      }
      if ((level == VERBOSE_USAGE) && (! (cmdarg->syntax() & CmdArg::isREQ))) {
         oss << char(']') ;
      }
      const size_t result = std::min<size_t>(oss.tellp(), maxcnt) ;
      memcpy(buf, oss.str().c_str(), result) ;
      buf[result] = '\0' ;
      return result;
   }

   if (! (cmdarg->syntax() & CmdArg::isPOS)) {
      switch(syntax) {
         case cmd_OPTS_ONLY :
            oss << OptionPrefix() << char(optchar) ;
            break ;

         case cmd_KWDS_ONLY :
            oss << ((cmd_flags & KWDS_ONLY) ? OptionPrefix()
                                            : KeywordPrefix(plus)) << keyword ;
            break ;

         case cmd_BOTH :
            oss << OptionPrefix() << char(optchar) << char('|')
                << KeywordPrefix(plus) << keyword ;
            break ;

         default :
            std::cerr << "*** Internal error in class CmdLine.\n"
                 << "\tunknown CmdLineSyntax value (" << int(syntax) << ")."
                 << std::endl ;
            ::exit(e_INTERNAL);
      } //switch
      if (cmdarg->syntax() & CmdArg::isVALTAKEN) {
         if (! (cmdarg->syntax() & CmdArg::isVALSTICKY)) {
            oss << char(' ') ;
         }
      }
   }

   // If the argument takes a value then print the value
   if (cmdarg->syntax() & CmdArg::isVALTAKEN) {
      if ((! (cmdarg->syntax() & CmdArg::isPOS)) &&
          (cmdarg->syntax() & CmdArg::isVALOPT))
      {
         oss << char('[') ;
      }
      if (cmdarg->syntax() & CmdArg::isVALSTICKY) {
         if (syntax == cmd_KWDS_ONLY)  oss << char('=') ;
      }
      oss << cmdarg->value_name() ;
      if ((level == VERBOSE_USAGE) && (cmdarg->syntax() & CmdArg::isLIST)) {
         oss << " ..." ;
      }
      if ((! (cmdarg->syntax() & CmdArg::isPOS)) &&
          (cmdarg->syntax() & CmdArg::isVALOPT))
      {
         oss << char(']') ;
      }
   }

   if ((level == VERBOSE_USAGE) && (! (cmdarg->syntax() & CmdArg::isREQ))) {
      oss << char(']') ;
   }

   const size_t result = std::min<size_t>(oss.tellp(), maxcnt) ;
   memcpy(buf, oss.str().c_str(), result) ;
   buf[result] = '\0' ;
   return result;
}
