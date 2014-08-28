/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
/*******************************************************************************
 FILE         :   unittest_sslutils.cpp
 COPYRIGHT    :   Yakov Markovitch, 2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   SSL utilities/helpers tests

 CREATION DATE:   14 May 2014
*******************************************************************************/
#include <pcomn_ssl/pcomn_sslutils.h>
#include <pcomn_unittest.h>
#include <pcomn_calgorithm.h>

#include <stdlib.h>
#include <stdio.h>

using namespace pcomn ;

typedef std::vector<cstrptr> cstrptr_vector ;

extern const char SSL_UTILS_FIXTURE[] = "ssl-utils" ;

/*******************************************************************************
                            class SSLUtilsTests
*******************************************************************************/
class SSLUtilsTests : public pcomn::unit::TestFixture<SSL_UTILS_FIXTURE> {
    typedef pcomn::unit::TestFixture<SSL_UTILS_FIXTURE> ancestor ;

    void Test_Iostream_BIO() ;
    void Test_X509_Certificate_Read() ;
    void Test_X509_Certificate_Issue() ;

    CPPUNIT_TEST_SUITE(SSLUtilsTests) ;

    CPPUNIT_TEST(Test_Iostream_BIO) ;
    CPPUNIT_TEST(Test_X509_Certificate_Read) ;
    CPPUNIT_TEST(Test_X509_Certificate_Issue) ;

    CPPUNIT_TEST_SUITE_END() ;

public:
    void setUp()
    {
        ancestor::setUp() ;

        www_facebook_com_pem = at_testdir_abs("www.facebook.com.pem") ;
        www_google_ru_pem = at_testdir_abs("www.google.ru.pem") ;
        www_httpsnow_org_pem = at_testdir_abs("www.httpsnow.org.pem") ;
        www_python_org_pem = at_testdir_abs("www.python.org.pem") ;
        www_twitter_com_pem = at_testdir_abs("www.twitter.com.pem") ;

        ca_pem = at_testdir_abs("ca.pem") ;
        issuer_key_pem = at_testdir_abs("issuer.key.pem") ;
        server_key_pem = at_testdir_abs("server.key.pem") ;

        broken_pem = at_testdir_abs("broken.pem") ;
    }

protected:
    // Paths of test certificates at pcomn_ssl/unittests directory
    std::string www_facebook_com_pem ;
    std::string www_google_ru_pem ;
    std::string www_httpsnow_org_pem ;
    std::string www_python_org_pem ;
    std::string www_twitter_com_pem ;

    std::string broken_pem ; /* Path to a broken .pem file */

    std::string ca_pem ;
    std::string issuer_key_pem ;
    std::string server_key_pem ;
} ;

void SSLUtilsTests::Test_Iostream_BIO()
{
}

void SSLUtilsTests::Test_X509_Certificate_Read()
{
    CPPUNIT_LOG_EXCEPTION(ssl_load_cert(nullptr), std::invalid_argument) ;
    CPPUNIT_LOG_EXCEPTION(ssl_load_cert(broken_pem), pcomn::ssl_error) ;

    std::unique_ptr<X509> cert ;
    std::unique_ptr<X509> other_cert ;
    std::unique_ptr<EVP_PKEY> pubkey ;

    CPPUNIT_LOG_ASSERT(cert = ssl_load_cert(www_twitter_com_pem)) ;

    CPPUNIT_LOG_EQUAL(cstrptr(ssl_cstr(PCOMN_SSL_ENSURE(X509_get_subject_name(cert.get())), NID_commonName, RAISE_ERROR)),
                      cstrptr("twitter.com")) ;

    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG_EQUAL(append_container(cstrptr_vector(), CPPUNIT_SORTED(ssl_subject_alt_names(cert))),
                      (cstrptr_vector{"twitter.com", "www.twitter.com"})) ;

    CPPUNIT_LOG_EQUAL(cstrptr(ssl_cstr(PCOMN_SSL_ENSURE(X509_get_subject_name(cert.get())), NID_commonName, RAISE_ERROR)),
                      cstrptr("twitter.com")) ;

    CPPUNIT_LOG_ASSERT((pubkey = ssl_ensure_pubkey(cert))) ;

    CPPUNIT_LOG_EQUAL(md5hash(cert), md5hash_t("aa9cfa743ee49b92da8e9fe0b4e1ec18")) ;

    CPPUNIT_LOG_ASSERT(ssl_get_pubkey(cert)) ;

    CPPUNIT_LOG_ASSERT(ssl_key_match(*pubkey, *pubkey)) ;
    CPPUNIT_LOG_ASSERT(ssl_key_match(*pubkey, *ssl_get_pubkey(cert))) ;
    CPPUNIT_LOG_IS_FALSE(ssl_key_match(*pubkey, *std::unique_ptr<EVP_PKEY>())) ;
    CPPUNIT_LOG_ASSERT(ssl_key_match(*cert, *pubkey)) ;

    CPPUNIT_LOG_ASSERT(other_cert = ssl_load_cert(www_facebook_com_pem)) ;
    CPPUNIT_LOG_ASSERT(ssl_key_match(*other_cert, *ssl_get_pubkey(other_cert))) ;
    CPPUNIT_LOG_IS_FALSE(ssl_key_match(*other_cert, *pubkey)) ;

    CPPUNIT_LOG(std::endl) ;
    cert_issuer issuer (ca_pem) ;

    CPPUNIT_LOG_EXCEPTION_MSG(cert_issuer{www_facebook_com_pem}, ssl_error, "read private key") ;
    CPPUNIT_LOG_EXCEPTION_MSG(cert_issuer{at_testdir_abs("ca.private.key.mismatch.pem")}, ssl_error, "key values mismatch") ;

    CPPUNIT_LOG(std::endl) ;

    std::unique_ptr<EVP_PKEY> issuer_pubkey ;
    std::unique_ptr<EVP_PKEY> issuer_privkey ;

    CPPUNIT_LOG_ASSERT((issuer_privkey = ssl_load_private_key(issuer_key_pem))) ;
    CPPUNIT_LOG_ASSERT((issuer_pubkey = ssl_load_public_key(issuer_key_pem))) ;

    CPPUNIT_LOG_ASSERT(ssl_key_match(issuer_privkey, issuer.privkey())) ;
    CPPUNIT_LOG_ASSERT(ssl_key_match(issuer_pubkey, ssl_get_pubkey(issuer.cert()))) ;
}

void SSLUtilsTests::Test_X509_Certificate_Issue()
{
    std::unique_ptr<X509> cert ;
    std::unique_ptr<X509> other_cert ;
    std::unique_ptr<EVP_PKEY> pubkey ;
    std::unique_ptr<EVP_PKEY> new_pubkey ;

    std::unique_ptr<X509> issuer_cert ;
    std::unique_ptr<EVP_PKEY> issuer_privkey ;

    // Load twitter.com certificate
    CPPUNIT_LOG_ASSERT(cert = ssl_load_cert(www_twitter_com_pem)) ;
    CPPUNIT_LOG_EQUAL(cstrptr(ssl_cstr(PCOMN_SSL_ENSURE(X509_get_subject_name(cert.get())), NID_commonName, RAISE_ERROR)),
                      cstrptr("twitter.com")) ;
    CPPUNIT_LOG_EQUAL(md5hash(cert), md5hash_t("aa9cfa743ee49b92da8e9fe0b4e1ec18")) ;

    // Get its public key
    CPPUNIT_LOG_ASSERT((pubkey = ssl_ensure_pubkey(cert))) ;
    // Get new public key
    CPPUNIT_LOG_ASSERT((new_pubkey = ssl_load_public_key(server_key_pem))) ;

    CPPUNIT_LOG(std::endl) ;

    CPPUNIT_LOG_LINE(*cert) ;
    CPPUNIT_LOG_ASSERT((other_cert = ssl_dup_cert(cert))) ;
    CPPUNIT_LOG_LINE(*other_cert) ;

    CPPUNIT_LOG_ASSERT(ssl_key_match(ssl_ensure_pubkey(other_cert), pubkey)) ;

    CPPUNIT_LOG_ASSERT((other_cert = ssl_dup_cert(cert, 777))) ;
    CPPUNIT_LOG_LINE(*other_cert) ;
    CPPUNIT_LOG_ASSERT(ssl_key_match(ssl_ensure_pubkey(other_cert), pubkey)) ;

    // Duplicate the certificate with new public key
    CPPUNIT_LOG_ASSERT((other_cert = ssl_dup_cert(other_cert, new_pubkey, 777))) ;
    CPPUNIT_LOG_LINE(*other_cert) ;
    CPPUNIT_LOG_IS_FALSE(ssl_key_match(ssl_ensure_pubkey(other_cert), pubkey)) ;

    // Load both the issuer certificate and its private key
    CPPUNIT_LOG_ASSERT((issuer_cert = ssl_load_cert(ca_pem))) ;
    CPPUNIT_LOG_ASSERT((issuer_privkey = ssl_load_private_key(ca_pem))) ;
    CPPUNIT_LOG_LINE(*issuer_cert) ;
    // Check the issuer certificate and the private key match
    CPPUNIT_LOG_ASSERT(ssl_key_match(issuer_cert, issuer_privkey)) ;

    // Sign the duplicate certificate
    CPPUNIT_LOG_RUN(ssl_sign_cert(other_cert, issuer_privkey)) ;
    // Save the signed certificate
    CPPUNIT_LOG_RUN(ssl_save_cert(at_data_dir("twitter.signed.1.pem"), other_cert)) ;

    // We didn't put Subject Issuer into other_cert, so check must fail
    CPPUNIT_LOG_LINE("Issuer Name: '" << *X509_get_issuer_name(other_cert.get()) << "'") ;
    CPPUNIT_LOG_IS_FALSE(ssl_check_issued(issuer_cert, other_cert)) ;
    CPPUNIT_LOG_EXCEPTION_MSG(ssl_ensure_issued(issuer_cert, other_cert), ssl_error, "issuer mismatch") ;

    CPPUNIT_LOG(std::endl) ;
    // Try again
    CPPUNIT_LOG_ASSERT((other_cert = ssl_dup_cert(cert, new_pubkey, 777))) ;
    CPPUNIT_LOG_IS_FALSE(ssl_check_issued(issuer_cert, other_cert)) ;
    /// Not signed at all
    CPPUNIT_LOG_EXCEPTION(ssl_ensure_issued(issuer_cert, other_cert), ssl_error) ;
    /// Set subject issuer
    CPPUNIT_LOG_RUN(PCOMN_SSL_ENSURE(X509_set_issuer_name(other_cert.get(),
                                                          PCOMN_SSL_ENSURE(X509_get_subject_name(issuer_cert.get()))))) ;
    // Again sign and save
    CPPUNIT_LOG_RUN(ssl_sign_cert(other_cert, issuer_privkey)) ;
    CPPUNIT_LOG_RUN(ssl_save_cert(at_data_dir("twitter.signed.2.pem"), other_cert)) ;

    // Now everything must be OK
    CPPUNIT_LOG_RUN(ssl_ensure_issued(issuer_cert, other_cert)) ;
    CPPUNIT_LOG_ASSERT(ssl_check_issued(issuer_cert, other_cert)) ;

    CPPUNIT_LOG(std::endl) ;
    CPPUNIT_LOG_EXCEPTION_MSG(cert_issuer("foobar.pem"), ssl_error, "certificate file") ;
    CPPUNIT_LOG_EXCEPTION_MSG(cert_issuer(at_data_dir("twitter.signed.2.pem")), ssl_error, "private key") ;

    cert_issuer issuer (ca_pem) ;
    std::unique_ptr<X509> icert ;

    CPPUNIT_LOG_ASSERT(ssl_key_match(issuer.privkey(), issuer_privkey)) ;

    CPPUNIT_LOG_ASSERT((icert = ssl_dup_cert(cert))) ;

    CPPUNIT_LOG_IS_FALSE(ssl_check_issued(issuer.cert(), icert)) ;

    CPPUNIT_LOG_RUN(issuer.issue(icert, new_pubkey, 888)) ;
    CPPUNIT_LOG_RUN(ssl_save_cert(at_data_dir("twitter.signed.3.pem"), icert)) ;

    CPPUNIT_LOG_RUN(ssl_ensure_issued(issuer.cert(), icert)) ;
    CPPUNIT_LOG_RUN(ssl_ensure_issued(issuer.cert(), icert)) ;
    CPPUNIT_LOG_RUN(ssl_ensure_issued(issuer_cert,   icert)) ;
    CPPUNIT_LOG_ASSERT(ssl_key_match(icert, new_pubkey)) ;
}

int main(int argc, char *argv[])
{
    SSL_load_error_strings() ;
    SSL_library_init() ;

    pcomn::unit::TestRunner runner ;
    runner.addTest(SSLUtilsTests::suite()) ;

    return
        pcomn::unit::run_tests(runner, argc, argv, "unittest.trace.ini",
                               "Testing SSL utilities.") ;
}
