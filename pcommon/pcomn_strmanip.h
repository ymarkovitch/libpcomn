/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_STRMANIP_H
#define __PCOMN_STRMANIP_H
/*******************************************************************************
 FILE         :   pcomn_strmanip.h
 COPYRIGHT    :   Yakov Markovitch, 2007-2018. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Advanced string functions, working though string traits thus
                  providing string-independence.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   7 Sep 2007
*******************************************************************************/
#include <pcomn_string.h>
#include <pcomn_iodevice.h>

#ifdef PCOMN_PL_WINDOWS
#include <windows.h>
#endif

#include <limits.h>

namespace pcomn {

/*******************************************************************************
 The family of narrow<->wide string converters.
 Please note that MSVC 7 RTL doesn't support the the restartable conversion
 function mbrtowc, so we have to make do with functions that require
 null-terminated strings, while converting general ranges.
*******************************************************************************/
#ifdef PCOMN_COMPILER_C99
template<typename Device>
const char *mbstowcdev(Device device, const char *begin, const char *end)
{
   if (begin == end)
      return begin ;

   static const size_t BUFSIZE = 1024 ;
   wchar_t outbuf[BUFSIZE] ;
   const wchar_t *outend = outbuf + BUFSIZE ;
   mbstate_t ps ;
   memset(&ps, 0, sizeof ps) ;
   long consumed ;
   do {
      wchar_t *outp = outbuf ;
      for (consumed = 0 ;
           outp != outend && (consumed = mbrtowc(outp, begin, end - begin, &ps)) > 0 ;
           ++outp)
         // Handle zero bytes in the initial sequence
         begin += consumed + !consumed ;
      io::writer<Device>::write(device, outbuf, outp) ;
   }
   while (begin != end && consumed >= 0) ;
   return begin ;
}

template<typename Device>
size_t wcstombdev(Device device, const wchar_t *begin, const wchar_t *end)
{
   static const size_t BUFSIZE = 1024 ;
   char outbuf[BUFSIZE*MB_LEN_MAX] ;
   mbstate_t ps ;
   memset(&ps, 0, sizeof ps) ;
   size_t produced_total = 0 ;
   while (begin != end)
   {
      size_t produced = 0 ;
      char *outp = outbuf ;
      while (begin != end
             && (produced += wcrtomb(outp, *begin++, &ps)) < sizeof outbuf - MB_LEN_MAX)
         outp = outbuf + produced ;
      produced_total += produced ;
      io::writer<Device>::write(device, outbuf + 0, outbuf + produced) ;
   }
   return produced_total ;
}
#elif defined(PCOMN_PL_WINDOWS)

template<typename Device>
const char *mbstowcdev(Device device, const char *begin, const char *end)
{
   if (begin == end)
      return begin ;

   static const size_t BUFSIZE = 1024 ;
   wchar_t outbuf[BUFSIZE] ;
   long consumed ;
   do {
      size_t produced ;
      size_t in_buf_size = std::min<size_t>(end - begin, BUFSIZE) ;
      // Some byte values can appear in both the leading and trailing
      // byte of a DBCS character. Thus, IsDBCSLeadByte can only
      // indicate a potential lead byte value.
      // so try twice to fine lead char
      int convert_attempt = 2 ;
      do
      {
         produced = MultiByteToWideChar(CP_THREAD_ACP, MB_ERR_INVALID_CHARS,
                                        begin, in_buf_size, outbuf, BUFSIZE) ;
         if (!produced && GetLastError()!=ERROR_NO_UNICODE_TRANSLATION)
            break ;
         if (!produced && convert_attempt)
            for(int i=MB_LEN_MAX;
                i-- && !IsDBCSLeadByteEx(CP_THREAD_ACP, begin[--in_buf_size]); ) ;
      } while (!produced && convert_attempt--) ;

      // if zero - some error ocurred
      if (!produced)
         return begin ;
      begin += in_buf_size ;
      io::writer<Device>::write(device, outbuf, outbuf + produced) ;
   }
   while (begin != end) ;
   return begin ;
}

template<typename Device>
size_t wcstombdev(Device device, const wchar_t *begin, const wchar_t *end)
{
   static const int BUFSIZE = 1024 ;
   char outbuf[BUFSIZE*MB_LEN_MAX] ;
   size_t produced_total = 0 ;
   while (begin < end)
   {
      size_t produced = WideCharToMultiByte(CP_THREAD_ACP, 0,
                                            begin, std::min<size_t>(end - begin, BUFSIZE),
                                            outbuf, BUFSIZE*MB_LEN_MAX, NULL, NULL) ;
      // if zero - some error ocurred
      if (!produced)
         return produced_total ;
      begin += BUFSIZE ;
      produced_total += produced ;
      io::writer<Device>::write(device, outbuf + 0, outbuf + produced) ;
   }
   return produced_total ;
}

#else

#error Unsupported platform (narrow<->wide string converters)

#endif

template<typename C>
std::basic_string<C> stdstr(const char *src) ;
template<typename C>
std::basic_string<C> stdstr(const wchar_t *src) ;

template<>
inline std::basic_string<char> stdstr(const char *src) { return std::string(src) ; }
template<>
inline std::basic_string<wchar_t> stdstr(const wchar_t *src) { return std::wstring(src) ; }
template<>
inline std::basic_string<char> stdstr(const wchar_t *src)
{
   std::string result ;
   wcstombdev<std::string &>(result, src, wcschr(src, 0)) ;
   return result ;
}
template<>
inline std::basic_string<wchar_t> stdstr(const char *src)
{
   std::wstring result ;
   mbstowcdev<std::wstring &>(result, src, strchr(src, 0)) ;
   return result ;
}

} // end of namespace pcomn

#endif /* __PCOMN_STRMANIP_H */
