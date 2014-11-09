/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_UTILS_H
#define __PCOMN_UTILS_H
/*******************************************************************************
 FILE         :   pcomn_utils.h
 COPYRIGHT    :   Yakov Markovitch, 1994-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Various supplemental functions & templates

 CREATION DATE:   14 Dec 1994
*******************************************************************************/
/** @file
    Various supplemental functions & templates
*******************************************************************************/
#include <pcommon.h>
#include <pcomn_assert.h>
#include <pcomn_omanip.h>
#include <pcomn_meta.h>

#include <iostream>
#include <utility>
#include <functional>
#include <vector>
#include <typeinfo>
#include <memory>

#include <stddef.h>

namespace pcomn {

template<typename T, size_t n>
struct static_buf {
      T *data() { return _data ; }
      const T *data() const { return _data ; }
      size_t size() const { return n ; }

      T _data[n] ;
} ;

/******************************************************************************/
/** This template class is intended to save an old variable value before it is
 changed and automatically restore upon exiting from the block.
*******************************************************************************/
template<typename T>
class vsaver {
      PCOMN_NONCOPYABLE(vsaver) ;
   public:
      explicit vsaver(T &variable) :
         _saved(variable),
         _var(&variable)
      {}

      vsaver(T &variable, const T &new_value) :
         _saved(variable),
         _var(&variable)
      {
         variable = new_value ;
      }

      ~vsaver()
      {
         if (_var) *_var = _saved ;
      }

      const T &release()
      {
         _var = NULL ;
         return _saved ;
      }

      const T &restore()
      {
         if (_var)
         {
            *_var = _saved ;
            _var = NULL ;
         }
         return _saved ;
      }

      const T &saved() const { return _saved ; }

   private:
      const T  _saved ;
      T *      _var ;
} ;

/******************************************************************************/
/** This template class is intended to temporarily set a bit mask and then automatically
 restore the saved settings in the destructor.
*******************************************************************************/
template<typename T>
class bitsaver {
      PCOMN_NONCOPYABLE(bitsaver) ;
   public:
      bitsaver(T &flags, T mask) :
        _flags(flags),
        _mask(mask),
        _status(flags & mask)
      {}

      ~bitsaver()
      {
         (_flags &= ~_mask) |= _status ;
      }

   private:
      T &      _flags ;
      const T  _mask ;
      const T  _status ;
} ;

/*******************************************************************************
                 template<class T, typename R>
                 class finalizer
*******************************************************************************/
template<class T>
class finalizer {
      PCOMN_NONCOPYABLE(finalizer) ;
      PCOMN_NONASSIGNABLE(finalizer) ;
   public:
      typedef std::function<void (T*)> finalizer_type ;

      finalizer(T &var, const finalizer_type &fn) : _var(var), _finalize(fn) {}
      finalizer(T &var, finalizer_type &&fn) : _var(var), _finalize(fn) {}

      ~finalizer() { finalize() ; }

      void release() { _finalize = finalizer_type() ; }

      void finalize()
      {
         if (_finalize)
         {
            _finalize(&_var) ;
            release() ;
         }
      }

   private:
      T & _var ;
      finalizer_type _finalize ;
} ;

template<typename Val, typename Name>
Name valmap_find_name(const std::pair<Name, Val> *valmap, Val value)
{
   while(valmap->first && valmap->second != value)
      ++valmap ;
   return valmap->first ;
}

template<typename Val, typename Name>
inline Name valmap_find_name(const std::pair<Name, Val> *valmap, Val value, const Name &defname)
{
   const Name &found = valmap_find_name(valmap, value) ;
   return found ? found : defname ;
}

template<typename Char, typename Val, typename Name>
const Val *valmap_find_value(const std::pair<Name, Val> *valmap, const Char *name)
{
   typedef std::char_traits<Char> traits ;
   // incrementing name_len allow as to compare real buffers
   // with the last \0 symbol
   size_t name_len = traits::length(name) + 1 ;
   for (; valmap->first ; ++valmap)
      if (!traits::compare(valmap->first, name, name_len))
         return &valmap->second ;
   return NULL ;
}

template<typename Char, typename Val, typename Name>
inline Val valmap_find_value(const std::pair<Name, Val> *valmap, const Char *name, const Val &defval)
{
   const Val * const found = valmap_find_value(valmap, name) ;
   return found ? *found : defval ;
}

/******************************************************************************/
/** type_info wrapper type
*******************************************************************************/
struct typeinfo {
      typeinfo(const std::type_info &info) :
         _info(&info)
      {}

      const std::type_info &info() const { return *_info ; }
      operator const std::type_info &() const { return *_info ; }

      const char *name() const { return info().name() ; }

   private:
      const std::type_info *_info ;
} ;

inline bool operator==(const typeinfo &lhs, const typeinfo &rhs)
{
   return lhs.info() == rhs.info() ;
}
inline bool operator<(const typeinfo &lhs, const typeinfo &rhs)
{
   return lhs.info().before(rhs.info()) ;
}

PCOMN_DEFINE_RELOP_FUNCTIONS(, typeinfo) ;

inline std::ostream &operator<<(std::ostream &os, const typeinfo &v)
{
   return os << PCOMN_DEMANGLE(v.name()) ;
}

/******************************************************************************/
/** Output stream with "embedded" stream buffer (i.e. the memory buffer is a C array -
 the member of of the class).

The template argument defines (fixed) size of the output buffer. This class doesn't make
dynamic allocations.
*******************************************************************************/
template<size_t sz>
class bufstr_ostream : private std::basic_streambuf<char>, public std::ostream {
   public:
      static constexpr size_t BUFSIZE = sz ;

      /// Create the stream with embedded buffer of @a sz bytes.
      bufstr_ostream() throw() ;

      /// Get the pouinter to internal buffer memory
      const char *str() const { return _buffer ; }
      char *str() { return _buffer ; }

      bufstr_ostream &reset()
      {
         clear() ;
         setp(_buffer + 0, _buffer + sizeof _buffer - 1) ;
         return *this ;
      }

   private:
      char _buffer[sz] ;
} ;

template<size_t sz>
bufstr_ostream<sz>::bufstr_ostream() throw() :
   std::ostream(static_cast<std::basic_streambuf<char> *>(this))
{
   *_buffer = _buffer[sizeof _buffer - 1] = 0 ;
   reset() ;
}

template<size_t sz>
constexpr size_t bufstr_ostream<sz>::BUFSIZE ;

/*******************************************************************************
 Utility functions
*******************************************************************************/
template<typename T>
inline T *clone_object(const T *obj)
{
   return obj ? new T(*obj) : (T *)NULL ;
}

/// Delete the scalar object a pointer points to and assign nullptr to the pointer
///
/// This function nullifies the passed pointer @em first and @em then deletes an object
/// the pointer pointed to before; this allows to work with cyclic pointer structures.
///
template<typename T>
inline T *&clear_delete(T *&ptr)
{
   T *temp = ptr ;
   ptr = 0 ;
   delete temp ;
   return ptr ;
}

/// Delete an array of objects a pointer points to and assign nullptr to the pointer
template<typename T>
inline T *&clear_deletev(T *&vec)
{
   T *temp = vec ;
   vec = 0 ;
   delete [] temp ;
   return vec ;
}

template<typename T>
inline int compare_values(const T &t1, const T &t2)
{
   return (t1 < t2) ? -1 : (t2 != t1) ;
}

template<typename T>
inline T &fill_mem(T &t, int value = 0)
{
   return *(T *)memset(&t, value, sizeof t) ;
}

template<class T>
inline const T &assign_by_ptr(T *ptr, const T &value)
{
   if (ptr)
      *ptr = value ;
   return value ;
}

/*******************************************************************************
 void * pointer arithmetics
*******************************************************************************/
template<typename T>
inline T *padd(T *p, ptrdiff_t offset)
{
   return (T *)((char *)p + offset) ;
}

inline ptrdiff_t pdiff(const void *p1, const void *p2)
{
   return (const char *)p1 - (const char *)p2 ;
}

template<typename T>
inline T *&preinc(T *&p, ptrdiff_t offset)
{
   return p = padd(p, offset) ;
}

template<typename T>
inline T *postinc(T *&p, ptrdiff_t offset)
{
   T *old = p ;
   preinc(p, offset) ;
   return old ;
}

template<typename T>
inline T *rebase(T *ptr, const void *oldbase, const void *newbase)
{
    return ptr ? (T *)padd(newbase, pdiff(ptr, oldbase)) : (T *)NULL ;
}

/*******************************************************************************
 Flagset processing functions
*******************************************************************************/
template<typename T>
inline bool is_flags_equal(T flags, T test, T mask)
{
   return !((flags ^ test) & mask) ;
}

template<typename T>
inline bool is_flags_on(T flags, T mask)
{
   return is_flags_equal(flags, mask, mask) ;
}

template<typename T>
inline bool is_flags_off(T flags, T mask)
{
   return is_flags_equal<T>(~flags, mask, mask) ;
}

template<typename T>
inline T &set_flags_on(T &flags, const T &fset)
{
   return flags |= fset ;
}

template<typename T>
inline T &set_flags_off(T &flags, const T &fset)
{
   return flags &= ~fset ;
}

template<typename T>
inline T &set_flags(T &flags, const T &fset, bool onOff)
{
   return (onOff) ?
      set_flags_on(flags, fset) :
      set_flags_off(flags, fset) ;
}

template<typename T>
inline T &set_flags(T &flags, const T &fset, const T &mask)
{
   return (flags &= ~mask) |= fset & mask ;
}

template<typename T>
inline T &inv_flags(T &flags, const T &fset)
{
   return flags ^= fset ;
}

template<typename T>
inline T is_inverted(const T &flag, const T &mask, long test)
{
   return !(flag & mask) ^ !test ;
}

template<typename T>
inline T sign(T val)
{
   return (val < 0) ? -1 : (!!val) ;
}

template<typename T>
inline T flag_if(T flag, bool cond)
{
   return (T)(static_cast<T>(-(long)!!cond) & flag) ;
}

template<typename T, typename U>
inline std::enable_if_t<std::is_convertible<U, T>::value, T>
xchange(T &dest, const U &src)
{
   T old (std::move(dest)) ;
   dest = src ;
   return old ;
}

template <class T>
inline const T& midval(const T& minVal, const T& maxVal, const T& val)
{
   return std::min(maxVal, std::max(minVal, val)) ;
}

template<typename T>
inline bool inrange(const T &value, const T &left, const T &right)
{
   return !(value < left || right < value) ;
}

template<typename T>
inline bool xinrange(const T &value, const T &left, const T &right)
{
   return !(value < left) && value < right ;
}

template<class T>
inline void swapByOrder(T &op1, T &op2)
{
   using std::swap ;
   if (op2 < op1)
      swap(op1, op2) ;
}

template<typename T>
inline ptrdiff_t range_length(const std::pair<T, T> &range)
{
   return range.second - range.first ;
}

template<typename T>
inline bool range_empty(const std::pair<T, T> &range)
{
   return range.second == range.first ;
}

template<typename T, typename R>
inline bool inrange(const T &value, const std::pair<R, R> &range)
{
   return inrange<R>(value, range.first, range.second) ;
}

template<typename T, typename R>
inline bool xinrange(const T &value, const std::pair<R, R> &range)
{
   return xinrange<R>(value, range.first, range.second) ;
}

/*******************************************************************************
*
* Tagged pointer hacks; work only for pointers with alignment > 1
*
*******************************************************************************/
#define static_check_taggable(T) \
   static_assert(alignof(T) > 1, "Taggable pointer element type alignemnt must be at least 2")

/// Make a pointer "tagged", set its LSBit to 1
template<typename T>
inline constexpr T *tag_ptr(T *ptr)
{
   static_check_taggable(T) ;
   return (T *)(reinterpret_cast<uintptr_t>(ptr) | 1) ;
}

/// Untag a pointer, clear its LSBit.
template<typename T>
inline constexpr T *untag_ptr(T *ptr)
{
   static_check_taggable(T) ;
   return (T *)(reinterpret_cast<uintptr_t>(ptr) &~ (uintptr_t)1) ;
}

/// Flip the pointer's LSBit.
template<typename T>
inline constexpr T *fliptag_ptr(T *ptr)
{
   static_check_taggable(T) ;
   return (T *)(reinterpret_cast<uintptr_t>(ptr) ^ 1) ;
}

template<typename T>
inline constexpr bool is_ptr_tagged(T *ptr)
{
   static_check_taggable(T) ;
   return reinterpret_cast<uintptr_t>(ptr) & 1 ;
}

template<typename T>
inline constexpr bool is_ptr_tagged_or_null(T *ptr)
{
   static_check_taggable(T) ;
   return (!(reinterpret_cast<uintptr_t>(ptr) & ~1)) | (reinterpret_cast<uintptr_t>(ptr) & 1) ;
}

/// If a pointer is tagged or NULL, return NULL; otherwise, return the untagged
/// pointer value.
template<typename T>
inline constexpr T *null_if_tagged_or_null(T *ptr)
{
   static_check_taggable(T) ;
   return (T *)(reinterpret_cast<uintptr_t>(ptr) & ((reinterpret_cast<uintptr_t>(ptr) & 1) - 1)) ;
}

/// If a pointer is untagged or NULL, return NULL; otherwise, return the untagged
/// pointer value.
template<typename T>
inline constexpr T *null_if_untagged_or_null(T *ptr)
{
   static_check_taggable(T) ;
   return (T *)(reinterpret_cast<uintptr_t>(ptr) & ((~uintptr_t() + ((reinterpret_cast<uintptr_t>(ptr) - 1) & 1)) & ~1)) ;
}

#undef static_check_taggable

/******************************************************************************/
/** Tagged union of two pointers with sizof(tagged_ptr_union)==sizeof(void *).

 The alignment of the memory pointed to by any of union members MUST be at least 2
*******************************************************************************/
template<typename T1, typename T2>
struct tagged_ptr_union_POD {
      typedef T1 *first_type ;
      typedef T2 *second_type ;

      constexpr operator const void *() const { return (const void *)(as_intptr() &~ (intptr_t)1) ; }

      template<int i>
      constexpr typename std::conditional<i, second_type, first_type>::
      type get() const
      {
         return this->get_(std::integral_constant<int, i>()) ;
      }

      template<int i>
      tagged_ptr_union_POD &set(typename std::conditional<i, second_type, first_type>::type v)
      {
         NOXCHECK(!((uintptr_t)v & 1)) ;
         this->set_(v, std::integral_constant<int, i>()) ;
         return *this ;
      }

      void reset() { _first = NULL ; }

   private:
      union {
            first_type  _first ;
            second_type _second ;
      } ;

      constexpr intptr_t as_intptr() const { return (intptr_t)_first ; }
      constexpr intptr_t first_mask() const { return (((intptr_t)_second & 1) - 1) ; }

      constexpr first_type get_(std::integral_constant<int, 0>) const
      {
         return reinterpret_cast<first_type>(as_intptr() & first_mask()) ;
      }
      void set_(first_type v, std::integral_constant<int, 0>) { _first = v ; }

      constexpr second_type get_(std::integral_constant<int, 1>) const
      {
         return reinterpret_cast<second_type>(as_intptr() &~ (intptr_t)1 &~ first_mask()) ;
      }
      void set_(second_type v, std::integral_constant<int, 1>)
      {
         _second = reinterpret_cast<second_type>
            ((((uintptr_t)v) | (uintptr_t)1) ^ (uintptr_t)!v) ;
      }

      static_assert(alignof(first_type) > 1 && alignof(second_type) > 1,
                    "Types pointed to by pcomn::tagged_ptr_union must have alignment at least 2") ;
} ;

/******************************************************************************/
/** Tagged pointer union with a default constructor, which resets it to NULL
*******************************************************************************/
template<typename T1, typename T2>
struct tagged_ptr_union : tagged_ptr_union_POD<T1, T2> {
   private:
      typedef tagged_ptr_union_POD<T1, T2> ancestor ;
   public:
      tagged_ptr_union() { this->reset() ; }
      tagged_ptr_union(const ancestor &src) : ancestor(src) {}
} ;

/*******************************************************************************
 ptr_cast<> - convert any pointerlike object to a pointer
*******************************************************************************/
template<typename Pointer, typename PointerLike>
inline Pointer ptr_cast(const PointerLike &pointer)
{
   return &*const_cast<PointerLike &>(pointer) ;
}

/// Converts its parameter to a constant reference.
/// Useful when it is necessary to pass an rvalue (e.g. a temporary object) to
/// a template function accepting a non-const reference.
/// For instance:
/// template<typename T> void foo(T &t) ;
/// foo(std::string("Hello, world!")) ; // Error, compiler will deduce T as
///                                     // std::string and won't allow passing
///                                     // a temporary by a non-const reference
/// foo(pcomn::vcref(std::string("Hello, world!"))) ; // OK
template<typename T>
inline const T &vcref(const T &value)
{
   return value ;
}

/*******************************************************************************
 ostream output for simple_slice, etc.
*******************************************************************************/
template<typename T> class simple_slice ;

template<typename T>
inline std::ostream &operator<<(std::ostream &os, const simple_slice<T> &v)
{
   return os << pcomn::osequence(v.begin(), v.end(), " ") ;
}

} // end of pcomn namespace


// We need std namespace to ensure proper Koenig lookup in some compilers
// (notably all MS compilers).
namespace std {

template<typename T1, typename T2>
inline std::ostream &operator<<(std::ostream &os, const std::pair<T1,T2> &p)
{
   return os << '{' << p.first << ',' << p.second << '}' ;
}

template<typename T, typename A>
inline std::ostream &operator<<(std::ostream &os, const std::vector<T, A> &v)
{
   return os << pcomn::osequence(v.begin(), v.end(), " ") ;
}

template<typename T, typename D>
inline std::ostream &operator<<(std::ostream &os, const std::unique_ptr<T, D> &v)
{
   return os << v.get() ;
}

} // end of namespace std

#endif /* __PCOMN_UTILS_H */
