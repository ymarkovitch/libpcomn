/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
#ifndef __PCOMN_SSLUTILS_H
#define __PCOMN_SSLUTILS_H
/*******************************************************************************
 FILE         :   pcomn_sslutils.h
 COPYRIGHT    :   Yakov Markovitch, 2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   SSL support/helper library

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   6 Apr 2014
*******************************************************************************/
/** @file
 SSL helper classes, functions, macros

  @li BIO over std::ostream
  @li Specializations of std::default_delete template to facilitate std::unique_ptr
      for openssl objects, like BIO, X509, etc.
*******************************************************************************/
#include <pcommon.h>
#include <pcomn_strslice.h>
#include <pcomn_cstrptr.h>
#include <pcomn_safeptr.h>

#include <iostream>
#include <memory>
#include <vector>
#include <type_traits>

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/x509v3.h>
#include <openssl/evp.h>
#include <openssl/crypto.h>

#include <time.h>

/*******************************************************************************
 BIO over std::ostream
*******************************************************************************/
/// Get the method pointer for BIO-over-std::ostream
BIO_METHOD *BIO_s_ostream() ;

/// Get the new BIO over an std::ostream
BIO *BIO_new_ostream(std::ostream &os) ;

/*******************************************************************************
 Specializations of std::default_delete template to facilitate std::unique_ptr
 for openssl objects, like BIO, X509, etc.
*******************************************************************************/
namespace std {

/******************************************************************************/
/** A specialization of std::default_delete template for std::unique_ptr<BIO>
*******************************************************************************/
template<> struct default_delete<BIO> {
    void operator()(BIO *bio) const { if (bio) BIO_free_all(bio) ; }
} ;

#define PCOMN_DEFINE_SSL_DEFAULT_DELETE(OPENSSL_TYPE)                   \
    template<> struct default_delete<OPENSSL_TYPE > {                   \
        void operator()(OPENSSL_TYPE *x) const { if (x) OPENSSL_TYPE##_free(x) ; } \
    }

PCOMN_DEFINE_SSL_DEFAULT_DELETE(X509) ;
PCOMN_DEFINE_SSL_DEFAULT_DELETE(EVP_PKEY) ;
PCOMN_DEFINE_SSL_DEFAULT_DELETE(SSL_CTX) ;
PCOMN_DEFINE_SSL_DEFAULT_DELETE(ASN1_STRING) ;
PCOMN_DEFINE_SSL_DEFAULT_DELETE(GENERAL_NAMES) ;

#undef PCOMN_DEFINE_SSL_DEFAULT_DELETE

} // end of namespace std

namespace pcomn {

/******************************************************************************/
/** std::ostream output manipulators for certificate output
*******************************************************************************/
template<typename> struct x509omanip ;

template<>
struct x509omanip<X509> {
    x509omanip(const X509 *x, unsigned long nmflags, unsigned long cflags) :
        _x509(x), _nmflags(nmflags), _cflags(cflags)
    {}
    std::ostream &operator()(std::ostream &os) const
    {
        if (!_x509)
            return os << "(null)" ;

        const std::unique_ptr<BIO> bio (BIO_new_ostream(os)) ;
        X509_print_ex(bio.get(), const_cast<X509 *>(_x509), _nmflags, _cflags) ;
        (void)BIO_flush(bio.get()) ;
        return os ;
    }

    friend std::ostream &operator<<(std::ostream &os, const x509omanip &manip)
    {
        return manip(os) ;
    }
private:
    const X509 * const  _x509 ;
    const unsigned long _nmflags ;
    const unsigned long _cflags ;
} ;

template<>
struct x509omanip<X509_NAME> {
    x509omanip(const X509_NAME *n, unsigned long nmflags, unsigned long ) :
        _x509name(n), _nmflags(nmflags)
    {}

    std::ostream &operator()(std::ostream &os) const
    {
        if (!_x509name)
            return os << "(null)" ;

        const std::unique_ptr<BIO> bio (BIO_new_ostream(os)) ;
        X509_NAME_print_ex(bio.get(), const_cast<X509_NAME *>(_x509name), 0, _nmflags) ;
        (void)BIO_flush(bio.get()) ;
        return os ;
    }
private:
    const X509_NAME * const  _x509name ;
    const unsigned long      _nmflags ;
} ;

template<typename T>
inline std::ostream &operator<<(std::ostream &os, const x509omanip<T> &manip)
{
    return manip(os) ;
}

/// Output manipulator for certificate output to std::ostream
template<typename T>
inline x509omanip<T> X509out(const T *x,
                             unsigned long nmflags = XN_FLAG_COMPAT,
                             unsigned long cflags = X509_FLAG_NO_SIGDUMP|X509_FLAG_NO_PUBKEY)
{
    return x509omanip<T>(x, nmflags, cflags) ;
}

/// @overload
template<typename T>
inline x509omanip<T> X509out(const std::unique_ptr<T> &x,
                             unsigned long nmflags = XN_FLAG_COMPAT,
                             unsigned long cflags = X509_FLAG_NO_SIGDUMP|X509_FLAG_NO_PUBKEY)
{
    return x509omanip<T>(x.get(), nmflags, cflags) ;
}

/*******************************************************************************
 SSL error handling
*******************************************************************************/
#define PCOMN_SSL_CHECK(ssl_result, ...)  ( ::pcomn::ssl_check((ssl_result), __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__) )
#define PCOMN_SSL_ENSURE(ssl_result, ...) ( ::pcomn::ssl_ensure((ssl_result), __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__) )

/// SSL error log callback type
typedef void (*ssl_error_logger)(const char *function, const char *file, int line, const char *msg) ;

/******************************************************************************/
/** SSL error exception
*******************************************************************************/
struct ssl_error : public std::runtime_error {
    explicit ssl_error(const char *msg) : std::runtime_error(msg) {}
    explicit ssl_error(const std::string &msg) : std::runtime_error(msg) {}
} ;

/// Set SSL error logging function
///
/// Both ssl_log_errors and ssl_log_throw use this function to log SSL error stack;
/// thus ssl_check(), ssl_ensure(), PCOMN_SSL_CHECK(), PCOMN_SSL_ENSURE() use it as well.
/// The default logger prints SSL errors to stderr.
///
/// @param logger Error logging function; if NULL, the default logger is set.
///
/// @return The old (previously set) logger.
///
ssl_error_logger ssl_set_error_logger(ssl_error_logger logger) ;

void ssl_log_errors(const cstrptr &function, const cstrptr &file, int line, const cstrptr &msg) ;

__noreturn void ssl_log_throw(const cstrptr &function, const cstrptr &file, int line, const cstrptr &msg) ;

template<typename T>
inline T *ssl_check(T *ssl_result, const cstrptr &function, const cstrptr &file, int line, const cstrptr &msg)
{
    if (!ssl_result)
        ssl_log_errors(function, file, line, msg) ;
    return ssl_result ;
}

template<typename T>
inline T *ssl_ensure(T *ssl_result, const cstrptr &function, const cstrptr &file, int line, const cstrptr &msg={})
{
    if (!ssl_result)
        ssl_log_throw(function, file, line, msg) ;
    return ssl_result ;
}

inline int ssl_check(int ssl_result, const cstrptr &function, const cstrptr &file, int line, const cstrptr &msg={})
{
    if (ssl_result <= 0)
        ssl_log_errors(function, file, line, msg) ;
    return ssl_result ;
}

inline int ssl_ensure(int ssl_result, const cstrptr &function, const cstrptr &file, int line, const cstrptr &msg={})
{
    if (ssl_result <= 0)
        ssl_log_throw(function, file, line, msg) ;
    return ssl_result ;
}

/*******************************************************************************
 ASN1 strings, GENERAL_NAME
*******************************************************************************/
/******************************************************************************/
/** Deleter object for raw openssl data
*******************************************************************************/
struct openssl_data_deleter {
    void operator()(const void *data) const { OPENSSL_free(const_cast<void *>(data)) ; }
} ;

/******************************************************************************/
/** Safepointer for raw openssl data
*******************************************************************************/
template<typename T>
using openssl_data_ptr = std::unique_ptr<T, openssl_data_deleter> ;

typedef basic_cstrptr<char, openssl_data_deleter> openssl_cstrptr ;

/// Get ASN1 string data in UTF8 encoding
openssl_cstrptr ssl_cstr(const ASN1_STRING *, RaiseError = DONT_RAISE_ERROR) ;

openssl_cstrptr ssl_cstr(const X509_NAME *, int nid, RaiseError = DONT_RAISE_ERROR) ;

template<typename T, typename D>
inline openssl_cstrptr ssl_cstr(const std::unique_ptr<T, D> &value, RaiseError raise = DONT_RAISE_ERROR)
{
    return ssl_cstr(value.get(), raise) ;
}

template<typename T, typename D>
inline openssl_cstrptr ssl_cstr(const std::unique_ptr<T, D> &value, int nid, RaiseError raise = DONT_RAISE_ERROR)
{
    return ssl_cstr(value.get(), nid, raise) ;
}

/// Get all DNS Subject Alternative Name values for a certificate
std::vector<openssl_cstrptr> ssl_subject_alt_names(const ptr_shim<X509> &cert, RaiseError = DONT_RAISE_ERROR) ;

/*******************************************************************************

*******************************************************************************/
std::unique_ptr<BIO> ssl_BIO(const pcomn::cstrptr &filename, const pcomn::cstrptr &mode,
                             const pcomn::cstrptr &raise_msg={},
                             RaiseError = RAISE_ERROR) ;

/// Load X509 certificate from PEM file
///
/// @throw ssl_error
std::unique_ptr<X509> ssl_load_cert(const cstrptr &filename) ;

/// Load a private key from PEM file
///
/// @throw ssl_error
std::unique_ptr<EVP_PKEY> ssl_load_private_key(const cstrptr &filename) ;

/// Load a public key from PEM file with a keypair
///
/// @throw ssl_error
std::unique_ptr<EVP_PKEY> ssl_load_public_key(const cstrptr &filename) ;

void ssl_save_cert(const pcomn::ptr_shim<BIO> &bio, const ptr_shim<const X509> &cert) ;

void ssl_save_cert(const cstrptr &filename, const ptr_shim<const X509> &cert) ;

/// Check if a certificate is signed
///
bool ssl_has_pubkey(const ptr_shim<const X509> &cert) ;

/// Get certificate's public key
///
/// @return Public key or NULL if !ssl_has_pubkey(cert)
///
std::unique_ptr<EVP_PKEY> ssl_get_pubkey(const ptr_shim<const X509> &cert) ;

/// Get certificate's public key or throw ssl_error if the certificate is not signed
///
/// @throw ssl_error
std::unique_ptr<EVP_PKEY> ssl_ensure_pubkey(const ptr_shim<const X509> &cert) ;

/// Sign a certificate
///
/// Determine message digest type automatically, depending on private key type
void ssl_sign_cert(const ptr_shim<X509> &cert, const ptr_shim<EVP_PKEY> &private_key) ;

/// Duplicate a certificate, create a certificate copy
///
/// @param src          Source certificate
/// @param new_pubkey   New public key to set to the duplicate certificate; if NULL, use
///                     the original certificate's public key.
/// @param serial       Serial number of the new certificate; if 0, generate a new 64-bit
///                     serial number as a count of nanoseconds since Unix Epoch
std::unique_ptr<X509> ssl_dup_cert(const ptr_shim<X509> &src, const ptr_shim<EVP_PKEY> &new_pubkey, uint64_t serial = 0) ;

inline std::unique_ptr<X509> ssl_dup_cert(const ptr_shim<X509> &src, uint64_t serial = 0)
{
    return ssl_dup_cert(src, nullptr, serial) ;
}

/// Get MD5 digest of X509 certificate
md5hash_t md5hash(const ptr_shim<const X509> &cert) ;

/// Get MD5 digest of the public key of a X509 certificate
md5hash_t md5hash_pubkey(const ptr_shim<const X509> &cert) ;

inline bool ssl_key_match(const ptr_shim<const EVP_PKEY> &key1, const ptr_shim<const EVP_PKEY> &key2)
{
    return key1 && key2 && (key1.get() == key2.get() || EVP_PKEY_cmp(key1.get(), key2.get()) > 0) ;
}

inline bool ssl_key_match(const ptr_shim<const X509> &cert, const ptr_shim<const EVP_PKEY> &key2)
{
    return ssl_key_match(ssl_get_pubkey(cert), key2) ;
}

inline bool ssl_key_match(const EVP_PKEY &key1, const EVP_PKEY &key2) { return ssl_key_match(&key1, &key2) ; }

inline bool ssl_key_match(const X509 &cert, const EVP_PKEY &key2) { return ssl_key_match(&cert, &key2) ; }

/// Indicate whether @a subject is issued by @a issuer
bool ssl_check_issued(const ptr_shim<X509> &issuer, const ptr_shim<X509> &subject) ;

/// Ensure @a subject is issued by @a issuer
///
/// @throw ssl_error if @a subject is not issued by @a issuer
///
void ssl_ensure_issued(const ptr_shim<X509> &issuer, const ptr_shim<X509> &subject) ;

namespace detail {
template<int lockndx, typename T>
inline T *ssl_incref(T *v) { if (v) CRYPTO_add(&v->references, 1, lockndx) ; return v ; }
}

#define PCOMN_DEFINE_SSL_INCREF(typ) \
inline typ *ssl_incref(const ptr_shim<typ> &v) { return detail::ssl_incref<CRYPTO_LOCK_##typ>(v.get()) ; }

PCOMN_DEFINE_SSL_INCREF(X509) ;
PCOMN_DEFINE_SSL_INCREF(EVP_PKEY) ;
PCOMN_DEFINE_SSL_INCREF(SSL_CTX) ;

/******************************************************************************/
/** Certificate issuer
*******************************************************************************/
class cert_issuer {
public:
    explicit cert_issuer(const pcomn::strslice &issuer_cert_pem) ;

    cert_issuer(std::unique_ptr<X509> &&issuer_cert, std::unique_ptr<EVP_PKEY> &&issuer_privkey)
    {
        // Ensure the certificate and private key match
        ensure_consistency(PCOMN_ENSURE_ARG(issuer_cert), PCOMN_ENSURE_ARG(issuer_privkey), NULL) ;
        _cert = std::move(issuer_cert) ;
        _privkey = std::move(issuer_privkey) ;
    }

    cert_issuer(cert_issuer &&) = default ;
    cert_issuer &operator=(cert_issuer &&) = default ;

    /// Issue a signed certificate based on already existing certificate (@em not a
    /// certificate request)
    ///
    void issue(const ptr_shim<X509> &request, const ptr_shim<EVP_PKEY> &new_pubkey = nullptr, uint64_t serial = 0) const ;

    /// @overload
    void issue(const ptr_shim<X509> &request, uint64_t serial) const { issue(request, nullptr, serial) ; }

    /// Get issuer's certificate: never NULL
    X509 *cert() const { return _cert.get() ; }
    /// Get issuer's private key: never NULL
    EVP_PKEY *privkey() const { return _privkey.get() ; }

private:
    std::unique_ptr<X509>       _cert ;     /* Issuer's certificate */
    std::unique_ptr<EVP_PKEY>   _privkey ;  /* Issuer's private key */

    void ensure_consistency(const std::unique_ptr<X509> &issuer_cert,
                            const std::unique_ptr<EVP_PKEY> &issuer_privkey,
                            const char *msg) ;
} ;

} // end of namespace pcomn

/*******************************************************************************
 std::ostream output for openssl objects
*******************************************************************************/
inline std::ostream &operator<<(std::ostream &os, const X509 &v)
{
    return os << pcomn::X509out(&v) ;
}

inline std::ostream &operator<<(std::ostream &os, const X509_NAME &v)
{
    return os << pcomn::X509out(&v) ;
}

#endif /* __PCOMN_SSLUTILS_H */
