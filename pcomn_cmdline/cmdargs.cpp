//------------------------------------------------------------------------
// ^FILE: cmdargs.c - implement the various predefined CmdArg subclasses
//
// ^DESCRIPTION:
//    This file implements the CmdArg derived classes that are declared
//    in <cmdargs.h>
//
// ^HISTORY:
//    03/25/92	Brad Appleton	<bradapp@enteract.com>	Created
//
//    03/03/93	Brad Appleton	<bradapp@enteract.com>
//    - Added exit_handler() and quit() member-functions to CmdLine
//-^^---------------------------------------------------------------------

#include <iostream>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "cmdargs.h"
#include "exits.h"
#include "fifolist.h"

   // return values for operator()
enum { SUCCESS = 0, FAILURE = -1 } ;


//-----------------------------------------------------------------------------
// ^FUNCTION: compile, operator() - handle an argument from the command-line
//
// ^SYNOPSIS:
//    int  operator()(arg, cmd);
//    int  compile(arg, cmd, value);
//    int  compile(arg, cmd, value, default_value);
//
// ^PARAMETERS:
//    const char * & arg;
//    -- the prospective value for this command argument.
//       upon returning this value should be updated to point to the first
//       character of "arg" that was NOT used as part of the value for this
//       argument (set "arg" to NULL if all of it was used).
//
//    CmdLine & cmd;
//    -- the command that matched this argument on its command-line
//
//    <Type> & value;
//    -- The internal value (of some appropriate type) that is "managed"
//       by this command argument.
//
//    unsigned  default_value;
//    -- What to assign to "value" if "arg" is NOT a value for this command
//       argument.
//
// ^DESCRIPTION:
//    These member functions are responsible for taking whatever action
//    is appropriate when its corresponding command argument is matched
//    on the command-line.  For argument-types that simply "compile"
//    their argument into some kind of internal value, "compile()" does
//    all the work and operator() merely calls compile() with the proper
//    value as a reference parameter.
//
// ^REQUIREMENTS:
//    The "arg_flags" data member of this command-argument must have been
//    set appropriately (by "cmd") to indicate to us exactly how "arg" was
//    specified on the command-line for this (and only this) occurrence of
//    "arg".
//
// ^SIDE-EFFECTS:
//    - If (cmd.flags() & QUIET) is NOT TRUE and FAILURE is to be returned,
//      then error messages should be printed using cmd.error().
//
//    - arg is modified to be NULL of to point to the unused portion of itself.
//
//    - If we need the value of "arg" to stick around, then storage is allocated
//      in order to make  a copy of "arg" (and the command-argument is responsible
//      for de-allocating this storage).
//
// ^RETURN-VALUE:
//    FAILURE (non-zero)  If something went wrong when performing the
//                        desired actions for this command-argument.
//                        A common problem would be that "arg" is
//                        syntactically incorrect.
//
//    SUCCESS (zero)  If "arg" is NULL and/or we were able to succesfully
//                    perform all desired actions for this command argument.
//-^^--------------------------------------------------------------------------


//-------------------------------------------------------------- Dummy Argument

bool CmdArgDummy::is_dummy() const { return true ; }

   // For a CmdArgDummy - operator() is a No-OP and should NEVER
   // be called.
   //
int
CmdArgDummy::operator()(const char * & , CmdLine & )
{
   return  SUCCESS;
}

//-------------------------------------------------------------- Usage Argument

CmdArgUsage::CmdArgUsage(char         optchar,
                         const char * keyword,
                         const char * description,
                         std::ostream    * osp)
   : CmdArg(optchar, keyword, description, CmdArg::isOPT), os_ptr(osp)
{
   if (os_ptr == NULL)  os_ptr = &std::cout;
}

void
CmdArgUsage::ostream_ptr(std::ostream * osp)
{
   os_ptr = (osp != NULL) ? osp : &std::cout ;
}

   // Just need to call cmd.usage and exit.
   //
int
CmdArgUsage::operator()(const char * & , CmdLine & cmd)
{
   cmd.usage(*os_ptr, CmdLine::VERBOSE_USAGE);
   cmd.quit((cmd.flags() & CmdLine::USAGE_STATUS) ? e_USAGE : e_SUCCESS);
   return  SUCCESS;
}

//----------------------------------------------------------- Integer Arguments

   // Compile a string into an integer value.
int
CmdArgIntCompiler::compile(const char * & arg, CmdLine & cmd, int & value)
{
   const char * ptr = NULL ;
   long  result = 0 ;

   if (arg == NULL) {
      return  SUCCESS ;  // no value given - nothing to do
   } else if (! *arg) {
      if (! (cmd.flags() & CmdLine::QUIET)) {
         cmd.error() << "empty integer value specified." << std::endl ;
      }
      return  FAILURE ;
   }

   // compile the string into an integer
   result = ::strtol(arg, (char **) &ptr, 0);  // watch out for -c0xa vs -axc0!
   if (ptr == arg) {
      // do we have a valid integer?
      if (! (cmd.flags() & CmdLine::QUIET)) {
         cmd.error() << "invalid integer value \"" << arg << "\"." << std::endl ;
      }
      return  FAILURE ;
   }
   value = (int) result;
   arg = ptr;

   return  SUCCESS ;
}

int
CmdArgInt::operator()(const char * & arg, CmdLine & cmd)
{
   return  compile(arg, cmd, val);
}

std::ostream &
operator<<(std::ostream & os, const CmdArgInt & int_arg)
{
   return  (os << (int) int_arg) ;
}

//---------------------------------------------------- Floating-point Arguments
   // Compile a string into a floating-point value.
int
CmdArgFloatCompiler::compile(const char * & arg, CmdLine & cmd, float & value)
{
   const char * ptr = NULL ;
   double  result = 0 ;

   if (arg == NULL) {
      return  SUCCESS ;  // no value given -- nothing to do
   } else if (! *arg) {
      if (! (cmd.flags() & CmdLine::QUIET)) {
         cmd.error() << "empty floating-point value specified." << std::endl ;
      }
      return  FAILURE ;
   }

   result = ::strtod(arg, (char **) &ptr);  // compile the string into a float
   if (ptr == arg) {
      // do we have a valid float?
      if (! (cmd.flags() & CmdLine::QUIET)) {
         cmd.error() << "invalid floating-point value \"" << arg << "\"."
                     << std::endl ;
      }
      return  FAILURE ;
   }
   value = (float) result;
   arg = ptr;

   return  SUCCESS ;
}

int
CmdArgFloat::operator()(const char * & arg, CmdLine & cmd)
{
   return  compile(arg, cmd, val);
}


std::ostream &
operator<<(std::ostream & os, const CmdArgFloat & float_arg)
{
   return  (os << (float) float_arg) ;
}

//--------------------------------------------------------- Character Argumrnts
int
CmdArgCharCompiler::compile(const char * & arg, CmdLine & cmd, char & value)
{
   if (arg == NULL) {
      return  SUCCESS ;  // no value given - nothing to do
   }

   // If "arg" contains more than 1 character, then the other characters
   // are either extraneous, or they are options (bundled together).
   //
   if (*arg  &&  *(arg+1)  &&
        ((! (flags() & CmdArg::OPTION)) || (flags() & CmdArg::VALSEP)))
   {
      if (! (cmd.flags() & CmdLine::QUIET)) {
         cmd.error() << "invalid character value \"" << arg << "\"." << std::endl ;
      }
      return  FAILURE ;
   }

   value = *arg;
   if (*arg) {
      ++arg;
   } else {
      arg = NULL;
   }

   return  SUCCESS ;
}

int
CmdArgChar::operator()(const char * & arg, CmdLine & cmd)
{
   return  compile(arg, cmd, val);
}

std::ostream &
operator<<(std::ostream & os, const CmdArgChar & char_arg)
{
   return  (os << (char) char_arg) ;
}

//------------------------------------------------------------ String Arguments

typedef  CmdArgStrCompiler::casc_string  CmdArgString ;

   // Copy a string (allocating storage if necessary)
void
CmdArgString::copy(bool source_is_temporary, const char *s)
{
   if (is_alloc) delete [] (char *)str;
   str = NULL ;
   is_alloc = source_is_temporary ;
   if (is_alloc && s)
      s = ::strcpy(new char[::strlen(s) + 1], s) ;
   str = s;
}

int
CmdArgStrCompiler::compile(const char  * & arg,
                           CmdLine       &,
                           CmdArgString  & value)
{
   if (arg == NULL) {
      return  SUCCESS;  // no value given -- nothing to do
   }

   value.copy(true, arg);
   arg = NULL;

   return  SUCCESS;
}

int
CmdArgStr::operator()(const char * & arg, CmdLine & cmd)
{
   return  compile(arg, cmd, val);
}

std::ostream &
operator<<(std::ostream & os, const CmdArgStrCompiler::casc_string & str)
{
   return  (os << str.str) ;
}

std::ostream &
operator<<(std::ostream & os, const CmdArgStr & str_arg)
{
   return  (os << (const char *) str_arg) ;
}

//-------------------------------------------------------------- List Arguments

          //------------------- Integer List -------------------

DECLARE_FIFO_LIST(IntList, int);

struct CmdArgIntListPrivate {
   IntList       list;
   IntListArray  array;

   CmdArgIntListPrivate();
} ;


CmdArgIntListPrivate::CmdArgIntListPrivate()
   : array(list)
{
   list.self_cleaning(1);
}

   // Compile the argument into an integer and append it to the list
int
CmdArgIntList::operator()(const char * & arg, CmdLine & cmd)
{
   int  value;
   const char * save_arg = arg;
   int  rc = compile(arg, cmd, value);
   if (save_arg && (rc == SUCCESS)) {
      if (val == NULL)  val = new CmdArgIntListPrivate;
      val->list.add(new int(value));
   }
   return  rc;
}

unsigned
CmdArgIntList::count() const
{
   return  (val) ? val->list.count() : 0 ;
}

int &
CmdArgIntList::operator[](unsigned  index)
{
   return  val->array[index];
}

CmdArgIntList::~CmdArgIntList() { delete val; }


          //------------------- Float List -------------------


DECLARE_FIFO_LIST(FloatList, float);

struct CmdArgFloatListPrivate {
   FloatList       list;
   FloatListArray  array;

   CmdArgFloatListPrivate();
} ;

CmdArgFloatListPrivate::CmdArgFloatListPrivate()
   : array(list)
{
   list.self_cleaning(1);
}


   // Compile the argument into a float and append it to the list
int
CmdArgFloatList::operator()(const char * & arg, CmdLine & cmd)
{
   float  value;
   const char * save_arg = arg;
   int  rc = compile(arg, cmd, value);
   if (save_arg && (rc == SUCCESS)) {
      if (val == NULL)  val = new CmdArgFloatListPrivate;
      float * new_value = new float;
      *new_value = value;
      val->list.add(new_value);
   }
   return  rc;
}

unsigned
CmdArgFloatList::count() const
{
   return  (val) ? val->list.count() : 0 ;
}

float &
CmdArgFloatList::operator[](unsigned  index)
{
   return  val->array[index];
}

CmdArgFloatList::~CmdArgFloatList() { delete val; }

          //------------------- String List -------------------

DECLARE_FIFO_LIST(StringList, CmdArgString);

struct CmdArgStrListPrivate {
   StringList       list;
   StringListArray  array;

   CmdArgStrListPrivate();
} ;

CmdArgStrListPrivate::CmdArgStrListPrivate()
   : array(list)
{
   list.self_cleaning(1);
}

int
CmdArgStrList::operator()(const char * & arg, CmdLine & cmd)
{
   CmdArgString * value = new CmdArgString ;
   const char * save_arg = arg;
   int  rc = compile(arg, cmd, *value);
   if (save_arg && (rc == SUCCESS)) {
      if (val == NULL)  val = new CmdArgStrListPrivate;
      val->list.add(value);
   } else {
      delete  value;
   }
   return  rc;
}

unsigned
CmdArgStrList::count() const
{
   return  (val) ? val->list.count() : 0 ;
}

CmdArgString &
CmdArgStrList::operator[](unsigned  index)
{
   return  val->array[index];
}

CmdArgStrList::~CmdArgStrList() { delete val; }

//----------------------------------------------------------- Boolean Arguments

int
CmdArgBoolCompiler::compile(const char  *&arg, CmdLine &cmd, bool &value, bool default_value)
{
   if (arg == NULL) {
         // if no argument was given use the default
      value = default_value ;
   } else {
      char ch = tolower(*arg);
      const char * kwd = arg++;

         // Map the argument to the corresponding value. We will accept
         // the following (case insensitive):
         //
         //     "+", "1", "ON", or "YES"  means set the value
         //     "-", "0", "OFF", or "NO"  means clear the value
         //     "~", "^", or "!" means toggle the value
         //
         // Anything else is considered to be an argument that is NOT
         // meant for us but for some other argument so we just use the
         // default value that was supplied and return SUCCESS.
         //
      switch(ch) {
         case '1' :
         case '+' : value = 1 ; break;

         case '0' :
         case '-' : value = 0 ; break;

         case '~' :
         case '^' :
         case '!' : value = (! value) ; break;

         default:
            if (flags() & CmdArg::KEYWORD) {
               char ch2 = *arg;
               arg = NULL;
               if (cmd.strmatch(kwd, "yes") != CmdLine::str_NONE) {
                  value = 1 ;
                  return  SUCCESS ;
               } else if (cmd.strmatch(kwd, "no") != CmdLine::str_NONE) {
                  value = 0 ;
                  return  SUCCESS ;
               } else if (cmd.strmatch(kwd, "true") != CmdLine::str_NONE) {
                  value = 1 ;
                  return  SUCCESS ;
               } else if (cmd.strmatch(kwd, "false") != CmdLine::str_NONE) {
                  value = 0 ;
                  return  SUCCESS ;
               } else if ((ch == 'o') && (! ch2)) {
                  // ambiguous - could be "ON" or "OFF"
                  if (! (cmd.flags() & CmdLine::QUIET)) {
                     cmd.error() << "ambiguous boolean value \"" << kwd
                                 << "\"." << std::endl ;
                  }
                  return  FAILURE ;
               } else if (cmd.strmatch(kwd, "on") != CmdLine::str_NONE) {
                  value = 1 ;
                  return  SUCCESS ;
               } else if (cmd.strmatch(kwd, "off") != CmdLine::str_NONE) {
                  value = 0 ;
                  return  SUCCESS ;
               } else {
                  // unknown
                  if (! (cmd.flags() & CmdLine::QUIET)) {
                     cmd.error() << "unknown boolean value \"" << kwd
                                 << "\"." << std::endl ;
                  }
                  return  FAILURE ;
               }
            } //if keyword
            arg = kwd;  // no characters used!
            value = default_value ;
            break;
      } //switch
   } //else

   return  SUCCESS ;
}

std::ostream &
operator<<(std::ostream & os, const CmdArgBool & bool_arg)
{
   return  (os << ((int)bool_arg)) ;
}

//------------------------------------------------------------------ CmdArgBool

int
CmdArgBool::operator()(const char * & arg, CmdLine & cmd)
{
   bool value = val;
   const int rc = compile(arg, cmd, value, true);
   val = value;
   return  rc;
}

//----------------------------------------------------------------- CmdArgClear

int
CmdArgClear::operator()(const char * & arg, CmdLine & cmd)
{
   bool value = val;
   const int rc = compile(arg, cmd, value, false);
   val = value;
   return  rc;
}

//---------------------------------------------------------------- CmdArgToggle

int
CmdArgToggle::operator()(const char * & arg, CmdLine & cmd)
{
   bool value = val;
   const int rc = compile(arg, cmd, value, !value);
   val = value;
   return  rc;
}

//--------------------------------------------------------------- CmdArgBoolRef

int
CmdArgBoolRef::operator()(const char * & arg, CmdLine & cmd)
{
   bool val = ref;
   const int rc = ref.compile(arg, cmd, val, true);
   ref = val;
   return  rc;
}

//-------------------------------------------------------------- CmdArgClearRef

int
CmdArgClearRef::operator()(const char * & arg, CmdLine & cmd)
{
   bool val = ref;
   const int rc = ref.compile(arg, cmd, val, 0);
   ref = val;
   return  rc;
}

//------------------------------------------------------------- CmdArgToggleRef

int
CmdArgToggleRef::operator()(const char * & arg, CmdLine & cmd)
{
   bool val = ref;
   const int rc = ref.compile(arg, cmd, val, (! val));
   ref = val;
   return  rc;
}
