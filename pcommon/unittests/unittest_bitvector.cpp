/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   unittest_bitvector.cpp
 COPYRIGHT    :   Yakov Markovitch, 2020
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unit tests of pcomn::bitvector (split from unittest_bitarray.cpp)

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   24 Jan 2020
*******************************************************************************/
#include <pcomn_bitvector.h>
#include <pcomn_unittest.h>

#include <initializer_list>

using namespace pcomn ;

template<typename T>
static T &set_bits(T &a, std::initializer_list<unsigned> bits, bool value = true)
{
    for (unsigned pos: bits)
        a.set(pos, value) ;
    return a ;
}

/*******************************************************************************
 class BitVectorTests
*******************************************************************************/
class BitVectorTests : public CppUnit::TestFixture {

    void Test_Constructors() ;
    void Test_Set_Reset_Bits() ;
    void Test_Bit_Count() ;
    void Test_Bit_Search() ;
    template<typename I>
    void Test_Positional_Iterator() ;
    void Test_Basic_Positional_Iterator() ;
    void Test_Boundary_Iterator() ;
    void Test_Atomic_Set_Reset_Bits() ;
    void Test_Equality() ;

    CPPUNIT_TEST_SUITE(BitVectorTests) ;

    CPPUNIT_TEST(Test_Constructors) ;
    CPPUNIT_TEST(Test_Set_Reset_Bits) ;
    CPPUNIT_TEST(Test_Bit_Count) ;
    CPPUNIT_TEST(Test_Bit_Search) ;
    CPPUNIT_TEST(Test_Positional_Iterator<uint32_t>) ;
    CPPUNIT_TEST(Test_Positional_Iterator<uint64_t>) ;
    CPPUNIT_TEST(Test_Basic_Positional_Iterator) ;
    CPPUNIT_TEST(Test_Boundary_Iterator) ;
    CPPUNIT_TEST(Test_Atomic_Set_Reset_Bits) ;
    CPPUNIT_TEST(Test_Equality) ;

    CPPUNIT_TEST_SUITE_END() ;
} ;

/*******************************************************************************
 BitVectorTests
*******************************************************************************/
void BitVectorTests::Test_Constructors()
{
    basic_bitvector<uint64_t> empty_64 ;
    basic_bitvector<uint32_t> empty_32 ;

    CPPUNIT_LOG_IS_NULL(empty_64.data()) ;
    CPPUNIT_LOG_IS_NULL(empty_32.data()) ;
    CPPUNIT_LOG_EQ(empty_64.size(), 0) ;
    CPPUNIT_LOG_EQ(empty_32.size(), 0) ;
    CPPUNIT_LOG_EQ(empty_64.nelements(), 0) ;
    CPPUNIT_LOG_EQ(empty_32.nelements(), 0) ;

    CPPUNIT_LOG_EQ(string_cast(empty_64), "") ;
    CPPUNIT_LOG_EQ(string_cast(empty_32), "") ;

    uint32_t v1[] = { 0, 4 } ;
    uint64_t v2[] = { 0, 0, 0x8000000000000002ULL } ;
    const unsigned long long v3[] = { 0x0800000000000055LL } ;

    auto bv1 = make_bitvector(v1) ;
    auto bv2 = make_bitvector(v2 + 0, 2) ;

    auto bv3 = make_bitvector(v3) ;

    CPPUNIT_LOG_EQ(bv1.size(), 64) ;
    CPPUNIT_LOG_EQ(bv1.nelements(), 2) ;

    CPPUNIT_LOG_EQ(bv2.size(), 128) ;
    CPPUNIT_LOG_EQ(bv2.nelements(), 2) ;

    CPPUNIT_LOG_EQ(bv3.size(), 64) ;
    CPPUNIT_LOG_EQ(bv3.nelements(), 1) ;

    CPPUNIT_LOG_EQ(string_cast(bv1), "0000000000000000000000000000000000100000000000000000000000000000") ;
    CPPUNIT_LOG_EQ(string_cast(bv2), std::string(128, '0')) ;
    CPPUNIT_LOG_EQ(string_cast(bv3), "1010101000000000000000000000000000000000000000000000000000010000") ;

    CPPUNIT_LOG_ASSERT(bv3.test(0)) ;
    CPPUNIT_LOG_IS_FALSE(bv3.test(1)) ;
    CPPUNIT_LOG_ASSERT(bv3.test(2)) ;
    CPPUNIT_LOG_IS_FALSE(bv3.test(3)) ;

    CPPUNIT_LOG_IS_FALSE(bv3.test(58)) ;
    CPPUNIT_LOG_ASSERT(bv3.test(59)) ;

    CPPUNIT_LOG_IS_FALSE(bv1.test(31)) ;
    CPPUNIT_LOG_IS_FALSE(bv1.test(32)) ;
    CPPUNIT_LOG_IS_FALSE(bv1.test(33)) ;
    CPPUNIT_LOG_ASSERT(bv1.test(34)) ;
}

void BitVectorTests::Test_Set_Reset_Bits()
{
    uint32_t v1[] = { 0, 4 } ;
    uint64_t v2[] = { 0, 0, 0x8000000000000002ULL } ;

    auto bv1 = make_bitvector(v1) ;
    auto bv2 = make_bitvector(v2 + 0, 2) ;
    auto bv3 = make_bitvector(v2) ;

    CPPUNIT_LOG(std::endl) ;
    CPPUNIT_LOG_ASSERT(bv1.flip(1)) ;
    CPPUNIT_LOG_ASSERT(bv1.test(1)) ;
    CPPUNIT_LOG_EQ(string_cast(bv1), "0100000000000000000000000000000000100000000000000000000000000000") ;

    CPPUNIT_LOG_IS_FALSE(bv1.set(4)) ;
    CPPUNIT_LOG_IS_FALSE(bv1.set(63)) ;
    CPPUNIT_LOG_ASSERT(bv1.set(1, false)) ;
    CPPUNIT_LOG_EQ(string_cast(bv1), "0000100000000000000000000000000000100000000000000000000000000001") ;

    CPPUNIT_LOG_EQ(string_cast(bv2),
                   "0000000000000000000000000000000000000000000000000000000000000000"
                   "0000000000000000000000000000000000000000000000000000000000000000"
        ) ;

    CPPUNIT_LOG_EQ(string_cast(bv3),
                   "0000000000000000000000000000000000000000000000000000000000000000"
                   "0000000000000000000000000000000000000000000000000000000000000000"
                   "0100000000000000000000000000000000000000000000000000000000000001"
        ) ;

    CPPUNIT_LOG_IS_FALSE(bv2.set(65)) ;
    CPPUNIT_LOG_IS_FALSE(bv2.set(66)) ;

    CPPUNIT_LOG_EQ(string_cast(bv2),
                   "0000000000000000000000000000000000000000000000000000000000000000"
                   "0110000000000000000000000000000000000000000000000000000000000000"
        ) ;

    CPPUNIT_LOG_EQ(string_cast(bv3),
                   "0000000000000000000000000000000000000000000000000000000000000000"
                   "0110000000000000000000000000000000000000000000000000000000000000"
                   "0100000000000000000000000000000000000000000000000000000000000001"
        ) ;
}

void BitVectorTests::Test_Bit_Count()
{
    basic_bitvector<uint64_t> empty_64 ;
    basic_bitvector<uint32_t> empty_32 ;

    CPPUNIT_LOG_EQ(empty_64.count(true), 0) ;
    CPPUNIT_LOG_EQ(empty_64.count(false), 0) ;
    CPPUNIT_LOG_EQ(empty_64.count(), 0) ;

    CPPUNIT_LOG_EQ(empty_32.count(true), 0) ;
    CPPUNIT_LOG_EQ(empty_32.count(false), 0) ;
    CPPUNIT_LOG_EQ(empty_32.count(), 0) ;

    uint32_t v1[] = { 0, 4 } ;
    uint64_t v2[] = { 0, 0, 0x8000000000000002ULL } ;
    const unsigned long long v3[] = { 0x0800000000000055LL } ;

    auto bv1 = make_bitvector(v1) ;
    auto bv2 = make_bitvector(v2) ;
    auto bv3 = make_bitvector(v3) ;

    auto bv4 = make_bitvector(130, v2) ;
    auto bv5 = make_bitvector(59,  v3) ;

    CPPUNIT_LOG_EQ(bv1.count(true), 1) ;
    CPPUNIT_LOG_EQ(bv1.count(false), 63) ;
    CPPUNIT_LOG_EQ(bv1.count(), 1) ;

    CPPUNIT_LOG_EQ(bv2.count(true), 2) ;
    CPPUNIT_LOG_EQ(bv2.count(false), 190) ;
    CPPUNIT_LOG_EQ(bv2.count(), 2) ;

    CPPUNIT_LOG_EQ(bv3.count(true), 5) ;
    CPPUNIT_LOG_EQ(bv3.count(false), 59) ;
    CPPUNIT_LOG_EQ(bv3.count(), 5) ;

    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG_EQ(bv4.count(true), 1) ;
    CPPUNIT_LOG_EQ(bv4.count(false), 129) ;
    CPPUNIT_LOG_EQ(bv4.count(), 1) ;

    CPPUNIT_LOG_EQ(bv5.count(true), 4) ;
    CPPUNIT_LOG_EQ(bv5.count(false), 55) ;
    CPPUNIT_LOG_EQ(bv5.count(), 4) ;

    CPPUNIT_LOG_EQ(string_cast(bv4),
                   "0000000000000000000000000000000000000000000000000000000000000000"
                   "0000000000000000000000000000000000000000000000000000000000000000"
                   "01"
        ) ;
}

void BitVectorTests::Test_Bit_Search()
{
    basic_bitvector<uint64_t> empty_64 ;
    basic_bitvector<uint32_t> empty_32 ;

    uint64_t v0_64[] = { 2 } ;
    uint32_t v0_32[] = { 2 } ;

    uint32_t v1[] = { 0, 4 } ;
    uint64_t v2[] = { 0, 0, 0x8000000000000002ULL } ;
    const unsigned long long v3[] = { 0x0800000000000055LL } ;

    auto bv1 = make_bitvector(v1) ;
    auto bv2 = make_bitvector(v2 + 0, 2) ;
    auto bv2_full = make_bitvector(v2) ;

    auto bv3 = make_bitvector(v3) ;

    auto bv0_64 = make_bitvector(v0_64) ;
    auto bv0_32 = make_bitvector(v0_32) ;

    CPPUNIT_LOG_EQ(empty_64.find_first_bit<1>(0), 0) ;
    CPPUNIT_LOG_EQ(empty_64.find_first_bit<1>(0, 0), 0) ;
    CPPUNIT_LOG_EQ(empty_64.find_first_bit<1>(2, 1), 0) ;
    CPPUNIT_LOG_EQ(empty_32.find_first_bit<1>(0), 0) ;
    CPPUNIT_LOG_EQ(empty_32.find_first_bit<1>(0, 0), 0) ;
    CPPUNIT_LOG_EQ(empty_32.find_first_bit<1>(2, 1), 0) ;

    CPPUNIT_LOG_EQ(empty_64.find_first_bit<0>(0), 0) ;
    CPPUNIT_LOG_EQ(empty_64.find_first_bit<0>(0, 0), 0) ;
    CPPUNIT_LOG_EQ(empty_64.find_first_bit<0>(2, 1), 0) ;
    CPPUNIT_LOG_EQ(empty_32.find_first_bit<0>(0), 0) ;
    CPPUNIT_LOG_EQ(empty_32.find_first_bit<0>(0, 0), 0) ;
    CPPUNIT_LOG_EQ(empty_32.find_first_bit<0>(2, 1), 0) ;
    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG_EQ(bv0_64.find_first_bit<1>(0), 1) ;
    CPPUNIT_LOG_EQ(bv0_64.find_first_bit<1>(0, 0), 0) ;
    CPPUNIT_LOG_EQ(bv0_64.find_first_bit<1>(2, 1), 1) ;
    CPPUNIT_LOG_EQ(bv0_64.find_first_bit<1>(2), 64) ;

    CPPUNIT_LOG_EQ(bv0_64.find_first_bit<0>(0), 0) ;
    CPPUNIT_LOG_EQ(bv0_64.find_first_bit<0>(0, 0), 0) ;
    CPPUNIT_LOG_EQ(bv0_64.find_first_bit<0>(2, 1), 1) ;
    CPPUNIT_LOG_EQ(bv0_64.find_first_bit<0>(1), 2) ;
    CPPUNIT_LOG_EQ(bv0_64.find_first_bit<0>(2), 2) ;

    CPPUNIT_LOG_EQ(bv0_32.find_first_bit<1>(0), 1) ;
    CPPUNIT_LOG_EQ(bv0_32.find_first_bit<1>(0, 0), 0) ;
    CPPUNIT_LOG_EQ(bv0_32.find_first_bit<1>(2, 1), 1) ;
    CPPUNIT_LOG_EQ(bv0_32.find_first_bit<1>(2), 32) ;

    CPPUNIT_LOG_EQ(bv0_32.find_first_bit<0>(0), 0) ;
    CPPUNIT_LOG_EQ(bv0_32.find_first_bit<0>(0, 0), 0) ;
    CPPUNIT_LOG_EQ(bv0_32.find_first_bit<0>(2, 1), 1) ;
    CPPUNIT_LOG_EQ(bv0_32.find_first_bit<0>(1), 2) ;
    CPPUNIT_LOG_EQ(bv0_32.find_first_bit<0>(2), 2) ;

    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG_EQ(bv1.find_first_bit<1>(), 34) ;
    CPPUNIT_LOG_EQ(bv1.find_first_bit<0>(), 0) ;
    CPPUNIT_LOG_EQ(bv1.find_first_bit<1>(34), 34) ;
    CPPUNIT_LOG_EQ(bv1.find_first_bit<1>(35), 64) ;

    CPPUNIT_LOG_EQ(bv3.find_first_bit<1>(), 0) ;
    CPPUNIT_LOG_EQ(bv3.find_first_bit<1>(1), 2) ;
    CPPUNIT_LOG_EQ(bv3.find_first_bit<1>(3), 4) ;
    CPPUNIT_LOG_EQ(bv3.find_first_bit<1>(5), 6) ;
    CPPUNIT_LOG_EQ(bv3.find_first_bit<1>(7), 59) ;
    CPPUNIT_LOG_EQ(bv3.find_first_bit<1>(60), 64) ;

    CPPUNIT_LOG_EQ(bv2.size(), 128) ;
    CPPUNIT_LOG_EQ(bv2.find_first_bit<1>(), 128) ;
    CPPUNIT_LOG_EQ(bv2.find_first_bit<0>(), 0) ;

    CPPUNIT_LOG_EQ(bv2_full.size(), 192) ;
    CPPUNIT_LOG_EQ(bv2_full.find_first_bit<1>(), 129) ;
    CPPUNIT_LOG_EQ(bv2_full.find_first_bit<0>(129), 130) ;
    CPPUNIT_LOG_EQ(bv2_full.find_first_bit<1>(130), 191) ;
}

template<typename I>
void BitVectorTests::Test_Positional_Iterator()
{
    typedef basic_bitvector<I>       bitvector ;
    typedef typename bitvector::element_type element_type ;
    typedef typename bitvector::template positional_iterator<true> positional_iterator ;

    element_type vdata[4096/bitvector::bits_per_element()]  = {} ;

    bitvector bv_empty ;
    CPPUNIT_LOG_ASSERT(bv_empty.begin_positional() == bv_empty.end_positional()) ;

    bitvector bv = make_bitvector(vdata) ;

    set_bits(bv, {36, 44, 48, 52, 64, 70, 72, 76, 100, 208}) ;

    positional_iterator bp = bv.begin_positional() ;
    const positional_iterator ep = bv.end_positional() ;

    CPPUNIT_LOG_ASSERT(bp != ep) ;
    CPPUNIT_LOG_EQ(*bp, 36) ;
    CPPUNIT_LOG_ASSERT(++bp != ep) ;
    CPPUNIT_LOG_EQ(*bp, 44) ;
    CPPUNIT_LOG_ASSERT(++bp != ep) ;
    CPPUNIT_LOG_EQ(*bp, 48) ;
    CPPUNIT_LOG_ASSERT(++bp != ep) ;
    CPPUNIT_LOG_EQ(*bp, 52) ;
    CPPUNIT_LOG_ASSERT(++bp != ep) ;
    CPPUNIT_LOG_EQ(*bp, 64) ;
    CPPUNIT_LOG_ASSERT(++bp != ep) ;
    CPPUNIT_LOG_EQ(*bp, 70) ;
    CPPUNIT_LOG_EQ(*bp, 70) ;
    CPPUNIT_LOG_ASSERT(++bp != ep) ;
    CPPUNIT_LOG_EQ(*bp, 72) ;
    CPPUNIT_LOG_ASSERT(++bp != ep) ;
    CPPUNIT_LOG_EQ(*bp, 76) ;
    CPPUNIT_LOG_ASSERT(++bp != ep) ;
    CPPUNIT_LOG_EQ(*bp, 100) ;
    CPPUNIT_LOG_ASSERT(++bp != ep) ;
    CPPUNIT_LOG_EQ(*bp, 208) ;
    CPPUNIT_LOG_ASSERT(++bp == ep) ;
    CPPUNIT_LOG_ASSERT(bp == ep) ;

    CPPUNIT_LOG(std::endl) ;

    bp = positional_iterator(bv, 36) ;
    CPPUNIT_LOG_ASSERT(bp != ep) ;
    CPPUNIT_LOG_EQ(*bp, 36) ;
    CPPUNIT_LOG_ASSERT(++bp != ep) ;
    CPPUNIT_LOG_EQ(*bp, 44) ;

    bp = positional_iterator(bv, 127) ;
    CPPUNIT_LOG_ASSERT(bp != ep) ;
    CPPUNIT_LOG_EQ(*bp, 208) ;
    CPPUNIT_LOG_ASSERT(++bp == ep) ;

    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG_RUN(std::fill_n(bv.data(), bv.nelements(), 0)) ;
    CPPUNIT_LOG_RUN(bp = bv.begin_positional()) ;
    CPPUNIT_LOG_ASSERT(bp == ep) ;

    CPPUNIT_LOG(std::endl) ;
    CPPUNIT_LOG_RUN(bv.set(4095)) ;
    CPPUNIT_LOG_RUN(bp = bv.begin_positional()) ;
    CPPUNIT_LOG_ASSERT(bp != ep) ;
    CPPUNIT_LOG_EQ(*bp, 4095) ;
    CPPUNIT_LOG_ASSERT(++bp == ep) ;
}

void BitVectorTests::Test_Basic_Positional_Iterator()
{
    typedef basic_bitvector<uint64_t> bitvector ;

    uint64_t vdata[16]  = {} ;

    bitvector bv (1000, vdata + 0) ;

    set_bits(bv, {0, 200, 300, 555, 999}) ;

    auto bp = bv.begin_positional() ;
    auto ep = bv.end_positional() ;

    CPPUNIT_LOG_EQ(bv.count(), 5) ;
    CPPUNIT_LOG_EQ(std::distance(bp, ep), 5) ;
    CPPUNIT_LOG_EQUAL(std::vector<unsigned>(bp, ep), (std::vector<unsigned>{0, 200, 300, 555, 999})) ;

    CPPUNIT_LOG_EQ(vdata[15], 0x8000000000) ;

    CPPUNIT_LOG_RUN(vdata[15] = 0x9000000000) ;
    CPPUNIT_LOG_EQ(bv.count(), 6) ;
    CPPUNIT_LOG_EQUAL(std::vector<unsigned>(bv.begin_positional(), bv.end_positional()),
                      (std::vector<unsigned>{0, 200, 300, 555, 996, 999})) ;

    CPPUNIT_LOG_RUN(vdata[15] = 0x18000000000) ;
    CPPUNIT_LOG_EQ(bv.count(), 5) ;
    CPPUNIT_LOG_EQUAL(std::vector<unsigned>(bv.begin_positional(), bv.end_positional()),
                      (std::vector<unsigned>{0, 200, 300, 555, 999})) ;
}

void BitVectorTests::Test_Boundary_Iterator()
{
    typedef basic_bitvector<uint64_t>       bitvector ;
    typedef basic_bitvector<const uint64_t> cbitvector ;
    typedef bitvector::boundary_iterator    boundary_iterator ;
    typedef bitvector::element_type         element_type ;

    element_type vdata[4096/bitvector::bits_per_element()]  = {} ;

    bitvector bv_empty ;
    bitvector bv = make_bitvector(1025, vdata) ;

    set_bits(bv, {36, 37, 38, 65, 67, 68}) ;

    cbitvector cbv (bv) ;

    CPPUNIT_LOG_ASSERT(bv_empty.begin_boundary() == bv_empty.end_boundary()) ;
    boundary_iterator b = bv.begin_boundary() ;
    const boundary_iterator e = bv.end_boundary() ;

    CPPUNIT_LOG_ASSERT(b != e) ;
    CPPUNIT_LOG_EQ(*b, 0) ;
    CPPUNIT_LOG_IS_FALSE(b()) ;
    CPPUNIT_LOG_ASSERT(b != e) ;

    CPPUNIT_LOG_EQ(*++b, 36) ;
    CPPUNIT_LOG_EQ(*b, 36) ;
    CPPUNIT_LOG_ASSERT(b()) ;
    CPPUNIT_LOG_ASSERT(b != e) ;

    CPPUNIT_LOG_EQ(*++b, 39) ;
    CPPUNIT_LOG_EQ(*b, 39) ;
    CPPUNIT_LOG_IS_FALSE(b()) ;
    CPPUNIT_LOG_ASSERT(b != e) ;

    CPPUNIT_LOG_EQ(*++b, 65) ;
    CPPUNIT_LOG_EQ(*b, 65) ;
    CPPUNIT_LOG_ASSERT(b()) ;
    CPPUNIT_LOG_ASSERT(b != e) ;

    CPPUNIT_LOG_EQ(*++b, 66) ;
    CPPUNIT_LOG_EQ(*b, 66) ;
    CPPUNIT_LOG_IS_FALSE(b()) ;
    CPPUNIT_LOG_ASSERT(b != e) ;

    CPPUNIT_LOG_EQ(*++b, 67) ;
    CPPUNIT_LOG_EQ(*b, 67) ;
    CPPUNIT_LOG_ASSERT(b()) ;
    CPPUNIT_LOG_ASSERT(b != e) ;

    CPPUNIT_LOG_EQ(*++b, 69) ;
    CPPUNIT_LOG_EQ(*b, 69) ;
    CPPUNIT_LOG_IS_FALSE(b()) ;
    CPPUNIT_LOG_ASSERT(b != e) ;

    CPPUNIT_LOG_EQ(*++b, 1025) ;
    CPPUNIT_LOG_EQ(*b, 1025) ;
    CPPUNIT_LOG_ASSERT(b == e) ;

    CPPUNIT_LOG_EQ(std::distance(bv.begin_boundary(), e), 7) ;

    CPPUNIT_LOG(std::endl) ;

    boundary_iterator b1 = boundary_iterator(bv, 37) ;

    CPPUNIT_LOG_EQ(*b1, 37) ;
    CPPUNIT_LOG_ASSERT(b1()) ;
    CPPUNIT_LOG_EQ(*++b1, 39) ;
    CPPUNIT_LOG_IS_FALSE(b1()) ;

    CPPUNIT_LOG_EQ(std::distance(boundary_iterator(bv, 37), e), 6) ;
    CPPUNIT_LOG_EQ(std::distance(boundary_iterator(bv, 69), e), 1) ;
    CPPUNIT_LOG_EQ(std::distance(boundary_iterator(bv, 1024), e), 1) ;
    CPPUNIT_LOG_EQ(std::distance(boundary_iterator(bv, 1025), e), 0) ;
}

void BitVectorTests::Test_Equality()
{
    typedef basic_bitvector<uint64_t> bvec ;
    typedef basic_bitvector<const uint64_t> cbvec ;

    uint32_t v1[] = { 0, 4 } ;
    uint64_t v2[] = { 0, 0, 0x8000000000000002ULL } ;
    uint64_t v3[] = { 0b1111100000111111000000000000000000000000000000000000000000000000ULL, 0b0000000000111111111000000000000000000011111, 0x8000000000000002ULL } ;
    uint64_t v4[] = { 0, 0, 0x8000000000000002ULL } ;
    uint64_t v5[] = { 0b1111100000111110111111111111110000000000000000000000000000000000ULL } ;

    auto bv1_0 = make_bitvector(v1) ;
    auto bv2_0 = make_bitvector(v2) ;
    auto bv2_1 = make_bitvector(v2 + 0, 2) ;
    auto bv2_2 = make_bitvector(189, v2) ;
    auto bv3_0 = make_bitvector(v3) ;
    auto bv4_0 = make_bitvector(v4) ;

    CPPUNIT_LOG(std::endl) ;
    CPPUNIT_LOG_EXPRESSION(bv1_0) ;
    CPPUNIT_LOG_EXPRESSION(bv2_0) ;
    CPPUNIT_LOG_EXPRESSION(bv2_1) ;
    CPPUNIT_LOG_EXPRESSION(bv2_2) ;
    CPPUNIT_LOG_EXPRESSION(bv3_0) ;
    CPPUNIT_LOG_EXPRESSION(bv4_0) ;

    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG_ASSERT(bvec() == bvec()) ;
    CPPUNIT_LOG_ASSERT(bvec() == cbvec()) ;
    CPPUNIT_LOG_ASSERT(cbvec() == bvec()) ;
    CPPUNIT_LOG_ASSERT(cbvec() == cbvec()) ;

    CPPUNIT_LOG_IS_FALSE(bvec() != bvec()) ;
    CPPUNIT_LOG_IS_FALSE(bvec() != cbvec()) ;
    CPPUNIT_LOG_IS_FALSE(cbvec() != bvec()) ;
    CPPUNIT_LOG_IS_FALSE(cbvec() != cbvec()) ;

    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG_EQUAL(bv1_0, bv1_0) ;

    CPPUNIT_LOG_NOT_EQUAL(bv2_0, bv2_1) ;
    CPPUNIT_LOG_NOT_EQUAL(bv2_1, bv2_2) ;

    CPPUNIT_LOG_EQUAL(bv2_0, bvec(v4)) ;
    CPPUNIT_LOG_NOT_EQUAL(bvec(v3), bvec(v5)) ;

    CPPUNIT_LOG_EQUAL(bvec(15, v3), bvec(15, v5)) ;
}

void BitVectorTests::Test_Atomic_Set_Reset_Bits()
{
    uint32_t v1[] = { 0, 4 } ;
    uint64_t v2[] = { 0, 0, 0x8000000000000002ULL } ;

    auto bv1 = make_bitvector(v1) ;
    auto bv2 = make_bitvector(v2 + 0, 2) ;
    auto bv3 = make_bitvector(v2) ;

    CPPUNIT_LOG(std::endl) ;
    CPPUNIT_LOG_ASSERT(bv1.flip(1, std::memory_order_acq_rel)) ;
    CPPUNIT_LOG_ASSERT(bv1.test(1, std::memory_order_acq_rel)) ;
    CPPUNIT_LOG_EQ(string_cast(bv1), "0100000000000000000000000000000000100000000000000000000000000000") ;

    CPPUNIT_LOG_IS_FALSE(bv1.set(4, std::memory_order_acq_rel)) ;
    CPPUNIT_LOG_IS_FALSE(bv1.set(63, std::memory_order_acq_rel)) ;
    CPPUNIT_LOG_ASSERT(bv1.set(1, false, std::memory_order_acq_rel)) ;
    CPPUNIT_LOG_EQ(string_cast(bv1), "0000100000000000000000000000000000100000000000000000000000000001") ;

    CPPUNIT_LOG_EQ(string_cast(bv2),
                   "0000000000000000000000000000000000000000000000000000000000000000"
                   "0000000000000000000000000000000000000000000000000000000000000000"
        ) ;

    CPPUNIT_LOG_EQ(string_cast(bv3),
                   "0000000000000000000000000000000000000000000000000000000000000000"
                   "0000000000000000000000000000000000000000000000000000000000000000"
                   "0100000000000000000000000000000000000000000000000000000000000001"
        ) ;

    CPPUNIT_LOG_IS_FALSE(bv2.set(65, std::memory_order_acq_rel)) ;
    CPPUNIT_LOG_IS_FALSE(bv2.set(66, std::memory_order_acq_rel)) ;

    CPPUNIT_LOG_EQ(string_cast(bv2),
                   "0000000000000000000000000000000000000000000000000000000000000000"
                   "0110000000000000000000000000000000000000000000000000000000000000"
        ) ;

    CPPUNIT_LOG_EQ(string_cast(bv3),
                   "0000000000000000000000000000000000000000000000000000000000000000"
                   "0110000000000000000000000000000000000000000000000000000000000000"
                   "0100000000000000000000000000000000000000000000000000000000000001"
        ) ;

    CPPUNIT_LOG(std::endl) ;
    CPPUNIT_LOG_ASSERT(bv2.flip(1, std::memory_order_relaxed)) ;
    CPPUNIT_LOG_EQ(string_cast(bv2),
                   "0100000000000000000000000000000000000000000000000000000000000000"
                   "0110000000000000000000000000000000000000000000000000000000000000"
        ) ;
    CPPUNIT_LOG_IS_FALSE(bv2.flip(65, std::memory_order_relaxed)) ;
    CPPUNIT_LOG_EQ(string_cast(bv2),
                   "0100000000000000000000000000000000000000000000000000000000000000"
                   "0010000000000000000000000000000000000000000000000000000000000000"
        ) ;
    CPPUNIT_LOG_ASSERT(bv2.flip(65, std::memory_order_relaxed)) ;
    CPPUNIT_LOG_EQ(string_cast(bv2),
                   "0100000000000000000000000000000000000000000000000000000000000000"
                   "0110000000000000000000000000000000000000000000000000000000000000"
        ) ;

    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG_ASSERT(bv2.cas(68, false, true)) ;
    CPPUNIT_LOG_EQ(string_cast(bv2),
                   "0100000000000000000000000000000000000000000000000000000000000000"
                   "0110100000000000000000000000000000000000000000000000000000000000"
        ) ;

    CPPUNIT_LOG_IS_FALSE(bv2.cas(3, true, true)) ;
    CPPUNIT_LOG_EQ(string_cast(bv2),
                   "0100000000000000000000000000000000000000000000000000000000000000"
                   "0110100000000000000000000000000000000000000000000000000000000000"
        ) ;

    CPPUNIT_LOG_ASSERT(bv2.cas(3, false, false)) ;
    CPPUNIT_LOG_EQ(string_cast(bv2),
                   "0100000000000000000000000000000000000000000000000000000000000000"
                   "0110100000000000000000000000000000000000000000000000000000000000"
        ) ;

    CPPUNIT_LOG_ASSERT(bv2.cas(3, false, true, std::memory_order_relaxed)) ;
    CPPUNIT_LOG_EQ(string_cast(bv2),
                   "0101000000000000000000000000000000000000000000000000000000000000"
                   "0110100000000000000000000000000000000000000000000000000000000000"
        ) ;
}

/*******************************************************************************
 main
*******************************************************************************/
int main(int argc, char *argv[])
{
   return unit::run_tests
       <
          BitVectorTests
       >
       (argc, argv) ;
}
