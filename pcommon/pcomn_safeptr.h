/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_SAFEPTR_H
#define __PCOMN_SAFEPTR_H
/*******************************************************************************
 FILE         :   pcomn_safeptr.h
 COPYRIGHT    :   Yakov Markovitch, 1994-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Safe pointer templates
 CREATION DATE:   25 Nov 1994
*******************************************************************************/
/** @file
 Safe pointers.

  - ptr_shim
  - safe_ref
  - safe_ptr
  - malloc_ptr
*******************************************************************************/
#include <pcomn_meta.h>
#include <pcommon.h>

#include <stdexcept>
#include <utility>
#include <memory>

namespace pcomn {

/***************************************************************************//**
 Safepointer for malloc-allocated objects
*******************************************************************************/
template<typename T>
using malloc_ptr = std::unique_ptr<T, malloc_delete> ;

/***************************************************************************//**
 Proxy object for any pointer-like or pointer type

 Useful as a function parameter object where serves as a "shim" converting any
 pointer-like objects to plain pointers
*******************************************************************************/
template<typename T>
struct ptr_shim {
      constexpr ptr_shim(nullptr_t) : _ptr(nullptr) {}

      template<typename P>
      constexpr ptr_shim(const P &p,
                         typename std::enable_if<std::is_convertible<decltype(&*p), T *>::value, Instantiate>::type = {}) :
         _ptr(&*p) {}

      constexpr T *get() const { return _ptr ; }
      constexpr operator T *() const { return get() ; }

      constexpr T &operator*() const { return *get() ; }
      constexpr T *operator->() const { return get() ; }

      constexpr T &operator[](ptrdiff_t ndx) const { return _ptr[ndx] ; }

   private:
      T *_ptr ;
} ;

/***************************************************************************//**
 Reference wrapper object, which may, depending on construction mode, delete the
 object whose reference it holds.

 Have two modes: owning and unowning.
*******************************************************************************/
template<typename T>
class safe_ref {
      PCOMN_NONCOPYABLE(safe_ref) ;
      PCOMN_NONASSIGNABLE(safe_ref) ;
   public:
      typedef T type ; /**< For compatibility with std::reference_wrapper */
      typedef T element_type ;

      /// Construct a safe reference that dows @e not own the passed object.
      constexpr safe_ref(element_type &unowned_object) : _ref(unowned_object) {}

      /// Construct a safe reference that @e owns the passed object.
      /// @throw std::invalid_argument if @a owned_object is nullptr.
      safe_ref(std::unique_ptr<element_type> &&owned_object) :
         _owner(std::move(PCOMN_ENSURE_ARG(owned_object))),
         _ref(*_owner)
      {}

      /// Allow runtime selection of object ownership.
      /// Either unowned or owned must be nonnull.
      safe_ref(element_type *unowned, std::unique_ptr<element_type> &&owned) :
         _owner(std::move(owned)),
         _ref(_owner ? *_owner : *PCOMN_ENSURE_ARG(unowned))
      {}

      constexpr T &get() const { return _ref ; }
      constexpr operator T&() const { return get() ; }
      constexpr T *operator->() const { return &get() ; }

      constexpr bool owns() const { return !!_owner ; }

   private:
      const std::unique_ptr<type>   _owner ;
      type &                        _ref ;
} ;

/***************************************************************************//**
 Reference wrapper object
*******************************************************************************/
template<typename T, const T &default_value = pcomn::default_constructed<T>::value>
class unique_value {
   public:
      PCOMN_STATIC_CHECK(!std::is_reference<T>() && !std::is_void<T>()&& !std::is_array<T>()) ;

      typedef const T type ; /**< For compatibility with std::reference_wrapper */
      typedef type element_type ;

      unique_value() ;

      unique_value(const T &value) :
         _owner(is_default(&value) ? default_value_unsafe_ptr() : new T(value))
      {}

      unique_value(T &&value) ;

      unique_value(unique_value &&other) ;
      unique_value(const unique_value &other) ;

      /// Make a safe reference that owns the passed object.
      /// @throw std::invalid_argument if `owned_object` is nullptr.
      unique_value(std::unique_ptr<T> &&value) :
         _owner(std::move(PCOMN_ENSURE_ARG(value)))
      {}

      ~unique_value()
      {
         if (is_default())
            _owner.release() ;
      }

      unique_value &operator=(unique_value &&other) ;
      unique_value &operator=(const unique_value &other) ;

      constexpr const T &get() const { return *_owner ; }
      constexpr operator const T&() const { return get() ; }
      constexpr const T *operator->() const { return _owner.get() ; }

   private:
      std::unique_ptr<T> _owner ;

   private:
      static T *default_value_unsafe_ptr() { return const_cast<T*>(&default_constructed) ; }

      static constexpr bool is_default(type *value) { return value == &default_constructed ; }
      bool is_default() const { return is_default(_owner.get()) ; }

      void release_reference()
      {
         if (is_default())
            return ;
         _owner.release() ;
         _owner.reset(const_cast<T*>(&default_value)) ;
      }
} ;


/***************************************************************************//**
 Safe pointer like std::unique_ptr but with *no* move semantics.
*******************************************************************************/
template<typename T>
class safe_ptr : private std::unique_ptr<T> {
      typedef std::unique_ptr<T> ancestor ;
      PCOMN_NONCOPYABLE(safe_ptr) ;
      PCOMN_NONASSIGNABLE(safe_ptr) ;
   public:
      using typename ancestor::element_type ;
      using typename ancestor::pointer ;

      using ancestor::release ;
      using ancestor::get ;

      constexpr safe_ptr() = default ;
      constexpr safe_ptr(nullptr_t) {}
      explicit constexpr safe_ptr(pointer p) : ancestor(p) {}

      typename std::add_lvalue_reference<element_type>::type operator*() const { return *get() ; }
      typename std::add_lvalue_reference<element_type>::type operator[](size_t i) const { return get()[i] ; }
      pointer operator->() const { return get() ; }
      operator pointer() const { return get() ; }

      constexpr explicit operator bool() const noexcept { return ancestor::operator bool() ; }

      safe_ptr &reset(pointer p)
      {
         ancestor::reset(p) ;
         return *this ;
      }
      safe_ptr &reset(nullptr_t = nullptr)
      {
         ancestor::reset() ;
         return *this ;
      }

      safe_ptr &operator=(nullptr_t) { return reset() ; }

      void swap(safe_ptr &other)
      {
         ancestor::swap(*static_cast<ancestor *>(&other)) ;
      }

      template<typename C>
      friend std::basic_ostream<C> &operator<<(std::basic_ostream<C> &os, const safe_ptr &p)
      {
         return os << static_cast<const safe_ptr::ancestor &>(p) ;
      }
} ;

PCOMN_DEFINE_SWAP(safe_ptr<T>, template<typename T>) ;

/***************************************************************************//**
 Both std::unique_ptr and pcomn::safe_ptr are trivially swappable, but only with
 the standard deleter object.
*******************************************************************************/
template<typename T>
struct is_trivially_swappable<std::unique_ptr<T>> : std::true_type {} ;

template<typename T>
struct is_trivially_swappable<safe_ptr<T>> : std::true_type {} ;

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
