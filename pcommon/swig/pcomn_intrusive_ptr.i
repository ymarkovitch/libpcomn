/*-*- mode:swig;tab-width:2;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_intrusive_ptr.i
 COPYRIGHT    :   Yakov Markovitch, 2015
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   SWIG typemaps for pcomn::shared_intrusive_ptr<T>

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   30 Oct 2015
*******************************************************************************/
%include "pcomn_macros.swg"

%{
#include <pcomn_smartptr.h>
%}

namespace pcomn {
template<class T> struct shared_intrusive_ptr {} ;
}

%define %pcomn_refcounted(type, ...)
%feature("ref")   type __VA_ARGS__ "$this->inc() ;"
%feature("unref") type __VA_ARGS__ "$this->dec() ;"
%enddef

%typemap(ctype)
     pcomn::shared_intrusive_ptr,
     pcomn::shared_intrusive_ptr &,
     const pcomn::shared_intrusive_ptr &,
     pcomn::shared_intrusive_ptr *self

    "pcomn::PRefCount *"

%typemap(imtype, out="IntPtr")
     pcomn::shared_intrusive_ptr,
     pcomn::shared_intrusive_ptr &,
     const pcomn::shared_intrusive_ptr &,
     pcomn::shared_intrusive_ptr *self

    "HandleRef"

/***************************************************************************
 %pcomn_shared_intrusive_ptr
***************************************************************************/
%define %pcomn_shared_intrusive_ptr(TYPE, TYPEDEF...)
%pcomn_refcounted(%arg(TYPE)) ;

%typemap(cstype)
    pcomn::shared_intrusive_ptr<TYPE>,
    pcomn::shared_intrusive_ptr<TYPE> &,
    const pcomn::shared_intrusive_ptr<TYPE> &,
    pcomn::shared_intrusive_ptr<TYPE> *self

   "$typemap(cstype, TYPE)"

%typemap(out)
    pcomn::shared_intrusive_ptr<TYPE>,
    pcomn::shared_intrusive_ptr<TYPE> &,
    const pcomn::shared_intrusive_ptr<TYPE> &,
    pcomn::shared_intrusive_ptr<TYPE> *self

    "$result = $1.get() ;"

%typemap(csout, excode=SWIGEXCODE)
    pcomn::shared_intrusive_ptr<TYPE>,
    pcomn::shared_intrusive_ptr<TYPE> &,
    const pcomn::shared_intrusive_ptr<TYPE> &,
    pcomn::shared_intrusive_ptr<TYPE> *self
{
     IntPtr cPtr = $imcall ;
     $typemap(cstype, TYPE) ret = (cPtr == IntPtr.Zero) ? null : new $typemap(cstype, TYPE)(cPtr, true);$excode
     return ret ;
}
%template(TYPEDEF) pcomn::shared_intrusive_ptr<TYPE> ;
%enddef // %pcomn_shared_intrusive_ptr
