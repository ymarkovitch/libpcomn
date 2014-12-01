//------------------------------------------------------------------------
// ^FILE: cmdarg.c - implement a CmdArg
//
// ^DESCRIPTION:
//    This file implements the CmdArg class which is the base
//    class for all command-arguments.
//
// ^HISTORY:
//    03/25/92	Brad Appleton	<bradapp@enteract.com>	Created
//
//    03/01/93	Brad Appleton	<bradapp@enteract.com>
//    - Added arg_sequence field to CmdArg
//-^^---------------------------------------------------------------------

#include <iostream>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "cmdline.h"

//---------------------------------------------------------------------- CmdArg

bool CmdArg::is_dummy() const { return !!(syntax() & isIGNORED) ; }

   // Constructors

CmdArg::CmdArg(char         optchar,
               const char * keyword,
               const char * value,
               const char * description,
               unsigned     syntax_flags)
   : arg_flags(0),
     arg_syntax(syntax_flags),
     arg_sequence(0),
     alloc_value_name(false),
     arg_char_name(optchar),
     arg_keyword_name(keyword),
     arg_value_name(value),
     arg_description(description)
{
   parse_description();
   parse_value_name();
   adjust_syntax();
}


CmdArg::CmdArg(char         optchar,
               const char * keyword,
               const char * description,
               unsigned     syntax_flags)
   : arg_flags(0),
     arg_syntax(syntax_flags),
     arg_sequence(0),
     alloc_value_name(false),
     arg_char_name(optchar),
     arg_keyword_name(keyword),
     arg_value_name(NULL),
     arg_description(description)
{
   parse_description();
   adjust_syntax();
}


CmdArg::CmdArg(const char * value,
               const char * description,
               unsigned     syntax_flags)
   : arg_flags(0),
     arg_syntax(syntax_flags),
     arg_sequence(0),
     alloc_value_name(false),
     arg_char_name(0),
     arg_keyword_name(NULL),
     arg_value_name(value),
     arg_description(description)
{
   parse_description();
   parse_value_name();
   adjust_syntax();
}


   // Destructor
CmdArg::~CmdArg()
{
   if (alloc_value_name)    delete [] (char *)arg_value_name;
}


//-------------------
// ^FUNCTION: adjust_syntax - adjust command argument syntax
//
// ^SYNOPSIS:
//    CmdArg::adjust_syntax()
//
// ^PARAMETERS:
//    None.
//
// ^DESCRIPTION:
//    This routine tries to "iron out" any inconsistencies (such as
//    conflicting syntax flags) in the way a command-argument is specified
//    and makes its best guess at what the user eally intended.
//
// ^REQUIREMENTS:
//    parse_value_name() and parse_description() must already have been called.
//
// ^SIDE-EFFECTS:
//    Modifies the argument syntax flags.
//    Modifies is keyword and value names if they are ""
//
// ^RETURN-VALUE:
//    None.
//
// ^ALGORITHM:
//    Follow along in the code ...
//-^^----------------
void
CmdArg::adjust_syntax()
{
   static const char default_value_name[] = "value" ;

      // If the value is specified as both OPTIONAL and REQUIRED
      // then assume it is required.
      //
   if ((arg_syntax & isVALREQ) && (arg_syntax & isVALOPT)) {
      arg_syntax &= ~isVALOPT ;
   }

      // If they said the argument was both STICKY and SEPARATE then
      // I dont know what to think just just ignore both of them.
      //
   if ((arg_syntax & isVALSTICKY) && (arg_syntax & isVALSEP)) {
      arg_syntax &= ~(isVALSTICKY | isVALSEP);
   }

      // If a non-NULL, non-empty value-name was given but we werent
      // told that the argument takes a value, then assume that it
      // does take a value and that the value is required.
      //
   if (arg_value_name && *arg_value_name && (! (arg_syntax & isVALTAKEN))) {
      arg_syntax |= isVALREQ;
   }

      // If a value is taken and the argument is positional, then
      // we need to make isREQ consistent with isVALREQ
      // and isVALOPT
      //
   if ((arg_syntax & isVALTAKEN) && (arg_syntax & isPOS)) {
      if (arg_syntax & isVALREQ)
        arg_syntax |= isREQ ;
      else
        arg_syntax &= ~isREQ ;
   }

      // If the keyword name is empty then just use NULL
   if (arg_keyword_name  &&  (! *arg_keyword_name)) {
      arg_keyword_name = NULL;
   }

      // If the value name is empty then just use NULL
   if (arg_value_name  &&  (! *arg_value_name)) {
      arg_value_name = NULL;
   }

      // If a value is taken but no value name was given,
      // then default the value name.
      //
   if ((arg_syntax & isVALTAKEN) && (! arg_value_name)) {
      arg_value_name = default_value_name;
   }

      // If no keyword name or character name was given, then the
      // argument had better take a value and it must be positional
      //
   if ((! arg_char_name) && (arg_keyword_name == NULL) &&
                            (! (arg_syntax & isPOS))) {
      if (arg_syntax & isVALTAKEN) {
         arg_syntax |= isPOS;
      } else {
         std::cerr << "*** Error: non-positional CmdArg "
              << "has no character or keyword name!\n"
              << "\t(error occurred in CmdArg constructor)" << std::endl ;
      }
   }
}


//-------------------
// ^FUNCTION: parse_description - parse the argument description string
//
// ^SYNOPSIS:
//    CmdLine::parse_description()
//
// ^PARAMETERS:
//    None.
//
// ^DESCRIPTION:
//    All we have to do is see if the first non-white character of
//    the description is string is ';'. If it is, then the argument
//    is a "hidden" argument and the description starts with the
//    next non-white character.
//
// ^REQUIREMENTS:
//    None.
//
// ^SIDE-EFFECTS:
//    Modifies arg_description
//
// ^RETURN-VALUE:
//    None.
//
// ^ALGORITHM:
//    Trivial.
//-^^----------------
enum { c_HIDDEN = ';', c_OPEN = '[', c_CLOSE = ']', c_LIST = '.' } ;

void
CmdArg::parse_description()
{
   if (arg_description == NULL)  return;
   while (isspace(*arg_description))  ++arg_description;
   if (*arg_description == c_HIDDEN) {
      arg_syntax |= isHIDDEN ;
      ++arg_description;
      while (isspace(*arg_description))  ++arg_description;
   }
}


//-------------------
// ^FUNCTION: parse_value - parse the argument value name
//
// ^SYNOPSIS:
//    CmdLine::parse_value_name()
//
// ^PARAMETERS:
//    None.
//
// ^DESCRIPTION:
//    This routine parses the argument value string. If the value name is
//    is enclosed between '[' and ']', then the value is optional (not
//    required) and we need to modify the arg_syntax flags. Also, if the
//    value name is suffixed by "..." then it means the value is a LIST
//    of values and we need to modify the arg_syntax flags.
//
// ^REQUIREMENTS:
//    This routine must be called BEFORE, adjust_syntax() is called/
//
// ^SIDE-EFFECTS:
//    Modifies the arg_value_name and the arg_syntax flags.
//
// ^RETURN-VALUE:
//    None.
//
// ^ALGORITHM:
//    Its kind of hairy so follow along.
//-^^----------------
void
CmdArg::parse_value_name()
{
   if (!arg_value_name) return;

   const char * save_value = arg_value_name;
   int  brace = 0;
   int  errors = 0;

   // Skip whitespace as necessary and look for a '['
   while (isspace(*arg_value_name))  ++arg_value_name;
   if (*arg_value_name == c_OPEN) {
      ++brace;
      ++arg_value_name;
      while (isspace(*arg_value_name))  ++arg_value_name;
      arg_syntax &= ~isVALREQ;
      arg_syntax |= isVALOPT;
   }

   // Now that arg_value_name points to the beginning of the value,
   // lets find the end of it.
   //
   const char * ptr = arg_value_name;
   while ((*ptr) && (! isspace(*ptr)) &&
          (*ptr != c_LIST) && (*ptr != c_CLOSE)) {
      ++ptr;
   }

   // See if we dont need to allocate a new string
   if ((! *ptr) && (save_value == arg_value_name))  return;

   // Copy the value name
   alloc_value_name = true;
   int  len = (int) (ptr - arg_value_name);
   char * copied_value = new char[len + 1];
   (void) ::strncpy(copied_value, arg_value_name, len);
   copied_value[len] = '\0';
   arg_value_name = copied_value;

   // Did we end on a ']' ?
   if (*ptr == c_CLOSE) {
      if (! brace) {
         std::cerr << "Error: unmatched '" << char(c_CLOSE) << "'." << std::endl ;
         ++errors;
         arg_syntax &= ~isVALREQ;
         arg_syntax |= isVALOPT;
      }
      brace = 0;
      ++ptr;
   }

   // Skip whitespace and see if we are finished.
   while (isspace(*ptr))  ++ptr;
   if (! *ptr) {
      // Was there an unmatched ']'
      if (brace) {
         std::cerr << "Error: unmatched '" << char(c_OPEN) << "'." << std::endl ;
         ++errors;
      }
      if (errors) {
         std::cerr << "*** Syntax error in value \"" << save_value << "\".\n"
              << "\t(error occurred in CmdArg constructor)" << std::endl ;
      }
      return;
   }

   // Not done - we had better see a "..."
   if (::strncmp(ptr, "...", 3) != 0) {
      std::cerr << "Error: unexpected token \"" << ptr << "\"." << std::endl ;
      ++errors;
   } else {
      arg_syntax |= isLIST;
      ptr += 3;
      while (isspace(*ptr))  ++ptr;
      if (brace && (*ptr != c_CLOSE)) {
         std::cerr << "Error: unmatched '" << char(c_OPEN) << "'." << std::endl ;
         ++errors;
      } else {
        // If theres anything left (except ']') it's an error
        if (brace && (*ptr == c_CLOSE))  ++ptr;
        while (isspace(*ptr))  ++ptr;
        if (*ptr) {
           std::cerr << "Error: unexpected token \"" << ptr << "\"." << std::endl ;
           ++errors;
        }
      }
   }

   // Were there any errors?
   if (errors) {
      std::cerr << "*** Syntax error in value \"" << save_value << "\".\n"
           << "\t(error occurred in CmdArg constructor)" << std::endl ;
   }
}

char *CmdArg::valstr(char *buf, size_t size, ValStr what_value) const
{
  if (!buf || !size)
    return NULL ;
  switch (what_value) {
    case VALSTR_ARGVAL:
    case VALSTR_DEFVAL:
      *buf = 0 ;
      return buf ;

    case VALSTR_DEFNOZERO:
    default:
      return NULL ;
  }
}
