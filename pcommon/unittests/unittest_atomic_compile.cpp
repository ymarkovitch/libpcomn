/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_atomic_compile.cpp
 COPYRIGHT    :   Yakov Markovitch, 2009-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   This file should compile OK on both 32- and 64-bit systems

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   17 Feb 2009
*******************************************************************************/
#include <pcomn_atomic.h>

template<typename T>
T atomic_compile_test()
{
   T value = T() ;
   return pcomn::atomic_op::inc(&value) ;
}

void atomic_compile()
{
   atomic_compile_test<int32_t>() ;
   atomic_compile_test<uint32_t>() ;
   atomic_compile_test<int64_t>() ;
   atomic_compile_test<uint64_t>() ;
   atomic_compile_test<void *>() ;
   atomic_compile_test<int *>() ;
}

int main(int, char *[])
{
   atomic_compile() ;
   return 0 ;
}
