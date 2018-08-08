/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_uri.cpp
 COPYRIGHT    :   Yakov Markovitch, 2000-2018. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   URI object, URL encode, URL decode, URL query

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   3 Mar 2008
*******************************************************************************/
#include <pcomn_uri.h>
#include <pcomn_regex.h>
#include <pcomn_except.h>
#include <pcomn_alloca.h>

#include <ctype.h>
#include <stdio.h>

namespace pcomn {
namespace uri {

/*******************************************************************************
 URL & query
*******************************************************************************/
#define hexdigit2num(hdigit) (((uint8_t)(hdigit) >= 'A') ? (((uint8_t)(hdigit) & 0xdf) - 'A') + 10 : ((uint8_t)(hdigit) - '0'))

static const char __c2h[] = "0123456789abcdef" ;

static inline char *c2h(uint8_t what, char *str)
{
   str[0] = '%' ;
   str[1] = __c2h[(what >> 4) & 0xf] ;
   str[2] = __c2h[what & 0xf] ;
   return str + 3 ;
}

static const size_t maxfixsize = 512 ;

std::string urlencode(const char *begin, const char *end)
{
   if (begin == end)
      return std::string() ;
   PCOMN_ENSURE_ARG(begin) ;
   PCOMN_ENSURE_ARG(end) ;

   const size_t length = end - begin ;
   // The length of the result is at most threefold as the source
   // (even if every character must be converted into its hex equivalent)
   // Optimize for "most-cases" to avoid too frequent memory allocations
   P_FAST_BUFFER(allocated, char, 3*length, 3*maxfixsize) ;
   char *begin_result = allocated ;
   char *end_result = begin_result ;
   for ( ; begin != end ; ++begin)
   {
      uint8_t c = *begin ;
      if (isalnum(c))
         *end_result++ = c ;
      else if (c == ' ')
         *end_result++ = '+' ;
      else
         end_result = c2h(c, end_result) ;
   }
   return std::string(begin_result, end_result) ;
}

std::string urldecode(const char *begin, const char *end)
{
   if (begin == end)
      return std::string() ;
   PCOMN_ENSURE_ARG(begin) ;
   PCOMN_ENSURE_ARG(end) ;

   const size_t length = end - begin ;
   // The length of the result is at most equal to the source's
   // Optimize for "most-cases" to avoid too frequent memory allocations
   P_FAST_BUFFER(allocated, char, length, 3*maxfixsize) ;
   char *begin_result = allocated ;
   char *end_result = begin_result ;

   for ( ; begin != end ; ++begin)
      switch(uint8_t c = *begin)
      {
         case '+' :
            *end_result++ = ' ' ;
            break ;

         case '%' :
            // We needn't check length here - if there is an eos at str[1] or str[2],
            // isxdigit() returns false anyway.
            if (isxdigit(begin[1]) && isxdigit(begin[2]))
            {
               const char mshb = *++begin ;
               const char lshb = *++begin ;
               *end_result++ = hexdigit2num(mshb) * 16 + hexdigit2num(lshb) ;
               break ;
            }

         default:
            *end_result++ = c ;
      }

   return std::string(begin_result, end_result) ;
}

std::string query_encode(const query_dictionary &query_dict)
{
   const size_t query_argcnt = query_dict.size() ;
   std::string result ;
   if (!query_argcnt)
      return result ;

   result.reserve(query_argcnt * 8) ; // At least 4, "a=b&"
   char delim = 0 ;
   for (query_dictionary::const_iterator iter (query_dict.begin()), end (query_dict.end()) ; iter != end ; ++iter)
   {
      const query_dictionary::value_type &entry = *iter ;
      if (delim)
         result.push_back(delim) ;
      else
         delim = '&' ;
      result
         .append(urlencode(entry.first))
         .append(1, '=')
         .append(urlencode(entry.second)) ;
   }

   return result ;
}

std::string query_decode(const char *begin_query, const char *end_query, query_dictionary &dictionary)
{
   // Either both are NULL or neither is.
   PCOMN_THROW_MSG_IF(!begin_query != !end_query, std::invalid_argument, "One of the query range arguments is NULL") ;

   std::string nondecoded ;

   for (const char *start_item = begin_query, *start_next ; start_item != end_query ; start_item = start_next)
   {
      const char * const end_item = std::find(start_item, end_query, '&') ;
      start_next = end_item == end_query ? end_item : end_item + 1 ;

      const char * const splitter = std::find(start_item, end_item, '=') ;
      if (splitter == start_item || splitter == end_item)
         nondecoded.append(start_item, start_next) ;
      else
         dictionary.insert(query_dictionary::value_type
                           (urldecode(start_item, splitter), urldecode(splitter + 1, end_item))) ;
   }
   return nondecoded ;
}

/*******************************************************************************
 URLParser
*******************************************************************************/
static const char URI_REGEXP[] = "^(([^:/?#]+)://([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?$" ;
static const char HOSTINFO_REGEXP[] = "^(([^:]*)(:(.*))?@)?([^@:]*)(:([0-9]*))?$" ;
static const std::pair<pcomn::regex, pcomn::regex> RX (URI_REGEXP, HOSTINFO_REGEXP) ;

void URLParser::parse(const strslice &url)
{
   reg_match match[10] ;

   if (!RX.first.is_matched(url, match))
   {
      // Bad url
      reset() ;
      return ;
   }

   _scheme = match[2] ;
   _path = match[4] ;
   _query = match[6] ;

   // Parse the hostinfo part (if exists) to extract user, password, host, and port
   const strslice hostinfo (url, match[3]) ;
   const reg_off hostoffs = PSUBEXP_BO(match[3]) ;

   if (!hostinfo || !RX.second.is_matched(hostinfo, match))
   {
      // Bad hostinfo
      PSUBEXP_RESET(_user) ;
      PSUBEXP_RESET(_password) ;
      PSUBEXP_RESET(_host) ;
      return ;
   }

   /* $      12      3 4        5       6 7            */
   /*      "^(([^:]*)(:(.*))?@)?([^@:]*)(:([0-9]*))?$" */
   /*         ^^user^ :pw        ^host^ ^:[port]^      */

   _user = PSUBEXP_OFFS(match[2], hostoffs) ;
   _password = PSUBEXP_OFFS(match[3], hostoffs) ;
   _host = PSUBEXP_OFFS(match[5], hostoffs) ;

   if (PSUBEXP_LENGTH(match[7]))
   {
      char buf[64] ;
      _port = atoi(strslicecpy(buf, strslice(hostinfo, match[7]))) ;
   }
}

/*******************************************************************************
 make_url
*******************************************************************************/
_PCOMNEXP std::string make_url(const strslice &scheme, const strslice &path,
                               const strslice &host, unsigned port, const strslice &query,
                               UriKind uri_kind)
{
   if (!path && !host)
      return std::string() ;

   // Replace all backslashes ('\') with forward slashes
   std::string urlpath ;
   urlpath.reserve(path.size() + 1) ;
   if (!path || path.front() != '/' && path.front() != '\\')
      urlpath.push_back('/') ;

   urlpath.append(path.begin(), path.end()) ;
   std::replace(urlpath.begin(), urlpath.end(), '\\', '/') ;

   std::string result ;
   if (uri_kind == ABS_PATH)
      result.swap(urlpath) ;
   else
   {
      result
         .append(scheme.begin(), scheme.end())
         .append("://").append(host.begin(), host.end()) ;

      if (port)
      {
         char portbuf[32] ;
         snprintf(portbuf, sizeof portbuf, ":%u", port) ;
         result.append(portbuf) ;
      }
      result.append(urlpath) ;
   }

   if (query)
      result.append(1U, '?').append(query.begin(), query.end()) ;

   return result ;
}

} // end of namespace pcomn::uri
} // end of namespace pcomn
