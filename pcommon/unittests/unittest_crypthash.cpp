/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_crypthash.cpp
 COPYRIGHT    :   Yakov Markovitch, 2011-2018. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for cryptohash classes/functions (MD5, SHA1, SHA256 etc.)

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   6 Oct 2011
*******************************************************************************/
#include <pcomn_hash.h>
#include <pcomn_path.h>
#include <pcomn_handle.h>
#include <pcomn_unittest.h>

#include "pcomn_testhelpers.h"

using namespace pcomn ;
using namespace pcomn::unit ;

extern const char CRYPTHASH_FIXTURE[] = "crypthash" ;

/*******************************************************************************
              class CryptHashFixture
*******************************************************************************/
class CryptHashFixture : public pcomn::unit::TestFixture<CRYPTHASH_FIXTURE> {
      typedef pcomn::unit::TestFixture<CRYPTHASH_FIXTURE> ancestor ;

      void Test_MD5Hash() ;
      void Test_SHA1Hash() ;
      void Test_SHA256Hash() ;

      CPPUNIT_TEST_SUITE(CryptHashFixture) ;

      CPPUNIT_TEST(Test_MD5Hash) ;
      CPPUNIT_TEST(Test_SHA1Hash) ;
      CPPUNIT_TEST(Test_SHA256Hash) ;

      CPPUNIT_TEST_SUITE_END() ;

      std::string datadir ;
      #define DEFNAME(filenum) std::string f##filenum
      DEFNAME(10) ;
      DEFNAME(20) ;
      DEFNAME(30) ;
      DEFNAME(3) ;
      DEFNAME(11) ;
      DEFNAME(16) ;
      DEFNAME(0) ;
      DEFNAME(20000) ;
      DEFNAME(8192) ;
      #undef DEFNAME

   public:
      void setUp()
      {
         ancestor::setUp() ;
         datadir = path::abspath<std::string>(dataDir()) ;

         #define DEFNAME(filenum) f##filenum = datadir + ("/"#filenum".txt")
         DEFNAME(10) ;
         DEFNAME(20) ;
         DEFNAME(30) ;
         DEFNAME(3) ;
         DEFNAME(11) ;
         DEFNAME(16) ;
         DEFNAME(0) ;
         DEFNAME(20000) ;
         DEFNAME(8192) ;
         #undef DEFNAME

         generate_seqn_file<8>(f10.c_str(), 0, 10) ;
         generate_seqn_file<8>(f20.c_str(), 10, 30) ;
         generate_seqn_file<8>(f30.c_str(), 0, 30) ;
         generate_seqn_file<8>(f3.c_str(), 0, 3) ;
         generate_seqn_file<8>(f11.c_str(), 3, 14) ;
         generate_seqn_file<8>(f16.c_str(), 14, 30) ;
         generate_seqn_file<8>(f0.c_str(), 0, 0) ;
         generate_seqn_file<8>(f20000.c_str(), 0, 20000) ;
         generate_seqn_file<8>(f8192.c_str(), 0, 8192) ;
      }

      template<typename T>
      static constexpr T be(T value) { return value_to_big_endian(value) ; }
} ;

void CryptHashFixture::Test_MD5Hash()
{
   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_IS_FALSE(md5hash_t()) ;
   CPPUNIT_LOG_IS_FALSE(md5hash()) ;
   CPPUNIT_LOG_EQUAL(md5hash(), md5hash_t()) ;

   CPPUNIT_LOG_EQUAL(md5hash().to_string(), std::string("00000000000000000000000000000000")) ;
   CPPUNIT_LOG_EQUAL(md5hash(), md5hash_t("00000000000000000000000000000000")) ;

   // MD5 of empty string
   CPPUNIT_LOG_EQUAL(md5hash_file(f0.c_str()).to_string(), std::string("d41d8cd98f00b204e9800998ecf8427e")) ;
   CPPUNIT_LOG_EQUAL(md5hash_file(f0.c_str()), md5hash_t("d41d8cd98f00b204e9800998ecf8427e")) ;
   CPPUNIT_LOG_EQUAL(md5hash_t("d41d8cd98f00b204e9800998ecf8427e"),
                     md5hash_t(binary128_t{0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04, 0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e})) ;
   CPPUNIT_LOG_NOT_EQUAL(md5hash_file(f0.c_str()), md5hash_t()) ;
   CPPUNIT_LOG_NOT_EQUAL(md5hash_file(f0.c_str()), md5hash_t("d41d8cd98f00b204e9800998ecf8427f")) ;

   CPPUNIT_LOG_EQUAL(md5hash("", 0).to_string(), std::string("d41d8cd98f00b204e9800998ecf8427e")) ;
   CPPUNIT_LOG_EXCEPTION(md5hash(NULL, 1), std::invalid_argument) ;
   CPPUNIT_LOG_EQUAL(md5hash(NULL, 0).to_string(), std::string("d41d8cd98f00b204e9800998ecf8427e")) ;

   CPPUNIT_LOG_EQUAL(md5hash_file(f10.c_str()), md5hash_t("f01569bf52df95ae55ac208f0d0ba8f6")) ;
   CPPUNIT_LOG_EQUAL(md5hash_file(f20.c_str()), md5hash_t("2d4c258a584ef0349202773e093261ec")) ;
   CPPUNIT_LOG_EQUAL(md5hash_file(f30.c_str()), md5hash_t("37c700f0300223e7b1adefef927809de")) ;
   CPPUNIT_LOG_EQUAL(md5hash_file(f3.c_str()), md5hash_t("171335943ed3403f45e90457a7e7f12c")) ;
   CPPUNIT_LOG_EQUAL(md5hash_file(f11.c_str()), md5hash_t("b1b9ac1eb7ac3179bb21c070337b85c0")) ;
   CPPUNIT_LOG_EQUAL(md5hash_file(f16.c_str()), md5hash_t("47c632e7d22fad7254786994946a017f")) ;
   CPPUNIT_LOG_EQUAL(md5hash_file(f8192.c_str()), md5hash_t("fa81534d5beb66b72c8acb613aa6f2db")) ;
   CPPUNIT_LOG_EQUAL(md5hash_file(f20000.c_str()), md5hash_t("47cda3f0617a7876d716fd341291a7b9")) ;

   CPPUNIT_LOG_EQUAL(md5hash_file(fd_safehandle(open(f10.c_str(), O_RDONLY))), md5hash_t("f01569bf52df95ae55ac208f0d0ba8f6")) ;
   CPPUNIT_LOG_EQUAL(md5hash_file(fd_safehandle(open(f20.c_str(), O_RDONLY))), md5hash_t("2d4c258a584ef0349202773e093261ec")) ;

   CPPUNIT_LOG(std::endl) ;
   MD5Hash h ;
   CPPUNIT_LOG_EQUAL(h.size(), (size_t)0) ;
   CPPUNIT_LOG_IS_FALSE(h.value()) ;
   CPPUNIT_LOG_EQUAL(h.value(), md5hash()) ;
   CPPUNIT_LOG_EXCEPTION(h.append_data(NULL, 1), std::invalid_argument) ;
   CPPUNIT_LOG_IS_FALSE(h.value()) ;
   CPPUNIT_LOG_EQUAL(h.value(), md5hash()) ;

   CPPUNIT_LOG_EQUAL(h.append_data("", 0).size(), (size_t)0) ;
   CPPUNIT_LOG_ASSERT(h.value()) ;
   CPPUNIT_LOG_EQUAL(h.value(), md5hash_t("d41d8cd98f00b204e9800998ecf8427e")) ;
   CPPUNIT_LOG_EQUAL(h.append_file(f10.c_str()).size(), (size_t)80) ;
   CPPUNIT_LOG_EQUAL(h.value(), md5hash_t("f01569bf52df95ae55ac208f0d0ba8f6")) ;
   CPPUNIT_LOG_EQUAL(h.append_file(f20.c_str()).size(), (size_t)240) ;
   CPPUNIT_LOG_EQUAL(h.value(), md5hash_t("37c700f0300223e7b1adefef927809de")) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(h = MD5Hash()) ;
   CPPUNIT_LOG_IS_FALSE(h.value()) ;
   CPPUNIT_LOG_EQUAL(h.size(), (size_t)0) ;
   CPPUNIT_LOG_EXCEPTION(h.append_file((FILE*)NULL), std::invalid_argument) ;

   CPPUNIT_LOG_EQUAL(h.append_file(FILE_safehandle(fopen(f3.c_str(), "r"))).size(), (size_t)24) ;
   CPPUNIT_LOG_EQUAL(h.value(), md5hash_t("171335943ed3403f45e90457a7e7f12c")) ;
   CPPUNIT_LOG_EQUAL(h.append_file(FILE_safehandle(fopen(f11.c_str(), "r"))).size(), (size_t)112) ;
   CPPUNIT_LOG_EQUAL(h.append_file(FILE_safehandle(fopen(f16.c_str(), "r"))).size(), (size_t)240) ;
   CPPUNIT_LOG_EQUAL(h.value(), md5hash_t("37c700f0300223e7b1adefef927809de")) ;

   CPPUNIT_LOG(std::endl) ;
   size_t s = 0 ;
   CPPUNIT_LOG_EQUAL(md5hash_file(FILE_safehandle(fopen(f10.c_str(), "r")), &s), md5hash_t("f01569bf52df95ae55ac208f0d0ba8f6")) ;
   CPPUNIT_LOG_EQUAL(s, (size_t)80) ;
   s = 0 ;
   CPPUNIT_LOG_EQUAL(md5hash_file(f10.c_str(), &s), md5hash_t("f01569bf52df95ae55ac208f0d0ba8f6")) ;
   CPPUNIT_LOG_EQUAL(s, (size_t)80) ;


   // Check hashtalble hasher for MD5 hash objects
   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(pcomn::valhash(md5hash_t("fa81534d5beb66b72c8acb613aa6f2db"))) ;
   CPPUNIT_LOG_ASSERT(pcomn::valhash(md5hash_t("47cda3f0617a7876d716fd341291a7b9"))) ;
   CPPUNIT_LOG_NOT_EQUAL(pcomn::valhash(md5hash_t("fa81534d5beb66b72c8acb613aa6f2db")),
                         pcomn::valhash(md5hash_t("47cda3f0617a7876d716fd341291a7b9"))) ;

   // Check MD5 POD objects
   CPPUNIT_LOG(std::endl) ;
   union local1 {
         pcomn::md5hash_t md5 ;
         double dummy ;
   } ;
}

void CryptHashFixture::Test_SHA1Hash()
{
   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_IS_FALSE(sha1hash_t()) ;
   CPPUNIT_LOG_IS_FALSE(sha1hash()) ;
   CPPUNIT_LOG_EQUAL(sha1hash(), sha1hash_t()) ;

   CPPUNIT_LOG_EQUAL(sha1hash().to_string(), std::string("0000000000000000000000000000000000000000")) ;
   CPPUNIT_LOG_EQUAL(sha1hash(), sha1hash_t("0000000000000000000000000000000000000000")) ;

   // SHA1 of empty string
   CPPUNIT_LOG_EQUAL(sha1hash_file(f0.c_str()).to_string(), std::string("da39a3ee5e6b4b0d3255bfef95601890afd80709")) ;
   CPPUNIT_LOG_EQUAL(sha1hash_file(f0.c_str()), sha1hash_t("da39a3ee5e6b4b0d3255bfef95601890afd80709")) ;
   CPPUNIT_LOG_EQUAL(sha1hash("", 0), sha1hash_t("da39a3ee5e6b4b0d3255bfef95601890afd80709")) ;

   CPPUNIT_LOG_NOT_EQUAL(sha1hash_file(f0.c_str()), sha1hash_t()) ;
   CPPUNIT_LOG_NOT_EQUAL(sha1hash_file(f0.c_str()), sha1hash_t("Fa39a3ee5e6b4b0d3255bfef95601890afd80709")) ;

   CPPUNIT_LOG_EQUAL(sha1hash("", 0).to_string(), std::string("da39a3ee5e6b4b0d3255bfef95601890afd80709")) ;
   CPPUNIT_LOG_EXCEPTION(sha1hash(NULL, 1), std::invalid_argument) ;
   CPPUNIT_LOG_EQUAL(sha1hash(NULL, 0).to_string(), std::string("da39a3ee5e6b4b0d3255bfef95601890afd80709")) ;

   CPPUNIT_LOG_EQUAL(sha1hash_file(f10.c_str()), sha1hash_t("00e2a2560e228d75e5eee5b59ff6459bfe2eb6d2")) ;
   CPPUNIT_LOG_EQUAL(sha1hash_file(f20.c_str()), sha1hash_t("ed703f7e4b79cae2ad24203a318bdea50ac59b1c")) ;
   CPPUNIT_LOG_EQUAL(sha1hash_file(f30.c_str()), sha1hash_t("0ee2a7d9fcddf8d5d4b215c90d776b12a8bea3ec")) ;
   CPPUNIT_LOG_EQUAL(sha1hash_file(f3.c_str()), sha1hash_t("1221df24908920e6c785fc6f3ecc279df4b68811")) ;
   CPPUNIT_LOG_EQUAL(sha1hash_file(f11.c_str()), sha1hash_t("ba4103c0b87c94cfc6dc3897ede2b5253d7da25a")) ;
   CPPUNIT_LOG_EQUAL(sha1hash_file(f16.c_str()), sha1hash_t("85d8f4d847f3f79bd5d36f5b7fa327afc43be9e6")) ;
   CPPUNIT_LOG_EQUAL(sha1hash_file(f8192.c_str()), sha1hash_t("1aa501b8ba9a38ff309a3b506b05021244482431")) ;
   CPPUNIT_LOG_EQUAL(sha1hash_file(f20000.c_str()), sha1hash_t("592686abc75e68e4121cdbb416f302a5636adc58")) ;

   CPPUNIT_LOG_EQUAL(sha1hash_file(fd_safehandle(open(f10.c_str(), O_RDONLY))),
                     sha1hash_t("00e2a2560e228d75e5eee5b59ff6459bfe2eb6d2")) ;
   CPPUNIT_LOG_EQUAL(sha1hash_file(fd_safehandle(open(f20.c_str(), O_RDONLY))),
                     sha1hash_t("ed703f7e4b79cae2ad24203a318bdea50ac59b1c")) ;

   CPPUNIT_LOG(std::endl) ;
   SHA1Hash h ;
   CPPUNIT_LOG_EQUAL(h.size(), (size_t)0) ;
   CPPUNIT_LOG_IS_FALSE(h.value()) ;
   CPPUNIT_LOG_EQUAL(h.value(), sha1hash()) ;
   CPPUNIT_LOG_EXCEPTION(h.append_data(NULL, 1), std::invalid_argument) ;
   CPPUNIT_LOG_IS_FALSE(h.value()) ;
   CPPUNIT_LOG_EQUAL(h.value(), sha1hash()) ;

   CPPUNIT_LOG_EQUAL(h.append_data("", 0).size(), (size_t)0) ;
   CPPUNIT_LOG_ASSERT(h.value()) ;
   CPPUNIT_LOG_EQUAL(h.value(), sha1hash_t("da39a3ee5e6b4b0d3255bfef95601890afd80709")) ;
   CPPUNIT_LOG_EQUAL(h.append_file(f10.c_str()).size(), (size_t)80) ;
   CPPUNIT_LOG_EQUAL(h.value(), sha1hash_t("00e2a2560e228d75e5eee5b59ff6459bfe2eb6d2")) ;
   CPPUNIT_LOG_EQUAL(h.append_file(f20.c_str()).size(), (size_t)240) ;
   CPPUNIT_LOG_EQUAL(h.value(), sha1hash_t("0ee2a7d9fcddf8d5d4b215c90d776b12a8bea3ec")) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(h = SHA1Hash()) ;
   CPPUNIT_LOG_IS_FALSE(h.value()) ;
   CPPUNIT_LOG_EQUAL(h.size(), (size_t)0) ;
   CPPUNIT_LOG_EXCEPTION(h.append_file((FILE*)NULL), std::invalid_argument) ;

   CPPUNIT_LOG_EQUAL(h.append_file(FILE_safehandle(fopen(f3.c_str(), "r"))).size(), (size_t)24) ;
   CPPUNIT_LOG_EQUAL(h.value(), sha1hash_t("1221df24908920e6c785fc6f3ecc279df4b68811")) ;
   CPPUNIT_LOG_EQUAL(h.append_file(FILE_safehandle(fopen(f11.c_str(), "r"))).size(), (size_t)112) ;
   CPPUNIT_LOG_EQUAL(h.append_file(FILE_safehandle(fopen(f16.c_str(), "r"))).size(), (size_t)240) ;
   CPPUNIT_LOG_EQUAL(h.value(), sha1hash_t("0ee2a7d9fcddf8d5d4b215c90d776b12a8bea3ec")) ;

   CPPUNIT_LOG(std::endl) ;
   size_t s = 0 ;
   CPPUNIT_LOG_EQUAL(sha1hash_file(FILE_safehandle(fopen(f10.c_str(), "r")), &s),
                     sha1hash_t("00e2a2560e228d75e5eee5b59ff6459bfe2eb6d2")) ;
   CPPUNIT_LOG_EQUAL(s, (size_t)80) ;
   s = 0 ;
   CPPUNIT_LOG_EQUAL(sha1hash_file(f10.c_str(), &s),
                     sha1hash_t("00e2a2560e228d75e5eee5b59ff6459bfe2eb6d2")) ;
   CPPUNIT_LOG_EQUAL(s, (size_t)80) ;


   // Check hashtalble hasher for SHA1 hash objects
   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(pcomn::valhash(sha1hash_t("1221df24908920e6c785fc6f3ecc279df4b68811"))) ;
   CPPUNIT_LOG_ASSERT(pcomn::valhash(sha1hash_t("592686abc75e68e4121cdbb416f302a5636adc58"))) ;
   CPPUNIT_LOG_NOT_EQUAL(pcomn::valhash(sha1hash_t("1221df24908920e6c785fc6f3ecc279df4b68811")),
                         pcomn::valhash(sha1hash_t("592686abc75e68e4121cdbb416f302a5636adc58"))) ;

   // Check SHA1 POD objects
   CPPUNIT_LOG(std::endl) ;
   union local1 {
         pcomn::sha1hash_t sha1 ;
         double dummy ;
   } ;
}

void CryptHashFixture::Test_SHA256Hash()
{
   CPPUNIT_LOG(std::endl) ;

   CPPUNIT_LOG_IS_FALSE(sha256hash_t()) ;
   CPPUNIT_LOG_IS_FALSE(sha256hash()) ;
   CPPUNIT_LOG_EQUAL(sha256hash(), sha256hash_t()) ;

   CPPUNIT_LOG_EQUAL(sha256hash().to_string(), std::string("0000000000000000000000000000000000000000000000000000000000000000")) ;
   CPPUNIT_LOG_EQUAL(sha256hash(), sha256hash_t("0000000000000000000000000000000000000000000000000000000000000000")) ;

   CPPUNIT_LOG("sha256hash_t vs binary256_t" << std::endl
               << "\tsha256('', 0)=" << sha256hash("", 0) << std::endl
               << "\tsha256hash_t(e3bc..)=" << sha256hash_t("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855") << std::endl
               << "\tbinary256_t(e3bc..)=" << binary256_t("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855") << std::endl
               << "\tsha256hash_t(binary256_t(e3bc..))="
                   << sha256hash_t(binary256_t("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")) << std::endl) ;
   CPPUNIT_LOG_NOT_EQUAL(sha256hash_t("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"),
                         sha256hash_t(binary256_t("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"))) ;
   // SHA256 of empty string
   CPPUNIT_LOG_EQUAL(sha256hash_file(f0.c_str()).to_string(), std::string("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")) ;
   CPPUNIT_LOG_EQUAL(sha256hash_file(f0.c_str()), sha256hash_t("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")) ;
   CPPUNIT_LOG_EQUAL(sha256hash("", 0), sha256hash_t("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")) ;
   CPPUNIT_LOG_EQUAL(sha256hash_t("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"),
                     sha256hash_t(binary256_t{be(0xe3b0c44298fc1c14ULL),
                                              be(0x9afbf4c8996fb924ULL),
                                              be(0x27ae41e4649b934cULL),
                                              be(0xa495991b7852b855ULL)})) ;
   CPPUNIT_LOG_NOT_EQUAL(sha256hash_file(f0.c_str()), sha256hash_t()) ;
   CPPUNIT_LOG_NOT_EQUAL(sha256hash_file(f0.c_str()), sha256hash_t("f3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")) ;

   CPPUNIT_LOG_EQUAL(sha256hash("", 0).to_string(), std::string("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")) ;
   CPPUNIT_LOG_EXCEPTION(sha256hash(NULL, 1), std::invalid_argument) ;
   CPPUNIT_LOG_EQUAL(sha256hash(NULL, 0).to_string(), std::string("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")) ;

   CPPUNIT_LOG_EQUAL(sha256hash_file(f10.c_str()), sha256hash_t("6f870f39d85c5c7239f605b927caf158c160540263674ff2f7be481f3c3356b5")) ;
   CPPUNIT_LOG_EQUAL(sha256hash_file(f20.c_str()), sha256hash_t("2023d9d7e7834fff05246a44746ddaea83bdde11e7dc3729e294906ee8db38aa")) ;
   CPPUNIT_LOG_EQUAL(sha256hash_file(f30.c_str()), sha256hash_t("1f84679648da093f61f875d1472c72ce56b80bb73e007259210731586f95bb9d")) ;
   CPPUNIT_LOG_EQUAL(sha256hash_file(f3.c_str()), sha256hash_t("6d70857e02c945dde5473497dcd6e5beb9e8c9dd67ab9bbfa301e35551102da1")) ;
   CPPUNIT_LOG_EQUAL(sha256hash_file(f11.c_str()), sha256hash_t("1031945767fc667b6e8c48b98ea41f0c053115131a6d29d09a8f1fc489b40579")) ;
   CPPUNIT_LOG_EQUAL(sha256hash_file(f16.c_str()), sha256hash_t("06f74a4d3ae03f0f5595b081c7788ab1d779ad22135d26dfdc565c8bf74e0a15")) ;
   CPPUNIT_LOG_EQUAL(sha256hash_file(f8192.c_str()), sha256hash_t("f016d3de61b5571284ac82f25c14d5d592f72d6e8dcd63656d29e6bccf31864b")) ;
   CPPUNIT_LOG_EQUAL(sha256hash_file(f20000.c_str()), sha256hash_t("7bc26cfd3efe365cfc619b0fc4f8dc02153d6935ac7a5a6fd051a5993ac66f29")) ;

   CPPUNIT_LOG_EQUAL(sha256hash_file(fd_safehandle(open(f10.c_str(), O_RDONLY))),
                     sha256hash_t("6f870f39d85c5c7239f605b927caf158c160540263674ff2f7be481f3c3356b5")) ;
   CPPUNIT_LOG_EQUAL(sha256hash_file(fd_safehandle(open(f20.c_str(), O_RDONLY))),
                     sha256hash_t("2023d9d7e7834fff05246a44746ddaea83bdde11e7dc3729e294906ee8db38aa")) ;

   CPPUNIT_LOG(std::endl) ;
   SHA256Hash h ;
   CPPUNIT_LOG_EQUAL(h.size(), (size_t)0) ;
   CPPUNIT_LOG_IS_FALSE(h.value()) ;
   CPPUNIT_LOG_EQUAL(h.value(), sha256hash()) ;
   CPPUNIT_LOG_EXCEPTION(h.append_data(NULL, 1), std::invalid_argument) ;
   CPPUNIT_LOG_IS_FALSE(h.value()) ;
   CPPUNIT_LOG_EQUAL(h.value(), sha256hash()) ;

   CPPUNIT_LOG_EQUAL(h.append_data("", 0).size(), (size_t)0) ;
   CPPUNIT_LOG_ASSERT(h.value()) ;
   CPPUNIT_LOG_EQUAL(h.value(), sha256hash_t("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")) ;
   CPPUNIT_LOG_EQUAL(h.append_file(f10.c_str()).size(), (size_t)80) ;
   CPPUNIT_LOG_EQUAL(h.value(), sha256hash_t("6f870f39d85c5c7239f605b927caf158c160540263674ff2f7be481f3c3356b5")) ;
   CPPUNIT_LOG_EQUAL(h.append_file(f20.c_str()).size(), (size_t)240) ;
   CPPUNIT_LOG_EQUAL(h.value(), sha256hash_t("1f84679648da093f61f875d1472c72ce56b80bb73e007259210731586f95bb9d")) ;

   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_RUN(h = SHA256Hash()) ;
   CPPUNIT_LOG_IS_FALSE(h.value()) ;
   CPPUNIT_LOG_EQUAL(h.size(), (size_t)0) ;
   CPPUNIT_LOG_EXCEPTION(h.append_file((FILE*)NULL), std::invalid_argument) ;

   CPPUNIT_LOG_EQUAL(h.append_file(FILE_safehandle(fopen(f3.c_str(), "r"))).size(), (size_t)24) ;
   CPPUNIT_LOG_EQUAL(h.value(), sha256hash_t("6d70857e02c945dde5473497dcd6e5beb9e8c9dd67ab9bbfa301e35551102da1")) ;
   CPPUNIT_LOG_EQUAL(h.append_file(FILE_safehandle(fopen(f11.c_str(), "r"))).size(), (size_t)112) ;
   CPPUNIT_LOG_EQUAL(h.append_file(FILE_safehandle(fopen(f16.c_str(), "r"))).size(), (size_t)240) ;
   CPPUNIT_LOG_EQUAL(h.value(), sha256hash_t("1f84679648da093f61f875d1472c72ce56b80bb73e007259210731586f95bb9d")) ;

   CPPUNIT_LOG(std::endl) ;
   size_t s = 0 ;
   CPPUNIT_LOG_EQUAL(sha256hash_file(FILE_safehandle(fopen(f10.c_str(), "r")), &s),
                     sha256hash_t("6f870f39d85c5c7239f605b927caf158c160540263674ff2f7be481f3c3356b5")) ;
   CPPUNIT_LOG_EQUAL(s, (size_t)80) ;
   s = 0 ;
   CPPUNIT_LOG_EQUAL(sha256hash_file(f10.c_str(), &s),
                     sha256hash_t("6f870f39d85c5c7239f605b927caf158c160540263674ff2f7be481f3c3356b5")) ;
   CPPUNIT_LOG_EQUAL(s, (size_t)80) ;


   // Check hashtalble hasher for SHA256 hash objects
   CPPUNIT_LOG(std::endl) ;
   CPPUNIT_LOG_ASSERT(pcomn::valhash(sha256hash_t("06f74a4d3ae03f0f5595b081c7788ab1d779ad22135d26dfdc565c8bf74e0a15"))) ;
   CPPUNIT_LOG_ASSERT(pcomn::valhash(sha256hash_t("6f870f39d85c5c7239f605b927caf158c160540263674ff2f7be481f3c3356b5"))) ;
   CPPUNIT_LOG_NOT_EQUAL(pcomn::valhash(sha256hash_t("06f74a4d3ae03f0f5595b081c7788ab1d779ad22135d26dfdc565c8bf74e0a15")),
                         pcomn::valhash(sha256hash_t("6f870f39d85c5c7239f605b927caf158c160540263674ff2f7be481f3c3356b5"))) ;

   // Check SHA256 POD objects
   CPPUNIT_LOG(std::endl) ;
   union local1 {
         pcomn::sha256hash_t hash ;
         double dummy ;
   } ;
}

int main(int argc, char *argv[])
{
   pcomn::unit::TestRunner runner ;
   runner.addTest(CryptHashFixture::suite()) ;

   return
      pcomn::unit::run_tests(runner, argc, argv, NULL, "Cryptographic hash classes tests.") ;
}
