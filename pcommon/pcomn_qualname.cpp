/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_qualname.cpp
 COPYRIGHT    :   Yakov Markovitch, 1997-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Qualified name implementation

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   8 May 1997
*******************************************************************************/
#include <pcomn_qualname.h>
#include <pcomn_assert.h>

#include <algorithm>

#include <string.h>
#include <ctype.h>

static const char qual_pfx = qualified_name::PfxQual ;
static const char name_pfx = qualified_name::PfxName ;
const char pfx[] = { qual_pfx, name_pfx, 0 } ;

const char * qual_delim  = "::" ;

/*******************************************************************************
 qualified_name
*******************************************************************************/
size_t qualified_name::check_part(const char *beg, int len)
{
   NOXPRECONDITION(beg != NULL && len >= 0) ;
   size_t result = 0 ;
   if (len > 0)
      if (isalpha(*beg) || *beg == '_')
         do {
            --len ;
            ++result ;
            ++beg ;
         }
         while(len && (isalnum(*beg) || *beg == '_')) ;
   return result ;
}

void qualified_name::check_mangled (const char *beg)
{
   NOXPRECONDITION(beg != NULL) ;

   // Let's check whether name is qualified or not
   if (!*beg)
      return ;

   const char *b = beg ;

   switch(*b)
   {
      case qual_pfx : // It's a qualified name or a qualifier, at least
         while (*b && *b != name_pfx)
         {
            size_t l = strcspn (++b, pfx) ;
            if (check_part(b, l) != l)
               return ;
            _ndxes += (unsigned char)(l+1) ;
            b += l ;
         }
         flags(Qualified) ;
         if (!*b) // Is there a name part?
            break ; // No, it's just a qualifier.

      case name_pfx : // It's just a name
         {
            size_t nl = strlen(++b) ;
            if (check_part(b, nl) != nl)
               return ;
            flags(HasNameLevel) ;
            _ndxes += (unsigned char)nl + 1 ;
         }
			break ;

      default:
         return ;
   }

   _name = beg ;
   _namendx = b-beg ;
}


void qualified_name::mangle(const std::string &nm, size_t offs, bigflag_t mode)
{
   _flags = 0 ;

   if (offs < nm.length())
   {
      const char *beg = nm.c_str()+offs ;

      if (mode & AlreadyMangled)
      {
         check_mangled(beg) ;
         return ;
      }

      size_t delim_len = strlen(qual_delim) ;
      size_t src_len = nm.size() ;

      if (flags(Rooted, !strncmp (beg, qual_delim, delim_len)) & Rooted) // Does it begin with delimiter?
      {
         beg += delim_len ;
         src_len -= delim_len ;
      }

      const char *last = beg ;
      size_t namepos = 0 ; // The current name position
      int partlen, nlen ;
      bool was_delim = false ;

      do
      {
         const char *dfound = strstr(last, qual_delim) ; // search next (or first) delimiter
         nlen = dfound ? dfound - last : src_len - (last - beg) ;
         partlen = check_part(last, nlen) ;

         // Does it take the whole string (especially if it have to)?
         if (partlen < nlen && (mode & FullString))
            _name = std::string() ;

         else if (!partlen)
         {
            if (was_delim)
               if (mode & TrailingDelim)
                  pcomn::set_flags_on(mode, (bigflag_t)Qualifier) ;
               else
                  _name = std::string() ;
         }
         else
         {
            // All right
            namepos = _name.length() ;
            _name.append (1, qual_pfx).append (last, partlen) ; // Adding <namespc_pfx>name
            _ndxes += (unsigned char)(partlen+1) ;
            was_delim = dfound != NULL ;

            if (was_delim) // There was a delimiter
               last = dfound + delim_len ;
            else
               last += partlen ;
         }
      } while (partlen && partlen == nlen) ;

      if (_name.empty()) // There was an error during parsing
      {
         _namendx = 0 ;
         _ndxes = _name ;
         _flags = 0 ;
      }
      else
      {
         if (!(mode & Qualifier))
         {
            flags (HasNameLevel) ;
            _name[namepos] = name_pfx ;  // Change last qual_pfx to name_pfx ;
         }
         else
         {
            namepos = _name.length() ;
            _name += name_pfx ;
         }

         flags (Qualified, *_name.c_str() == qual_pfx) ;
         _namendx = namepos+1 ;
      }
   }

}

qualified_name &qualified_name::append (const qualified_name &qn, bool full)
{
   if (!(!*this || !qn))
   {
      _name.resize (_namendx-1) ;	// ќтрезаем им€ и оставл€ем только квалификатор (без хвостового '?')
      _name.append (qn._name, full ? 0 : qn._namendx-1, std::string::npos) ;

      // ≈сли добавление полное (с квалификатором), добавл€ем индексы добавл€емого имени
      if (full)
      {
         _ndxes.resize (level())  ;
         _ndxes.append (qn._ndxes) ;
         _namendx += qn._namendx-1 ;
         (_flags &= ~HasNameLevel) |= qn.flags() & FullyQualified ;
      }
      else
      {
         _ndxes[level()] = qn._ndxes[qn.level()] ;
         pcomn::set_flags(_flags, qn.flags(), (bigflag_t)HasNameLevel) ;
      }
   }
   return *this ;
}

size_t qualified_name::length() const
{
   return !*this ? 0 : _name.size()-1+level()+2*rooted() ;
}

std::string qualified_name::demangle (int begin, int end) const
{
   begin = std::min(begin, (int)_ndxes.length()) ;
   end = std::min(end, (int)_ndxes.length()) ;

   std::string result ;

   if (begin < end)
   {
      const char *beg = _name.c_str()+1 ;
      int i ;

      for(i = 0 ; i < begin ; beg += _ndxes [i++]) ;
      for (!i && rooted() ? (result += qual_delim) : result ; i < end ; beg += _ndxes [i++])
      {
         result.append (beg, _ndxes[i]-1) ;
         if (i < end - 1)
            result += qual_delim ;
      }
   }

   return result ;
}
