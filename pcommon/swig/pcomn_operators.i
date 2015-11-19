/*-*- mode:swig;tab-width:2;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_operators.i
 COPYRIGHT    :   Yakov Markovitch, 2015
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   SWIG library for mapping C++ operator members to C# operators

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   9 Nov 2015
*******************************************************************************/
%include "pcomn_macros.swg"
%include "pcomn_property.swg"

%csmethodmodifiers operator bool() const "private" ;
%rename(boolCast) operator bool() const ;
%csattributes operator bool() const %{
  public static explicit operator bool($csclassname self) { return self.boolCast() ; }
  public static bool operator !($csclassname self) { return !(bool)self ; }
  public static bool operator true($csclassname self) { return (bool)self ; }
  public static bool operator false($csclassname self) { return !self ; }
%}
