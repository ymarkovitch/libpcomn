/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   unittest_crypthash.cpp
 COPYRIGHT    :   Yakov Markovitch, 2011-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Unittests for cryptohash classes/functions (MD5, SHA1, etc.)

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   6 Oct 2011
*******************************************************************************/
#include <pcomn_hash.h>
#include <pcomn_path.h>
#include <pcomn_handle.h>
#include <pcomn_unittest.h>

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

      CPPUNIT_TEST_SUITE(CryptHashFixture) ;

      CPPUNIT_TEST(Test_MD5Hash) ;
      CPPUNIT_TEST(Test_SHA1Hash) ;

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
   CPPUNIT_LOG_ASSERT(pcomn::hasher(md5hash_t("fa81534d5beb66b72c8acb613aa6f2db"))) ;
   CPPUNIT_LOG_ASSERT(pcomn::hasher(md5hash_t("47cda3f0617a7876d716fd341291a7b9"))) ;
   CPPUNIT_LOG_NOT_EQUAL(pcomn::hasher(md5hash_t("fa81534d5beb66b72c8acb613aa6f2db")),
                         pcomn::hasher(md5hash_t("47cda3f0617a7876d716fd341291a7b9"))) ;

   // Check MD5 POD objects
   CPPUNIT_LOG(std::endl) ;
   union local1 {
         pcomn::md5hash_pod_t md5 ;
         double dummy ;
   } ;
}

void CryptHashFixture::Test_SHA1Hash()
{
   CPPUNIT_LOG(std::endl) ;

   // Check SHA1 POD objects
   CPPUNIT_LOG(std::endl) ;
   union local1 {
         pcomn::sha1hash_pod_t sha1 ;
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
