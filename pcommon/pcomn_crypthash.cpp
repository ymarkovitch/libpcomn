/*-*- tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_crypthash.cpp
 COPYRIGHT    :   Yakov Markovitch, Maxim Dementiev, 2010-2011. All rights reserved.
                  Yakov Markovitch, 2012-2014. All rights reserved.
                  All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   C++ wrappers for OpenSSL MD5 and SHA1 hash calculation

 CREATION DATE:   30 Jul 2010
*******************************************************************************/
#include "pcomn_hash.h"
#include "pcomn_strnum.h"
#include "pcomn_buffer.h"
#include "pcomn_except.h"
#include "pcomn_mmap.h"
#include "pcomn_binascii.h"

#include <openssl/md5.h>
#include <openssl/sha.h>
#include <stdexcept>

namespace pcomn {
/*******************************************************************************
 Hash calculator
*******************************************************************************/
namespace {
template<typename V, typename C,
         int(*init)(C *), int(*update)(C *, const void *, size_t), int(*final)(unsigned char *, C *)>
struct HashCalc {
      typedef V hash_type ;
      typedef C context_type ;

      template<size_t n>
      static hash_type value(const detail::crypthash_state<n> &state)
      {
         PCOMN_STATIC_CHECK(sizeof state._statebuf >= sizeof(context_type)) ;

         hash_type result ;
         if (state.is_init())
         {
            context_type context ;
            memcpy(&context, state._statebuf, sizeof context) ;
            final(result.data(), &context) ;
         }
         return result ;
      }

      template<size_t n>
      static void append_data(detail::crypthash_state<n> &state, const void *buf, size_t size)
      {
         PCOMN_ENSURE_ARG(buf) ;
         ensure_init_state(state) ;
         update_state(state, buf, size) ;
      }

      template<size_t n>
      static void append_file(detail::crypthash_state<n> &state, FILE *file)
      {
         const size_t CHUNK_SIZE = 64*pcomn::KiB ;

         PCOMN_ENSURE_ARG(file) ;
         ensure_init_state(state) ;
         PBasicBuffer buf (CHUNK_SIZE) ;

         for (ssize_t rcount ; (rcount = fread(buf.get(), 1, CHUNK_SIZE, file)) > 0 ; )
            update_state(state, buf.get(), rcount) ;
      }

      template<size_t n>
      static void ensure_init_state(detail::crypthash_state<n> &state)
      {
         if (!state.is_init())
            init((context_type *)(state._statebuf + 0)) ;
         NOXCHECK(state.is_init()) ;
      }
      template<size_t n>
      static void update_state(detail::crypthash_state<n> &state, const void *buf, size_t size)
      {
         update((context_type *)(state._statebuf + 0), buf, size) ;
         state._size += size ;
      }
} ;

template<typename H>
static inline H &hash_append_file(H &h, const char *filename)
{
   const PMemMapping file(filename) ;
   return h.append_data(file.data(), file.size()) ;
}

template<typename F, typename V>
static inline V &calc_hash_mem(F calc, const void *buf, size_t size, V &result)
{
   !size || PCOMN_ENSURE_ARG(buf) ;
   calc((const unsigned char *)(buf ? buf : ""), size, result.data()) ;
   return result ;
}

template<typename F, typename V, typename D>
static inline V &calc_hash_file_(F calc, D fname_or_fd, size_t *size, RaiseError raise_error, V &result)
{
   size_t filesize ;
   if (!size)
      size = &filesize ;
   try {
      const PMemMapping file(fname_or_fd) ;
      filesize = file.size() ;
      const unsigned char * const buf =
         (const unsigned char *)(filesize ? file.cdata() : static_cast<const char *>("")) ;

      calc(buf, filesize, result.data()) ;
      *size = filesize ;
   }
   catch (const std::exception &)
   {
      if (raise_error)
         throw ;
      *size = 0 ;
   }
   return result ;
}

template<typename F, typename V>
static inline V &calc_hash_file(F calc, const char *filename, size_t *size, RaiseError raise_error, V &result)
{
   PCOMN_ENSURE_ARG(filename) ;
   return calc_hash_file_(calc, filename, size, raise_error, result) ;
}

template<typename F, typename V>
static inline V &calc_hash_file(F calc, int fd, size_t *size, RaiseError raise_error, V &result)
{
   if (fd < 0 && !raise_error)
   {
      if (size)
         *size = 0 ;
      return result ;
   }
   return calc_hash_file_(calc, fd, size, raise_error, result) ;
}
}

/*******************************************************************************
 binary128_t
*******************************************************************************/
std::string binary128_t::to_string() const { return b2a_hex(&_cdata, sizeof _cdata) ; }

char *binary128_t::to_strbuf(char *buf) const { return b2a_hex(&_cdata, sizeof _cdata, buf) ; }

/*******************************************************************************
 md5hash_t
*******************************************************************************/
md5hash_t md5hash(const void *buf, size_t size)
{
   md5hash_t result ;
   return calc_hash_mem(&MD5, buf, size, result) ;
}

md5hash_t md5hash_file(const char *filename, size_t *size, RaiseError raise_error)
{
   md5hash_t result ;
   return calc_hash_file(&MD5, filename, size, raise_error, result) ;
}

md5hash_t md5hash_file(int fd, size_t *size, RaiseError raise_error)
{
   md5hash_t result ;
   return calc_hash_file(&MD5, fd, size, raise_error, result) ;
}

/*******************************************************************************
 sha1hash_t
*******************************************************************************/
std::string sha1hash_pod_t::to_string() const { return b2a_hex(_cdata, sizeof _cdata) ; }

sha1hash_t sha1hash(const void *buf, size_t size)
{
   sha1hash_t result ;
   return calc_hash_mem(&SHA1, buf, size, result) ;
}

sha1hash_t sha1hash_file(const char *filename, size_t *size, RaiseError raise_error)
{
   sha1hash_t result ;
   return calc_hash_file(&SHA1, filename, size, raise_error, result) ;
}

sha1hash_t sha1hash_file(int fd, size_t *size, RaiseError raise_error)
{
   sha1hash_t result ;
   return calc_hash_file(&SHA1, fd, size, raise_error, result) ;
}

/*******************************************************************************
 MD5Hash
*******************************************************************************/
typedef HashCalc<md5hash_t, MD5_CTX, &MD5_Init, &MD5_Update, &MD5_Final> md5calc ;

md5hash_t MD5Hash::value() const { return md5calc::value(_state) ; }

MD5Hash &MD5Hash::append_data(const void *buf, size_t size)
{
   md5calc::append_data(_state, buf, size) ;
   return *this ;
}

MD5Hash &MD5Hash::append_file(FILE *file)
{
   md5calc::append_file(_state, file) ;
   return *this ;
}

MD5Hash &MD5Hash::append_file(const char *filename)
{
   return hash_append_file(*this, filename) ;
}

/*******************************************************************************
 SHA1Hash
*******************************************************************************/
typedef HashCalc<sha1hash_t, SHA_CTX, &SHA1_Init, &SHA1_Update, &SHA1_Final> sha1calc ;

sha1hash_t SHA1Hash::value() const { return sha1calc::value(_state) ; }

SHA1Hash &SHA1Hash::append_data(const void *buf, size_t size)
{
   sha1calc::append_data(_state, buf, size) ;
   return *this ;
}

SHA1Hash &SHA1Hash::append_file(FILE *file)
{
   sha1calc::append_file(_state, file) ;
   return *this ;
}

SHA1Hash &SHA1Hash::append_file(const char *filename)
{
   return hash_append_file(*this, filename) ;
}

} // namespace pcomn
