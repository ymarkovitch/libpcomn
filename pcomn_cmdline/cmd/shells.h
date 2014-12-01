//------------------------------------------------------------------------
// ^FILE: shells.h - define classes for the various Unix shells
//
// ^DESCRIPTION:
//     This file encapsulates all the information that cmdparse(1) needs
//     to know about each of the various shells that it will support.
//
//     To add a new shell to the list of shells supported here:
//        1) Add its class definition in this file.
//
//        2) Implement its member functions in "shells.h"
//           (dont forget the NAME data-member to hold the name).
//
//        3) Add an "else if" statement for the new shell into
//           the virtual constructor UnixShell::UnixShell(const char *).
//
// ^HISTORY:
//    04/19/92	Brad Appleton	<bradapp@enteract.com>	Created
//-^^---------------------------------------------------------------------

#ifndef _shells_h
#define _shells_h

   // A ShellVariable object houses the name and value of a shell
   // environment variable.
   //
class  ShellVariable {
public:
   ShellVariable(const char * name);

   virtual ~ShellVariable();

      // Return the name of this variable
   const char *
   name() const { return  var_name ; }

      // Set the value of this variable
   void
   set(const char * value) { var_value = value; }

      // Return the value of this variable
   const char *
   value() const { return  var_value; }

protected:
   const char * var_name ;
   const char * var_value ;
};


   // A ShellArray object houses the name and values of a shell array.
   //
struct  ShellArrayValues;
class  ShellArray {
public:
   ShellArray(const char * name);

   virtual ~ShellArray();

      // Return the name of this array
   const char *
   name() const { return  array_name; }

      // Append to the list of values in this array
   void
   append(const char * value);

      // Return the number of items in this array.
   unsigned
   count() const;

      // Return the desired element of an array
      //
      // NOTE: the elements range in index from 0 .. count-1,
      //       an out-of-range index will result in a run-time
      //       NULL-ptr dereferencing error!
      //
   const char *
   operator[](unsigned  index) const;

protected:
   const char       * array_name ;
   ShellArrayValues * array_value ;
} ;


   // AbstractUnixShell is an abstract class for an arbitrary Unix shell
   // program.  It represents all the functionality that cmdparse(1)
   // requires of a command-interpreter.
   //
class  AbstractUnixShell {
public:
   virtual
   ~AbstractUnixShell();

      // Return the name of this shell
   virtual  const char *
   name() const = 0;

      // Does "name" correspond to the positional-parameters for this shell?
   virtual  int
   is_positionals(const char * name) const = 0;

      // Unset the positional parameters of this shell.
      //
      // The parameter "name" is the name of a shell variable
      // for which is_positionals() returns TRUE.
      //
   virtual  void
   unset_args(const char * name) const = 0;

      // Set the given variable name to the given value
   virtual  void
   set(const ShellVariable & variable) const = 0;

      // Set the given array name to the given values.
      // Some shells have more than one way to set an array.
      // Such shells should label these varying methods as
      // variant0 .. variantN, the desired variant method to use
      // (which defaults to zero), should be indicated by the
      // last parameter.
      //
      // This member function is responsible for checking to see
      // if the array name corresponds to the positional-parameters
      // (and for behaving accordingly if this is the case).
      //
   virtual  void
   set(const ShellArray & array, int variant) const = 0;

protected:
   AbstractUnixShell() {};

} ;


   // UnixShell is used as an envelope class (using its siblings as
   // letter classes). It is a "shell" that does not decide what
   // type of shell it is until runtime.
   //
class  UnixShell {
public:
      // This is a virtual constructor that constructs a Unix shell object
      // that is the appropriate derived class of AbstractUnixShell.
      //
   UnixShell(const char * shell_name);

   virtual
   ~UnixShell();

      // See if this shell is valid
   int
   is_valid() const { return  (valid) ? 1 : 0; }

      // Return the name of this shell
   virtual  const char *
   name() const;

   virtual  void
   unset_args(const char * name) const;

   virtual  int
   is_positionals(const char * name) const;

   virtual  void
   set(const ShellVariable & variable) const;

   virtual  void
   set(const ShellArray & array, int variant) const;

private:
   unsigned    valid : 1 ;
   AbstractUnixShell * shell;

} ;


   // BourneShell (sh) - the most common of the Unix Shells - implemented
   //                    by Stephen R. Bourne
   //
   // Variables are set using:
   //      name='value';
   //
   // Arrays (by default) are set using:
   //      name='value1 value2 value3 ...';
   //
   // but if requested, the following array-variant will be used instead:
   //      name_count=N;
   //      name1='value1';
   //      name2='value2';
   //          ...
   //      nameN='valueN';
   //
   // If a variable name matches one of "@", "*", "-", or "--", then the
   // variable is assumed to refer to the positional-parameters of the
   // shell-script and the following syntax will be used:
   //      set -- 'value1' 'value2' 'value3' ...
   //
class  BourneShell : public  AbstractUnixShell {
public:
   BourneShell();

   virtual  ~BourneShell();

   virtual  const char *
   name() const;

   virtual  void
   unset_args(const char * name) const;

   virtual  int
   is_positionals(const char * name) const;

   virtual  void
   set(const ShellVariable & variable) const;

   virtual  void
   set(const ShellArray & array, int variant) const;

   static const char * NAME ;

protected:
   void
   escape_value(const char * value) const;

private:

} ;


   // KornShell (ksh) -- David G. Korn's reimplementation of the Bourne shell
   //
   // Variables are set using the same syntax as in the Bourne Shell.
   //
   // Arrays (by default) are set using:
   //      set -A name 'value1' 'value2' 'value3' ...;
   //
   // but if requested, the following array-variant will be used instead:
   //      set +A name 'value1' 'value2' 'value3' ...;
   //
   // If a variable name matches one of "@", "*", "-", or "--", then the
   // variable is assumed to refer to the positional-parameters of the
   // shell-script and the following syntax will be used:
   //      set -- 'value1' 'value2' 'value3' ...
   //
class  KornShell : public BourneShell {
public:
   KornShell();

   virtual  ~KornShell();

   virtual  const char *
   name() const;

   virtual  void
   unset_args(const char * name) const;

   virtual  void
   set(const ShellVariable & variable) const;

   virtual  void
   set(const ShellArray & array, int variant) const;

   static const char * NAME ;

protected:

private:

} ;


   // BourneAgainShell (bash) -- The Free Software Foundation's answer to ksh
   //
   // bash is treated exactlt like the Bourne Shell, this will change when
   // bash supports arrays.
   //
class  BourneAgainShell : public BourneShell {
public:
   BourneAgainShell();

   virtual  ~BourneAgainShell();

   virtual  const char *
   name() const;

   virtual  void
   set(const ShellVariable & variable) const;

   virtual  void
   set(const ShellArray & array, int variant) const;

   static const char * NAME ;

protected:

private:

} ;


   // CShell (csh) -- Bill Joy's rewrite of "sh" with C like syntax.
   //                 this will work for tcsh and itcsh as well.
   //
   // Variables are set using:
   //      set name='value';
   //
   // Arrays (by default) are set using:
   //      set name=('value1' 'value2' 'value3' ...);
   //
class  CShell : public AbstractUnixShell {
public:
   CShell();

   virtual  ~CShell();

   virtual  const char *
   name() const;

   virtual  void
   unset_args(const char * name) const;

   virtual  int
   is_positionals(const char * name) const;

   virtual  void
   set(const ShellVariable & variable) const;

   virtual  void
   set(const ShellArray & array, int variant) const;

   static const char * NAME ;

protected:
   void
   escape_value(const char * value) const;

private:

} ;


   // ZShell (zsh) -- Paul Falstad's shell combining lots of stuff from
   //                 csh and ksh and some stuff of his own.
   //
   // Variables are set using:
   //    name='value';
   //
   // Arrays are set using:
   //    name=('value1' 'value2' 'value3' ...);
   //
   // If a variable name matches one of "@", "*", "-", "--", or "argv" then
   // the variable is assumed to refer to the positional-parameters of the
   // shell-script and the following syntax will be used:
   //    argv=('value1' 'value2' 'value3' ...);
   //
class  ZShell : public AbstractUnixShell {
public:
   ZShell();

   virtual  ~ZShell();

   virtual  const char *
   name() const;

   virtual  void
   unset_args(const char * name) const;

   virtual  int
   is_positionals(const char * name) const;

   virtual  void
   set(const ShellVariable & variable) const;

   virtual  void
   set(const ShellArray & array, int variant) const;

   static const char * NAME ;

protected:
   void
   escape_value(const char * value) const;

private:

} ;


   // Plan9Shell (rc) -- Tom Duff's shell from the Plan 9 papers.
   //        A public domain version (with some enhancements) was
   //        written by Byron Rakitzis.
   //
   // Variables are set using:
   //    name='value';
   //
   // Arrays are set using:
   //    name=('value1' 'value2' 'value3' ...);
   //
class  Plan9Shell : public AbstractUnixShell {
public:
   Plan9Shell();

   virtual  ~Plan9Shell();

   virtual  const char *
   name() const;

   virtual  void
   unset_args(const char * name) const;

   virtual  int
   is_positionals(const char * name) const;

   virtual  void
   set(const ShellVariable & variable) const;

   virtual  void
   set(const ShellArray & array, int variant) const;

   static const char * NAME ;

protected:
   void
   escape_value(const char * value) const;

private:

} ;


   // Perl (perl) -- Larry Wall's Practical Extraction and Report Generation
   //                utility.
   //
   // Variables are set using:
   //    $name = 'value';
   //
   // Arrays are set using:
   //    @name = ('value1', 'value2', 'value3', ...);
   //
class  PerlShell : public AbstractUnixShell {
public:
   PerlShell();

   virtual  ~PerlShell();

   virtual  const char *
   name() const;

   virtual  void
   unset_args(const char * name) const;

   virtual  int
   is_positionals(const char * name) const;

   virtual  void
   set(const ShellVariable & variable) const;

   virtual  void
   set(const ShellArray & array, int variant) const;

   static const char * NAME ;

protected:
   void
   escape_value(const char * value) const;

private:

} ;


   // Tcl -- Karl Lehenbauer and friends implementation of a shell based
   //        on John K. Ousterhout's Tool Command Language
   //
   // Variables are set using:
   //    set name "value";
   //
   // Arrays are set using:
   //     set name [list "value1" "value2" "value3" ...];
   //
class  TclShell : public AbstractUnixShell {
public:
   TclShell();

   virtual  ~TclShell();

   virtual  const char *
   name() const;

   virtual  void
   unset_args(const char * name) const;

   virtual  int
   is_positionals(const char * name) const;

   virtual  void
   set(const ShellVariable & variable) const;

   virtual  void
   set(const ShellArray & array, int variant) const;

   static const char * NAME ;

protected:
   void
   escape_value(const char * value) const;

private:

} ;


#endif  /* _shells_h */
