/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_sslutils.cpp
 COPYRIGHT    :   Yakov Markovitch, 2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   SSL support/helper library

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   6 Apr 2014
*******************************************************************************/
#include "pcomn_sslutils.h"

#include <atomic>
#include <chrono>

#include <openssl/err.h>
#include <openssl/x509v3.h>

#include <string.h>
#include <stdio.h>

#define SSL_CHECK_RESULT(raise, ssl_result, ...)  \
    (ssl_check_result((raise), (ssl_result), __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__))

#define BIO_OPEN(bio_constructor, checkmode, ...) (PCOMN_SSL_##checkmode(BIO_##bio_constructor, ##__VA_ARGS__))
#define BIO_RFILE_OPEN(filename, checkmode, ...)  (BIO_OPEN(new_file((filename), "r"), checkmode, ##__VA_ARGS__))
#define BIO_WFILE_OPEN(filename, checkmode, ...)  (BIO_OPEN(new_file((filename), "w"), checkmode, ##__VA_ARGS__))

#define ENSURE_CPTR_ARG(arg) (deconst(&*PCOMN_ENSURE_ARG(arg)))

template<typename T>
static inline T *deconst(const T *ptr) { return const_cast<T *>(ptr) ; }

static inline bool bad_ssl_result(int ssl_result) { return ssl_result <= 0 ; }

template<typename T>
static inline bool bad_ssl_result(T *ssl_result) { return !ssl_result ; }

template<typename T>
static inline T &ssl_check_result(pcomn::RaiseError raise, T &&ssl_result, const pcomn::cstrptr &function,
                                  const pcomn::cstrptr &file, int line, const pcomn::cstrptr &msg)
{
    if (bad_ssl_result(ssl_result))
        raise
            ? pcomn::ssl_log_throw(function, file, line, msg)
            : pcomn::ssl_log_errors(function, file, line, msg) ;

    return ssl_result ;
}

/*******************************************************************************
 BIO over std::ostream
*******************************************************************************/
static int bio_ostream_write(BIO *bio, const char *buf, int size)
{
    std::ostream * const os = static_cast<std::ostream *>(bio->ptr) ;
    BIO_clear_retry_flags(bio) ;
    return os->write(buf, size) ? size : -1 ;
}

static int bio_ostream_puts(BIO *bio, const char *str)
{
    return !str ? 0 : bio_ostream_write(bio, str, strlen(str)) ;
}

BIO_METHOD *BIO_s_ostream()
{
    static const BIO_METHOD ostream_method =
        {
            BIO_TYPE_NONE,
            "std::ostream writer",

            bio_ostream_write,
            NULL,
            bio_ostream_puts,
            NULL,
            NULL,
            NULL,

            [](BIO *){ return 1 ; }, /* destroy */

            NULL
        } ;
    return const_cast<BIO_METHOD *>(&ostream_method) ;
}

BIO *BIO_new_ostream(std::ostream &os)
{
    BIO * const bio = BIO_new(BIO_s_ostream()) ;
    if(bio)
    {
        bio->ptr = &os ;
        bio->shutdown = 1 ;
        bio->init = 1 ;
        bio->flags |= BIO_FLAGS_WRITE ;
        bio->num = 0 ;
    }
    return bio ;
}

namespace pcomn {

std::unique_ptr<BIO> ssl_BIO(const cstrptr &filename, const cstrptr &mode, const cstrptr &msg, RaiseError raise)
{
    return std::unique_ptr<BIO>
        (SSL_CHECK_RESULT(raise, BIO_new_file(PCOMN_ENSURE_ARG(filename), mode), msg.c_str())) ;
}

std::unique_ptr<X509> ssl_load_cert(const cstrptr &filename)
{
    const std::unique_ptr<BIO> in
        (BIO_RFILE_OPEN(PCOMN_ENSURE_ARG(filename), ENSURE, "Cannot open certificate file")) ;

    return
        std::unique_ptr<X509>
        (PCOMN_SSL_ENSURE(PEM_read_bio_X509(in.get(), NULL, NULL, NULL),
                          "Cannot read a certificate")) ;
}

std::unique_ptr<EVP_PKEY> ssl_load_private_key(const cstrptr &filename)
{
    const std::unique_ptr<BIO> in
        (BIO_RFILE_OPEN(PCOMN_ENSURE_ARG(filename), ENSURE, "Cannot open a certificate or key file")) ;

    return std::unique_ptr<EVP_PKEY>
        (PCOMN_SSL_ENSURE(PEM_read_bio_PrivateKey(in.get(), NULL, NULL, NULL), "Cannot read private key")) ;
}

std::unique_ptr<EVP_PKEY> ssl_load_public_key(const cstrptr &filename)
{
    const std::unique_ptr<BIO> in
        (BIO_RFILE_OPEN(PCOMN_ENSURE_ARG(filename), ENSURE, "Cannot open a certificate or key file")) ;

    std::unique_ptr<EVP_PKEY> pkey (EVP_PKEY_new()) ;

    EVP_PKEY_assign_RSA(pkey.get(),
                        PCOMN_SSL_ENSURE(PEM_read_bio_RSAPrivateKey(in.get(), NULL, NULL, NULL),
                                         "Cannot read RSA public key")) ;
    return pkey ;
}

void ssl_save_cert(const pcomn::ptr_shim<BIO> &bio, const ptr_shim<const X509> &cert)
{
    PCOMN_SSL_ENSURE(PEM_write_bio_X509(PCOMN_ENSURE_ARG(bio), ENSURE_CPTR_ARG(cert)),
                     "Cannot write a certificate to a file") ;
}

void ssl_save_cert(const cstrptr &filename, const ptr_shim<const X509> &cert)
{
    const std::unique_ptr<BIO> bio
        (BIO_WFILE_OPEN(PCOMN_ENSURE_ARG(filename), ENSURE, "Cannot open writable file to save a certificate")) ;
    ssl_save_cert(bio, cert) ;
}

bool ssl_has_pubkey(const ptr_shim<const X509> &cert)
{
    return OBJ_obj2nid(PCOMN_ENSURE_ARG(cert)->cert_info->key->algor->algorithm) != NID_undef ;
}

std::unique_ptr<EVP_PKEY> ssl_get_pubkey(const ptr_shim<const X509> &cert)
{
    return std::unique_ptr<EVP_PKEY>(X509_get_pubkey(ENSURE_CPTR_ARG(cert))) ;
}

std::unique_ptr<EVP_PKEY> ssl_ensure_pubkey(const ptr_shim<const X509> &cert)
{
    return std::unique_ptr<EVP_PKEY>
        (PCOMN_SSL_ENSURE(X509_get_pubkey(const_cast<X509 *>(cert.get())), "Cannot get certificate public key")) ;
}

static const EVP_MD *key_digest_type(const EVP_PKEY *private_key)
{
    switch (EVP_PKEY_base_id(private_key))
    {
        case EVP_PKEY_RSA:
        case EVP_PKEY_RSA2:
        case EVP_PKEY_DSA2:
        case EVP_PKEY_DSA3:
        case EVP_PKEY_DSA4:
            return EVP_sha1() ;

        case EVP_PKEY_DSA:
        case EVP_PKEY_DSA1:
            return EVP_dss1() ;

        default:
            PCOMN_SSL_ENSURE(false, "Cannot select valid digest algorithm for a private key") ;
    }
    return EVP_md_null() ;
}

void ssl_sign_cert(const ptr_shim<X509> &cert, const ptr_shim<EVP_PKEY> &private_key)
{
    struct mdclean { void operator()(EVP_MD_CTX *ctx) const { EVP_MD_CTX_cleanup(ctx) ; } } ;

    EVP_MD_CTX md_ctx ;
    EVP_MD_CTX_init(&md_ctx) ;
    std::unique_ptr<EVP_MD_CTX, mdclean> guard (&md_ctx) ;
    EVP_PKEY_CTX *pkey_ctx = NULL ;

    PCOMN_SSL_ENSURE
        (EVP_DigestSignInit(&md_ctx, &pkey_ctx, key_digest_type(private_key), NULL, private_key),
         "Cannot init signing context") ;

    PCOMN_SSL_ENSURE(X509_sign_ctx(cert, &md_ctx), "Cannot sign a certificate") ;
}

static inline void ssl_set_serial(X509 *cert, X509 *src, uint64_t serial)
{
    using namespace std::chrono ;
    if (serial != (uint64_t)-1)
        // Set serial number. If serial is not specified, set the count of nanoseconds since
        // Unix Epoch
        ASN1_INTEGER_set(X509_get_serialNumber(cert),
                         serial ? serial : duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count()) ;
    else if (src)
        X509_set_serialNumber(cert, X509_get_serialNumber(src)) ;
}

/*
  { "nsComment", "\"OpenSSL Generated Certificate\"" },
  { "subjectKeyIdentifier", "hash" },
  { "authorityKeyIdentifier", "keyid,issuer:always" },
*/
std::unique_ptr<X509> ssl_dup_cert(const ptr_shim<X509> &src, const ptr_shim<EVP_PKEY> &new_pubkey, uint64_t serial)
{
    if (!src)
        return std::unique_ptr<X509>() ;

    // New ceritificate
    std::unique_ptr<X509> result
        (PCOMN_SSL_ENSURE(X509_new(), "Cannot create a new X509 object")) ;
    X509 *cert = result.get() ;

    // Set version number
    PCOMN_SSL_ENSURE(X509_set_version(cert, 2), "Cannot set certificate version") ;

    // Copy subject name
    PCOMN_SSL_ENSURE(X509_set_subject_name(cert,
                                           PCOMN_SSL_ENSURE(X509_get_subject_name(src),
                                                            "Cannot get certificate subject")),
                     "Cannot set certificate subject") ;

    // Set serial number.
    // If serial is -1, copy source's serial.
    // If serial is not specified (i.e. 0), set the count of nanoseconds since Unix Epoch
    ssl_set_serial(cert, src, serial) ;

    // Set new public key or copy the source one
    PCOMN_SSL_ENSURE(X509_set_pubkey(cert, new_pubkey ? new_pubkey.get() : ssl_ensure_pubkey(src).get()),
                     "Cannot set certificate public key") ;

    // Set certificate validity period
    /* set duration for the certificate */
    PCOMN_SSL_ENSURE(X509_set_notBefore(cert, X509_get_notBefore(src)), "Cannot set certificate start time") ;
    PCOMN_SSL_ENSURE(X509_set_notAfter(cert, X509_get_notAfter(src)), "Cannot set certificate end time") ;

    X509_EXTENSION *ext ;
    for (int i = 0 ; (ext = X509_get_ext(src, i)) != nullptr ; ++i)
        switch (OBJ_obj2nid(ext->object)) {
            case NID_subject_alt_name:
            case NID_key_usage:
            case NID_ext_key_usage:
            case NID_basic_constraints:
                PCOMN_SSL_ENSURE(X509_add_ext(cert, ext, -1), "Cannot append certificate extension") ;
                break ;
        }

    return result ;
}

bool ssl_check_issued(const ptr_shim<X509> &issuer, const ptr_shim<X509> &subject)
{
    return !X509_check_issued(PCOMN_ENSURE_ARG(issuer), PCOMN_ENSURE_ARG(subject)) ;
}

void ssl_ensure_issued(const ptr_shim<X509> &issuer, const ptr_shim<X509> &subject)
{
    if (const int errcode = X509_check_issued(PCOMN_ENSURE_ARG(issuer), PCOMN_ENSURE_ARG(subject)))
        throw ssl_error(X509_verify_cert_error_string(errcode)) ;
}

/*******************************************************************************

*******************************************************************************/
#define ENSURE_RESULT(result, exception, message)                       \
    do { if (!(result)) { conditional_throw<exception>(!!raise, (message)) ; return {} ; } } while(false)

openssl_cstrptr ssl_cstr(const ASN1_STRING *str, RaiseError raise)
{
    ENSURE_RESULT(str, std::invalid_argument, "NULL string argument passed to ssl_str") ;

    char *utf8 = NULL ;
    ENSURE_RESULT
        (ASN1_STRING_to_UTF8(reinterpret_cast<unsigned char **>(&utf8), const_cast<ASN1_STRING *>(str)) >= 0,
         std::invalid_argument, "Invalid ASN1 string, unable to convert to UTF8") ;

    return openssl_cstrptr(utf8) ;
}

openssl_cstrptr ssl_cstr(const X509_NAME *name, int nid, RaiseError raise)
{
    ENSURE_RESULT(name, std::invalid_argument, "NULL name argument passed to ssl_str") ;

    const int ndx = X509_NAME_get_index_by_NID(const_cast<X509_NAME *>(name), nid, -1) ;
    ENSURE_RESULT(ndx >= 0, std::invalid_argument, "Invalid NID passed to ssl_str") ;

    return ssl_cstr(X509_NAME_ENTRY_get_data(X509_NAME_get_entry(const_cast<X509_NAME *>(name), ndx))) ;
}

std::vector<openssl_cstrptr> ssl_subject_alt_names(const ptr_shim<X509> &cert, RaiseError raise)
{
    ENSURE_RESULT(cert, std::invalid_argument, "NULL string argument passed to ssl_subject_alt_names") ;

    const std::unique_ptr<GENERAL_NAMES> subj_altnames
        (static_cast<GENERAL_NAMES *>(X509_get_ext_d2i(cert, NID_subject_alt_name, NULL, NULL))) ;

    std::vector<openssl_cstrptr> result ;
    const unsigned size = subj_altnames ? sk_GENERAL_NAME_num(subj_altnames.get()) : 0 ;

    if (!size)
        return std::move(result) ;

    result.reserve(size) ;

    for (unsigned n = sk_GENERAL_NAME_num(subj_altnames.get()), i = 0 ; i < n ; ++i)
    {
        int nametype = 0 ;
        const ASN1_STRING * const value = static_cast<const ASN1_STRING *>
            (GENERAL_NAME_get0_value(sk_GENERAL_NAME_value(subj_altnames.get(), i), &nametype)) ;

        // Only DNS is of our interest
        if (value && nametype == GEN_DNS)
        {
            auto dns (ssl_cstr(value, raise)) ;
            if (!dns.empty())
                result.push_back(std::move(dns)) ;
        }
    }
    return std::move(result) ;
}

md5hash_t md5hash(const ptr_shim<const X509> &cert)
{
    md5hash_t digest ;
    X509_digest(PCOMN_ENSURE_ARG(cert), EVP_md5(), digest.data(), NULL) ;
    return digest ;
}

md5hash_t md5hash_pubkey(const ptr_shim<const X509> &cert)
{
    md5hash_t digest ;
    X509_pubkey_digest(PCOMN_ENSURE_ARG(cert), EVP_md5(), digest.data(), NULL) ;
    return digest ;
}

#undef ENSURE_RESULT

/*******************************************************************************
 cert_issuer
*******************************************************************************/
cert_issuer::cert_issuer(const pcomn::strslice &issuer_cert_pem)
{
    const std::string &pem_filename = issuer_cert_pem.stdstring() ;
    _cert = ssl_load_cert(pem_filename.c_str()) ;
    _privkey = ssl_load_private_key(pem_filename.c_str()) ;

    ensure_consistency(_cert, _privkey, pem_filename.c_str()) ;
}

void cert_issuer::ensure_consistency(const std::unique_ptr<X509> &issuer_cert,
                                     const std::unique_ptr<EVP_PKEY> &issuer_privkey,
                                     const char *filename)
{
    PCOMN_SSL_ENSURE(X509_check_private_key(issuer_cert.get(), issuer_privkey.get()), filename) ;
}

void cert_issuer::issue(const ptr_shim<X509> &request, const ptr_shim<EVP_PKEY> &new_pubkey, uint64_t serial) const
{
    // If serial is -1, keep the serial number of request.
    // If serial is 0, set it as the count of nanoseconds since Unix Epoch.
    ssl_set_serial(PCOMN_ENSURE_ARG(request), NULL, serial) ;

    if (new_pubkey)
        PCOMN_SSL_ENSURE(X509_set_pubkey(request, new_pubkey), "Cannot set certificate public key") ;

    /// Set subject issuer
    PCOMN_SSL_ENSURE(X509_set_issuer_name(request, PCOMN_SSL_ENSURE(X509_get_subject_name(cert()))),
                     "Cannot set Issuer Subject") ;
    // Sign the certificate
    ssl_sign_cert(request, privkey()) ;
}

/*******************************************************************************
 Diagnostics
*******************************************************************************/
// Default logger
static void ssl_default_errlogger(const char *function, const char *file, int line, const char *msg)
{
    const char *basename = strrchr(file, '/') ;
    basename = basename ? (basename + 1) : file ;

    function && *function
        ? fprintf(stderr, "<%s:%d (%s)> %s\n", basename, line, function, msg)
        : fprintf(stderr, "<%s:%d> %s\n", basename, line, msg) ;
}

static std::atomic<ssl_error_logger> log_errors { ssl_default_errlogger } ;

static void ssl_log(const char *locfunction, const char *locfile, int locline, const char *msg, char *buf, size_t bufsize)
{
    if (!locfile) locfile = "" ;
    if (!msg) msg = "" ;

    const unsigned long threadid = CRYPTO_thread_id() ;

    const char *file, *text ;
    int line, flags ;
    char strerrbuf[256] ;

    if (buf && bufsize)
    {
        *buf = 0 ;
        if (const unsigned long errcode = ERR_peek_error_line_data(&file, &line, &text, &flags))
            snprintf(buf, bufsize, "SSL::%lu:%s:%s:%d:%s%s%s",
                     threadid, ERR_error_string(errcode, strerrbuf),
                     file, line, (flags & ERR_TXT_STRING) ? text : "", *msg ? ": " : "", msg) ;
    }

    const ssl_error_logger log = log_errors ;

    while (const unsigned long errcode = ERR_get_error_line_data(&file, &line, &text, &flags))
    {
        char msgbuf[1024] ;
        snprintf(msgbuf, sizeof msgbuf, "SSL::%lu:%s:%s:%d:%s%s%s",
                 threadid, ERR_error_string(errcode, strerrbuf),
                 file, line, (flags & ERR_TXT_STRING) ? text : "", *msg ? ": " : "", msg) ;

        msg = "" ;

        log(locfunction, locfile, locline, msgbuf) ;
    }
}

ssl_error_logger ssl_set_error_logger(ssl_error_logger logger)
{
    return log_errors.exchange(logger ? logger : ssl_default_errlogger) ;
}

void ssl_log_errors(const cstrptr &function, const cstrptr &file, int line, const cstrptr &msg)
{
    ssl_log(function, file, line, msg, NULL, 0) ;
}

__noreturn void ssl_log_throw(const cstrptr &function, const cstrptr &file, int line, const cstrptr &msg)
{
    char buf[1024] ;
    *buf = 0 ;
    ssl_log(function, file, line, msg, buf, sizeof buf) ;
    throw ssl_error(buf) ;
}

} // end of namespace pcomn
