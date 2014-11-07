/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_URI_H
#define __PCOMN_URI_H
/*******************************************************************************
 FILE         :   pcomn_uri.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   URI object, URL encode, URL decode, URL query

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   3 Mar 2008
*******************************************************************************/
/** @file
    Objects and functions to manipulate URI.
*******************************************************************************/
#include <pcommon.h>
#include <pcomn_string.h>
#include <pcomn_strslice.h>
#include <pcomn_regex.h>
#include <pcomn_except.h>

#include <map>
#include <iostream>

namespace pcomn {
namespace uri {

/*******************************************************************************
** HTTP request URI kind
*******************************************************************************/
enum UriKind { ABS_URL, ABS_PATH } ;

/*******************************************************************************
** The dictionary of URL query arguments
*******************************************************************************/
class query_dictionary : public std::map<std::string, std::string> {
      typedef std::map<std::string, std::string> ancestor ;
   public:
      using ancestor::insert ;

      query_dictionary() :
         ancestor()
      {}

      template<class InputIterator>
      query_dictionary(InputIterator from, InputIterator to) :
         ancestor(from, to)
      {}

      bool has_key(const std::string &header) const
      {
         return !!count(header) ;
      }
      /// Insert a new key/value pair.
      /// Does @em not replace existing key.
      /// @return true, if there if were was no @a key in the dictionary (new key
      /// added); false, if @a key is already present.
      bool insert(const std::string &key, const std::string &value)
      {
         return ancestor::insert(value_type(key, value)).second ;
      }

      /// Insert a new key/value pair, where value is integer.
      /// Does @em not replace existing key.
      /// @return true, if there if were was no @a key in the dictionary (new key
      /// added); false, if @a key is already present.
      bool insert(const std::string &key, int value)
      {
         return insert(key, pcomn::strprintf("%d", value)) ;
      }

      bool find(const std::string &key, std::string &value) const
      {
         if (const std::string * const found = find(key))
         {
            value = *found ;
            return true ;
         }
         return false ;
      }

      bool find(const key_type &key, unsigned &value) const
      {
         if (const std::string * const found = find(key))
         {
            value = atoi(found->c_str()) ;
            return true ;
         }
         return false ;
      }

      const std::string *find(const std::string &key) const
      {
         const const_iterator found (ancestor::find(key)) ;
         return found == end() ? NULL : &found->second ;
      }

      const std::string &get(const std::string &key, const std::string &defval) const
      {
         const std::string * const found = find(key) ;
         return found ? *found : defval ;
      }

      const std::string &get(const std::string &key) const
      {
         return get(key, pcomn::emptystr<std::string>::value) ;
      }
} ;

/// Convert a string into the url-encoded format
/// @param begin Start of a string to encode.
/// @param end   End of of a string to encode.
/// @ingroup URIManip
_PCOMNEXP std::string urlencode(const char *begin, const char *end) ;

/// @ingroup URIManip
template<typename S>
inline typename enable_if_strchar<S, char, std::string>::type
urlencode(const S &str)
{
   const char * const begin = str::cstr(str) ;
   return urlencode(begin, begin + str::len(str)) ;
}

/// @ingroup URIManip
inline std::string urlencode(const strslice &s)
{
   return urlencode(s.begin(), s.end()) ;
}

/// Convert a url-encoded string into normal string.
/// @param begin Start of a string to decode.
/// @param end   End of of a string to decode.
/// @ingroup URIManip
_PCOMNEXP std::string urldecode(const char *begin, const char *end) ;

/// @ingroup URIManip
template<typename S>
inline typename enable_if_strchar<S, char, std::string>::type
urldecode(const S &urlencoded_string)
{
   const char * const begin = str::cstr(urlencoded_string) ;
   return urldecode(begin, begin + str::len(urlencoded_string)) ;
}

/// @ingroup URIManip
inline std::string urldecode(const strslice &s)
{
   return urldecode(s.begin(), s.end()) ;
}

/// Convert a dictionary of key-value pair into URL-encoded query string,
///
/// @return URL-encoded query string; e.g., { "key1":"interesting string",
/// "key2":"string + &" } will be converted into
/// "key1=interesting+string&key2=string+%2B+%26".
///
/// @param dictionary Source dictionary.
/// @ingroup URIManip
_PCOMNEXP std::string query_encode(const query_dictionary &dictionary) ;

/// Convert a query string like "key1=interesting+string&key2=string+%2B+%26" into a
/// dictionary.
/// @param begin_query  Start of an URL-query string.
/// @param end_query    Past-the-end of an URL-query string.
/// @param result       The target dictionary.
///
/// @return The part of string which cannot be converted. I.e. if request is completely
/// correct, the return value is empty.
///
/// @note The @a result is @em not cleared before conversion, so dictionary entries that
/// are not  present in the query remain untouched.
/// @ingroup URIManip
_PCOMNEXP std::string query_decode(const char *begin_query, const char *end_query, query_dictionary &result) ;

template<typename S>
inline std::string query_decode(const S &query_string,
                                typename enable_if_strchar<S, char, query_dictionary>::type  &result)
{
   const char * const begin = str::cstr(query_string) ;
   return query_decode(begin, begin + str::len(query_string), result) ;
}

/******************************************************************************/
/** URL parser
*******************************************************************************/
class _PCOMNEXP URLParser {
   public:
      /// Create empty URLParser
      URLParser() { reset() ; }
      /// Parse an URL; note this constructor defines implicit conversion from any
      /// string.
      URLParser(const strslice &url) { parse(url) ; }

      void parse(const strslice &url) ;

      const reg_match &scheme() const { return _scheme ; }

      /// Get the path part of the URL.
      ///
      /// A valid URL @em always has this part, so the URL with empty path() is
      /// considered illegal, thus operator bool() for such URL returns false.
      const reg_match &path() const { return _path ; }
      const reg_match &query() const { return _query ; }
      // Hostinfo part
      const reg_match &user() const { return _user ; }
      const reg_match &password() const { return _password ; }
      const reg_match &host() const { return _host ; }
      unsigned port() const { return _port ; }

      /// Test URL for validity.
      ///
      /// An URL considered invalid when both path and host are empty.
      bool is_valid() const { return !PSUBEXP_EMPTY(_path) || !PSUBEXP_EMPTY(_host) ; }

      void reset()
      {
         PSUBEXP_RESET(_scheme) ;
         PSUBEXP_RESET(_path) ;
         PSUBEXP_RESET(_query) ;
         PSUBEXP_RESET(_user) ;
         PSUBEXP_RESET(_password) ;
         PSUBEXP_RESET(_host) ;
         _port = 0 ;
      }

      URLParser &set_query(unsigned query_length)
      {
         if (!query_length || !PSUBEXP_MATCHED(_path))
            PSUBEXP_RESET(_query) ;
         else
         {
            _query.rm_so = PSUBEXP_EO(_path) + 1 ;
            _query.rm_len = query_length ;
         }
         return *this ;
      }

      /// Test URL for validity.
      ///
      /// Provides "boolean conversion" for objects of URLParser class, allowing to check
      /// an URLParser object through, e.g., if (URLParser)...
      explicit operator bool() const { return is_valid() ; }

   private:
      // The main part
      reg_match  _scheme ;
      reg_match  _path ;
      reg_match  _query ;

      // Hostinfo part
      reg_match  _user ;
      reg_match  _password ;
      reg_match  _host ;
      unsigned    _port ;
} ;

/*******************************************************************************
 make_url
*******************************************************************************/
_PCOMNEXP std::string make_url(const strslice &scheme, const strslice &path,
                               const strslice &host, unsigned port, const strslice &query,
                               UriKind uri_kind) ;

inline std::string make_url(const strslice &scheme, const strslice &host, const strslice &path)
{
   return make_url(scheme, path, host, 0, strslice(), ABS_URL) ;
}

inline std::string make_url(const strslice &scheme, const strslice &host, unsigned port, const strslice &path)
{
   return make_url(scheme, path, host, port, strslice(), ABS_URL) ;
}

/******************************************************************************/
/** URL template class.
*******************************************************************************/
template<typename S>
class URL {
   public:
      typedef S string_type ;

      URL() : _str() {}
      URL(const string_type &u) : _str(u) { parse() ; }


      URL(const string_type &url, const query_dictionary &uquery) : _str(url)
      {
         parse() ;
         subst_query(uquery) ;
      }

      URL(const URL &url, const query_dictionary &query_dict) :
         _str(url._str),
         _parser(url._parser)
      {
         subst_query(query_dict) ;
      }

      URL(const strslice &scheme, const strslice &host, const strslice &path) :
         _str(make_url(scheme, host, path))
      {
         parse() ;
      }

      URL(const strslice &scheme, const strslice &host, unsigned port, const strslice &path) :
         _str(make_url(scheme, host, port, path))
      {
         parse() ;
      }

      URL(const strslice &scheme, const strslice &uhost, unsigned uport, const strslice &upath,
          const query_dictionary &uquery) :
         _str(make_url(scheme, upath, uhost, uport, query_encode(uquery), ABS_URL))
      {
         parse() ;
      }

      const URLParser &parser() const { return _parser ; }

      /// Test URL for validity; considered invalid when both path and host are empty.
      bool is_valid() const { return parser().is_valid() ; }

      /// Test URL for validity; provides "boolean conversion" to check like if(url)...
      explicit operator bool() const { return !!parser() ; }

      /// Get the original string representation of the URL.
      const string_type &str() const { return _str ; }

      /// Scheme part of the URL.
      strslice scheme() const { return strslice(str(), parser().scheme()) ; }
      /// Path part of the URL.
      strslice path() const { return strslice(str(), parser().path()) ; }
      /// Query part of the URL.
      strslice query() const { return strslice(str(), parser().query()) ; }

      // Hostinfo part
      strslice user() const { return strslice(str(), parser().user()) ; }
      strslice password() const { return strslice(str(), parser().password()) ; }
      strslice host() const { return strslice(str(), parser().host()) ; }
      unsigned port() const { return parser().port() ; }

      query_dictionary &query(query_dictionary &result) const
      {
         result.clear() ;
         const strslice &q = query() ;
         query_decode(q.begin(), q.end(), result) ;
         return result ;
      }

      /// Get the canonicalized "unparsed" URL string representaion, either as an
      /// absolute URL (with hostinfo) or as a relative URL (w/o hostinfo).
      //
      /// Even with @a kind==ABS_URL this may result in a slightly different but
      /// equivalent URL, if the original URL had redundant delimiters, like a '?'
      /// with an empty query (RFC states such URLs @em are equivalent).
      std::string str(UriKind uri_kind) const
      {
         return make_url(scheme(), path(), host(), port(), query(), uri_kind) ;
      }

      std::string canonical() const { return str(ABS_URL) ; }

   private:
      string_type _str ;
      URLParser   _parser ;

      void parse() { _parser.parse(_str) ; }
      void reset() { _parser.reset() ; }
      void subst_query(const query_dictionary &uri_query)
      {
         if (!is_valid())
            return ;
         const std::string &qe = query_encode(uri_query) ;
         // Remove the old query part
         _str.resize(PSUBEXP_EO(_parser.path())) ;
         if (!qe.empty())
            _str.append(1U, '?').append(qe) ;
         _parser.set_query(qe.size()) ;
      }

      void parse_with_query(const query_dictionary &uquery)
      {
         parse() ;
         subst_query(uquery) ;
      }
} ;

/*******************************************************************************
 parse_url
*******************************************************************************/
inline URL<strslice> parse_url(const strslice &url) { return URL<strslice>(url) ; }

/*******************************************************************************
 Debug output
*******************************************************************************/
template<typename S>
inline std::ostream &operator<<(std::ostream &os, const URL<S> &v)
{
    return os << v.str() ;
}

/// Backward compatibility
typedef URL<std::string> URI ;

} // end of namespace pcomn::uri
} // end of namespace pcomn

#endif /* __PCOMN_URI_H */
