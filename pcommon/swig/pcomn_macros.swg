/*-*- mode:swig;tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_macros.swg
 COPYRIGHT    :   Yakov Markovitch, 2004-2015
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Helper SWIG macros used for writing typemaps

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   8 Sep 2004
*******************************************************************************/

#define %str2(Arg...) %str(Arg)
#define %cat(a1, a2, a3...) a1 ## a2 ## a3
#define %cat2(a1, a2, a3...) %cat(a1, a2, a3)

/*******************************************************************************
 Rename objects to avoid the common prefixes and convert the remaining name part
 to camelcase
 HELLO_WORLD -> World
 HELLO_NICE_WORLD -> NiceWorld
*******************************************************************************/
#define %rename_noprefix_camelcase(...) \
    %rename("%(regex:/^([^_]+)_(([^_])([^_]*))((_?)([^_]?)([^_]*))/\\u\\3\\L\\4\\u\\7\\L\\8/)s", ##__VA_ARGS__)

%define %typedef_template(typedef_name, ...)
#if #__VA_ARGS__ == ""
#error Second argument required to %typedef_template, please specify template instance
#endif
%template(typedef_name) __VA_ARGS__ ;
typedef __VA_ARGS__ typedef_name ;
%enddef

%define %typedef_template_inline(typedef_name, ...)
%typedef_template(typedef_name, ##__VA_ARGS__)
%{typedef __VA_ARGS__ typedef_name ;%}
%enddef

/*******************************************************************************
 %mapmacro1(macro_1, arg [, ...]) - call the macro_1 for every argument
 The the macro_1 should be a one-argument macro.
 Thus, calling %mapmacro(m, a, b, c) will produce a sequence of calls
 m(a)
 m(b)
 m(c)

 %mapmacro2(macro_2, arg1, arg2 [, ...])  - call the macro_2 for every pair
                                            of arguments
 The the macro_2 should be a two-argument macro.
 Thus, calling %mapmacro(m, a, b, c, d) will produce a sequence of calls
 m(a, b)
 m(c, d)

 %mapmacro(macro, arg [, ...])  - call the macro for every argument, given
                                  that %mapmacro arguments are tuples of
                                  arguments to macro
 Thus, calling %mapmacro(m, (a), (b, c)) will produce a sequence of calls
 m(a)
 m(b, c)

 Since SWIG macroprocessor can recursively call macros defined using %define
 (as opposed to usual C/C99 #define'd macros), we can create recursive 'loops',
 generating multiple fragments of text (that, in their own turn, can be macro
 calls)
*******************************************************************************/
%define %mapmacro1(macro,...)
#if #__VA_ARGS__ == ""
#else
%_mapmacro1(macro, __VA_ARGS__)
#endif
%enddef

%define %mapmacro2(macro,...)
#if #__VA_ARGS__ == ""
#else
%_mapmacro2(macro, __VA_ARGS__)
#endif
%enddef

%define %mapmacro(macro,...)
#if #__VA_ARGS__ == ""
#else
%_mapmacro(macro, __VA_ARGS__)
#endif
%enddef

%define %_mapmacro1(macro, arg1,...)
macro(arg1)
#if #__VA_ARGS__ == ""
#else
%_mapmacro1(macro, __VA_ARGS__)
#endif
%enddef

%define %_mapmacro2(macro, arg1, arg2, ...)
macro(arg1, arg2)
#if #__VA_ARGS__ == ""
#else
%_mapmacro2(macro, __VA_ARGS__)
#endif
%enddef

%define %_mapmacro(macro, arg1,...)
macro arg1
#if #__VA_ARGS__ == ""
#else
%_mapmacro(macro, __VA_ARGS__)
#endif
%enddef

/*******************************************************************************
 %apply_prefix(prefix, ...)
*******************************************************************************/
%define %apply_prefix(prefix, ...)
#if #__VA_ARGS__ == ""
#else
%_apply_prefix(prefix, __VA_ARGS__)
#endif
%enddef

%define %_apply_prefix(prefix, macro, ...)
prefix##macro
#if #__VA_ARGS__ == ""
#else
%_apply_prefix(prefix, __VA_ARGS__)
#endif
%enddef

/*******************************************************************************
 %setflag(flag)   - define macro name as 1 but ignore it in the interface.
 %resetflag(flag) - undefine macro name
*******************************************************************************/
%define %setflag(flag) %define flag flag %enddef %enddef

%define %resetflag(flag) #undef flag %enddef

%define %setmacro(macro, value)
%resetmacro(macro)
#define macro(arg) value
%enddef

%define %resetmacro(macro) #undef macro %enddef

%define %ToString(funcname)
%csmethodmodifiers funcname "public override" ;
%rename(ToString) funcname ;
std::string funcname() const ;
%enddef
