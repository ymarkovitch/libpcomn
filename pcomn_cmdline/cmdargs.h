//------------------------------------------------------------------------
// ^FILE: cmdargs.h - define the most commonly used argument types
//
// ^DESCRIPTION:
//    This file defines classes for the most commonly used types of
//    command-line arguments.  Most command-line arguments are either
//    boolean-flags, a number, a character, or a string (or a list of
//    numbers or strings).  In each of these cases, the call operator
//    (operator()) of the argument just compiles the value given into
//    some internal value and waits for the programmer to query the
//    value at some later time.
//
//    I call these types of arguments "ArgCompilers". For each of the
//    most common argument types, a corresponding abstract ArgCompiler
//    class is declared.  All that this class does is to add a member
//    function named "compile" to the class.  The "compile()" function
//    looks exactly like the call operator but it takes an additional
//    parameter: a reference to the value to be modified by compiling
//    the argument value.  In all other respects, the "compile()" member
//    function behaves exactly like the call operator.  In fact, most
//    of the call-operator member functions simply call the ArgCompiler's
//    "compile()" member function with the appropriate value and return
//    whatever the compile function returned.
//
//    Once all of these ArgCompilers are declared, it is a simple matter
//    to declare a class that holds a single item, or a list of items,
//    by deriving it from the corresponding ArgCompiler type.
//
//    For derived classes of these ArgCompilers that hold a single item,
//    The derived class implements some operators (such as operator=
//    and an appropriate cast operator) to treat the argument as if it
//    were simply an item (instead of an argument that contains an item).
//    The output operator (std::ostream & operator<<) is also defined.
//
//    For derived classes of ArgCompilers that hold a list of items,
//    the subscript operator[] is defined in order to treat the argument
//    as if it were simply an array of items and not an argument that
//    contains a list of items.
//
//    *NOTE*
//    ======
//    It is important to remember that each subclass of CmdArg MUST be able
//    to handle NULL as the first argument to the call-operator (and it
//    should NOT be considered an error).  This is because NULL will be
//    passed if the argument takes no value, or if it takes an optional
//    value that was NOT provided on the command-line.  Whenever an
//    argument is correctly specified on the command-line, its call
//    operator is always invoked (regardless of whether or not there
//    is a corresponding value from the command-line).
//
// ^HISTORY:
//    03/25/92	Brad Appleton	<bradapp@enteract.com>	Created
//-^^---------------------------------------------------------------------

#ifndef _usr_include_cmdargs_h
#define _usr_include_cmdargs_h

#include "cmdline.h"

//-------------------------------------------------------------- Dummy Argument

   // A Dummy argument is one that is used only for its appearance in
   // usage messages. It is completely ignored by the CmdLine object
   // when parsing the command-line.
   //
   // Examples:
   //     CmdArgDummy  dummy1('c', "keyword", "value", "dummy argument # 1");
   //     CmdArgDummy  dummy2("value", "dummy argument # 2");
   //
class CmdArgDummy : public CmdArg {
public:
   CmdArgDummy(char         optchar,
               const char * keyword,
               const char * value,
               const char * description,
               unsigned     syntax_flags = isVALREQ)
      : CmdArg(optchar, keyword, value, description, syntax_flags|isIGNORED) {}

   CmdArgDummy(char         optchar,
               const char * keyword,
               const char * description,
               unsigned     syntax_flags = 0)
      : CmdArg(optchar, keyword, description, syntax_flags|isIGNORED) {}

   CmdArgDummy(const char * value,
               const char * description,
               unsigned     syntax_flags = isPOSVALREQ)
      : CmdArg(value, description, syntax_flags|isIGNORED) {}

   CmdArgDummy(const CmdArgDummy & cp) : CmdArg(cp) {}

   CmdArgDummy(const CmdArg & cp) : CmdArg(cp) {}

   bool is_dummy() const ;   // return non-zero

   int operator()(const char * & arg, CmdLine & cmd);  // NO-OP
} ;

//-------------------------------------------------------------- Usage Argument

   // The sole purpose of a usage argument is to immediately print the
   // program usage (as soon as it is matched) and to exit.
   //
   // There is a default usage argument in every CmdLine object.
   //
   // Example:
   //    CmdArgUsage  usage_arg('?', "help", "print usage and exit");
   //
class  CmdArgUsage : public CmdArg {
public:
   CmdArgUsage(char         optchar,
               const char * keyword,
               const char * description,
               std::ostream    * osp =NULL);   // std::cout is used if "osp" is NULL

   int operator()(const char * & arg, CmdLine & cmd);

      // get/set the std::ostream that usage is printed on
   std::ostream *
   ostream_ptr() const { return  os_ptr; }

   void
   ostream_ptr(std::ostream * osp);

private:
   std::ostream * os_ptr;
} ;

//----------------------------------------------------------- Integer Arguments

   // Look under "List Arguments" for a CmdArg that is a list of ints

   // CmdArgIntCompiler is the base class for all arguments that need to
   // convert the string given on the command-line into an integer.
   //
class  CmdArgIntCompiler : public CmdArg {
public:
   CmdArgIntCompiler(char         optchar,
                     const char * keyword,
                     const char * value,
                     const char * description,
                     unsigned     syntax_flags = isVALREQ)
      : CmdArg(optchar, keyword, value, description, syntax_flags) {}

   CmdArgIntCompiler(const char * value,
                     const char * description,
                     unsigned     syntax_flags = isPOSVALREQ)
      : CmdArg(value, description, syntax_flags) {}

   int compile(const char * & arg, CmdLine & cmd, int & value);
} ;


   // CmdArgInt - an argument that contains a single integer.
   //
   // The following member functions are provided to treat
   // a CmdArgInt as if it were an integer:
   //
   //    operator=(int);
   //    operator int();
   //    operator<<(os, CmdArgInt);
   //
   // The integer value is initialized to zero by the constructor.
   //
   // Examples:
   //     CmdArgInt  count('c', "count", "number", "# of copies to print");
   //     CmdArgInt  nlines("lines", "number of lines to print);
   //
   //     count = 1;
   //     nlines = 0;
   //
   //     if (count > 1) { ... }
   //     std::cout << "number of lines is " << nlines << std::endl ;
   //
class  CmdArgInt : public CmdArgIntCompiler {
public:
   CmdArgInt(char         optchar,
             const char * keyword,
             const char * value,
             const char * description,
             unsigned     syntax_flags = isVALREQ)
      : CmdArgIntCompiler(optchar, keyword, value, description, syntax_flags),
        val(0) {}

   CmdArgInt(const char * value,
             const char * description,
             unsigned     syntax_flags = isPOSVALREQ)
      : CmdArgIntCompiler(value, description, syntax_flags), val(0) {}

   int operator()(const char * & arg, CmdLine & cmd);

   CmdArgInt &
   operator=(const CmdArgInt & cp)  { val = cp.val; return  *this; }

   CmdArgInt &
   operator=(int cp)  { val = cp; return  *this; }

   operator int()  const { return  val; }

private:
   int   val;
} ;

std::ostream &
operator<<(std::ostream & os, const CmdArgInt & int_arg);

//---------------------------------------------------- Floating-point Arguments

   // Look under "List Arguments" for a CmdArg that is a list of floats

   // CmdArgFloatCompiler is the base class for all arguments
   // need to convert the string given on the command-line into
   // a floating-point value.
   //
class  CmdArgFloatCompiler : public CmdArg {
public:
   CmdArgFloatCompiler(char         optchar,
                       const char * keyword,
                       const char * value,
                       const char * description,
                       unsigned     syntax_flags = isVALREQ)
      : CmdArg(optchar, keyword, value, description, syntax_flags) {}

   CmdArgFloatCompiler(const char * value,
                       const char * description,
                       unsigned     syntax_flags = isPOSVALREQ)
      : CmdArg(value, description, syntax_flags) {}

   int compile(const char * & arg, CmdLine & cmd, float & value);
} ;


   // CmdArgFloat - an argument that contains a single floating-point value.
   //
   // The following member functions are provided to treat
   // a CmdArgFloat as if it were a float:
   //
   //    operator=(float);
   //    operator float();
   //    operator<<(os, CmdArgFloat);
   //
   // The floating-point value is initialized to zero by the constructor.
   //
   // Examples:
   //     CmdArgFloat  major('m', "major", "#", "major radius of ellipse");
   //     CmdArgFloat  minor("minor", "minor radius of ellipse");
   //
   //     major = 2.71828;
   //     minor = 3.14159;
   //
   //     if (minor > major)  {  /* swap major and minor */ }
   //
   //     std::cout << "major radius is " << major << std::endl ;
   //
class  CmdArgFloat : public CmdArgFloatCompiler {
public:
   CmdArgFloat(char         optchar,
               const char * keyword,
               const char * value,
               const char * description,
               unsigned     syntax_flags = isVALREQ)
      : CmdArgFloatCompiler(optchar, keyword, value, description, syntax_flags),
        val(0) {}

   CmdArgFloat(const char * value,
               const char * description,
               unsigned     syntax_flags = isPOSVALREQ)
      : CmdArgFloatCompiler(value, description, syntax_flags), val(0) {}


   int operator()(const char * & arg, CmdLine & cmd);

   CmdArgFloat &
   operator=(const CmdArgFloat & cp)  { val = cp.val; return  *this; }

   CmdArgFloat &
   operator=(float cp)  { val = cp; return  *this; }

   operator float()  const  { return  val; }

private:
   float   val;
} ;

std::ostream &
operator<<(std::ostream & os, const CmdArgFloat & float_arg);


//--------------------------------------------------------- Character Arguments

   // CmdArgCharCompiler is the base class for all arguments need to
   // convert the string given on the command-line into a character.
   //
class  CmdArgCharCompiler : public CmdArg {
public:
   CmdArgCharCompiler(char         optchar,
                      const char * keyword,
                      const char * value,
                      const char * description,
                      unsigned     syntax_flags = isVALREQ)
      : CmdArg(optchar, keyword, value, description, syntax_flags) {}

   CmdArgCharCompiler(const char * value,
                      const char * description,
                      unsigned     syntax_flags = isPOSVALREQ)
      : CmdArg(value, description, syntax_flags) {}

   int
   compile(const char * & arg, CmdLine & cmd, char & value);
} ;


   // CmdArgChar - an argument that contains a single character.
   //
   // The following member functions are provided to treat
   // a CmdArgChar as if it were a character:
   //
   //    operator=(char);
   //    operator char();
   //    operator<<(os, CmdArgChar);
   //
   // The character value is initialized to '\0' by the constructor.
   //
   // Examples:
   //     CmdArgChar  ignore('i', "ignore", "character to ignore);
   //     CmdArgChar  sep("field-separator");
   //
   //     ignore = ' ';
   //     sep = ',';
   //
   //     if (sep == '\0') { /* error */ }
   //
   //     std::cout << "ignore character is '" << ignore << "'" << std::endl ;
   //
class  CmdArgChar : public CmdArgCharCompiler {
public:
   CmdArgChar(char         optchar,
              const char * keyword,
              const char * value,
              const char * description,
              unsigned     syntax_flags = isVALREQ)
      : CmdArgCharCompiler(optchar, keyword, value, description, syntax_flags),
        val(0) {}

   CmdArgChar(const char * value,
              const char * description,
              unsigned     syntax_flags = isPOSVALREQ)
      : CmdArgCharCompiler(value, description, syntax_flags), val(0) {}

   int operator()(const char * & arg, CmdLine & cmd);

   CmdArgChar &
   operator=(const CmdArgChar & cp)  { val = cp.val; return  *this; }

   CmdArgChar &
   operator=(char cp)  { val = cp; return  *this; }

   operator char()  const  { return  val; }

private:
   char   val;
} ;

std::ostream &
operator<<(std::ostream & os, const CmdArgChar & char_arg);

//------------------------------------------------------------ String Arguments

   // Look under "List Arguments" for a CmdArg that is a list of strings

   // CmdArgIntCompiler is the base class for all arguments need to
   // convert the string given on the command-line into a string.
   //
class  CmdArgStrCompiler : public CmdArg {
public:
   // We need an internal string type here because sometimes we need
   // to allocate new space for the string, and sometimes we dont.
   // We need a string type that knows how it was allocated and
   // to behave accordingly.
   //
   // Since the programmer, will be seeing our string type instead of
   // a "char *" we need to provide some operators for our string
   // type that make it unnecessary to know the difference between
   // it and a "char *" (in most cases).
   //
   struct  casc_string {
      const char * str ;
      bool         is_alloc ;

      casc_string() : str(), is_alloc(false) {}

      casc_string(const char *s) : str(s), is_alloc(false) {}

      casc_string(bool source_is_temporary, const char *source)
         : str(), is_alloc() { copy(source_is_temporary, source); }

      casc_string(const casc_string &cp)
         : str(), is_alloc() { copy(cp.is_alloc, cp.str); }

      ~casc_string() { if (is_alloc)  delete [] (char *)str; }

      void
      copy(bool source_is_temporary, const char *source);

      casc_string &
      operator=(const casc_string & cp)
         { copy(cp.is_alloc, cp.str); return *this; }

      casc_string &
      operator=(const char * cp) { copy(false, cp); return *this; }

      operator const char*()  const { return  str; }

   } ;

   CmdArgStrCompiler(char         optchar,
                     const char * keyword,
                     const char * value,
                     const char * description,
                     unsigned     syntax_flags = isVALREQ)
      : CmdArg(optchar, keyword, value, description, syntax_flags) {}

   CmdArgStrCompiler(const char * value,
                     const char * description,
                     unsigned     syntax_flags = isPOSVALREQ)
      : CmdArg(value, description, syntax_flags) {}

   int
   compile(const char * & arg, CmdLine & cmd, casc_string & value) ;
} ;


   // CmdArgStr - an argument that holds a single string
   //
   // The following member functions are provided to treat
   // a CmdArgStr as if it were a string:
   //
   //    operator=(char*);
   //    operator char*();
   //    operator<<(os, CmdArgStr);
   //
   // The string value is initialized to NULL by the constructor.
   //
   // Examples:
   //     CmdArgStr  input('i', "input", "filename", "file to read");
   //     CmdArgStr  output("output-file", "file to write);
   //
   //     input = "/usr/input" ;
   //     output = "/usr/output" ;
   //
   //     if (strcmp(input, output) == 0) {
   //        std::cerr << "input and output are the same file: " << input << std::endl ;
   //     }
   //
class  CmdArgStr : public CmdArgStrCompiler {
public:
   CmdArgStr(char         optchar,
             const char * keyword,
             const char * value,
             const char * description,
             unsigned     syntax_flags = isVALREQ)
      : CmdArgStrCompiler(optchar, keyword, value, description, syntax_flags),
        val(0) {}

   CmdArgStr(const char * value,
             const char * description,
             unsigned     syntax_flags = isPOSVALREQ)
      : CmdArgStrCompiler(value, description, syntax_flags), val(0) {}


   int operator()(const char * & arg, CmdLine & cmd);

   CmdArgStr &
   operator=(const CmdArgStr & cp)  { val = cp.val; return  *this; }

   CmdArgStr &
   operator=(const CmdArgStrCompiler::casc_string & cp)
      { val = cp; return  *this; }

   CmdArgStr &
   operator=(const char * cp)  { val = cp; return  *this; }

   operator CmdArgStrCompiler::casc_string()  { return  val; }

   operator const char*()  const { return  val.str; }

   // use this for comparing to NULL
   bool isNULL() const { return !val.str ; }

private:
   CmdArgStrCompiler::casc_string  val;
} ;

std::ostream &
operator<<(std::ostream & os, const CmdArgStrCompiler::casc_string & str);

std::ostream &
operator<<(std::ostream & os, const CmdArgStr & str_arg);

//-------------------------------------------------------------- List Arguments

   // For each of the list argument types:
   //    The list is initially empty. The only way to add to the list
   //    is with operator(). The number of items in the list may
   //    be obtained by the "count()" member function and a given
   //    item may be obtained by treating the list as an array and
   //    using operator[] to index into the list.
   //


   // CmdArgIntList - an argument that holds a list of integers
   //
   // Example:
   //     CmdArgIntList ints('i', "ints", "numbers ...", "a list of integers");
   //     CmdArgIntList ints("numbers ...", "a positional list of integers");
   //
   //     for (int i = 0 ; i < ints.count() ; i++) {
   //        std::cout << "integer #" << i << " is " << ints[i] << std::endl ;
   //     }
   //
struct CmdArgIntListPrivate;
class  CmdArgIntList : public CmdArgIntCompiler {
public:
   CmdArgIntList(char    optchar,
            const char * keyword,
            const char * value,
            const char * description,
            unsigned   syntax_flags = isVALREQ | isLIST)
      : CmdArgIntCompiler(optchar, keyword, value, description, syntax_flags),
        val(0) {}

   CmdArgIntList(const char * value,
            const char * description,
            unsigned   syntax_flags = isPOSVALREQ | isLIST)
      : CmdArgIntCompiler(value, description, syntax_flags), val(0) {}

   ~CmdArgIntList();

   int operator()(const char * & arg, CmdLine & cmd);

   unsigned
   count() const;

   int &
   operator[](unsigned  index);

private:
   CmdArgIntList(const CmdArgInt & cp);

   CmdArgIntList &
   operator=(const CmdArgInt & cp);

   CmdArgIntListPrivate * val;
} ;


   // CmdArgFloatList - an argument that holds a list of floats
   //
   // Example:
   //     CmdArgFloatList flts('f', "flts", "numbers ...", "a list of floats");
   //     CmdArgFloatList flts("numbers ...", "a positional list of floats");
   //
   //     for (int i = 0 ; i < flts.count() ; i++) {
   //        std::cout << "Float #" << i << " is " << flts[i] << std::endl ;
   //     }
   //
struct CmdArgFloatListPrivate;
class  CmdArgFloatList : public CmdArgFloatCompiler {
public:
   CmdArgFloatList(char    optchar,
              const char * keyword,
              const char * value,
              const char * description,
              unsigned   syntax_flags = isVALREQ | isLIST)
      : CmdArgFloatCompiler(optchar, keyword, value, description, syntax_flags),
        val(0) {}

   CmdArgFloatList(const char * value,
              const char * description,
              unsigned   syntax_flags = isPOSVALREQ | isLIST)
      : CmdArgFloatCompiler(value, description, syntax_flags), val(0) {}

   ~CmdArgFloatList();

   int
   operator()(const char * & arg, CmdLine & cmd);

   unsigned
   count() const;

   float &
   operator[](unsigned  index);

private:
   CmdArgFloatList(const CmdArgFloatList &);
   void operator=(const CmdArgFloatList &);

   CmdArgFloatListPrivate * val;
} ;


   // CmdArgStrList - an argument that holds a list of strings
   //
   // Example:
   //     CmdArgStrList strs('s', "strs", "strings ...", "a list of strings");
   //     CmdArgStrList strs("strings ...", "a positional list of strings");
   //
   //     for (int i = 0 ; i < strs.count() ; i++) {
   //        std::cout << "String #" << i << " is " << strs[i] << std::endl ;
   //     }
   //
struct CmdArgStrListPrivate;
class  CmdArgStrList : public CmdArgStrCompiler {
public:
   CmdArgStrList(char    optchar,
            const char * keyword,
            const char * value,
            const char * description,
            unsigned   syntax_flags = isVALREQ | isLIST)
      : CmdArgStrCompiler(optchar, keyword, value, description, syntax_flags),
        val(0) {}

   CmdArgStrList(const char * value,
            const char * description,
            unsigned   syntax_flags = isPOSVALREQ | isLIST)
      : CmdArgStrCompiler(value, description, syntax_flags), val(0) {}

   virtual ~CmdArgStrList();

   int operator()(const char * & arg, CmdLine & cmd);

   unsigned
   count() const;

   CmdArgStrCompiler::casc_string &
   operator[](unsigned  index);

private:
   CmdArgStrList(const CmdArgStrList &);
   void operator=(const CmdArgStrList &);

   CmdArgStrListPrivate * val;
} ;

//----------------------------------------------------------- Boolean Arguments

   // Boolean arguments are a bit tricky, first of all - we have three
   // different kinds:
   //
   //    1) An argument whose presence SETS a value
   //
   //    2) An argument whose presence UNSETS a value
   //
   //    3) An argument whose presence TOGGLES a value
   //
   // Furthermore, it is not uncommon (especially in VAX/VMS) to have
   // one argument that SETS and value, and another argument that
   // UNSETS the SAME value.
   //

   // CmdArgBoolCompiler is a special type of ArgCompiler, not only does
   // its "compile" member function take a reference to the boolean value,
   // but it also needs to know what default-value to use if no explicit
   // value (such as '0', '1', "ON" or "FALSE") was given.
   //
class CmdArgBoolCompiler : public CmdArg {
public:
   CmdArgBoolCompiler(char         optchar,
                      const char * keyword,
                      const char * description,
                      unsigned     syntax_flags = 0)
      : CmdArg(optchar, keyword, description, syntax_flags) {}

   int
   compile(const char * & arg,
           CmdLine      & cmd,
           bool         & value,
           bool         default_value = true);
} ;


   // CmdArgBool is a boolean ArgCompiler that holds a single
   // boolean value, it has three subclasses:
   //
   //   1) CmdArgSet (which is just an alias for CmdArgBool)
   //      -- This argument SETS a boolean value.
   //         The initial value is 0 (OFF).
   //
   //   2) CmdArgClear
   //      -- This argument CLEARS a boolean value
   //         The initial value is 1 (ON).
   //
   //   3) CmdArgToggle
   //      -- This argument TOGGLES a boolean value
   //         The initial value is 0 (OFF).
   //
   // All of these classes have the following member functions
   // to help make it easier to treat a Boolean Argument as
   // a Boolean Value:
   //
   //   operator=(int);
   //   operator int();
   //
   // Examples:
   //    CmdArgBool    xflag('x', "xmode", "turn on xmode);
   //    CmdArgClear   yflag('y', "ymode", "turn on ymode);
   //    CmdArgToggle  zflag('z', "zmode", "turn on zmode);
   //
   //    std::cout << "xmode is " << (xflag ? "ON" : "OFF") << std::endl ;
   //
class CmdArgBool : public CmdArgBoolCompiler {
public:
   CmdArgBool(char         optchar,
              const char * keyword,
              const char * description,
              unsigned     syntax_flags = 0)
      : CmdArgBoolCompiler(optchar, keyword, description, syntax_flags),
        val(false) {}


   CmdArgBool &
   operator=(const CmdArgBool & cp) { val = cp.val; return  *this; }

   CmdArgBool &
   operator=(bool new_value) { val = new_value ; return *this; }

   operator bool() const { return  val; }

   int operator()(const char * & arg, CmdLine & cmd);

protected:
   bool val ;
} ;

std::ostream &
operator<<(std::ostream & os, const CmdArgBool & bool_arg);

typedef  CmdArgBool  CmdArgSet ;

class CmdArgClear : public CmdArgBool {
public:
   CmdArgClear(char         optchar,
               const char * keyword,
               const char * description,
               unsigned     syntax_flags = 0)
      : CmdArgBool(optchar, keyword, description, syntax_flags) { val = true; }

   CmdArgClear &
   operator=(const CmdArgBool &cp) { CmdArgBool::operator=(cp) ; return  *this ; }

   CmdArgClear &
   operator=(bool new_value) { CmdArgBool::operator=(new_value); return *this; }

   int operator()(const char * & arg, CmdLine & cmd);
} ;

class CmdArgToggle : public CmdArgBool {
public:
   CmdArgToggle(char         optchar,
                const char * keyword,
                const char * description,
                unsigned     syntax_flags = 0)
      : CmdArgBool(optchar, keyword, description, syntax_flags) {}

   CmdArgToggle &
   operator=(const CmdArgBool &cp) { CmdArgBool::operator=(cp) ; return  *this ; }

   CmdArgToggle &
   operator=(bool new_value) { CmdArgToggle::operator=(new_value); return *this; }

   int operator()(const char * & arg, CmdLine & cmd);
} ;


   // Now we come to the Reference Boolean arguments, these are boolean
   // arguments that reference the very same value as some other boolean
   // argument. The constructors for Reference Boolean arguments require
   // a reference to the boolean argument whose value they are referencing.
   //
   // The boolean reference classes are as follows:
   //
   //   1) CmdArgBoolRef and CmdArgSetRef
   //      -- SET the boolean value referenced by a CmdArgBool
   //
   //   2) CmdArgClearRef
   //      -- CLEAR the boolean value referenced by a CmdArgBool
   //
   //   3) CmdArgToggleRef
   //      -- TOGGLE the boolean value referenced by a CmdArgBool
   //
   // Examples:
   //    CmdArgBool    xflag('x', "xmode", "turn on xmode");
   //    CmdArgClear   yflag('Y', "noymode", "turn off ymode");
   //    CmdArgToggle  zflag('z', "zmode", "toggle zmode");
   //
   //    CmdArgClearRef x_off(xflag, 'X', "noxmode", "turn off xmode");
   //    CmdArgBoolRef  y_on(yflag, 'Y', "ymode", "turn on ymode");
   //
   //    std::cout << "xmode is " << (xflag ? "ON" : "OFF") << std::endl ;
   //
class CmdArgBoolRef : public CmdArg {
public:
   CmdArgBoolRef(CmdArgBool & bool_arg,
                 char         optchar,
                 const char * keyword,
                 const char * description,
                 unsigned     syntax_flags = 0)
      : CmdArg(optchar, keyword, description, syntax_flags), ref(bool_arg) {}

   int operator()(const char * & arg, CmdLine & cmd);

protected:
   CmdArgBool & ref;
} ;

typedef CmdArgBoolRef  CmdArgSetRef ;

class CmdArgClearRef : public CmdArg {
public:
   CmdArgClearRef(CmdArgBool & bool_arg,
                  char         optchar,
                  const char * keyword,
                  const char * description,
                  unsigned     syntax_flags = 0)
      : CmdArg(optchar, keyword, description, syntax_flags), ref(bool_arg) {}

   int operator()(const char * & arg, CmdLine & cmd);

protected:
   CmdArgBool & ref;
} ;

class CmdArgToggleRef : public CmdArg {
public:
   CmdArgToggleRef(CmdArgBool & bool_arg,
                  char         optchar,
                  const char * keyword,
                  const char * description,
                  unsigned     syntax_flags = 0)
      : CmdArg(optchar, keyword, description, syntax_flags), ref(bool_arg) {}

   int operator()(const char * & arg, CmdLine & cmd);

protected:
   CmdArgBool & ref;
} ;

#endif /* _usr_include_cmdargs_h */
