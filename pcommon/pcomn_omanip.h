/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_OMANIP_H
#define __PCOMN_OMANIP_H
/*******************************************************************************
 FILE         :   pcomn_omanip.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Output manipulators

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   12 May 2000
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcomn_iterator.h>

#include <iostream>
#include <iomanip>
#include <functional>
#include <type_traits>

#include <stdio.h>

namespace pcomn {

enum class NoOut { nout } ;
const NoOut nout = NoOut::nout ;

inline std::ostream &operator<<(std::ostream &os, NoOut) { return os ; }

template<typename> struct omanip ;

/*******************************************************************************
 Note the MSVC++ does allow C++14 constructs in C++11 code, but frequently
 unable to compile pretty valid C++11 constructs!
*******************************************************************************/
#if defined(PCOMN_COMPILER_CXX14) || PCOMN_WORKAROUND(_MSC_VER, >=1800)
#  define PCOMN_DERIVE_OMANIP(basecall) { return basecall ; }
#  define PCOMN_MAKE_OMANIP(...) { return pcomn::make_omanip(__VA_ARGS__) ; }
#  define PCOMN_MAKE_OMANIP_RETTYPE()
#else
#  define PCOMN_DERIVE_OMANIP(basecall) ->decltype(basecall) { return basecall ; }
#  define PCOMN_MAKE_OMANIP(...) ->decltype(pcomn::make_omanip(__VA_ARGS__)) { return pcomn::make_omanip(__VA_ARGS__) ; }
#  define PCOMN_MAKE_OMANIP_RETTYPE() -> omanip<decltype(std::bind(std::forward<F>(fn), std::placeholders::_1, std::forward<Args>(args)...))>
#endif


template<typename F, typename... Args>
inline auto make_omanip(F &&fn, Args &&...args) PCOMN_MAKE_OMANIP_RETTYPE()
{
   using namespace std ;

   typedef valtype_t<decltype(bind(forward<F>(fn), placeholders::_1, forward<Args>(args)...))> manip ;
   return omanip<manip>(manip(bind(forward<F>(fn), placeholders::_1, forward<Args>(args)...))) ;
}


/******************************************************************************/
/* Universal ostream manipulator
*******************************************************************************/
template<typename Bind>
struct omanip final {
      static_assert(std::is_same<typename std::remove_reference<typename std::result_of<Bind(std::ostream &)>::type>::type, std::ostream>::value,
                    "Invalid pcomn::omanip template argument: must be callable with std::ostream & argument and return std::ostream &") ;

      omanip(omanip &&other) : _fn(std::move(other._fn)) {}

      omanip(const omanip &) = delete ;
      omanip &operator=(const omanip &) = delete ;
      omanip &operator=(omanip &&) = delete ;

      std::ostream &operator()(std::ostream &os) const { return _fn(os) ; }

      template<typename F, typename... Args>
      friend auto make_omanip(F &&fn, Args &&...args) PCOMN_MAKE_OMANIP_RETTYPE() ;

   private:
      mutable Bind _fn ;

      omanip(Bind &&f) : _fn(std::move(f)) {}
} ;

/*******************************************************************************
 Printing algorithms
*******************************************************************************/
template<typename InputIterator, typename Delim>
std::ostream &print_sequence(InputIterator begin, InputIterator end, std::ostream &os,
                             const Delim &delimiter)
{
   int count = -1 ;
   for (; begin != end ; ++begin)
   {
      if (++count)
         os << delimiter ;
      os << *begin ;
   }
   return os ;
}

template<typename InputIterator, typename Delim, typename Fn>
std::ostream &print_sequence(InputIterator begin, InputIterator end, std::ostream &os,
                             const Delim &delimiter, Fn &&fn)
{
   int count = -1 ;
   for (; begin != end ; ++begin)
   {
      if (++count)
         os << delimiter ;
      std::forward<Fn>(fn)(os, *begin) ;
   }
   return os ;
}

template<typename InputIterator>
inline std::ostream &print_sequence(InputIterator begin, InputIterator end, std::ostream &os)
{
   return print_sequence(begin, end, os, ", ") ;
}

/*******************************************************************************
 Various ostream manipulators
*******************************************************************************/
namespace detail {
template<typename T>
using decayed = typename std::decay<T>::type ;

template<typename InputIterator, typename Before, typename After>
std::ostream &o_sequence(std::ostream &os, InputIterator begin, InputIterator end,
                         const Before &before, const After &after)
{
   for (; begin != end ; ++begin)
      os << before << *begin << after ;
   return os ;
}

template<typename InputIterator, typename Delim>
inline std::ostream &o_sequence(std::ostream &os, InputIterator begin, InputIterator end,
                                const Delim &delimiter)
{
   return print_sequence(begin, end, os, delimiter) ;
}

template<typename InputIterator>
inline std::ostream &o_sequence(std::ostream &os, InputIterator begin, InputIterator end)
{
   return print_sequence(begin, end, os) ;
}
}

template<typename InputIterator, typename Before, typename After>
inline auto osequence(InputIterator begin, InputIterator end, const Before &before, const After &after)
   PCOMN_MAKE_OMANIP(detail::o_sequence<InputIterator, detail::decayed<Before>, detail::decayed<After> >,
                     begin, end, detail::decayed<Before>(before), detail::decayed<After>(after)) ;


template<typename InputIterator, typename After>
inline auto osequence(InputIterator begin, InputIterator end, const After &after)
   PCOMN_DERIVE_OMANIP(osequence(begin, end, nout, after)) ;

template<typename InputIterator>
inline auto osequence(InputIterator begin, InputIterator end)
   PCOMN_DERIVE_OMANIP(osequence(begin, end, nout, '\n')) ;

template<class Container, typename Before, typename After>
inline auto ocontainer(const Container &container, const Before &before, const After &after)
   PCOMN_DERIVE_OMANIP(osequence(std::begin(container), std::end(container), before, after)) ;

template<class Container, typename After>
inline auto ocontainer(const Container &container, const After &after)
   PCOMN_DERIVE_OMANIP(osequence(std::begin(container), std::end(container), after)) ;

template<class Container>
inline auto ocontainer(const Container &container)
   PCOMN_DERIVE_OMANIP(osequence(std::begin(container), std::end(container))) ;

template<typename InputIterator, typename Delim>
inline auto oseqdelim(InputIterator begin, InputIterator end, const Delim &delim)
   PCOMN_MAKE_OMANIP(detail::o_sequence<InputIterator, detail::decayed<Delim> >,
                     begin, end, detail::decayed<Delim>(delim)) ;

template<typename InputIterator>
inline auto oseqdelim(InputIterator begin, InputIterator end)
   PCOMN_MAKE_OMANIP(detail::o_sequence<InputIterator>, begin, end) ;

template<class Container, typename Delim>
inline auto ocontdelim(const Container &container, const Delim &delim)
   PCOMN_DERIVE_OMANIP(oseqdelim(std::begin(container), std::end(container), delim)) ;

template<typename T, size_t sz, typename Delim>
inline auto ocontdelim(T (&container)[sz], const Delim &delim)
   PCOMN_DERIVE_OMANIP(oseqdelim(std::begin(container), std::end(container), delim)) ;

template<class Container>
inline auto ocontdelim(const Container &container)
   PCOMN_DERIVE_OMANIP(oseqdelim(std::begin(container), std::end(container)))

inline char *hrsize(unsigned long long sz, char *buf)
{
   if (sz < KiB)
      sprintf(buf, "%lluB", sz) ;
   else if (sz < MiB)
      sprintf(buf, "%.1fK", sz/(KiB * 1.0)) ;
   else if (sz < GiB)
      sprintf(buf, "%.1fM", sz/(MiB * 1.0)) ;
   else
      sprintf(buf, "%.1fG", sz/(GiB * 1.0)) ;
   return buf ;
}

namespace detail {
inline std::ostream &print_hrsize(std::ostream &os, unsigned long long sz)
{
   char buf[64] ;
   return os << hrsize(sz, buf) ;
}

template<typename T>
std::ostream &print_quoted_string(std::ostream &os, const T &str)
{
   return os << '\'' << str << '\'' ;
}
}

template<typename T>
inline auto ohrsize(const T &sz)
   PCOMN_MAKE_OMANIP(&detail::print_hrsize, (unsigned long long)(sz)) ;

template<typename T>
inline auto ostrq(const T &str)
   PCOMN_MAKE_OMANIP((&detail::print_quoted_string<T>), std::cref(str)) ;

/******************************************************************************/
/** Apply pcomn::omanip<F> manipulator object to an output stream
*******************************************************************************/
template<typename F>
inline std::ostream &operator<<(std::ostream &os, const omanip<F> &manip)
{
   return manip(os) ;
}
} // end of namespace pcomn


#endif /* __PCOMN_OMANIP_H */
