/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   test_bitops.cpp
 COPYRIGHT    :   Yakov Markovitch, 2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Test bit operations

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   23 Mar 2016
*******************************************************************************/
#include <pcomn_bitops.h>
#include <pcommon.h>
#include <iostream>
#include <iomanip>
#include <stdlib.h>
#include <stddef.h>
#if defined(PCOMN_PL_UNIX) || defined(PCOMN_COMPILER_C99)
#include <inttypes.h>
#else
#include <stdint.h>
#endif

using namespace pcomn ;

template<typename T, typename I>
static void print(ptrdiff_t result, I source, const char *callname)
{
   std::cout
      << std::setw(24) << PCOMN_CLASSNAME(T) << ": "
      << std::setw(20) << callname << "((" << PCOMN_TYPENAME(source) << ')'
      << std::hex << "0x" << (uintmax_t)source << ") is " << std::dec << result
      << std::endl ;
}

template<typename T, typename I>
static void test(I value)
{
#define PRINT(callname) (print<T>(r, value, callname))

   static const T tag ;
   ptrdiff_t r ;

   r = native_bitcount(value, tag) ;
   PRINT("native_bitcount") ;

   #undef PRINT
}

int main(int argc, char *argv[])
{
   const uintmax_t source = argc > 1 ? strtoumax(argv[1], nullptr, 0) : 0 ;

   test<generic_isa_tag>((int8_t)source) ;
   test<sse42_isa_tag>((int8_t)source) ;

   std::cout << '\n' ;
   test<generic_isa_tag>((uint8_t)source) ;
   test<sse42_isa_tag>((uint8_t)source) ;

   std::cout << '\n' ;
   test<generic_isa_tag>((int16_t)source) ;
   test<sse42_isa_tag>((int16_t)source) ;

   std::cout << '\n' ;
   test<generic_isa_tag>((uint16_t)source) ;
   test<sse42_isa_tag>((uint16_t)source) ;

   std::cout << '\n' ;
   test<generic_isa_tag>((int32_t)source) ;
   test<sse42_isa_tag>((int32_t)source) ;

   std::cout << '\n' ;
   test<generic_isa_tag>((uint32_t)source) ;
   test<sse42_isa_tag>((uint32_t)source) ;

   std::cout << '\n' ;
   test<generic_isa_tag>((int64_t)source) ;
   test<sse42_isa_tag>((int64_t)source) ;

   std::cout << '\n' ;
   test<generic_isa_tag>((uint64_t)source) ;
   test<sse42_isa_tag>((uint64_t)source) ;
}
