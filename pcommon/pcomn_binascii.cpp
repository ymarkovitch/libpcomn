/*-*- tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_binascii.cpp
 COPYRIGHT    :   Yakov Markovitch, 2001-2017. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Binary-to-ASCII and ASCII-to-binary encoding routines

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   25 Dec 2001
*******************************************************************************/
#include <pcomn_binascii.h>
#include <pcomn_utils.h>
#include <pcommon.h>

#include <algorithm>

#include <string.h>
#include <stdlib.h>

/*******************************************************************************
 Base64 encoding/decoding routines
 Mostly inherited from Jack Jansen's code for the Python language
 Please note that Base64 encoding is essentially "liitle-endian" (though
 encoding/decoding routines is, of course, endianness-neutral)
*******************************************************************************/

const char base64_a2b_table[] =
{
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,62, -1,-1,-1,63,
    52,53,54,55, 56,57,58,59, 60,61,-1,-1, -1, (char)BASE64_PAD,-1,-1,
    -1, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
    15,16,17,18, 19,20,21,22, 23,24,25,-1, -1,-1,-1,-1,
    -1,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
    41,42,43,44, 45,46,47,48, 49,50,51,-1, -1,-1,-1,-1
} ;


static const char base64_b2a_table[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

size_t a2b_base64(const char *ascii_data, size_t *ascii_len_ptr, void *buf, size_t buf_len)
{
   size_t ascii_len = ascii_len_ptr ? *ascii_len_ptr : 0;
   if (!buf || !ascii_len)
      return a2b_bufsize_base64(ascii_len) ;

   unsigned char *bin_data = reinterpret_cast<unsigned char *>(buf) ;

   const char * const ascii_beg = ascii_data ;
   const char * ascii_full_parsed = ascii_data ;
   size_t bin_len = 0 ;
   int quad_pos = 0 ;
   int leftbits = 0 ;
   unsigned leftchar = 0 ;

   for(unsigned char prev_ch = 0 ; ascii_len ; --ascii_len, ++ascii_data)
   {
      unsigned char this_ch = *(const unsigned char *)ascii_data ;

      // Ignore illegal characters
      if (this_ch > 0x7f || (this_ch = base64_a2b_table[this_ch]) == (unsigned char)-1) {
         if (!quad_pos)
            ascii_full_parsed = ascii_data + 1;
         continue ;
      }

      // Check for pad sequences and ignore the invalid ones.
      if (this_ch == BASE64_PAD)
         if (quad_pos < 2 || quad_pos == 2 && prev_ch != BASE64_PAD) {
            prev_ch = BASE64_PAD ;
            continue ;
         }
         else {
            // A pad sequence means no more input.
            // We've already interpreted the data from the quad at this point.
            *ascii_len_ptr = ascii_data + 1 - ascii_beg ;
            return bin_len ;
         }

      // Shift it in on the low end
      ++quad_pos &= 0x03 ;
      (leftchar <<= 6) |= this_ch ;
      leftbits += 6 ;

      // Is there a byte ready for output?
      if (leftbits >= 8)
      {
         // OK, one more full byte is here
         leftbits -= 8 ;
         *bin_data++ = (leftchar >> leftbits) & 0xff ;
         ++bin_len ;
         leftchar &= ((1 << leftbits) - 1) ;
         // only fully decoded
         if (quad_pos == 0)
            ascii_full_parsed = ascii_data + 1;
         if (bin_len == buf_len)
            break ;
      }
      prev_ch = this_ch ;
   }

   *ascii_len_ptr = ascii_full_parsed - ascii_beg ;
   return bin_len ;
}

size_t b2a_base64(const void *source, size_t source_len,
                  char *ascii_data, size_t ascii_len)
{
   unsigned char *bin_data = (unsigned char *)source ;
   int leftbits = 0 ;
   unsigned char this_ch;
   unsigned int leftchar = 0;

   if (!ascii_len)
      return 0 ;
   ascii_len = ((ascii_len - 1) / 4) * 4 ;

   for( ; ascii_len && source_len > 0 ; --source_len, ++bin_data)
   {
      // Shift the data into the buffer
      (leftchar <<= 8) |= *bin_data ;
      leftbits += 8 ;

      /* See if there are 6-bit groups ready */
      do
      {
         this_ch = (leftchar >> (leftbits -= 6)) & 0x3f ;
         *ascii_data++ = base64_b2a_table[this_ch] ;
         --ascii_len ;
      }
      while (leftbits >= 6) ;
   }
   switch (leftbits)
   {
      case 2:
         *ascii_data++ = base64_b2a_table[(leftchar & 3) << 4] ;
         *ascii_data++ = BASE64_PAD_CHAR ;
         *ascii_data++ = BASE64_PAD_CHAR ;
         break ;

      case 4:
         *ascii_data++ = base64_b2a_table[(leftchar&0xf) << 2] ;
         *ascii_data++ = BASE64_PAD_CHAR ;
         break ;
   }
   *ascii_data = 0 ;
   return source_len ;
}

size_t a2b_base64(pcomn::shared_buffer &buffer, const char *ascii_data, size_t *ascii_len_ptr)
{
   if (!ascii_len_ptr)
      return 0 ;
   size_t buflen = a2b_bufsize_base64(*ascii_len_ptr) ;
   if (buflen)
   {
      const size_t initsize = buffer.size() ;
      buflen = a2b_base64(ascii_data, ascii_len_ptr, pcomn::padd(buffer.resize(initsize + buflen), initsize), buflen) ;
      buffer.resize(initsize + buflen) ;
   }
   return buflen ;
}

std::ostream &b2a_base64(std::ostream &os,
                         const void *data,
                         size_t datasize,
                         unsigned line_length)
{
   if (!line_length)
      line_length = 80 ;
   pcomn::auto_buffer<256> outbuf (line_length) ;

   for (size_t remains = datasize ; remains && os ; os << outbuf.get() << std::endl)
      remains = b2a_base64(pcomn::padd(data, (datasize - remains)),
                           remains, outbuf.get(), line_length) ;
   return os ;
}

static const char nonprintable[] = "\x01\x02\x03\x04\x05\x06\x07\x08\x0A\x0B\x0C\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\\\x7F" ;

static const char * const nonprintable_symbol[] =
{
   "\\x1", "\\x2", "\\x3", "\\x4", "\\x5", "\\x6", "\\a", "\\b", "\\n",
   "\\xB", "\\f", "\\xE", "\\xF", "\\x10", "\\x11", "\\x12", "\\x13", "\\x14", "\\x15", "\\x16",
   "\\x17", "\\x18", "\\x19", "\\\\", "\\x7F", "\\0"
} ;

std::ostream &b2a_cstring(std::ostream &os, const void *data, size_t size)
{
   const char *cdata = static_cast<const char *>(data) ;
   const char * const cend = cdata + size ;

   while (cdata != cend)
   {
      const char *end = std::find_first_of(cdata, cend,
                                           nonprintable + 0, nonprintable + sizeof nonprintable) ;
      os.write(cdata, end - cdata) ;

      for (cdata = end ; cdata != cend && (end = strchr(nonprintable, *cdata)) != NULL ; ++cdata)
         os << nonprintable_symbol[end - nonprintable] ;
   }
   return os ;
}

std::string b2a_cstring(const void *data, size_t size)
{
   !size || PCOMN_ENSURE_ARG(data) ;

   std::string result ;
   result.reserve(size) ;

   const char *cdata = static_cast<const char *>(data) ;
   const char * const cend = cdata + size ;
   while(cdata != cend)
   {
      const char *end = std::find_first_of(cdata, cend,
                                           nonprintable + 0, nonprintable + sizeof nonprintable) ;
      result.append(cdata, end - cdata) ;

      for (cdata = end ; cdata != cend && (end = strchr(nonprintable, *cdata)) != NULL ; ++cdata)
         result.append(nonprintable_symbol[end - nonprintable]) ;
   }
   return result ;
}

char *b2a_hex(const void *data, size_t size, char *result)
{
   if (!size)
      return result ;

   PCOMN_ENSURE_ARG(data) ;
   PCOMN_ENSURE_ARG(result) ;

   static const char hdigits[16] =
      { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' } ;

   char *digit = result ;
   for (const unsigned char *d = (const unsigned char *)data, *e = d + size ; d != e ; ++d, digit += 2)
   {
      digit[0] = hdigits[(size_t)*d >> 4] ;
      digit[1] = hdigits[*d & 0x0f] ;
   }
   return digit ;
}

char *b2a_hexz(const void *data, size_t size, char *result)
{
   PCOMN_ENSURE_ARG(result) ;
   *b2a_hex(data, size, result) = 0 ;
   return result ;
}
