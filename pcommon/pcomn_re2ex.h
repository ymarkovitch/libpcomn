/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_RE2EX_H
#define __PCOMN_RE2EX_H
/*******************************************************************************
 FILE         :   pcomn_re2ex.h
 COPYRIGHT    :   Yakov Markovitch, 2013-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Regular expression class, a wrapper around RE2 regular expressions.
                  Provides the same C++ interface as pcomn::regex.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   15 Jul 2013
*******************************************************************************/
#include <pcomn_regex.h>
#include <pcomn_alloca.h>

#include <re2/re2.h>

namespace pcomn {

/******************************************************************************/
/** Exception class for re2ex constructor
*******************************************************************************/
class re2ex_error : public regex_error {
      typedef regex_error ancestor ;
   public:
      explicit re2ex_error(const re2::RE2 &rx) :
         ancestor(map_errcode(rx.error_code()), rx.error().c_str(), rx.pattern().c_str(), rx.error_arg().c_str())
      {}

   private:
      static PRegError map_errcode(re2::RE2::ErrorCode err)
      {
         static const std::pair<re2::RE2::ErrorCode, PRegError> errmap[] =
         {
         #define REGERRMAP(re2err, pregerr) { re2::RE2::Error##re2err, PREG_##pregerr }
            REGERRMAP(Internal, INTERNAL_ERROR),
            REGERRMAP(BadEscape, BAD_ESCAPE),
            REGERRMAP(BadCharClass, BAD_CHAR_CLASS),
            REGERRMAP(BadCharRange, BAD_CHAR_RANGE),
            REGERRMAP(MissingBracket, UNMATCHED_BRACKETS),
            REGERRMAP(MissingParen, UNMATCHED_PARENTHESIS),
            REGERRMAP(TrailingBackslash, TRAILING_BSLASH),
            REGERRMAP(RepeatArgument, BAD_REPEAT),
            REGERRMAP(RepeatSize, BAD_REPEAT_SIZE),
            REGERRMAP(RepeatOp, BAD_REPEAT),
            REGERRMAP(BadPerlOp, CORRUPTED_REGEXP),
            REGERRMAP(BadUTF8, BAD_ENCODING),
            REGERRMAP(BadNamedCapture, BAD_NMCAPTURE),
            REGERRMAP(PatternTooLarge, TOO_BIG),
            // This is a sentry item: we pass end-1 to find_if, so may avoid checking for
            // end-iterator when error code is not in the map
            REGERRMAP(Internal, CORRUPTED_REGEXP)
         #undef REGERRMAP
         } ;

         return !err ? PREG_OK
            : std::find_if(P_ARRAY_BEGIN(errmap), P_ARRAY_END(errmap) - 1,
                           [err](decltype(*errmap) &m){ return m.first == err ; })->second ;
      }
} ;

/******************************************************************************/
/**
*******************************************************************************/
class _PCOMNEXP re2ex : public regex_matcher {
      typedef regex_matcher ancestor ;
   public:
      /************************************************************************/
      /** Regexp option flags; correspond to re::Options
      *************************************************************************/
      enum Flags {
         OPT_LATIN1     = 0x0001, /**< Text and pattern are Latin-1; default is UTF-8 */
         OPT_POSIX      = 0x0002, /**< Restrict regexps to POSIX egrep syntax */
         OPT_LONGEST_MATCH = 0x0004, /**< Search for longest match, not first match */
         OPT_LITERAL    = 0x0008, /**< Interpret string as literal, not regexp */
         OPT_NONL       = 0x0010, /**< Never match \n, even if it is in regexp */
         OPT_NOCAPTURE  = 0x0020, /**< Parse all parens as non-capturing */
         OPT_NOCASE     = 0x0040  /**< Case-insensitibe match */
      } ;

      re2ex() : ancestor(create_pattern(strslice(), 0)) {}

      /// Compile the regexp from the given string.
      re2ex(const strslice &regstr, unsigned options = 0) :
         ancestor(create_pattern(regstr, options))
      {}

   private:
      static strslice make_strslice(const re2::StringPiece &s) { return strslice(s.begin(), s.end()) ; }
      static re2::StringPiece make_stringpiece(const strslice &s)
      {
         return re2::StringPiece(s.begin() ? s.begin() : "", s.size()) ;
      }

      struct pattern_type : regex_pattern {
            explicit pattern_type(const strslice &s, const re2::RE2::Options &options) :
               exp(make_stringpiece(s), options)
            {
               ensure<re2ex_error>(exp.ok(), exp) ;
            }

            std::pair<bool, reg_match *>
            exec_match(const char_type *begin, const char_type *end,
                       reg_match *begin_subexp, size_t subexp_count) const override
            {
               static const char zero = 0 ;
               const re2::StringPiece &str =
                  end   ? re2::StringPiece(begin, end - begin) :
                  begin ? re2::StringPiece(begin) :
                  re2::StringPiece(&zero, 0) ;

               P_FAST_BUFFER(submatch, re2::StringPiece, subexp_count + 1, 512) ;
               memset((void*)submatch, 0, sizeof *submatch * subexp_count) ;

               std::pair<bool, reg_match *> result
                  (exp.Match(str, 0, str.size(), re2::RE2::UNANCHORED, submatch, subexp_count),
                   begin_subexp) ;

               if (result.first && subexp_count)
               {
                  result.second += subexp_count ;
                  const re2::StringPiece *e = submatch + subexp_count - 1 ;
                  while (!e->data())
                  {
                     PSUBEXP_RESET(*(--result.second)) ;
                     if (e == submatch)
                        return result ;
                     --e ;
                  }
                  reg_match *last_subexp = result.second - 1 ;
                  do
                  {
                     if (e->data())
                     {
                        last_subexp->rm_so = e->begin() - str.begin() ;
                        last_subexp->rm_len = e->size() ;
                     }
                     else
                        PSUBEXP_RESET(*last_subexp) ;
                     --last_subexp ;
                  }
                  while(e-- != submatch) ;
               }
               return result ;
            }

            re2::RE2 exp ;
      } ;

   private:
      static re2::RE2::Options &init_options(unsigned optflags, re2::RE2::Options &options)
      {
         if (optflags)
         {
            #define SET_RE_OPT(OPTNAME, optname, ...) options.set_##optname(__VA_ARGS__ !!(optflags & OPT_##OPTNAME))
            SET_RE_OPT(LATIN1,         utf8, !) ;
            SET_RE_OPT(POSIX,          posix_syntax) ;
            SET_RE_OPT(LONGEST_MATCH,  longest_match) ;
            SET_RE_OPT(LITERAL,        literal) ;
            SET_RE_OPT(NONL,           never_nl) ;
            SET_RE_OPT(NOCAPTURE,      never_capture) ;
            SET_RE_OPT(NOCASE,         case_sensitive, !) ;
            #undef SET_RE_OPT
         }
         return options ;
      }

      static __noinline regex_pattern *create_pattern(const strslice &regstr, unsigned optflags)
      {
         re2::RE2::Options options (re2::RE2::Quiet) ;
         return new pattern_type(regstr, init_options(optflags, options)) ;
      }
} ;

}  // end of namespace pcomn

#endif /* __PCOMN_RE2EX_H */
