/*-*- mode: c; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel" -*-*/
#ifndef __PREGUTIL_H
#define __PREGUTIL_H
/*******************************************************************************
 FILE         :   pregutil.h
 COPYRIGHT    :   Yakov Markovitch, 1998-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Some useful regular expressions.

 CREATION DATE:   14 Apr 1998
*******************************************************************************/
#ifndef __PCOMMON_H
#include <pcommon.h>
#endif /* PCOMMON.H */

#define RSUB(n)      "("n")"                          // Подвыражение в скобках

#define RCCLASS(n)   "["n"]"                          // Класс символов

#define RDECDIGIT    "[0-9]"                          // Десятичные цифры
#define ROCTDIGIT    "[0-7]"                          // Восьмеричные цифры
#define RHEXDIGIT    "[0-9A-Fa-f]"                    // Шестнадцатеричные цифры
#define RDECNOZERO   "[1-9]"                          // Десятичные цифры без нуля
#define ROCTNOZERO   "[1-7]"                          // Восьмеричные цифры без нуля
#define RHEXNOZERO   "[1-9A-Fa-f]"                    // Шестнадцатеричные цифры без нуля
#define RDECNUM      "0|"RDECNOZERO RDECDIGIT"*"      // Десятичное число без знака
#define RHEXNUM      "0[Xx]"RHEXDIGIT"+"              // Шестнадцатеричное число без знака
#define ROCTNUM      "0"ROCTDIGIT"*"                  // Восьмеричное число без знака
#define RINTEGER     RDECNUM "|" RHEXNUM "|" ROCTNUM  // Целое число в любой форме (dec/oct/hex)
#define RSTRIPL      "[^ ].*"                         // Удалить лидирующие пробелы
#define RSTRIPR      ".*[^ ]"                         // Удалить хвостовые пробелы
#define RSTRIP       "[^ ].*[^ ]|[^ ]"                // Удалить пробелы слева и справа


#ifndef PCOMN_PL_AS400   // Мы должны учесть особенности EBCDIC-кодировки

#  define RSEQALPHA    "A-Za-z"                         // Латинница - все буквы
#  define RSEQALNUM    "0-9A-Za-z"                      // Латинница - все буквы и цифры
#  define RSEQUPPER    "A-Z"                            // Латинница - верхний регистр
#  define RSEQLOWER    "a-z"                            // Латинница - нижний регистр

#else // AS400/EBCDIC

#  define RSEQALPHA    "A-IJ-RS-Za-ij-rs-z"             // Латинница - все буквы
#  define RSEQALNUM    "0-9A-IJ-RS-Za-ij-rs-z"          // Латинница - все буквы и цифры
#  define RSEQUPPER    "A-IJ-RS-Z"                      // Латинница - верхний регистр
#  define RSEQLOWER    "a-ij-rs-z"                      // Латинница - нижний регистр

#endif

#define RALPHA  RCCLASS(RSEQALPHA)                       // Латинница - все буквы
#define RALNUM  RCCLASS(RSEQALNUM)                       // Латинница - все буквы и цифры
#define RUPPER  RCCLASS(RSEQUPPER)                       // Латинница - верхний регистр
#define RLOWER  RCCLASS(RSEQLOWER)                       // Латинница - нижний регистр

#define RCNAME  RCCLASS(RSEQALPHA"_")RCCLASS(RSEQALNUM"_")"*"  // Имя (в стиле языка программирования)

#endif /* __PREGUTIL_H */
