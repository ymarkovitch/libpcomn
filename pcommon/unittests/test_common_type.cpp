/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#include <pcommon.h>
#include <typeinfo>
#include <type_traits>
#include <iostream>

#include <stdio.h>

#define PRINT_COMMON(...) \
   std::cout << "The common type of " << #__VA_ARGS__ << " is " << PCOMN_TYPENAME(P_PASS(std::common_type_t<__VA_ARGS__>)) << std::endl

#define PRINT_TYPE(...) \
   std::cout << '(' << #__VA_ARGS__ << ") -> " << PCOMN_TYPENAME(decltype(__VA_ARGS__)) << std::endl

int main(int argc, char *argv[])
{
   PRINT_COMMON(int8_t, uint64_t) ;
   PRINT_COMMON(uint64_t, double) ;
   PRINT_COMMON(int64_t, double) ;
   PRINT_TYPE(std::declval<int8_t>() + std::declval<uint64_t>()) ;
   PRINT_TYPE(std::declval<int8_t>() + std::declval<int8_t>()) ;
   PRINT_TYPE(std::declval<int8_t>() + std::declval<uint8_t>()) ;
   PRINT_TYPE(std::declval<int8_t>() + std::declval<uint16_t>()) ;
   PRINT_TYPE(std::declval<int32_t>() + std::declval<int32_t>()) ;
   PRINT_TYPE(std::declval<int32_t>() + std::declval<uint32_t>()) ;

   return 0 ;
}
