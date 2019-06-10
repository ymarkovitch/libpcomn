/*-*- tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_uuid.cpp
 COPYRIGHT    :   Yakov Markovitch, 2014-2018

 DESCRIPTION  :   UUID and network MAC data types implementation

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   15 Sep 2014
*******************************************************************************/
#include "pcomn_uuid.h"
#include "pcomn_strnum.h"
#include "pcomn_binascii.h"

namespace pcomn {

/*******************************************************************************
 uuid
*******************************************************************************/
uuid::uuid(const strslice &str, RaiseError raise_error)
{
   if (!str)
   {
      _idata[1] = _idata[0] = 0 ;
      return ;
   }
   const auto cvt = [](char *begin, char *end, uint64_t &result)
      {
         end = std::remove_if(begin, end, [](char c){ return !isxdigit(c) ; }) ;
         *end = 0 ;
         return end - begin == 16 && (result = strtoull(begin, &end, 16), !*end) ;
      } ;

   char buf[slen() + 1] ;
   if (str.size() != slen()

       || strslicecpy(buf, str)[8] != '-'
       || buf[13] != '-' || buf[18] != '-' || buf[23] != '-'

       || !cvt(buf, buf + 18, _idata[0]) || !cvt(buf + 19, std::end(buf), _idata[1]))
   {
      PCOMN_THROW_IF(raise_error, invalid_str_repr, "Invalid UUID format " P_STRSLICEQF, P_STRSLICEV(str)) ;
      _idata[1] = _idata[0] = 0 ;
   }
   else
   {
      to_big_endian(_idata[0]) ;
      to_big_endian(_idata[1]) ;
   }
}

char *uuid::to_strbuf(char *buf) const
{
   char *p = buf ;
   const unsigned char *d = _cdata ;
   p = b2a_hex(d, 4, p)      ; *p++ = '-' ;
   p = b2a_hex(d += 4, 2, p) ; *p++ = '-' ;
   p = b2a_hex(d += 2, 2, p) ; *p++ = '-' ;
   p = b2a_hex(d += 2, 2, p) ; *p++ = '-' ;
   b2a_hexz(d += 2, 6, p) ;
   return buf ;
}

/*******************************************************************************
 MAC
*******************************************************************************/
MAC::MAC(const strslice &str, RaiseError raise_error)
{
   if (!str)
   {
      _idata = 0 ;
      return ;
   }
   const auto cvt = [](char *begin, char *end)
      {
         end = std::remove_if(begin, end, [](char c){ return !isxdigit(c) ; }) ;
         *end = 0 ;
         return end - begin == 12 ? strtoll(begin, nullptr, 16) : -1LL ;
      } ;

   char buf[slen() + 1] ;
   int64_t result ;
   if (str.size() != slen()
       || strslicecpy(buf, str)[2] != ':' || buf[5] != ':' || buf[8] != ':' || buf[11] != ':' || buf[14] != ':'
       || (result = cvt(std::begin(buf), std::end(buf))) < 0)
   {
      PCOMN_THROW_IF(raise_error, invalid_str_repr, "Invalid MAC format " P_STRSLICEQF, P_STRSLICEV(str)) ;
      _idata = 0 ;
   }
   else
      _idata = value_to_little_endian(result) ;
}

char *MAC::to_strbuf(char *buf) const
{
   union {
         const uint64_t vbuf ;
         const char cbuf[8] ;
   } data = { value_to_big_endian(_idata) } ;

   char *p = b2a_hex(data.cbuf + (sizeof data.cbuf - size()), size(), buf) ;
   char *end = p + 5 ;
   *end = 0 ;
   *--end = toupper(*--p) ;
   *--end = toupper(*--p) ;
   while (p != buf)
   {
      *--end = ':' ;
      *--end = toupper(*--p) ;
      *--end = toupper(*--p) ;
   }
   return buf ;
}

std::string MAC::to_string() const
{
   char buf[slen() + 1] ;
   return std::string(to_strbuf(buf)) ;
}

} // end of namespace pcomn
