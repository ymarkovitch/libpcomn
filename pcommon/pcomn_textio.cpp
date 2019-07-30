/*-*- tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_textio.cpp
 COPYRIGHT    :   Yakov Markovitch, 2007-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Universal newline text reader/writer

 PROGRAMMED BY:   Yakov Markovitch, Vadim Hohlov
 CREATION DATE:   19 Jul 2007
*******************************************************************************/
#include <pcomn_textio.h>

namespace pcomn {

/*******************************************************************************
 text_writer
*******************************************************************************/
size_t text_writer::count_inbytes(const char *begin, size_t outbytes_count) const
{
   if (eoltype() != eol_CRLF)
      return outbytes_count ;

   const char *end = begin ;
   for (ssize_t remain = outbytes_count ; remain > 0 ; --remain, ++end)
      if (*end == '\n')
         --remain ;
   return end - begin ;
}

size_t text_writer::write_translate(const char *begin, size_t count)
{
   char *out_end = (char *)memchr(begin, '\n', count) ;
   if (!out_end)
      out_end = const_cast<char *>(begin) + count ;
   ssize_t outbytes = std::max(write_raw(begin, out_end), (ssize_t)0) ;
   if (outbytes < out_end - begin)
   {
      _written_text += outbytes ;
      _written_binary += outbytes ;
      return outbytes ;
   }

   const size_t bufsize = 2048 ;
   char outbuf[bufsize] ;
   char * const endbuf = outbuf + (bufsize - 1) ;
   size_t inbytes = count ;
   ssize_t lastwritten ;
   for (const char *cp = out_end, *end = begin + count ; cp < end ; outbytes += lastwritten)
   {
      out_end = outbuf ;
      do {
         if (*cp != '\n')
            *out_end = *cp ;
         else
         {
            *out_end = _newline._text[0] ;
            if (_newline._text[1])
               *++out_end = _newline._text[1] ;
         }
      }
      while (++cp != end && ++out_end < endbuf) ;

      try {
         lastwritten = write_raw(outbuf, out_end) ;
      }
      catch (...)
      {
         _written_binary += outbytes ;
         _written_text += count_inbytes(begin, outbytes) ;
         throw ;
      }
      if (lastwritten < out_end - outbuf)
      {
         if (lastwritten > 0)
            outbytes += lastwritten ;
         inbytes = count_inbytes(begin, outbytes) ;
         break ;
      }
   }
   _written_text += inbytes ;
   _written_binary += outbytes ;
   return inbytes ;
}

/*******************************************************************************
 text_reader
*******************************************************************************/

} // end of namespace pcomn
