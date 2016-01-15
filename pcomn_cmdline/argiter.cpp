//------------------------------------------------------------------------
// ^FILE: argiter.c - implementation of CmdLineArgIter subclasses
//
// ^DESCRIPTION:
//    This file implements the derived classes of a CmdLineArgIter that
//    are declazred in <cmdline.h>.
//
// ^HISTORY:
//    04/03/92	Brad Appleton	<bradapp@enteract.com>	Created
//-^^---------------------------------------------------------------------

#include <iostream>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "cmdline.h"

//-------------------------------------------------------- class CmdLineArgIter

CmdLineArgIter::CmdLineArgIter() {}

CmdLineArgIter::~CmdLineArgIter() {}

//----------------------------------------------------------- class CmdArgvIter

CmdArgvIter::~CmdArgvIter() {}

const char *
CmdArgvIter::operator()() {
   return  ((index != count) && (array[index])) ? array[index++] : 0 ;
}

//--------------------------------------------------------- class CmdStrTokIter

static const char WHITESPACE[] = " \t\n\r\v\f" ;

   // Use a new string and a new set of delimiters
void
CmdStrTokIter::reset(const char * tokens, const char * delimiters)
{
   seps = delimiters ? delimiters : WHITESPACE ;

   delete [] tokstr;
   tokstr = NULL;
   token = NULL;
   if (tokens && *tokens) {
      // Make a copy of the token-string (because ::strtok() modifies it)
      // and get the first token from the string
      //
      tokstr = ::strcpy(new char[::strlen(tokens) + 1], tokens) ;
      token = ::strtok(tokstr, seps);
   }
}

   // Iterator function -- operator()
   //   Just use ::strtok to get the next token from the string
   //
const char *
CmdStrTokIter::operator()()
{
   if (seps == NULL)  seps = WHITESPACE ;
   const char * result = token;
   if (token) token = ::strtok(NULL, seps);
   return  result;
}

//-------------------------------------------------------- class CmdIstreamIter

const unsigned  CmdIstreamIter::MAX_LINE_LEN = 1024 ;

   // Constructor
CmdIstreamIter::CmdIstreamIter(std::istream & input) : is(input), tok_iter(NULL)
{
}

   // Destructor
CmdIstreamIter::~CmdIstreamIter()
{
   delete  tok_iter;
}

   enum { c_COMMENT = '#' } ;

   // Iterator function -- operator()
   //
   // What we do is this: for each line of text in the std::istream, we use
   // a CmdStrTokIter to iterate over each token on the line.
   //
   // If the first non-white character on a line is c_COMMENT, then we
   // consider the line to be a comment and we ignore it.
   //
const char *
CmdIstreamIter::operator()()
{
   const char * result = NULL;
   if (tok_iter)  result = tok_iter->operator()();
   if (result)  return  result;
   if (! is)  return  NULL;

   char buf[CmdIstreamIter::MAX_LINE_LEN];
   do {
      *buf = '\0';
      is.getline(buf, sizeof(buf));
      char * ptr = buf;
      while (isspace(*ptr)) ++ptr;
      if (*ptr && (*ptr != c_COMMENT)) {
         if (tok_iter) {
            tok_iter->reset(ptr);
         } else {
            tok_iter = new CmdStrTokIter(ptr);
         }
         return  tok_iter->operator()();
      }
   } while (is);
   return  NULL;
}
