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
 A smart reference with value semantics, like std::reference_ptr crossed with
 std::unique_ptr, with shared default value.

 Has an extra non-type template parameter of type T, which specifies a reference
 to the *global default value*, pointer to which is used instead of nullptr.
 Default-constructed objects of the instance of this template refer to this
 default object.

 For any two different unique_value<T> objects x and y, the following holds:

 @code
 &x.get() != &y.get() || &x.get() == unique_value<T>::unique_value_ptr()
 @endcode

 That is, every unique_value<T> refers either to the single global constant default
 object of type T, or to a *unique* T object allocated with operator new; no two
 different unique_value<T> objects can refer to the same *non-default* T object.

 @code
 struct Foo {
     Foo() ;
     int bar() const ;
     void set_bar(int) ;
     friend bool operator==(const Foo&,const Foo&) ;
 } ;

 // Default constructor: after this there will be
 // &foo0.get()==&pcomn::default_constructed<Foo>::value
 // No Foo constructors called, no dynamic allocations, foo0 is an unowning reference
 // to &pcomn::default_constructed<Foo>::value.
 pcomn::unique_value<Foo> foo0 ;

 // &foo1.get()==&foo0.get(), both foo0 and foo1 refer to pcomn::default_constructed<Foo>::value
 pcomn::unique_value<Foo> foo1 ;

 // Note that
 // foo2.get()==pcomn::default_constructed<Foo>::value, but
 // &foo2.get()!=&pcomn::default_constructed<Foo>::value
 pcomn::unique_value<Foo> foo2 (Foo()) ;

 // The underlying object of foo2 is copied by value.
 // Now foo1.get(), foo2.get(), and pcomn::default_constructed<Foo>::value are
 // different objects.
 foo1 = foo2 ;

 @endcode
*******************************************************************************/
template<typename T, const T &default_value = pcomn::default_constructed<T>::value>
class unique_value {
   public:
      PCOMN_STATIC_CHECK(!std::is_reference<T>() && !std::is_void<T>()&& !std::is_array<T>()) ;

      typedef const T type ; /**< For compatibility with std::reference_wrapper */
      typedef type element_type ;

      /// Create a reference to default_value.
      /// After this constructor, `&this->get()==default_value_ptr()`
      constexpr unique_value() noexcept = default ;

      /// Create an owning reference to the copy of the argument.
      /// This is "by value" constructor that calls T copy constructor, *except* for the
      /// case when `value` is the  reference to `default_value`.
      ///
      unique_value(const T &value) :
         _owner(is_default(&value) ? default_value_unsafe_ptr() : new T(value))
      {}

      /// Value-move constructor.
      /// Calls T move constructor, *except* for the case when `value` is the  reference
      /// to `default_value`.
      ///
      unique_value(T &&value) :
         _owner(is_default(&value) ? default_value_unsafe_ptr() : new T(std::move(value)))
      {}

      unique_value(const unique_value &other) : unique_value(other.get()) {}

      /// Move constructor.
      /// No allocations, no T copy/move constructor calls.
      /// After this constructor `other` always refers to default_value.
      ///
      unique_value(unique_value &&other) noexcept : unique_value()
      {
         _owner.swap(other._owner) ;
      }

      /// Make a safe reference that owns the passed object.
      ///
      /// @throw std::invalid_argument if `owned_object` is nullptr.
      unique_value(std::unique_ptr<T> &&value) :
         _owner(std::move(PCOMN_ENSURE_ARG(value)))
      {}

      /// Destructor deletes the object referred to *iff* the internal reference does not
      /// refer to `default_value`.
      ~unique_value()
      {
         discharge_default_reference() ;
      }

      unique_value &operator=(unique_value &&other) noexcept
      {
         if (_owner == other._owner)
            return *this ;

         if (other.is_default())
            _owner = std::unique_ptr<T>(default_value_unsafe_ptr()) ;
         else
            _owner.swap(other._owner) ;

         return *this ;
      }

      unique_value &operator=(const unique_value &other)
      {
         if (_owner == other._owner)
            return *this ;

         std::unique_ptr<T> value_copy (other.is_default() ? default_value_unsafe_ptr() : new T(other.get())) ;

         discharge_default_reference() ;
         _owner = std::move(value_copy) ;

         return *this ;
      }

      void swap(unique_value &other) noexcept
      {
         _owner.swap(other._owner) ;
      }

      constexpr const T &get() const noexcept { return *_owner ; }
      constexpr operator const T&() const noexcept { return get() ; }
      constexpr const T *operator->() const noexcept { return _owner.get() ; }

      /// Get a nonconstant reference to the underlying value, doing copy-on-write if the
      /// internal reference is to `default_value`.
      ///
      T &mutable_value()
      {
         PCOMN_STATIC_CHECK(!std::is_const_v<T>) ;

         if (is_default())
         {
            _owner.release() ;
            _owner.reset(new T(*default_value_ptr())) ;
         }
         return *_owner ;
      }

      /// Get the pointer to the (global) default value.
      /// This is a pointer to `default_value` template parameter.
      /// @note Never nullptr.
      ///
      static constexpr const T *default_value_ptr() noexcept { return &default_value ; }

   private:
      std::unique_ptr<T> _owner {default_value_unsafe_ptr()} ;

   private:
      static constexpr T *default_value_unsafe_ptr() noexcept { return const_cast<T*>(default_value_ptr()) ; }

      static constexpr bool is_default(type *value) noexcept { return value == default_value_unsafe_ptr() ; }
      constexpr bool is_default() const noexcept { return is_default(_owner.get()) ; }

      void discharge_default_reference() noexcept
      {
         if (is_default())
            _owner.release() ;
      }
} ;

PCOMN_DEFINE_SWAP(unique_value<T>, template<typename T, const T &>) ;

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

template<typename T>
struct is_trivially_swappable<unique_value<T>> : std::true_type {} ;

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
