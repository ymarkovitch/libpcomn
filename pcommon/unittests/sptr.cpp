/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#include <pcomn_smartptr.h>
#include <iostream>

struct Foo : public pcomn::PRefCount {
      Foo(const char *s) : _str(s) {}
      const char * const _str ;
} ;

__noinline void p(const pcomn::shared_intrusive_ptr<Foo> &foo)
{
   std::cout << foo->_str << &foo << std::endl ;
}

__noinline void pp(const pcomn::shared_intrusive_ptr<Foo> &foo)
{
   std::cout << &foo << foo->_str << std::endl ;
}

decltype (&p) pv ;

int main(int argc, char *argv[])
{
   pv = argc > 10 ? &pp : &p ;
   pcomn::shared_intrusive_ptr<Foo> pf (new Foo(*argv)) ;
   pv(pf) ;
   return 0 ;
}
