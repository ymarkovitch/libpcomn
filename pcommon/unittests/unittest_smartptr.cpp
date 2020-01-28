/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_smartptr.cpp
 COPYRIGHT    :   Yakov Markovitch, 2007-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unit tests of smartpointers.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   28 Mar 2007
*******************************************************************************/
#include <pcomn_smartptr.h>
#include <pcomn_unittest.h>
#include <pcomn_function.h>

#include <functional>
#include <iostream>
#include <memory>
#include <string>

struct LifetimeRegister {
      LifetimeRegister() :
         ptr(),
         constructed(),
         destructed()
      {}

      void construct(void *object)
      {
         CPPUNIT_ASSERT(object) ;
         CPPUNIT_ASSERT(!ptr) ;
         CPPUNIT_ASSERT(!constructed) ;
         ptr = object ;
         constructed = true ;

      }
      void destruct()
      {
         CPPUNIT_ASSERT(ptr) ;
         CPPUNIT_ASSERT(constructed) ;
         CPPUNIT_ASSERT(!destructed) ;
         destructed = true ;
      }

      void *ptr ;
      bool  constructed ;
      bool  destructed ;
} ;

/*******************************************************************************
                     class IntrusiveSmartPtrTests
*******************************************************************************/
class IntrusiveSmartPtrTests : public CppUnit::TestFixture {

      void Test_Constructors() ;
      void Test_Wrapper() ;
      void Test_Bind_ThisPtr() ;

      CPPUNIT_TEST_SUITE(IntrusiveSmartPtrTests) ;

      CPPUNIT_TEST(Test_Constructors) ;
      CPPUNIT_TEST(Test_Wrapper) ;
      CPPUNIT_TEST(Test_Bind_ThisPtr) ;

      CPPUNIT_TEST_SUITE_END() ;

   public:
      class Foo : public pcomn::PRefCount {
         public:
            Foo(LifetimeRegister &reg) :
               _reg(reg),
               incval()
            {
               _reg.construct(this) ;
            }

            ~Foo()
            {
               _reg.destruct() ;
            }

            int increment() { return ++incval ; }

            LifetimeRegister &_reg ;
            int incval ;
      } ;

      class Bar : public Foo {
         public:
            Bar(LifetimeRegister &reg) :
               Foo(reg)
            {}
      } ;

      class Quux : public Foo {
         public:
            Quux(LifetimeRegister &reg) :
               Foo(reg)
            {}
      } ;

      class WrapTester : public Foo {
         public:
            WrapTester(int inc, LifetimeRegister &reg) :
               Foo(reg)
            {
               incval = inc ;
            }

            ~WrapTester() { incval = 0 ; }

            int append(int v) const { return v + incval ; }
      } ;
} ;

/*******************************************************************************
                     class CustomPolicySmartPtrTests
*******************************************************************************/
class CustomPolicySmartPtrTests : public CppUnit::TestFixture {

      void Test_Constructors() ;
      void Test_Bind_ThisPtr() ;

      CPPUNIT_TEST_SUITE(CustomPolicySmartPtrTests) ;

      CPPUNIT_TEST(Test_Constructors) ;
      CPPUNIT_TEST(Test_Bind_ThisPtr) ;

      CPPUNIT_TEST_SUITE_END() ;

   public:
      class Foo {
         public:
            Foo(LifetimeRegister &reg) :
               _reg(reg),
               _counter(0),
               _incval(0)
            {
               _reg.construct(this) ;
            }

            int counter() const { return _counter ; }

            int increment() { return ++_incval ; }
            int append(int v) { return _incval += v ; }

         private:
            LifetimeRegister &_reg ;
            int               _counter ;
            int               _incval ;

            ~Foo() { _reg.destruct() ; }
            // This firiendship is necessary to allow deleting Foo (note ~Foo() is private)
            friend pcomn::refcount_basic_policy<Foo, int, &Foo::_counter> ;

         public:
            // This typedef is needed to use Foo::_counter - it is private and so this
            // the only way to use it as refcount_basic_policy argument
            typedef pcomn::refcount_basic_policy<Foo, int, &Foo::_counter> refcount_policy ;
      } ;
} ;

namespace pcomn {
template<> struct refcount_policy<CustomPolicySmartPtrTests::Foo> : CustomPolicySmartPtrTests::Foo::refcount_policy {} ;
} // end of namespace pcomn

/*******************************************************************************
 IntrusiveSmartPtrTests
*******************************************************************************/
void IntrusiveSmartPtrTests::Test_Constructors()
{
   LifetimeRegister foo_reg ;
   LifetimeRegister bar_reg ;
   LifetimeRegister quux_reg ;

   pcomn::shared_intrusive_ptr<Foo> foo (new Foo(foo_reg)) ;
   pcomn::shared_intrusive_ptr<Foo> bar (new Bar(bar_reg)) ;

   auto quux (sptr_cast(new Quux(quux_reg))) ;
   pcomn::shared_intrusive_ptr<Foo> quux1 (quux) ;

   CPPUNIT_LOG_EQUAL(typeid(quux), typeid(pcomn::shared_intrusive_ptr<Quux>)) ;
   CPPUNIT_LOG_ASSERT(quux.get()) ;

   CPPUNIT_LOG_IS_TRUE(foo_reg.constructed) ;
   CPPUNIT_LOG_IS_FALSE(foo_reg.destructed) ;
   CPPUNIT_LOG_EQ(foo.instances(), 1) ;
   CPPUNIT_LOG_IS_TRUE(bar_reg.constructed) ;
   CPPUNIT_LOG_IS_FALSE(bar_reg.destructed) ;
   CPPUNIT_LOG_EQ(bar.instances(), 1) ;
   CPPUNIT_LOG_IS_TRUE(quux_reg.constructed) ;
   CPPUNIT_LOG_IS_FALSE(quux_reg.destructed) ;
   CPPUNIT_LOG_EQ(quux.instances(), 2) ;
   CPPUNIT_LOG_EQ(quux1.instances(), 2) ;
   CPPUNIT_LOG_EQUAL(static_cast<Foo *>(&*quux), &*quux1) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(foo = bar) ;
   CPPUNIT_LOG_IS_TRUE(foo_reg.destructed) ;
   CPPUNIT_LOG_EQ(foo.instances(), 2) ;
   CPPUNIT_LOG_EQ(bar.instances(), 2) ;
   CPPUNIT_LOG_EQUAL(foo, bar) ;
   CPPUNIT_LOG_RUN(bar = foo) ;
   CPPUNIT_LOG_IS_FALSE(bar_reg.destructed) ;
   CPPUNIT_LOG_EQ(foo.instances(), 2) ;
   CPPUNIT_LOG_EQ(bar.instances(), 2) ;
   CPPUNIT_LOG_EQUAL(foo, bar) ;
   CPPUNIT_LOG_RUN(foo = NULL) ;
   CPPUNIT_LOG_IS_FALSE(foo) ;
   CPPUNIT_LOG_IS_FALSE(bar_reg.destructed) ;
   CPPUNIT_LOG_EQ(bar.instances(), 1) ;
   CPPUNIT_LOG_RUN(bar = foo) ;
   CPPUNIT_LOG_IS_FALSE(bar) ;
   CPPUNIT_LOG_IS_TRUE(bar_reg.destructed) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(foo = bar = quux) ;
   CPPUNIT_LOG_EQ(foo.instances(), 4) ;
   CPPUNIT_LOG_EQ(quux.instances(), 4) ;

   LifetimeRegister newbar_reg ;
   CPPUNIT_LOG_RUN(foo = new Bar(newbar_reg)) ;
   CPPUNIT_LOG_EQ(foo.instances(), 1) ;
   CPPUNIT_LOG_EQ(quux.instances(), 3) ;

   CPPUNIT_LOG(std::endl) ;
   LifetimeRegister constfoo_reg ;
   pcomn::shared_intrusive_ptr<const Foo> cfoo (new Foo(constfoo_reg)) ;
   CPPUNIT_LOG_IS_TRUE(constfoo_reg.constructed) ;
   CPPUNIT_LOG_IS_FALSE(constfoo_reg.destructed) ;
   CPPUNIT_LOG_EQ(cfoo.instances(), 1) ;
   CPPUNIT_LOG_EQ(quux.instances(), 3) ;
   CPPUNIT_LOG_EQ((cfoo = quux).instances(), 4) ;
   CPPUNIT_LOG_IS_TRUE(constfoo_reg.destructed) ;
}

void IntrusiveSmartPtrTests::Test_Wrapper()
{
   using namespace std::placeholders ;
   static const int initdata[] = { 3, 11, 16 } ;
   {
      LifetimeRegister lifereg ;
      std::vector<int> vt (P_ARRAY_BEGIN(initdata), P_ARRAY_END(initdata)) ;
      CPPUNIT_LOG_RUN
         (std::transform(vt.begin(), vt.end(), vt.begin(),
                         std::bind1st(std::mem_fun(&WrapTester::append),
                                      pcomn::sptr(pcomn::shared_intrusive_ptr<const WrapTester>(new WrapTester(7, lifereg)))))) ;

      CPPUNIT_LOG_IS_TRUE(lifereg.constructed) ;
      CPPUNIT_LOG_IS_TRUE(lifereg.destructed) ;
      CPPUNIT_LOG_EQUAL(vt, CPPUNIT_CONTAINER(std::vector<int>, (10)(18)(23))) ;
   }
   {
      LifetimeRegister lifereg ;
      std::vector<int> vt (P_ARRAY_BEGIN(initdata), P_ARRAY_END(initdata)) ;
      CPPUNIT_LOG_RUN
         (std::transform(vt.begin(), vt.end(), vt.begin(),
                         std::bind(&WrapTester::append,
                                        pcomn::sptr(pcomn::shared_intrusive_ptr<const WrapTester>(new WrapTester(7, lifereg))),
                                        _1))) ;

      CPPUNIT_LOG_IS_TRUE(lifereg.constructed) ;
      CPPUNIT_LOG_IS_TRUE(lifereg.destructed) ;
      CPPUNIT_LOG_EQUAL(vt, CPPUNIT_CONTAINER(std::vector<int>, (10)(18)(23))) ;
   }
}

void IntrusiveSmartPtrTests::Test_Bind_ThisPtr()
{
   {
      LifetimeRegister lifereg ;
      pcomn::shared_intrusive_ptr<WrapTester> wt (new WrapTester(5, lifereg)) ;

      std::function<int()> fn = pcomn::bind_thisptr(&WrapTester::increment, wt) ;

      CPPUNIT_LOG_RUN(wt = NULL) ;
      CPPUNIT_LOG_IS_TRUE(lifereg.constructed) ;
      CPPUNIT_LOG_IS_FALSE(lifereg.destructed) ;
      CPPUNIT_LOG_EQUAL(fn(), 6) ;
      CPPUNIT_LOG_EQUAL(fn(), 7) ;
      CPPUNIT_LOG_RUN(fn = std::function<int()>()) ;
      CPPUNIT_LOG_IS_TRUE(lifereg.destructed) ;
      CPPUNIT_LOG_IS_TRUE(lifereg.constructed) ;
   }
   {
      LifetimeRegister lifereg ;
      pcomn::shared_intrusive_ptr<WrapTester> wt (new WrapTester(2, lifereg)) ;

      std::function<int(int)> fn = pcomn::bind_thisptr(&WrapTester::append, wt) ;

      CPPUNIT_LOG_RUN(wt = NULL) ;
      CPPUNIT_LOG_IS_TRUE(lifereg.constructed) ;
      CPPUNIT_LOG_IS_FALSE(lifereg.destructed) ;
      CPPUNIT_LOG_EQUAL(fn(10), 12) ;
      CPPUNIT_LOG_EQUAL(fn(30), 32) ;
      CPPUNIT_LOG_RUN(fn = std::function<int(int)>()) ;
      CPPUNIT_LOG_IS_TRUE(lifereg.destructed) ;
      CPPUNIT_LOG_IS_TRUE(lifereg.constructed) ;
   }
}

/*******************************************************************************
 CustomPolicySmartPtrTests
*******************************************************************************/
void CustomPolicySmartPtrTests::Test_Constructors()
{
   LifetimeRegister foo_reg ;
   LifetimeRegister bar_reg ;
   LifetimeRegister quux_reg ;

   pcomn::shared_intrusive_ptr<Foo> foo (new Foo(foo_reg)) ;
   pcomn::shared_intrusive_ptr<Foo> bar (new Foo(bar_reg)) ;
   pcomn::shared_intrusive_ptr<Foo> quux (new Foo(quux_reg)) ;
   pcomn::shared_intrusive_ptr<Foo> quux1 (quux) ;

   CPPUNIT_LOG_IS_TRUE(foo_reg.constructed) ;
   CPPUNIT_LOG_IS_FALSE(foo_reg.destructed) ;
   CPPUNIT_LOG_EQUAL(foo->counter(), 1) ;
   CPPUNIT_LOG_IS_TRUE(bar_reg.constructed) ;
   CPPUNIT_LOG_IS_FALSE(bar_reg.destructed) ;
   CPPUNIT_LOG_EQUAL(bar->counter(), 1) ;
   CPPUNIT_LOG_IS_TRUE(quux_reg.constructed) ;
   CPPUNIT_LOG_IS_FALSE(quux_reg.destructed) ;
   CPPUNIT_LOG_EQUAL(quux->counter(), 2) ;
   CPPUNIT_LOG_EQUAL(quux1->counter(), 2) ;
   CPPUNIT_LOG_EQUAL(static_cast<Foo *>(&*quux), &*quux1) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(foo = bar) ;
   CPPUNIT_LOG_IS_TRUE(foo_reg.destructed) ;
   CPPUNIT_LOG_EQUAL(foo->counter(), 2) ;
   CPPUNIT_LOG_EQUAL(bar->counter(), 2) ;
   CPPUNIT_LOG_EQUAL(foo, bar) ;
   CPPUNIT_LOG_RUN(bar = foo) ;
   CPPUNIT_LOG_IS_FALSE(bar_reg.destructed) ;
   CPPUNIT_LOG_EQUAL(foo->counter(), 2) ;
   CPPUNIT_LOG_EQUAL(bar->counter(), 2) ;
   CPPUNIT_LOG_EQUAL(foo, bar) ;
   CPPUNIT_LOG_RUN(foo = NULL) ;
   CPPUNIT_LOG_IS_FALSE(foo) ;
   CPPUNIT_LOG_IS_FALSE(bar_reg.destructed) ;
   CPPUNIT_LOG_EQUAL(bar->counter(), 1) ;
   CPPUNIT_LOG_RUN(bar = foo) ;
   CPPUNIT_LOG_IS_FALSE(bar) ;
   CPPUNIT_LOG_IS_TRUE(bar_reg.destructed) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(foo = bar = quux) ;
   CPPUNIT_LOG_EQUAL(foo->counter(), 4) ;
   CPPUNIT_LOG_EQUAL(quux->counter(), 4) ;

   LifetimeRegister newfoo_reg ;
   CPPUNIT_LOG_RUN(foo = new Foo(newfoo_reg)) ;
   CPPUNIT_LOG_EQUAL(foo->counter(), 1) ;
   CPPUNIT_LOG_EQUAL(quux->counter(), 3) ;

   CPPUNIT_LOG(std::endl) ;
   LifetimeRegister constfoo_reg ;
   pcomn::shared_intrusive_ptr<const Foo> cfoo (new Foo(constfoo_reg)) ;
   CPPUNIT_LOG_IS_TRUE(constfoo_reg.constructed) ;
   CPPUNIT_LOG_IS_FALSE(constfoo_reg.destructed) ;
   CPPUNIT_LOG_EQUAL(cfoo->counter(), 1) ;
   CPPUNIT_LOG_EQUAL(quux->counter(), 3) ;
   CPPUNIT_LOG_EQUAL((cfoo = quux)->counter(), 4) ;
   CPPUNIT_LOG_IS_TRUE(constfoo_reg.destructed) ;
}

void CustomPolicySmartPtrTests::Test_Bind_ThisPtr()
{
   {
      LifetimeRegister lifereg ;
      pcomn::shared_intrusive_ptr<Foo> wt (new Foo(lifereg)) ;

      std::function<int()> fn = pcomn::bind_thisptr(&Foo::increment, wt) ;

      CPPUNIT_LOG_RUN(wt = NULL) ;
      CPPUNIT_LOG_IS_TRUE(lifereg.constructed) ;
      CPPUNIT_LOG_IS_FALSE(lifereg.destructed) ;
      CPPUNIT_LOG_EQUAL(fn(), 1) ;
      CPPUNIT_LOG_EQUAL(fn(), 2) ;
      CPPUNIT_LOG_RUN(fn = std::function<int()>()) ;
      CPPUNIT_LOG_IS_TRUE(lifereg.destructed) ;
      CPPUNIT_LOG_IS_TRUE(lifereg.constructed) ;
   }
   {
      LifetimeRegister lifereg ;
      pcomn::shared_intrusive_ptr<Foo> wt (new Foo(lifereg)) ;

      std::function<int(int)> fn = pcomn::bind_thisptr(&Foo::append, wt) ;

      CPPUNIT_LOG_RUN(wt = NULL) ;
      CPPUNIT_LOG_IS_TRUE(lifereg.constructed) ;
      CPPUNIT_LOG_IS_FALSE(lifereg.destructed) ;
      CPPUNIT_LOG_EQUAL(fn(10), 10) ;
      CPPUNIT_LOG_EQUAL(fn(30), 40) ;
      CPPUNIT_LOG_RUN(fn = std::function<int(int)>()) ;
      CPPUNIT_LOG_IS_TRUE(lifereg.destructed) ;
      CPPUNIT_LOG_IS_TRUE(lifereg.constructed) ;
   }
}

/*******************************************************************************
                     class SmartRefTests
*******************************************************************************/
class SmartRefTests : public CppUnit::TestFixture {

      void Test_Constructors() ;

      CPPUNIT_TEST_SUITE(SmartRefTests) ;

      CPPUNIT_TEST(Test_Constructors) ;

      CPPUNIT_TEST_SUITE_END() ;

   public:
      class Foo : public pcomn::PRefCount {
         public:
            Foo(LifetimeRegister &reg) :
               _reg(reg)
            {
               _reg.construct(this) ;
            }

            ~Foo()
            {
               _reg.destruct() ;
            }

            LifetimeRegister &_reg ;
      } ;

      class Bar : public Foo {
         public:
            Bar(LifetimeRegister &reg) :
               Foo(reg)
            {}
      } ;

      class Quux {
         public:
            Quux() :
               a(1),
               b(2)
            {}

            Quux(int aa) :
               a(aa),
               b(2)
            {}

            Quux(int aa, int bb) :
               a(aa),
               b(bb)
            {}

            int a ;
            int b ;
      } ;
} ;

void SmartRefTests::Test_Constructors()
{
   LifetimeRegister foo_reg ;
   LifetimeRegister bar_reg ;

   pcomn::shared_ref<Foo> foo {std::ref(foo_reg)} ;
   pcomn::shared_ref<Foo> bar {std::ref(bar_reg)} ;

   pcomn::shared_ref<Quux> quux12 ;

   pcomn::shared_ref<Quux> quux52 (5) ;

   pcomn::shared_ref<Quux> quux67 (6, 7) ;

   CPPUNIT_LOG_IS_TRUE(foo_reg.constructed) ;
   CPPUNIT_LOG_IS_FALSE(foo_reg.destructed) ;
   CPPUNIT_LOG_EQ(foo.instances(), 1) ;
   CPPUNIT_LOG_EQ(foo.get().instances(), 1) ;

   CPPUNIT_LOG_IS_TRUE(bar_reg.constructed) ;
   CPPUNIT_LOG_IS_FALSE(bar_reg.destructed) ;
   CPPUNIT_LOG_EQ(bar.instances(), 1) ;
   CPPUNIT_LOG_EQ(bar.get().instances(), 1) ;

   pcomn::shared_intrusive_ptr<Foo> foobar (bar) ;

   CPPUNIT_LOG_IS_TRUE(bar_reg.constructed) ;
   CPPUNIT_LOG_IS_FALSE(bar_reg.destructed) ;
   CPPUNIT_LOG_EQ(bar.instances(), 2) ;
   CPPUNIT_LOG_EQ(bar.get().instances(), 2) ;
   CPPUNIT_LOG_EQ(foobar.instances(), 2) ;

   CPPUNIT_LOG_EQ(quux12.instances(), 1) ;
   CPPUNIT_LOG_EQ(quux52.instances(), 1) ;
   CPPUNIT_LOG_EQ(quux67.instances(), 1) ;

   CPPUNIT_LOG_EQUAL(quux12.get().a, 1) ;
   CPPUNIT_LOG_EQUAL(quux12.get().b, 2) ;
   CPPUNIT_LOG_EQUAL(quux52.get().a, 5) ;
   CPPUNIT_LOG_EQUAL(quux52.get().b, 2) ;
   CPPUNIT_LOG_EQUAL(quux67.get().a, 6) ;
   CPPUNIT_LOG_EQUAL(quux67.get().b, 7) ;

   CPPUNIT_LOG_EQUAL(pcomn::shared_ref<int>(55), pcomn::shared_ref<int>(55)) ;
   CPPUNIT_LOG_ASSERT(pcomn::shared_ref<int>(51) < pcomn::shared_ref<int>(55)) ;
   CPPUNIT_LOG_ASSERT(pcomn::shared_ref<int>(66) > pcomn::shared_ref<int>(55)) ;

   pcomn::shared_ref<const Quux> quux90 (9, 0) ;
   CPPUNIT_LOG_EQUAL(quux90.get().a, 9) ;
   CPPUNIT_LOG_EQ(quux67.instances(), 1) ;
   CPPUNIT_LOG_RUN(quux90 = quux67) ;
   CPPUNIT_LOG_EQUAL(quux90.get().a, 6) ;
   CPPUNIT_LOG_EQ(quux67.instances(), 2) ;
   CPPUNIT_LOG_EQ(quux90.instances(), 2) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(IntrusiveSmartPtrTests::suite()) ;
   runner.addTest(CustomPolicySmartPtrTests::suite()) ;
   runner.addTest(SmartRefTests::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv,
                             "unittest.diag.ini", "Test smartpointers.") ;
}
