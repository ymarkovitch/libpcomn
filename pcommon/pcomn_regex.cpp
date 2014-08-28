/*-*- tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets: ((innamespace . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_regex.cpp
 COPYRIGHT    :   Yakov Markovitch, 1997-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Regular expresion class implementation

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   3 Dec 1997
*******************************************************************************/
#include <pcomn_regex.h>
#include <pcomn_smartptr.h>
#include <pcomn_trace.h>
#include <pcomn_utils.h>

#include <stddef.h>
#include <string.h>

#define LITERAL_STR(str) P_CSTR(char_type, str)

/*******************************************************************************
 C functions
*******************************************************************************/
extern "C" char *regmemmove(char *dest, const char *src, const reg_match *match)
{
   const int slen = PSUBEXP_LENGTH(*match) ;

   if (slen > 0)
      memmove(dest, PSUBEXP_BEGIN(src, *match), slen) ;
   return dest ;
}

extern "C" char *regstrcpy(char *dest, const char *src, const reg_match *match)
{
   const int slen = PSUBEXP_LENGTH(*match) ;
   if (slen <= 0 && dest)
      *dest = 0 ;
   else
   {
      memmove(dest, PSUBEXP_BEGIN(src, *match), slen) ;
      // Terminate copied string
      dest[slen] = 0 ;
   }
   return dest ;
}

namespace pcomn {

/*******************************************************************************
 regex_error
*******************************************************************************/
regex_error::regex_error(PRegError errcode, const char *description, const char *exp, const char *pos) :
   ancestor(description ? description : "<unknown> "),
   _expression(exp ? exp : ""),
   _code(errcode),
   _pos(exp && pos && xinrange(pos, exp, exp + _expression.size()) ? pos - exp : 0),
   _invalid(_expression.c_str() + _pos)
{}

regex_error::~regex_error() throw()
{}

/*******************************************************************************
 regex::pattern_type
*******************************************************************************/
class regex::pattern_type : public regex::regex_pattern {
   PCOMN_NONCOPYABLE(pattern_type) ;
   PCOMN_NONASSIGNABLE(pattern_type) ;
public:
   explicit pattern_type(const char *ex)
   {
      memset(&exp, 0, sizeof exp) ;
      pcomn_regcomp_ex(&exp, ex, 0, (regexp_handler)errh) ;
   }

   ~pattern_type() { pcomn_regfree(&exp) ; }

   pcomn_regex_t exp ;
private:
   std::pair<bool, reg_match *>
   exec_match(const char_type *begin, const char_type *end,
              reg_match *begin_subexp, size_t subexp_count) const override ;

   static void errh(PRegError errcode, const char *err, const char *exp, const char *pos) ;
} ;

void regex::pattern_type::errh(PRegError errcode, const char *err, const char *exp, const char *pos)
{
   struct rx_error : regex_error {
      rx_error(Status errcode, const char *descritpion, const char *expression, const char *pos) :
         regex_error(errcode, descritpion, expression, pos)
      {}
   } ;
   throw rx_error(errcode, err, exp, pos) ;
} ;

// The main regex functions, all others work through this
std::pair<bool, reg_match *>
regex::pattern_type::exec_match(const char_type *begin, const char_type *end,
                                reg_match *begin_matched, size_t subexp_count) const
{
   std::pair<bool, reg_match *> result
      (pcomn_xregexec(&exp, begin, end, subexp_count, begin_matched, 0),
       begin_matched) ;

   if (result.first)
      for (result.second = begin_matched + subexp_count ;
           result.second > begin_matched && !PSUBEXP_MATCHED(result.second[-1]) ;
           --result.second) ;

   return result ;
}

/*******************************************************************************
 regex
*******************************************************************************/
inline const regex::pattern_type &regex::raw_pattern() const
{
   return static_cast<const regex::pattern_type &>(pattern()) ;
}

regex::regex_pattern *regex::create_pattern(const char_type *s)
{
   return new pattern_type(s) ;
}

void regex::dump() const { pcomn_regdump(&raw_pattern().exp) ; }

/*******************************************************************************
 wildcard_matcher
*******************************************************************************/
regex wildcard_matcher::translate_to_regexp(const char_type *pattern, bool unix_neg_charclass)
{
   // Translate a shell-like pattern to a regular expression.
   // There is no way to quote meta-characters.
   std::basic_string<char_type> regpat (1, (char_type)'^') ;
   if (pattern)
      while (*pattern)
         switch(*pattern)
         {
            case '*':
               // Skip all redundant wildcards
               while(*++pattern == '*' || *pattern == '?') ;
               regpat.append(LITERAL_STR(".*")) ;
               break ;

            case '?':
               regpat.append(1, '.') ;
               ++pattern ;
               break ;

            case '.':
            case '^':
            case '$':
            case ']':
            case '\\':
            case '+':
               regpat.append(1, '\\').append(1, *pattern++) ;
               break ;

            case '[':
            {
               const char_type *charclass = pattern + 1 ;
               const char_type *negclass = LITERAL_STR("") ;
               if (unix_neg_charclass)
                  switch (*charclass)
                  {
                     case '!': negclass = LITERAL_STR("^") ; ++charclass ; break ;
                     case '^': negclass = LITERAL_STR("\\") ; break ;
                  }
               const char_type *endclass = cstrchr(charclass, ']') ;
               if (!endclass || endclass == charclass)
                  regpat.append("\\[") ;
               else
               {
                  for(regpat.append(1, '[').append(negclass) ; charclass <= endclass ; ++charclass)
                     if (*charclass == '\\')
                        regpat.append(LITERAL_STR("\\\\")) ;
                     else
                        regpat.append(1, *charclass) ;
                  pattern = endclass ;
               }
               ++pattern ;
               break ;
            }

            default:
               regpat.append(1, *pattern++) ;
         }

   regpat.append(1, '$') ;
   return regex(regpat) ;
}

} // end of namespace pcomn
