//------------------------------------------------------------------------
// ^FILE: syntax.c - implement the ArgSyntax class
//
// ^DESCRIPTION:
//    This file uses a SyntaxFSM to implement a class to parse an argument
//    syntax string from input and to hold the "compiled" result.
//
// ^HISTORY:
//    03/25/92	Brad Appleton	<bradapp@enteract.com>	Created
//-^^---------------------------------------------------------------------

#include <cmdline.h>

#include "syntax.h"
#include "quoted.h"

#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//------------------------------------------------------------------ copy_token

//-------------------
// ^FUNCTION: copy_token - copy into a token
//
// ^SYNOPSIS:
//    copy_token(dest, src)
//
// ^PARAMETERS:
//    const char * & dest;
//    -- where to house the duplicated token
//
//    const SyntaxFSM::token_t & src;
//    -- the token to copy.
//
// ^DESCRIPTION:
//    Duplicate the token denoted by "src" into "dest".
//
// ^REQUIREMENTS:
//    None.
//
// ^SIDE-EFFECTS:
//    Allocates storage for "dest" is token length is non-zero.
//
// ^RETURN-VALUE:
//    None.
//
// ^ALGORITHM:
//    Trivial.
//-^^----------------
void
copy_token(const char * & dest, const SyntaxFSM::token_t & src)
{
   char * tok = new char[src.len + 1] ;
   ::strncpy(tok, src.start, src.len);
   tok[src.len] = '\0';
   dest = tok;
}

//---------------------------------------------------------------- ArgSyntax

//-------------------
// ^FUNCTION: parse_syntax - parse syntax string
//
// ^SYNOPSIS:
//    parse_syntax(str)
//
// ^PARAMETERS:
//    const char * str;
//    -- the string (containing the argument syntax) to parse.
//
// ^DESCRIPTION:
//    Parse the syntax-string and compile it into an internal format
//    (namely an ArgSyntax object).
//
// ^REQUIREMENTS:
//    "str" should correspond to the following:
//
//       [<KEYWORD-SPEC>] [<VALUE-SPEC>]
//
//    Where <KEYWORD-SPEC> is of the form:
//       c|keyword
//
//    Where 'c' is the option-character and "keyword" is the keyword.
//
//    (There must be no spaces surrounding the '|', if there arem then a space
//    before the '|' means an "empty" option and a space after the '|' means
//    an empty keyword).
//
//    <VALUE-SPEC> should look like:
//        value [...]
//
//    Where "value" is the value name and "..." indicates the value is really
//    a list of values. The entire VALUE-SPEC should be surrounded by '[' and
//    ']' if the value is optional.
//
//    If the argument itself is optional then the entire syntax string
//    should be inside of square brackets.
//
//    Lastly - a positional AND keyword argument may be denoted by
//        "[c|keyword] value"
//
// ^SIDE-EFFECTS:
//    - modifies all parts of the ArgSyntax object.
//    - prints syntax error messages on std::cout.
//
// ^RETURN-VALUE:
//    None.
//
// ^ALGORITHM:
//    Too complicated to be described here - follow along.
//-^^----------------
int
ArgSyntax::parse_syntax(const char * syntax)
{
   const char * ptr = syntax;
   SyntaxFSM  fsm;
   SyntaxFSM::token_t  token;

   while (fsm(ptr, token)) {
      switch(fsm.state()) {
      case  SyntaxFSM::OPTION :
         // We have an option character - save it and move on
         if (token.len)  arg_char = *(token.start) ;
         if (! fsm.level())  arg_syntax |= CmdArg::isREQ;
         break;

      case  SyntaxFSM::KEYWORD :
         // We have a keyword - save it and move on
         ::copy_token(arg_keyword, token);
         if (! fsm.level())  arg_syntax |= CmdArg::isREQ;
         break;

      case  SyntaxFSM::VALUE :
         // We have a value - save it and call parse_value to
         // figure out what the flags are.
         //
         if (token.len)  ::copy_token(arg_value, token);
         parse_value(fsm);
         break;

      case  SyntaxFSM::LIST :
         // We have an ellipsis -- update the syntax flags
         arg_syntax |= CmdArg::isLIST;
         break;

      case  SyntaxFSM::ERROR :
         // Error!
         std::cerr << "syntax error in \"" << syntax << "\"." << std::endl ;
         return  -1;

      default :
         std::cerr << "internal error in class SyntaxFSM.\n\tunexpected state "
              << "(" << fsm.state() << ") encountered." << std::endl ;
         return  -1;
      } //switch
   } //while

   return  0;
}


//-------------------
// ^FUNCTION: parse_value - parse an argument value
//
// ^SYNOPSIS:
//    parse_value(fsm)
//
// ^PARAMETERS:
//    const SyntaxFSM & fsm;
//    -- the finite-state machine that is reading input.
//
// ^DESCRIPTION:
//    The "value" has already been read and saved, we need to figure out
//    what syntax_flags to associate with the argument.
//
// ^REQUIREMENTS:
//    "fsm" MUST be in the SyntaxFSM::VALUE state!
//
// ^SIDE-EFFECTS:
//    Modifies the arg_syntax flags of an ArgSyntax object.
//
// ^RETURN-VALUE:
//    None.
//
// ^ALGORITHM:
//    Too complicated to be described here - follow along.
//
//-^^----------------
void
ArgSyntax::parse_value(const SyntaxFSM & fsm)
{
   // Each of the possibilities we encounter in the SyntaxFSM::VALUE state
   // will correspond to some combination of num_tokens, num_braces, and
   // level. Let us determine all the valid possibilites below:
   //
   //   (num_tokens, num_braces, level)            syntax-string
   //   -------------------------------     ---------------------------
   //             (1, 0, 0)                      "value"
   //             (1, 0, 1)                      "[value]"
   //             (3, 0, 0)                      "c|string value"
   //             (3, 0, 1)                      "c|string [value]"
   //             (3, 0, 1)                      "[c|string value]"
   //             (3, 0, 2)                      "[c|string [value]]"
   //             (3, 1, 0)                      "[c|string] value"
   //             (3, 1, 1)                      "[c|string] [value]"
   //             (3, 1, 1)                      "[[c|string] value]"
   //
   // There are only two case where a given (num_token, num_braces, level)
   // combination corresponds to more than one possible syntax-string. These
   // two cases are (3, 0, 1) and (3, 1, 1). We can ignore the "ambiguity"
   // of (3, 1, 1) because although the two possible syntax-strings are
   // different, they mean exactly the same thing. (3, 0, 1) is a different
   // case however: how do we tell if the whole argument is optional or if
   // just the value is optional? If the whole argument is required (meaning
   // "not optional") then we will already have set the isREQ flag when we
   // parsed the option and/or the keyword name.
   //
   if (fsm.num_tokens() == 1) {
      // cases (1, 0, 0) and (1, 0, 1)
      arg_syntax |= CmdArg::isPOS;
      if (! fsm.level()) {
         arg_syntax |= (CmdArg::isREQ | CmdArg::isVALREQ);
      } else {
         arg_syntax |= CmdArg::isVALOPT ;
      }
   } else {
      if (fsm.num_braces()) {
         // cases (3, 1, 0) and (3, 1, 1)
         arg_syntax |= CmdArg::isPOS;
         if (! fsm.level()) {
            // case (3, 1, 0)
            arg_syntax |= (CmdArg::isREQ | CmdArg::isVALREQ);
         } else {
            // case (3, 1, 1)
            arg_syntax |= CmdArg::isVALOPT ;
         }
      } else {
         if (! fsm.level()) {
            // case (3, 0, 0)
            arg_syntax |= (CmdArg::isREQ | CmdArg::isVALREQ);
         } else if (fsm.level() == 1) {
            // case (3, 0, 1)
            if (arg_syntax & CmdArg::isREQ) {
               arg_syntax |= CmdArg::isVALOPT;
            } else {
               arg_syntax |= CmdArg::isVALREQ;
            }
         } else {
            // case (3, 0, 2)
            arg_syntax |= CmdArg::isVALOPT;
         } //if level
      } //if num-braces
   } //if num-tokens
}


//-------------------
// ^FUNCTION: parse_flag - parse a flag
//
// ^SYNOPSIS:
//    parse_flag(is)
//
// ^PARAMETERS:
//    std::istream & is;
//    -- the input stream to read the flag from.
//
// ^DESCRIPTION:
//    By specifying a string that is accepted by "parse_syntax" one
//    can specify almost any combination of CmdArg::SyntaxFlags.
//    The only ones that cannot be specified in this manner are the
//    CmdArg::isVALSTICKY and CmdArg::isVALSEP flags. In order to
//    specify these flags, we allow the syntax string to be followed
//    by a colon (':') and one of "SEPARATE" or "STICKY".
//
// ^REQUIREMENTS:
//    None.
//
// ^SIDE-EFFECTS:
//    - modifies the syntax-flags of an ArgSyntax object.
//    - prints syntax error messages on stderr.
//    - modifies the state of "is" if an error occurs.
//    - consumes characters from is.
//
// ^RETURN-VALUE:
//    A reference to the input stream used.
//
// ^ALGORITHM:
//    Trivial.
//-^^----------------
std::istream &
ArgSyntax::parse_flag(std::istream & is)
{
   char  ch;
   is >> ch;
   if (! is)  return  is;

      // If `ch' is a quote then the flags were omitted
   if ((ch == '\'') || (ch == '"')) {
      is.putback(ch);
      return  is ;
   }

      // The flags are here, make sure they start with ':'
   if (ch != ':') {
      std::cerr << "Unexpected token after syntax string.\n"
           << "\texpecting a colon, or a double or single quote." << std::endl ;
      is.clear(std::ios::failbit);
      return  is;
   }

      // Now parse the flag
   char  arg_flag[16];
   is.width(sizeof(arg_flag) - 1);
   is >> arg_flag;
   if (! is) {
      if (is.eof()) {
         std::cerr << "Error - premature end-of-input.\n"
              << "\texpecting one of \"sticky\" or \"separate\"." << std::endl ;
      } else {
         std::cerr << "Unable to extract argument flag." << std::endl ;
      }
      return  is;
   }

   char * flag = arg_flag;

      // Skip any leading "CmdArg::isVAL" portion of the flag
   if (CmdLine::strmatch("Cmd", flag, 3) == CmdLine::str_EXACT)  flag += 3;
   if (CmdLine::strmatch("Arg", flag, 3) == CmdLine::str_EXACT)  flag += 3;
   if (CmdLine::strmatch("::", flag, 2) == CmdLine::str_EXACT)   flag += 2;
   if (CmdLine::strmatch("is", flag, 2) == CmdLine::str_EXACT)   flag += 2;
   while ((*flag == '_') || (*flag == '-'))  ++flag;
   if (CmdLine::strmatch("VAL", flag, 3) == CmdLine::str_EXACT)  flag += 3;
   while ((*flag == '_') || (*flag == '-'))  ++flag;

      // check for an ambiguous flag
   if (((*flag == 's') || (*flag == 'S')) && (! *(flag + 1))) {
      std::cerr << "Ambiguous flag \"" << flag << "\"." << std::endl ;
      is.clear(std::ios::failbit);
      return  is;
   }

   if (CmdLine::strmatch("Sticky", flag) != CmdLine::str_NONE) {
      arg_syntax |= CmdArg::isVALSTICKY ;
   } else if (CmdLine::strmatch("Separate", flag) != CmdLine::str_NONE) {
      arg_syntax |= CmdArg::isVALSEP ;
   } else {
      std::cerr << "Invalid flag \"" << flag << "\".\n"
           << "\tmust be one of \"sticky\" or \"separate\"." << std::endl ;
      is.clear(std::ios::failbit);
      return  is;
   }

   return  is ;
}

//------------------------------------------------------------------ operator>>

std::istream &
operator>>(std::istream & is, ArgSyntax & arg)
{
   QuotedString  qstr(256);

   is >> qstr ;
   if (! is)  return  is;

   if (arg.parse_syntax(qstr))  return  is;
   return  arg.parse_flag(is);
}
