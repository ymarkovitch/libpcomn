/*-*- mode:swig;tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_property.swg
 COPYRIGHT    :   Yakov Markovitch, 2015
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Property implementation

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   27 Oct 2015
*******************************************************************************/
%include <attribute.i>
%include "pcomn_valtype.i"

// Define PropClass__ macro with a dummy argument to workaround SWIG preprocessor wart:
// SWIG interprets function-like macro w/o arguments as a simple variable-like macro!
// I.e.
// #define foo() bar
// foo() foo
// will result to
// bar bar
//
%define %properties(Class)
#undef PropClass__
#define PropClass__(dummy) Class
%enddef

%define %endproperties()
#undef PropClass__
%enddef

%define %define_gs_etter__(Class, PropName, Op)%#define %cat(%mangle(Class), _ ## PropName, Op) %enddef

%define %property__(PropType, PropName, Getter, Setter, GetterCall, SetterCall, writable...)
#if #writable == ""
%immutable  PropName ;
%naturalvar PropName ;
#else
%{
%define_gs_etter__(PropClass__(1), PropName, _set(self_, val_)) (SetterCall)
%}
#endif
%{
%define_gs_etter__(PropClass__(1), PropName, _get(self_)) (placement_ref(_global_result, GetterCall))
%}
%extend { PropType PropName ; }
%enddef

%define %property(PropType, PropName, Getter, Setter...)
%newobject PropName ;
%property__(%arg(PropType), PropName, Getter, Setter, self_->Getter(), self_->Setter(val_), Setter)
%enddef

%define %propertyref(PropType, PropName, Getter, Setter...)
%property__(%arg(PropType), PropName, Getter, Setter, &self_->Getter(), self_->Setter(*val_), Setter)
%enddef

%define %propertyclone(PropType, PropName, Getter, Setter...)
#if #Setter == ""
%immutable PropName ;
#endif
%newobject PropName ;
%extend { %arg(PropType) PropName ; }
#if #Setter != ""
%{
%define_gs_etter__(PropClass__(1), PropName, _set(self_, val_)) (self_->Setter(*val_))
%}
#endif
%{
%define_gs_etter__(PropClass__(1), PropName, _get(self_)) (*clone_copy(self_->Getter()))
%}
%enddef

%define %propertystring(PropType, PropName, Getter, Setter...)
#if #Setter != ""
%{
%define_gs_etter__(PropClass__(1), PropName, _set(self_, val_)) (self_->Setter(val_))
%}
#else
%immutable PropName ;
#endif
%{
%define_gs_etter__(PropClass__(1), PropName, _get(self_)) (*new std::string(self_->Getter()))
%}
%newobject PropName ;
%typemap(newfree) const PropType &PropName "delete $1;"
%extend { PropType PropName ; }
%enddef
