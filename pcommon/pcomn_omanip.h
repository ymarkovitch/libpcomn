/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_OMANIP_H
#define __PCOMN_OMANIP_H
/*******************************************************************************
 FILE         :   pcomn_omanip.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Output manipulators

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   12 May 2000
*******************************************************************************/
#include <pcomn_platform.h>
#include <iostream>
#include <stdio.h>

namespace pcomn {

struct noparam {} ;
typedef noparam empty_param ;
template<class P>
struct param_count { enum { Count = 1 } ; } ;
template<>
struct param_count<empty_param> { enum { Count = 0 } ; } ;

template<int i>
struct type2int { enum { Value = i } ; } ;

template<class Fn,
         class P1 = empty_param,
         class P2 = empty_param,
         class P3 = empty_param,
         class P4 = empty_param>
class omanip {
   public:
      enum { ParamCount =
             param_count<P1>::Count +
             param_count<P2>::Count +
             param_count<P3>::Count +
             param_count<P4>::Count } ;

      omanip(Fn f, P1 arg1, P2 arg2, P3 arg3, P4 arg4) :
         _f(f),
         _arg1(arg1),
         _arg2(arg2),
         _arg3(arg3),
         _arg4(arg4)
      {}

      omanip(Fn f, P1 arg1, P2 arg2, P3 arg3) :
         _f(f),
         _arg1(arg1),
         _arg2(arg2),
         _arg3(arg3)
      {}

      omanip(Fn f, P1 arg1, P2 arg2) :
         _f(f),
         _arg1(arg1),
         _arg2(arg2)
      {}

      omanip(Fn f, P1 arg1) :
         _f(f),
         _arg1(arg1)
      {}

      omanip(Fn f) :
         _f(f)
      {}

      template<typename E>
      std::basic_ostream<E> &operator()(std::basic_ostream<E> &os) const
      {
         return call(os, type2int<ParamCount>()) ;
      }

   private:
      Fn _f ;
      P1 _arg1 ;
      P2 _arg2 ;
      P3 _arg3 ;
      P4 _arg4 ;

      template<typename E>
      std::basic_ostream<E> &
      call(std::basic_ostream<E> &os, type2int<0>) const { return _f(os) ; }

      template<typename E>
      std::basic_ostream<E> &
      call(std::basic_ostream<E> &os, type2int<1>) const { return _f(os, _arg1) ; }

      template<typename E>
      std::basic_ostream<E> &
      call(std::basic_ostream<E> &os, type2int<2>) const { return _f(os, _arg1, _arg2) ; }

      template<typename E>
      std::basic_ostream<E> &
      call(std::basic_ostream<E> &os, type2int<3>) const { return _f(os, _arg1, _arg2, _arg3) ; }

      template<typename E>
      std::basic_ostream<E> &
      call(std::basic_ostream<E> &os, type2int<4>) const { return _f(os, _arg1, _arg2, _arg3, _arg4) ; }
} ;

template<class InputIterator, typename Before, typename After>
struct _oseq {
      std::ostream &operator() (std::ostream &os,
                                InputIterator begin,
                                InputIterator end,
                                const Before &before,
                                const After &after) const ;
} ;

template<class InputIterator, typename Before, typename After>
std::ostream &_oseq<InputIterator, Before, After>::operator()
   (std::ostream &os,
    InputIterator begin, InputIterator end,
    const Before &before, const After &after) const
{
   for (; begin != end ; ++begin)
      os << before << *begin << after ;
   return os ;
}

template<class InputIterator, typename Before, typename After>
inline omanip<_oseq<InputIterator, Before, After>,
              InputIterator, InputIterator, const Before &, const After &>
osequence(InputIterator begin, InputIterator end, const Before &before, const After &after)
{
   typedef _oseq<InputIterator, Before, After> fn_t ;
   return
      omanip<fn_t, InputIterator, InputIterator, const Before &, const After &>
      (fn_t(), begin, end, before, after) ;
}

template<class InputIterator, typename After>
inline omanip<_oseq<InputIterator, const char *, After>,
              InputIterator, InputIterator, const char *, const After &>
osequence(InputIterator begin, InputIterator end, const After &after)
{
   typedef _oseq<InputIterator, const char *, After> fn_t ;
   return
      omanip<fn_t, InputIterator, InputIterator, const char *, const After &>
      (fn_t(), begin, end, "", after) ;
}

template<class InputIterator>
inline omanip<_oseq<InputIterator, const char *, char>,
              InputIterator, InputIterator, const char *, char>
osequence(InputIterator begin, InputIterator end)
{
   typedef _oseq<InputIterator, const char *, char> fn_t ;
   return
      omanip<fn_t, InputIterator, InputIterator, const char *, char>
      (fn_t(), begin, end, "", '\n') ;
}

template<class Container, typename Before, typename After>
inline omanip<_oseq<typename Container::const_iterator, Before, After>,
              typename Container::const_iterator, typename Container::const_iterator,
              const Before &, const After &>
ocontainer(const Container &container, const Before &before, const After &after)
{
   return osequence(container.begin(), container.end(), before, after) ;
}

template<class Container, typename After>
inline omanip<_oseq<typename Container::const_iterator, const char *, After>,
              typename Container::const_iterator, typename Container::const_iterator,
              const char *, const After &>
ocontainer(const Container &container, const After &after)
{
   return osequence(container.begin(), container.end(), after) ;
}

template<class Container>
inline omanip<_oseq<typename Container::const_iterator, const char *, char>,
              typename Container::const_iterator, typename Container::const_iterator,
              const char *, char>
ocontainer(const Container &container)
{
   return osequence(container.begin(), container.end()) ;
}

template<class E, class Fn, class P1, class P2, class P3, class P4>
inline std::basic_ostream<E> &operator<<(std::basic_ostream<E> &os,
                                         const pcomn::omanip<Fn, P1, P2, P3, P4> &omanip)
{
   return omanip(os) ;
}

template<class InputIterator, typename Delim>
struct _oseqdelim {
      std::ostream &operator() (std::ostream &os,
                                InputIterator begin,
                                InputIterator end,
                                const Delim &delim) const ;
} ;

template<class InputIterator, typename Delim>
std::ostream &_oseqdelim<InputIterator, Delim>::operator()
   (std::ostream &os,
    InputIterator begin, InputIterator end,
    const Delim &delim) const
{
    for (bool first = true; begin != end ; ++begin, first = false) {
        if (!first)
            os << delim ;
        os << *begin ;
    }
    return os ;
}

template<class InputIterator, typename Delim>
inline omanip<_oseqdelim<InputIterator, Delim>,
              InputIterator, InputIterator, const Delim&>
oseqdelim(InputIterator begin, InputIterator end, const Delim &delim)
{
   typedef _oseqdelim<InputIterator, Delim> fn_t ;
   return
      omanip<fn_t, InputIterator, InputIterator, const Delim&>
      (fn_t(), begin, end, delim) ;
}

template<class InputIterator>
inline omanip<_oseqdelim<InputIterator, const char *>,
              InputIterator, InputIterator, const char *>
oseqdelim(InputIterator begin, InputIterator end, const char *delim = ", ")
{
   typedef _oseqdelim<InputIterator, const char*> fn_t ;
   return
      omanip<fn_t, InputIterator, InputIterator, const char*>
      (fn_t(), begin, end, delim) ;
}

template<class Container, typename Delim>
inline omanip<_oseqdelim<typename Container::const_iterator, Delim>,
              typename Container::const_iterator, typename Container::const_iterator,
              const Delim&>
ocontdelim(const Container &container, const Delim &delim)
{
    return oseqdelim(container.begin(), container.end(), delim) ;
}

template<class Container>
inline omanip<_oseqdelim<typename Container::const_iterator, const char*>,
              typename Container::const_iterator, typename Container::const_iterator,
              const char*>
ocontdelim(const Container &container)
{
    return oseqdelim(container.begin(), container.end()) ;
}

inline char *hrsize(uint64_t sz, char *buf)
{
   if (sz < KiB)
      sprintf(buf, "%uB", (unsigned)sz) ;
   else if (sz < MiB)
      sprintf(buf, "%.1fK", sz/(KiB * 1.0)) ;
   else if (sz < GiB)
      sprintf(buf, "%.1fM", sz/(MiB * 1.0)) ;
   else
      sprintf(buf, "%.1fG", sz/(GiB * 1.0)) ;
   return buf ;
}

template<typename T>
struct _ohrsize {
      std::ostream &operator() (std::ostream &os, const T &sz) const
      {
         char buf[64] ;
         return os << hrsize(sz, buf) ;
      }
} ;

template<typename T>
inline omanip<_ohrsize<T>, const T &> ohrsize(const T &sz)
{
   typedef _ohrsize<T> fn_t ;
   return omanip<fn_t, const T &>(fn_t(), sz) ;
}

template<typename T>
struct _ostrq {
      std::ostream &operator() (std::ostream &os, const T &str) const
      {
         return os << '\'' << str << '\'' ;
      }
} ;

template<typename T>
inline omanip<_ostrq<T>, const T &> ostrq(const T &str)
{
   return omanip<_ostrq<T>, const T &>(_ostrq<T>(), str) ;
}

} // end of namespace pcomn

#endif /* __PCOMN_OMANIP_H */
