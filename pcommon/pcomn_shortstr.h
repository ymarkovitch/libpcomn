/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_SHORTSTR_H
#define __PCOMN_SHORTSTR_H
/*******************************************************************************
 FILE         :   pcomn_shortstr.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   The short string template.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   7 May 2008
*******************************************************************************/
#include <pcomn_string.h>
#include <pcomn_except.h>

#include <utility>
#include <iterator>
#include <algorithm>
#include <iostream>

namespace pcomn {

/******************************************************************************/
/** Objects of this class represent strings whose maximal size is specified at
 compile-time and data is contained in empbedded buffer (i.e. an object of this class
 doesn't hold a pointer to string data but string data itself).

 This is similar to Delphi ShortString class. Efficient when the maximum size of a string
 is small and known in advance (typically up to 256 characters). Handy, e.g. for
 returning string values from functions when the maximal size of the returned string is
 restricted by the nature of the task (e.g., converttion of an integer to a string can
 hold only so many characters) and using std::string or any other kind
 dynamically-allocated storage is expensive. From the storage/copying/assignment point of
 view this is essentially very much like std::array<n,char>.
*******************************************************************************/
template<size_t Capacity, typename Char = char>
class short_string {
   public:
      typedef Char                     value_type ;
      typedef Char                     char_type ;
      typedef std::char_traits<Char>   traits_type ;

      typedef size_t            size_type ;
      typedef const char_type * pointer ;
      typedef const char_type * const_pointer ;

      typedef const char_type * iterator ;
      typedef const char_type * const_iterator;
      typedef std::reverse_iterator<const_iterator> reverse_iterator ;
      typedef std::reverse_iterator<const_iterator> const_reverse_iterator ;

      static const size_type npos = static_cast<size_type>(-1) ;
      static const size_t    capacity_value = Capacity ;

   public:
      short_string() { memset(_buf, 0, sizeof _buf) ; }

      short_string(size_type n, char_type c)
      {
         const size_t sz = std::min(n, Capacity) ;
         std::fill_n(c, sz, c) ;
         memset(_buf + sz, 0, sizeof(char_type) * (Capacity + 1 - sz)) ;
      }

      short_string(const short_string &s) { memcpy(_buf, s._buf, sizeof _buf) ; }

      template<typename S, typename = enable_if_strchar_t<S, char_type, nullptr_t>>
      short_string(const S &s)
      {
         *this = s ;
         _buf[Capacity] = 0 ;
      }

      template<class InputIterator>
      short_string(InputIterator b, InputIterator e)
      {

         char_type *p = begin() ;
         char_type *end_storage = p + capacity() ;
         for (; p != end_storage && b != e ; ++b)
            *p++ = *b ;
         *p = *end_storage = 0 ;
      }

      const char_type *c_str() const { return _buf ; }
      const char_type *data() const { return _buf ; }

      const char_type *begin() const { return _buf ; }
      const char_type *end() const { return traits_type::find(_buf, Capacity + 1, 0) ; }
      char_type *begin() { return _buf ; }
      char_type *end() { return const_cast<char_type *>(traits_type::find(_buf, Capacity + 1, 0)) ; }

      const_reverse_iterator rbegin() const { return reverse_iterator(end()) ; }
      const_reverse_iterator rend() const { return reverse_iterator(begin()) ; }

      size_type size() const { return str::len(_buf) ; }
      size_type length() const { return size() ; }
      size_type capacity() const { return Capacity ; }
      bool empty() const { return !*begin() ; }

      const char_type &operator[](size_type pos) const
      {
         NOXCHECK(pos <= size()) ;
         return _buf[pos] ;
      }

      char_type &operator[](size_type pos)
      {
         NOXCHECK(pos <= size()) ;
         return _buf[pos] ;
      }

      char_type at(size_type pos) const
      {
         const size_t sz = size() ;
         PCOMN_THROW_MSG_IF(pos >= sz, std::out_of_range,
                            "Position %u is out of range for small string of size %u.", pos, sz) ;
         return (*this)[pos] ;
      }

      template<typename S>
      enable_if_strchar_t<S, char_type, int>
      compare(const S &s) const
      {
         const size_type s_sz = str::len(s) ;
         const int intermed = traits_type::compare(c_str(), str::cstr(s), s_sz + 1) ;
         return !intermed ? static_cast<int>(size() - s_sz) : intermed ;
      }

      short_string &operator=(const short_string &src)
      {
         memcpy(_buf, src._buf, sizeof _buf) ;
         return *this ;
      }

      short_string &operator=(char_type c)
      {
         (&(*begin() = c) + 1) = 0 ;
         return *this ;
      }

      template<typename S>
      enable_if_strchar_t<S, char_type, short_string &>
      operator=(const S &src)
      {
         ctype_traits<char_type>::strncpy(_buf, str::cstr(src), Capacity) ;
         return *this ;
      }

   private:
      char_type _buf[Capacity + 1] ;
} ;

template<size_t Capacity, typename Char>
const size_t short_string<Capacity, Char>::capacity_value ;

/*******************************************************************************
 Stream output
*******************************************************************************/
template<typename Char, size_t Capacity>
inline std::basic_ostream<Char> &operator<<(std::basic_ostream<Char> &os,
                                            const short_string<Capacity, Char> &str)
{
   return os << str.c_str() ;
}

template<size_t Capacity>
inline std::ostream &operator<<(std::ostream &os, const short_string<Capacity, wchar_t> &str)
{
   const wchar_t *begin = str.c_str() ;
   return narrow_output(os, begin, begin + str.size()) ;
}

/*******************************************************************************
 Formatted output
*******************************************************************************/
template<size_t n>
short_string<n> &bufprintf(short_string<n> &buf, const char *format, ...) PCOMN_ATTR_PRINTF(2, 3) ;

template<size_t n>
inline short_string<n> &bufprintf(short_string<n> &buf, const char *format, ...)
{
   va_list parm ;
   va_start(parm, format) ;
   vsnprintf(buf.begin(), buf.capacity() + 1, format, parm) ;
   va_end(parm) ;

   return buf ;
}

/*******************************************************************************
 String traits for small strings
*******************************************************************************/
template<size_t Capacity, typename Char>
struct string_traits<short_string<Capacity, Char> > :
         anystring_traits<short_string<Capacity, Char>, Char>
{} ;

/*******************************************************************************
 small string comparison
*******************************************************************************/
#define STR_DEFINE_RELOPS(type)       \
DEFINE_RELOP_##type(==)               \
DEFINE_RELOP_##type(!=)               \
DEFINE_RELOP_##type(<)                \
DEFINE_RELOP_##type(<=)               \
DEFINE_RELOP_##type(>)                \
DEFINE_RELOP_##type(>=)

#define DEFINE_RELOP_STR_STR(op)                                  \
template<class Char, size_t N1, size_t N2>                         \
inline bool operator op(const pcomn::short_string<N1, Char> &lhs, \
                        const pcomn::short_string<N2, Char> &rhs) \
{                                                                 \
   return lhs.compare(rhs) op 0 ;                                 \
}

#define DEFINE_RELOP_STR_CSTR(op)                                       \
template<class Str, size_t Capacity, class Char>                        \
inline typename pcomn::enable_if_other_string<pcomn::short_string<Capacity, Char>, Str, bool>::type \
operator op(const pcomn::short_string<Capacity, Char> &lhs, const Str &rhs) \
{                                                                       \
   return lhs.compare(pcomn::str::cstr(rhs)) op 0 ;                     \
}

#define DEFINE_RELOP_CSTR_STR(op)                                       \
template<class Str, size_t Capacity, class Char>                        \
inline typename pcomn::enable_if_other_string<pcomn::short_string<Capacity, Char>, Str, bool>::type \
operator op(const Str &lhs, const pcomn::short_string<Capacity, Char> &rhs) \
{                                                                       \
   return -rhs.compare(pcomn::str::cstr(lhs)) op 0 ;                    \
}

STR_DEFINE_RELOPS(STR_STR)
STR_DEFINE_RELOPS(STR_CSTR)
STR_DEFINE_RELOPS(CSTR_STR)

#undef STR_DEFINE_RELOPS
#undef DEFINE_RELOP_STR_CSTR
#undef DEFINE_RELOP_CSTR_STR
#undef DEFINE_RELOP_STR_STR

/// A debugging function, converts a character to a short string containing its C literal
/// representation, like ' to "'\''", a to "'a'", etc.
inline short_string<7> charrepr(char c)
{
   short_string<7> result ;
   char *p = result.begin() ;
   if (!isascii(c) || iscntrl(c))
      snprintf(p, result.capacity(), "'\\x%2.2X'", (unsigned)(unsigned char)c) ;
   else if (c == '\\' || c == '\'')
   {
      p[3] = p[0] = '\'' ;
      p[1] = '\\' ;
      p[2] = c ;
   }
   else
   {
      p[2] = p[0] = '\'' ;
      p[1] = c ;
   }
   return result ;
}

/*******************************************************************************
 Case conversion
*******************************************************************************/
namespace str {

template<typename Converter, size_t Capacity, typename C>
inline short_string<Capacity, C> &convert_inplace(short_string<Capacity, C> &s, Converter converter,
                                                  size_t offs = 0, size_t size = (size_t)-1)
{
   convert_inplace(s.begin(), offs, size) ;
   return s ;
}

template<size_t Capacity, typename C>
inline short_string<Capacity, C> to_lower(const short_string<Capacity, C> &s)
{
   short_string<Capacity, C> result(s) ;
   return convert_inplace(s, ctype_traits<C>::tolower) ;
}

template<size_t Capacity, typename C>
inline short_string<Capacity, C> to_upper(const short_string<Capacity, C> &s)
{
   short_string<Capacity, C> result(s) ;
   return convert_inplace(s, ctype_traits<C>::toupper) ;
}

} // end of namespace pcomn::str
} // end of namespace pcomn

#endif /* __PCOMN_SHORTSTR_H */
