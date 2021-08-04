/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_atomic_compile.cpp
 COPYRIGHT    :   Yakov Markovitch, 2009-2020. All rights reserved.
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
   const T arg = T() ;
   pcomn::atomic_op::xchg(&value, arg) ;
   pcomn::atomic_op::cas(&value, value, arg) ;
   return value ;
}

template<typename T>
T atomic_arith_compile_test()
{
   atomic_compile_test<T>() ;

   T value = T() ;
   pcomn::atomic_op::preinc(&value) ;
   pcomn::atomic_op::postinc(&value) ;
   pcomn::atomic_op::predec(&value) ;
   pcomn::atomic_op::postdec(&value) ;
   pcomn::atomic_op::add(&value, 2) ;
   return
      pcomn::atomic_op::sub(&value, 2) ;
}

void atomic_compile()
{
   atomic_arith_compile_test<int32_t>() ;
   atomic_arith_compile_test<uint32_t>() ;
   atomic_arith_compile_test<int64_t>() ;
   atomic_arith_compile_test<uint64_t>() ;
   atomic_arith_compile_test<int *>() ;

   atomic_compile_test<void *>() ;
}

int main(int, char *[])
{
   atomic_compile() ;
   return 0 ;
}
