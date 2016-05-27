/*-*- tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel" -*-*/
/*
 * regcomp and regexec
 *
 * Copyright (c) 1986 by University of Toronto.
 *
 * Originally written by Henry Spencer.  Not derived from licensed software.
 *
 * Here's not an original version - some parts are changed/added by Yakov E. Markovitch
 *
 * Changed:
 *    forward declarations of static functions made properly;
 *
 * Added:
 *    First parameter (pointer to execution data pseudo-object) added to the most functions for
 *    thread-safety purposes
 */

/*
 * regcomp and regexec -- regsub and regerror are elsewhere
 *
 * Copyright (c) 1986 by University of Toronto.
 * Written by Henry Spencer.  Not derived from licensed software.
 *
 * Permission is granted to anyone to use this software for any
 * purpose on any computer system, and to redistribute it freely,
 * subject to the following restrictions:
 *
 * 1. The author is not responsible for the consequences of use of
 *   this software, no matter how awful, even if they arise
 *   from defects in it.
 *
 * 2. The origin of this software must not be misrepresented, either
 *   by explicit claim or by omission.
 *
 * 3. Altered versions must be plainly marked as such, and must not
 *   be misrepresented as being the original software.
 *
 * Beware that some of this code is subtly aware of the way operator
 * precedence is structured in regular expressions.  Serious changes in
 * regular-expression syntax might require a total rethink.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pbregex.h>
#include <pcomn_assert.h>

typedef struct {
      PRegError       lasterror ;
      regexp_handler  errhandler ;
} _regex_errproc ;

_regex_errproc *regex_errproc(void) ;

// size_t -> int assignment
MS_IGNORE_WARNING(4244 4267)

/*
 * The "internal use only" fields in regexp.h are present to pass info from
 * compile to execute that permits the execute phase to run lots faster on
 * simple cases.  They are:
 *
 * regstart   char that must begin a match; '\0' if none obvious
 * reganch is the match anchored (at beginning-of-line only)?
 * regmust string (pointer into program) that match must include, or NULL
 * regmlen length of regmust string
 *
 * Regstart and reganch permit very fast decisions on suitable starting points
 * for a match, cutting down the work a lot.  Regmust permits fast rejection
 * of lines that cannot possibly match.  The regmust tests are costly enough
 * that pcomn_regcomp() supplies a regmust only if the r.e. contains something
 * potentially expensive (at present, the only such thing detected is * or +
 * at the start of the r.e., which can involve a lot of backup).  Regmlen is
 * supplied because the test in pcomn_regexec() needs it and pcomn_regcomp()
 * is computing it anyway.
 */

/*
 * Structure for regexp "program".  This is essentially a linear encoding
 * of a nondeterministic finite-state machine (aka syntax charts or
 * "railroad normal form" in parsing technology).  Each node is an opcode
 * plus a "next" pointer, possibly plus an operand.  "Next" pointers of
 * all nodes except BRANCH implement concatenation; a "next" pointer with
 * a BRANCH on both ends of it is connecting two alternatives.  (Here we
 * have one of the subtle syntax dependencies:  an individual BRANCH (as
 * opposed to a collection of them) is never concatenated with anything
 * because of operator precedence.)  The operand of some types of node is
 * a literal string; for others, it is a node leading into a sub-FSM.  In
 * particular, the operand of a BRANCH node is the first node of the branch.
 * (NB this is *not* a tree structure:  the tail of the branch connects
 * to the thing following the set of BRANCHes.)  The opcodes are:
 */

/* definition number   opnd? meaning */
#define END   0  /* no End of program. */
#define BOL   1  /* no Match "" at beginning of line. */
#define EOL   2  /* no Match "" at end of line. */
#define ANY   3  /* no Match any one character. */
#define ANYOF 4  /* str   Match any character in this string. */
#define ANYBUT   5  /* str   Match any character not in this string. */
#define BRANCH   6  /* node  Match this alternative, or the next... */
#define BACK  7  /* no Match "", "next" ptr points backward. */
#define EXACTLY  8  /* str   Match this string. */
#define NOTHING  9  /* no Match empty string. */
#define STAR  10 /* node  Match this (simple) thing 0 or more times. */
#define PLUS  11 /* node  Match this (simple) thing 1 or more times. */
#define OPEN  20 /* no Mark this point in input as start of #n. */
        /* OPEN+1 is number 1, etc. */
#define CLOSE (OPEN+MAXNUMEXP) /* no Analogous to OPEN. */

/*
 * Opcode notes:
 *
 * BRANCH  The set of branches constituting a single choice are hooked
 *   together with their "next" pointers, since precedence prevents
 *   anything being concatenated to any individual branch.  The
 *   "next" pointer of the last BRANCH in a choice points to the
 *   thing following the whole choice.  This is also where the
 *   final "next" pointer of each individual branch points; each
 *   branch starts with the operand node of a BRANCH node.
 *
 * BACK    Normal "next" pointers all implicitly point forward; BACK
 *   exists to make loop structures possible.
 *
 * STAR,PLUS  '?', and complex '*' and '+', are implemented as circular
 *   BRANCH structures using BACK.  Simple cases (one character
 *   per match) are implemented with STAR and PLUS for speed
 *   and to minimize recursive plunges.
 *
 * OPEN,CLOSE ...are numbered at compile time.
 */

/*
 * A node is one char of opcode followed by two chars of "next" pointer.
 * "Next" pointers are stored as two 8-bit pieces, high order first.  The
 * value is a positive offset from the opcode of the node containing it.
 * An operand, if any, simply follows the node.  (Note that much of the
 * code generation knows about this implicit relationship.)
 *
 * Using two bytes for the "next" pointer is vast overkill for most things,
 * but allows patterns to get big without disasters.
 */
#define OP(p) (*(p))
#define NEXT(p)  (((*((p)+1)&0377)<<8) + (*((p)+2)&0377))
#define OPERAND(p)  ((p) + 3)

#define INRANGE(c,l,r) ((l) <= (c) && (c) <= (r))

/*
 * The first byte of the regexp internal "program" is actually this magic
 * number; the start node begins in the second byte.
 */
#define MAGIC 0234

/*
 * Utility definitions.
 */
#ifndef CHARBITS
#define UCHARAT(p)  ((int)*(unsigned char *)(p))
#else
#define UCHARAT(p)  ((int)*(p)&CHARBITS)
#endif

#define FAIL(code,m,data)  { errh((code),(m),(data)); return(NULL); }
#define FAIL_COMPLETELY(s) PCOMN_FAIL(s)
#define ISMULT(c)   ((c) == '*' || (c) == '+' || (c) == '?')
#define META  "^$.[()|?+*\\"

/*
 * Flags to be passed up and down.
 */
#define HASWIDTH  01 /* Known never to match null string. */
#define SIMPLE    02 /* Simple enough to be STAR/PLUS operand. */
#define SPSTART   04 /* Starts with * or +. */
#define WORST     0  /* Worst case. */

/*
 * Global work variables for pcomn_regcomp().
*/

static char regdummy;

typedef struct regcomp_data {
   const char *exp ;    /* Source expression */
   regexp *    result ;
   char * regparse;     /* Input-scan pointer. */
   int    regnpar;      /* () count. */
   char * regcode;      /* Code-emit pointer; &regdummy = don't. */
   long   regsize;      /* Code size. */
   regexp_handler handler ;
   PRegError errcode ;
} regcomp_data ;

typedef struct subexp_match {
   const char *start ;  /* Start pointer (begin matching) */
   const char *end ;  /*  End pointer (end matching) */
} subexp_match ;

/*
* Global work variables for pcomn_regexec().
*/

typedef struct regexec_data {
   const char *   reginput;    /* String-input pointer. */
   const char *   reginpend;   /* End of reginput or NULL */
   const char *   regbol;      /* Beginning of input, for ^ check. */
   subexp_match * regsubexps ; /* Pointer to startp array. */
} regexec_data ;


/*
 * Forward declarations for pcomn_regcomp()'s friends.
 */
static const char *_regcomp(regexp *r, const char *exp, regcomp_data *data) ;
static char *reg(regcomp_data *data, int paren /* Parenthesized? */, int *flagp);
static char *regbranch(regcomp_data *data, int *flagp) ;
static char *regpiece(regcomp_data *data, int *flagp);
static char *regatom(regcomp_data *data, int *flagp);
static char *regnode(regcomp_data *data, char op);
static char *regnext(const char *p);
static void regc(regcomp_data *data, char b);
static void reginsert(regcomp_data *data, char op, char *opnd);
static void regtail(char *p, char *val);
static void regoptail(char *p, char *val);

static void errh(PRegError errcode, const char *err, regcomp_data *data) ;

int pcomn_regcomp(regexp *r, const char *exp, int cflags)
{
   return pcomn_regcomp_ex(r, exp, cflags, NULL) ;
}

int pcomn_regcomp_ex(regexp *r, const char *exp, int cflags, regexp_handler handler)
{
   regcomp_data data ;
   (void)cflags ;
   memset(&data, 0, sizeof data) ;
   data.handler = handler ;
   /* For error handling purposes. */
   data.exp = exp ;
   data.result = NULL ;

   return
      _regcomp(r, exp, &data) ? PREG_OK : data.errcode ;
}

/*
 - _regcomp - compile a regular expression into internal code
 *
 * We can't allocate space until we know how big the compiled form will be,
 * but we can't compile it (and thus know how big it is) until we've got a
 * place to put the code.  So we cheat:  we compile it twice, once with code
 * generation turned off and size counting turned on, and once "for real".
 * This also means that we don't allocate space until we are sure that the
 * thing really will compile successfully, and we never have to move the
 * code and thus invalidate pointers into it.  (Note that it has to be in
 * one piece because free() must be able to free it all.)
 *
 * Beware that the optimization-preparation code in here knows about some
 * of the structure of the compiled regexp.
 */
static const char *_regcomp(regexp *r, const char *exp, regcomp_data *data)
{
   char *scan;
   char *longest;
   int len;
   int flags;

   if (data->exp == NULL || r == NULL)
     FAIL(PREG_BAD_ARG, "NULL argument", data);

   /* First pass: determine size, legality. */
   data->regparse = (char *)exp;
   data->regnpar = 1;
   data->regsize = 0L;
   data->regcode = &regdummy ;

   regc(data, MAGIC);

   if (reg(data, 0, &flags) == NULL)
     FAIL(PREG_INTERNAL_ERROR, "internal error", data);

   /* Small enough for pointer-storage convention? */
   if (data->regsize >= 32767L)    /* Probably could be 65535L. */
     FAIL(PREG_TOO_BIG, "regexp too big", data);

   /* Allocate space. */
   (data->result = r)->program = (char *)malloc((unsigned)(data->regsize+1));
   if (r->program == NULL)
     FAIL(PREG_OUT_OF_MEM, "out of space", data);

   /* Second pass: emit code. */
   data->regparse = (char *)exp;
   data->regnpar = 1;
   data->regcode = r->program;

   regc(data, MAGIC);
   if (reg(data, 0, &flags) == NULL)
     FAIL(PREG_INTERNAL_ERROR, "internal error", data);

   /* Dig out information for optimizations. */
   r->regstart = '\0'; /* Worst-case defaults. */
   r->reganch = 0;
   r->regmust = NULL;
   r->regmlen = 0;
   scan = r->program+1;         /* First BRANCH. */
   if (OP(regnext(scan)) == END) {    /* Only one top-level choice. */
     scan = OPERAND(scan);

     /* Starting-point info. */
     if (OP(scan) == EXACTLY)
        r->regstart = *OPERAND(scan);
     else if (OP(scan) == BOL)
        r->reganch++;

     /*
      * If there's something expensive in the r.e., find the
      * longest literal string that must appear and make it the
      * regmust.  Resolve ties in favor of later strings, since
      * the regstart check works with the beginning of the r.e.
      * and avoiding duplication strengthens checking.  Not a
      * strong reason, but sufficient in the absence of others.
      */
     if (flags&SPSTART) {
        longest = NULL;
        len = 0;
        for (; scan != NULL; scan = regnext(scan))
           if (OP(scan) == EXACTLY && (int)strlen(OPERAND(scan)) >= len) {
              longest = OPERAND(scan);
              len = strlen(OPERAND(scan));
           }
        r->regmust = longest;
        r->regmlen = len;
     }
   }

   return data->exp ;
}

/*
 - reg - regular expression, i.e. main body or parenthesized thing
 *
 * Caller must absorb opening parenthesis.
 *
 * Combining parenthesis handling with the base level of regular expression
 * is a trifle forced, but the need to tie the tails of the branches to what
 * follows makes it hard to avoid.
 */
static char *reg(regcomp_data *data, int paren/* Parenthesized? */, int *flagp)
{
   char *ret;
   char *br;
   char *ender;
   int end = END;
   int flags;

   *flagp = HASWIDTH;  /* Tentatively. */

   /* Make an OPEN node, if parenthesized. */
   if (paren) {
     int parno;
     if ((data->regnpar) >= MAXNUMEXP)
        FAIL(PREG_TOO_MANY_PARENTHESIS, "too many ()", data);
     parno = (data->regnpar);
     (data->regnpar)++;
     ret = regnode(data, OPEN+parno);
     end = CLOSE+parno;
   } else
     ret = NULL;

   /* Pick up the branches, linking them together. */
   br = regbranch(data, &flags);
   if (br == NULL)
     return(NULL);
   if (ret != NULL)
     regtail(ret, br); /* OPEN -> first. */
   else
     ret = br;
   if (!(flags&HASWIDTH))
     *flagp &= ~HASWIDTH;
   *flagp |= flags&SPSTART;
   while (*(data->regparse) == '|') {
     (data->regparse)++;
     br = regbranch(data, &flags);
     if (br == NULL)
        return(NULL);
     regtail(ret, br); /* BRANCH -> BRANCH. */
     if (!(flags&HASWIDTH))
        *flagp &= ~HASWIDTH;
     *flagp |= flags&SPSTART;
   }

   /* Make a closing node, and hook it on the end. */
   ender = regnode(data, end);
   regtail(ret, ender);

   /* Hook the tails of the branches to the closing node. */
   for (br = ret; br != NULL; br = regnext(br))
     regoptail(br, ender);

   /* Check for proper termination. */
   if (paren && *(data->regparse)++ != ')') {
     FAIL(PREG_UNMATCHED_PARENTHESIS, "unmatched ()", data);
   } else if (!paren && *(data->regparse) != '\0') {
     if (*(data->regparse) == ')') {
        FAIL(PREG_UNMATCHED_PARENTHESIS, "unmatched ()", data);
     } else
        FAIL(PREG_INTERNAL_ERROR, "junk on end", data); /* "Can't happen". */
     /* NOTREACHED */
   }

   return(ret);
}

/*
 - regbranch - one alternative of an | operator
 *
 * Implements the concatenation operator.
 */
static char *regbranch(regcomp_data *data, int *flagp)
{
   char *ret;
   char *chain;
   char *latest;
   int flags;

   *flagp = WORST;     /* Tentatively. */

   ret = regnode(data, BRANCH);
   chain = NULL;
   while (*(data->regparse) != '\0' && *(data->regparse) != '|' && *(data->regparse) != ')') {
     latest = regpiece(data, &flags);
     if (latest == NULL)
        return(NULL);
     *flagp |= flags&HASWIDTH;
     if (chain == NULL)   /* First piece. */
        *flagp |= flags&SPSTART;
     else
        regtail(chain, latest);
     chain = latest;
   }
   if (chain == NULL)  /* Loop ran zero times. */
     (void) regnode(data, NOTHING);

   return(ret);
}

/*
 - regpiece - something followed by possible [*+?]
 *
 * Note that the branching code sequences used for ? and the general cases
 * of * and + are somewhat optimized:  they use the same NOTHING node as
 * both the endmarker for their branch list and the body of the last branch.
 * It might seem that this node could be dispensed with entirely, but the
 * endmarker role is not redundant.
 */
static char *regpiece(regcomp_data *data, int *flagp)
{
   char *ret;
   char op;
   char *next;
   int flags;

   ret = regatom(data, &flags);
   if (ret == NULL)
     return(NULL);

   op = *(data->regparse);
   if (!ISMULT(op)) {
     *flagp = flags;
     return(ret);
   }

   if (!(flags&HASWIDTH) && op != '?')
     FAIL(PREG_BAD_REPEAT, "*+ operand could be empty", data);
   *flagp = (op != '+') ? (WORST|SPSTART) : (WORST|HASWIDTH);

   if (op == '*' && (flags&SIMPLE))
     reginsert(data, STAR, ret);
   else if (op == '*') {
     /* Emit x* as (x&|), where & means "self". */
     reginsert(data, BRANCH, ret);       /* Either x */
     regoptail(ret, regnode(data, BACK));      /* and loop */
     regoptail(ret, ret);       /* back */
     regtail(ret, regnode(data, BRANCH));      /* or */
     regtail(ret, regnode(data, NOTHING));     /* null. */
   } else if (op == '+' && (flags&SIMPLE))
     reginsert(data, PLUS, ret);
   else if (op == '+') {
     /* Emit x+ as x(&|), where & means "self". */
     next = regnode(data, BRANCH);       /* Either */
     regtail(ret, next);
     regtail(regnode(data, BACK), ret);     /* loop back */
     regtail(next, regnode(data, BRANCH));     /* or */
     regtail(ret, regnode(data, NOTHING));     /* null. */
   } else if (op == '?') {
     /* Emit x? as (x|) */
     reginsert(data, BRANCH, ret);       /* Either x */
     regtail(ret, regnode(data, BRANCH));      /* or */
     next = regnode(data, NOTHING);      /* null. */
     regtail(ret, next);
     regoptail(ret, next);
   }
   (data->regparse)++;
   if (ISMULT(*(data->regparse)))
     FAIL(PREG_BAD_REPEAT, "nested *?+", data);

   return(ret);
}

/*
 - regatom - the lowest level
 *
 * Optimization:  gobbles an entire sequence of ordinary characters so that
 * it can turn them into a single node, which is smaller to store and
 * faster to run.  Backslashed characters are exceptions, each becoming a
 * separate node; the code is simpler that way and it's not worth fixing.
 */
static char *regatom(regcomp_data *data, int *flagp)
{
   char *ret;
   int flags;

   *flagp = WORST;     /* Tentatively. */

   switch (*(data->regparse)++) {
   case '^':
     ret = regnode(data, BOL);
     break;
   case '$':
     ret = regnode(data, EOL);
     break;
   case '.':
     ret = regnode(data, ANY);
     *flagp |= HASWIDTH|SIMPLE;
     break;
   case '[': {
        int cclass;
        int classend;

        if (*(data->regparse) == '^') { /* Complement of range. */
           ret = regnode(data, ANYBUT);
           (data->regparse)++;
        } else
           ret = regnode(data, ANYOF);
        if (*(data->regparse) == ']' || *(data->regparse) == '-')
           regc(data, *(data->regparse)++);
        while (*(data->regparse) != '\0' && *(data->regparse) != ']') {
           if (*(data->regparse) == '-') {
              (data->regparse)++;
              if (*(data->regparse) == ']' || *(data->regparse) == '\0')
                 regc(data, '-');
              else {
                 cclass = UCHARAT((data->regparse)-2)+1;
                 classend = UCHARAT((data->regparse));
                 if (cclass > classend+1)
                    FAIL(PREG_BAD_CHAR_RANGE, "invalid [] range", data);
                 for (; cclass <= classend; cclass++)
                    regc(data, cclass);
                 (data->regparse)++;
              }
           } else
              regc(data, *(data->regparse)++);
        }
        regc(data, '\0');
        if (*(data->regparse) != ']')
           FAIL(PREG_UNMATCHED_BRACKETS, "unmatched []", data);
        (data->regparse)++;
        *flagp |= HASWIDTH|SIMPLE;
     }
     break;
   case '(':
     ret = reg(data, 1, &flags);
     if (ret == NULL)
        return(NULL);
     *flagp |= flags&(HASWIDTH|SPSTART);
     break;
   case '\0':
   case '|':
   case ')':
     FAIL(PREG_INTERNAL_ERROR, "internal urp", data);   /* Supposed to be caught earlier. */
     break;
   case '?':
   case '+':
   case '*':
     FAIL(PREG_BAD_REPEAT, "?+* follows nothing", data);
     break;
   case '\\':
     if (*(data->regparse) == '\0')
        FAIL(PREG_TRAILING_BSLASH, "trailing \\", data);
     ret = regnode(data, EXACTLY);
     regc(data, *(data->regparse)++);
     regc(data, '\0');
     *flagp |= HASWIDTH|SIMPLE;
     break;
   default: {
        int len;
        char ender;

        (data->regparse)--;
        len = strcspn((data->regparse), META);
        if (len <= 0)
           FAIL(PREG_INTERNAL_ERROR, "internal disaster", data);
        ender = *((data->regparse)+len);
        if (len > 1 && ISMULT(ender))
           len--;      /* Back off clear of ?+* operand. */
        *flagp |= HASWIDTH;
        if (len == 1)
           *flagp |= SIMPLE;
        ret = regnode(data, EXACTLY);
        while (len > 0) {
           regc(data, *(data->regparse)++);
           len--;
        }
        regc(data, '\0');
     }
     break;
   }

   return(ret);
}

/*
 - regnode - emit a node
 */
static char *       /* Location. */
regnode(regcomp_data *data, char op)
{
   char *ret;
   char *ptr;

   ret = (data->regcode);
   if (ret == &regdummy) {
     (data->regsize) += 3;
     return(ret);
   }

   ptr = ret;
   *ptr++ = op;
   *ptr++ = '\0';      /* Null "next" pointer. */
   *ptr++ = '\0';
   (data->regcode) = ptr;

   return(ret);
}

/*
 - regc - emit (if appropriate) a byte of code
 */
static void regc(regcomp_data *data, char b)
{
   if ((data->regcode) != &regdummy)
     *(data->regcode)++ = b;
   else
     (data->regsize)++;
}

/*
 - reginsert - insert an operator in front of already-emitted operand
 *
 * Means relocating the operand.
 */
static void reginsert(regcomp_data *data, char op, char *opnd)
{
   char *src;
   char *dst;
   char *place;

   if ((data->regcode) == &regdummy) {
     (data->regsize) += 3;
     return;
   }

   src = (data->regcode);
   (data->regcode) += 3;
   dst = (data->regcode);
   while (src > opnd)
     *--dst = *--src;

   place = opnd;    /* Op node, where operand used to be. */
   *place++ = op;
   *place++ = '\0';
   *place++ = '\0';
}

/*
 - regtail - set the next-pointer at the end of a node chain
 */
static void regtail(char *p, char *val)
{
   char *scan;
   char *temp;
   int offset;

   if (p == &regdummy)
     return;

   /* Find last node. */
   scan = p;
   for (;;) {
     temp = regnext(scan);
     if (temp == NULL)
        break;
     scan = temp;
   }

   if (OP(scan) == BACK)
     offset = scan - val;
   else
     offset = val - scan;
   *(scan+1) = (offset>>8)&0377;
   *(scan+2) = offset&0377;
}

/*
 - regoptail - regtail on operand of first argument; nop if operandless
 */
static void regoptail(char *p, char *val)
{
   /* "Operandless" and "op != BRANCH" are synonymous in practice. */
   if (p == NULL || p == &regdummy || OP(p) != BRANCH)
     return;
   regtail(OPERAND(p), val);
}

/*
 * regexec and friends
 */

/*
 * Forwards.
 */
struct reinput_data ;
typedef struct reinput_data reinput_data ;

static int regtry(regexec_data *data, const regexp *prog, const reinput_data *input,
                  subexp_match *subexps);
static int regmatch(regexec_data *data, const char *prog);
static int regrepeat(regexec_data *data, const char *p);

#ifdef DEBUG
int regnarrate = 0;
#define DEBUG_PREG(prog, pfx) ((void)((prog) && (regnarrate) && fprintf(stderr, "%s"pfx"\n", regprop((prog)))))
#else
#define DEBUG_PREG(prog, pfx) ((void)0)
#endif

static const char *regprop(const char *op);

void pcomn_regfree(regexp *preg)
{
   if (!preg)
   {
      if (preg->program)
         free(preg->program) ;
      memset(preg, 0, sizeof *preg) ;
   }
}

struct reinput_data {
   const char *   reginput;    /* String-input pointer. */
   const char *   reginpend;   /* End of reginput or NULL */
} ;

static const char zdummy = 0 ;

#define RMIN(a,b) (((a) < (b)) ? (a) : (b))

#define INIT_INPUT(data, begin, end) ((void)(((data)->reginput = (begin)), ((data)->reginpend = (end))))
#define INC_INPUT(data) (++(data)->reginput)
#define EQ_INPUT(data, s, len) (((data)->reginpend                      \
                                 ? !memcmp((data)->reginput, (s), RMIN((len), (data)->reginpend - (data)->reginput)) \
                                 : !strncmp((data)->reginput, (s), (len))))

#define END_INPUT(data) ((data)->reginput == (data)->reginpend || !*(data)->reginput)
#define SAVE_INPUT(data, name)    const char * const name = (data)->reginput
#define RESTORE_INPUT(data, name) ((data)->reginput = (name))

static const char *NXT_INPUT(reinput_data *data, char c)
{
   if (data->reginput == data->reginpend)
      data->reginput = data->reginpend = NULL ;
   else if (data->reginpend)
   {
      do if (*data->reginput == c) return data->reginput ;
      while (++data->reginput != data->reginpend) ;
      data->reginput = data->reginpend = NULL ;
   }
   else
      data->reginput = strchr(data->reginput, c) ;
   return data->reginput ;
}

/*******************************************************************************

*******************************************************************************/
int pcomn_xregexec(const regexp *prog, const char *begin, const char *end,
                   size_t num, reg_match *subexps, int cflags)
{
   subexp_match dummy[MAXNUMEXP] ;
   reinput_data input_range ;
   int result = 0 ;

   (void)cflags ;

   if (begin)
      INIT_INPUT(&input_range, begin, end) ;
   else if (!end)
      INIT_INPUT(&input_range, &zdummy, &zdummy) ;
   else
      prog = NULL ; /* Make it erroneous */

   /* Be paranoid... */
   if (!prog)
      FAIL_COMPLETELY("pcomn_regexec(): NULL parameter");

   /* Check validity of program. */
   if (UCHARAT(prog->program) != MAGIC)
      FAIL_COMPLETELY("pcomn_regexec(): corrupted program");

   /* If there is a "must appear" begin, look for it. */
   if (prog->regmust != NULL) {
      reinput_data input = input_range ;
      for(;;)
      {
         if (!NXT_INPUT(&input, prog->regmust[0]))
            /* Not present. */
            return 0 ;
         if (EQ_INPUT(&input, prog->regmust, prog->regmlen))
            /* Found it. */
            break;
         if (!*INC_INPUT(&input))
            /* Not present. */
            return 0 ;
      }
   }

   /* Check for match. */
   {
      regexec_data data ;
      reinput_data input = input_range ;

      /* Mark beginning of line for ^ . */
      data.regbol = input_range.reginput;

      /* Simplest case:  anchored match need be tried only once. */
      if (prog->reganch)
         result = regtry(&data, prog, &input, dummy);

      /* Messy cases:  unanchored match. */
      else if (prog->regstart != '\0')
         /* We know what char it must start with. */
         while (NXT_INPUT(&input, prog->regstart))
         {
            if (regtry(&data, prog, &input, dummy))
            {
               result = 1 ;
               break ;
            }
            INC_INPUT(&input) ;
         }
      else
         /* We don't -- general case. */
         while ((result = regtry(&data, prog, &input, dummy)) == 0 && *input.reginput)
            INC_INPUT(&input) ;
   }

   /* Success or failure? If success - copy matched ranges to result */
   if (result && subexps)
   {
      static const reg_match nullrange = { -1, 0 } ;
      int i ;
      for (i = 0 ; i < (int)num ; ++i, ++subexps)
         if (i < MAXNUMEXP && dummy[i].start)
         {
            subexps->rm_so = dummy[i].start - input_range.reginput ;
            subexps->rm_len = dummy[i].end - dummy[i].start ;
         }
         else
            *subexps = nullrange ;
   }

   return result ;
}

int pcomn_regexec(const regexp *prog, const char *begin,
                  size_t num, reg_match *subexps, int cflags)
{
   return pcomn_xregexec(prog, begin, NULL, num, subexps, cflags) ;
}

/*
 - regtry - try match at specific point
 */
/* 0 failure, 1 success */
static int regtry(regexec_data *data, const regexp *prog, const reinput_data *input,
                  subexp_match *subexps)
{
   data->reginput = input->reginput;
   data->reginpend = input->reginpend;
   data->regsubexps = subexps ;
   memset(subexps, 0, MAXNUMEXP * sizeof *subexps) ;

   if (!regmatch(data, prog->program + 1))
      return 0 ;

   subexps[0].start = input->reginput;
   subexps[0].end = data->reginput;
   return 1;
}

/*
 - regmatch - main matching routine
 *
 * Conceptually the strategy is simple:  check to see whether the current
 * node matches, call self recursively to see whether the rest matches,
 * and then act accordingly.  In practice we make some effort to avoid
 * recursion, in particular by going through "ordinary" nodes (that don't
 * need to know whether the rest of the match failed) by a loop instead of
 * by recursion.
 */
static int       /* 0 failure, 1 success */
regmatch(regexec_data *data, const char *prog)
{
   const char *scan;        /* Current node. */
   const char *next; /* Next node. */

   DEBUG_PREG(prog, "(") ;
   for (scan = prog; scan; scan = next)
   {
     int opscan ;

     DEBUG_PREG(scan, "...") ;
     next = regnext(scan);

     switch (opscan = OP(scan)) {

     case NOTHING:
     case BACK:    continue;

     case END: return 1;  /* Success! */

     case BOL:
        if ((data->reginput) != (data->regbol))
           return(0);
        break;

     case EOL:
        if (!END_INPUT(data))
           return(0);
        break;

     case ANY:
        if (END_INPUT(data))
           return(0);
        INC_INPUT(data) ;
        break;

     case EXACTLY: {
           const char *opnd;
           for (opnd = OPERAND(scan) ; *opnd && *opnd == *(data->reginput) ; ++opnd, INC_INPUT(data)) ;
           if (*opnd)
              return 0;
        }
        break;

     case ANYOF:
        if (END_INPUT(data) || strchr(OPERAND(scan), *(data->reginput)) == NULL)
           return(0);
        INC_INPUT(data) ;
        break;

     case ANYBUT:
        if (END_INPUT(data) || strchr(OPERAND(scan), *(data->reginput)) != NULL)
           return(0);
        INC_INPUT(data) ;
        break;

     case BRANCH:
           if (OP(next) != BRANCH)    /* No choice. */
           {
              next = OPERAND(scan);   /* Avoid recursion. */
              break;
           }

           do {
              SAVE_INPUT(data, save) ;
              if (regmatch(data, OPERAND(scan)))
                 return(1);
              RESTORE_INPUT(data, save) ;
              scan = regnext(scan);
           } while (scan != NULL && OP(scan) == BRANCH);

           return 0;
           /* NOTREACHED */

     case STAR:
     case PLUS: {
          int no;
          const char nextch = OP(next) == EXACTLY ? *OPERAND(next) : '\0';
          const int min = OP(scan) != STAR ; /* 0 for '*', 1 for '+' */

          SAVE_INPUT(data, save) ;
          /*
           * Lookahead to avoid useless match attempts
           * when we know what character comes next.
           */
          no = regrepeat(data, OPERAND(scan));
          while (no >= min) {
             /* If it could work, try it. */
             if ((nextch == '\0' || *data->reginput == nextch) && regmatch(data, next))
                return(1);
             /* Couldn't or didn't -- back up. */
             RESTORE_INPUT(data, save) ;
             data->reginput += --no ;
          }
        }
        return 0;

     default: { /* Open/close parenthesis */
           const char * const save = data->reginput;

           if (!regmatch(data, next))
              return 0;

           if (INRANGE(opscan, OPEN, OPEN+MAXNUMEXP-1))
           {
              const int no = OP(scan) - OPEN;
              /*
               * Don't set startp if some later invocation of the same parentheses
               * already has.
               */
              if (!data->regsubexps[no].start)
                 data->regsubexps[no].start = save;
           }
           else if (INRANGE(opscan, CLOSE, CLOSE+MAXNUMEXP-1))
           {
              const int no = OP(scan) - CLOSE;
              /*
               * Don't set endp if some later invocation of the same parentheses
               * already has.
               */
              if (!data->regsubexps[no].end)
                 data->regsubexps[no].end = save;
           }
           else // unknown opcode
              FAIL_COMPLETELY("pcomn_regexec(): memory corruption");
        }
        return 1;
     }
   }

   /*
    * We get here only if there's trouble -- normally "case END" is
    * the terminating point.
    */
   FAIL_COMPLETELY("pcomn_regexec(): corrupt pointers");
   return 0;
}

/*
 - regrepeat - repeatedly match something simple, report how many
 */
static int regrepeat(regexec_data *data, const char *p)
{
   int count = 0;
   const char * const opnd = OPERAND(p);

   switch (OP(p)) {
   case ANY:
     while (!END_INPUT(data)) {
        ++count;
        INC_INPUT(data);
     }
     break;
   case EXACTLY:
     while (*opnd == *data->reginput) {
        ++count;
        INC_INPUT(data);
     }
     break;
   case ANYOF:
     while (!END_INPUT(data) && strchr(opnd, *data->reginput)) {
        ++count;
        INC_INPUT(data);
     }
     break;
   case ANYBUT:
     while (!END_INPUT(data) && !strchr(opnd, *data->reginput)) {
        ++count;
        INC_INPUT(data);
     }
     break;
   default:      /* Oh dear.  Called inappropriately. */
     FAIL_COMPLETELY("pcomn_regexec(): internal foulup");
   }

   return count;
}

/*
 - regnext - dig the "next" pointer out of a node
 */
static char *regnext(const char *p)
{
   int offset;

   if (p == &regdummy)
     return(NULL);

   offset = NEXT(p);
   if (offset == 0)
     return(NULL);

   return (char *)
      ((OP(p) == BACK) ?
            (p-offset) :
            (p+offset)) ;
}


/*
 - pcomn_regdump - dump a regexp onto stdout in vaguely comprehensible form
 */
void pcomn_regdump(const regexp *r)
{
   const char *s;
   char op = EXACTLY;  /* Arbitrary non-END op. */
   const char *next;


   s = r->program + 1;
   while (op != END) { /* While that wasn't END last time... */
     op = OP(s);
     printf("%2lu%s", (unsigned long)(s-r->program), regprop(s));   /* Where, what. */
     next = regnext(s);
     if (next == NULL)    /* Next ptr. */
        printf("(0)");
     else
        printf("(%lu)", (unsigned long)((s-r->program)+(next-s)));
     s += 3;
     if (op == ANYOF || op == ANYBUT || op == EXACTLY) {
        /* Literal string, where present. */
        while (*s != '\0') {
           putchar(*s);
           s++;
        }
        s++;
     }
     putchar('\n');
   }

   /* Header fields of interest. */
   if (r->regstart != '\0')
     printf("start `%c' ", r->regstart);
   if (r->reganch)
     printf("anchored ");
   if (r->regmust != NULL)
     printf("must have \"%s\"", r->regmust);
   printf("\n");
}

/*
 - regprop - printable representation of opcode
 */
static const char *regprop(const char *op)
{
   char *p;
   static char buf[50];

   (void) strcpy(buf, ":");

   switch (OP(op)) {
   case BOL:
     p = "BOL";
     break;
   case EOL:
     p = "EOL";
     break;
   case ANY:
     p = "ANY";
     break;
   case ANYOF:
     p = "ANYOF";
     break;
   case ANYBUT:
     p = "ANYBUT";
     break;
   case BRANCH:
     p = "BRANCH";
     break;
   case EXACTLY:
     p = "EXACTLY";
     break;
   case NOTHING:
     p = "NOTHING";
     break;
   case BACK:
     p = "BACK";
     break;
   case END:
     p = "END";
     break;
   case STAR:
     p = "STAR";
     break;
   case PLUS:
     p = "PLUS";
     break;
   default:
      // Open/close parenthesis
      if (INRANGE(OP(op), OPEN, OPEN+MAXNUMEXP-1))
      {
         sprintf(buf+strlen(buf), "OPEN%d", OP(op)-OPEN) ;
         p = NULL ;
      }
      else if (INRANGE(OP(op), CLOSE, CLOSE+MAXNUMEXP-1))
      {
         sprintf(buf+strlen(buf), "CLOSE%d", OP(op)-CLOSE);
         p = NULL;
      }
      else
         FAIL_COMPLETELY("pcomn_regdump(): corrupted opcode");
     break;
   }
   if (p != NULL)
     (void) strcat(buf, p);
   return(buf);
}

void errh(PRegError errcode, const char *err, regcomp_data *data)
{
   if (data->handler)
   {
      if (data->result)
      {
         free (data->result) ;
         data->result = NULL ;
      }
      data->handler(errcode, err, data->exp, data->regparse) ;
   }
   data->errcode = errcode ;
}

void pcomn_regerror(PRegError errcode, const char *s, const char *exp, const char *pos)
{
   fprintf(stderr, "Regerror: %s (code %d)  Regexp: \"%s\"  At pos: %ld\n", s, errcode, exp, (long)(pos-exp));
}
