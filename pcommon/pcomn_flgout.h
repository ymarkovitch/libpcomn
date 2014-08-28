/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_FLGOUT_H
#define __PCOMN_FLGOUT_H
/*******************************************************************************
 FILE         :   pcomn_flgout.h
 COPYRIGHT    :   Yakov Markovitch, 1999-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Manipulators and traits for human-readable stream output of enum values
                  and bit flags.

 CREATION DATE:   2 Aug 1999
*******************************************************************************/
/** @file
 Manipulators and traits for human-readable stream output of enum values and bit flags
*******************************************************************************/
#include <pcomn_omanip.h>
#include <pcomn_macros.h>
#include <pcommon.h>

#include <iostream>
#include <iomanip>
#include <utility>

#define PCOMN_FLGOUT_DESC(flag) {(flag), #flag, 0}
#define PCOMN_FLGOUT_DESCF(flag, mask) {(flag), #flag, (mask)}

#define PCOMN_FLGOUT_TEXT(flag, name) {(flag), (name), 0}
#define PCOMN_FLGOUT_TEXTF(flag, mask, name) {(flag), (name), (mask)}

namespace pcomn {

struct flg2txt_s {
   bigflag_t   flag ;
   const char *text ;
   bigflag_t   mask ;
} ;

/******************************************************************************/
/** std::ostream manipulator for debugging bit flags output.
*******************************************************************************/
struct oflags {
      /// Create a manipulator for human-readable output of bit a flag combination.
      /// @param flags  flags to output
      /// @param desc   flagset description
      /// @param delim  delimiter string (between printed flag names); if NULL, a single
      /// space is used
      /// @param  width field width for every printed flag; 0 means the width is variable
      /// to fit the flag's name
      oflags(bigflag_t flags, const pcomn::flg2txt_s *desc, const char *delim, int width = 0) :
         _desc(desc),
         _delim(delim),
         _flags(flags),
         _width(width)
      {}

      /// @overload
      oflags(bigflag_t flags, const pcomn::flg2txt_s *desc, int width = 0) :
         _desc(desc),
         _delim(NULL),
         _flags(flags),
         _width(width)
      {}

      friend std::ostream & operator<<(std::ostream &os, const oflags &manip)
      {
         return manip.out(os) ;
      }

   protected:
      template<typename C>
      std::basic_ostream<C> &out(std::basic_ostream<C> &os) const ;

   private:
      const flg2txt_s * _desc ;
      const char *      _delim ;
      bigflag_t         _flags ;
      int               _width ;
} ;

template<typename C>
std::basic_ostream<C> &oflags::out(std::basic_ostream<C> &os) const
{
   if (!_desc)
      return os ;

   const char * const actual_delim = !_delim ? " " : _delim ;

   for (const flg2txt_s *desc = _desc ; desc->text ; ++desc)
      if (is_flags_equal(_flags, desc->flag,
                         desc->mask ? desc->mask : (desc->flag ? desc->flag : ~desc->flag)))
      {
         if (desc != _desc)
            os << actual_delim ;
         if (_width > 0)
            os << std::setw(_width) ;
         os << desc->text ;
      }

   return os ;
}

/// Backward compatibility name
typedef oflags flgout ;

/******************************************************************************/
/** Names of enum values.
*******************************************************************************/
template<typename Enum, Instantiate = Instance> struct enum_names ;

template<typename Enum>
inline const char *enum_name(Enum value)
{
   return valmap_find_name(enum_names<Enum>::values, value) ;
}

/******************************************************************************/
/** Define enum value-to-name mapping.

 Provides data for pcomn::enum_name() value-to-name mapping function and pcomn::oenum
 output manipulator.

 @code
 namespace bar {
 enum Foo {
   FooNull,
   FooValue1,
   FooValue2
 } ;
 }
 // Note that the mapping defining macros can be used ONLY in the global namespace
 PCOMN_STARTDEF_ENUM_ELEMENTS(bar::Foo)
   PCOMN_ENUM_ELEMENT(bar, FooNull),
   PCOMN_ENUM_ELEMENT(bar, FooValue1),
   PCOMN_ENUM_ELEMENT(bar, FooValue2),
 PCOMN_ENDDEF_ENUM_ELEMENTS

 @endcode

 @note PCOMN_STARTDEF_ENUM_ELEMENTS() opens pcomn namespace; PCOMN_ENDDEF_ENUM_ELEMENTS
 closes the namespace
*******************************************************************************/
#define PCOMN_STARTDEF_ENUM_ELEMENTS(Enum)                              \
   namespace pcomn {                                                    \
   template<Instantiate dummy>                                          \
   struct enum_names< Enum, dummy>                                      \
   { static const std::pair<const char *, Enum> values[] ; } ;          \
   template<Instantiate dummy>                                          \
   const std::pair<const char *, Enum> enum_names< Enum, dummy>::values[] = {

#define PCOMN_ENUM_ELEMENT(ns, value) {#value, ns::value}

#define PCOMN_ENDDEF_ENUM_ELEMENTS {NULL} } ; }

#define PCOMN_DEFINE_ENUM_ELEMENTS(ns, Enum, count, ...)  \
   PCOMN_STARTDEF_ENUM_ELEMENTS(ns::Enum)                 \
   P_APPL1(PCOMN_ENUM_ELEMENT, ns, count, __VA_ARGS__),   \
   {NULL, (ns::Enum)0} } ; }

/******************************************************************************/
/** Enum output std::ostream manipulator.

 Providing there is enum_traits defined, outputs the name of an enum value.
*******************************************************************************/
template<typename Enum>
struct _oenum {
      std::ostream &operator() (std::ostream &os, Enum value) const
      {
         if (const char * const name = enum_name(value))
            return os << name ;
         return os << "<UNKNOWN>(" << (long long)value << ')' ;
      }
} ;

template<typename Enum>
inline omanip<_oenum<Enum>, Enum> oenum(Enum value)
{
   typedef _oenum<Enum> fn_t ;
   return omanip<fn_t, Enum>(fn_t(), value) ;
}

} // end of namespace pcomn

namespace diag { using pcomn::oenum ; }

#endif /* __PFLGOUT_H */
