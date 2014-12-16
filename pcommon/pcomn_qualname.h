/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_QUALNAME_H
#define __PCOMN_QUALNAME_H
/*******************************************************************************
 FILE         :   pcomn_qualname.h
 COPYRIGHT    :   Yakov Markovitch, 1997-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Qualified name class(es)

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   8 May 1997
*******************************************************************************/
#include <pcomn_def.h>
#include <pcommon.h>
#include <pcomn_utils.h>
#include <string>
#include <iostream>
#include <numeric>
#include <limits.h>

/*******************************************************************************
                     class qualified_name

 Represents name of appearance
 qualifier<delim>qualifier<delim>...<delim>qualifier<delim>name
 as
 <\2>qualifier<\2>qualifier<\2>...<\2>qualifier<\1>name
*******************************************************************************/
class _PCOMNEXP qualified_name {

   public:
      /*******************************************************************************
               enum qualified_name::Flags
       Properties of mangled qualified name
      *******************************************************************************/
      enum Flags
      {
         Rooted         = 0x0001,   /* The name is explicitly 'rooted', i.e. begins with delimiter
                                     * (e.g. ::qualifier::name)
                                     */
         Qualified      = 0x0002,   /* The name _is_ qualified (i.e. has at least one qualifier) */
         HasNameLevel   = 0x0004,   /* The name has name level (not just qualifiers) */

         FullyQualified = Qualified | HasNameLevel
      } ;

      /*******************************************************************************
               enum qualified_name::Mode
       Mode flags of a qualified name construction (for giving to constructor).
      *******************************************************************************/
      enum Mode
      {
         TrailingDelim  = 0x0001,   /* The name can end with delimiter - e.g. hello::world::
                                     * In such a case the full name _is_ a merely qualifier,
                                     * so name() returns empty string.
                                     */
         Qualifier      = 0x0002,   /* The full name is explicitly interpreted as qualifier
                                     * (it doesn't depend on trailing delimiter).
                                     */
         AlreadyMangled = 0x0004,   /* The source string is an already mangled name */
         FullString     = 0x0008    /* The name must occupy the whole string (without trailing/leading spaces) */
      } ;

      /*******************************************************************************
               enum qualified_name::ManglePrefix
        The prefixes (delimiters) of a mangled name parts.
      *******************************************************************************/
      enum ManglePrefix
      {
         PfxQual = '\2',
         PfxName = '\1'
      } ;

      // The default constructor.
      // Creates an empty qualified name.
      //
      qualified_name() :
         _namendx (0),
         _flags(0)
      {}

      qualified_name (const std::string &nm, size_t offs = 0, unsigned mode = 0) :
         _namendx (0),
         _flags(0)
      {
         mangle (nm, offs, mode) ;
      }

      qualified_name (const char *nm, size_t offs = 0, unsigned mode = 0) :
         _namendx (0),
         _flags(0)
      {
         mangle (nm, offs, mode) ;
      }

      // qual()   -  return qualifier up to the given level
      //
      std::string qual(bool mangled = false, int lev = -1) const
      {
         if (!_namendx)    // If qualified name isn't incorrect, _name is an empty string
           return _name ;
         if (lev < 0)
            lev = level() ;
         else
            ++lev ;
         return mangled ?
            _name.substr(0, mangled_length(0, lev)) :
            demangle(0, lev) ;
      }

      // name()   -  get the name w/o qualifier
      // e.g. for "hello::my::world" returns "world"; for "hello::" returns ""; for illegal name returns ""
      //
      std::string name() const { return _name.substr(_namendx) ; }

      // fullname()  -  get the full qualified name as a string
      //
      std::string fullname(bool mangle) const { return mangle ? mangled() : fullname() ; }

      // fullname()  -  get the full demangled qualified name as a string
      //
      std::string fullname() const { return demangle() ; }

      const std::string &mangled() const { return _name ; }

      // flags()  -  get the name flags (see enum Flags)
      //
      unsigned flags() const { return _flags ; }

      // operator!()  -  is the name correct?
      // Returns true for empty/illegal qualified names.
      //
      explicit operator bool() const { return !!_namendx ; }

      // qualified()  -  is the name qualified?
      //
      bool qualified() const { return (flags() & Qualified) != 0 ; }

      bool rooted() const { return (flags() & Rooted) != 0 ; }

      friend bool operator<(const qualified_name &qn1, const qualified_name &qn2)
      {
         return qn1._name < qn2._name ;
      }

      friend bool operator==(const qualified_name &qn1, const qualified_name &qn2)
      {
         return qn1._name == qn2._name ;
      }

      // length() -  get the length of string (printable) representation of the qualified name
      //
      size_t length() const ;

      // length() -  get the length of string (printable) representation of the qualified name
      //
      size_t level() const
      {
         return _ndxes.length() - !!(_flags & HasNameLevel) ;
      }

      // demangle()  -  reconstruct qualified name as a string (source form)
      // Parameters:
      //    begin -  the number of quialification level to begin from
      //    end   -  the number of quialification level to end up with
      //
      std::string demangle(int begin = 0, int end = INT_MAX) const ;

      qualified_name &append (const qualified_name &, bool full = true) ;

   protected:
      int mangled_length(int begin, int end) const
      {
         return std::accumulate(_ndxes.c_str() + begin, _ndxes.c_str() + end, 0) ;
      }

      unsigned flags(unsigned f, bool onOff = true)
      {
        return pcomn::set_flags(_flags, f, onOff) ;
      }

   private:
      std::string   _name ;      /* The full mangled name. */
      std::string   _ndxes ;     /* The name components lengths array.
                                  * Every char in this string contains length of the corresponding component.
                                  */
      unsigned      _namendx ; /* The index of name beginning (i.e. position the last component of name begins at) */
      unsigned      _flags ;   /* The name flags (see Flags) */

      void mangle(const std::string &, size_t offs, unsigned mode = 0) ;
      void check_mangled(const char *beg) ;
      size_t check_part(const char *beg, int len) ;
} ;

inline std::ostream &operator << (std::ostream &os, const qualified_name &qn)
{
   return os << qn.fullname() ;
}

#endif /* __PCOMN_QUALNAME_H */
