//------------------------------------------------------------------------
// ^FILE: shells.c - implement classes for the various Unix shells
//
// ^DESCRIPTION:
//     This file packages all the information we need to know about each
//  of the shells that cmdparse(1) will support into a set of (sub)classes.
//
// ^HISTORY:
//    04/19/92	Brad Appleton	<bradapp@enteract.com>	Created
//-^^---------------------------------------------------------------------

#include <iostream>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <fifolist.h>

#include "shells.h"
#include "argtypes.h"

//--------------------------------------------------------------- ShellVariable

ShellVariable::ShellVariable(const char * name)
   : var_name(name), var_value(NULL)
{
}

ShellVariable::~ShellVariable()
{
}

//------------------------------------------------------------ ShellArrayValues

DECLARE_FIFO_LIST(CharPtrList, char *);

struct  ShellArrayValues {
   CharPtrList       list;
   CharPtrListArray  array;

   ShellArrayValues();
} ;

ShellArrayValues::ShellArrayValues()
   : array(list)
{
   list.self_cleaning(1);
}

//------------------------------------------------------------------ ShellArray

ShellArray::ShellArray(const char * name)
   : array_name(name), array_value(NULL)
{
}

ShellArray::~ShellArray()
{
   delete  array_value ;
}

void
ShellArray::append(const char * value)
{
   if (array_value == NULL) {
      array_value = new ShellArrayValues ;
   }
   char ** valptr = new char* ;
   if (valptr) {
      *valptr = (char *)value;
      array_value->list.add(valptr);
   }
}

unsigned
ShellArray::count() const
{
   return  ((array_value) ? array_value->list.count() : 0);
}

const char *
ShellArray::operator[](unsigned  index) const
{
   return  ((array_value) ? array_value->array[index] : NULL);
}

//----------------------------------------------------------- AbstractUnixShell

AbstractUnixShell::~AbstractUnixShell()
{
}

//------------------------------------------------------------------- UnixShell

UnixShell::UnixShell(const char * shell_name)
   : shell(NULL), valid(1)
{
   if (::strcmp(BourneShell::NAME, shell_name) == 0) {
      shell = new BourneShell ;
   } else if (::strcmp("ash", shell_name) == 0) {
      shell = new BourneShell ;
   } else if (::strcmp(KornShell::NAME, shell_name) == 0) {
      shell = new KornShell ;
   } else if (::strcmp(BourneAgainShell::NAME, shell_name) == 0) {
      shell = new BourneAgainShell ;
   } else if (::strcmp(CShell::NAME, shell_name) == 0) {
      shell = new CShell ;
   } else if (::strcmp("tcsh", shell_name) == 0) {
      shell = new CShell ;
   } else if (::strcmp("itcsh", shell_name) == 0) {
      shell = new CShell ;
   } else if (::strcmp(ZShell::NAME, shell_name) == 0) {
      shell = new ZShell ;
   } else if (::strcmp(Plan9Shell::NAME, shell_name) == 0) {
      shell = new Plan9Shell ;
   } else if (::strcmp(PerlShell::NAME, shell_name) == 0) {
      shell = new PerlShell ;
   } else if (::strcmp(TclShell::NAME, shell_name) == 0) {
      shell = new TclShell ;
   } else {
      valid = 0;
   }
}

UnixShell::~UnixShell()
{
   delete  shell;
}

const char *
UnixShell::name() const
{
   return  ((shell) ? shell->name() : NULL);
}

void
UnixShell::unset_args(const char * name) const
{
   if (shell)  shell->unset_args(name);
}

int
UnixShell::is_positionals(const char * name) const
{
   return  ((shell) ? shell->is_positionals(name) : 0);
}

void
UnixShell::set(const ShellVariable & variable) const
{
   if (shell)  shell->set(variable);
}

void
UnixShell::set(const ShellArray & array, int variant) const
{
   if (shell)  shell->set(array, variant);
}

//----------------------------------------------------------------- varname

// Remove any "esoteric" portions of a vraible name (such as a leading '$')
//
inline  static  const char *
varname(const char * name, char skip)
{
   return  ((*name == skip) && (*(name + 1))) ? (name + 1): name ;
}

//----------------------------------------------------------------- BourneShell

const char * BourneShell::NAME = "sh" ;

BourneShell::BourneShell()
{
}

BourneShell::~BourneShell()
{
}

const char *
BourneShell::name() const
{
   return  BourneShell::NAME ;
}

void
BourneShell::unset_args(const char *) const
{
   std::cout << "shift $# ;" << std::endl ;
}

int
BourneShell::is_positionals(const char * name) const
{
   name = varname(name, '$');
   return  ((::strcmp(name, "--") == 0) || (::strcmp(name, "-") == 0) ||
            (::strcmp(name, "@") == 0)  || (::strcmp(name, "*") == 0)) ;
}

void
BourneShell::set(const ShellVariable & variable) const
{
   const char * name = varname(variable.name(), '$');
   if (is_positionals(name)) {
      std::cout << "set -- '" ;
   } else {
      std::cout << name << "='" ;
   }
   escape_value(variable.value());
   std::cout << "';" << std::endl ;
}

void
BourneShell::set(const ShellArray & array, int variant) const
{
   int  ndx;
   const char * name = varname(array.name(), '$');

   if (is_positionals(name)) {
      // set -- 'arg1' 'arg2' ...
      std::cout << "set -- ";
      for (ndx = 0 ; ndx < array.count() ; ndx++) {
         if (ndx)  std::cout << ' ' ;
         std::cout << '\'' ;
         escape_value(array[ndx]);
         std::cout << '\'' ;
      }
      std::cout << ';' << std::endl ;
   } else if (variant) {
      // argname_count=N
      // argname1='arg1'
      //   ...
      // argnameN='argN'
      std::cout << name << "_count=" << array.count() << ';' << std::endl ;
      for (ndx = 0 ; ndx < array.count() ; ndx++) {
         std::cout << name << (ndx + 1) << "='";
         escape_value(array[ndx]);
         std::cout << "';" << std::endl ;
      }
   } else {
      // argname='arg1 arg2 ...'
      std::cout << name << "='";
      for (ndx = 0 ; ndx < array.count() ; ndx++) {
         if (ndx)  std::cout << ' ' ;
         escape_value(array[ndx]);
      }
      std::cout << "';" << std::endl ;
   }
}

void
BourneShell::escape_value(const char * value) const
{
   for ( ; *value ; value++) {
      switch (*value) {
      case '\'' :
         std::cout << "'\\''" ;
         break ;

      case '\\' :
      case '\b' :
      case '\r' :
      case '\v' :
      case '\f' :
         std::cout << '\\' ;    // fall thru to default case
      default :
         std::cout << char(*value) ;
      }
   } //for
}

//------------------------------------------------------------------- KornShell

const char * KornShell::NAME = "ksh" ;

KornShell::KornShell()
{
}

KornShell::~KornShell()
{
}

const char *
KornShell::name() const
{
   return  KornShell::NAME ;
}

void
KornShell::unset_args(const char *) const
{
   std::cout << "set -- ;" << std::endl ;
}

void
KornShell::set(const ShellVariable & variable) const
{
   BourneShell::set(variable);
}

void
KornShell::set(const ShellArray & array, int variant) const
{
   const char * name = varname(array.name(), '$');
   if (is_positionals(name)) {
      std::cout << "set -- " ;
   } else {
      std::cout << "set " << (variant ? '+' : '-') << "A " << name << ' ' ;
   }
   for (int  ndx = 0 ; ndx < array.count() ; ndx++) {
      if (ndx)  std::cout << ' ' ;
      std::cout << '\'' ;
      escape_value(array[ndx]);
      std::cout << '\'' ;
   }
   std::cout << ';' << std::endl ;
}

//------------------------------------------------------------ BourneAgainShell

const char * BourneAgainShell::NAME = "bash" ;

BourneAgainShell::BourneAgainShell()
{
}

BourneAgainShell::~BourneAgainShell()
{
}

const char *
BourneAgainShell::name() const
{
   return  BourneAgainShell::NAME ;
}

void
BourneAgainShell::set(const ShellVariable & variable) const
{
   BourneShell::set(variable);
}

void
BourneAgainShell::set(const ShellArray & array, int variant) const
{
   BourneShell::set(array, variant);
}

//---------------------------------------------------------------------- CShell

const char * CShell::NAME = "csh" ;

CShell::CShell()
{
}

CShell::~CShell()
{
}

const char *
CShell::name() const
{
   return  CShell::NAME ;
}

void
CShell::unset_args(const char *) const
{
   std::cout << "set argv=();" << std::endl ;
}

int
CShell::is_positionals(const char * name) const
{
   name = varname(name, '$');
   return  (::strcmp(name, "argv") == 0);
}

void
CShell::set(const ShellVariable & variable) const
{
   const char * name = varname(variable.name(), '$');
   int  posl = is_positionals(name);
   std::cout << "set " << name << '=' ;
   if (posl)  std::cout << '(' ;
   std::cout << '\'' ;
   escape_value(variable.value());
   std::cout << '\'' ;
   if (posl)  std::cout << ')' ;
   std::cout << ';' << std::endl ;;
}

void
CShell::set(const ShellArray & array, int ) const
{
   std::cout << "set " << varname(array.name(), '$') << "=(" ;
   for (int  ndx = 0 ; ndx < array.count() ; ndx++) {
      if (ndx)  std::cout << ' ' ;
      std::cout << '\'' ;
      escape_value(array[ndx]);
      std::cout << '\'' ;
   }
   std::cout << ");" << std::endl ;
}

void
CShell::escape_value(const char * value) const
{
   for ( ; *value ; value++) {
      switch (*value) {
      case '\'' :
         std::cout << "'\\''" ;
         break ;

      case '!' :
      case '\n' :
      case '\b' :
      case '\r' :
      case '\v' :
      case '\f' :
         std::cout << '\\' ;    // fall thru to default case
      default :
         std::cout << char(*value) ;
      }
   } //for
}

//---------------------------------------------------------------------- ZShell

const char * ZShell::NAME = "zsh" ;

ZShell::ZShell()
{
}

ZShell::~ZShell()
{
}

const char *
ZShell::name() const
{
   return  ZShell::NAME ;
}

void
ZShell::unset_args(const char *) const
{
   std::cout << "argv=();" << std::endl ;
}

int
ZShell::is_positionals(const char * name) const
{
   name = varname(name, '$');
   return  ((::strcmp(name, "--") == 0) || (::strcmp(name, "-") == 0) ||
            (::strcmp(name, "@") == 0)  || (::strcmp(name, "*") == 0) ||
            (::strcmp(name, "argv") == 0));
}

void
ZShell::set(const ShellVariable & variable) const
{
   const char * name = varname(variable.name(), '$');
   int  posl = is_positionals(name);
   std::cout << name << '=' ;
   if (posl)  std::cout << '(' ;
   std::cout << '\'' ;
   escape_value(variable.value());
   std::cout << '\'' ;
   if (posl)  std::cout << ')' ;
   std::cout << ';' << std::endl ;;
}

void
ZShell::set(const ShellArray & array, int ) const
{
   std::cout << varname(array.name(), '$') << "=(" ;
   for (int  ndx = 0 ; ndx < array.count() ; ndx++) {
      if (ndx)  std::cout << ' ' ;
      std::cout << '\'' ;
      escape_value(array[ndx]);
      std::cout << '\'' ;
   }
   std::cout << ");" << std::endl ;
}

void
ZShell::escape_value(const char * value) const
{
   for ( ; *value ; value++) {
      switch (*value) {
      case '\'' :
         std::cout << "'\\''" ;
         break ;

      case '!' :
      case '\\' :
      case '\b' :
      case '\r' :
      case '\v' :
      case '\f' :
         std::cout << '\\' ;    // fall thru to default case
      default :
         std::cout << char(*value) ;
      }
   } //for
}

//------------------------------------------------------------------ Plan9Shell

const char * Plan9Shell::NAME = "rc" ;

Plan9Shell::Plan9Shell()
{
}

Plan9Shell::~Plan9Shell()
{
}

const char *
Plan9Shell::name() const
{
   return  Plan9Shell::NAME ;
}

void
Plan9Shell::unset_args(const char *) const
{
   std::cout << "*=();" << std::endl ;
}

int
Plan9Shell::is_positionals(const char * name) const
{
   name = varname(name, '$');
   return  (::strcmp(name, "*") == 0);
}

void
Plan9Shell::set(const ShellVariable & variable) const
{
   const char * name = varname(variable.name(), '$');
   int  posl = is_positionals(name);
   std::cout << name << '=' ;
   if (posl)  std::cout << '(' ;
   std::cout << '\'' ;
   escape_value(variable.value());
   std::cout << '\'' ;
   if (posl)  std::cout << ')' ;
   std::cout << ';' << std::endl ;;
}

void
Plan9Shell::set(const ShellArray & array, int ) const
{
   std::cout << varname(array.name(), '$') << "=(" ;
   for (int  ndx = 0 ; ndx < array.count() ; ndx++) {
      if (ndx)  std::cout << ' ' ;
      std::cout << '\'' ;
      escape_value(array[ndx]);
      std::cout << '\'' ;
   }
   std::cout << ");" << std::endl ;
}

void
Plan9Shell::escape_value(const char * value) const
{
   for ( ; *value ; value++) {
      switch (*value) {
      case '\'' :
         std::cout << "''" ;
         break ;

      case '\\' :
      case '\b' :
      case '\r' :
      case '\v' :
      case '\f' :
         std::cout << '\\' ;    // fall thru to default case
      default :
         std::cout << char(*value) ;
      }
   } //for
}

//------------------------------------------------------------------- PerlShell

const char * PerlShell::NAME = "perl" ;

PerlShell::PerlShell()
{
   static const char perl_true[]  = "1" ;
   static const char perl_false[] = "0" ;

      // use different defaults for TRUE and FALSE
   ShellCmdArgBool::True(perl_true);
   ShellCmdArgBool::False(perl_false);
}

PerlShell::~PerlShell()
{
}

const char *
PerlShell::name() const
{
   return  PerlShell::NAME ;
}

void
PerlShell::unset_args(const char *) const
{
   std::cout << "@ARGV = ();" << std::endl ;
}

int
PerlShell::is_positionals(const char * name) const
{
   name = varname(name, '@');
   return  (::strcmp(name, "ARGV") == 0);
}

void
PerlShell::set(const ShellVariable & variable) const
{
   const char * name = varname(variable.name(), '$');
   int array = (*name == '@') ;
   std::cout << (array ? "" : "$") << name << " = " ;
   if (array)  std::cout << '(' ;
   std::cout << '\'' ;
   escape_value(variable.value());
   std::cout << '\'' ;
   if (array)  std::cout << ')' ;
   std::cout << ';' << std::endl ;;
}

void
PerlShell::set(const ShellArray & array, int ) const
{
   const char * name = varname(array.name(), '@');
   int scalar = (*name == '$') ;
   std::cout << (scalar ? "" : "@") << name << " = " ;
   std::cout << (scalar ? '\'' : '(') ;
   for (int  ndx = 0 ; ndx < array.count() ; ndx++) {
      if (ndx)  std::cout << (scalar ? " " : ", ") ;
      if (! scalar)  std::cout << '\'' ;
      escape_value(array[ndx]);
      if (! scalar)  std::cout << '\'' ;
   }
   std::cout << (scalar ? '\'' : ')') ;
   std::cout << ";" << std::endl ;
}

void
PerlShell::escape_value(const char * value) const
{
   for ( ; *value ; value++) {
      switch (*value) {
      case '\t' :  std::cout << "\\t" ; break ;
      case '\n' :  std::cout << "\\n" ; break ;
      case '\b' :  std::cout << "\\b" ; break ;
      case '\r' :  std::cout << "\\r" ; break ;
      case '\v' :  std::cout << "\\v" ; break ;
      case '\f' :  std::cout << "\\f" ; break ;

      case '\'' :
      case '\\' :
         std::cout << "\\" ;    // fall thru to default
      default :
         std::cout << char(*value) ;
      }
   } //for
}

//------------------------------------------------------------------- TclShell

const char * TclShell::NAME = "tcl" ;

TclShell::TclShell()
{
   static const char tcl_true[]  = "1" ;
   static const char tcl_false[] = "0" ;

      // use different defaults for TRUE and FALSE
   ShellCmdArgBool::True(tcl_true);
   ShellCmdArgBool::False(tcl_false);
}

TclShell::~TclShell()
{
}

const char *
TclShell::name() const
{
   return  TclShell::NAME ;
}

void
TclShell::unset_args(const char * name) const
{
   std::cout << "set " << varname(name, '$') << " {};" << std::endl ;
}

int
TclShell::is_positionals(const char * name) const
{
   name = varname(name, '$');
   return  ((::strcmp(name, "argv") == 0) || (::strcmp(name, "args") == 0));
}

void
TclShell::set(const ShellVariable & variable) const
{
   const char * name = varname(variable.name(), '$');
   std::cout << "set " << name << ' ' ;
   std::cout << '"' ;
   escape_value(variable.value());
   std::cout << '"' ;
   std::cout << ';' << std::endl ;;
}

void
TclShell::set(const ShellArray & array, int ) const
{
   const char * name = varname(array.name(), '@');
   int scalar = (*name == '$') ;
   std::cout << "set " << name << " [ list " ;
   for (int  ndx = 0 ; ndx < array.count() ; ndx++) {
      if (ndx)  std::cout << ' ' ;
      std::cout << '"' ;
      escape_value(array[ndx]);
      std::cout << '"' ;
   }
   std::cout << " ]" ;
   std::cout << ";" << std::endl ;
}

void
TclShell::escape_value(const char * value) const
{
   for ( ; *value ; value++) {
      switch (*value) {
      case '\t' :  std::cout << "\\t" ; break ;
      case '\n' :  std::cout << "\\n" ; break ;
      case '\b' :  std::cout << "\\b" ; break ;
      case '\r' :  std::cout << "\\r" ; break ;
      case '\v' :  std::cout << "\\v" ; break ;
      case '\f' :  std::cout << "\\f" ; break ;

      case '\'' :
      case '\\' :
      case '{' :
      case '}' :
      case '[' :
      case ']' :
      case '$' :
      case ';' :
      case '"' :
         std::cout << "\\" ;    // fall thru to default
      default :
         std::cout << char(*value) ;
      }
   } //for
}
