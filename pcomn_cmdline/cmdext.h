/*-*- mode: c++; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)) -*-*/
#ifndef __CMDLINE_CMDEXT_H
#define __CMDLINE_CMDEXT_H
/*******************************************************************************
 FILE         :   cmdext.h
 COPYRIGHT    :   Yakov Markovitch, 2009-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Extensions for Brad Aplleton's CmdLine library

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   24 Feb 2009
*******************************************************************************/
/** @file
 Template extensions for Brad Aplleton's CmdLine library

 Argument objects may describe both command-line options (i.e. an arguments starting
 with "-" or "--", like "-f filename") and "non-options", i.e. positional arrguments
 following the options.

 The description of an argument object consists of 7 parameters, all or part of them,
 depending on actual argument object type, should be passed to the constructor or
 constructor macro. Below is the list of such parameters:

  -# type:        argument type (unsigned, int, std::string, etc.) This is a static
                  parameter, which is passed to parameterize cmdl::Arg<> template.

  -# initval:     the argument's default value.

  -# optchar:     a short, single-character option name, like 'x'. Has type 'char'; to
                  specify that the option has no short name, one should pass 0 as value
                  of this argument. E.g., 'x' means that the option may be specified in
                  the command line as "-x".

  -# keyword:     a long option name, like "extra". Has type 'const char *'. E.g.,
                  "extra" means that the option may be specified in the command line as
                  "--extra".

  -# valname:     a value name (if the argument takes a value); note that the only
                  arguments that do @em not take a value are flags (i.e. boolean
                  switches). This name will appear in the argument description. Besides,
                  the form of the value of this parameter allows to implicitly specify
                  some syntax flags for the argument. One may enclose the value name
                  between '[' and ']' to indicate the value is optional. Also, one may
                  follow the actual value name with "..." to indicate that the value
                  corresponds to a LIST of values. E.g. "[file]" sets CmdArg::isVALOPT;
                  "files ..." sets CmdArg::isLIST; "[files ...]"sets
                  CmdArg::isVALOPT|CmdArg::isLIST.

  -# description: an argument description, which should appear when command usage text is
                  printed, e.g. "turn on debugging".

  -# flags:       optional set of flags an describing the syntax of the argument.

  Macros:
  CMDL_FLAG(varname, optchar, keyword, description, ...)
*******************************************************************************/
#include "cmdline.h"
#include "cmdargs.h"

#include <string>
#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <list>
#include <stdexcept>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if !defined(EXIT_USAGE)
/// Exit code for invalid usage (bad argument, etc.)
#define EXIT_USAGE 2
#elif EXIT_USAGE != 2
#error "Invalid redefinition of EXIT_USAGE"
#endif


#define CMDL_FLAG(varname, optchar, keyword, description, ...)           \
   ::cmdl::Arg<bool> varname ((optchar), (keyword), (description), ##__VA_ARGS__)

#define CMDL_BOOL(varname, initval, optchar, keyword, description, ...)      \
   ::cmdl::Arg<bool> varname ((initval), (optchar), (keyword), (description), ##__VA_ARGS__)

#define CMDL_OPT(varname, type, initval, optchar, keyword, valname, description, ...) \
   ::cmdl::Arg<type > varname ((initval), (optchar), (keyword), (valname), (description), ##__VA_ARGS__)

#define CMDL_LISTOPT(varname, type, optchar, keyword, valname, description, ...) \
   ::cmdl::Arg<type > varname ((optchar), (keyword), (valname), (description), ##__VA_ARGS__)

#define CMDL_ARG(varname, type, initval, valname, description, ...)     \
   ::cmdl::Arg<type > varname ((initval), (valname), (description), ##__VA_ARGS__)

#define CMDL_LISTARG(varname, type, valname, description, ...) \
   ::cmdl::Arg<type > varname ((valname), (description), ##__VA_ARGS__)

#define CMDL_REGISTER_GLOBAL(type, name) \
   type &ARG_##name = ::cmdl::global::register_arg(name).value()

#define CMDL_GLOBAL_FLAG(varname, optchar, keyword, description, ...)   \
   static CMDL_FLAG(varname, (optchar), (keyword), (description), ##__VA_ARGS__) ; \
   CMDL_REGISTER_GLOBAL(bool, varname)

#define CMDL_GLOBAL_BOOL(varname, initval, optchar, keyword, description, ...) \
   static CMDL_BOOL(varname, (initval), (optchar), (keyword), (description), ##__VA_ARGS__) ; \
   CMDL_REGISTER_GLOBAL(bool, varname)

#define CMDL_GLOBAL_OPT(varname, type, initval, optchar, keyword, valname, description, ...) \
   static CMDL_OPT(varname, type, (initval), (optchar), (keyword), (valname), (description), ##__VA_ARGS__) ; \
   CMDL_REGISTER_GLOBAL(type, varname)

#define CMDL_GLOBAL_ARG(varname, type, ...)                             \
   ::cmdl::Arg<type > varname (__VA_ARGS__) ;                           \
   CMDL_REGISTER_GLOBAL(type, varname)

#define CMDL_GLOBAL_ARGOBJ(varname, argobj_type, ...)                   \
   argobj_type varname (__VA_ARGS__) ;                                  \
   CMDL_REGISTER_GLOBAL(argobj_type::type, varname)

#define CMDL_LOG_CMDERROR(cmdline, output)            \
   do {                                               \
      const ::CmdLine &_##__LINE__ = (cmdline) ;      \
      if (!(_##__LINE__.flags() & ::CmdLine::QUIET))  \
         _##__LINE__.error() << output << std::endl ; \
   } while(false)

/*******************************************************************************
 namespace cmdl
*******************************************************************************/
namespace cmdl {
namespace detail {

// std quasi: we don't want to engage std:: here, so create adhoc
// integral_constant<>
template<int n> struct intconst { static const int value = n ; } ;
template<int n> const int intconst<n>::value ;

inline const char *cstr(const char *s) { return s ; }

inline const char *cstr(const std::string &s) { return s.c_str() ; }

struct buf_ostream : private std::basic_streambuf<char>, public std::ostream {
   buf_ostream(char *buf, size_t sz) throw() :
      std::ostream(static_cast<std::basic_streambuf<char> *>(this))
   {
      if (buf && sz)
      {
         *buf = buf[sz - 1] = 0 ;
         setp(buf, buf + (sz - 1)) ;
      }
   }
} ;

/*******************************************************************************
 Argument baseclass templates
 Traits classes
*******************************************************************************/
#define CMDL_CONSTRUCTORS(classnam)                                     \
      classnam(type default_value,                                      \
               char         optchar,                                    \
               const char * keyword,                                    \
               const char * value_name,                                 \
               const char * description,                                \
               unsigned     flags =CmdArg::isVALREQ) :                  \
         ancestor(default_value, optchar, keyword, value_name, description, flags) \
      {}                                                                \
                                                                        \
      classnam(type default_value,                                      \
               const char * value_name,                                 \
               const char * description,                                \
               unsigned     flags =CmdArg::isPOSVALREQ) :               \
         ancestor(default_value, value_name, description, flags)        \
      {}

/******************************************************************************/
/** Base class for all scalar arguments

 @param T Argument type
 @param B Cmdline base argument class
*******************************************************************************/
template<typename T, typename B = CmdArg>
class scalar_argument : public B {
   typedef B ancestor ;
public:
   typedef T type ;

   // FIXME: should be virtual CmdArg::print()

   type &value() { return _value ; }
   const type &value() const { return _value ; }

   operator type() const { return value() ; }

   const type &default_value() const { return _default_value ; }

protected:
   type _value ;
private:
   const type _default_value ;

protected:
   scalar_argument(type default_value,
                   char         optchar,
                   const char * keyword,
                   const char * value_name,
                   const char * description,
                   unsigned     flags = CmdArg::isVALREQ) :
      ancestor(optchar, keyword, value_name, description, flags),
      _value(),
      _default_value(default_value)
   {}

   scalar_argument(type default_value,
                   const char * value_name,
                   const char * description,
                   unsigned     flags = CmdArg::isVALREQ) :
      ancestor(value_name, description, CmdArg::isPOS | flags),
      _value(default_value),
      _default_value(default_value)
   {}

   void assign(const type &value) { _value = value ; }

   void reset()
   {
      ancestor::reset() ;
      assign(default_value()) ;
   }

   char *valstr(char *buf, size_t size, CmdArg::ValStr what_value) const
   {
      if (!buf || !size || (what_value == CmdArg::VALSTR_DEFNOZERO && default_value() == type()))
         return NULL ;

      buf_ostream os (buf, size) ;
      os << (what_value == CmdArg::VALSTR_ARGVAL ? value() : default_value()) << std::ends ;
      return buf ;
   }

   int operator() (const char *&arg, CmdLine &cmdline)
   {
      return compile(arg, cmdline) && validate(_value, cmdline) ? 0 : -1 ;
   }

   virtual bool validate(type &, CmdLine &) { return true ; }
   virtual bool compile(const char *&arg, CmdLine &) = 0 ;
} ;

/******************************************************************************/
/** Generic argtype traits
*******************************************************************************/
template<typename T> struct argtype_traits {
   class basetype : public scalar_argument<T> {
      typedef scalar_argument<T> ancestor ;
   public:
      typedef typename ancestor::type type ;
   protected:
      CMDL_CONSTRUCTORS(basetype) ;
   } ;
} ;

/*******************************************************************************
 Base class for integer argument traits (of different integral types)
*******************************************************************************/
template<typename T>
struct argint_traits {
   static const bool is_signed = (T)~(T)0 < (T)1 ;

   /*******************************************************************************
                  class argint_traits<T>::basetype
   *******************************************************************************/
   class basetype : public scalar_argument<T> {
      typedef scalar_argument<T> ancestor ;
   public:
      typedef typename ancestor::type type ;

   protected:
      CMDL_CONSTRUCTORS(basetype) ;

      bool compile(const char *&arg, CmdLine &) ;

   private:
      static type strtoint(const char *nptr, char **endptr, intconst<true> * /*signed_tag*/)
      {
         const type mint = (type)1 << (sizeof(type)*8 - 1) ;
         const type maxt = ~mint ;

         const long long r = strtoll(nptr, endptr, 0) ;

         if (r < mint || r > maxt)
         {
            // Out of range
            *endptr = const_cast<char *>(nptr) ;
            return 1 ;
         }
         return r ;
      }

      static type strtoint(const char *nptr, char **endptr, intconst<false> * /*signed_tag*/)
      {
         const unsigned long long r = strtoull(nptr, endptr, 0) ;
         if (r > (unsigned long long)(type)~0ULL)
         {
            // Out of range
            *endptr = const_cast<char *>(nptr) ;
            return 1 ;
         }
         return r ;
      }

      static const char *type_name()
      {
         static char name[] = "uint\0\0\0" ;
         if (!name[4])
            sprintf(name + 4, "%d", ((int)sizeof(type)*8)) ;
         return name + is_signed ;
      }

      char *valstr(char *buf, size_t size, CmdArg::ValStr what_value) const
      {
         if (!buf || !size || (what_value == CmdArg::VALSTR_DEFNOZERO && this->default_value() == type()))
            return NULL ;

         char tmpbuf[64] ;
         const type v = what_value == CmdArg::VALSTR_ARGVAL ? this->value() : this->default_value() ;
         is_signed ? sprintf(tmpbuf, "%lld", (long long)v) : sprintf(tmpbuf, "%llu", (unsigned long long)v) ;
         const size_t sz = std::min(strlen(tmpbuf), size) ;
         memcpy(buf, tmpbuf, sz) ;
         buf[sz] = 0 ;
         return buf ;
      }
   } ;
} ;

/*******************************************************************************
 argint_traits::argbase<T>
*******************************************************************************/
template<typename T>
bool argint_traits<T>::basetype::compile(const char *&arg, CmdLine &cmdline)
{
   if (!arg)
      return true ;  // no value given - nothing to do

   if (!*arg)
   {
      CMDL_LOG_CMDERROR(cmdline, "empty " << type_name() << " value specified.") ;
      return false ;
   }

   char *ptr = NULL ;

   // Compile the string into an integer; watch out for -c0xa vs -axc0!
   const type result = strtoint(arg, &ptr, (intconst<is_signed> *)NULL) ;
   if (ptr == arg)
   {
      // Invalid integer (or overflow, or a sign where should be unsigned)
      if (result)
         CMDL_LOG_CMDERROR(cmdline, "'" << arg << "' does not fit into " << type_name() << '.') ;
      else
         CMDL_LOG_CMDERROR(cmdline, "invalid " << type_name() << " value '" << arg << "'.") ;
      return false ;
   }

   this->_value = result ;
   arg = ptr ;

   return true ;
}

/*******************************************************************************
 argtype_traits<T> for integral types
*******************************************************************************/
#define CMDL_DEFINE_INTTRAITS(type) \
   template<> struct argtype_traits<type > : argint_traits<type > {}

CMDL_DEFINE_INTTRAITS(signed char) ;
CMDL_DEFINE_INTTRAITS(unsigned char) ;
CMDL_DEFINE_INTTRAITS(short) ;
CMDL_DEFINE_INTTRAITS(unsigned short) ;
CMDL_DEFINE_INTTRAITS(int) ;
CMDL_DEFINE_INTTRAITS(unsigned) ;
CMDL_DEFINE_INTTRAITS(long) ;
CMDL_DEFINE_INTTRAITS(unsigned long) ;
CMDL_DEFINE_INTTRAITS(long long) ;
CMDL_DEFINE_INTTRAITS(unsigned long long) ;

#undef CMDL_DEFINE_INTTRAITS

/*******************************************************************************
                     struct argtype_traits<double>
*******************************************************************************/
template<> struct argtype_traits<double> {
   class basetype : public scalar_argument<double> {
      typedef scalar_argument<double> ancestor ;
   protected:
      CMDL_CONSTRUCTORS(basetype) ;

      bool compile(const char *&arg, CmdLine &cmdline)
      {
         if (!arg)
            return true ;  // no value given - nothing to do

         if (!*arg)
         {
            CMDL_LOG_CMDERROR(cmdline, "empty double value specified.") ;
            return false ;
         }

         char *ptr = NULL ;

         const type result = ::strtod(arg, &ptr) ;
         if (ptr == arg)
         {
            CMDL_LOG_CMDERROR(cmdline, "invalid double value '" << arg << "'.") ;
            return false ;
         }

         _value = result ;
         arg = ptr ;

         return true ;
      }
   } ;
} ;

/*******************************************************************************
                     struct argtype_traits<char>
*******************************************************************************/
template<> struct argtype_traits<char> {
   class basetype : public scalar_argument<char, CmdArgCharCompiler> {
      typedef scalar_argument<char, CmdArgCharCompiler> ancestor ;
   protected:
      CMDL_CONSTRUCTORS(basetype) ;

      bool compile(const char *&arg, CmdLine &cmdline)
      {
         return !CmdArgCharCompiler::compile(arg, cmdline, _value) ;
      }
   } ;
} ;

/*******************************************************************************
                     struct argtype_traits<std::string>
*******************************************************************************/
template<> struct argtype_traits<std::string> {
   class basetype : public scalar_argument<std::string> {
      typedef scalar_argument<std::string> ancestor ;
   protected:
      CMDL_CONSTRUCTORS(basetype) ;

      bool compile(const char *&arg, CmdLine &)
      {
         if (!arg)
            return true ;


         std::string(arg).swap(_value) ;
         arg = NULL ;
         return true ;
      }
   } ;
} ;

/*******************************************************************************
                     struct argtype_traits<std::pair>
*******************************************************************************/
template<typename T>
struct item_compiler : argtype_traits<T>::basetype {
   item_compiler() : argtype_traits<T>::basetype(T(), "", NULL, CmdArg::isPOS) {}

   bool compile(const char *&arg, CmdLine &cmdline)
   {
      return argtype_traits<T>::basetype::compile(arg, cmdline) ;
   }
} ;

template<typename K, typename V> struct argtype_traits<std::pair<K, V> > {
   class basetype : public scalar_argument<std::pair<K, V> > {
      typedef scalar_argument<std::pair<K, V> > ancestor ;
      typedef K key_type ;
      typedef V value_type ;
      typedef typename ancestor::type type ;
   private:
      template<typename T>
      bool compile_half(item_compiler<T> &compiler, const char *half, CmdLine &cmdline, T &result)
      {
         if (!compiler.compile(half, cmdline) || (half && *half))
         {
               CMDL_LOG_CMDERROR(cmdline, "invalid pair format.") ;
               return false ;
         }
         result = compiler.value() ;
         return true ;
      }
   protected:
      item_compiler<key_type>   _key_compiler ;
      item_compiler<value_type> _value_compiler ;
      std::pair<char, char>      _delim ;

      CMDL_CONSTRUCTORS(basetype) ;

      bool compile(const char *&arg, CmdLine &cmdline)
      {
         if (!arg)
            return true ;
         if (!*arg)
         {
            CMDL_LOG_CMDERROR(cmdline, "empty pair argument specified.") ;
            return false ;
         }

         // Reset the value
         this->_value = this->default_value() ;
         const char *delim = arg + strcspn(arg, ":=") ;
         _delim.first = *delim ? *delim : ':' ;
         if ((delim != arg &&
              !compile_half(_key_compiler, std::string(arg, delim).c_str(), cmdline, this->_value.first)) ||

             (*delim && *++delim &&
              !compile_half(_value_compiler, delim, cmdline, this->_value.second)))
            return false ;

         arg = NULL ;

         return true ;
      }

      char *valstr(char *buf, size_t size, CmdArg::ValStr what_value) const
      {
         if ((!buf || !size)
             || (what_value == CmdArg::VALSTR_DEFNOZERO && this->default_value() == type()))
            return NULL ;

         buf_ostream os (buf, size) ;
         const type &ref = (what_value == CmdArg::VALSTR_ARGVAL) ? this->value() : this->default_value() ;
         os << ref.first << _delim.first << ref.second << std::ends ;
         return buf ;
      }
   } ;
} ;

/*******************************************************************************
                     struct argtype_traits<bool>
*******************************************************************************/
template<> struct argtype_traits<bool> {
   class arg_compiler : public CmdArgBoolCompiler {
   public:
      arg_compiler(char         optchar,
                   const char * keyword,
                   const char *, // value_name - unused
                   const char * description,
                   unsigned     flags) :
         CmdArgBoolCompiler(optchar, keyword, description, flags)
      {}
   } ;

   class basetype : public scalar_argument<bool, arg_compiler> {
      typedef scalar_argument<bool, arg_compiler> ancestor ;
   protected:
      basetype(bool default_value,
               char         optchar,
               const char * keyword,
               const char * description,
               unsigned     flags) :
         ancestor(default_value, optchar, keyword, NULL, description, flags),
         _initval(default_value)
      {}

      bool compile(const char *&arg, CmdLine &cmdline)
      {
         return !arg_compiler::compile(arg, cmdline, _value, !_initval) ;
      }
   private:
      const bool _initval ;
   } ;
} ;

/******************************************************************************/
/** Base class for all sequence arguments

 @param T   Container type, e.g. std::list<int>; there must be S::value_type defined and
            scalar_argument<S::value_type> must constitute a valid class
*******************************************************************************/
template<typename T>
class list_argument : public CmdArg {
   typedef CmdArg ancestor ;
public:
   typedef T                           container_type ;
   typedef T                           type ;
   typedef typename T::value_type      value_type ;
   typedef typename T::const_iterator  const_iterator ;
   typedef const_iterator              iterator ;

   type &value() { return _container ; }
   const type &value() const { return _container ; }

   const_iterator begin() const { return value().begin() ; }
   const_iterator end() const { return value().end() ; }

   size_t size() const { return value().size() ; }
   bool empty() const { return value().empty() ; }

protected:
   container_type             _container ;
   item_compiler<value_type>  _compiler ;

   list_argument(char         optchar,
                 const char * keyword,
                 const char * value_name,
                 const char * description,
                 unsigned     flags = CmdArg::isVALREQ) :
      ancestor(optchar, keyword, value_name, description, CmdArg::isLIST | flags)
   {}

   list_argument(const char * value_name,
                 const char * description,
                 unsigned     flags = CmdArg::isVALREQ) :
      ancestor(value_name, description, CmdArg::isLIST | CmdArg::isPOS | flags)
   {}

   void assign(const type &value) { _container = value ; }

   int operator() (const char *&arg, CmdLine &cmdline)
   {
      return compile(arg, cmdline) && validate(value(), cmdline) ? 0 : -1 ;
   }

   virtual bool validate(type &, CmdLine &) { return true ; }

private:
   bool compile(const char *&arg, CmdLine &cmdline)
   {
      if (!arg)
         return true ;  // no value given - nothing to do

      if (!_compiler.compile(arg, cmdline))
         return false ;

      _container.insert(_container.end(), _compiler.value()) ;
      return true ;
   }
} ;

} /******* end of namespace cmdl::detail *******/
/******************************************************************************/
/******************************************************************************/


/******************************************************************************/
/** Command-line argument class template.

 Instances of this template represent the actual command-line argument classes. This is
 the main interface to command-line arguments.

 @param T   Argument value type, like int, std::string, bool, etc.
*******************************************************************************/
template<typename T>
class Arg : public detail::argtype_traits<T>::basetype {
   typedef typename detail::argtype_traits<T>::basetype ancestor ;
public:
   Arg(T default_value,
       char         optchar,
       const char * keyword,
       const char * value_name,
       const char * description,
       unsigned     flags =CmdArg::isVALREQ) :
      ancestor(default_value, optchar, keyword, value_name, description, flags)
   {}

   Arg(char         optchar,
       const char * keyword,
       const char * value_name,
       const char * description,
       unsigned     flags =CmdArg::isVALREQ) :
      ancestor(T(), optchar, keyword, value_name, description, flags)
   {}

   Arg(T default_value,
       const char * value_name,
       const char * description,
       unsigned     flags =CmdArg::isPOSVALREQ) :
      ancestor(default_value, value_name, description, flags)
   {}

   Arg(const char * value_name,
       const char * description,
       unsigned     flags =CmdArg::isPOSVALREQ) :
      ancestor(T(), value_name, description, flags)
   {}

   Arg &operator=(const T &value)
   {
      this->assign(value) ;
      return *this ;
   }
} ;

/******************************************************************************/
/** Arg<T> specialization for std::string arguments.
*******************************************************************************/
template<>
class Arg<std::string> : public detail::argtype_traits<std::string>::basetype {
   typedef typename detail::argtype_traits<std::string>::basetype ancestor ;
public:
   CMDL_CONSTRUCTORS(Arg) ;

   Arg(char         optchar,
       const char * keyword,
       const char * value_name,
       const char * description,
       unsigned     flags =CmdArg::isVALREQ) :
      ancestor(std::string(), optchar, keyword, value_name, description, flags)
   {}

   Arg(const char * value_name,
       const char * description,
       unsigned     flags =CmdArg::isPOSVALREQ) :
      ancestor(std::string(), value_name, description, flags)
   {}

   Arg(const char * defvalue,
       char         optchar,
       const char * keyword,
       const char * valname,
       const char * description,
       unsigned     flags =CmdArg::isVALREQ) :
      ancestor(defvalue ? defvalue : "", optchar, keyword, valname, description, flags)
   {}

   Arg(const char * defvalue,
       const char * valname,
       const char * description,
       unsigned     flags =CmdArg::isPOSVALREQ) :
      ancestor(defvalue ? defvalue : "", valname, description, flags)
   {}

   Arg &operator=(const std::string &value)
   {
      assign(value) ;
      return *this ;
   }
} ;

/******************************************************************************/
/** Arg<T> specialization for boolean arguments.
*******************************************************************************/
template<> class Arg<bool> : public detail::argtype_traits<bool>::basetype {
   typedef detail::argtype_traits<bool>::basetype ancestor ;
public:
   Arg(char         optchar,
       const char * keyword,
       const char * description,
       unsigned     flags = 0) :
      ancestor(false, optchar, keyword, description, flags)
   {}

   Arg(bool initval,
       char         optchar,
       const char * keyword,
       const char * description,
       unsigned     flags = 0) :
      ancestor(initval, optchar, keyword, description, flags)
   {}

   Arg &operator=(bool value)
   {
      assign(value) ;
      return *this ;
   }
} ;

/******************************************************************************/
/** Macro to define a command-line argument class for a multiple-value sequence argument
 type (e.g. std::list<std::string>).

 @param stemplate  Sequence template, like std::vector.
*******************************************************************************/
#define CMDL_DEFINE_LIST_ARGTYPE(stemplate)                          \
template<typename T>                                                    \
class Arg<stemplate<T> > : public detail::list_argument<stemplate<T> > { \
   typedef detail::list_argument<stemplate<T> > ancestor ;               \
public:                                                                 \
   Arg(char         optchar,                                            \
       const char * keyword,                                            \
       const char * value_name,                                         \
       const char * description,                                        \
       unsigned     flags =CmdArg::isVALREQ) :                          \
      ancestor(optchar, keyword, value_name, description, flags)        \
   {}                                                                   \
                                                                        \
   Arg(const char * value_name,                                         \
       const char * description,                                        \
       unsigned     flags =CmdArg::isPOSVALREQ) :                       \
      ancestor(value_name, description, flags)                          \
   {}                                                                   \
                                                                        \
   Arg &operator=(const typename ancestor::type &value)                 \
   {                                                                    \
      this->assign(value) ;                                             \
      return *this ;                                                    \
   }                                                                    \
                                                                        \
   friend std::ostream &operator<<(std::ostream &os, const Arg &v)      \
   {                                                                    \
      const char *delim = "" ;                                          \
      for (typename ancestor::const_iterator i = v.begin(), e = v.end() ; \
           i != e ; ++i, delim = " ")                                   \
         os << delim << *i ;                                            \
      return os ;                                                       \
   }                                                                    \
}

CMDL_DEFINE_LIST_ARGTYPE(std::vector) ;
CMDL_DEFINE_LIST_ARGTYPE(std::list) ;
CMDL_DEFINE_LIST_ARGTYPE(std::set) ;

/******************************************************************************/
/** Enumeration argument: associates names with values.

 @param T Argument value type
*******************************************************************************/
template<typename T>
class ArgEnum : public Arg<std::string> {
   typedef Arg<std::string> ancestor ;
public:
   typedef T type ;
   typedef std::pair<std::string, type> keyed_value ; ;

   ArgEnum(keyed_value default_value,
           char         optchar,
           const char * keyword,
           const char * value_name,
           const char * description,
           unsigned     flags = CmdArg::isVALREQ) :
      ancestor(default_value.first, optchar, keyword, value_name, description, flags),
      _default_value(default_value.second),
      _value(),
      _valmap(&default_value, &default_value + 1)
   {}

   ArgEnum(keyed_value default_value,
           const char * value_name,
           const char * description,
           unsigned     flags = CmdArg::isVALREQ) :
      ancestor(default_value.first, value_name, description, flags),
      _default_value(default_value.second),
      _value(),
      _valmap(&default_value, &default_value + 1)
   {}

   type &value() { return _value ; }
   const type &value() const { return _value ; }

   operator type() const { return value() ; }

   const type &default_value() const { return _default_value ; }

   ArgEnum &append(const std::string &key, const type &val) ;

   ArgEnum &append(const keyed_value &keyval)
   {
      return append(keyval.first, keyval.second) ;
   }

private:
   typedef std::map<std::string, type> valmap_type ;

   type        _default_value ;
   type        _value ;
   valmap_type _valmap ;

protected:
   void reset()
   {
      ancestor::reset() ;
      type v (default_value()) ;
      using std::swap ;
      swap(_value, v) ;
   }

   bool compile(const char *&arg, CmdLine &cmdl)
   {
      if (!ancestor::compile(arg, cmdl))
         return false ;
      const typename valmap_type::const_iterator vi (_valmap.find(ancestor::value())) ;
      if (vi == _valmap.end())
         return false ;
      type v (vi->second) ;
      using std::swap ;
      swap(v, _value) ;
      return true ;
   }
} ;

template<typename T>
ArgEnum<T> &ArgEnum<T>::append(const std::string &key, const type &val)
{
   using std::swap ;
   type v (val) ;
   if (key == ancestor::default_value())
      swap(_default_value, v) ;
   if (key == ancestor::value())
      swap(_value, v) ;
   swap(_valmap[key], v) ;
   return *this ;
}

/******************************************************************************/
/** Counter option: its value is equal to initial value plus number of times it
 appears in the command line.

 The good example of such argument is "verbosity": -v or -vv or -vvv, etc.
 This type of argument may appear only as a non-positional option without a value.
*******************************************************************************/
class ArgCounter : public Arg<int> {
   typedef Arg<int> ancestor ;
public:
   ArgCounter(char         optchar,
              const char * keyword,
              const char * description,
              unsigned     flags = 0) :
      ancestor(0, optchar, keyword, NULL, description, flags & ~(isVALTAKEN|isPOS))
   {}

   ArgCounter(int          defvalue,
              char         optchar,
              const char * keyword,
              const char * description,
              unsigned     flags = 0) :
      ancestor(defvalue, optchar, keyword, NULL, description, flags & ~(isVALTAKEN|isPOS))
   {}

   bool compile(const char *&, CmdLine &)
   {
      ++value() ;
      return true ;
   }
} ;

/******************************************************************************/
/** Command-line iterator over STL iterator
*******************************************************************************/
template<typename InputIterator>
class ArgIter : public CmdLineArgIter {
public:
   typedef InputIterator iterator ;

   ArgIter() : _begin(), _end() {}

   ArgIter(iterator b, iterator e) : _begin(b), _end(e) {}

   const char *operator()()
   {
      if (_begin == _end)
         return NULL ;
      const char * const result = detail::cstr(*_begin) ;
      ++_begin ;
      return result ;
   }
private:
   iterator _begin ;
   iterator _end ;
} ;

/******************************************************************************/
/** Exception to indicate invalid command-line argument(s)

 @note: Cmdline library itself does not use exception anywhere, this exception
 type is provided for a library user.
*******************************************************************************/
class invalid_cmdarg : public std::runtime_error {
   typedef std::runtime_error ancestor ;
public:
   explicit invalid_cmdarg(const std::string &msg) : ancestor(msg) {}
} ;

/*******************************************************************************
 Global command line
*******************************************************************************/
namespace global {

void set_description(const char *desc) ;

void set_name(const char *name) ;

const char *get_name() ;

CmdArg &register_arg(CmdArg &arg) ;

template<typename T>
inline Arg<T> &register_arg(Arg<T> &arg)
{
   register_arg(*static_cast<CmdArg *>(&arg)) ;
   return arg ;
}

template<typename T>
inline ArgEnum<T> &register_arg(ArgEnum<T> &arg)
{
   register_arg(*static_cast<CmdArg *>(&arg)) ;
   return arg ;
}

unsigned parse_cmdline(int argc, const char * const *argv) ;

unsigned parse_cmdline(int argc, const char * const *argv,
                       unsigned flags, unsigned mask,
                       CmdLine::arglogger logger = NULL, void *logger_data = NULL) ;

} // end of namespace cmdl::global

inline bool is_given(const CmdArg &arg)
{
   return !!(arg.flags() & CmdArg::GIVEN) ;
}

template<typename T>
inline std::ostream &operator<<(std::ostream &os, const Arg<T> &v)
{
   return os << (T)v ;
}
} // end of namespace cmdl

#endif /* __CMDLINE_CMDEXT_H */
