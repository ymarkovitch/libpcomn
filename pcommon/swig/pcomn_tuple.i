/*-*- mode:swig;tab-width:2;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_tuple.i
 COPYRIGHT    :   Yakov Markovitch, 2015
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   SWIG typemaps std::tuple specializations

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   30 Oct 2015
*******************************************************************************/
%include "pcomn_macros.swg"
%include "pcomn_property.swg"

%include <std_common.i>
%include <exception.i>

%{
#include <pcomn_tuple.h>
%}

namespace pcomn {

template<typename T1, typename T2, typename T3>
struct triple {
    triple() ;
    triple(T1 a1, T2 a2, T3 a3) ;
    triple(const triple &) ;

    template<typename U1, typename U2, typename U3>
    triple(const triple<U1, U2, U3> &) ;

%csmethodmodifiers get0_ const "private" ;
%csmethodmodifiers get1_ const "private" ;
%csmethodmodifiers get2_ const "private" ;
%csmethodmodifiers get3_ const "private" ;

%extend {
  const T1 &get0_() const { return std::get<0>(*$self) ; }
  const T2 &get1_() const { return std::get<1>(*$self) ; }
  const T3 &get2_() const { return std::get<2>(*$self) ; }
}
%typemap(cscode) triple<T1, T2, T3> %{
  public $csclassname(Tuple<$typemap(cstype, T1), $typemap(cstype, T2), $typemap(cstype, T3)> source)
      : this(source.Item1, source.Item2, source.Item3)
  {}

  public static implicit operator Tuple<$typemap(cstype, T1), $typemap(cstype, T2), $typemap(cstype, T3)>($csclassname v)
  {
      return Tuple.Create(v.Item1, v.Item2, v.Item3) ;
  }

  public $typemap(cstype, T1) Item1 { get { return get0_() ; } }

  public $typemap(cstype, T2) Item2 { get { return get1_() ; } }

  public $typemap(cstype, T3) Item3 { get { return get2_() ; } }

  public override string ToString() { return Tuple.Create(Item1, Item2, Item3).ToString() ; }

%}
} ;

}
