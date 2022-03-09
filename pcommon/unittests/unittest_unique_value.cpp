/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   unittest_unique_value.cpp
 COPYRIGHT    :   Yakov Markovitch, 2022. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unit tests of pcomn::unique_value.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   9 Mar 2022
*******************************************************************************/
#include <pcomn_safeptr.h>
#include <pcomn_unittest.h>

#include <functional>
#include <iostream>
#include <memory>
#include <string>

/*******************************************************************************
                     class UniqueValueTests
*******************************************************************************/
class UniqueValueTests : public CppUnit::TestFixture {

    void Test_Constructors() ;
    void Test_Assignment() ;

    CPPUNIT_TEST_SUITE(UniqueValueTests) ;

    CPPUNIT_TEST(Test_Constructors) ;
    CPPUNIT_TEST(Test_Assignment) ;

    CPPUNIT_TEST_SUITE_END() ;
} ;

/*******************************************************************************
 UniqueValueTests
*******************************************************************************/
void UniqueValueTests::Test_Constructors()
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

void UniqueValueTests::Test_Assignment()
{
    LifetimeRegister foo_reg ;
    LifetimeRegister bar_reg ;
    LifetimeRegister quux_reg ;

    typedef pcomn::shared_intrusive_ptr<Foo> foo_ptr ;
    typedef pcomn::shared_intrusive_ptr<Bar> bar_ptr ;

    foo_ptr foo1 (new Foo(foo_reg)) ;

    CPPUNIT_LOG_ASSERT(foo1.get()) ;
    CPPUNIT_LOG_EQ(foo1.use_count(), 1) ;

    foo_ptr foo2 (foo1.get()) ;

    CPPUNIT_LOG_EQUAL(foo1.get(), foo2.get()) ;
    CPPUNIT_LOG_ASSERT(foo1.get()) ;
    CPPUNIT_LOG_EQ(foo1.use_count(), 2) ;

    foo_ptr foo3 (foo2) ;

    CPPUNIT_LOG_EQUAL(foo3.get(), foo1.get()) ;
    CPPUNIT_LOG_ASSERT(foo3.get()) ;
    CPPUNIT_LOG_EQ(foo1.use_count(), 3) ;

    CPPUNIT_LOG_RUN(foo3 = foo3) ;
    CPPUNIT_LOG_EQUAL(foo3.get(), foo1.get()) ;
    CPPUNIT_LOG_EQ(foo3.use_count(), 3) ;

    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG_RUN(foo3 = std::move(foo3)) ;
    CPPUNIT_LOG_EQUAL(foo3.get(), foo1.get()) ;
    CPPUNIT_LOG_EQ(foo3.use_count(), 3) ;

    CPPUNIT_LOG_RUN(foo3 = std::move(foo2)) ;
    CPPUNIT_LOG_EQUAL(foo3.get(), foo1.get()) ;
    CPPUNIT_LOG_EQ(foo2.get(), nullptr) ;
    CPPUNIT_LOG_EQ(foo3.use_count(), 2) ;

    CPPUNIT_LOG_RUN(foo2 = std::move(foo3)) ;
    CPPUNIT_LOG_EQUAL(foo2.get(), foo1.get()) ;
    CPPUNIT_LOG_EQ(foo3.get(), nullptr) ;
    CPPUNIT_LOG_EQ(foo2.use_count(), 2) ;

    CPPUNIT_LOG(std::endl) ;

    foo_ptr foo4 (std::move(foo3)) ;

    CPPUNIT_LOG_EQ(foo4.get(), nullptr) ;
    CPPUNIT_LOG_RUN(foo4 = foo4) ;
    CPPUNIT_LOG_EQ(foo4.get(), nullptr) ;
    CPPUNIT_LOG_RUN(foo4 = std::move(foo4)) ;
    CPPUNIT_LOG_EQ(foo4.get(), nullptr) ;

    CPPUNIT_LOG_RUN(foo4 = foo1.get()) ;
    CPPUNIT_LOG_EQUAL(foo4.get(), foo1.get()) ;
    CPPUNIT_LOG_EQ(foo4.use_count(), 3) ;

    CPPUNIT_LOG(std::endl) ;
    CPPUNIT_LOG_RUN(foo2 = {}) ;
    CPPUNIT_LOG_RUN(foo3 = nullptr) ;
    CPPUNIT_LOG_RUN(foo4 = foo3) ;

    CPPUNIT_LOG_IS_NULL(foo2.get()) ;
    CPPUNIT_LOG_IS_NULL(foo3.get()) ;
    CPPUNIT_LOG_IS_NULL(foo4.get()) ;
    CPPUNIT_LOG_ASSERT(foo1.get()) ;
    CPPUNIT_LOG_EQ(foo1.use_count(), 1) ;

    CPPUNIT_LOG(std::endl) ;
    CPPUNIT_LOG_IS_TRUE(foo_reg.constructed) ;
    CPPUNIT_LOG_IS_FALSE(foo_reg.destructed) ;

    CPPUNIT_LOG(std::endl) ;

    bar_ptr bar1 (new Bar(bar_reg)) ;

    CPPUNIT_LOG_EQ(bar1.use_count(), 1) ;
    CPPUNIT_LOG_RUN(foo2 = bar1) ;
    CPPUNIT_LOG_EQ(bar1.use_count(), 2) ;
    CPPUNIT_LOG_EQ(foo2.get(), bar1.get()) ;

    CPPUNIT_LOG_IS_FALSE(foo_reg.destructed) ;
    CPPUNIT_LOG_IS_TRUE(bar_reg.constructed) ;
    CPPUNIT_LOG_IS_FALSE(bar_reg.destructed) ;

    CPPUNIT_LOG(std::endl) ;
    CPPUNIT_LOG_RUN(foo1 = std::move(bar1)) ;
    CPPUNIT_LOG_EQ(foo1.use_count(), 2) ;
    CPPUNIT_LOG_IS_NULL(bar1.get()) ;

    CPPUNIT_LOG_IS_TRUE(foo_reg.destructed) ;
    CPPUNIT_LOG_IS_TRUE(bar_reg.constructed) ;
    CPPUNIT_LOG_IS_FALSE(bar_reg.destructed) ;
}

/*******************************************************************************
 main
*******************************************************************************/
int main(int argc, char *argv[])
{
    return pcomn::unit::run_tests
        <
            UniqueValueTests
        >
        (argc, argv) ;
}
