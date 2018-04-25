/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_OMANIP_H
#define __PCOMN_OMANIP_H
/*******************************************************************************
 FILE         :   pcomn_omanip.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2017. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Output manipulators

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   12 May 2000
*******************************************************************************/
/** @file
  Output manipulators

  @li osequence
  @li oseqdelim
  @li ocontainer
  @li ocontdelim

  @li onothing
  @li oskip

  @li ohrsize
  @li ohrsizex
  @li ostrq
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcomn_iterator.h>
#include <pcomn_flgout.h>
#include <pcomn_meta.h>

#include <iomanip>
#include <vector>
#include <memory>
#include <typeindex>
#include <utility>
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
#ifdef PCOMN_STL_CXX14
#  define PCOMN_DERIVE_OMANIP(basecall) { return basecall ; }
#  define PCOMN_MAKE_OMANIP(...) { return pcomn::make_omanip(__VA_ARGS__) ; }
#else
#  define PCOMN_DERIVE_OMANIP(basecall) ->decltype(basecall) { return basecall ; }
#  define PCOMN_MAKE_OMANIP(...) ->decltype(pcomn::make_omanip(__VA_ARGS__)) { return pcomn::make_omanip(__VA_ARGS__) ; }
#endif

namespace detail {
struct omanip_maker {
      template<typename F, typename... Args>
      auto operator()(F &&fn, Args &&...args) const
         ->omanip<decltype(std::bind(std::forward<F>(fn), std::placeholders::_1, std::forward<Args>(args)...))>
      {
         return {std::bind(std::forward<F>(fn), std::placeholders::_1, std::forward<Args>(args)...)} ;
      }
} ;
}

static constexpr const detail::omanip_maker make_omanip {} ;

/******************************************************************************/
/* Universal ostream manipulator
*******************************************************************************/
template<typename F>
struct omanip final {
      static_assert(std::is_same<std::remove_reference_t<std::result_of_t<F(std::ostream &)>>, std::ostream>::value,
                    "Invalid pcomn::omanip template argument: must be callable with std::ostream & argument and return std::ostream &") ;

      omanip(omanip &&other) : _fn(std::move(other._fn)) {}

      omanip(const omanip &) = delete ;
      omanip &operator=(const omanip &) = delete ;
      omanip &operator=(omanip &&) = delete ;

      std::ostream &operator()(std::ostream &os) const { return _fn(os) ; }

      friend detail::omanip_maker ;

   private:
      mutable F _fn ;

      omanip(F &&fn) : _fn(std::move(fn)) {}
} ;

/*******************************************************************************
 Printing algorithms
*******************************************************************************/
template<typename InputIterator, typename Delim, typename Fn>
std::ostream &print_sequence(InputIterator begin, InputIterator end, std::ostream &os,
                             Delim &&delimiter, Fn &&fn)
{
   bool init = false ;
   for (; begin != end ; ++begin)
   {
      if (init)
         os << std::forward<Delim>(delimiter) ;
      else
         init = true ;
      std::forward<Fn>(fn)(os, *begin) ;
   }
   return os ;
}

template<typename Range, typename Delim, typename Fn>
std::ostream &print_range(Range &&r, std::ostream &os, Delim &&delimiter, Fn &&fn)
{
   bool init = false ;
   for (const auto &v: std::forward<Range>(r))
   {
      if (init)
         os << std::forward<Delim>(delimiter) ;
      else
         init = true ;
      std::forward<Fn>(fn)(os, v) ;
   }
   return os ;
}

/*******************************************************************************
 Various ostream manipulators
*******************************************************************************/
namespace detail {
struct o_sequence {
      template<typename InputIterator, typename Before, typename After>
      __noinline std::ostream &operator()(std::ostream &os, InputIterator begin, InputIterator end,
                                          const Before &before, const After &after) const
      {
         for (; begin != end ; ++begin)
            os << before << *begin << after ;
         return os ;
      }

      template<typename InputIterator, typename Delim>
      std::ostream &operator()(std::ostream &os, InputIterator begin, InputIterator end,
                               const Delim &delimiter) const
      {
         return print_sequence(begin, end, os, delimiter,
                               [](std::ostream &s, decltype(*begin) &v) { s << v ; }) ;
      }

      template<typename InputIterator>
      std::ostream &operator()(std::ostream &os, InputIterator begin, InputIterator end) const
      {
         return (*this)(os, begin, end, ", ") ;
      }
} ;

struct o_call {
      template<typename F>
      std::ostream &operator()(std::ostream &os, F &&fn) const
      {
         std::forward<F>(fn)(os) ; return os ;
      }
} ;

inline std::ostream &o_nothing(std::ostream &os) { return os ; }
inline std::ostream &o_skip(std::ostream &os, unsigned w) { return !w ? os : (os << std::setw(w) << "") ; }
}


template<typename InputIterator, typename Before, typename After>
inline auto osequence(InputIterator begin, InputIterator end, const Before &before, const After &after)
   PCOMN_MAKE_OMANIP(detail::o_sequence(), begin, end, std::decay_t<Before>(before), std::decay_t<After>(after)) ;

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
   PCOMN_MAKE_OMANIP(detail::o_sequence(), begin, end, std::decay_t<Delim>(delim)) ;

template<typename InputIterator>
inline auto oseqdelim(InputIterator begin, InputIterator end)
   PCOMN_MAKE_OMANIP(detail::o_sequence(), begin, end) ;

template<class Container, typename Delim>
inline auto ocontdelim(const Container &container, const Delim &delim)
   PCOMN_DERIVE_OMANIP(oseqdelim(std::begin(container), std::end(container), delim)) ;

template<typename T, size_t sz, typename Delim>
inline auto ocontdelim(T (&container)[sz], const Delim &delim)
   PCOMN_DERIVE_OMANIP(oseqdelim(std::begin(container), std::end(container), delim)) ;

template<class Container>
inline auto ocontdelim(const Container &container)
   PCOMN_DERIVE_OMANIP(oseqdelim(std::begin(container), std::end(container))) ;

template<typename... Args>
inline auto onothing(Args &&...)
   PCOMN_MAKE_OMANIP(detail::o_nothing) ;

template<typename... Args>
inline auto oskip(unsigned width)
   PCOMN_MAKE_OMANIP(detail::o_skip, width) ;

inline char *hrsize(unsigned long long sz, char *buf)
{
   if (sz < KiB)
      sprintf(buf, "%llu", sz) ;
   else if (sz < MiB)
      sprintf(buf, "%.2fK", sz/(KiB * 1.0)) ;
   else if (sz < GiB)
      sprintf(buf, "%.2fM", sz/(MiB * 1.0)) ;
   else
      sprintf(buf, "%.2fG", sz/(GiB * 1.0)) ;
   return buf ;
}

inline char *hrsize_exact(unsigned long long sz, char *buf)
{
   char m ;
   if (!sz || (sz % KiB))
      m = 0 ;

   else if (!(sz % GiB))
   {
      sz /= GiB ;
      m = 'G' ;
   }
   else if (!(sz % MiB))
   {
      sz /= MiB ;
      m = 'M' ;
   }
   else
   {
      sz /= KiB ;
      m = 'K' ;
   }
   sprintf(buf, "%llu%c", sz, m) ;
   return buf ;
}

namespace detail {
inline std::ostream &print_hrsize(std::ostream &os, unsigned long long sz)
{
   char buf[64] ;
   return os << hrsize(sz, buf) ;
}

inline std::ostream &print_hrsizex(std::ostream &os, unsigned long long sz)
{
   char buf[64] ;
   return os << hrsize_exact(sz, buf) ;
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
inline auto ohrsizex(const T &sz)
   PCOMN_MAKE_OMANIP(&detail::print_hrsizex, (unsigned long long)(sz)) ;

template<typename T>
inline auto ostrq(const T &str)
   PCOMN_MAKE_OMANIP((&detail::print_quoted_string<T>), std::cref(str)) ;

template<typename F>
inline auto ocall(F &&fn)
   PCOMN_MAKE_OMANIP(detail::o_call(), std::forward<F>(fn)) ;

template<typename Enum>
inline auto oenum(Enum value)
   PCOMN_MAKE_OMANIP(print_enum<Enum>, value) ;

/******************************************************************************/
/** Apply pcomn::omanip<F> manipulator object to an output stream
*******************************************************************************/
template<typename F>
inline std::ostream &operator<<(std::ostream &os, const omanip<F> &manip)
{
   return manip(os) ;
}

/******************************************************************************
 std::ostream output for various classes, including std namespace members
******************************************************************************/
template<typename> class simple_slice ;
template<typename> class simple_vector ;
template<typename, size_t> class static_vector ;
template<typename> struct matrix_slice ;
template<typename, bool> struct simple_matrix ;

template<typename T>
inline std::ostream &operator<<(std::ostream &os, const simple_slice<T> &v)
{
   return os << oseqdelim(v.begin(), v.end(), ' ') ;
}

template<typename T>
inline std::ostream &operator<<(std::ostream &os, const simple_vector<T> &v)
{
   return os << oseqdelim(v.begin(), v.end(), ' ') ;
}

template<typename T, size_t n>
inline std::ostream &operator<<(std::ostream &os, const static_vector<T,n> &v)
{
   return os << oseqdelim(v.begin(), v.end(), ' ') ;
}

} // end of pcomn namespace

/*******************************************************************************
 Half-legal imlementation of formatted output for several standard library
 objects.
 These are half-legal because injected into std:: namespace which is bad
 according to Standard, but we'll make do with it.
*******************************************************************************/
// We need std namespace to ensure proper Koenig lookup
namespace std {

template<typename T1, typename T2>
ostream &operator<<(ostream &os, const pair<T1,T2> &p)
{
   return os << '{' << p.first << ',' << p.second << '}' ;
}

template<typename T, typename A>
inline ostream &operator<<(ostream &os, const vector<T,A> &v)
{
   return os << pcomn::ocontdelim(v, ' ') ;
}

template<typename T, typename D>
inline ostream &operator<<(ostream &os, const unique_ptr<T,D> &v)
{
   return os << v.get() ;
}

inline ostream &operator<<(ostream &os, const type_index &v)
{
   return os << PCOMN_DEMANGLE(v.name()) ;
}

inline ostream &operator<<(ostream &os, nullptr_t)
{
   return os << "NULL" ;
}
} // end of namespace std


#endif /* __PCOMN_OMANIP_H */
