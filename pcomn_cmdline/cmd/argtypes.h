//------------------------------------------------------------------------
// ^FILE: argtypes.h - argument types for use by shell-scripts
//
// ^DESCRIPTION:
//   This file defines the specific types of shell-script command-line
//   arguments.  All such argument types are derived from the class
//   ShellCmdArg.
//
// ^HISTORY:
//    04/26/92	Brad Appleton	<bradapp@enteract.com>	Created
//-^^---------------------------------------------------------------------

#ifndef _argtypes_h
#define _argtypes_h

#include "shell_arg.h"

//-------------------------------------------------------------- ShellCmdArgInt

   // A ShellCmdArgInt is a CmdArg object that maintains an integer
   // valued shell-variable.

class ShellCmdArgInt : public ShellCmdArg {
public:
   ShellCmdArgInt(char *       name,
                  char         optchar,
                  char *       keyword,
                  char *       value,
                  char *       description,
                  unsigned     syntax_flags = isVALREQ)
      : ShellCmdArg(name, optchar, keyword, value, description, syntax_flags)
      {}

   ShellCmdArgInt(char *       name,
                  char *       value,
                  char *       description,
                  unsigned     syntax_flags = isPOSVALREQ)
      : ShellCmdArg(name, value, description, syntax_flags)
      {}

   virtual ~ShellCmdArgInt();

   virtual  int
   operator()(const char * & arg, CmdLine & cmd);
} ;

//------------------------------------------------------------ ShellCmdArgFloat

   // A ShellCmdArgFloat is a CmdArg object that maintains a floating-point
   // valued shell-variable.

class ShellCmdArgFloat : public ShellCmdArg {
public:
   ShellCmdArgFloat(char *       name,
                    char         optchar,
                    char *       keyword,
                    char *       value,
                    char *       description,
                    unsigned     syntax_flags = isVALREQ)
      : ShellCmdArg(name, optchar, keyword, value, description, syntax_flags)
      {}

   ShellCmdArgFloat(char *       name,
                    char *       value,
                    char *       description,
                    unsigned     syntax_flags = isPOSVALREQ)
      : ShellCmdArg(name, value, description, syntax_flags)
      {}

   virtual ~ShellCmdArgFloat();

   virtual  int
   operator()(const char * & arg, CmdLine & cmd);
} ;

//------------------------------------------------------------- ShellCmdArgChar

   // A ShellCmdArgChar is a CmdArg object that maintains a character
   // valued shell-variable.

class ShellCmdArgChar : public ShellCmdArg {
public:
   ShellCmdArgChar(char *       name,
                   char         optchar,
                   char *       keyword,
                   char *       value,
                   char *       description,
                   unsigned     syntax_flags = isVALREQ)
      : ShellCmdArg(name, optchar, keyword, value, description, syntax_flags)
      {}

   ShellCmdArgChar(char *       name,
                   char *       value,
                   char *       description,
                   unsigned     syntax_flags = isPOSVALREQ)
      : ShellCmdArg(name, value, description, syntax_flags)
      {}

   virtual ~ShellCmdArgChar();

   virtual  int
   operator()(const char * & arg, CmdLine & cmd);
} ;

//-------------------------------------------------------------- ShellCmdArgStr

   // A ShellCmdArgStr is a CmdArg object that maintains a string
   // valued shell-variable.

class ShellCmdArgStr : public ShellCmdArg {
public:
   ShellCmdArgStr(char *       name,
                  char         optchar,
                  char *       keyword,
                  char *       value,
                  char *       description,
                  unsigned     syntax_flags = isVALREQ)
      : ShellCmdArg(name, optchar, keyword, value, description, syntax_flags)
      {}

   ShellCmdArgStr(char *       name,
                  char *       value,
                  char *       description,
                  unsigned     syntax_flags = isPOSVALREQ)
      : ShellCmdArg(name, value, description, syntax_flags)
      {}

   virtual ~ShellCmdArgStr();

   virtual  int
   operator()(const char * & arg, CmdLine & cmd);
} ;

//------------------------------------------------------------- ShellCmdArgBool

   // A ShellCmdArgBool is a CmdArg object that maintains a boolean valued
   // shell variable.

class ShellCmdArgBool : public ShellCmdArg {
public:
   ShellCmdArgBool(char *       name,
                   char         optchar,
                   char *       keyword,
                   char *       description,
                   unsigned     syntax_flags = 0)
      : ShellCmdArg(name, optchar, keyword, description, syntax_flags)
      {}

   virtual ~ShellCmdArgBool();

   virtual  int
   operator()(const char * & arg, CmdLine & cmd);

   // Need to know what the values to use for TRUE and FALSE are
   //

   static const char *
   True() { return  true_string ; }

   static const char *
   False() { return  false_string ; }

   // Need to be able to set the values to use for TRUE and FALSE

   static void
   True(const char * t_str) { true_string = t_str; }

   static void
   False(const char * f_str) { false_string = f_str; }

private:
   static  const char * true_string ;
   static  const char * false_string ;

} ;

typedef  ShellCmdArgBool  ShellCmdArgSet ;

//------------------------------------------------------------ ShellCmdArgClear

class ShellCmdArgClear : public ShellCmdArgBool {
public:
   ShellCmdArgClear(char *       name,
                    char         optchar,
                    char *       keyword,
                    char *       description,
                    unsigned     syntax_flags = 0)
      : ShellCmdArgBool(name, optchar, keyword, description, syntax_flags)
      {}

   virtual ~ShellCmdArgClear();

   virtual  int
   operator()(const char * & arg, CmdLine & cmd);
} ;

//----------------------------------------------------------- ShellCmdArgToggle

class ShellCmdArgToggle : public ShellCmdArgBool {
public:
   ShellCmdArgToggle(char *       name,
                     char         optchar,
                     char *       keyword,
                     char *       description,
                     unsigned     syntax_flags = 0)
      : ShellCmdArgBool(name, optchar, keyword, description, syntax_flags)
      {}

   virtual ~ShellCmdArgToggle();

   virtual  int
   operator()(const char * & arg, CmdLine & cmd);
} ;

#endif  /* _argtypes_h */
