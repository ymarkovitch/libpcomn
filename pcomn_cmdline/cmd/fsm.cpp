//------------------------------------------------------------------------
// ^FILE: fsm.c - implement a finite staet machine
//
// ^DESCRIPTION:
//     This file implements a finite state machine tailored to the task of
//     parsing syntax strings for command-line arguments.
//
// ^HISTORY:
//    03/27/92	Brad Appleton	<bradapp@enteract.com>	Created
//-^^---------------------------------------------------------------------

#include <iostream>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "fsm.h"

   // define the characters that have a "special" meaning
enum {
  c_LBRACE = '[',
  c_RBRACE = ']',
  c_ALT    = '|',
  c_LIST   = '.',
} ;


//-------------------
// ^FUNCTION: SyntaxFSM::skip - skip to the next token
//
// ^SYNOPSIS:
//    SyntaxFSM::skip(input)
//
// ^PARAMETERS:
//    const char * & input;
//    -- the current "read" position in the syntax string.
//
// ^DESCRIPTION:
//    Skip past all whitespace and past square braced (recording the
//    current brace-nesting level and the number of balanced braces
//    parsed).
//
// ^REQUIREMENTS:
//    None.
//
// ^SIDE-EFFECTS:
//    Updates "input" to point to the next token (or eos)
//
// ^RETURN-VALUE:
//    None.
//
// ^ALGORITHM:
//    Trivial.
//-^^----------------
void
SyntaxFSM::skip(const char * & input) {
   if ((! input) || (! *input))  return;

   while (isspace(*input))  ++input;
   while ((*input == c_LBRACE) || (*input == c_RBRACE)) {
      if (*input == c_LBRACE) {
         ++lev;
      } else {
         if (lev > 0) {
            ++nbpairs;
         } else {
            fsm_state = ERROR;
            std::cerr << "too many '" << char(c_RBRACE) << "' characters." << std::endl;
         }
         --lev;
      }
      ++input;
      while (isspace(*input))  ++input;
   }//while
}


//-------------------
// ^FUNCTION: SyntaxFSM::parse_token - parse a token
//
// ^SYNOPSIS:
//    SyntaxFSM::parse_token(input)
//
// ^PARAMETERS:
//    const char * & input;
//    -- the current "read" position in the syntax string.
//
// ^DESCRIPTION:
//    Get the next token from the input string.
//
// ^REQUIREMENTS:
//    input should be non-NULL.
//
// ^SIDE-EFFECTS:
//    Updates "input" to point to the next token (or eos)
//
// ^RETURN-VALUE:
//    None.
//
// ^ALGORITHM:
//    Trivial.
//-^^----------------
void
SyntaxFSM::parse_token(const char * & input)
{
   while (*input && (! isspace(*input)) &&
          (*input != c_LBRACE) && (*input != c_RBRACE) &&
          ((*input != c_LIST) || (fsm_state == OPTION)))
   {
      ++input;
   }
}


//-------------------
// ^FUNCTION: SyntaxFSM::operator() - get a token
//
// ^SYNOPSIS:
//    SyntaxFSM::operator()(input, token)
//
// ^PARAMETERS:
//    const char * & input;
//    -- the current "read" position in the syntax string.
//
//    token_t & token;
//    -- where to place the token that we will find.
//
// ^DESCRIPTION:
//    Get the next token from the input string.
//
// ^REQUIREMENTS:
//    None.
//
// ^SIDE-EFFECTS:
//    - updates "input" to point to the next token (or eos)
//    - updates "token" to be the token that we found
//
// ^RETURN-VALUE:
//    0 if we are in a non-FINAL state; non-zero otherwise..
//
// ^ALGORITHM:
//    It gets complicated so follow along.
//-^^----------------
int
SyntaxFSM::operator()(const char * & input, token_t & token)
{
   token.set(NULL, 0);

      // if inout is NULL or empty - then we are finished
   if ((! input) || (! *input)) {
      if (lev) {
         std::cerr << "not enough '" << char(c_RBRACE) << "' characters." << std::endl ;
         fsm_state = ERROR;
         return  (fsm_state != FINAL);
      } else {
         fsm_state = FINAL;
         return  (fsm_state != FINAL);
      }
   }

   skip(input);  // skip whitespace

   const char * start = input;

      // the token we are to parse depends on what state we are in
   switch(fsm_state) {
   case  START :
      // We are parsing either an option-character name or a value.
      // If it is an option-character name, the character that stops
      // the input scan will be c_ALT.
      //
      if (*input != c_ALT)  ++input;
      if (*input == c_ALT) {
         fsm_state = OPTION;
         if (start != input)  token.set(start, 1);
      } else {
         parse_token(input);
         fsm_state = VALUE;
         token.set(start, (input - start));
      }
      ++ntoks;
      break;

   case  OPTION :
      // We parsed an option-character already so we had better see a keyword
      // name this time around.
      //
      start = ++input;  // skip past the '|' character
      if (! isspace(*input)) {
         parse_token(input);
         token.set(start, (input - start));
      }
      fsm_state = KEYWORD;
      ++ntoks;
      break;

   case  KEYWORD :
      // We parsed a keyword already - if anything is here then it better be a
      // value name.
      //
      if (*input) {
         parse_token(input);
         fsm_state = VALUE;
         token.set(start, (input - start));
         ++ntoks;
      } else {
         fsm_state = FINAL;
      }
      break;

   case  VALUE :
      // We already parsed a value name - all that could possibly be left
      // (that we be valid) is an ellipsis ("...") indicating a list.
      //
      if (! *input) {
         fsm_state = FINAL;
      } else if (::strncmp(input, "...", 3) == 0) {
         fsm_state = LIST;
         token.set(input, 3);
         input += 3;
         ++ntoks;
      } else {
         fsm_state = ERROR;
         std::cerr << "unexpected token \"" << input << "\"." << std::endl ;
      }
      break;

   case  LIST :
      // We already parsed an ellipsis, there better not be anything left
      if (! *input) {
         fsm_state = FINAL;
      } else {
         fsm_state = ERROR;
         std::cerr << "unexpected token \"" << input << "\"." << std::endl ;
      }
      break;

   case  ERROR :
   case  FINAL :
   default :
      break;
   }

   if (fsm_state == FINAL) {
      skip(input);
      if ((! *input) && lev) {
         std::cerr << "not enough '" << char(c_RBRACE) << "' characters." << std::endl ;
         fsm_state = ERROR;
      } else if (*input) {
         std::cerr << "unexpected token \"" << input << "\"." << std::endl ;
         fsm_state = ERROR;
      }
   }

   return  (fsm_state != FINAL);
}
