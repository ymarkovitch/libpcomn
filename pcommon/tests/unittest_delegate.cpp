/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_delegate.cpp
 COPYRIGHT    :   Yakov Markovitch, 2007-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittest for pcomn::delegate and pcomn::functor.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   16 Feb 2007
*******************************************************************************/
#include <pcomn_delegate.h>
#include <pcomn_immutablestr.h>
#include <pcomn_unittest.h>

static int statint ;
static double statdouble ;

/*******************************************************************************
                     class DelegateTest
*******************************************************************************/
class DelegateTest : public CppUnit::TestFixture {

      void Test_Delegate_Construction() ;
      void Test_Delegate_Call() ;

      CPPUNIT_TEST_SUITE(DelegateTest) ;

      CPPUNIT_TEST(Test_Delegate_Construction) ;
      CPPUNIT_TEST(Test_Delegate_Call) ;

      CPPUNIT_TEST_SUITE_END() ;

      static void reset()
      {
         statint = 0 ;
         statdouble = 0 ;
      }
   public:
      void setUp()
      {
         reset() ;
      }
} ;

int foo_int()
{
   return ++statint ;
}

double foo_double()
{
   return statdouble += 1.5 ;
}

void foo_void()
{
   ++statint ;
}

std::string foo_repeat(char c, unsigned rep)
{
   return std::string(rep, c) ;
}

const char *foo_hello()
{
   return "Hello, world!" ;
}

struct Foo {
      virtual ~Foo() {}
      virtual std::string repeat(char c, unsigned rep)
      {
         return ::foo_repeat(c, rep) ;
      }

      virtual double mul(double lhs, double rhs)
      {
         return lhs * rhs ;
      }
} ;

struct Bar : public Foo {
      std::string repeat(char c, unsigned rep)
      {
         return Foo::repeat(c, 2*rep) ;
      }
      virtual double mul(double lhs, double rhs)
      {
         return -Foo::mul(rhs, lhs) ;
      }
      PCOMN_WEAK_REFERENCEABLE(Bar) ;
} ;

/*******************************************************************************
 DelegateTest
*******************************************************************************/
void DelegateTest::Test_Delegate_Construction()
{
   pcomn::delegate<> FooEmpty1 ;
   pcomn::delegate<> FooEmpty2(NULL) ;
   pcomn::delegate<> FooVoid (&foo_void) ;
   pcomn::delegate<> FooVoid1 (FooVoid) ;
   Foo fooObject ;
   Bar barObject ;

   CPPUNIT_LOG_IS_TRUE(FooEmpty1.empty()) ;
   CPPUNIT_LOG_IS_FALSE(FooEmpty1) ;
   CPPUNIT_LOG_IS_TRUE(FooEmpty2.empty()) ;
   CPPUNIT_LOG_IS_FALSE(FooEmpty2) ;
   CPPUNIT_LOG_EXCEPTION(pcomn::delegate<>(2), std::invalid_argument) ;
   CPPUNIT_LOG_IS_FALSE(pcomn::delegate<>(FooEmpty2)) ;
   CPPUNIT_LOG("FooEmpty1=" << FooEmpty1 << std::endl << "FooVoid=" << FooVoid << std::endl) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_IS_FALSE(FooVoid.empty()) ;
   CPPUNIT_LOG_IS_TRUE(FooVoid) ;
   CPPUNIT_LOG_IS_TRUE(FooVoid1) ;

   CPPUNIT_LOG_IS_TRUE(pcomn::delegate<>(&foo_double)) ;
   CPPUNIT_LOG_IS_TRUE(pcomn::delegate<std::string(int, int)>(&foo_repeat)) ;
   CPPUNIT_LOG_IS_TRUE(pcomn::delegate<std::string()>(&foo_hello)) ;
   CPPUNIT_LOG(pcomn::delegate<std::string()>(&foo_hello) << std::endl) ;
   CPPUNIT_LOG_IS_TRUE(pcomn::delegate<std::string(int, int)>(&fooObject, &Foo::repeat)) ;
   CPPUNIT_LOG(pcomn::delegate<std::string(int, int)>(&fooObject, &Foo::repeat) << std::endl) ;
   CPPUNIT_LOG_IS_TRUE(pcomn::delegate<double(int, int)>(&barObject, &Bar::mul)) ;
   CPPUNIT_LOG(pcomn::delegate<double(int, int)>(&barObject, &Bar::mul) << std::endl) ;
   CPPUNIT_LOG_IS_TRUE(pcomn::delegate<double(int, int)>(&barObject, &Foo::mul)) ;
   CPPUNIT_LOG(pcomn::delegate<double(int, int)>(&barObject, &Foo::mul) << std::endl) ;
   CPPUNIT_LOG_IS_TRUE(pcomn::delegate<double(int, int)>(std::make_pair(&barObject, &Foo::mul))) ;
   CPPUNIT_LOG(pcomn::delegate<double(int, int)>(std::make_pair(&barObject, &Foo::mul)) << std::endl) ;
   CPPUNIT_LOG_IS_TRUE(pcomn::delegate<double(Foo *, int, int)>(&Foo::mul)) ;
   CPPUNIT_LOG(pcomn::delegate<double(Foo *, int, int)>(&Foo::mul) << std::endl) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_IS_FALSE(pcomn::delegate<double(int, int)>
                        ((Bar *)NULL, &Bar::mul)) ;
   CPPUNIT_LOG_IS_FALSE(pcomn::delegate<double(int, int)>
                        (&barObject, (double (Foo::*)(double, double))NULL)) ;
}

void DelegateTest::Test_Delegate_Call()
{
   pcomn::delegate<> FooVoid  ;
   pcomn::delegate<int()> FooInt  ;
   pcomn::delegate<double()> FooDouble  ;
   pcomn::delegate<double(double, double)> FooBinOpDouble  ;
   pcomn::delegate<double(double, int)> FooBinOpDoubleInt  ;
   pcomn::delegate<std::string(char, int)> FooStr1  ;
   pcomn::delegate<pcomn::istring(char, int)> FooStr2  ;

   Foo fooObject ;
   Bar barObject ;

   CPPUNIT_LOG_IS_FALSE(FooVoid) ;
   CPPUNIT_LOG_EXCEPTION(FooVoid(), std::bad_function_call) ;
   CPPUNIT_LOG_EQUAL(statint, 0) ;
   CPPUNIT_LOG_EQUAL(statdouble, 0.0) ;
   CPPUNIT_LOG_IS_TRUE(FooVoid = foo_void) ;
   CPPUNIT_LOG_RUN(FooVoid()) ;
   CPPUNIT_LOG_EQUAL(statint, 1) ;
   CPPUNIT_LOG_RUN(FooVoid.clear()) ;
   CPPUNIT_LOG_IS_FALSE(FooVoid) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_IS_FALSE(FooInt) ;
   CPPUNIT_LOG_IS_FALSE(FooDouble) ;
   CPPUNIT_LOG_IS_TRUE(FooInt = foo_int) ;
   CPPUNIT_LOG_IS_TRUE(FooDouble = foo_int) ;
   CPPUNIT_LOG_EQUAL(FooDouble(), 2.0) ;
   CPPUNIT_LOG_EQUAL(statint, 2) ;
   CPPUNIT_LOG_EQUAL(statdouble, 0.0) ;
   CPPUNIT_LOG_EQUAL(FooInt(), 3) ;
   CPPUNIT_LOG_EQUAL(statint, 3) ;
   CPPUNIT_LOG_IS_TRUE(FooDouble = foo_double) ;
   CPPUNIT_LOG_EQUAL(FooDouble(), 1.5) ;
   CPPUNIT_LOG_EQUAL(statdouble, 1.5) ;

   // Testing member function
   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_IS_FALSE(FooBinOpDouble)  ;
   CPPUNIT_LOG_IS_FALSE(FooBinOpDoubleInt)  ;

   CPPUNIT_LOG_IS_TRUE(FooBinOpDouble = pcomn::delegate<double(double, double)>(&fooObject, &Foo::mul))  ;
   CPPUNIT_LOG_IS_TRUE(FooBinOpDoubleInt = pcomn::delegate<double(double, int)>(&fooObject, &Foo::mul)) ;
   CPPUNIT_LOG_EQUAL(FooBinOpDouble(0.5, 15.0), 7.5) ;
   CPPUNIT_LOG_EQUAL(FooBinOpDoubleInt(0.5, 24), 12.0) ;

   CPPUNIT_LOG_IS_TRUE(FooBinOpDouble = std::make_pair(&fooObject, &Foo::mul))  ;
   CPPUNIT_LOG_IS_TRUE(FooBinOpDoubleInt = std::make_pair(&fooObject, &Foo::mul)) ;
   CPPUNIT_LOG_EQUAL(FooBinOpDouble(0.5, 15.0), 7.5) ;
   CPPUNIT_LOG_EQUAL(FooBinOpDoubleInt(0.5, 24), 12.0) ;

   CPPUNIT_LOG_EQUAL(pcomn::delegate<double(Foo *, int, int)>(&Foo::mul)(&fooObject, 10, 3), 30.0) ;
   CPPUNIT_LOG_EQUAL(pcomn::delegate<double(Foo *, int, int)>(&Foo::mul)(&barObject, 10, 3), -30.0) ;

   pcomn::PTSafePtr<Bar> barPointer (new Bar) ;
   pcomn::delegate<double(int, int)> multiplier
      (pcomn::PTWeakReference<Bar>(barPointer), &Foo::mul) ;
   CPPUNIT_LOG(multiplier << std::endl) ;
   CPPUNIT_LOG_IS_TRUE(multiplier) ;
   CPPUNIT_LOG_EQUAL(multiplier(10, 3), -30.0) ;
   CPPUNIT_LOG_RUN(barPointer = NULL) ;
   CPPUNIT_LOG_IS_FALSE(multiplier) ;
   CPPUNIT_LOG_EXCEPTION(multiplier(10, 3), std::bad_function_call) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(DelegateTest::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv, "unittest.diag.ini",
                             "Functors and delegates tests") ;
}
