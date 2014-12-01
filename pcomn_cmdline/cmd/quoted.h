//------------------------------------------------------------------------
// ^FILE: quoted.h - a class for "quoted" strings
//
// ^DESCRIPTION:
//    This file implements a quoted-string class.  The main purpose of
//    this class is the input extraction operator (operator>>) which
//    reads a quoted string from input (enclosed in either single or
//    double quotes) and places the result (minus containing quotes)
//    into a character string.  Single and double quotes may be made part
//    of the string be preceding them with a backslash ('\') in the input
//    stream.
//
// ^HISTORY:
//    05/01/92	Brad Appleton	<bradapp@enteract.com>	Created
//-^^---------------------------------------------------------------------

#ifndef _quoted_h
#define _quoted_h

#include <iostream>

class  QuotedString {
public:
      // constructors and destructors
   QuotedString(unsigned  max_size);

   QuotedString(const char * str);

   QuotedString(const char * str, unsigned  max_size);

   QuotedString(const QuotedString & qstr);

   virtual ~QuotedString();

      // assignment
   QuotedString &
   operator=(const QuotedString & qstr);

   QuotedString &
   operator=(const char * str);

      // convert to a string
   operator  char*() { return  buffer; }

      // operator >> reads a quoted string from input.
      // If no beginning or ending quote is seen, than
      // a message is printed on cerr and the failbit
      // of the input stream is set.
      //
   friend  std::istream &
   operator>>(std::istream & is, QuotedString & qstr);

private:
   unsigned  size;
   char    * buffer;
} ;

#endif  /* _quoted_h */
