/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_REGEX_H
#define __PCOMN_REGEX_H
/*******************************************************************************
 FILE         :   pcomn_regex.h
 COPYRIGHT    :   Yakov Markovitch, 1997-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Class implementing regular expression search.
                  Based on Henry Spencer regular expressions implementation.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   3 Dec 1997
*******************************************************************************/
/** @file
 Classes implementing regular expression and wildcards search, based on Henry Spencer
 regular expressions implementation.

 Until std::regex becomes a commonplace, this file provides a good "least common
 denominator" of regular expression. There is a pcomn::regex class, which is a wrapper
 around POSIX-like regex functions that are slightly modified Henry Spencer's regular
 expressions implementation (1986).
*******************************************************************************/
#include <pcomn_def.h>
#include <pcomn_string.h>
#include <pcomn_strslice.h>
#include <pcomn_iterator.h>
#include <pcomn_smartptr.h>
#include <pcomn_meta.h>

#include <pbregex.h>

#include <stdexcept>

extern "C" _PCOMNEXP int pcomn_xregexec(const pcomn_regex_t *compiled_expression,
                                        const char *begin, const char *end,
                                        size_t subexp_count, reg_match *subexpressions, int cflags) ;

namespace pcomn {

/******************************************************************************/
/** Base regular expression matcher
*******************************************************************************/
class _PCOMNEXP regex_matcher {
   public:
      typedef char char_type ;
      /// Regexp compilation status, returned by PRegError::status().
      typedef PRegError Status ;

      /// Find the first occurence of the regex in a string.
      /// @return The range, corresponding to the matched text.
      template<typename S>
      typename enable_if_strchar<S, char_type, reg_match>::type
      match(const S &string_to_match) const
      {
         reg_match result ;
         pattern().exec_match(str::cstr(string_to_match), NULL, &PSUBEXP_RESET(result), 1) ;
         return result ;
      }
      /// @overload
      reg_match match(const strslice &s) const
      {
         reg_match result ;
         pattern().exec_match(s.begin(), s.end(), &PSUBEXP_RESET(result), 1) ;
         return result ;
      }

      /// Match a string against a regexp.
      /// @param string_to_match   A string to match;
      /// @param begin_subexp      A vector of matched subexpressions. The first item
      /// describes the whole match; all subsequent items corrrespond to
      /// subexpressions in parenthesis. The items corresponding to unmatched
      /// subexpressions are empty (e.g. if the second subexpression didn't match,
      /// then !subx[2] == true).
      /// @param subexp_maxcount   The maximum allowed size of resulting subexpression
      /// vector.
      /// @return The pointer to a @a subx item following the last matched one
      /// (past-the-end iterator a la STL).
      template<typename S>
      typename enable_if_strchar<S, char_type, reg_match *>::type
      match(const S &string_to_match, reg_match *begin_subexp, size_t subexp_maxcount) const
      {
         return pattern().exec_match(str::cstr(string_to_match), NULL, begin_subexp, subexp_maxcount).second ;
      }
      /// @overload
      reg_match *match(const strslice &s, reg_match *begin_subexp, size_t subexp_maxcount) const
      {
         return pattern().exec_match(s.begin(), s.end(), begin_subexp, subexp_maxcount).second ;
      }

      /// @overload
      template<typename S, size_t m>
      typename enable_if_strchar<S, char_type, reg_match *>::type
      match(const S &string_to_match, reg_match (&subexp)[m]) const
      {
         return pattern().exec_match(str::cstr(string_to_match), NULL, subexp, m).second ;
      }
      /// @overload
      template<size_t m>
      reg_match *match(const strslice &s, reg_match (&subexp)[m]) const
      {
         return pattern().exec_match(s.begin(), s.end(), subexp, m).second ;
      }

      /// @overload
      bool is_matched(const strslice &s) const
      {
         return pattern().exec_match(s.begin(), s.end(), NULL, 0).first ;
      }

      /// @overload
      template<typename S>
      typename enable_if_strchar<S, char_type, bool>::type
      is_matched(const S &str, reg_match *begin_subexp, size_t num) const
      {
         return pattern().exec_match(str::cstr(str), NULL, begin_subexp, num).first ;
      }
      /// @overload
      bool is_matched(const strslice &s, reg_match *begin_subexp, size_t num) const
      {
         return pattern().exec_match(s.begin(), s.end(), begin_subexp, num).first ;
      }

      /// @overload
      template<typename S, size_t m>
      typename enable_if_strchar<S, char_type, bool>::type
      is_matched(const S &str, reg_match (&subexp)[m]) const
      {
         return pattern().exec_match(str::cstr(str), NULL, subexp, m).first ;
      }
      /// @overload
      template<size_t m>
      bool is_matched(const strslice &s, reg_match (&subexp)[m]) const
      {
         return pattern().exec_match(s.begin(), s.end(), subexp, m).first ;
      }

      /// Get the index of the last matched subexpression (if there is such).
      /// @return The index of the last matched subexpression. 0 means there is
      /// no subexpressions matched, only the whole expression; -1 means there is
      /// no match at all.
      template<typename S>
      typename enable_if_strchar<S, char_type, int>::type
      last_submatch_ndx(const S &string_to_match) const
      {
         reg_match sexp[MAXNUMEXP] ;
         return int(match(string_to_match, sexp) - sexp - 1) ;
      }

      /// @overload
      int last_submatch_ndx(const strslice &s) const
      {
         reg_match sexp[MAXNUMEXP] ;
         return int(match(s, sexp) - sexp - 1) ;
      }

      /// Check if the character is a regex metachracter
      ///
      /// Metacharacters are '*', '?', '+', '[', ']', '{', '}', '(', ')', '.', '\\', '^', '$'
      static bool ismeta(char c)
      {
         static const uint64_t metamask[] = { 0x80004f1000000000ULL, 0x2800000078000000ULL } ;
         return ((unsigned char)c < 128) && ((1ULL << (c &~ 64)) & metamask[c >> 6]) ;
      }

      friend bool operator==(const regex_matcher &x, const regex_matcher &y) { return &x.pattern() == &y.pattern() ; }
      friend bool operator!=(const regex_matcher &x, const regex_matcher &y) { return !(x == y) ; }

   protected:
      /// Abstract base class for any regular expression pattern
      struct _PCOMNEXP regex_pattern : PRefCount {
            virtual std::pair<bool, reg_match *>
            exec_match(const char_type *begin, const char_type *end,
                       reg_match *begin_subexp, size_t subexp_count) const = 0 ;
      } ;

   protected:
      explicit regex_matcher(regex_pattern *pattern) : _pattern(pattern)
      {
         PCOMN_VERIFY(pattern) ;
      }

      void swap(regex_matcher &other) { other._pattern.swap(_pattern) ; }

      const regex_pattern &pattern() const { return *_pattern ; }

   private:
      shared_intrusive_ptr<regex_pattern> _pattern ;
} ;

/******************************************************************************/
/** The exception to throw by the pcomn::regex constructor when there are errors
 in a regular expression.
*******************************************************************************/
class _PCOMNEXP regex_error : public std::invalid_argument {
      typedef std::invalid_argument ancestor ;
   public:
      ~regex_error() throw() ;

      /// Get error code.
      int code() const { return _code ; }

      /// Get the regular expression, which caused the exception.
      const char *expression() const { return _expression.c_str() ; }

      /// The position (starting with 0) in the regular expression where
      /// error is detected.
      ptrdiff_t position() const { return _pos ; }

   protected:
      /// The constructor.
      /// @param errcode is error status.
      /// @param description is an error descrition (text string).
      /// @param exp is a regular expression, by the compilation of which the error
      ///            is occured (can be NULL).
      /// @param err is an error position inside the regular expression (character offset).
      regex_error(PRegError errcode, const char *description, const char *exp, const char *err) ;

   private:
      const std::string _expression ;
      const PRegError   _code ;
      const ptrdiff_t   _pos ;
      const std::string _invalid ;
} ;

/******************************************************************************/
/** Regular expression class, implements Henry Spencer's regular expressions

 @note Copying/assigning objects of this class is cheap, its actual contents (compiled
 regular expression) is reference-counted.
 @note The maximum number of subexpressions in parenthesis is MAXNUMEXP
*******************************************************************************/
class _PCOMNEXP regex : public regex_matcher {
      typedef regex_matcher ancestor ;
   public:
      regex() : ancestor(create_pattern("")) {}

      /// Compile the regexp from the given string.
      /// @throw regex_error, if there are errors in the regular expression.
      template<typename S>
      regex(const S &str, PCOMN_ENABLE_CTR_IF_STRCHAR(S, char_type)) :
         ancestor(create_pattern(str::cstr(str)))
      {}

      /// Debugging method, which outputs parsed regular expression in human-readable format
      /// into stdout.
      void dump() const ;

   private:
      class pattern_type ;
      inline const pattern_type &raw_pattern() const ;
      static regex_pattern *create_pattern(const char *) ;

} ;

/******************************************************************************/
/** Simplified regular expression a la shell wildcards.
*******************************************************************************/
class _PCOMNEXP wildcard_matcher {
   public:
      typedef char char_type ;

      wildcard_matcher() :
         _regexp(P_CSTR(char_type, "^$"))
      {}

      template<typename S>
      wildcard_matcher(const S &pattern,
                       typename enable_if_strchar<S, char_type, bool>::type unix_neg_charclass = true) :
         _regexp(translate_to_regexp(pcomn::str::cstr(pattern), unix_neg_charclass))
      {}

      template<typename S>
      typename enable_if_strchar<S, char_type, bool>::type
      match(const S &s) const { return _regexp.is_matched(s) ; }

      /// @overload
      bool match(const strslice &s) const { return _regexp.is_matched(s) ; }

   protected:
      static regex translate_to_regexp(const char_type *pattern,
                                       bool unix_neg_charclass) ;

   private:
      regex  _regexp ;
} ;

/*******************************************************************************
 regexp_quote
*******************************************************************************/
/// Create a regexp string which matches the passed literal string and nothing else.
template<typename InputIterator, typename OutputIterator>
inline
OutputIterator regexp_quote(InputIterator begin, InputIterator end, OutputIterator out)
{
   for (; begin != end ; ++begin, ++out)
   {
      const char c = *begin ;
      if (regex_matcher::ismeta(c))
      {
         // Quote a metacharacter
         *out = '\\' ;
         ++out ;
      }
      *out = c ;
   }
   return out ;
}

/// @overload
inline
std::string regexp_quote(const strslice &s)
{
   std::string result ;
   const size_t ssz = s.size() ;
   if (ssz)
   {
      result.reserve(ssz + 1) ;
      regexp_quote(s.begin(), s.end(), pcomn::appender(result)) ;
   }
   return result ;
}

/// @overload
///
/// A special overloaded version for std::string argument. For refcounted std::string
/// implementation (e.g. libstdc++) prevents allocation of new string store if passed
/// argument does not contain regexp metacaracters (and thus a quoted string is the same
/// as a source)
///
template<typename S>
std::enable_if_t<std::is_same<std::remove_cv_t<S>, std::string>::value, std::string>
regexp_quote(const S &s)
{
   const strslice seq (s) ;
   for (const char &c: seq)
      if (regex_matcher::ismeta(c))
      {
         std::string result ;
         result.reserve(seq.size() + 1) ;
         regexp_quote(&c, seq.end(), pcomn::appender(result)) ;
         return result ;
      }
   return s ;
}

/*******************************************************************************
 Substring
*******************************************************************************/
namespace str {
template<class S>
inline std::enable_if_t<string_traits<S>::has_std_read, S>
substr(const S &str, const reg_match &range)
{
   NOXCHECK(!range || PSUBEXP_END(0U, range) <= pcomn::str::len(str)) ;
   return !range
      ? emptystr<S>::value
      : str.substr(PSUBEXP_BEGIN(0, range), PSUBEXP_LENGTH(range)) ;
}
}  // end of namespace pcomn::str

/*******************************************************************************
 Global functions working with pcomn_regex_t
*******************************************************************************/
template<typename S, size_t nsubexps>
inline unsigned regmatch(const pcomn_regex_t *compiled_expression,
                         const S &string_to_match,
                         reg_match (&subexpressions)[nsubexps],
                         int cflags = 0)
{
   return
      pcomn_regexec(compiled_expression, str::cstr(string_to_match), nsubexps, subexpressions, cflags) ;
}

}  // end of namespace pcomn

/*******************************************************************************
 Debug output
*******************************************************************************/
inline std::ostream &operator<<(std::ostream &os, const reg_match &v)
{
   return os << '(' << PSUBEXP_BO(v) << ", " << PSUBEXP_EO(v) << ')' ;
}

#endif /* __PCOMN_REGEX_H */

#ifndef __PCOMN_REGEX_STRSLICE_H
#ifdef  __PCOMN_STRSLICE_H
#define __PCOMN_REGEX_STRSLICE_H

namespace pcomn {

template<typename C>
template<typename S>
inline basic_strslice<C>::basic_strslice(const S &str, typename enable_if_strchar<S, C, const reg_match &>::type range)
{
   if (!PSUBEXP_EMPTY(range))
   {
      NOXCHECK(PSUBEXP_END(0U, range) <= str::len(str)) ;
      const C * const s = str::cstr(str) ;
      _begin = PSUBEXP_BEGIN(s, range) ;
      _end = PSUBEXP_END(s, range) ;
   }
   else
      _end = _begin = nullptr ;
}

template<typename C>
inline basic_strslice<C>::basic_strslice(const basic_strslice &s, const reg_match &range) :
   basic_strslice(s(PSUBEXP_BO(range), PSUBEXP_EO(range)))
{}

}  // end of namespace pcomn

#endif /* __PCOMN_STRSLICE_H */
#endif /* __PCOMN_REGEX_STRSLICE_H */
