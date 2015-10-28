/*-*- mode:swig;tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_property.swg
 COPYRIGHT    :   Yakov Markovitch, 2015
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Property implementation

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   27 Oct 2015
*******************************************************************************/
%include attribute.i
%include pcomn_macros.swg

%{
#include <type_traits>

template<typename T>
static inline std::remove_const_t<T> *clone_copy(T &&value)
{
    typedef std::remove_const_t<T> result_t ;
    return new result_t(std::forward<T>(value)) ;
}
%}

%define %properties(classname) %define PropClass__ classname %enddef %enddef

%define %define_gs_etter__(Class, PropName, Op)%#define %cat(%mangle(Class), _ ## PropName, Op) %enddef

%define %property__(PropType, PropName, Getter, Setter, GetterCall, SetterCall, writable...)
#if #writable == ""
%immutable PropName ;
#else
%{
%define_gs_etter__(PropClass__, PropName, _set(self_, val_)) (SetterCall)
%}
#endif
%{
%define_gs_etter__(PropClass__, PropName, _get(self_)) (GetterCall)
%}
%extend { PropType PropName ; }
%enddef

%define %property(PropType, PropName, Getter, Setter...)
%property__(%arg(PropType), PropName, Getter, Setter, self_->Getter(), self_->Setter(val_), Setter)
%enddef

%define %property2(PropType, PropName, Getter, Setter...)
%property__(%arg(PropType), PropName, Getter, Setter, &self_->Getter(), self_->Setter(*val_), Setter)
%enddef

%define %propertyval(PropType, PropName, Getter, Setter...)
#if #Setter == ""
%immutable PropName;
#endif
%newobject PropName;
%extend { %arg(PropType) PropName; }
#if #Setter != ""
%{
%define_gs_etter__(PropClass__, PropName, _set(self_, val_)) (self_->Setter(*val_))
%}
#endif
%{
%define_gs_etter__(PropClass__, PropName, _get(self_)) (*clone_copy(self_->Getter()))
%}
%enddef

%define %propertystring(PropType, PropName, Getter, Setter...)
#if #Setter != ""
%{
%define_gs_etter__(PropClass__, PropName, _set(self_, val_)) (self_->Setter(val_))
%}
#else
%immutable PropName;
#endif
%{
%define_gs_etter__(PropClass__, PropName, _get(self_)) (clone_copy(self_->Getter()))
%}
%newobject PropName;
%typemap(newfree) const PropType &PropName "delete $1;"
%extend { PropType PropName; }
%enddef

%define %propertyref(PropType, PropName, AccessorMethod...)
#if #AccessorMethod != ""
%property__(%arg(PropType), PropName, AccessorMethod, AccessorMethod, self_->AccessorMethod(), self_->AccessorMethod() = val_, true)
#else
%property__(%arg(PropType), PropName, PropName, PropName, self_->PropName(), self_->PropName() = val_, true)
#endif
%enddef

%define %property2ref(PropType, PropName, AccessorMethod...)
#if #AccessorMethod != ""
%property__(%arg(PropType), PropName, AccessorMethod, AccessorMethod, &self_->AccessorMethod(), self_->AccessorMethod() = *val_)
#else
%property__(%arg(PropType), AccessorName, AccessorName, AccessorName, &self_->AccessorName(), self_->AccessorName() = *val_)
#endif
%enddef
