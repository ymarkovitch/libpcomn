/*-*- mode: c; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel" -*-*/
#ifndef __PCOMN_MACROS_H
#define __PCOMN_MACROS_H
/*******************************************************************************
 FILE         :   pcomn_macros.h
 COPYRIGHT    :   Yakov Markovitch, 1996-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Helper macros for C preprocessor tricks and for writing other
                  macros

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   21 Sep 2005
*******************************************************************************/
/*
 * Empty argument for use as macro parameter
 */
#define P_EMPTY_ARG

#define P_EMPTY()

#define P_PASS(...) __VA_ARGS__

#ifdef _MSC_VER
#   define P_LPAREN (
#   define P_RPAREN )
#   define P_PASS_I(...) P_PASS P_LPAREN __VA_ARGS__ P_RPAREN
#else
#   define P_PASS_I(...) P_PASS(__VA_ARGS__)
#endif

/*
 * A comma for using in macro parameters to pass a parameter list as a single parameter
 */
#define P_COMMA ,
#define _J_ ,

/*
 * Stringifying macros
 */
#define P_STRINGIFY(arg) #arg
#define P_STRINGIFY_I(arg) P_STRINGIFY(arg)
#define P_STRINGIFY_INDIRECT(arg) P_STRINGIFY(arg)

/*
 * Concatenate arguments
 */
#define _P_CONCAT(arg1, arg2) arg1##arg2
/** Concatenate arguments. */
#define P_CONCAT(arg1, arg2) _P_CONCAT(arg1, arg2)
/** Alias for P_CONCAT. */
#define P_CAT(arg1, arg2) _P_CONCAT(arg1, arg2)

/** Get count of C array items. */
#define P_ARRAY_COUNT(a) ((sizeof a)/sizeof *(a))
/** Get the begin pointer (iterator) of a C array. */
#define P_ARRAY_BEGIN(a) ((a) + 0)
/** Get the end pointer (iterator) of a C array. */
#define P_ARRAY_END(a) ((a) + P_ARRAY_COUNT((a)))

/** Returns true if char pointer is either NULL or empty string.
 */
#define P_NULL_STR(s) (!(s) || !*(s))

/*
 * Macro returning factor to use as number of elements of array<elem>
 * so that size of memory array occupies is minimal possible to be no less then size given.
 */
#define P_MIN_FACTOR(size,elem_size) (((size)+(elem_size)-1)/(elem_size))

/*
 * Macro returning size of type or object in bits instead of bytes
 */
#define P_BIT_SIZEOF(a) (sizeof(a) * 8)

#define PCOMN_THREEWAY(lhs, rhs, vlt, veq, vgt) ((lhs) < (rhs) ? (vlt) : ((lhs) == (rhs) ? (veq) : (vgt)))

#define P_1ST(a1, ...) a1
#define P_2ND(a1, a2, ...) a2

/*******************************************************************************
 Arguments declaration macros
*******************************************************************************/
#define P_REMPAREN(a)      P_PASS a
#define P_REMCOMMA(a, ...) a __VA_ARGS__
#define P_ARGDECL(a)       P_REMCOMMA a
#define P_ARGTYPE(a)       P_1ST a
#define P_ARGNAME(a)       P_2ND a

/*******************************************************************************
 Multi-argument appliers
*******************************************************************************/
#define P_APPLY(macro, nargs, ...) P_APPLY_##nargs##_(macro, __VA_ARGS__)

#define P_APPL1(macro, arg, nargs, ...) P_APPL1_##nargs##_(macro, arg, __VA_ARGS__)

#define P_FOR(count, macro, ...) P_FOR_##count##_(macro, __VA_ARGS__)

#define P_TARGLIST(count, ...)     P_FOR(count, P_TARG_, __VA_ARGS__)

#define P_TREFARGLIST(count, ...)  P_FOR(count, P_TREFARG_, __VA_ARGS__)

#define P_TPTRARGLIST(count, ...)  P_FOR(count, P_TPTRARG_, __VA_ARGS__)

#define P_TPARMLIST(count, ...)    P_FOR(count, P_TPARAM_, __VA_ARGS__)

#define P_REF_ARGLIST(count, ...)  P_FOR(count, P_REFARG_, __VA_ARGS__)

#define P_PTR_ARGLIST(count, ...)  P_FOR(count, P_PTRARG_, __VA_ARGS__)

#define P_VAL_ARGLIST(count, ...)  P_FOR(count, P_VALARG_, __VA_ARGS__)

#define P_PARMLIST(count, ...)     P_FOR(count, P_PARAM_, __VA_ARGS__)

/*******************************************************************************
 Private macros
*******************************************************************************/
#define P_FOR_0_(macro, ...)
#define P_FOR_1_(macro, ...) macro(1, __VA_ARGS__)
#define P_FOR_2_(macro, ...) P_FOR_1_(macro, __VA_ARGS__), macro(2, __VA_ARGS__)
#define P_FOR_3_(macro, ...) P_FOR_2_(macro, __VA_ARGS__), macro(3, __VA_ARGS__)
#define P_FOR_4_(macro, ...) P_FOR_3_(macro, __VA_ARGS__), macro(4, __VA_ARGS__)
#define P_FOR_5_(macro, ...) P_FOR_4_(macro, __VA_ARGS__), macro(5, __VA_ARGS__)
#define P_FOR_6_(macro, ...) P_FOR_5_(macro, __VA_ARGS__), macro(6, __VA_ARGS__)
#define P_FOR_7_(macro, ...) P_FOR_6_(macro, __VA_ARGS__), macro(7, __VA_ARGS__)
#define P_FOR_8_(macro, ...) P_FOR_7_(macro, __VA_ARGS__), macro(8, __VA_ARGS__)
#define P_FOR_9_(macro, ...) P_FOR_8_(macro, __VA_ARGS__), macro(9, __VA_ARGS__)
#define P_FOR_10_(macro, ...) P_FOR_9_(macro, __VA_ARGS__), macro(10, __VA_ARGS__)

#define P_TARG_(num, ...)        P_PASS_I(__VA_ARGS__) P##num

#define P_TREFARG_(num, ...)  P_PASS_I(__VA_ARGS__) P##num &

#define P_TPRARG_(num, ...)   P_PASS_I(__VA_ARGS__) P##num *

#define P_TPARAM_(num, ...)   P_PASS_I(__VA_ARGS__)(P##num)

#define P_VALARG_(num, ...)   P_PASS_I(__VA_ARGS__) P##num p##num

#define P_REFARG_(num, ...)   P_PASS_I(__VA_ARGS__) P##num &p##num

#define P_PTRARG_(num, ...)   P_PASS_I(__VA_ARGS__) P##num *p##num

#define P_PARAM_(num, ...)    P_PASS_I(__VA_ARGS__)(p##num)

#define P_APPLY_0_(macro, ...)
#define P_APPLY_1_(macro, a1) macro(a1)
#define P_APPLY_2_(macro, a1, a2) P_APPLY_1_(macro, a1), P_APPLY_1_(macro, a2)
#define P_APPLY_3_(macro, a1, a2, a3) P_APPLY_2_(macro, a1, a2), P_APPLY_1_(macro, a3)
#define P_APPLY_4_(macro, a1, a2, a3, a4) P_APPLY_3_(macro, a1, a2, a3), P_APPLY_1_(macro, a4)
#define P_APPLY_5_(macro, a1, a2, a3, a4, a5) P_APPLY_4_(macro, a1, a2, a3, a4), P_APPLY_1_(macro, a5)

#define P_APPLY_6_(macro, a1, a2, a3, a4, a5, a6) \
   P_APPLY_5_(macro, a1, a2, a3, a4, a5), P_APPLY_1_(macro, a6)

#define P_APPLY_7_(macro, a1, a2, a3, a4, a5, a6, a7)                \
   P_APPLY_6_(macro, a1, a2, a3, a4, a5, a6), P_APPLY_1_(macro, a7)

#define P_APPLY_8_(macro, a1, a2, a3, a4, a5, a6, a7, a8) \
   P_APPLY_7_(macro, a1, a2, a3, a4, a5, a6, a7), P_APPLY_1_(macro, a8)

#define P_APPLY_9_(macro, a1, a2, a3, a4, a5, a6, a7, a8, a9) \
   P_APPLY_8_(macro, a1, a2, a3, a4, a5, a6, a7, a8), P_APPLY_1_(macro, a9)

#define P_APPLY_10_(macro, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) \
   P_APPLY_9_(macro, a1, a2, a3, a4, a5, a6, a7, a8, a9), P_APPLY_1_(macro, a10)

#define P_APPL1_1_(macro, a, a1) macro(a, a1)
#define P_APPL1_2_(macro, a, a1, a2) P_APPL1_1_(macro, a, a1), P_APPL1_1_(macro, a, a2)
#define P_APPL1_3_(macro, a, a1, a2, a3) P_APPL1_2_(macro, a, a1, a2), P_APPL1_1_(macro, a, a3)
#define P_APPL1_4_(macro, a, a1, a2, a3, a4) P_APPL1_3_(macro, a, a1, a2, a3), P_APPL1_1_(macro, a, a4)
#define P_APPL1_5_(macro, a, a1, a2, a3, a4, a5) P_APPL1_4_(macro, a, a1, a2, a3, a4), P_APPL1_1_(macro, a, a5)

#define P_APPL1_6_(macro, a, a1, a2, a3, a4, a5, a6) \
   P_APPL1_5_(macro, a, a1, a2, a3, a4, a5), P_APPL1_1_(macro, a, a6)

#define P_APPL1_7_(macro, a, a1, a2, a3, a4, a5, a6, a7)                \
   P_APPL1_6_(macro, a, a1, a2, a3, a4, a5, a6), P_APPL1_1_(macro, a, a7)

#define P_APPL1_8_(macro, a, a1, a2, a3, a4, a5, a6, a7, a8) \
   P_APPL1_7_(macro, a, a1, a2, a3, a4, a5, a6, a7), P_APPL1_1_(macro, a, a8)

#define P_APPL1_9_(macro, a, a1, a2, a3, a4, a5, a6, a7, a8, a9) \
   P_APPL1_8_(macro, a, a1, a2, a3, a4, a5, a6, a7, a8), P_APPL1_1_(macro, a, a9)

#define P_APPL1_10_(macro, a, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) \
   P_APPL1_9_(macro, a, a1, a2, a3, a4, a5, a6, a7, a8, a9), P_APPL1_1_(macro, a, a10)

/*******************************************************************************/
/** Workaround macro mechanism, imlemented after image of corresponding Boost machinery.

    @code
     #if PCOMN_WORKAROUND(_MSC_VER, <= 1200)
        ... // workaround code here
     #endif
    @endcode
   The first argument must be undefined or expand to a numeric value.
*******************************************************************************/
#define PCOMN_WORKAROUND(symbol, test) ((symbol != 0) && (symbol test))

#endif /* __PCOMN_MACROS_H */
