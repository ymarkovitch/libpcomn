/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_SAFEPTR_H
#define __PCOMN_SAFEPTR_H
/*******************************************************************************
 FILE         :   pcomn_safeptr.h
 COPYRIGHT    :   Yakov Markovitch, 1994-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Safe pointer templates

 CREATION DATE:   25 Nov 1994
*******************************************************************************/
#include <pcommon.h>
#include <stdexcept>

#include <utility>
#include <memory>

namespace pcomn {

/******************************************************************************/
/** Proxy object for any pointer-like or pointer type

 Useful as a function parameter object where serves as a "shim" converting any
 pointer-like objects to plain pointers
*******************************************************************************/
template<typename T>
struct ptr_shim {
      constexpr ptr_shim(nullptr_t) : _ptr(nullptr) {}

      template<typename P>
      constexpr ptr_shim(const P &p,
                         typename std::enable_if<std::is_convertible<decltype(&*p), T *>::value, Instantiate>::type = Instance) :
         _ptr(&*p) {}

      constexpr T *get() { return _ptr ; }
      constexpr operator T *() { return get() ; }

      constexpr T &operator*() { return *get() ; }
      constexpr T *operator->() { return get() ; }

      constexpr T &operator[](ptrdiff_t ndx) { return _ptr[ndx] ; }

   private:
      T *_ptr ;
} ;

/******************************************************************************/
/** Reference wrapper object, which may, depending on construction mode, delete the
 object whose reference it holds.
*******************************************************************************/
template<typename T>
class safe_ref {
   public:
      typedef T type ; /**< For compatibility with std::reference_wrapper */
      typedef T element_type ;

      /// Construct a safe reference that @e owns the passed object.
      /// @throw std::invalid_argument if @a owned_object is nullptr.
      safe_ref(type *owned_object) :
         _owner(ensure_arg<std::invalid_argument>(owned_object, "owned_object", __FUNCTION__)),
         _ref(*owned_object)
      {}

      /// Construct a safe reference that dows @e not own the passed object.
      safe_ref(type &unowned_object) : _ref(unowned_object) {}

      T &get() const { return _ref ; }
      operator T&() const { return get() ; }
      T* operator->() const { return get() ; }

   private:
      std::unique_ptr<type>   _owner ;
      type &                  _ref ;
} ;

/******************************************************************************/
/** Safepointer for malloc-allocated objects
*******************************************************************************/
template<typename T>
using malloc_ptr = std::unique_ptr<T, malloc_delete> ;

/******************************************************************************/
/** unique_ptr creation helper

 Don't confuse with C++14 make_unique<T>(), which calls T's constructor forwarding
 its arguments.
*******************************************************************************/
template<typename T>
inline std::unique_ptr<T> make_unique_ptr(T *p) { return std::unique_ptr<T>(p) ; }

/*******************************************************************************
 Backward compatibility declaration
*******************************************************************************/
template<typename T, typename D = std::default_delete<T> >
using PTSafePtr = std::unique_ptr<T, D> ;

template<typename T, typename D = std::default_delete<T[]>>
using PTVSafePtr = std::unique_ptr<T[]> ;

template<typename T>
using PTMallocPtr = std::unique_ptr<T, malloc_delete> ;

} // end of namespace pcomn

#endif /* __PCOMN_SAFEPTR_H */
