/*-*- mode: c++; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)) -*-*/
#ifndef __CMDLINE_CMDEXT_H
#define __CMDLINE_CMDEXT_H
/*******************************************************************************
 FILE         :   cmdext.h
 COPYRIGHT    :   Yakov Markovitch, 2009-2017. All rights reserved.
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
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <initializer_list>
#include <limits>
#include <functional>

#include <vector>
#include <set>
#include <map>
#include <list>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#if !defined(EXIT_USAGE)
/// Exit code for invalid usage (bad argument, etc.)
#define EXIT_USAGE 2
#elif EXIT_USAGE != 2
#error "Invalid redefinition of EXIT_USAGE"
#endif

CMDL_MS_DIAGNOSTIC_PUSH()
CMDL_MS_IGNORE_WARNING(4996)

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

#define CMDL_ARGTYPE_TRAITS_INSTANCE(type)                              \
   namespace cmdl {                                                     \
   template<> struct argtype_traits<type> : argtype_handler<type> {     \
   protected: using argtype_handler<type>::argtype_handler ;            \
   } ; }

/*******************************************************************************
 namespace cmdl
*******************************************************************************/
namespace cmdl {

const unsigned ARGSYNTAX_FLAGS = 0xffff ;

enum ArgSyntaxExt : unsigned {
   isEXT0   = 0x10000,
   isEXT1   = 0x20000,
   isEXT2   = 0x40000,
   isEXT3   = 0x80000,
   isEXT4  = 0x100000,
   isEXT5  = 0x200000,
   isEXT6  = 0x400000,
   isEXT7  = 0x800000
} ;

const ArgSyntaxExt isNOCASE = isEXT0 ;

/******************************************************************************/
/** Generic argument type traits, used as a base class for argument classes
*******************************************************************************/
template<typename T> struct argtype_traits ;

/*******************************************************************************
 Argument baseclass templates
 Traits classes
*******************************************************************************/
#define CMDL_CONSTRUCTORS(classnam)                                     \
   classnam(type default_value, char optchar, const char * keyword,     \
            const char * value_name, const char * description,          \
            unsigned     flags) :                                       \
      ancestor(std::move(default_value), optchar, keyword, value_name, description, flags) {} \
                                                                        \
   classnam(type default_value, char optchar, const char *keyword, const char *value_name, \
            const char *description) :                                  \
      classnam(std::move(default_value), optchar, keyword, value_name, description, CmdArg::isVALREQ) {} \
                                                                        \
   classnam(type default_value, const char *value_name,                 \
            const char *description, unsigned flags) :                  \
      ancestor(std::move(default_value), value_name, description, flags) {} \
                                                                        \
   classnam(type default_value, const char *value_name, const char *description) : \
      classnam(std::move(default_value), value_name, description, CmdArg::isPOSVALREQ) {}

/******************************************************************************/
/** Output stream over an external character buffer of fixed size
*******************************************************************************/
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

/******************************************************************************/
/** Base class for all scalar arguments

 @param T Argument type
 @param B Cmdline base argument class
*******************************************************************************/
template<typename T, typename B = CmdArg>
struct scalar_argument : B {
   typedef T      type ;

   // FIXME: should be virtual CmdArg::print()

   type &value() { return _value ; }
   const type &value() const { return _value ; }
   operator type() const { return value() ; }
   const type &default_value() const { return _default_value ; }

   friend std::ostream &operator<<(std::ostream &os, const scalar_argument &v)
   {
      return os << v.value() ;
   }

protected:
   type _value ;
private:
   const type _default_value ;

   typedef B ancestor ;
public:
   scalar_argument(type &&default_value, char optchar, const char *keyword,
                   const char *value_name, const char *description,
                   unsigned flags) :
      ancestor(optchar, keyword, value_name, description, flags &~ CmdArg::isLIST),
      _value(), _default_value(std::move(default_value))
   {}

   scalar_argument(type &&default_value, char optchar, const char *keyword,
                   const char *value_name, const char *description) :
      scalar_argument(default_value, optchar, keyword, value_name, description, CmdArg::isVALREQ)
   {}

   scalar_argument(type &&default_value, const char *value_name,
                   const char *description, unsigned flags) :
      ancestor(value_name, description, CmdArg::isPOS | (flags &~ CmdArg::isLIST)),
      _value(std::move(default_value)), _default_value(_value)
   {}

   scalar_argument(type &&default_value, const char *value_name, const char *description) :
      scalar_argument(default_value, value_name, description, CmdArg::isPOSVALREQ)
   {}

protected:
   void assign(const type &value) { _value = value ; }

   void reset() override
   {
      ancestor::reset() ;
      assign(default_value()) ;
   }

   char *valstr(char *buf, size_t size, CmdArg::ValStr what_value) const override
   {
      if (!buf || !size || (what_value == CmdArg::VALSTR_DEFNOZERO && default_value() == type()))
         return NULL ;

      buf_ostream os (buf, size) ;
      os << (what_value == CmdArg::VALSTR_ARGVAL ? value() : default_value()) << std::ends ;
      return buf ;
   }

   int operator() (const char *&arg, CmdLine &cmdline) override
   {
      return compile(arg, cmdline) && validate(_value, cmdline) ? 0 : -1 ;
   }

   virtual bool validate(type &, CmdLine &) { return true ; }
   virtual bool compile(const char *&arg, CmdLine &) = 0 ;
} ;

/*******************************************************************************

*******************************************************************************/
template<typename T, typename B = scalar_argument<T, CmdArg> >
struct argtype_handler : B {
private:
   typedef B ancestor ;
public:
   using typename ancestor::type ;

protected:
   bool validate(type &v, CmdLine &c) override { return ancestor::validate(v, c) ; }
   /// Intentionally unimplemented
   bool compile(const char *&arg, CmdLine &) override ;

   using ancestor::ancestor ;
} ;

template<typename T>
struct interval : std::pair<T, T> {
   using std::pair<T, T>::pair ;

   template<typename A>
   interval &operator=(A &&a)
   {
      std::pair<T, T>::operator=(std::forward<A>(a)) ;
      return *this ;
   }
} ;

/*******************************************************************************

*******************************************************************************/
struct separator {
   constexpr separator() : _size(0), _sep{0, 0} {}
   explicit constexpr separator(char sep) : _size(!!sep), _sep{sep, 0} {}
   explicit separator(const char *sep)
   {
      if (!sep || !*sep)
      {
         _size = 0 ;
         _sep[1] = _sep[0] = 0 ;
      }
      else
      {
         memcpy(_sep, sep, _size = (uint8_t)std::min(strlen(sep), sizeof _sep - 1)) ;
         _sep[_size] = 0 ;
      }
   }

   size_t size() const { return _size ; }
   char front() const { return *_sep ; }

   const char *c_str() const { return _sep ; }

   const char *begin() const { return _sep ; }
   const char *end() const { return _sep + _size ; }

   const char *find_in(const char *start, const char *finish) const
   {
      return size() > 1
         ? std::search(start, finish, begin(), end())
         : std::find(start, finish, front()) ;
   }
   const char *find_in(const char *str) const
   {
      return find_in(str, str + strlen(str)) ;
   }

private:
   uint8_t _size ;
   char    _sep[7] ;
} ;

/******************************************************************************/
/** Command-line argument class template.

 Instances of this template represent the actual command-line argument classes. This is
 the main interface to command-line arguments.

 @param T   Argument value type, like int, std::string, bool, etc.
*******************************************************************************/
template<typename T>
class Arg : public argtype_traits<T> {
   typedef argtype_traits<T> ancestor ;
public:
   Arg(T default_value, char optchar, const char *keyword, const char *value_name, const char *description, unsigned flags) :
      ancestor(std::move(default_value), optchar, keyword, value_name, description, flags) {}

   Arg(T default_value, char optchar, const char *keyword, const char *value_name, const char *description) :
      ancestor(std::move(default_value), optchar, keyword, value_name, description, CmdArg::isVALREQ) {}

   Arg(char optchar, const char *keyword, const char *value_name, const char *description, unsigned flags) :
      Arg(T(), optchar, keyword, value_name, description, flags) {}

   Arg(char optchar, const char *keyword, const char *value_name, const char *description) :
       Arg(optchar, keyword, value_name, description, CmdArg::isVALREQ) {}

   Arg(T default_value, const char *value_name, const char *description, unsigned flags) :
      ancestor(std::move(default_value), value_name, description, flags) {}

   Arg(T default_value, const char *value_name, const char *description) :
      ancestor(std::move(default_value), value_name, description, CmdArg::isPOSVALREQ) {}

   Arg(std::nullptr_t, const char *value_name, const char *description, unsigned flags) :
      Arg(T(), value_name, description, flags) {}

   Arg(std::nullptr_t, const char *value_name, const char *description) :
      Arg(nullptr, value_name, description, CmdArg::isPOSVALREQ) {}

   Arg(const char *value_name, const char *description, unsigned flags) :
      Arg(T(), value_name, description, flags) {}

   Arg(const char *value_name, const char *description) :
      Arg(T(), value_name, description, CmdArg::isPOSVALREQ) {}

   Arg &operator=(const T &value)
   {
      this->assign(value) ;
      return *this ;
   }
} ;

/******************************************************************************/
/** "Subargument" parser
*******************************************************************************/
template<typename T>
struct item_compiler : Arg<T> {
   item_compiler() : Arg<T>("", "") {}

   bool compile(const char *&arg, CmdLine &cmdline) override
   {
      return Arg<T>::compile(arg, cmdline) ;
   }
} ;

/*******************************************************************************
 cmdl::detail
*******************************************************************************/
namespace detail {

inline const char *cstr(const char *s) { return s ; }
inline const char *cstr(const std::string &s) { return s.c_str() ; }

template<typename T, typename... A>
struct rebind___c {
   template<template<typename...> class Template, typename U, typename... Args>
   static Template<T, A...> eval(Template<U, Args...> *) ;
} ;

template<typename C, typename T, typename... A>
using rebind_container = decltype(rebind___c<T, A...>::eval((C *)nullptr)) ;

/*******************************************************************************
 Base class for integer argument traits (of different integral types)
*******************************************************************************/
template<typename T>
struct argint_traits : scalar_argument<T> {
   typedef typename std::is_signed<T>::type is_signed ;
   typedef scalar_argument<T> ancestor ;

   using typename ancestor::type ;
   /// Inherit constructors
   using ancestor::ancestor ;

protected:
   bool compile(const char *&arg, CmdLine &) override ;

private:
   static const char *type_name()
   {
      static char name[] = "uint\0\0\0" ;
      if (!name[4])
         sprintf(name + 4, "%d", ((int)sizeof(type)*8)) ;
      return name + is_signed() ;
   }

   char *valstr(char *buf, size_t size, CmdArg::ValStr what_value) const override
   {
      if (!buf || !size || (what_value == CmdArg::VALSTR_DEFNOZERO && this->default_value() == type()))
         return NULL ;

      struct local {
         static void itostr(type v, char *buf, std::true_type) { sprintf(buf, "%lld", (long long)v) ; }
         static void itostr(type v, char *buf, std::false_type) { sprintf(buf, "%llu", (unsigned long long)v) ; }
      } ;
      char tmpbuf[64] ;
      local::itostr(what_value == CmdArg::VALSTR_ARGVAL ? this->value() : this->default_value(), buf, is_signed()) ;
      const size_t sz = std::min(strlen(tmpbuf), size) ;
      memcpy(buf, tmpbuf, sz) ;
      buf[sz] = 0 ;
      return buf ;
   }
} ;

/*******************************************************************************
 argint_traits<T>::compile
*******************************************************************************/
template<typename T>
bool argint_traits<T>::compile(const char *&arg, CmdLine &cmdline)
{
   if (!arg)
      return true ;  // no value given - nothing to do

   if (!*arg)
   {
      CMDL_LOG_CMDERROR(cmdline, "empty " << type_name() << " value specified.") ;
      return false ;
   }

   CMDL_MS_PUSH_IGNORE_WARNING(4018)
   struct local {
      static type strtoint(const char *nptr, char **endptr, std::true_type)
      {
         const long long r = strtoll(nptr, endptr, 0) ;
         if (r >= (long long)std::numeric_limits<type>::min() && r <= (long long)std::numeric_limits<type>::max())
            return (type)r ;

         // Out of range
         *endptr = const_cast<char *>(nptr) ;
         return 1 ;
      }

      static type strtoint(const char *nptr, char **endptr, std::false_type)
      {
         const unsigned long long r = strtoull(nptr, endptr, 0) ;
         if (r <= (unsigned long long)(type)~0ULL)
            return (type)r ;

         // Out of range
         *endptr = const_cast<char *>(nptr) ;
         return 1 ;
      }
   } ;
   CMDL_MS_DIAGNOSTIC_POP()

   char *ptr = NULL ;
   // Compile the string into an integer; watch out for -c0xa vs -axc0!
   const type result = local::strtoint(arg, &ptr, is_signed()) ;
   if (ptr == arg || (*ptr && std::any_of(ptr, ptr + strlen(ptr), [](char c) { return !isspace(c) ; })))
   {
      // Invalid integer (or overflow, or a sign where should be unsigned)
      if (result && ptr == arg)
         CMDL_LOG_CMDERROR(cmdline, "'" << arg << "' does not fit into " << type_name() << '.') ;
      else
         CMDL_LOG_CMDERROR(cmdline, "invalid " << type_name() << " value '" << arg << "'.") ;
      return false ;
   }

   this->_value = result ;
   arg = ptr ;

   return true ;
}

/******************************************************************************/
/** Base class for boolean argments
*******************************************************************************/
struct argbool_compiler : CmdArgBoolCompiler {
   argbool_compiler(char optchar,
                    const char * keyword,
                    const char *, // value_name - unused
                    const char * description,
                    unsigned flags) :
      CmdArgBoolCompiler(optchar, keyword, description, flags &~ isLIST)
   {}
} ;

/*******************************************************************************

*******************************************************************************/
template<unsigned ForceFlagsOn = 0, unsigned ForceFlagsOff = 0>
class seq_arg : public CmdArg {
   typedef CmdArg ancestor ;
public:
   seq_arg(char optchar,
           const char *keyword, const char *value_name,
           const char *description,
           unsigned flags,
           separator sep) :
      ancestor(optchar, keyword, value_name, description, force_flags(flags)),
      _separator(sep)
   {}

   seq_arg(char optchar,
           const char *keyword, const char *value_name,
           const char *description,
           unsigned flags) :
      seq_arg(optchar, keyword, value_name, description, flags, separator()) {}

   seq_arg(char optchar,
           const char *keyword, const char *value_name,
           const char *description,
           separator sep) :
      seq_arg(optchar, keyword, value_name, description, CmdArg::isVALREQ, sep) {}

   seq_arg(char optchar,
           const char *keyword, const char *value_name,
           const char *description) :
      seq_arg(optchar, keyword, value_name, description, separator()) {}

   seq_arg(const char *value_name, const char *description, unsigned flags) :
      ancestor(value_name, description, force_flags(CmdArg::isPOS | flags)) {}

   seq_arg(const char *value_name, const char *description) :
      seq_arg(value_name, description, 0) {}

   const separator &sep() const { return _separator ; }

protected:
   static constexpr unsigned force_flags(unsigned flags)
   {
      return (flags | ForceFlagsOn) &~ ForceFlagsOff ;
   }

private:
   separator _separator ;

   virtual bool compile(const char *&arg, CmdLine &cmdline) = 0 ;
} ;

/******************************************************************************/
/** Base class for all sequence arguments

 @param Container Container type, e.g. std::list<int>; there must be S::value_type
        defined and scalar_argument<S::value_type> must constitute a valid class
*******************************************************************************/
template<typename Container>
class list_argument : public seq_arg<CmdArg::isLIST> {
   typedef seq_arg<CmdArg::isLIST> ancestor ;
   typedef item_compiler<typename Container::value_type> compiler_type ;
public:
   typedef rebind_container<Container, typename compiler_type::type> type ;
   typedef typename type::value_type      value_type ;
   typedef typename type::const_iterator  const_iterator ;
   typedef const_iterator                 iterator ;

   /// Derive constructors
   using ancestor::ancestor ;

   type &value() { return _container ; }
   const type &value() const { return _container ; }

   const_iterator begin() const { return value().begin() ; }
   const_iterator end() const { return value().end() ; }

   size_t size() const { return value().size() ; }
   bool empty() const { return value().empty() ; }

protected:
   int operator() (const char *&arg, CmdLine &cmdline) override
   {
      return compile(arg, cmdline) && validate(value(), cmdline) ? 0 : -1 ;
   }

   virtual bool validate(type &, CmdLine &) { return true ; }

   void assign(const type &value) { _container = value ; }

   void reset() override
   {
      using namespace std ;
      ancestor::reset() ;
      _container.clear() ;
   }

   bool compile(const char *&arg, CmdLine &cmdline) override ;

private:
   type           _container ;
   compiler_type  _compiler ;
} ;

template<typename T>
bool list_argument<T>::compile(const char *&arg, CmdLine &cmdline)
{
   if (!arg)
      return true ;  // no value given - nothing to do

   char buf[1024] ;
   std::vector<char> bufvec ;
   for (const char *end = arg + strlen(arg), *finish = arg ; arg != end ; arg = finish)
   {
      const char *current_arg ;

      finish = this->sep().find_in(finish, end) ;
      if (!*finish)
         current_arg = arg ;
      else
      {
         const size_t argsize = finish - arg ;
         if (argsize < sizeof buf)
            current_arg = buf ;
         else
         {
            bufvec.resize(argsize + 1) ;
            current_arg = bufvec.data() ;
         }
         *((char *)memcpy(const_cast<char *>(current_arg), arg, argsize)  + argsize) = 0 ;
         finish += this->sep().size() ;
      }

      if (!_compiler.compile(current_arg, cmdline))
         return false ;
      _container.insert(_container.end(), std::move(_compiler.value())) ;
   }
   return true ;
}

template<typename T>
std::ostream &operator<<(std::ostream &os, const list_argument<T> &v)
{
   const char * const s = v.sep().size() ? v.sep().c_str() : " " ;
   const char *delim = "" ;
   for (const auto &value: v)
   {
      os << delim << value ;
      delim = s ;
   }
   return os ;
}

} // end of namespace cmdl::detail

/*******************************************************************************
 argtype_traits<T> for integral types
*******************************************************************************/
#define CMDL_DEFINE_INTTRAITS(type)                                     \
   template<> struct argtype_traits<type> : detail::argint_traits<type> \
   { using detail::argint_traits<type>::argint_traits ; }

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
 argtype_traits<bool>
*******************************************************************************/
template<> class argtype_traits<bool> : public scalar_argument<bool, detail::argbool_compiler> {
   typedef scalar_argument<bool, detail::argbool_compiler> ancestor ;
protected:
   argtype_traits(bool default_value,
                  char         optchar,
                  const char * keyword,
                  const char * description,
                  unsigned     flags) :
      ancestor(bool(default_value), optchar, keyword, NULL, description, flags),
      _initval(default_value)
   {}

   bool compile(const char *&arg, CmdLine &cmdline) override
   {
      return !detail::argbool_compiler::compile(arg, cmdline, _value, !_initval) ;
   }
private:
   const bool _initval ;
} ;

/*******************************************************************************
  argtype_traits<double>
*******************************************************************************/
template<> struct argtype_traits<double> : scalar_argument<double> {
protected:
   /// Inherit constructors
   using scalar_argument<double>::scalar_argument ;

   bool compile(const char *&arg, CmdLine &cmdline) override
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

/*******************************************************************************
 argtype_traits<char>
*******************************************************************************/
template<> struct argtype_traits<char> : scalar_argument<char, CmdArgCharCompiler> {
protected:
   /// Inherit constructors
   using scalar_argument<char, CmdArgCharCompiler>::scalar_argument ;

   bool compile(const char *&arg, CmdLine &cmdline) override
   {
      return !CmdArgCharCompiler::compile(arg, cmdline, _value) ;
   }
} ;

/*******************************************************************************
 argtype_traits<std::string>
*******************************************************************************/
template<> class argtype_traits<std::string> : public scalar_argument<std::string> {
   typedef scalar_argument<std::string> ancestor ;
public:
   enum : unsigned {
      isLOWER = 0x00010000,
      isUPPER = 0x00020000
   } ;
protected:
   /// Inherit constructors
   using ancestor::ancestor ;

   bool compile(const char *&arg, CmdLine &) override
   {
      if (arg)
      {
         _value = arg ;
         if (syntax() & (isLOWER|isUPPER))
            std::transform(_value.data(), _value.data() + _value.size(), &*_value.begin(),
                           (syntax() & isLOWER) ? tolower : toupper) ;
         arg = nullptr ;
      }
      return true ;
   }

   void reset() override
   {
      ancestor::reset() ;
      if (default_value().empty())
         value().clear() ;
      else
         assign(default_value()) ;
   }
} ;

/*******************************************************************************
 argtype_traits<std::pair>
*******************************************************************************/
template<typename K, typename V>
struct argtype_traits<std::pair<K, V> > : scalar_argument<std::pair<K, V> > {

   typedef scalar_argument<std::pair<K, V> > ancestor ;
   typedef K key_type ;
   typedef V value_type ;
   using typename ancestor::type ;

   const char *valid_delimiters() const { return _valid_delims ; }

   void set_valid_delimiters(const char *d)
   {
      const char * const err =
         !d ? "NULL valid delimiters set" :
         !*d ? "Empty valid delimiters set" :
         strlen(d) >= sizeof _valid_delims ? "Valid delimiters set is too large" :
         nullptr ;

      if (err)
         throw std::invalid_argument(err) ;

      strcpy(_valid_delims, d) ;
   }

protected:
   /// Inherit constructors
   using ancestor::ancestor ;

   bool compile(const char *&arg, CmdLine &cmdline) override
   {
      if (!arg)
         return true ;
      if (!*arg)
      {
         CMDL_LOG_CMDERROR(cmdline, "empty pair argument specified.") ;
         return false ;
      }

      const char *delim = arg + strcspn(arg, _valid_delims) ;
      _delim.first = *delim ? *delim : *_valid_delims ;
      if ((delim != arg &&
           !compile_half(_key_compiler, std::string(arg, delim).c_str(), cmdline, this->_value.first)) ||

          (*delim && *++delim &&
           !compile_half(_value_compiler, delim, cmdline, this->_value.second)))
         return false ;

      arg = NULL ;

      return true ;
   }

   char *valstr(char *buf, size_t size, CmdArg::ValStr what_value) const override
   {
      if ((!buf || !size)
          || (what_value == CmdArg::VALSTR_DEFNOZERO && this->default_value() == type()))
         return NULL ;

      buf_ostream os (buf, size) ;
      const type &ref = (what_value == CmdArg::VALSTR_ARGVAL) ? this->value() : this->default_value() ;
      os << ref.first << _delim.first << ref.second << std::ends ;
      return buf ;
   }

private:
   item_compiler<key_type>   _key_compiler ;
   item_compiler<value_type> _value_compiler ;
   std::pair<char, char>     _delim ;
   char                      _valid_delims[14] = ":=" ;

   template<typename T>
   bool compile_half(item_compiler<T> &compiler, const char *half, CmdLine &cmdline, T &result)
   {
      if (!compiler.compile(half, cmdline) || (half && *half))
      {
         CMDL_LOG_CMDERROR(cmdline, "invalid pair format.") ;
         return false ;
      }
      result = std::move(compiler.value()) ;
      return true ;
   }
} ;

/******************************************************************************
 *
 * Arg<T> specializations
 *
*******************************************************************************/

/******************************************************************************/
/** Arg<T> specialization for boolean arguments.
*******************************************************************************/
template<> struct Arg<bool> : argtype_traits<bool> {

   Arg(bool initval, char optchar, const char *keyword, const char *description, unsigned flags = 0) :
      argtype_traits<bool>(initval, optchar, keyword, description, flags) {}

   Arg(char optchar, const char *keyword, const char *description, unsigned flags = 0) :
      Arg(false, optchar, keyword, description, flags) {}

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
   using ancestor::ancestor ;                                           \
                                                                        \
   Arg &operator=(const typename ancestor::type &value)                 \
   {                                                                    \
      this->assign(value) ;                                             \
      return *this ;                                                    \
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

   ArgEnum(keyed_value default_value, char optchar, const char *keyword, const char *value_name,
           const char *description,
           unsigned    flags = 0) :
      ancestor(default_value.first, optchar, keyword, value_name, description, flags | CmdArg::isVALREQ)
   {
      set_defvalue(default_value, flags & isNOCASE) ;
   }

   ArgEnum(keyed_value default_value, const char *value_name,
           const char *description,
           unsigned    flags = 0) :
      ancestor(default_value.first, value_name, description, flags | CmdArg::isVALREQ)
   {
      set_defvalue(default_value, flags & isNOCASE) ;
   }

   type &value() { return _value ; }
   const type &value() const { return _value ; }

   operator type() const { return value() ; }

   const type &default_value() const { return _default_value ; }

   ArgEnum &append(const std::string &key, const type &val) ;
   ArgEnum &append(const keyed_value &keyval) { return append(keyval.first, keyval.second) ; }

private:
   typedef std::map<std::string, type, std::function<bool(const std::string &, const std::string &)>> valmap_type ;

   type        _default_value ;
   type        _value {} ;
   valmap_type _valmap ;

   void set_defvalue(keyed_value &default_value, bool nocase) ;

protected:
   void reset() override
   {
      ancestor::reset() ;
      type v (default_value()) ;
      using std::swap ;
      swap(_value, v) ;
   }

   bool compile(const char *&arg, CmdLine &cmdl) override ;
} ;

template<typename T>
void ArgEnum<T>::set_defvalue(keyed_value &default_value, bool nocase)
{
   typedef typename valmap_type::key_compare key_compare ;

   _valmap = valmap_type(&default_value, &default_value + 1, !nocase
                         ? key_compare(std::less<std::string>())
                         : key_compare([](const std::string &x, const std::string &y)
                           {
                              return std::lexicographical_compare
                              (x.begin(), x.end(), y.begin(), y.end(),
                               [](char a, char b) { return tolower(a) < tolower(b) ; }) ;
                           })) ;
   _default_value = std::move(default_value.second) ;
}

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

template<typename T>
bool ArgEnum<T>::compile(const char *&arg, CmdLine &cmdl)
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
/** Interval argument: a pair of ordered values of the same type, which specifies
 a closed interval

 The argument is represented as VALUE_FROM..VALUE_TO or VALUE.
 VALUE_FROM..VALUE_TO specifies a closed interval from VALUE_FROM to VALUE_TO
 (e.g. 20..30 specifies an interval from 20 to 30, inclusive),

 Single VALUE is equivalent to VALUE..VALUE

 The separator ".." is default, it is possible to specify an arbitrary separator.
*******************************************************************************/
template<typename T>
class Arg<interval<T> > : public detail::seq_arg<0, CmdArg::isLIST> {
   typedef detail::seq_arg<0, CmdArg::isLIST> ancestor ;
public:
   typedef std::pair<T,T> type ;

   using ancestor::ancestor ;

   type &value() { return _value ; }
   const type &value() const { return _value ; }
   operator type() const { return value() ; }

   const separator &sep() const
   {
      return ancestor::sep().front() ? ancestor::sep() : _default_sep ;
   }

   item_compiler<T> &compiler()
   {
      return const_cast<item_compiler<T> &>(_item_compiler) ;
   }

   friend std::ostream &operator<<(std::ostream &os, const Arg &v)
   {
      return os << v.value() ;
   }

protected:
   int operator() (const char *&arg, CmdLine &cmdline) override
   {
      return compile(arg, cmdline) && validate(value(), cmdline) ? 0 : -1 ;
   }

   virtual bool validate(type &, CmdLine &) { return true ; }

   bool compile(const char *&arg, CmdLine &cmdline) override
   {
      if (!arg)
         return true ;

      char buf[1024] ;
      std::vector<char> bufvec ;

      const char *current_arg = buf ;
      const char * const first_end = this->sep().find_in(arg) ;
      const size_t first_size = first_end - arg ;

      if (first_size >= sizeof buf)
      {
         bufvec.resize(first_size + 1) ;
         current_arg = bufvec.data() ;
      }
      *((char *)memcpy(const_cast<char *>(current_arg), arg, first_size)  + first_size) = 0 ;

      if (!_item_compiler.compile(current_arg, cmdline))
         return false ;

      value().first = std::move(_item_compiler.value()) ;
      current_arg = first_end + this->sep().size() ;

      if (*first_end)
      {
         if (!_item_compiler.compile(current_arg, cmdline))
            return false ;
         value().second = std::move(_item_compiler.value()) ;
      }
      else
         value().second = value().first ;

      arg = nullptr ;
      return true ;
   }

   void reset() override
   {
      ancestor::reset() ;
      _value = {} ;
   }

private:
   type             _value ;
   item_compiler<T> _item_compiler ;

   static const separator _default_sep ;
} ;

template<typename T>
const separator Arg<interval<T> >::_default_sep ("..") ;

/******************************************************************************/
/** Command-line iterator over STL iterator
*******************************************************************************/
template<typename InputIterator>
class ArgIter : public CmdLineArgIter {
public:
   typedef InputIterator iterator ;

   ArgIter() = default ;
   ArgIter(iterator b, iterator e) : _begin(b), _end(e) {}

   const char *operator()()
   {
      const char *result = nullptr ;
      if (_begin != _end)
      {
         result = detail::cstr(*_begin) ;
         ++_begin ;
      }
      return result ;
   }
private:
   iterator _begin = {} ;
   iterator _end = {} ;
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

inline bool any_given(std::initializer_list<const CmdArg *> args)
{
   return std::any_of(args.begin(), args.end(), [](const CmdArg *a) { return a && is_given(*a) ; } ) ;
}

inline bool none_given(std::initializer_list<const CmdArg *> args)
{
   return !any_given(args) ;
}

inline bool all_given(std::initializer_list<const CmdArg *> args)
{
   return std::all_of(args.begin(), args.end(), [](const CmdArg *a) { return a && is_given(*a) ; } ) ;
}

} // end of namespace cmdl

CMDL_MS_DIAGNOSTIC_POP()

#endif /* __CMDLINE_CMDEXT_H */
