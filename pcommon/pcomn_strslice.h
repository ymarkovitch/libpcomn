/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_STRSLICE_H
#define __PCOMN_STRSLICE_H
/*******************************************************************************
 FILE         :   pcomn_strslice.h
 COPYRIGHT    :   Yakov Markovitch, 1997-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Substring class.
                  An object that represents a slice of any string object.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   2 Dec 1997
*******************************************************************************/
/** @file
  basic_strslice<C>
  strslice

  ssafe_strslice
  stdstr
  strnew
  strslicecpy
  memslicemove

  cstrseq_iterator
  cstrseq_keyval_iterator

  eqi
  lti

  to_lower_inplace
  to_upper_inplace

  lstrip
  rstrip
  strip
  strsplit
  strrsplit

  startswith
  endswith

  quote
  squote
  dquote

  is_identifier
*******************************************************************************/
#include <pcomn_string.h>
#include <pcomn_utils.h>
#include <pcomn_hash.h>

#include <iostream>
#include <algorithm>
#include <iterator>
#include <array>

#include <limits.h>

struct reg_match ;

/// Format string for formatted-output of a string slice; requires @em two arguments to
/// formatted output: int size, const char *begin.
/// @note See P_STRSLICEV()
#define P_STRSLICEF "%.*s"
/// Format string for formatted-output of a quoted string slice; see P_STRSLICEF
#define P_STRSLICEQF "'%.*s'"

#define P_STRSLICEDQF "\"%.*s\""

/// Arguments for formatted-output of a string slice
#define P_STRSLICEV(s) ((int)(s).size()), (&*(s).begin())

#define PCOMN_STRSLICEBUF(size, slice)                                  \
   (pcomn::strslicecpy(std::array<decltype(((slice).front())), (size)>().data(), (slice), (size)))

namespace pcomn {

/***************************************************************************//**
 Non-owning reference to a part (range) of a string, "unowning substring".

 Its purpose is to enable handling of read-only substring of a string whose data
 is already owned somewhere.

 This is, in fact, a pair of pointers, designating the start and the end of
 a substring. Such a substring is, generally, not zero-terminated.

 basic_strslice provides a set of constructors from any "string-like" type,
 i.e. from any T for which pcomn::string_traits<T> is defined.

 It has boolean-conversion operator, which returns 'true' for any nonempty slice
 and 'false' for empty one; a nonzero-length slice is considered @em nonempty,
 a zero-length slice is considered @em empty.
*******************************************************************************/
template<typename C>
struct basic_strslice {
      typedef C char_type ;
      typedef const char_type *iterator ;
      typedef const char_type *const_iterator ;
      typedef std::reverse_iterator<iterator> reverse_iterator ;
      typedef reverse_iterator const_reverse_iterator ;

      template<typename S>
      static constexpr bool is_compatible_v = is_strchar<std::remove_cvref_t<S>, C>::value ;
      template<typename S, typename T = void>
      using enable_if_compatible_t = enable_if_strchar_t<std::remove_cvref_t<S>, C, T> ;

      /// Default constructor, creates an empty null slice
      ///
      constexpr basic_strslice() = default ;
      constexpr basic_strslice(const basic_strslice &) = default ;
      constexpr basic_strslice(basic_strslice &&) = default ;

      template<typename O>
      constexpr basic_strslice(const basic_strslice<O> &other) noexcept :
         _begin(other.begin()), _end(other.end())
      {}

      template<typename S, typename = enable_if_compatible_t<S>>
      basic_strslice(const S &s) :
         _begin(str::cstr(s)), _end(_begin + str::len(s))
      {}

      template<typename S,
               typename = std::enable_if_t<!is_compatible_v<S> &&
                                           is_compatible_v<decltype(std::declval<S>().c_str())>>>
      basic_strslice(S &&s) :
         basic_strslice(s.c_str())
      {}

      /// Explicitly specify a constructor for const char* argument, despite the generic
      /// one-parameter constructor handles this case, to prevent taking reference to the
      /// argument, thus allowing to pass declared and initialized but not defined static
      /// const class members of type const char*.
      ///
      basic_strslice(const char_type *s) noexcept :
         _begin(s), _end(_begin + str::len(s))
      {}

      template<typename S>
      basic_strslice(const S &s, size_t from, enable_if_compatible_t<S, size_t> to) noexcept :
         _begin(str::cstr(s)), _end(_begin)
      {
         const size_t len = str::len(s) ;
         _begin += std::min(from, len) ;
         _end += std::min(to, len) ;
      }

      template<typename S>
      basic_strslice(const S &str, enable_if_compatible_t<S, const reg_match &> range) ;

      basic_strslice(const basic_strslice &str, const reg_match &range) ;

      basic_strslice(const char_type *begin, const char_type *end) noexcept :
         _begin(begin), _end(end)
      {
         NOXCHECK(!begin == !end && begin <= end) ;
      }

      basic_strslice &operator=(const basic_strslice &) = default ;
      basic_strslice &operator=(basic_strslice &&) = default ;

      template<typename O>
      basic_strslice &operator=(const basic_strslice<O> &other) noexcept
      {
         _begin = other.begin() ;
         _end = other.end() ;
         return *this ;
      }

      constexpr const char_type *begin() const { return _begin ; }
      constexpr const char_type *end() const { return _end ; }

      char_type front() const { return *begin() ; }
      char_type back() const { NOXPRECONDITION(!empty()) ; return *(end() - 1) ; }

      const_reverse_iterator rbegin() const { return const_reverse_iterator(end()) ; }
      const_reverse_iterator rend() const { return const_reverse_iterator(begin()) ; }

      constexpr const char_type *data() const { return begin() ; }

      char operator[](ptrdiff_t pos) const
      {
         NOXPRECONDITION((size_t)pos < size()) ;
         return *(begin() + pos) ;
      }

      template<class S>
      S string() const { return S(begin(), end()) ; }

      std::basic_string<C> stdstring() const { return {begin(), end()} ; }

      explicit operator std::basic_string<C>() const { return {begin(), end()} ; }

      constexpr size_t size() const { return end() - begin() ; }

      /// Indicate if the slice is empty
      constexpr bool empty() const { return begin() == end() ; }

      /// Indicate if the slice has has both begin() and end() equal to NULL
      ///
      /// Every null slice is empty but not every empty slice is null. Particularly, a
      /// default constructed slice @em is null. A distinction between "simple" empty and
      /// null slices makes sense in e.g. function return values to indicate difference
      /// between e.g. an empty string match and no match at all.
      ///
      bool is_null() const { return !((uintptr_t)begin() | (uintptr_t)end()) ; }

      /// Indicate that the slice is nonempty.
      constexpr explicit operator bool() const { return begin() != end() ; }

      /// Check if all characters in the slice satisfy the property.
      ///
      /// As @a isproperty is usually used a classifying function from the standard
      /// <cctype> header, e.g. std::isdigit, etc. So, the call is like
      /// s.all<std::isdigit>()
      ///
      /// @return true if all the characters in the slice satisfy @a isproperty @em or
      /// the slice is empty.
      ///
      template<int(isproperty)(int)>
      bool all() const { return std::all_of(begin(), end(), isproperty) ; }
      /// @overload
      template<bool(isproperty)(int)>
      bool all() const { return std::all_of(begin(), end(), isproperty) ; }

      /// Check if none of the characters in the slice satisfy the property.
      ///
      /// @return true if none the characters in the slice satisfy @a isproperty @em or
      /// the slice is empty.
      ///
      template<int(isproperty)(int)>
      bool none() const { return std::none_of(begin(), end(), isproperty) ; }
      /// @overload
      template<bool(isproperty)(int)>
      bool none() const { return std::none_of(begin(), end(), isproperty) ; }

      /// Check if there is at least one characters in the slice satisfying
      /// the property.
      ///
      /// @return true if at least one characters in the slice satisfy @a isproperty
      /// and false if not @em or the slice is empty.
      ///
      template<int(isproperty)(int)>
      bool any() const { return std::any_of(begin(), end(), isproperty) ; }
      /// @overload
      template<bool(isproperty)(int)>
      bool any() const { return std::any_of(begin(), end(), isproperty) ; }

      /// Compare slices lexicografically, like strcmp.
      /// @return 0 for equal, -1 for less, 1 for greater.
      ///
      int compare(const basic_strslice &other) const
      {
         if (other.begin() == begin() && other.end() == end())
            return 0 ;

         typedef const typename ctype_traits<C>::uchar_type * puchr ;

         return lexicographical_compare_3way
            ((puchr)begin(), (puchr)end(),
             (puchr)other.begin(), (puchr)other.end()) ;
      }

      basic_strslice operator()(ptrdiff_t from, ptrdiff_t to) const
      {
         const ptrdiff_t sz = size() ;
         from = std::min(sz, std::max((ptrdiff_t)0, from < 0 ? from + sz : from)) ;
         to = std::min(sz, std::max((ptrdiff_t)0, to < 0 ? to + sz : to)) ;
         return from >= to
            ? basic_strslice()
            : basic_strslice(_begin + from, _begin + to) ;
      }

      basic_strslice operator()(ptrdiff_t from) const
      {
         const ptrdiff_t sz = size() ;
         from = std::min(sz, std::max((ptrdiff_t)0, from < 0 ? from + sz : from)) ;
         return basic_strslice(_begin + from, _end) ;
      }

      bool startswith(const basic_strslice &rhs) const
      {
         return size() >= rhs.size() && std::equal(rhs.begin(), rhs.end(), begin()) ;
      }

      bool endswith(const basic_strslice &rhs) const
      {
         const size_t rsz = rhs.size() ;
         return size() >= rsz && std::equal(rhs.begin(), rhs.end(), end() - rsz) ;
      }

      basic_strslice &lstrip_inplace(const char_type *chars)
      {
         if (!empty())
            _begin = find_first_not_of(begin(), end(), chars, chars + str::len(chars)) ;
         return *this ;
      }

      basic_strslice &lstrip_inplace()
      {
         return lstrip_inplace(str::detail::ws<char_type>::spaces()) ;
      }

      basic_strslice &rstrip_inplace(const char_type *chars)
      {
         if (!empty())
            _end = find_first_not_of(rbegin(), rend(), chars, chars + str::len(chars)).base() ;
         return *this ;
      }

      basic_strslice &rstrip_inplace()
      {
         return rstrip_inplace(str::detail::ws<char_type>::spaces()) ;
      }

      basic_strslice &strip_inplace(const char_type *chars)
      {
         return lstrip_inplace(chars).rstrip_inplace(chars) ;
      }

      basic_strslice &strip_inplace() { return lstrip_inplace().rstrip_inplace() ; }

   private:
      const char_type *_begin = nullptr ;
      const char_type *_end = nullptr ;
} ;

/*******************************************************************************
 Typedefs for narrow and wide chars
*******************************************************************************/
typedef basic_strslice<char>     strslice ;
typedef basic_strslice<wchar_t>  wstrslice ;

/***************************************************************************//**
 basic_strslice equality compare and weak ordering
*******************************************************************************/
template<typename C>
inline bool operator==(const basic_strslice<C> &lhs, const basic_strslice<C> &rhs)
{
   const size_t lsz = lhs.size() ;
   const size_t rsz = rhs.size() ;
   return lsz == rsz && !memcmp(lhs.begin(), rhs.begin(), lsz) ;
}

/// basic_strslice comparison treates character as unsigned
template<typename C>
inline bool operator<(const basic_strslice<C> &lhs, const basic_strslice<C> &rhs)
{
   typedef const typename ctype_traits<C>::uchar_type * puchr ;
   return
      std::lexicographical_compare((puchr)lhs.begin(), (puchr)lhs.end(),
                                   (puchr)rhs.begin(), (puchr)rhs.end()) ;
}

// Define "derived" operators: !=, >, <=, >=
PCOMN_DEFINE_RELOP_FUNCTIONS(template<typename C>, basic_strslice<C>) ;

/// Case-insensitive equality compare
template<typename C>
inline bool eqi(const basic_strslice<C> &lhs, const basic_strslice<C> &rhs)
{
   return lhs.size() == rhs.size() &&
      (lhs.begin() == rhs.begin() ||
       std::equal(lhs.begin(), lhs.end(), rhs.begin(),
                  [](C a, C b){ return ctype_traits<C>::tolower(a) == ctype_traits<C>::tolower(b) ; })) ;
}

template<typename C>
inline bool lti(const basic_strslice<C> &lhs, const basic_strslice<C> &rhs)
{
   typedef ctype_traits<C> char_traits ;
   typedef typename ctype_traits<C>::uchar_type uchar_type ;
   return
      (lhs.begin() != rhs.begin() || lhs.end() != rhs.end()) &&
      std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
                                   [](C a, C b)
                                   {
                                      return
                                         (uchar_type)char_traits::tolower(a) <
                                         (uchar_type)char_traits::tolower(b) ;
                                   }) ;
}

/*******************************************************************************
 basic_strslice vs string equality compare and weak ordering
*******************************************************************************/
template<typename C, typename S>
inline enable_if_strchar_t<S, C, bool>
operator==(const basic_strslice<C> &lhs, const S &rhs) { return lhs == basic_strslice<C>(rhs) ; }

template<typename C, typename S>
inline enable_if_strchar_t<S, C, bool>
operator==(const S &lhs, const basic_strslice<C> &rhs) { return rhs == lhs ; }

template<typename C, typename S>
inline enable_if_strchar_t<S, C, bool>
operator<(const basic_strslice<C> &lhs, const S &rhs) { return lhs < basic_strslice<C>(rhs) ; }

template<typename C, typename S>
inline enable_if_strchar_t<S, C, bool>
operator<(const S &lhs, const basic_strslice<C> &rhs) { return basic_strslice<C>(lhs) < rhs ; }

#define PCOMN_STRSLICE_RELOP_PREFIX                      \
   template<typename C, typename S>                      \
   inline enable_if_strchar_t<S, C, bool>

#define PCOMN_DEFINE_STRSLICE_RELOP_FUNCTIONS(ltype, rtype)             \
   PCOMN_STRSLICE_RELOP_PREFIX operator!=(const ltype &lhs, const rtype &rhs) { return !(lhs == rhs) ; } \
   PCOMN_STRSLICE_RELOP_PREFIX operator> (const ltype &lhs, const rtype &rhs) { return rhs < lhs ; } \
   PCOMN_STRSLICE_RELOP_PREFIX operator<=(const ltype &lhs, const rtype &rhs) { return !(rhs < lhs) ; } \
   PCOMN_STRSLICE_RELOP_PREFIX operator>=(const ltype &lhs, const rtype &rhs) { return !(lhs < rhs) ; }

PCOMN_DEFINE_STRSLICE_RELOP_FUNCTIONS(S, basic_strslice<C>) ;
PCOMN_DEFINE_STRSLICE_RELOP_FUNCTIONS(basic_strslice<C>, S) ;

#undef PCOMN_DEFINE_STRSLICE_RELOP_FUNCTIONS
#undef PCOMN_STRSLICE_RELOP_PREFIX

template<typename C, typename S>
inline enable_if_strchar_t<S, C, bool>
eqi(const basic_strslice<C> &lhs, const S &rhs) { return eqi(lhs, basic_strslice<C>(rhs)) ; }

template<typename C, typename S>
inline enable_if_strchar_t<S, C, bool>
eqi(const S &lhs, const basic_strslice<C> &rhs) { return eqi(rhs, lhs) ; }

template<typename S1, typename S2>
inline typename enable_if_compatible_strings<S1, S2, bool>::type
eqi(const S1 &lhs, const S2 &rhs)
{
   typedef basic_strslice<typename string_traits<S1>::char_type> strslice_type ;
   return eqi(strslice_type(lhs), strslice_type(rhs)) ;
}

template<typename C, typename S>
inline enable_if_strchar_t<S, C, bool>
lti(const basic_strslice<C> &lhs, const S &rhs) { return lti(lhs, basic_strslice<C>(rhs)) ; }

template<typename C, typename S>
inline enable_if_strchar_t<S, C, bool>
lti(const S &lhs, const basic_strslice<C> &rhs) { return lti(basic_strslice<C>(lhs), rhs) ; }

template<typename S1, typename S2>
inline typename enable_if_compatible_strings<S1, S2, bool>::type
lti(const S1 &lhs, const S2 &rhs)
{
   typedef basic_strslice<typename string_traits<S1>::char_type> strslice_type ;
   return lti(strslice_type(lhs), strslice_type(rhs)) ;
}

/***************************************************************************//**
 basic_strslice and std::string concatenation.
*******************************************************************************/
/**@{*/
inline std::string operator+(const std::string &x, const strslice &y)
{
   std::string result (x) ;
   result.append(y.begin(), y.end()) ;
   return result ;
}

inline std::string operator+(const strslice &x, const std::string &y)
{
   std::string result (x.begin(), x.end()) ;
   result.append(y) ;
   return result ;
}

inline std::string &&operator+(std::string &&x, const strslice &y)
{
   x.append(y.begin(), y.end()) ;
   return std::move(x) ;
}

inline std::string &&operator+(const strslice &x, std::string &&y)
{
   y.insert(0, x.begin(), x.size()) ;
   return std::move(y) ;
}
/**@}*/

/*******************************************************************************
 strslice_buffer
*******************************************************************************/
template<size_t threshold>
struct strslice_buffer final {

      explicit strslice_buffer(const strslice &s) : _data(s.size() + 1)
      {
         memmove(_data.get(), s.begin(), s.size()) ;
         *(_data.get() + s.size()) = 0 ;
      }

      const char *c_str() const { return _data.get() ; }

      operator const char *() const { return c_str() ; }

   private:
      auto_buffer<threshold> _data ;
} ;

/***************************************************************************//**
 An iterator over a sequence of strings contained in a character sequence,
 separated with '\0' and terminated by additional '\0' (like e.g. argv)
*******************************************************************************/
template<typename Value = const char *>
class cstrseq_iterator :
   public std::iterator<std::forward_iterator_tag, Value, ptrdiff_t, void, Value> {
      typedef std::iterator<std::forward_iterator_tag, Value, ptrdiff_t, void, Value> ancestor ;

   public:
      typedef typename ancestor::value_type value_type ;

      explicit constexpr cstrseq_iterator(const char *buffer) : _buffer(buffer) {}
      constexpr cstrseq_iterator() : _buffer() {}

      value_type operator*() const { return value_type(_buffer) ; }

      cstrseq_iterator &operator++()
      {
         NOXCHECK(_buffer) ;
         _buffer += strlen(_buffer) + 1 ;
         return *this ;
      }

      PCOMN_DEFINE_POSTCREMENT(cstrseq_iterator, ++)  ;

      friend bool operator==(const cstrseq_iterator &left, const cstrseq_iterator &right)
      {
         if (left._buffer == right._buffer)
            return true ;
         if (!right._buffer)
            return !*left._buffer ;
         if (!left._buffer)
            return !*right._buffer ;
         return false ;
      }

      friend bool operator!=(const cstrseq_iterator &left, const cstrseq_iterator &right)
      {
         return !(left == right) ;
      }

   private:
      const char *_buffer ;
} ;

/***************************************************************************//**
 An iterator over a sequence of "key=value" strings contained in a character
 sequence, separated with '\0' and terminated by additional '\0' (like argv)
*******************************************************************************/
template<typename Key = basic_strslice<char>, typename Value = const char *>
class cstrseq_keyval_iterator :
   public std::iterator<std::forward_iterator_tag, std::pair<Key, Value>, ptrdiff_t, void, std::pair<Key, Value> > {

      typedef std::iterator<std::forward_iterator_tag, std::pair<Key, Value>, ptrdiff_t,
                            void, std::pair<Key, Value> > ancestor ;

   public:
      typedef typename ancestor::value_type value_type ;
      typedef cstrseq_iterator<>            base_iterator ;

      cstrseq_keyval_iterator() {}

      explicit cstrseq_keyval_iterator(const char *buffer) : _base(buffer) {}

      cstrseq_keyval_iterator(const base_iterator &baseiter) : _base(baseiter) {}

      value_type operator*() const
      {
         const char * const begin = *_base ;
         const char * const sep = strchr(begin, '=') ;
         if (!sep)
         {
            const char * const end = begin + strlen(begin) ;
            return value_type(Key(begin, end), Value(end)) ;
         }
         return value_type(Key(begin, sep), Value(sep + 1)) ;
      }

      base_iterator base() const { return _base ; }

      cstrseq_keyval_iterator &operator++()
      {
         ++_base ;
         return *this ;
      }

      PCOMN_DEFINE_POSTCREMENT(cstrseq_keyval_iterator, ++)  ;

      friend bool operator==(const cstrseq_keyval_iterator &left, const cstrseq_keyval_iterator &right)
      {
         return left._base == right._base ;
      }
      friend bool operator!=(const cstrseq_keyval_iterator &left, const cstrseq_keyval_iterator &right)
      {
         return !(left == right) ;
      }

   private:
      base_iterator _base ;
} ;

/*******************************************************************************
 Global functions
*******************************************************************************/
template<typename C>
inline C *strslicecpy(C *dest, const basic_strslice<C> &slice, size_t n)
{
   if (!n)
      return dest ;
   const size_t sz = std::min(slice.size(), n - 1) ;
   memmove(dest, slice.begin(), sz) ;
   dest[sz] = 0 ;
   return dest ;
}

template<typename C, size_t n>
inline C *strslicecpy(C (&dest)[n], const basic_strslice<C> &slice)
{
   return strslicecpy(dest, slice, n) ;
}

template<typename C>
inline C *memslicemove(C *dest, const basic_strslice<C> &slice)
{
   memmove(dest, slice.begin(), slice.size()) ;
   return dest ;
}

template<typename C>
inline basic_strslice<C> &to_lower_inplace(basic_strslice<C> &s)
{
   convert_inplace(s.begin(), ctype_traits<C>::tolower, 0, s.size()) ;
   return s ;
}

template<typename C>
inline basic_strslice<C> &to_upper_inplace(basic_strslice<C> &s)
{
   convert_inplace(s.begin(), ctype_traits<C>::toupper, 0, s.size()) ;
}

/***************************************************************************//**
 Create a slice from a string like basic_strslice constructor does, allow null pointer
 as a source string argument and return empty slice if str::cstr(s) is NULL.
*******************************************************************************/
/**@{*/
template<typename C>
auto inline ssafe_strslice(const C *s) -> std::enable_if_t<is_char<C>::value, basic_strslice<C>>
{
    return s ? basic_strslice<C>(s) : basic_strslice<C>() ;
}

template<typename C, typename S>
inline enable_if_strchar_t<S, C, basic_strslice<C>> ssafe_strslice(const S &s)
{
    return str::cstr(s) ? basic_strslice<C>(s) : basic_strslice<C>() ;
}

template<typename C>
constexpr inline basic_strslice<C> ssafe_strslice(std::nullptr_t) { return {} ; }

/**@}*/
/*******************************************************************************
 Name/value pair functions
*******************************************************************************/
template<typename V, typename N>
const N &valmap_find_name(const std::pair<N, V> *valmap, V &&value)
{
   while(valmap->first && valmap->second != std::forward<V>(value))
      ++valmap ;
   return valmap->first ;
}

template<typename V, typename N>
inline N valmap_find_name(const std::pair<N, V> *valmap, V &&value, const N &default_name)
{
   const N &found = valmap_find_name(valmap, std::forward<V>(value)) ;
   return found ? found : default_name ;
}

/// Implementation of valmap_find_value<> with strslice @a name argument (see
/// valmap_find_value() in pcomn_utils.h)
template<typename C, typename V, typename N>
const V *valmap_find_value(const std::pair<N, V> *valmap, const basic_strslice<C> &name)
{
   if (std::find(name.begin(), name.end(), 0) == name.end())
      // No embedded zeros
      for (const size_t sz = name.size() ; valmap->first ; ++valmap)
         if (!std::char_traits<C>::compare(valmap->first, name.begin(), sz) && !valmap->first[sz])
            return &valmap->second ;

   return nullptr ;
}

template<typename C, typename V, typename N>
inline V valmap_find_value(const std::pair<N, V> *valmap, const basic_strslice<C> &name, const V &default_value)
{
   const V * const found = valmap_find_value(valmap, name) ;
   return found ? *found : default_value ;
}

/*******************************************************************************
 Split string slice into two slices, either from left or from right
*******************************************************************************/
/// Split a string into 2 parts: the part before a separator and the part after it.
///
/// @param s            A string to split.
/// @param separators   The list of possible separator characters.
///
/// @return pair(before_separator,after_separator), e.g. strsplit("a.b.c",".") will
/// return ("a","b.c"); if no separator characters found in @a s, will return (s,"")
///
template<typename C>
inline unipair<basic_strslice<C>>
strsplit(const basic_strslice<C> &s, const basic_strslice<C> &separators)
{
   typedef basic_strslice<C> slice_type ;
   typedef std::pair<slice_type, slice_type> result_type ;

   if (const size_t dsize = separators.size())
   {
      for (const C *i = s.begin(), *e = s.end() ; i != e ; ++i)
         if (memchr(separators.begin(), *i, dsize))
            return result_type(slice_type(s.begin(), i), slice_type(i + 1, s.end())) ;
      return result_type(s, slice_type()) ;
   }
   else
      return result_type(slice_type(), s) ;
}

/// @overload
/// @param s         A string to split.
/// @param separator A delimiting character.
///
template<typename C>
inline unipair<basic_strslice<C>>
strsplit(const basic_strslice<C> &s, C separator)
{
   typedef basic_strslice<C> slice_type ;
   typedef std::pair<slice_type, slice_type> result_type ;

   for (const C *i = s.begin(), *e = s.end() ; i != e ; ++i)
      if (*i == separator)
         return result_type(slice_type(s.begin(), i), slice_type(i + 1, s.end())) ;

   return result_type(s, slice_type()) ;
}

/// @overload
template<typename D>
inline unipair<basic_strslice<typename string_traits<D>::char_type>>
strsplit(const basic_strslice<typename string_traits<D>::char_type> &s, const D &separators)
{
   typedef typename string_traits<D>::char_type char_type ;
   return strsplit(s, basic_strslice<char_type>(separators)) ;
}

/// @overload
template<typename S, typename D>
inline unipair<basic_strslice<typename string_traits<S>::char_type>>
strsplit(const S &s, const D &sep)
{
   typedef typename string_traits<S>::char_type char_type ;
   return strsplit<char_type>(basic_strslice<char_type>(s), sep) ;
}

/// Split a string into 2 parts, the part before a separator and the part after it,
/// starting from the end of the string and working to the front
///
/// @param s            A string to split.
/// @param separators   The list of possible separator characters.
///
/// @return pair(before_separator,after_separator), e.g. strsplit("a.b.c",".") will
/// return ("a.b","c"); if no separator characters found in @a s, will return ("",s)
///
template<typename C>
inline unipair<basic_strslice<C>>
strrsplit(const basic_strslice<C> &s, const basic_strslice<C> &separators)
{
   typedef basic_strslice<C> slice_type ;
   typedef std::pair<slice_type, slice_type> result_type ;

   if (const size_t dsize = separators.size())
   {
      for (const C *i = s.end(), *e = s.begin() ; i != e ;)
         if (memchr(separators.begin(), *--i, dsize))
            return result_type(slice_type(s.begin(), i), slice_type(i + 1, s.end())) ;
      return result_type(slice_type(), s) ;
   }
   else
      return result_type(s, slice_type()) ;
}

/// @overload
/// @param s      A string to split.
/// @param delim  A delimiting character.
///
template<typename C>
inline unipair<basic_strslice<C>>
strrsplit(const basic_strslice<C> &s, C delim)
{
   typedef basic_strslice<C> slice_type ;
   typedef std::pair<slice_type, slice_type> result_type ;

   for (const C *i = s.end(), *e = s.begin() ; i != e ;)
      if (*--i == delim)
         return result_type(slice_type(s.begin(), i), slice_type(i + 1, s.end())) ;

   return result_type(slice_type(), s) ;
}

/// @overload
template<typename D>
inline std::pair<basic_strslice<typename string_traits<D>::char_type>,
                 basic_strslice<typename string_traits<D>::char_type> >
strrsplit(const basic_strslice<typename string_traits<D>::char_type> &s, const D &separators)
{
   typedef typename string_traits<D>::char_type char_type ;
   return strrsplit(s, basic_strslice<char_type>(separators)) ;
}

/// @overload
template<typename S, typename D>
inline std::pair<basic_strslice<typename string_traits<S>::char_type>,
                 basic_strslice<typename string_traits<S>::char_type> >
strrsplit(const S &s, const D &sep)
{
   typedef typename string_traits<S>::char_type char_type ;
   return strrsplit<char_type>(basic_strslice<char_type>(s), sep) ;
}

/*******************************************************************************
 Misc functions
*******************************************************************************/
/// Indicate whether a string slice is a valid C identifier
/// @return true, if @a s is '[A-Za-z_][A-Za-z_0-9]*', false if not or @a s is empty
template<typename C>
inline bool is_identifier(const basic_strslice<C> &s)
{
   typedef ctype_traits<C> cttraits ;

   if (s.empty() || s.front() != '_' && (!isascii(s.front()) || !cttraits::isalpha(s.front())))
      return false ;
   for (const C *i = s.begin() ; ++i != s.end() ; )
      if (!isascii(*i) || !cttraits::isalnum(*i) && *i != '_')
         return false ;
   return true ;
}

/*******************************************************************************
 ostream output
*******************************************************************************/
template<typename C>
inline std::basic_ostream<C> &operator<<(std::basic_ostream<C> &os, const basic_strslice<C> &slice)
{
   os.write(slice.begin(), slice.size()) ;
   return os ;
}

inline std::ostream &operator<<(std::ostream &os, const wstrslice &slice)
{
   return narrow_output(os, slice.begin(), slice.end()) ;
}

template<typename C, typename OutputIterator>
inline OutputIterator escape_string(const basic_strslice<std::remove_cv_t<C>> &s, OutputIterator out, C delim)
{
   return escape_range(s.begin(), s.end(), out, (std::remove_cv_t<C>)delim) ;
}

template<typename S, typename OutputIterator>
inline OutputIterator escape_string(const S &s, OutputIterator out, typename string_traits<S>::char_type delim)
{
   return escape_string(basic_strslice<decltype(delim)>(s), out, delim) ;
}

template<typename C, typename OutputIterator>
inline OutputIterator quote_string(const basic_strslice<std::remove_cv_t<C>> &s, OutputIterator out, C quote)
{
   *out = quote ;
   out = escape_range(s.begin(), s.end(), ++out, (std::remove_cv_t<C>)quote) ;
   *out = quote ;
   return ++out ;
}

template<typename S, typename OutputIterator>
inline OutputIterator quote_string(const S &s, OutputIterator out, typename string_traits<S>::char_type quote)
{
   return quote_string(basic_strslice<decltype(quote)>(s), out, quote) ;
}

namespace detail {
template<typename C>
struct quote_ {
      typedef std::remove_cv_t<C> char_type ;

      constexpr quote_(const basic_strslice<C> &s) : _str(s) {}
      constexpr quote_(const basic_strslice<C> &s, C q) : _str(s), _quote(q ? q : '"') {}

      friend std::ostream &operator<<(std::ostream &os, const quote_&s)
      {
         quote_string(s._str, std::ostream_iterator<char_type>(os), s._quote) ;
         return os ;
      }

   private:
      const basic_strslice<char_type> _str ;
      const char_type                 _quote = '"' ;
} ;
} // end of namespace pcomn::detail

/***************************************************************************//**
 Stream manipulator: outputs a string slice quoted with the specified quote.

 @note While usually is '\'', '"', or '`', _any_ character can be used as a quote.
*******************************************************************************/
/**@{*/
template<typename C>
inline detail::quote_<C> quote(const basic_strslice<C> &s) { return {s} ; }

template<typename C>
inline detail::quote_<C> quote(const basic_strslice<C> &s, C q) { return {s, q} ; }

template<typename S>
inline auto quote(const S &s) -> decltype(detail::quote_<string_char_type_t<S>>(s))
{
   return {s} ;
}

inline detail::quote_<char> quote(const char &c) { return {{&c, &c + 1}, '\''} ; }

template<typename S>
inline detail::quote_<string_char_type_t<S>> quote(const S &s, string_char_type_t<S> q)
{
   return {s, q} ;
}
/**@}*/

/// Output manipulator: allows to insert a single-quoted string into an output stream.
/// Equivalent of C++14 std:quoted(s,'\'','\\') but works with _any_ string type
/// described by pcomn::string_traits as well as with pcomn::strslice.
template<typename S>
inline auto squote(const S &s) -> decltype(quote(s, '\'')) { return {s, '\''} ; }

/// Output manipulator: allows to insert a double-quoted string into an output stream.
/// Equivalent of C++14 std:quoted(s,'"','\\') but works with _any_ string type
/// described by pcomn::string_traits as well as with pcomn::strslice.
template<typename S>
inline auto dquote(const S &s) -> decltype(quote(s, '"'))  { return {s, '"'} ; }

/*******************************************************************************
 omemstream
*******************************************************************************/
inline omemstream::omemstream(const strslice &initstr) : omemstream()
{
   _data = (std::string)initstr ;
   init_buf() ;
}

inline strslice omemstream::str() const noexcept
{
   return {pbase(), pptr()} ;
}

/***************************************************************************//**
 MD5-, SHA-, and T1HA2 hashing strings and string slices
*******************************************************************************/
/**@{*/
template<typename C>
inline md5hash_t md5hash(const basic_strslice<C> &s) { return md5hash(s.begin(), s.size() * sizeof(C)) ; }
template<typename C>
inline md5hash_t sha1hash(const basic_strslice<C> &s) { return sha1hash(s.begin(), s.size() * sizeof(C)) ; }
template<typename C>
inline t1ha2hash_t t1ha2hash(const basic_strslice<C> &s) { return t1ha2hash(s.begin(), s.size() * sizeof(C)) ; }
/**@}*/

/*******************************************************************************
 pcomn::str
*******************************************************************************/
namespace str {
template<typename Char>
inline std::basic_string<Char> stdstr(const basic_strslice<Char> &slice)
{
   return {slice.begin(), slice.end()} ;
}

template<typename Char>
inline Char *strnew(const basic_strslice<Char> &slice)
{
   const size_t sz = slice.size() ;
   Char *result = new Char[sz + 1] ;
   memcpy(result, slice.begin(), sz * sizeof(Char)) ;
   result[sz] = 0 ;
   return result ;
}

template<typename S>
basic_strslice<typename string_traits<S>::char_type>
lstrip(const S &s)
{
   typedef basic_strslice<typename string_traits<S>::char_type> slice ;
   return !cstr(s) ? slice() : slice(s).lstrip_inplace() ;
}

template<typename S>
basic_strslice<typename string_traits<S>::char_type>
lstrip(const S &s, const typename string_traits<S>::char_type *chars)
{
   typedef basic_strslice<typename string_traits<S>::char_type> slice ;
   return !cstr(s) ? slice() : slice(s).lstrip_inplace(chars) ;
}


template<typename S>
basic_strslice<typename string_traits<S>::char_type>
rstrip(const S &s)
{
   typedef basic_strslice<typename string_traits<S>::char_type> slice ;
   return !cstr(s) ? slice() : slice(s).rstrip_inplace() ;
}

template<typename S>
basic_strslice<typename string_traits<S>::char_type>
rstrip(const S &s, const typename string_traits<S>::char_type *chars)
{
   typedef basic_strslice<typename string_traits<S>::char_type> slice ;
   return !cstr(s) ? slice() : slice(s).rstrip_inplace(chars) ;
}

template<typename S>
basic_strslice<typename string_traits<S>::char_type>
strip(const S &s)
{
   typedef basic_strslice<typename string_traits<S>::char_type> slice ;
   return !cstr(s) ? slice() : slice(s).strip_inplace() ;
}

template<typename S>
basic_strslice<typename string_traits<S>::char_type>
strip(const S &s, const typename string_traits<S>::char_type *chars)
{
   typedef basic_strslice<typename string_traits<S>::char_type> slice ;
   return !cstr(s) ? slice() : slice(s).strip_inplace(chars) ;
}

template<typename C>
inline basic_strslice<C> lstrip(const basic_strslice<C> &s)
{
   return basic_strslice<C>(s).lstrip_inplace() ;
}

template<typename C>
inline basic_strslice<C> rstrip(const basic_strslice<C> &s)
{
   return basic_strslice<C>(s).rstrip_inplace() ;
}

template<typename C>
inline basic_strslice<C> strip(const basic_strslice<C> &s)
{
   return basic_strslice<C>(s).strip_inplace() ;
}

inline bool startswith(const pcomn::strslice &lhs, const pcomn::strslice &rhs)
{
   return lhs.startswith(rhs) ;
}

inline bool startswith(const pcomn::wstrslice &lhs, const pcomn::wstrslice &rhs)
{
   return lhs.startswith(rhs) ;
}

inline bool endswith(const pcomn::strslice &lhs, const pcomn::strslice &rhs)
{
   return lhs.endswith(rhs) ;
}

inline bool endswith(const pcomn::wstrslice &lhs, const pcomn::wstrslice &rhs)
{
   return lhs.endswith(rhs) ;
}

} // end of namespace pcomn::str
} // end of namespace pcomn

namespace std {
template<typename T> struct hash ;

/***************************************************************************//**
 std::hash specialization for basic_strslice.
*******************************************************************************/
template<typename C>
struct hash<pcomn::basic_strslice<C>> : pcomn::hash_fn_rawdata<pcomn::basic_strslice<C>> {} ;

} // end of namespace std

#endif /* __PCOMN_STRSLICE_H */
