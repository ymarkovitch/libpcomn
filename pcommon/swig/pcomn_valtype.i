/*-*- mode:swig;tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_valtype.i
 COPYRIGHT    :   Yakov Markovitch, 2015
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   VALUE_TYPE SWIG typemaps

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   5 Nov 2015
*******************************************************************************/
%include "pcomn_macros.swg"

/*******************************************************************************
 %value_class
*******************************************************************************/
%define %value_class(Class)
%apply VALUE_TYPE         { Class } ;
%apply VALUE_TYPE &       { Class & } ;
%apply VALUE_TYPE const & { Class const & } ;
%apply VALUE_TYPE *       { Class * } ;
%apply VALUE_TYPE const * { Class const * } ;
%ignore Class::Class() ;
%ignore Class::~Class() ;
%exception Class::Class %{
   #define new new(&_global_result)
   try { $action }
   #undef new
   catch(const std::exception &) { SWIG_StdCSharpException() ; return $null ; }
%}
%enddef

/*******************************************************************************
 VALUE_TYPE
*******************************************************************************/
%fragment("PCOMN_placement_new", "header") {
#include <new>
#include <type_traits>
}

%typemap(ctype) VALUE_TYPE,
                VALUE_TYPE *, const VALUE_TYPE *,
                VALUE_TYPE &, const VALUE_TYPE &
   "uint64_t"

%typemap(imtype) VALUE_TYPE,
                 VALUE_TYPE *, const VALUE_TYPE *,
                 VALUE_TYPE &, const VALUE_TYPE &
   "ulong"

%typemap(in) VALUE_TYPE
     %{ $1 = *static_cast<const $&1_type>(static_cast<const void *>(&$input)) ; %}

%typemap(in) VALUE_TYPE &, VALUE_TYPE const &
     %{ $1 = static_cast<$1_ltype>(const_cast<void *>((static_cast<const void *>(&$input)))) ; %}

%typemap(in) VALUE_TYPE *, VALUE_TYPE const *
     %{ $1 = static_cast<$1_ltype>(static_cast<void *>(&$input)) ; %}

%typemap(out, fragment="PCOMN_placement_new") VALUE_TYPE
     %{ $result = *static_cast<const $typemap(ctype, VALUE_TYPE) *>(static_cast<const void *>(&$1)) ; %}

%typemap(out, fragment="PCOMN_placement_new") VALUE_TYPE * (uint64_t _global_result)
      %{ $result = *static_cast<const $typemap(ctype, VALUE_TYPE) *>(static_cast<const void *>($1)) ; (void)_global_result ; %}

%typemap(out) VALUE_TYPE const *, VALUE_TYPE &, VALUE_TYPE const & = VALUE_TYPE * ;

/*******************************************************************************
 C# typemaps for value types

 Use getCValue instead of getCPtr everywhere.
 There is no such thing as $owner, value types are immutable and only passed/returned
 by value.
*******************************************************************************/
%typemap(csclassmodifiers) VALUE_TYPE "public struct"

%typemap(csinterfaces)     VALUE_TYPE ""

%pragma(csharp) imclasscode = %{
  internal sealed class TagClass {}
  static internal readonly TagClass Tag = null ;
%}

%typemap(csconstruct, excode=SWIGEXCODE) VALUE_TYPE %{: this($imcall, $imclassname.Tag) {$excode
  }
%}

%typemap(csin) VALUE_TYPE,
               const VALUE_TYPE *,
               VALUE_TYPE *,
               const VALUE_TYPE &,
               VALUE_TYPE &
   "$csclassname.getCValue($csinput)" ;

%typemap(csout, excode=SWIGEXCODE) VALUE_TYPE,
                                   const VALUE_TYPE *,
                                   VALUE_TYPE *,
                                   const VALUE_TYPE &,
                                   VALUE_TYPE &
{
    $csclassname ret = new $csclassname($imcall, $imclassname.Tag) ; $excode
    return ret;
}

%typemap(csvarout, excode=SWIGEXCODE2) VALUE_TYPE,
                                       const VALUE_TYPE *,
                                       VALUE_TYPE *,
                                       const VALUE_TYPE &,
                                       VALUE_TYPE &
%{
    get {
      $csclassname ret = new $csclassname($imcall, $imclassname.Tag) ; $excode
      return ret;
    } %}

%typemap(csbody) VALUE_TYPE %{

  private $typemap(imtype, VALUE_TYPE) cValue ;
  private $typemap(imtype, VALUE_TYPE) swigCPtr { get { return cValue ; } }

  internal $csclassname($typemap(imtype, VALUE_TYPE) cValue, $imclassname.TagClass tag) { this.cValue = cValue ; }

  internal static $typemap(imtype, VALUE_TYPE) getCValue($csclassname obj) { return obj.cValue ; }
%}

// Dummy typemap to fool C# language module into that we _have_ a
// correct destructor method (it insists on that)
%typemap(csdestruct) VALUE_TYPE "" ;
%typemap(csfinalize) VALUE_TYPE "" ;

/*******************************************************************************
 Missing typemaps: if ever used, ensure compilation error rather than runtime bug
*******************************************************************************/
// Cannot derive from value class
%typemap(csbody_derived) VALUE_TYPE %{
#error "Deriving $csclassname from value type not available"
%}

/*******************************************************************************

*******************************************************************************/
%{
#include <new>
#include <new>
#include <type_traits>

#ifdef _MSC_VER
#include <malloc.h>
#define SWIG_alloca(sz) (_alloca((sz)))
#else
#include <alloca.h>
#define SWIG_alloca(sz) (alloca((sz)))
#endif

static const nullptr_t _global_result ;

namespace {

template<typename M>
struct placement_ref_allocator {
    template<typename T>
    static T *clone(M *buf, T &&data)
    {
        typedef std::remove_cv_t<std::remove_reference_t<T>> result_value_t ;

        static_assert(std::is_trivial<M>::value, "Use of nontrivial type for placement buffer") ;
        static_assert(sizeof(M) >= sizeof(result_value_t), "Size and/or alignment of a buffer is inappropriate") ;

        return new (static_cast<void *>(&buf)) result_value_t(std::forward<T>(data)) ;
    }
} ;

template<>
struct placement_ref_allocator<const nullptr_t> {
    template<typename T>
    static T *clone(const nullptr_t *, T &&data)
    {
        typedef std::remove_cv_t<std::remove_reference_t<T>> result_value_t ;
        static_assert(!std::is_scalar<result_value_t>::value, "Dynamic allocation of a scalar type object") ;

        return new result_value_t(std::forward<T>(data)) ;
    }
} ;
}

template<typename T, typename M>
static inline T &placement_ref(M &buf, T &&data, std::true_type)
{
    return *placement_ref_allocator<M>::clone(&buf, std::forward<T>(data)) ;
}

template<typename T, typename M>
static inline T &placement_ref(M &, T &&data, std::false_type) { return data ; }

template<typename T, typename M>
static inline T &placement_ref(M &buf, T &&data)
{
    using namespace std ;
    return
        placement_ref(buf, std::forward<T>(data),
                      integral_constant<bool,
                      is_rvalue_reference<decltype(data)>::value &&
                      !is_scalar<remove_cv_t<remove_reference_t<T>>>::value>()) ;
}

template<typename T>
static inline std::remove_cv_t<std::remove_reference_t<T>> *clone_copy(T &&value)
{
    typedef std::remove_cv_t<std::remove_reference_t<T>> result_t ;
    return new result_t(std::forward<T>(value)) ;
}
%}
