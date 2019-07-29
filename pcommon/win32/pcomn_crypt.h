#ifndef __PCOMN_CRYPT_H
#define __PCOMN_CRYPT_H
/*******************************************************************************
 FILE         :   pcomn_crypt.h
 COPYRIGHT    :   Yakov Markovitch, 2001-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Win32 CryptAPI wrapper

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   14 Jun 2001
*******************************************************************************/
#include <windows.h>
#include <wincrypt.h>
#include <pcomn_except.h>
#include <sptrbase.h>
#include <fcntl.h>
#include <pcomn_buffer.h>

namespace pcomn {

namespace Crypt {

class crypt_error : public system_error {} ;

struct Handle : public PRefCount {
      Handle(unsigned long h = 0) :
         _handle(h)
      {}

      static BOOL check_result(BOOL result)
      {
         conditional_throw<crypt_error>(!result) ;
         return result ;
      }
   protected:
      unsigned long _handle ;
} ;

struct ContextHandle : public Handle {
      ContextHandle(HCRYPTPROV handle = 0) :
         Handle(handle)
      {}
      ~ContextHandle() { _handle && ::CryptReleaseContext(*this, 0) ; }
      operator HCRYPTPROV() const { return (HCRYPTPROV)_handle ; }
} ;

struct KeyHandle : public Handle {
      KeyHandle(HCRYPTKEY handle = 0) :
         Handle(handle)
      {}
      ~KeyHandle() { _handle && ::CryptDestroyKey(*this) ; }
      operator HCRYPTKEY() const { return (HCRYPTKEY)_handle ; }
} ;

struct HashHandle : public Handle {
      HashHandle(HCRYPTHASH handle = 0) :
         Handle(handle)
      {}
      ~HashHandle() { _handle && ::CryptDestroyHash(*this) ; }
      operator HCRYPTHASH() const { return (HCRYPTHASH)_handle ; }
} ;

/*******************************************************************************
                     class Crypt::Context
*******************************************************************************/
class Context {

   public:
      // Constructor.
      // Gets the default key context/container for the RSA_FULL provider type
      //
      Context(const char *name = NULL, unsigned flags = 0)
      {
         HCRYPTPROV provider = 0 ;
         BOOL result ;
         if (!(flags & (O_WRONLY | O_RDWR)))
            result = _open_context(provider, NULL, CRYPT_VERIFYCONTEXT) ;
         else
         {
            result = _open_context(provider, name, 0) ;
            // If context doesn't exist and we are requested to create, do it!
            if (!result && (flags & O_CREAT) && GetLastError() == (unsigned)NTE_BAD_KEYSET)
               result = _open_context(provider, name, CRYPT_NEWKEYSET) ;
         }
         Handle::check_result(result) ;
         _handle = shared_intrusive_ptr<ContextHandle> (new ContextHandle(provider)) ;
      }

      HCRYPTPROV handle() const { return *_handle ; }
      operator HCRYPTPROV() const { return handle() ; }

   private:
      shared_intrusive_ptr<ContextHandle> _handle ;

      static BOOL _open_context(HCRYPTPROV &provider,
                                const char *name,
                                unsigned effective)
      {
         return CryptAcquireContextA(&provider,
                                     name,
                                     MS_DEF_PROV,
                                     PROV_RSA_FULL,
                                     effective) ;
      }
} ;

/*******************************************************************************
                     class Crypt::Hash
 A cryptographic hash object.
 Every hash object exists in a particular cryptographic context (Crypt::Context)
*******************************************************************************/
class Hash {
   public:
      // Constructor.
      // Creates an empty hash object.
      //
      Hash(const Context &context, int algorithm = CALG_SHA) :
         _context(context)
      {
         _create(algorithm) ;
      }

      // Constructor.
      // Creates a hash object initially updated with some data
      //
      Hash(const Context &context, const char *data, int algorithm = CALG_SHA) :
         _context(context)
      {
         _create(algorithm) ;
         hash(data, strlen(data)) ;
      }

      Hash(const Context &context, const void *data, size_t length, int algorithm = CALG_SHA) :
         _context(context)
      {
         _create(algorithm) ;
         hash(data, length) ;
      }

      // hash()   -  update the hash with data.
      // This function can be called for a particular hash object arbitrary times
      // until the hash object first time requested of its value (directly  can be u
      // Parameters:
      //    data     -  a pointer to data to update the hash with
      //    length   -  data length
      //
      Hash &hash(const void *data, size_t length)
      {
         Handle::check_result(
            CryptHashData(*this, (const unsigned char *)data, length, 0)) ;
         return *this ;
      }

      // hash()   -  update the hash with null-terminated string.
      // Parameters:
      //    str   -  null-terminated string (final null is not included in the hash)
      //
      Hash &hash(const char *str) { return hash(str, strlen(str)) ; }

      // sign()   -  sign the hash using current context's signature key
      // Returns:
      //    buffer with hash's signature
      //
      basic_buffer sign()
      {
         DWORD size = 0 ;
         Handle::check_result(CryptSignHash(*this, AT_SIGNATURE, 0, 0, 0, &size)) ;
         basic_buffer result (size) ;
         Handle::check_result(CryptSignHash(*this, AT_SIGNATURE, 0, 0, (BYTE *)result.get(), &size)) ;
         return result ;
      }

      basic_buffer data()
      {
         DWORD size = 0 ;
         Handle::check_result(CryptGetHashParam(*this, HP_HASHVAL, NULL, &size, 0)) ;
         basic_buffer result (size) ;
         Handle::check_result(CryptGetHashParam(*this,
                                                HP_HASHVAL,
                                                (unsigned char *)result.get(),
                                                &size,
                                                0)) ;
         return result ;
      }

      const Context &context() const { return _context ; }

      HCRYPTHASH handle() const { return *_handle ; }
      operator HCRYPTHASH() const { return handle() ; }

   private:
      void _create(int algorithm)
      {
         HCRYPTHASH hash = 0 ;
         Handle::check_result(CryptCreateHash(context(), algorithm, 0, 0, &hash)) ;
         _handle = shared_intrusive_ptr<HashHandle> (new HashHandle(hash)) ;
      }

      Context _context ;
      shared_intrusive_ptr<HashHandle> _handle ;
} ;

/*******************************************************************************
                     class Crypt::Key
*******************************************************************************/
class Key {

   public:
      // Constructor.
      // Gets the default user keypair for the context.
      explicit Key(const Context &context, int keyspec = AT_SIGNATURE) :
         _context(context)
      {
         HCRYPTKEY key = 0 ;
         Handle::check_result(CryptGetUserKey(context, keyspec, &key)) ;
         _create(key) ;
      }

      // Constructor.
      // Derives a symmetric session key from the hash value
      //
      Key(const Hash &hash, int algorithm = CALG_RC4) :
         _context(hash.context())
      {
         HCRYPTKEY key = 0 ;
         Handle::check_result(CryptDeriveKey(context(), algorithm, hash, CRYPT_EXPORTABLE, &key)) ;
         _create(key) ;
      }

      // Constructor.
      // Generates new private/public keypair
      // Parameters:
      //    keyspec  -  what keypair to generate (AT_KEYEXCHANGE or AT_SIGNATURE)
      //    context  -  a cryptographic context to create the keypair in
      //    bits     -  the length of keys
      //
      Key(int keyspec, const Context &context, unsigned bits = 0) :
         _context(context)
      {
         HCRYPTKEY key = 0 ;
         Handle::check_result(CryptGenKey(context, keyspec, CRYPT_EXPORTABLE | ((bits & 0x07fff) << 16), &key)) ;
         _create(key) ;
      }

      // Constructor.
      // Imports previously exported into raw BLOB key
      // Parameters:
      //    context  -  a cryptographic context to import the key pair into
      //    data     -  the data address
      //    length   -  the data length
      //
      Key(const Context &context, const void *data, size_t length) :
         _context(context)
      {
         HCRYPTKEY key = 0 ;
         Handle::check_result(CryptImportKey(context, (const BYTE *)data, length, 0, CRYPT_EXPORTABLE, &key)) ;
         _create(key) ;
      }

      basic_buffer encrypt(const void *source, size_t length) const
      {
         return _encrypt(source, length) ;
      }
      basic_buffer encrypt(const void *source, size_t length, const Hash &hash) const
      {
         return _encrypt(source, length, hash) ;
      }

      void *encrypt_inplace(void *source, size_t length) const
      {
         _encrypt_inplace(source, length) ;
         return source ;
      }

      void *encrypt_inplace(void *source, size_t length, const Hash &hash) const
      {
         _encrypt_inplace(source, length, hash) ;
         return source ;
      }

      basic_buffer decrypt(const void *source, size_t length) const
      {
         return _decrypt(source, length) ;
      }

      basic_buffer decrypt(const void *source, size_t length, const Hash &hash) const
      {
         return _decrypt(source, length, hash) ;
      }

      void *decrypt_inplace(void *source, size_t length) const
      {
         _decrypt_inplace(source, length) ;
         return source ;
      }

      void *decrypt_inplace(void *source, size_t length, const Hash &hash) const
      {
         _decrypt_inplace(source, length, hash) ;
         return source ;
      }

      bool verify_signature(const Hash &hash, const void *signature, size_t length) const
      {
         return
            CryptVerifySignature(hash, (const BYTE *)signature, length, *this, 0, 0) != 0 ||
            (int)GetLastError() != NTE_BAD_SIGNATURE && Handle::check_result(false) ;
      }

      basic_buffer export_key(unsigned blobtype = PUBLICKEYBLOB) const
      {
         DWORD size = 0 ;
         Handle::check_result(CryptExportKey(*this, 0, blobtype, 0, 0, &size)) ;
         basic_buffer result (size) ;
         Handle::check_result(CryptExportKey(*this, 0, blobtype, 0, (BYTE *)result.get(), &size)) ;
         return result ;
      }

      const Context &context() const { return _context ; }

      HCRYPTKEY handle() const { return *_handle ; }
      operator HCRYPTKEY() const { return handle() ; }

   private:
      void _encrypt_inplace(void *source, size_t length, HCRYPTHASH hash = 0) const
      {
         DWORD reslen = length ;
         Handle::check_result(CryptEncrypt(*this, hash, TRUE, 0, (BYTE *)source, &reslen, reslen)) ;
      }

      void _decrypt_inplace(void *source, size_t length, HCRYPTHASH hash = 0) const
      {
         DWORD reslen = length ;
         Handle::check_result(CryptDecrypt(*this, hash, TRUE, 0, (BYTE *)source, &reslen)) ;
      }

      basic_buffer _encrypt(const void *source, size_t length, HCRYPTHASH hash = 0) const
      {
         basic_buffer result (source, length) ;
         _encrypt_inplace(result, length, hash) ;
         return result ;
      }

      basic_buffer _decrypt(const void *source, size_t length, HCRYPTHASH hash = 0) const
      {
         basic_buffer result (source, length) ;
         _decrypt_inplace(result, length, hash) ;
         return result ;
      }

      void _create(HCRYPTKEY key)
      {
         _handle = shared_intrusive_ptr<KeyHandle> (new KeyHandle(key)) ;
      }

      Context _context ;
      shared_intrusive_ptr<KeyHandle> _handle ;
} ;

} // end of namespace Crypt

} // end of namespace pcomn

#endif /* __PCOMN_CRYPT_H */
