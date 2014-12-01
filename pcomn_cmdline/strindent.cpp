//-------------------------------------------------------------------------
// ^FILE: strindent.c - implement the strindent() function
//
// ^DESCRIPTION:
//    This file implements the function strmatch() which matches a
//    keyword (case insensitive) and the function strindent() which
//    prints a hanging, indented paragraph.
//
//    On VMS systems - we also implement a replacement for "getenv()"
//    named "getsym()" to get the value of a VMS symbol.
//
// ^HISTORY:
//    12/05/91 	Brad Appleton 	<bradapp@enteract.com> 	Created
//-^^-----------------------------------------------------------------------

#include <iostream>
#include <iomanip>

#include <string.h>
#include <ctype.h>

#include "cmdline.h"

//-------
// ^FUNCTION: strmatch - match a keyword
//
// ^SYNOPSIS:
//    static CmdLine::strmatch_t CmdLine::strmatch(src, attempt, len);
//
// ^PARAMETERS:
//    const char * src;
//    -- the actual keyword to match against
//
//    const char * attempt;
//    -- the "candidate" that may or may not match the keyword
//
//    unsigned len;
//    -- the number of character of "attempt" to consider (==0 if all
//       characters of "attempt" should be used).
//
// ^DESCRIPTION:
//    See if "attempt" matches "src" (either partially or completely) and
//    return the result.
//
// ^REQUIREMENTS:
//    None that havent been discusses in the PARAMETERS section.
//
// ^SIDE-EFFECTS:
//    None.
//
// ^RETURN-VALUE:
//    str_EXACT if "attempt" completely matches "src"
//    str_PARTIAL is "attempt" partially matches "src"
//    str_NONE otherwise
//
// ^ALGORITHM:
//    For each character (in order) of "attempt" to be considered
//       if attempt[i] != src[i] (case insensitive) return str_NONE
//    end-for
//    if we have exhausted "src" return str_EXACT,
//    else return str_PARTIAL
//-^^----
CmdLine::strmatch_t
CmdLine::strmatch(const char * src, const char * attempt, unsigned len)
{
   unsigned  i;

   if (src == attempt)  return  str_EXACT ;
   if ((src == NULL) || (attempt == NULL))  return  str_NONE ;
   if ((! *src) && (! *attempt))  return  str_EXACT ;
   if ((! *src) || (! *attempt))  return  str_NONE ;

   for (i = 0 ; ((i < len) || (! len)) && (attempt[i] != '\0') ; i++) {
      if (tolower(src[i]) != tolower(attempt[i]))  return  str_NONE ;
   }

   return  (src[i] == '\0') ? str_EXACT : str_PARTIAL ;
}


//--------------------------------------------------------------------------
// ^FUNCTION: strindent - print a hanging indented paragraph
//
// ^SYNOPSIS:
//    void CmdLine::strindent(os, maxcols, margin, title, indent, text)
//
// ^PARAMETERS:
//    std::ostream & os;
//    -- the stream to which output is sent
//
//    unsigned maxcols;
//    -- the maximum width (in characters) of the output
//
//    unsigned margin;
//    -- the number of spaces to use as the left margin
//
//    char * title;
//    -- the paragraph title
//
//    unsigned indent;
//    -- the distance between the title and the paragraph body
//
//    char * text;
//    -- the body of the paragraph
//
// ^DESCRIPTION:
//    Strindent will print on os, a titled, indented paragraph as follows:
//
//    <----------------------- maxcols --------------------------->
//    <--- margin --><----- indent ---->
//                   title              This is the first sentence
//                                      of the paragraph. Etc ...
//
// ^REQUIREMENTS:
//    - maxcols and indent must be positive numbers with maxcols > indent.
//    - title should NOT contain any tabs or newlines.
//
// ^SIDE-EFFECTS:
//    Output is printed to os.
//
// ^RETURN-VALUE:
//    None.
//
// ^ALGORITHM:
//    Print the paragraph title and then print the text.
//    Lines are automatically adjusted so that each one starts in the
//    appropriate column.
//-^^-----------------------------------------------------------------------
void
CmdLine::strindent(std::ostream    & os,
                   unsigned     maxcols,
                   unsigned     margin,
                   const char * title,
                   unsigned     indent,
                   const char * text)
{
   // If we were given non-sensical parameters then dont use them
   if (margin > maxcols)  margin = 0;
   if ((indent + margin) >= maxcols)  indent = 1;

   // print the title (left-justified)
   const std::ios::fmtflags save_flags = os.flags();
   os << std::setw(margin) << ""
      << std::left << std::setw(indent) << ((title) ? title : "") ;
   os.flags(save_flags);

   if (text == NULL) {
      os << std::endl ;
      return;
   }

   // If the title is too big, start the paragraph on a new line
   if (title  &&  (::strlen(title) > indent))
      os << std::endl << std::setw(margin + indent) << "";

   // Loop through the paragraph text witing to print until we absolutely
   // have to.
   //
   unsigned  col = margin + indent + 1;
   unsigned  index = 0 ;
   unsigned  last_white = 0 ;
   const char * p = text ;

   while (p[index]) {
      switch (p[index]) {
         // If we have a space - just remember where it is
      case ' ' :
         last_white = index;
         ++col;
         ++index;
         break;

         // If we have a tab - remember where it is and assume it
         // will take up 8 spaces in the output.
         //
      case '\t' :
         last_white = index;
         col += 8;
         ++index;
         break;

         // If we have a form-feed, carriage-return, or newline, then
         // print what we have so far (including this character) and
         // start a new line.
         //
      case '\n' :
      case '\r' :
      case '\f' :
         os.write(p, index + 1);
         p += index + 1;
         col = margin + indent + 1;
         index = last_white = 0;
         if (*p) {
            os.width(margin + indent);
            os << "";
         }
         break;

      default:
         ++col;
         ++index;
         break;
      } //switch

         // Are we forced to start a new line?
      if (col > maxcols) {
            // Yes - if possible, print upto the last whitespace character
            //       and start the next line on a word-boundary
         if (last_white) {
            os.write(p, last_white);
            p += last_white;
            while (*p == ' ')  ++p;
         } else {
               // No word boundary in sight - just split the line here!
            os.write(p, index);
            p += index;
         }
         os << std::endl ;

            // We just printed a newline - dont print another one right now
         while ((*p == '\n') || (*p == '\r') || (*p == '\f'))  ++p;
         col = margin + indent + 1;
         index = last_white = 0;
         if (*p) {
            os.width(margin + indent);
            os << "";
         }
      } else if (index && (! p[index])) {
         os << p << std::endl ;
      }
   } //while
}


#ifdef vms

#  include <descrip.h>
#  include <libdef.h>

   extern "C" {
      long   lib$get_symbol(...);
      void   exit(int);
   }

//----------------------
// ^FUNCTION: getsym - retrieve the value of a VMS symbol
//
// ^SYNOPSIS:
//    const char * getsym(sym_name)
//
// ^PARAMETERS:
//    char * sym_name;
//    -- name of the symbol to retrieve
//
// ^DESCRIPTION:
//    Get_symbol will lookup the named symbol and return its value
//    as a string.
//
// ^REQUIREMENTS:
//    sym_name should correspond to the name of a pre-defined symbol.
//
// ^SIDE-EFFECTS:
//    None.  NO storage is allocated for the symbol-value and it will
//    be overwritten by the next call to this function.
//
// ^RETURN-VALUE:
//    NULL if the symbol is not found, otherwise it returns a pointer
//    to a static buffer containing the contents of the symbol value.
//    You MUST be finished using this buffer before another call to
//    get_symbol is made.
//
// ^ACKNOWLEDGEMENTS:
//    Thanx to Jim Barbour for writing most of this code. --BDA
//
// ^ALGORITHM:
//    Trivial - just use the LIB$GET_SYMBOL system service.
//-^^-------------------
const char *
getsym(const char * sym_name)
{
   static  char  sym_value[256];
   unsigned long   stat;
   unsigned short  buflen;
   $DESCRIPTOR(sym_name_d, sym_name);
   $DESCRIPTOR(sym_value_d, sym_value);

   sym_value_d.dsc$w_length = sizeof(sym_value);
   sym_name_d.dsc$w_length = ::strlen(sym_name);
   stat = lib$get_symbol(&sym_name_d, &sym_value_d, &buflen, (void *)0);
   if (stat == LIB$_NOSUCHSYM) {
      return  NULL;
   } else if (! (stat & 1)) {
      ::exit(stat);
   }
   sym_value[buflen] = '\0';
   return  sym_value;
}

#endif  /* vms */
