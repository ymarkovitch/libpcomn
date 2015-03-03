/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_STRNUM_H
#define __PCOMN_STRNUM_H
/*******************************************************************************
 FILE         :   pcomn_integer.h
 COPYRIGHT    :   Yakov Markovitch, 2006-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Numeric <-> string conversions.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   27 Nov 2006
*******************************************************************************/
/** @file
 Numeric <-> string conversions.
*******************************************************************************/
#include <pcomn_integer.h>
#include <pcomn_string.h>
#include <pcomn_macros.h>
#include <pcomn_except.h>
#include <pcomn_shortstr.h>

#include <functional>
#include <iterator>
#include <stdexcept>

#include <ctype.h>

/// Convert an integer/boolean @a number to a string in a temporary on-stack buffer
#define PCOMN_NUMTOSTR10(number) (pcomn::numtostr((number), std::array<char, 32>().data(), 32))
/// Convert an integer/boolean @a number to a string in a temporary on-stack buffer
/// according to the given @a radix
#define PCOMN_NUMTOSTR(number, radix) (pcomn::numtostr((number), std::array<char, 72>().data(), 72, (radix)))

namespace pcomn {

/******************************************************************************/
/** Exception that indicates unexpected character while parsing a string.
*******************************************************************************/
class _PCOMNEXP unexpected_char : public std::runtime_error {
      typedef std::runtime_error ancestor ;
   public:
      explicit unexpected_char(const std::string &what) :
         ancestor(what)
      {}
} ;

/// @cond
namespace detail {

template<typename Char> struct digits {
      static const Char data[36] ;
} ;

template<typename Char>
const Char digits<Char>::data[] =
{
   '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
   'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
   'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
   'U', 'V', 'W', 'X', 'Y', 'Z'
} ;

template<typename Char, typename Integer>
Char *inttobuf(Integer value, Char *end, int base, std::true_type *)
{
   if (value < 0)
   {
      do *--end = digits<Char>::data[-(value % base)] ;
      while (value /= base) ;
      *--end = '-' ;
   }
   else
      do *--end = digits<Char>::data[value % base] ;
      while (value /= base) ;
   return end ;
}

template<typename Char, typename Integer>
Char *inttobuf(Integer value, Char *end, int base, std::false_type *)
{
   do *--end = digits<Char>::data[value % base] ;
   while (value /= base) ;
   return end ;
}

template<typename Char, typename Integer>
inline Char *inttostr(Integer number, Char *buffer, size_t bufsize, int base)
{
   if (!bufsize || !buffer)
      return buffer ;
   if (base == 1 || (unsigned)base > 36 || bufsize == 1)
      *buffer = 0 ;
   else
   {
      Char tmpbuf[sizeof(uintmax_t) * 8] ;
      Char * const end = tmpbuf + P_ARRAY_COUNT(tmpbuf) ;
      Char * const start =
         inttobuf(number, end, base ? base : 10, (bool_constant<std::is_signed<Integer>::value> *)NULL) ;
      const size_t sz = std::min<size_t>(end - start, bufsize - 1) ;
      memcpy(buffer, start, sz * sizeof(Char)) ;
      buffer[sz] = 0 ;
   }
   return buffer ;
}

template<typename Char>
inline Char *inttostr(bool value, Char *buffer, size_t bufsize, int base)
{
   if (bufsize && buffer)
   {
      if (base == 1 || (unsigned)base > 36 || bufsize == 1)
         *buffer = 0 ;
      else
      {
         buffer[0] = value ? '1' : '0' ;
         buffer[1] =  0 ;
      }
   }
   return buffer ;
}
} // end of namespace pcomn::detail
/// @endcond

/// Convert a value of any integral type to a C string with specified radix.
/// @ingroup NumStrConversion
/// @return The passed pointer to a buffer.
/// @note The function @em always terminates a buffer with zero (except for
/// the case of @c bufsize==0), so it is pretty safe.
/// @param number    Value to convert.
/// @param buffer    Buffer to place result to.
/// @param bufsize   Buffer size (in characters).
/// @param base      Conversion radix, from 2 to 36; is not specified, then 10.
template<typename Char, typename Integer>
inline typename ctype_traits<Char>::char_type *
numtostr(Integer number, Char *buffer, size_t bufsize,
         typename if_integer<Integer, int>::type base = 0)
{
   return detail::inttostr(number, buffer, bufsize, base) ;
}

/// @overload
/// @ingroup NumStrConversion
template<typename Char, typename Integer, size_t bufsize>
inline typename ctype_traits<Char>::char_type *
numtostr(Integer number, Char (&buffer)[bufsize],
         typename if_integer<Integer, int>::type base = 0)
{
   return numtostr(number, buffer + 0, bufsize, base) ;
}

/// @overload
/// @ingroup NumStrConversion
template<typename Str, typename Integer>
inline Str numtostr(Integer number, typename if_integer<Integer, int>::type base = 0)
{
   // The buffer is big enough even for base==2
   char buf[sizeof(number) * 8 + 3] ;
   return Str(numtostr(number, buf, base)) ;
}

/// Convert a value of any integral type to a string and copy the result to an iterator.
/// @ingroup NumStrConversion
/// @param number    Value to convert.
/// @param out       Iterator to copy the result to.
/// @param base      Conversion radix, from 2 to 36; is not specified, then 10.
/// @return @a out + len(numtostr(@a number))
template<typename OutputIterator, typename Integer>
inline OutputIterator numtoiter(Integer number, OutputIterator out,
                                typename if_integer<Integer, int>::type base = 0)
{
   // The buffer is big enough even for base==2
   char buf[sizeof(number) * 8 + 3] ;
   return std::copy(buf + 0, buf + strlen(numtostr(number, buf, base)), out) ;
}

/*******************************************************************************
 String to integer conversions
*******************************************************************************/
/// @cond
namespace detail {

template<typename T>
inline typename if_signed_int<T, bool>::type check_overflow(T newval, T oldval, int sign)
{
   return sign > 0 ? (newval < oldval) : (newval > oldval) ;
}

template<typename T>
inline typename if_unsigned_int<T, bool>::type check_overflow(T newval, T oldval, int)
{
   return newval < oldval ;
}

template<int radix, typename Integer>
inline Integer augment_with_digit(Integer n, int digit, int sign)
{
   PCOMN_STATIC_CHECK(1 <= radix && radix <= 36) ;
   const Integer next_digit = digit * sign ;
   const Integer oldval = n ;
   const Integer newval = oldval * (Integer)radix + next_digit ;
   PCOMN_THROW_IF(check_overflow(newval, oldval, sign), std::overflow_error,
                  "Overflow while converting string to %s.", typeid(Integer).name()) ;
   return newval ;
}

template<typename Integer>
inline Integer ensure_next_digit(Integer n, int c, int sign)
{
   PCOMN_THROW_IF(!isdigit(c), unexpected_char,
                  "Unexpected character: %s encountered while expecting a decimal digit.",
                  charrepr(c).c_str()) ;
   return
      augment_with_digit<10>(n, c - '0', sign) ;
}
} // end of namespace pcomn::detail
/// @endcond

template<typename Signed, typename Range>
inline typename if_signed_int<Signed, Range>::type
strtonum(Range input, Signed &result)
{
   Signed intermed = 0 ;
   int count = 0 ;
   for (int sign = 0 ; input ; ++input)
   {
      const int cc = *input ;
      if (sign || (sign = cc == '-' ? -1 : 1) > 0)
      {
         if (!count++)
            intermed = detail::ensure_next_digit(intermed, cc, sign) ;
         else if (isdigit(cc))
            intermed = detail::augment_with_digit<10>(intermed, cc - '0', sign) ;
         else
            break ;
      }
   }
   if (count)
      result = intermed ;
   return input ;
}

template<typename Unsigned, typename Range>
inline typename if_unsigned_int<Unsigned, Range>::type
strtonum(Range input, Unsigned &result)
{
   if (input)
   {
      Unsigned intermed = detail::ensure_next_digit(0, *input, 1) ;
      int cc ;
      while (++input && isdigit(cc = *input))
         intermed = detail::augment_with_digit<10>(intermed, cc - '0', 1) ;
      result = intermed ;
   }
   return input ;
}

template<typename Range>
inline Range strtonum(Range input, bool &result)
{
   if (input)
   {
      const uint8_t intermed = detail::ensure_next_digit((uint8_t)0, *input, 1) ;
      PCOMN_THROW_IF(intermed > 1, std::overflow_error, "Overflow while converting string to bool.") ;
      result = !!intermed ;
      ++input ;
   }
   return input ;
}

template<typename Num, typename Range>
inline typename if_integer<Num, Num>::
type strtonum(Range input)
{
   Num result = Num() ;
   strtonum(input, result) ;
   return result ;
}

/// Convert a string to a number, don't throw exceptions on conversion error
///
/// @return pair(num,bool), where pair.first is the result of a conversiom, pair.second
/// is true if conversion is successful and false otherwise; in case of conversion
/// failure, pair.first is 0.
template<typename Num, typename Range>
typename if_integer<Num, std::pair<Num, bool> >::
type strtonum_safe(Range input)
{
   std::pair<Num, bool> result (Num(), false) ;
   try {
      strtonum(input, result.first) ;
      result.second = true;
   }
   catch(const std::exception &)
   {}
   return result ;
}

template<typename Num, typename Range>
inline Num strtonum_def(Range input, Num def)
{
    std::pair<Num, bool> res = pcomn::strtonum_safe<Num>(input) ;
    return input && res.second ? res.first : def ;
}

} // end of namespace pcomn

#endif /* __PCOMN_STRNUM_H */
