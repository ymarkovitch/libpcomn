/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_BINASCII_H
#define __PCOMN_BINASCII_H
/*******************************************************************************
 FILE         :   pcomn_binascii.h
 COPYRIGHT    :   Yakov Markovitch, 2001-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Binary-to-ASCII and ASCII-to-binary encoding routines

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   25 Dec 2001
*******************************************************************************/
#include <pcomn_platform.h>

#ifdef __cplusplus
extern "C" {
#endif

const unsigned char BASE64_PAD = (unsigned char)-2 ;
const char BASE64_PAD_CHAR = '=' ;

// Unsupported values filled with (uchar)-1
// For PAD return BASE64_PAD
extern const char base64_a2b_table[256] ;

static inline
const char* skip_invalid_base64(const char *ascii, size_t len)
{
   for(const char *end = ascii + len ; ascii != end ; ++ascii) {
      unsigned char ch = *(unsigned char*)ascii ;
      if (ch <= 0x7f && base64_a2b_table[ch] != (char)-1)
         break;
   }
   return ascii ;
}

/* a2b_base64  -  convert a base64-encoded string into a binary buffer
 * Parameters:
 *    ascii_data    -  a base64-encoded source string
 *    ascii_len_ptr -  source string length pointer, returning really processed
 *    buf           -  resulting binary buffer
 *    buf_len       -  resulting buffer len
 */
_PCOMNEXP size_t a2b_base64(const char *ascii_data, size_t *ascii_len_ptr,
                            void *buf, size_t buf_len) ;

_PCOMNEXP size_t b2a_base64(const void *source, size_t srclen,
                            char *ascii_data, size_t ascii_len) ;

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

#include <pcomn_buffer.h>

/// Get the length of BASE64 string from data length
inline size_t b2a_strlen_base64(size_t data_size)
{
   return ((data_size+2)/3)*4 ;
}

/// Get the size of buffer large enough to hold data from BASE64 string of given length
inline size_t a2b_bufsize_base64(size_t ascii_len)
{
   return ((ascii_len+3)/4)*3 ;
}

/// suppose that @buf size >= a2b_bufsize_base64(ascii_len)
inline size_t a2b_base64(const char *ascii_data, size_t ascii_len, void *buf)
{
   return a2b_base64(ascii_data, &ascii_len, buf, a2b_bufsize_base64(ascii_len)) ;
}

_PCOMNEXP size_t a2b_base64(pcomn::basic_buffer &buffer,
                            const char *ascii_data,
                            size_t *ascii_len_ptr) ;

inline size_t a2b_base64(pcomn::basic_buffer &buffer, const char *ascii_data, size_t ascii_len)
{
   return a2b_base64(buffer, ascii_data, &ascii_len) ;
}

inline size_t a2b_base64(pcomn::basic_buffer &buffer, const char *ascii_data)
{
   return a2b_base64(buffer, ascii_data, strlen(ascii_data)) ;
}

inline size_t a2b_base64(const char *ascii_data, void *buf)
{
   return a2b_base64(ascii_data, strlen(ascii_data), buf) ;
}

_PCOMNEXP std::ostream &b2a_base64(std::ostream &os,
                                   const void *data,
                                   size_t datasize,
                                   unsigned line_length = 80) ;

_PCOMNEXP std::ostream &b2a_cstring(std::ostream &os, const void *data, size_t size) ;

inline std::ostream &b2a_cstring(std::ostream &os, const char *data)
{
   return b2a_cstring(os, data, strlen(data)) ;
}

_PCOMNEXP std::string b2a_cstring(const void *data, size_t size) ;

inline std::string b2a_cstring(const char *data)
{
   return b2a_cstring(data, strlen(data)) ;
}

_PCOMNEXP char *b2a_hex(const void *data, size_t size, char *result) ;

inline std::string b2a_hex(const void *data, size_t size)
{
   if (!size)
      return std::string() ;
   std::string result (2*size, 'A') ;
   b2a_hex(data, size, &*result.begin()) ;
   return result ;
}

_PCOMNEXP char *b2a_hexz(const void *data, size_t size, char *result) ;

#endif

#endif /* __PCOMN_BINASCII_H */
