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

using namespace pcomn ;

template<typename T, typename U>
static bool unique_value_invariant_holds(const T &x, const U &y)
{
    if (&x == &y) return true ;
    return &x.get() != &y.get() || &x.get() == y.default_value_ptr() ;
}

#define CPPUNIT_ASSERT_VALUE_INVARIANT(x, y) CPPUNIT_LOG_ASSERT(unique_value_invariant_holds((x), (y)))

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
    CPPUNIT_LOG_ASSERT(unique_value<std::string>::default_value_ptr()) ;
    CPPUNIT_LOG_EQUAL(*unique_value<std::string>::default_value_ptr(), std::string()) ;

    unique_value<std::string> v1 ;
    unique_value<std::string> v2 ;

    CPPUNIT_LOG_EQUAL(&v1.get(), unique_value<std::string>::default_value_ptr()) ;
    CPPUNIT_LOG_EQUAL(&v2.get(), unique_value<std::string>::default_value_ptr()) ;
    CPPUNIT_LOG_EQUAL(v1.get(), std::string()) ;

    CPPUNIT_LOG(std::endl) ;

    unique_value<std::string> v0 (*unique_value<std::string>::default_value_ptr()) ;

    CPPUNIT_LOG_EQUAL(&v0.get(), unique_value<std::string>::default_value_ptr()) ;

    unique_value<std::string> v3 ("") ;
    unique_value<std::string> v4 ("Hello") ;

    CPPUNIT_LOG_NOT_EQUAL(&v3.get(), unique_value<std::string>::default_value_ptr()) ;
    CPPUNIT_LOG_NOT_EQUAL(&v4.get(), unique_value<std::string>::default_value_ptr()) ;

    CPPUNIT_ASSERT_VALUE_INVARIANT(v3, v4) ;
    CPPUNIT_ASSERT_VALUE_INVARIANT(v1, v3) ;

    CPPUNIT_LOG_EQUAL(v3.get(), std::string()) ;
    CPPUNIT_LOG_EQUAL(v4.get(), std::string("Hello")) ;

    CPPUNIT_LOG(std::endl) ;

    const std::string * const v3p = &v3.get() ;
    const std::string * const v4p = &v4.get() ;

    // Test copy constructors
    unique_value<std::string> v1_1 (v1) ;
    unique_value<std::string> v3_1 (v3) ;
    unique_value<std::string> v4_1 (v4) ;

    CPPUNIT_LOG_EQUAL(v3_1.get(), std::string()) ;
    CPPUNIT_LOG_EQUAL(v4_1.get(), std::string("Hello")) ;

    CPPUNIT_LOG_EQUAL(&v3.get(), v3p) ;
    CPPUNIT_LOG_EQUAL(&v4.get(), v4p) ;

    CPPUNIT_ASSERT_VALUE_INVARIANT(v3, v3_1) ;
    CPPUNIT_ASSERT_VALUE_INVARIANT(v4, v4_1) ;

    CPPUNIT_LOG_EQUAL(&v1.get(), &v1_1.get()) ;
    CPPUNIT_ASSERT_VALUE_INVARIANT(v1, v1_1) ;

    CPPUNIT_LOG(std::endl) ;

    // Test move constructors
    unique_value<std::string> v2_2 (std::move(v2)) ;
    unique_value<std::string> v4_2 (std::move(v4)) ;

    CPPUNIT_LOG_EQUAL(&v2.get(), unique_value<std::string>::default_value_ptr()) ;
    CPPUNIT_LOG_EQUAL(&v2_2.get(), unique_value<std::string>::default_value_ptr()) ;
    CPPUNIT_LOG_EQUAL(&v4.get(), unique_value<std::string>::default_value_ptr()) ;

    CPPUNIT_LOG_EQUAL(v4_2.get(), std::string("Hello")) ;
    CPPUNIT_LOG_EQUAL(&v4_2.get(), v4p) ;

    std::string bar ("bar") ;

    const unique_value<std::string> v5_2 (std::move(bar)) ;

    CPPUNIT_LOG_EQUAL(v5_2.get(), std::string("bar")) ;
    CPPUNIT_LOG_EQUAL(bar, std::string()) ;

    std::unique_ptr<std::string> hello_uniq = std::make_unique<std::string>("Hello, world!") ;
    const std::string * const hello_uniqp = hello_uniq.get() ;

    const unique_value<std::string> v6_2 (std::move(hello_uniq)) ;

    CPPUNIT_LOG_EQUAL(v6_2.get(), std::string("Hello, world!")) ;
    CPPUNIT_LOG_EQUAL(&v6_2.get(), hello_uniqp) ;
    CPPUNIT_LOG_IS_NULL(hello_uniq) ;
}

void UniqueValueTests::Test_Assignment()
{
    CPPUNIT_LOG_ASSERT(std::is_nothrow_swappable_v<unique_value<std::string>>) ;

    unique_value<std::string> v1 ;
    unique_value<std::string> v2 ;

    CPPUNIT_LOG_EQUAL(&v1.get(), &v2.get()) ;
    CPPUNIT_ASSERT_VALUE_INVARIANT(v1, v2) ;

    // Assignment default -> default
    CPPUNIT_LOG_RUN(v1 = v2) ;
    CPPUNIT_LOG_EQUAL(&v1.get(), unique_value<std::string>::default_value_ptr()) ;
    CPPUNIT_LOG_EQUAL(&v2.get(), unique_value<std::string>::default_value_ptr()) ;

    unique_value<std::string> v4 ("Hello") ;

    CPPUNIT_LOG_RUN(v1 = v4) ;
    CPPUNIT_LOG_EQUAL(v4.get(), std::string("Hello")) ;
    CPPUNIT_LOG_EQUAL(v1.get(), std::string("Hello")) ;
    CPPUNIT_ASSERT_VALUE_INVARIANT(v1, v4) ;

    CPPUNIT_LOG_EQUAL(v1.mutable_value(), std::string("Hello")) ;

    CPPUNIT_LOG_RUN(v1.mutable_value() = "foobar") ;
    CPPUNIT_LOG_EQUAL(v1.get(), std::string("foobar")) ;
    CPPUNIT_LOG_EQUAL(v4.get(), std::string("Hello")) ;

    unique_value<std::string> v0 ;

    CPPUNIT_LOG_EQUAL(&v0.get(), unique_value<std::string>::default_value_ptr()) ;

    CPPUNIT_LOG_RUN(v0.mutable_value() = "foo") ;

    CPPUNIT_LOG_EQUAL(v0.get(), std::string("foo")) ;
    CPPUNIT_LOG_NOT_EQUAL(&v0.get(), unique_value<std::string>::default_value_ptr()) ;
    CPPUNIT_LOG_EQUAL(*unique_value<std::string>::default_value_ptr(), std::string()) ;

    CPPUNIT_LOG(std::endl) ;

    // Assign default to nondefault

    CPPUNIT_LOG_RUN(v0 = v2) ;
    CPPUNIT_LOG_EQUAL(&v2.get(), unique_value<std::string>::default_value_ptr()) ;
    CPPUNIT_LOG_EQUAL(&v0.get(), unique_value<std::string>::default_value_ptr()) ;

    CPPUNIT_LOG(std::endl) ;

    // Move-assign
    const std::string * const hellop = &v4.get() ;

    CPPUNIT_LOG_RUN(v0 = std::move(v4)) ;
    CPPUNIT_LOG_EQUAL(v0.get(), std::string("Hello")) ;
    CPPUNIT_LOG_EQUAL(&v4.get(), unique_value<std::string>::default_value_ptr()) ;
    CPPUNIT_LOG_EQUAL(&v0.get(), hellop) ;

    // Swap
    CPPUNIT_LOG_EQUAL(v1.get(), std::string("foobar")) ;

    const std::string * const foobarp = &v1.get() ;

    CPPUNIT_LOG_RUN(swap(v1, v0)) ;

    CPPUNIT_LOG_EQUAL(v0.get(), std::string("foobar")) ;
    CPPUNIT_LOG_EQUAL(v1.get(), std::string("Hello")) ;

    CPPUNIT_LOG_EQUAL(&v1.get(), hellop) ;
    CPPUNIT_LOG_EQUAL(&v0.get(), foobarp) ;
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
