/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PBREGEX_H
#define __PBREGEX_H
/*******************************************************************************
 FILE         :   pbregex.h
 COPYRIGHT    :   Henry Spencer, 1986-1988. All rights reserved.
                  Yakov Markovitch, 1998-2017. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Definitions etc. for regexp(3) routines.

 CREATION DATE:   7 Apr 1998
*******************************************************************************/
#include <pcomn_platform.h>

#include <stddef.h>
#include <stdlib.h>

typedef int reg_off ;

typedef struct reg_match {
   reg_off rm_so ;  /* Start offset (begin matching) */
   reg_off rm_len ; /* Length of the match */
} reg_match ;

/*
 * Definitions etc. for regexp(3) routines.
 *
 * Caveat:  this is V8 regexp(3) [actually, a reimplementation thereof],
 * not the System V one.
 */

#define MAXNUMEXP 32 /* Max number of parenthesized subexpressions */

typedef struct regexp {
   char *regmust;          /* Internal use only. */
   char *program;        /* Unwarranted chumminess with compiler. */
   int   regmlen;            /* Internal use only. */
   char  regstart;          /* Internal use only. */
   char  reganch;           /* Internal use only. */
} regexp, pcomn_regex_t ;

typedef enum PRegError
{
   PREG_OK,                   /**< Regular expression is OK */

   PREG_BAD_ARG,              /**< NULL argument */
   PREG_TOO_BIG,              /**< Pattern too large, compilation failed */
   PREG_OUT_OF_MEM,           /**< Not enough memory for compiled regexp */
   PREG_TOO_MANY_PARENTHESIS, /**< Too many parenthesis */
   PREG_UNMATCHED_PARENTHESIS, /**< Unmatched parenthesis () */
   PREG_UNMATCHED_BRACKETS,   /**< Unmatched brackets [] */
   PREG_BAD_CHAR_CLASS,       /**< Invalid char class */
   PREG_BAD_CHAR_RANGE,       /**< Invalid char range */
   PREG_BAD_REPEAT,           /**< Nested *?+ or ?+* follows nothing */
   PREG_BAD_REPEAT_SIZE,      /**< Too big argument to {} */
   PREG_TRAILING_BSLASH,      /**< Trailing backslash */

   PREG_BAD_ESCAPE,           /**< Invalid escape sequence */
   PREG_BAD_ENCODING,         /**< Invalid character encoding (e.g. UTF-8) in regexp */
   PREG_BAD_NMCAPTURE,        /**< Bad named capture */

   PREG_CORRUPTED_REGEXP   = 999,  /**< Unspecified catch-all error */
   PREG_INTERNAL_ERROR     = 1000  /**< Internal error */

} PRegError ;

typedef void (*regexp_handler) (PRegError errcode, const char *err, const char *exp, const char *pos) ;

#ifdef  __cplusplus
extern "C"{
#endif

/** Compile a regular expression.
   @param source_expression   a source regular expression (in the string form)
   @param compiled_expression a buffer for resulting compiled expression
   @param cflags              regular expression compilation flags

   @return PREG_OK (0) if the regular expression is valid and successfully compiled; one of
   the error codes an from PRegError otherwise.

   @note Compiled expression must be deallocated by pcomn_regfree.
*/
_PCOMNEXP int pcomn_regcomp(pcomn_regex_t *compiled_expression, const char *source_expression, int cflags) ;

/*
   Match string by regular expression.
   Parameters:
      compiled_expression  -  a compiled regular expression
      string_to_match      -  a string to be matched
      num_of_subexps       -  the size of subexpressions array (number of elements)
      subexpressions       -  an array of subexpressions

   Result:
      number of matched subexpressions (0 if none matched)
*/
_PCOMNEXP int pcomn_regexec(const pcomn_regex_t *compiled_expression, const char *string_to_match,
                            size_t num_of_subexps, reg_match *subexpressions, int cflags) ;

/*
   Print error text to stderr
   Parameters:
      errcode  -  error code
      err      -  additional error text
      exp      -  regular expression (in source form)
      pos      -  position of error

   Result:
      number of matched subexpressions (0 if none matched)
*/
_PCOMNEXP void pcomn_regerror(PRegError errcode, const char *err, const char *exp, const char *pos) ;

_PCOMNEXP void pcomn_regfree(pcomn_regex_t *preg) ;

/*******************************************************************************
 Nonstandard (non-POSIX) functions
*******************************************************************************/
_PCOMNEXP int pcomn_regcomp_ex(pcomn_regex_t *r, const char *exp, int cflags, regexp_handler handler) ;

_PCOMNEXP void  pcomn_regdump(const pcomn_regex_t *r) ;

_PCOMNEXP char *regmemmove(char *dest, const char *src, const reg_match *match) ;

_PCOMNEXP char *regstrcpy(char *dest, const char *src, const reg_match *match) ;

#ifdef  __cplusplus
}
#endif

#ifndef __cplusplus

#define PSUBEXP_BO(s)         ((s).rm_so)
#define PSUBEXP_EO(s)         ((s).rm_so + (s).rm_len)
#define PSUBEXP_LENGTH(s)     ((s).rm_len) /* Length of a matched (sub)expression */
#define PSUBEXP_MATCHED(s)    ((s).rm_so >= 0)        /* Checking whether a subexpression matches */
#define PSUBEXP_EMPTY(s)      (!PSUBEXP_MATCHED(s) || !PSUBEXP_LENGTH(s))
#define PSUBEXP_RESET(s)      (*(reg_match *)memset(&(s), -1, sizeof(s)))
#define PSUBEXP_OFFS(s, offs) ((((s).rm_so += (offs)), (s)))

#else

inline int  PSUBEXP_BO(const reg_match &rm) { return rm.rm_so ; }
inline int  PSUBEXP_EO(const reg_match &rm) { return rm.rm_so + rm.rm_len ; }
inline int  PSUBEXP_LENGTH(const reg_match &rm) { return rm.rm_len ; }
inline bool PSUBEXP_MATCHED(const reg_match &rm) { return rm.rm_so >= 0 ; }
inline bool PSUBEXP_EMPTY(const reg_match &rm) { return !PSUBEXP_MATCHED(rm) || !PSUBEXP_LENGTH(rm) ; }
inline reg_match &PSUBEXP_RESET(reg_match &rm) { rm.rm_so = -1 ; rm.rm_len = 0 ; return rm ; }
inline reg_match &PSUBEXP_OFFS(reg_match &rm, int offs) { rm.rm_so += offs ; return rm ; }

/*******************************************************************************
 reg_match constructor
*******************************************************************************/
inline reg_match make_reg_match(reg_off start, reg_off end)
{
   reg_match result ;
   result.rm_so = start ;
   result.rm_len = end - start ;
   return result ;
}

/*******************************************************************************
 reg_match operators
*******************************************************************************/
inline reg_match &operator+=(reg_match &lhs, int rhs)
{
   lhs.rm_so += rhs ;
   return lhs ;
}

inline reg_match &operator-=(reg_match &lhs, int rhs)
{
   return lhs += -rhs ;
}

inline reg_match operator+(const reg_match &lhs, int rhs)
{
   reg_match result = lhs ;
   return result += rhs ;
}

inline reg_match operator-(const reg_match &lhs, int rhs)
{
   reg_match result = lhs ;
   return result -= rhs ;
}

inline bool operator==(const reg_match &lhs, const reg_match &rhs)
{
   return lhs.rm_so == rhs.rm_so && lhs.rm_len == rhs.rm_len ;
}

inline bool operator!=(const reg_match &lhs, const reg_match &rhs)
{
   return !(lhs == rhs) ;
}

inline bool operator!(const reg_match &item)
{
   return !PSUBEXP_MATCHED(item) ;
}

#endif

#define PSUBEXP_BEGIN(str, regmatch)  ((str) + PSUBEXP_BO((regmatch)))
#define PSUBEXP_END(str, regmatch)    ((str) + PSUBEXP_EO((regmatch)))

#endif /* __PBREGEX_H */
