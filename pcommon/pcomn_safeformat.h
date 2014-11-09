/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
//
// FILE         :   pcomn_safeformat.h
// VERSION INFO :   $URL: https://swflab.ashmanov.com/svn/webfilter/trunk/share/pcommon/pcomn_safeformat.h $
//                  $Revision: 2553 $
//                  $Author: markovitch $
//                  $LastChangedDate: 2013-12-09 21:50:35 +0400 (Mon, 09 Dec 2013) $
////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2005 by Andrei Alexandrescu
// Copyright (c) 2006 Peter Kümmel
// Copyright (c) 2006-2010 Yakov Markovitch
// Permission to use, copy, modify, distribute, and sell this software for any
//     purpose is hereby granted without fee, provided that the above copyright
//     notice appear in all copies and that both that copyright notice and this
//     permission notice appear in supporting documentation.
// The author makes no representations about the suitability of this software
//     for any purpose. It is provided "as is" without express or implied
//     warranty.
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// This file contains definitions for SafePrintf. SafeScanf coming soon (the
//   design is similar).
// See Alexandrescu, Andrei: Type-safe Formatting, C/C++ Users Journal, Aug 2005
////////////////////////////////////////////////////////////////////////////////
// Please note that this is NOT the original SafePrintf definition as it present
// in Loki, but instead the version modified by Yakov Markovitch
// (ymarkovitch@gmail.com)
////////////////////////////////////////////////////////////////////////////////
#ifndef __PCOMN_SAFEFORMAT_H
#define __PCOMN_SAFEFORMAT_H

#include <pcommon.h>
#include <pcomn_assert.h>
#include <pcomn_except.h>
#include <pcomn_iodevice.h>

#include <cstdio>
#include <string>
#include <stdexcept>
#include <utility>
#include <ctype.h>

#include <pcomn_tuple.h>
#include <functional>
#include <type_traits>

namespace pcomn {

// Namespace fmt is for placing everything format-related
namespace fmt {

// long is 32 bit on 64-bit Windows!
#ifdef PCOMN_PL_WINDOWS
typedef intptr_t  safeformat_long_t ;
typedef uintptr_t safeformat_ulong_t ;
#else
typedef signed long   safeformat_long_t ;
typedef unsigned long safeformat_ulong_t ;
#endif

/******************************************************************************/
/** Safe string formatter: holds the formatting state, and implements operator() to
 format data.
*******************************************************************************/
template<typename Device, typename Char>
class State {
      PCOMN_NONASSIGNABLE(State) ;
   public:
      typedef Char char_type ;

      State(Device dev, const char_type *format) :
         _device(dev),
         _format(format),
         _width(0),
         _prec(0),
         _flags(0),
         _result(0)
      {
         Advance() ;
      }

#define PCOMN_SAFEFORMAT_STATE_FORWARD(type)                      \
      State& operator()(type par)                                 \
      {                                                           \
         return (*this)(static_cast<safeformat_ulong_t>(par)) ;   \
      }

      PCOMN_SAFEFORMAT_STATE_FORWARD(bool) ;
      PCOMN_SAFEFORMAT_STATE_FORWARD(char) ;
      PCOMN_SAFEFORMAT_STATE_FORWARD(signed char) ;
      PCOMN_SAFEFORMAT_STATE_FORWARD(unsigned char) ;
      PCOMN_SAFEFORMAT_STATE_FORWARD(signed short) ;
      PCOMN_SAFEFORMAT_STATE_FORWARD(unsigned short) ;
      PCOMN_SAFEFORMAT_STATE_FORWARD(signed int) ;
#ifndef PCOMN_PL_WIN32
      PCOMN_SAFEFORMAT_STATE_FORWARD(unsigned int) ;
#else
      PCOMN_SAFEFORMAT_STATE_FORWARD(unsigned long) ;
#endif
      PCOMN_SAFEFORMAT_STATE_FORWARD(signed long) ;

#undef PCOMN_SAFEFORMAT_STATE_FORWARD

#define PCOMN_SAFEFORMAT_ENSURE() { if ((_result |= -(ptrdiff_t)!*_format) < 0) { return *this ; } } // don't even bother
#define PCOMN_SAFEFORMAT_FAIL { _result = -1 ; return *this ; }

      // Print (or gobble in case of the "*" specifier) an int
      State &operator()(safeformat_ulong_t i)
      {
         PCOMN_SAFEFORMAT_ENSURE() ;
         // % [flags] [width] [.prec] [modifier] type_char
         // Fetch the flags
         ReadFlags();
         if (*_format == '*') {
            // read the width and get out
            SetWidth(static_cast<size_t>(i));
            ++_format;
            return *this;
         }
         ReadWidth();
         // precision
         if (*_format == '.') {
            // deal with precision
            if (_format[1] == '*') {
               // read the precision and get out
               SetPrec(static_cast<size_t>(i));
               _format += 2;
               return *this;
            }
            ReadPrecision();
         }
         ReadModifiers();
         // input size modifier
         if (ForceShort())
            switch(*_format)
            {
              case 'x': case 'X': case 'u': case 'o':
               // short int
               i = static_cast<safeformat_ulong_t>(static_cast<unsigned short>(i)) ;
            }
         return FormatWithCurrentFlags(i) ;
      }

      State& operator()(const void* n) { return PrintUsing_snprintf(n, "p") ; }

      State& operator()(double n) { return PrintUsing_snprintf(n,"eEfgG") ; }

      State& operator()(long double n) { return PrintUsing_snprintf(n,"eEfgG") ; }

      // Store the number of characters printed so far
      State& operator()(int *pi) { return StoreCountHelper(pi) ; }

      // Store the number of characters printed so far
      State& operator()(short *pi) { return StoreCountHelper(pi) ; }

      // Store the number of characters printed so far
      State& operator()(long *pi) { return StoreCountHelper(pi) ; }

      // Print any string that has pcomn::string_traits<> defined
      template<class S>
      typename enable_if_strchar<S, char_type, State &>::type
      operator()(const S &str) { return PrintString(str) ; }

      // read the result
      operator int() const { return static_cast<int>(_result) ; }

   private:
      // State
      Device            _device ;
      const char_type * _format ;
      size_t            _width ;
      size_t            _prec ;
      unsigned long     _flags ;
      ptrdiff_t         _result ;

      template<typename T>
      State &StoreCountHelper(T * const pi)
      {
         PCOMN_SAFEFORMAT_ENSURE() ;
         ReadLeaders() ;
         switch (const char fmt = *_format)
         {
            case 'p':
               return FormatWithCurrentFlags(reinterpret_cast<safeformat_ulong_t>(pi)) ;
            case 'n':
               NOXCHECK(pi != 0) ;
               *pi = _result ;
               Next() ;
               return *this ;
         }
         PCOMN_SAFEFORMAT_FAIL ;
      }

      // This is big and should be made out-of-line
      State &FormatWithCurrentFlags(const safeformat_ulong_t) ;

      template<typename S>
      disable_if_t<std::is_pointer<S>::value, State &> PrintString(const S &s)
      {
         return PrintString(str::cstr(s), str::len(s)) ;
      }

      State &PrintString(const char *s) ;
      State &PrintString(const char *s, const size_t slen) ;
      State &PrintString(const char *s, const size_t slen, char fmt) ;

      void Write(const char_type *b, const char_type *e)
      {
         if (_result < 0) return ;
         const ssize_t x = e - b ;
         if (io::writer<Device>::write(_device, b, e) == x)
            _result += x ;
         else
            _result = -1 ;
      }

      template<typename Value>
      State &PrintUsing_snprintf(Value n, const char *check_fmt_char) ;

      void Fill(const char_type c, size_t n)
      {
         while(n--) Write(&c, &c + 1) ;
      }

      static char_type *RenderWithoutSign(safeformat_ulong_t n, char* bufLast,
                                          unsigned int base, bool uppercase)
      {
         const char_type hex1st = uppercase ? 'A' : 'a';
         for (;;)
         {
            const safeformat_ulong_t next = n / base;
            char_type c = static_cast<char_type>(n - next * base) ;
            c += (c <= 9) ? '0' : hex1st - 10;
            *bufLast = c;
            n = next;
            if (n == 0) break;
            --bufLast;
         }
         return bufLast;
      }

      static char_type *RenderWithoutSign(safeformat_long_t n, char* bufLast,
                                          unsigned int base, bool uppercase)
      {
         if (n != LONG_MIN)
            return RenderWithoutSign(static_cast<safeformat_ulong_t>(n < 0 ? -n : n),
                                     bufLast, base, uppercase);
         // annoying corner case
         char* save = bufLast;
         ++n;
         bufLast = RenderWithoutSign(static_cast<safeformat_ulong_t>(n),
                                     bufLast, base, uppercase);
         --(*save);
         return bufLast;
      }

      void Next()
      {
         ++_format ;
         Advance() ;
      }

      void Advance() ;

      void ReadFlags()
      {
         for (;; ++_format) {
            switch (*_format) {
               case '-': SetLeftJustify(); break;
               case '+': SetShowSignAlways(); break;
               case ' ': SetBlank(); break;
               case '#': SetAlternateForm(); break;
               case '0': SetFillZeros(); break;
               default: return;
            }
         }
      }

      void ParseDecimalSizeT(size_t& dest)
      {
         if (!isdigit(*_format)) return;
         size_t r = 0;
         do {
            // TODO: inefficient - rewrite
            r *= 10;
            r += *_format - '0';
            ++_format;
         } while (isdigit(*_format));
         dest = r;
      }

      void ReadWidth() { ParseDecimalSizeT(_width); }

      void ReadPrecision()
      {
         NOXCHECK(*_format == '.');
         ++_format;
         ParseDecimalSizeT(_prec);
      }

      void ReadModifiers()
      {
         switch (*_format)
         {
            case 'h': SetForceShort(); ++_format; break;
            case 'l': ++_format; break;
               // more (C99 and platform-specific modifiers) to come
         }
      }

      void ReadLeaders()
      {
         ReadFlags();
         ReadWidth();
         if (*_format == '.') ReadPrecision();
         ReadModifiers();
      }

      // Format flags
      static const unsigned leftJustify    = 0x0001 ;
      static const unsigned showSignAlways = 0x0002 ;
      static const unsigned blank          = 0x0004 ;
      static const unsigned alternateForm  = 0x0008 ;
      static const unsigned fillZeros      = 0x0010 ;
      static const unsigned forceShort     = 0x0020 ;

      bool LeftJustify() const { return !!(_flags & leftJustify) ; }
      void SetLeftJustify() { _flags |= leftJustify ; }

      bool ShowSignAlways() const { return !!(_flags & showSignAlways) ; }
      void SetShowSignAlways() { _flags |= showSignAlways; }

      bool Blank() const { return !!(_flags & blank) ; }
      void SetBlank() { _flags |= blank ; }

      bool AlternateForm() const { return !!(_flags & alternateForm); }
      void SetAlternateForm() { _flags |= alternateForm ; }

      bool FillZeros() const { return !!(_flags & fillZeros) ; }
      void SetFillZeros() { _flags |= fillZeros ; }

      bool ForceShort() const { return !!(_flags & forceShort) ; }
      void SetForceShort() { _flags |= forceShort ; }

      void SetWidth(size_t w) { _width  = w ; }
      void SetPrec(size_t p) { _prec = p ; }
      void ResetFillZeros() { _flags &= ~fillZeros ; }

      void ResetAll()
      {
         NOXCHECK(_result != EOF);
         _width = 0;
         _prec = size_t(-1);
         _flags = 0;
      }
} ;

/*******************************************************************************
 State<Device, Char>
*******************************************************************************/
template<typename Device, typename Char>
void State<Device, Char>::Advance()
{
   ResetAll();
   const char_type* begin = _format;
   for (;;) {
      if (*_format == '%') {
         if (_format[1] != '%') { // It's a format specifier
            Write(begin, _format);
            ++_format;
            break;
         }
         // It's a "%%"
         Write(begin, ++_format);
         begin = ++_format;
         continue;
      }
      if (*_format == 0) {
         Write(begin, _format);
         break;
      }
      ++_format;
   }
}

template<typename Device, typename Char>
template<typename Value>
State<Device, Char> &State<Device, Char>::PrintUsing_snprintf(Value n, const char* check_fmt_char)
{
   PCOMN_SAFEFORMAT_ENSURE() ;
   const char_type *const fmt = _format - 1;
   NOXCHECK(*fmt == '%');
   // enforce format string validity
   ReadLeaders();
   // enforce format spec
   if (!strchr(check_fmt_char, *_format))
      PCOMN_SAFEFORMAT_FAIL ;
   // format char validated, copy it to a temp and use legacy sprintf
   ++_format;
   char_type fmtBuf[128], resultBuf[1024];
   if (_format  >= fmt + sizeof(fmtBuf) / sizeof(char_type))
      PCOMN_SAFEFORMAT_FAIL ;
   memcpy(fmtBuf, fmt, (_format - fmt) * sizeof(char_type));
   fmtBuf[_format - fmt] = 0;

   const int stored =
      snprintf(resultBuf, sizeof(resultBuf) / sizeof(char_type), fmtBuf, n);

   if (stored < 0)
      PCOMN_SAFEFORMAT_FAIL ;
   Write(resultBuf, resultBuf + strlen(resultBuf));
   Advance(); // output stuff to the next format directive
   return *this ;
}

template<typename Device, typename Char>
State<Device, Char> &State<Device, Char>::FormatWithCurrentFlags(const safeformat_ulong_t i)
{
   // look at the format character
   Char formatChar = *_format;
   bool isSigned = formatChar == 'd' || formatChar == 'i';
   if (formatChar == 'p')
   {
      formatChar = 'x'; // pointers go to hex
      SetAlternateForm(); // printed with '0x' in front
      isSigned = true; // that's what gcc does
   }
   if (!strchr("cdiuoxX", formatChar))
      PCOMN_SAFEFORMAT_FAIL ;
   Char buf[
      sizeof(safeformat_ulong_t) * 3 // digits
      + 1 // sign or ' '
      + 2 // 0x or 0X
      + 1]; // terminating zero
   const Char *const bufEnd = buf + (sizeof(buf) / sizeof(Char));
   Char * bufLast = buf + (sizeof(buf) / sizeof(Char) - 1);
   Char signChar = 0;
   unsigned int base = 10;

   if (formatChar == 'c') {
      // Format only one character
      // The 'fill with zeros' flag is ignored
      ResetFillZeros();
      *bufLast = static_cast<char>(i);
   } else {
      // TODO: inefficient code, refactor
      const bool negative = isSigned && static_cast<safeformat_long_t>(i) < 0;
      if (formatChar == 'o') base = 8;
      else if (formatChar == 'x' || formatChar == 'X') base = 16;
      bufLast = isSigned
         ? RenderWithoutSign(static_cast<safeformat_long_t>(i), bufLast, base,
                             formatChar == 'X')
         : RenderWithoutSign(i, bufLast, base,
                             formatChar == 'X');
      // Add the sign
      if (isSigned) {
         negative ? signChar = '-'
            : ShowSignAlways() ? signChar = '+'
            : Blank() ? signChar = ' '
            : 0;
      }
   }
   // precision
   size_t
      countDigits = bufEnd - bufLast,
      countZeros = _prec != size_t(-1) && countDigits < _prec &&
      formatChar != 'c'
      ? _prec - countDigits
      : 0,
      countBase = base != 10 && AlternateForm() && i != 0
      ? (base == 16 ? 2 : countZeros > 0 ? 0 : 1)
      : 0,
      countSign = (signChar != 0),
      totalPrintable = countDigits + countZeros + countBase + countSign;
   size_t countPadLeft = 0, countPadRight = 0;
   if (_width > totalPrintable) {
      if (LeftJustify()) {
         countPadRight = _width - totalPrintable;
         countPadLeft = 0;
      } else {
         countPadLeft = _width - totalPrintable;
         countPadRight = 0;
      }
   }
   if (FillZeros() && _prec == size_t(-1)) {
      // pad with zeros and no precision - transfer padding to precision
      countZeros = countPadLeft;
      countPadLeft = 0;
   }
   // ok, all computed, ready to print to device
   Fill(' ', countPadLeft);
   if (signChar != 0) Write(&signChar, &signChar + 1);
   if (countBase > 0) Fill('0', 1);
   if (countBase == 2) Fill(formatChar, 1);
   Fill('0', countZeros);
   Write(bufLast, bufEnd);
   Fill(' ', countPadRight);
   // done, advance
   Next();
   return *this ;
}

template<typename Device, typename Char>
State<Device, Char> &State<Device, Char>::PrintString(const char *s)
{
   PCOMN_SAFEFORMAT_ENSURE() ;
   ReadLeaders() ;
   const char fmt = *_format ;
   if (fmt == 'p')
      return FormatWithCurrentFlags(reinterpret_cast<safeformat_ulong_t>(s)) ;

   if (!s)
      PCOMN_SAFEFORMAT_FAIL ;

   return PrintString(s, strlen(s), fmt) ;
}

template<typename Device, typename Char>
State<Device, Char> &State<Device, Char>::PrintString(const char *s, const size_t slen)
{
   PCOMN_SAFEFORMAT_ENSURE() ;
   ReadLeaders() ;
   return PrintString(s, slen, *_format) ;
}

template<typename Device, typename Char>
State<Device, Char> &State<Device, Char>::PrintString(const char *s, const size_t slen, char fmt)
{
   if (fmt != 's')
      PCOMN_SAFEFORMAT_FAIL ;

   const size_t len = std::min(slen, _prec) ;
   const char * const s_end = s + len ;
   if (_width <= len)
      Write(s, s_end) ;
   else if (LeftJustify())
   {
      Write(s, s_end) ;
      Fill(' ', _width - len) ;
   }
   else
   {
      Fill(' ', _width - len);
      Write(s, s_end) ;
   }
   Next();
   return *this;
}

// Analyze the format and give results as a formatting character sequence
template<typename Char, typename OutputIterator>
static int analyze(const Char *format, OutputIterator result) ;

#undef PCOMN_SAFEFORMAT_ENSURE
#undef PCOMN_SAFEFORMAT_FAIL

} // end of namespaces pcomn::fmt

/*******************************************************************************
 Global functions
 Please note, they all are in the namespace pcomn, _not_ in the pcomn::fmt
*******************************************************************************/
template<typename Dev, typename S>
inline typename enable_if_string<S, fmt::State<Dev &, typename string_traits<S>::char_type> >::type
xprintf(Dev &device, const S &format)
{
   return fmt::State<Dev &, typename string_traits<S>::char_type>
      (device, pcomn::str::cstr(format)) ;
}

template<typename Devp, typename S>
inline typename enable_if_string<S, fmt::State<Devp *, typename string_traits<S>::char_type> >::type
xprintf(Devp *device, const S &format)
{
   return fmt::State<Devp *, typename string_traits<S>::char_type>
      (device, pcomn::str::cstr(format)) ;
}

template<typename S>
inline typename enable_if_strchar<S, char, fmt::State<std::FILE *, char> >::type
xprintf(const S &format)
{
   return fmt::State<std::FILE*, char>(stdout, format);
}

template<typename S>
inline fmt::State<std::basic_string<typename string_traits<S>::char_type> &,
                  typename string_traits<S>::char_type>
xsprintf(std::basic_string<typename string_traits<S>::char_type> &buffer, const S &format)
{
   typedef typename string_traits<S>::char_type char_type ;
   return
      fmt::State<std::basic_string<char_type>&, char_type>(buffer, ::pcomn::str::cstr(format)) ;
}

template<typename S>
inline fmt::State<std::pair<typename string_traits<S>::char_type *, size_t>,
                  typename string_traits<S>::char_type>
xsprintf(typename string_traits<S>::char_type *buf, size_t count, const S &format)
{
   typedef typename string_traits<S>::char_type char_type ;
   typedef std::pair<char_type *, size_t>       device_type ;
   return fmt::State<device_type, char_type>(device_type(buf, count), ::pcomn::str::cstr(format)) ;
}

template <typename Char, typename S, size_t n>
inline typename std::enable_if<std::is_same<Char, typename string_traits<S>::char_type>::value,
                               fmt::State<std::pair<Char *, size_t>, Char> >::type
xsprintf(Char (&buf)[n], const S &format)
{
   typedef std::pair<Char *, size_t> device_type ;
   return fmt::State<device_type, Char>(device_type(buf, n), ::pcomn::str::cstr(format)) ;
}

#define PCOMN_TPARGS(...) (::pcomn::const_tie(__VA_ARGS__))

template<class ResultStr, typename FmtStr, class Tuple>
inline int sprintf_tp(ResultStr &buffer, const FmtStr &format, const Tuple &params)
{
   typedef fmt::State<ResultStr &, typename string_traits<ResultStr>::char_type> state_type ;

   state_type state(buffer, str::cstr(format)) ;
   tuple_for_each<const Tuple>::apply_lvalue(params, state) ;
   return state ;
}

// strprintf_tp()
template<class ResultStr, typename FmtStr, class Tuple>
ResultStr strprintf_tp(const FmtStr &format, const Tuple &params)
{
   ResultStr buffer ;
   sprintf_tp(buffer, format, params) ;
   return buffer ;
}

} // end of namespace pcomn

#endif /* __PCOMN_SAFEFORMAT_H */
