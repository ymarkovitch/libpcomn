/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_uuid.cpp
 COPYRIGHT    :   Yakov Markovitch, 2014-2018. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for uuid and network MAC classes/functions

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   20 Oct 2014
*******************************************************************************/
#include <pcomn_uuid.h>
#include <pcomn_unittest.h>

#include <type_traits>
#include <new>

using namespace pcomn ;
using namespace pcomn::unit ;

extern const char UUID_FIXTURE[] = "uuid" ;
extern const char MAC_FIXTURE[]  = "MAC" ;

/*******************************************************************************
              class UUIDFixture
*******************************************************************************/
class UUIDFixture : public unit::TestFixture<UUID_FIXTURE> {
      typedef unit::TestFixture<UUID_FIXTURE> ancestor ;

      void Test_Empty_UUID() ;
      void Test_UUID() ;
      void Test_Cast128() ;
      void Test_Binary256() ;

      CPPUNIT_TEST_SUITE(UUIDFixture) ;

      CPPUNIT_TEST(Test_Empty_UUID) ;
      CPPUNIT_TEST(Test_UUID) ;
      CPPUNIT_TEST(Test_Cast128) ;
      CPPUNIT_TEST(Test_Binary256) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

void UUIDFixture::Test_Empty_UUID()
{
   CPPUNIT_LOG_ASSERT(std::is_trivially_copyable<uuid>::value) ;

   CPPUNIT_LOG_IS_FALSE(uuid()) ;
   CPPUNIT_LOG_EQUAL(uuid::size(), (size_t)16) ;
   CPPUNIT_LOG_EQUAL(uuid(), uuid()) ;
   CPPUNIT_LOG_IS_FALSE(uuid() != uuid()) ;
   CPPUNIT_LOG_IS_FALSE(uuid() < uuid()) ;
   CPPUNIT_LOG_IS_FALSE(uuid() > uuid()) ;
   CPPUNIT_LOG_ASSERT(uuid() >= uuid()) ;
   CPPUNIT_LOG_ASSERT(uuid() <= uuid()) ;
   const uuid null_uuid ;
   for (unsigned i = 0 ; i < uuid::size() ; ++i)
      CPPUNIT_IS_FALSE(null_uuid.octet(i)) ;

   char buf[uuid::slen() + 3] = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" ;
   CPPUNIT_LOG_EQ(null_uuid.to_string(), "00000000-0000-0000-0000-000000000000") ;
   CPPUNIT_LOG_EQUAL(null_uuid.to_strbuf(buf), buf + 0) ;
   CPPUNIT_LOG_EQ(std::string(buf), "00000000-0000-0000-0000-000000000000") ;
   CPPUNIT_LOG_EQUAL(buf[uuid::slen() + 1], 'A') ;
   CPPUNIT_LOG_EQUAL(buf[uuid::slen() + 2], '\0') ;

   CPPUNIT_LOG_EQUAL(null_uuid.version(), 0U) ;

   union {
         char mem[sizeof(uuid)] ;
         uint64_t _ ;
   } storage ;
   memset(&storage, 'Q', sizeof storage) ;
   const uuid *null_a = new (&storage) uuid ;
   CPPUNIT_LOG_ASSERT(null_a) ;
   CPPUNIT_LOG_IS_FALSE(*null_a) ;

   CPPUNIT_LOG(std::endl) ;
}

void UUIDFixture::Test_UUID()
{
   const uuid random_uuid ("f47ac10b-58cc-4372-a567-0e02b2c3d479") ;
   const uuid other_uuid  ("f47ac10b-58cc-4372-a567-0e02b2c3d478") ;
   const uuid small_uuid  ("E47AC10B-58cC-4372-a567-0e02b2c3d478") ;
   const uuid other_uuid_2 (0xf47a, 0xc10b, 0x58cc, 0x4372, 0xa567, 0x0e02, 0xb2c3, 0xd478) ;

   const uuid other_uuid_3 (0x123456780a0b0c0dULL, 0x1a1b1c1d2a2b2c2dULL) ;

   CPPUNIT_LOG_ASSERT(random_uuid) ;
   CPPUNIT_LOG_EQ(random_uuid.to_string(), "f47ac10b-58cc-4372-a567-0e02b2c3d479") ;
   CPPUNIT_LOG_ASSERT(small_uuid) ;
   CPPUNIT_LOG_EQ(small_uuid.to_string(),  "e47ac10b-58cc-4372-a567-0e02b2c3d478") ;
   CPPUNIT_LOG_ASSERT(other_uuid) ;
   CPPUNIT_LOG_EQ(other_uuid.to_string(),  "f47ac10b-58cc-4372-a567-0e02b2c3d478") ;

   CPPUNIT_LOG_EQUAL(other_uuid, other_uuid_2) ;

   CPPUNIT_LOG_EQ(other_uuid_3.to_string(), "12345678-0a0b-0c0d-1a1b-1c1d2a2b2c2d") ;

   CPPUNIT_LOG_NOT_EQUAL(random_uuid, uuid()) ;
   CPPUNIT_LOG_NOT_EQUAL(random_uuid, small_uuid) ;
   CPPUNIT_LOG_NOT_EQUAL(random_uuid, other_uuid) ;
   CPPUNIT_LOG_NOT_EQUAL(small_uuid, other_uuid) ;

   CPPUNIT_LOG_EQUAL(random_uuid, uuid("f47ac10b-58cc-4372-a567-0e02b2c3d479")) ;

   const uuid null_uuid ("00000000-0000-0000-0000-000000000000", RAISE_ERROR) ;
   CPPUNIT_LOG_EQUAL(null_uuid, uuid()) ;

   CPPUNIT_LOG(std::endl) ;
   // Invalid formats
   CPPUNIT_LOG_IS_FALSE(uuid("f47ac10b-58cc-4372-a567-0e02b2c3d47", DONT_RAISE_ERROR)) ;
   CPPUNIT_LOG_EXCEPTION(uuid("f47ac10b-58cc-4372-a567-0e02b2c3d47"), std::invalid_argument) ;
   CPPUNIT_LOG_IS_FALSE(uuid(nullptr)) ;
   CPPUNIT_LOG_IS_FALSE(uuid(nullptr, RAISE_ERROR)) ;
   CPPUNIT_LOG_IS_FALSE(uuid("", RAISE_ERROR)) ;
   CPPUNIT_LOG_IS_FALSE(uuid(strslice(), RAISE_ERROR)) ;
   CPPUNIT_LOG_IS_FALSE(uuid(std::string(), RAISE_ERROR)) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EXCEPTION(uuid("f47ac10b-58cc-4372-a567-0e02b2c3d479 "), std::invalid_argument) ;
   CPPUNIT_LOG_EXCEPTION(uuid(" f47ac10b-58cc-4372-a567-0e02b2c3d479"), std::invalid_argument) ;
   CPPUNIT_LOG_EXCEPTION(uuid("f47ac10b 58cc-4372-a567-0e02b2c3d479"), std::invalid_argument) ;
   CPPUNIT_LOG_EXCEPTION(uuid("f47ac10b-58cc 4372-a567-0e02b2c3d479"), std::invalid_argument) ;
   CPPUNIT_LOG_EXCEPTION(uuid("f47ac10b-58cc-4372 a567-0e02b2c3d479"), std::invalid_argument) ;
   CPPUNIT_LOG_EXCEPTION(uuid("f47ac10b-58cc-4372-a56750e02b2c3d479", RAISE_ERROR), std::invalid_argument) ;
   CPPUNIT_LOG_EXCEPTION(uuid("f47ac10b-58cc-4372-a56750e02b2c3d4790", RAISE_ERROR), std::invalid_argument) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(other_uuid < random_uuid) ;
   CPPUNIT_LOG_ASSERT(small_uuid < other_uuid) ;
   CPPUNIT_LOG_IS_FALSE(random_uuid < small_uuid) ;
   CPPUNIT_LOG_ASSERT(small_uuid < random_uuid) ;
   CPPUNIT_LOG_ASSERT(uuid() < small_uuid) ;
}

void UUIDFixture::Test_Cast128()
{
   uuid mutable_uuid ("f47ac10b-58cc-4372-a567-0e02b2c3d479") ;
   binary128_t mutable_bin ((binary128_t)mutable_uuid) ;

   const uuid const_uuid  ("007ac10b-58cc-4372-a567-0e02b2c3d478") ;
   const binary128_t &const_bin {const_uuid} ;
   char buf[64] ;
   std::fill_n(buf, sizeof buf, 'A') ;

   CPPUNIT_LOG_EQ(mutable_uuid.to_string(), "f47ac10b-58cc-4372-a567-0e02b2c3d479") ;
   CPPUNIT_LOG_EQ(mutable_bin.to_string(),  "f47ac10b58cc4372a5670e02b2c3d479") ;

   std::fill_n(buf + 0, sizeof buf, 'A') ;
   CPPUNIT_LOG_EQ(strslice(mutable_uuid.to_strbuf(buf)), "f47ac10b-58cc-4372-a567-0e02b2c3d479") ;
   std::fill_n(buf + 0, sizeof buf, 'A') ;
   CPPUNIT_LOG_EQ(strslice(mutable_bin.to_strbuf(buf)), "f47ac10b58cc4372a5670e02b2c3d479") ;

   CPPUNIT_LOG_EQ(string_cast(mutable_uuid),"f47ac10b-58cc-4372-a567-0e02b2c3d479") ;
   CPPUNIT_LOG_EQ(string_cast(mutable_bin), "f47ac10b58cc4372a5670e02b2c3d479") ;

   CPPUNIT_LOG_EQ(const_uuid.to_string(), "007ac10b-58cc-4372-a567-0e02b2c3d478") ;
   CPPUNIT_LOG_EQ(const_bin.to_string(),  "007ac10b58cc4372a5670e02b2c3d478") ;
   CPPUNIT_LOG_EQ(string_cast(const_uuid),"007ac10b-58cc-4372-a567-0e02b2c3d478") ;
   CPPUNIT_LOG_EQ(string_cast(const_bin), "007ac10b58cc4372a5670e02b2c3d478") ;

   CPPUNIT_LOG_EQUAL(cast128<uuid>(const_bin), const_uuid) ;
   CPPUNIT_LOG_EQUAL(cast128<uuid>(mutable_bin), mutable_uuid) ;

   CPPUNIT_LOG_ASSERT(std::is_reference<decltype(cast128<uuid>(const_bin))>()) ;
   CPPUNIT_LOG_ASSERT(std::is_const<std::remove_reference_t<decltype(cast128<uuid>(const_bin))>>()) ;

   CPPUNIT_LOG_ASSERT(std::is_reference<decltype(cast128<uuid>(mutable_bin))>()) ;
   CPPUNIT_LOG_IS_FALSE(std::is_const<std::remove_reference_t<decltype(cast128<uuid>(mutable_bin))>>()) ;
}

void UUIDFixture::Test_Binary256()
{
   PCOMN_STATIC_CHECK(!binary256_t()) ;
   PCOMN_STATIC_CHECK(binary256_t::size() == 32) ;
   PCOMN_STATIC_CHECK(binary256_t::slen() == 64) ;

   PCOMN_STATIC_CHECK(!binary256_t(0, 0, 0, 0)) ;
   PCOMN_STATIC_CHECK(binary256_t(0, 0, 0, 1)) ;

   static constexpr binary256_t b1(0, 0, 0, 1) ;
   PCOMN_STATIC_CHECK(*(b1.idata() + 3) == 1) ;

   CPPUNIT_LOG_EQUAL(binary256_t(0, 0, 0, 1), binary256_t(0, 0, 0, 1)) ;
   CPPUNIT_LOG_NOT_EQUAL(binary256_t(0, 0, 0, 1), binary256_t()) ;
   CPPUNIT_LOG_NOT_EQUAL(binary256_t(0, 3, 0, 1), binary256_t(0, 0, 0, 1)) ;
   CPPUNIT_LOG_EQUAL(binary256_t(0, 3, 0, 1), binary256_t(0, 3, 0, 1)) ;
   CPPUNIT_LOG_NOT_EQUAL(binary256_t(0, 3, 0, 1), binary256_t(0, 3, 0, 2)) ;

   CPPUNIT_LOG_EQ(string_cast(binary256_t(0, 3, 0, 1)),
                  "0000000000000001000000000000000000000000000000030000000000000000") ;

   CPPUNIT_LOG_EQUAL(binary256_t(string_cast(binary256_t(0, 3, 0, 1)).c_str()), binary256_t(0, 3, 0, 1)) ;
}

/*******************************************************************************
              class MACFixture
*******************************************************************************/
class MACFixture : public unit::TestFixture<MAC_FIXTURE> {
      typedef unit::TestFixture<MAC_FIXTURE> ancestor ;

      void Test_Empty_MAC() ;
      void Test_MAC() ;

      CPPUNIT_TEST_SUITE(MACFixture) ;

      CPPUNIT_TEST(Test_Empty_MAC) ;
      CPPUNIT_TEST(Test_MAC) ;

      CPPUNIT_TEST_SUITE_END() ;
} ;

void MACFixture::Test_Empty_MAC()
{
   CPPUNIT_LOG_ASSERT(std::is_trivially_copyable<MAC>::value) ;

   CPPUNIT_LOG_IS_FALSE(MAC()) ;
   CPPUNIT_LOG_EQUAL(MAC::size(), (size_t)6) ;
   CPPUNIT_LOG_EQUAL(MAC(), MAC()) ;
   CPPUNIT_LOG_IS_FALSE(MAC() != MAC()) ;
   CPPUNIT_LOG_IS_FALSE(MAC() < MAC()) ;
   CPPUNIT_LOG_IS_FALSE(MAC() > MAC()) ;
   CPPUNIT_LOG_ASSERT(MAC() >= MAC()) ;
   CPPUNIT_LOG_ASSERT(MAC() <= MAC()) ;
   const MAC null_mac ;
   for (unsigned i = 0 ; i < MAC::size() ; ++i)
      CPPUNIT_IS_FALSE(null_mac.octet(i)) ;

   union {
         char mem[sizeof(MAC)] ;
         uint64_t _ ;
   } storage ;
   memset(&storage, 'Q', sizeof storage) ;
   const MAC *null_a = new (&storage) MAC ;
   CPPUNIT_LOG_ASSERT(null_a) ;
   CPPUNIT_LOG_IS_FALSE(*null_a) ;

   CPPUNIT_LOG(std::endl) ;
}

void MACFixture::Test_MAC()
{
   const MAC random_mac ("E0:CB:4E:8C:FF:5C") ;
   const MAC other_mac  ("e0:CB:4E:8C:4f:5C") ;
   const MAC small_mac  ("E0:CB:4E:8C:4f:50") ;
   const MAC other_mac_2 (0xE0, 0xCB, 0x4E, 0x8C, 0x4f, 0x5C) ;
   const MAC other_mac_3 (0xE0CB4E8C4f5CULL) ;

   union raw_mac {
         constexpr raw_mac() : _u64() {}
         uint64_t _u64 ;
         MAC      _mac ;
   } ;

   CPPUNIT_LOG_ASSERT(random_mac) ;
   CPPUNIT_LOG_EQ(random_mac.to_string(), "E0:CB:4E:8C:FF:5C") ;
   CPPUNIT_LOG_ASSERT(small_mac) ;
   CPPUNIT_LOG_EQ(small_mac.to_string(),  "E0:CB:4E:8C:4F:50") ;
   CPPUNIT_LOG_ASSERT(other_mac) ;
   CPPUNIT_LOG_EQ(other_mac.to_string(),  "E0:CB:4E:8C:4F:5C") ;

   CPPUNIT_LOG_EQUAL(other_mac, other_mac_2) ;

   CPPUNIT_LOG_NOT_EQUAL(random_mac, MAC()) ;
   CPPUNIT_LOG_NOT_EQUAL(random_mac, small_mac) ;
   CPPUNIT_LOG_NOT_EQUAL(random_mac, other_mac) ;
   CPPUNIT_LOG_NOT_EQUAL(small_mac, other_mac) ;

   const MAC null_mac ("00:00:00:00:00:00", RAISE_ERROR) ;
   CPPUNIT_LOG_EQUAL(null_mac, MAC()) ;
   // MAC constructor with invalid argument string must create NULL MAC
   CPPUNIT_LOG_EQUAL(null_mac, MAC("E0:CB:4E:8C:4F:5", DONT_RAISE_ERROR)) ;

   CPPUNIT_LOG(std::endl) ;
   // Invalid formats
   CPPUNIT_LOG_IS_FALSE(MAC("E0:CB:4E:8C:4F:5", DONT_RAISE_ERROR)) ;
   CPPUNIT_LOG_EXCEPTION(MAC("E0:CB:4E:8C:4F:5"), std::invalid_argument) ;
   CPPUNIT_LOG_IS_FALSE(MAC(nullptr)) ;
   CPPUNIT_LOG_IS_FALSE(MAC(nullptr, RAISE_ERROR)) ;
   CPPUNIT_LOG_IS_FALSE(MAC("", RAISE_ERROR)) ;
   CPPUNIT_LOG_IS_FALSE(MAC(strslice(), RAISE_ERROR)) ;
   CPPUNIT_LOG_IS_FALSE(MAC(std::string(), RAISE_ERROR)) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_EXCEPTION(MAC("E0:CB:4E:8C:FF:5C "), std::invalid_argument) ;
   CPPUNIT_LOG_EXCEPTION(MAC(" E0:CB:4E:8C:FF:5C"), std::invalid_argument) ;
   CPPUNIT_LOG_EXCEPTION(MAC("G0:CB:4E:8C:FF:5C"), std::invalid_argument) ;
   CPPUNIT_LOG_EXCEPTION(MAC("E00CB:4E:8C:FF:5C"), std::invalid_argument) ;
   CPPUNIT_LOG_EXCEPTION(MAC("E0:CB 4E:8C:FF:5C", RAISE_ERROR), std::invalid_argument) ;
   CPPUNIT_LOG_EXCEPTION(MAC("E0:CB:4E:8C:FF05C", RAISE_ERROR), std::invalid_argument) ;
   CPPUNIT_LOG_EXCEPTION(MAC("E0:CB:4E:8CFFFF5C", RAISE_ERROR), std::invalid_argument) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(other_mac < random_mac) ;
   CPPUNIT_LOG_ASSERT(small_mac < other_mac) ;
   CPPUNIT_LOG_IS_FALSE(random_mac < small_mac) ;
   CPPUNIT_LOG_ASSERT(small_mac < random_mac) ;
   CPPUNIT_LOG_ASSERT(MAC() < small_mac) ;

   raw_mac other_raw ;
   raw_mac random_raw ;
   raw_mac small_raw ;

   CPPUNIT_LOG_RUN(other_raw._mac = other_mac) ;
   CPPUNIT_LOG_RUN(random_raw._mac = random_mac) ;
   CPPUNIT_LOG_RUN(small_raw._mac = small_mac) ;

   CPPUNIT_LOG_ASSERT(other_raw._u64 < random_raw._u64) ;
   CPPUNIT_LOG_ASSERT(small_raw._u64 < other_raw._u64) ;
   CPPUNIT_LOG_ASSERT(small_raw._u64 < random_raw._u64) ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(UUIDFixture::suite()) ;
   runner.addTest(MACFixture::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv, NULL, "UUID and MAC tests.") ;
}
