/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   test_type_traits.cpp
 COPYRIGHT    :   Yakov Markovitch, 2006-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Test type traits from pcomn_meta.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   15 Nov 2006
*******************************************************************************/
#include <pcomn_meta.h>
#include <pcomn_algorithm.h>
#include <pcomn_unittest.h>
#include <pcommon.h>
#include <iostream>

#define PRINT_MUTUAL_TRAIT(t, u, op)                                           \
   '\'' << PCOMN_TYPENAME(t) << "' is" << (op<t, u>::value ? " " : " not ") \
   << #op << " '" << PCOMN_TYPENAME(u) << '\''


template<typename T, typename U>
void print_mutual_traits(std::ostream &os)
{
   os << PCOMN_TYPENAME(T) << " is" << (std::is_same<T, U>::value ? " " : " not ")
       << PCOMN_TYPENAME(U) << std::endl ;
   os << PRINT_MUTUAL_TRAIT(T, U, std::is_convertible) << std::endl ;
   os << PRINT_MUTUAL_TRAIT(T, U, std::is_base_of) << std::endl ;
   os << PRINT_MUTUAL_TRAIT(T, U, pcomn::is_base_of_strict) << std::endl ;
   os << std::endl ;
}

class Foo {} ;
class Bar {} ;
class Quux : private Foo { public: operator Bar() ; } ;
class FooBar : public Foo, public Bar {} ;

template<typename T>
class Hello {} ;

int main(int, char *[])
{
   print_mutual_traits<void, void>(std::cout) ;
   print_mutual_traits<void, int>(std::cout) ;
   print_mutual_traits<int, void>(std::cout) ;
   print_mutual_traits<int, long>(std::cout) ;
   print_mutual_traits<long, int>(std::cout) ;
   print_mutual_traits<Foo, FooBar>(std::cout) ;
   print_mutual_traits<FooBar, Foo>(std::cout) ;
   print_mutual_traits<Foo, Bar>(std::cout) ;

   CPPUNIT_LOG(PRINT_MUTUAL_TRAIT(const char *, std::string, std::is_convertible) << std::endl) ;
   CPPUNIT_LOG(PRINT_MUTUAL_TRAIT(std::string, const char *, std::is_convertible) << std::endl) ;
   CPPUNIT_LOG(PRINT_MUTUAL_TRAIT(const char *, const std::string &, std::is_convertible) << std::endl) ;
   CPPUNIT_LOG(PRINT_MUTUAL_TRAIT(short, unsigned long long, std::is_convertible) << std::endl) ;
   CPPUNIT_LOG(PRINT_MUTUAL_TRAIT(unsigned long long, short, std::is_convertible) << std::endl) ;

   CPPUNIT_LOG(PRINT_MUTUAL_TRAIT(std::vector<const char *>, std::vector<std::string>, std::is_convertible) << std::endl) ;

//   print_mutual_traits<Quux, Foo>(std::cout) ;
   std::cout << std::endl ;
   CPPUNIT_LOG_EXPRESSION(PCOMN_TYPENAME(Hello<pcomn::parmtype_t<int> >)) ;
   CPPUNIT_LOG_EXPRESSION(PCOMN_TYPENAME(Hello<pcomn::parmtype_t<void *> >)) ;
   CPPUNIT_LOG_EXPRESSION(PCOMN_TYPENAME(Hello<pcomn::parmtype_t<Foo> >)) ;
   CPPUNIT_LOG_EXPRESSION(PCOMN_TYPENAME(Hello<pcomn::parmtype_t<Foo &> >)) ;
   CPPUNIT_LOG_EXPRESSION(PCOMN_TYPENAME(Hello<pcomn::parmtype_t<const Foo &> >)) ;
   CPPUNIT_LOG_EXPRESSION(PCOMN_TYPENAME(Hello<pcomn::parmtype_t<Foo *> >)) ;
   CPPUNIT_LOG_EXPRESSION(PCOMN_TYPENAME(Hello<pcomn::parmtype_t<const Foo *> >)) ;
   CPPUNIT_LOG_EXPRESSION(PCOMN_TYPENAME(Hello<pcomn::parmtype_t<Foo * const *> >)) ;
   CPPUNIT_LOG_EXPRESSION(PCOMN_TYPENAME(Hello<pcomn::parmtype_t<int(*)(int, double)>>)) ;
   CPPUNIT_LOG_EXPRESSION(std::is_member_function_pointer<int(*)()>::value) ;
   CPPUNIT_LOG_EXPRESSION(std::is_member_function_pointer<int(Foo::*)()>::value) ;
   CPPUNIT_LOG_EXPRESSION(std::is_member_function_pointer<int(Foo::*)(double)>::value) ;
   CPPUNIT_LOG_EXPRESSION(std::is_member_function_pointer<int(Foo::*)(double) const>::value) ;
   return 0 ;
}
