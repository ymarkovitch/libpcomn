/*-*- tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel" -*-*/
/*******************************************************************************
 FILE         :   pcomn_hash.c
 COPYRIGHT    :   Yakov Markovitch, 2000-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Hash functions.
                  Got from "Hashing Rehashed" article of Andrew Binstock (DDJ Apr 96)

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   10 Jan 2000
*******************************************************************************/
#include <pcomn_hash.h>
#include <limits.h>

/* hashPJW
 *
 *  An adaptation of Peter Weinberger's (PJW) generic hashing
 *  algorithm based on Allen Holub's version. Accepts a pointer
 *  to a datum to be hashed and returns an unsigned integer.
 */

#define BITS_IN_int     (sizeof(int) * CHAR_BIT)
#define THREE_QUARTERS  ((int) ((BITS_IN_int * 3) / 4))
#define ONE_EIGHTH      ((int) (BITS_IN_int / 8))
#define HIGH_BITS       (~((unsigned int)(~0) >> ONE_EIGHTH))

unsigned int hashPJWstr(const char *datum)
{
   unsigned int hash_value, i ;
   for (hash_value = 0 ; *datum ; ++datum)
   {
      hash_value = (hash_value << ONE_EIGHTH) + *datum ;
      if ((i = hash_value & HIGH_BITS) != 0)
         hash_value =
            (hash_value ^ (i >> THREE_QUARTERS)) &
            ~HIGH_BITS ;
   }
   return hash_value ;
}

unsigned int hashPJWmem(const void *datum, size_t size)
{
   unsigned int hash_value, i ;
   const char *current = (const char *)datum ;
   const char * const end = current + size ;
   for (hash_value = 0 ; current != end ; ++current)
   {
      hash_value = (hash_value << ONE_EIGHTH) + *current ;
      if ((i = hash_value & HIGH_BITS) != 0)
         hash_value =
            (hash_value ^ (i >> THREE_QUARTERS)) &
            ~HIGH_BITS ;
   }
   return hash_value ;
}

/* hashELF
 *
 * The hash algorithm used in the UNIX ELF format
 *  for object files. Accepts a pointer to a string to be hashed
 *  and returns an unsigned long.
 */
unsigned long hashELFstr(const char *name)
{
   unsigned long h = 0, g ;
   while (*name)
   {
      h = (h << 4) + *(const unsigned char *)name++ ;
      if ((g = h & 0xF0000000) != 0)
         h ^= g >> 24 ;
      h &= ~g ;
   }
   return h ;
}

unsigned long hashELFmem(const void *mem, size_t size)
{
   unsigned long h = 0, g ;
   size_t count = 0 ;
   while (count < size)
   {
      h = (h << 4) + *((const unsigned char *)mem + count++) ;
      if ((g = h & 0xF0000000) != 0)
         h ^= g >> 24 ;
      h &= ~g ;
   }
   return h ;
}
