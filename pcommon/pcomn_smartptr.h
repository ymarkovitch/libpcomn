/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_SMARTPTR_H
#define __PCOMN_SMARTPTR_H
/*******************************************************************************
 FILE         :   pcomn_smartptr.h
 COPYRIGHT    :   Yakov Markovitch, 1994-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Templates of "smart pointers", i.e. pointers with
                  reference counting and automatic object cleanup.

 CREATION DATE:   25 Nov 1994
*******************************************************************************/
#include <pcomn_counter.h>
#include <pcomn_assert.h>
#include <pcomn_meta.h>
#include <pcommon.h>

#include <typeinfo>
#include <algorithm>
#include <memory>
#include <iostream>

#include <stddef.h>

namespace pcomn {

/// Add an additional reference to a refcounted object.
/// @param counted  An object to add a reference to. If @a counted is, NULL the function
/// does nothing.
/// @return Value of @a counted.
template<typename C, intptr_t init>
inline active_counter<C, init> *inc_ref(active_counter<C, init> * __restrict counted) noexcept
{
   counted && counted->inc() ;
   return counted ;
}

template<typename C, intptr_t init>
inline active_counter<C, init> *dec_ref(active_counter<C, init> * __restrict counted) noexcept
{
   counted && counted->dec() ;
   return counted ;
}

template<typename Refcounted>
inline void assign_ref(Refcounted *&target, Refcounted *source) noexcept
{
   if (target != source)
   {
      inc_ref(source) ; /* First we must increment counter of the source object
                         * to avoid possible (indirect) removal of all remaining references
                         * to it as a result of dec_ref() call. */
      dec_ref(target) ;
      target = source ;
   }
}

template<typename Refcounted>
inline void clear_ref(Refcounted *&target) noexcept
{
   if (target)
   {
      dec_ref(target) ;
      target = nullptr ;
   }
}

template<typename T> struct refcount_policy ;
template<typename T> class shared_intrusive_ptr ;

/******************************************************************************/
/** Basic policy for intrusive reference-counted pointers
*******************************************************************************/
template<typename T, typename C, C (T::*counter)>
struct refcount_basic_policy {

      static void threshold_action(T *ptr) noexcept { delete ptr ; }

      static C instances(const T *ptr) noexcept
      {
         NOXCHECK(ptr) ;
         return atomic_op::load(&(ptr->*counter), std::memory_order_consume) ;
      }

      static void inc_ref(T *ptr) noexcept
      {
         NOXCHECK(ptr) ;
         atomic_op::preinc(&(ptr->*counter)) ;
      }

      static void dec_ref(T *ptr) noexcept
      {
         NOXCHECK(ptr) ;
         if (!atomic_op::predec(&(ptr->*counter)))
            refcount_policy<T>::threshold_action(ptr) ;
      }
} ;

/******************************************************************************/
/** Reference counter: the base class for reference counted objects.
*******************************************************************************/
template<typename C = std::atomic<intptr_t>>
class PTRefCounter : public active_counter<C> {
      typedef active_counter<C> ancestor ;
   public:
      using typename ancestor::count_type ;
      typedef refcount_policy<PTRefCounter<C>> refcount_policy_type ;

      /// Alias for instances(), added to match std::shared_ptr interface
      count_type use_count() const noexcept { return instances() ; }

      /// Get current instance counter value.
      count_type instances() const noexcept { return this->count() ; }

   protected:
      /// The default constructor creates object with zero counter.
      PTRefCounter() noexcept = default ;

      /// Copy constructor.
      /// Creates object with zeroed counter, same as default constructor.
      /// In fact, does nothing about the copying.
      PTRefCounter (const PTRefCounter &) noexcept : ancestor() {}

      /// Destructor does nothing, declared for inheritance protection and debugging purposes.
      ~PTRefCounter() override = default ;

      /// Deletes "this" (itself); overrides pure virtual active_counter::dec_action().
      count_type dec_action(count_type threshold) noexcept override
      {
         self_destroy(this) ;
         return threshold ;
      }

      /// Does nothing, provided in order to implement pure virtual function of
      /// active_counter.
      count_type inc_action(count_type threshold) noexcept override
      {
         return threshold ;
      }

   private:
      static void self_destroy(PTRefCounter *object) noexcept { delete object ; }
} ;

typedef PTRefCounter<> PRefCount ;

/***************************************************************************//**
 Intrusive reference-counted pointer to anything based on PRefCount
*******************************************************************************/
typedef shared_intrusive_ptr<PRefCount>       refcounted_ptr ;
typedef shared_intrusive_ptr<const PRefCount> refcounted_cptr ;

/***************************************************************************//**
 Intrusive reference-counted pointer policy for objects based on PTRefCounter
 (or PRefCount)
*******************************************************************************/
template<typename C>
struct refcount_policy<PTRefCounter<C>> {
      typedef typename PTRefCounter<C>::count_type count_type ;

      static count_type instances(const PTRefCounter<C> *counted) noexcept
      {
         NOXCHECK(counted) ;
         return counted->instances() ;
      }
      static void inc_ref(PTRefCounter<C> *counted) noexcept
      {
         NOXCHECK(counted) ;
         counted->inc_passive() ;
      }
      static void dec_ref(PTRefCounter<C> *counted) noexcept
      {
         NOXCHECK(counted) ;
         counted->dec() ;
      }
} ;

/*******************************************************************************

*******************************************************************************/
namespace detail {
template<typename T, typename>
struct refcount_policy_ { typedef typename std::remove_pointer_t<T>::refcount_policy_type type ; } ;
template<typename T, typename U>
struct refcount_policy_<T, refcount_policy<U> *> { typedef refcount_policy<U> type ; } ;

// SFINAE selector for refcount policy
// If refcount_policy<T> is a complete type, use it;
// otherwise, use T::refcount_policy_type
template<typename U>
refcount_policy<std::remove_cv_t<U>> *
eval_refcount_policy(U *, std::void_t<decltype(refcount_policy<std::remove_cv_t<U>>::inc_ref(nullptr))>* = {}) ;
void *eval_refcount_policy(...) ;
}

template<typename U>
using refcount_policy_t = typename detail::refcount_policy_<U, decltype(detail::eval_refcount_policy(autoval<U *>()))>::type ;

/***************************************************************************//**
 Intrusive reference-counted shared pointer.

 Intrusive shared pointer requires the pointed object to provide reference counting
 logic. This can be done in one of the two possible ways:

  - The template parameter T is (directly or indirectly) derived from the PTRefCounter
    instance.
  - There is a specialization of refcount_policy template for the class T.
*******************************************************************************/
template<typename T>
class shared_intrusive_ptr {
   public:
      typedef T element_type ;

      template<typename U>
      friend class shared_intrusive_ptr ;

      constexpr shared_intrusive_ptr() noexcept = default ;
      constexpr shared_intrusive_ptr(nullptr_t) noexcept  {}

      explicit shared_intrusive_ptr(element_type *object) noexcept :
         _object(object)
      { inc_ref() ; }

      /// Constructor to allow copy-list initialization of a shared pointer from the
      /// plain pointer.
      ///
      /// The constructor from element_type* is intentionally made explicit to avoid
      /// unintended automatic conversions from the plain pointer to a shared pointer.
      /// This however hinders copy-list initialization on e.g. returning
      /// shared_intrusive_ptr from functions. For example:
      /// @code
      ///   class Foo { public: virtual ~Foo() ; ... } ;
      ///   class Bar: public Foo { ... } ;
      ///   typedef pcomn::shared_intrusive_ptr<Foo> FooPtr ;
      ///   typedef pcomn::shared_intrusive_ptr<Bar> BarPtr ;
      ///
      ///   FooPtr bar()
      ///   {
      ///      // Cannot `return {new Bar}`, the constructor is explicit.
      ///      // Must either explicitly mention FooPtr or use sptr_cast(new Bar).
      ///      // But sptr_cast(new Bar) prevents RVO, because returns BarPtr,
      ///      // so move constructor will be called.
      ///      // But this will do:
      ///
      ///      return {pcomn::instantiate, new Bar} ;
      ///   }
      /// @endcode
      ///
      shared_intrusive_ptr(Instantiate, element_type *object) noexcept :
         shared_intrusive_ptr(object)
      {}

      shared_intrusive_ptr(const shared_intrusive_ptr &src) noexcept :
         shared_intrusive_ptr(src.get())
      {}

      shared_intrusive_ptr(shared_intrusive_ptr &&src) noexcept :
         shared_intrusive_ptr(src._object, int_constant<42>())
      {}

      template<typename U, typename = instance_if_t<std::is_convertible<U*, element_type*>::value> >
      shared_intrusive_ptr(const shared_intrusive_ptr<U> &src) noexcept :
         shared_intrusive_ptr(src.get())
      {}

      template<typename U, typename = instance_if_t<std::is_convertible<U*, element_type*>::value> >
      shared_intrusive_ptr(shared_intrusive_ptr<U> &&src) noexcept :
         shared_intrusive_ptr(src._object, int_constant<42>())
      {}

      ~shared_intrusive_ptr() noexcept { dec_ref() ; }

      constexpr element_type *get() const noexcept { return _object ; }
      constexpr element_type *operator->() const noexcept { return get() ; }
      constexpr element_type &operator*() const noexcept { return *get() ; }

      constexpr explicit operator bool() const noexcept { return !!get() ; }

      /// Get the number of different intrusive_sptr instances (this included) managing
      /// the current object
      ///
      intptr_t instances() const noexcept
      {
         return _object ? refcount_policy_t<element_type>::instances(_object) : 0 ;
      }

      /// Alias for instances(), added to match std::shared_ptr insterface
      intptr_t use_count() const noexcept { return instances() ; }

      void reset() noexcept
      {
         if (auto *element = as_ptr_mutable(std::exchange<element_type*>(_object, nullptr)))
            refcount_policy_t<element_type>::dec_ref(element) ;
      }

      void reset(std::nullptr_t) noexcept { reset() ; }

      void reset(element_type *other) noexcept
      {
         if (other == _object) return ;

         auto *element = as_ptr_mutable(std::exchange<element_type*>(_object, other)) ;
         inc_ref() ;
         if (element)
            refcount_policy_t<element_type>::dec_ref(element) ;
      }

      void swap(shared_intrusive_ptr &other) noexcept
      {
         std::swap(_object, other._object) ;
      }

      shared_intrusive_ptr &operator=(const shared_intrusive_ptr &other) noexcept
      {
         assign_element(other.get()) ;
         return *this ;
      }

      shared_intrusive_ptr &operator=(element_type *other) noexcept
      {
         assign_element(other) ;
         return *this ;
      }

      template<typename U>
      auto operator=(const shared_intrusive_ptr<U> &other) noexcept
         ->decltype(shared_intrusive_ptr(other.get())) &
      {
         assign_element(other.get()) ;
         return *this ;
      }

      shared_intrusive_ptr &operator=(shared_intrusive_ptr &&other) noexcept
      {
         move_element(other._object) ;
         return *this ;
      }

      template<typename U>
      auto operator=(shared_intrusive_ptr<U> &&other) noexcept
         ->decltype(shared_intrusive_ptr(other.get())) &
      {
         move_element(other._object) ;
         return *this ;
      }

      /// Create an instance of shared_intrusive_ptr whose stored pointer is _moved_ from
      /// this's stored pointer with static_cast'ing.
      /// This is used to implement the "moving" sptr_cast.
      ///
      template<class U>
      shared_intrusive_ptr<transfer_cv_t<element_type, U>> cast_move() noexcept
      {
         typedef transfer_cv_t<element_type, U> result_element ;
         return shared_intrusive_ptr<result_element>(_object, int_constant<42>()) ;
      }

   private:
      element_type *_object = nullptr ;

   private:
      template<typename U>
      shared_intrusive_ptr(U *&src_object, int_constant<42>) noexcept :
         _object(static_cast<element_type *>(src_object))
      {
         if (!std::is_same<refcount_policy_t<element_type>, refcount_policy_t<U>>() && src_object)
         {
            inc_ref() ;
            refcount_policy_t<U>::dec_ref(as_ptr_mutable(src_object)) ;
         }
         src_object = nullptr ;
      }

      void inc_ref() const noexcept { if (_object) refcount_policy_t<element_type>::inc_ref(as_ptr_mutable(_object)) ; }
      void dec_ref() noexcept { if (_object) refcount_policy_t<element_type>::dec_ref(as_ptr_mutable(_object)) ; }

      template<class E>
      void assign_element(E *other_element) noexcept
      {
         typedef refcount_policy_t<E> other_policy_type ;

         auto *other_object = as_ptr_mutable(other_element) ;

         if (_object == other_object) return ;

         if (other_object)
            other_policy_type::inc_ref(other_object) ; /* First we must increment counter of source object
                                                        * due to possibility of (indirect) removal of all
                                                        * remaining references to that one as a result of
                                                        * dec_ref() call. */
         dec_ref() ;
         _object = other_object ;
      }

      template<class E>
      void move_element(E *&other_element) noexcept
      {
         typedef refcount_policy_t<element_type> this_policy_type ;
         typedef refcount_policy_t<E>            other_policy_type ;

         if (!other_element)
         {
            reset() ;
            return ;
         }

         auto *other_object = as_ptr_mutable(other_element) ;
         other_element = nullptr ;
         element_type *old_this_object = std::exchange(_object, static_cast<element_type*>(other_object)) ;

         if (other_object == old_this_object)
         {
            other_policy_type::dec_ref(other_object) ;
            return ;
         }

         if (!std::is_same<this_policy_type, other_policy_type>())
         {
            this_policy_type::inc_ref(as_ptr_mutable(_object)) ;
            other_policy_type::dec_ref(other_object) ;
         }

         if (old_this_object)
            this_policy_type::dec_ref(as_ptr_mutable(old_this_object)) ;
      }
} ;

/// Create a new instance of shared_intrusive_ptr whose stored pointer is obtained from
/// src's stored pointer using a static_cast expression.
///
/// The result is a copy of the source, so the source remains intact and the reference
/// count is incremented.
template<class T, class U>
inline shared_intrusive_ptr<transfer_cv_t<U, T>> sptr_cast(const shared_intrusive_ptr<U> &src) noexcept
{
   typedef transfer_cv_t<U, T> result_element ;
   return shared_intrusive_ptr<result_element>(static_cast<result_element *>(src.get())) ;
}

/// Create an instance of shared_intrusive_ptr whose stored pointer is _moved_ from
/// src's stored pointer with static_cast'ing.
///
/// This is a "moving" variant of sptr_cast(): the source is zeroed, so no reference
/// counters incremented or decremented.
///
template<class T, class U>
inline shared_intrusive_ptr<transfer_cv_t<U, T>> sptr_cast(shared_intrusive_ptr<U> &&src) noexcept
{
   return src.template cast_move<T>() ;
}

template<class T>
inline shared_intrusive_ptr<T> sptr_cast(T *plain_ptr) noexcept
{
   return shared_intrusive_ptr<T>(plain_ptr) ;
}

template<class T, class U>
inline shared_intrusive_ptr<transfer_cv_t<U, T>> sptr_dynamic_cast(const shared_intrusive_ptr<U> &src) noexcept
{
   typedef transfer_cv_t<U, T> result_element ;
   return shared_intrusive_ptr<result_element>(dynamic_cast<result_element *>(src.get())) ;
}

/*******************************************************************************
                     template<class T>
                     class ref_lease
*******************************************************************************/
template<class T>
struct ref_lease  {
      explicit ref_lease(T *guarded) noexcept : _guarded(guarded) { inccount(guarded) ; }
      ~ref_lease() noexcept { deccount(_guarded) ; }

   private:
      T * const _guarded ;

      template<typename C, intptr_t init>
      static void inccount(active_counter<C, init> *counter) noexcept
      {
         if (counter) counter->inc_passive() ;
      }
      template<typename C, intptr_t init>
      static void deccount(active_counter<C, init> *counter) noexcept
      {
         if (counter) counter->dec_passive() ;
      }
      PCOMN_NONCOPYABLE(ref_lease) ;
} ;

/*******************************************************************************
 Operators ==, !=, <
*******************************************************************************/
#define PCOMN_SPTR_RELOP(OP)                                            \
   template<typename T, typename U>                                     \
   inline auto operator OP(const U *x, const shared_intrusive_ptr<T> &y) noexcept \
      ->decltype(x OP y.get())                                          \
   {                                                                    \
      return x OP y.get() ;                                             \
   }                                                                    \
                                                                        \
   template<typename T, typename U>                                     \
   inline auto operator OP(const shared_intrusive_ptr<T> &x, const U *y) noexcept \
      ->decltype(x.get() OP y)                                          \
   {                                                                    \
      return x.get() OP y ;                                             \
   }                                                                    \
   template<typename T>                                                 \
   inline bool operator OP(nullptr_t, const shared_intrusive_ptr<T> &y) noexcept \
   {                                                                    \
      return nullptr OP y.get() ;                                       \
   }                                                                    \
                                                                        \
   template<typename T>                                                 \
   inline bool operator OP(const shared_intrusive_ptr<T> &x, nullptr_t) noexcept \
   {                                                                    \
      return x.get() OP nullptr ;                                       \
   }                                                                    \
                                                                        \
   template<typename T, typename U>                                     \
   inline auto operator OP(const shared_intrusive_ptr<T> &x, const shared_intrusive_ptr<U> &y) noexcept \
      ->decltype(x.get() OP y.get())                                    \
   {                                                                    \
      return x.get() OP y.get() ;                                       \
   }

PCOMN_SPTR_RELOP(==) ;
PCOMN_SPTR_RELOP(!=) ;
PCOMN_SPTR_RELOP(<) ;


/***************************************************************************//**
 Smart reference: template class like a smartpointer that constructs its pointee
 object and thus is never NULL.
*******************************************************************************/
template<typename T>
class shared_ref {
      typedef typename std::remove_cv_t<T> mutable_type ;

      template<typename... Args, typename = decltype(new mutable_type(std::declval<Args>()...))>
      static std::true_type test_constructible(int) ;
      template<typename...>
      static std::false_type test_constructible(...) ;

      template<typename C>
      static type_t<shared_intrusive_ptr<T>, typename C::refcount_policy_type> smartptr_mode(C *) ;
      static std::shared_ptr<T> smartptr_mode(...) ;

   public:
      typedef T type ;
      typedef T element_type ;
      typedef element_type &reference ;

      typedef decltype(smartptr_mode((type *)nullptr)) smartptr_type ;

      shared_ref(const shared_ref &other) noexcept = default ;

      template<typename R, typename = instance_if_t<std::is_convertible<R*, element_type*>::value> >
      shared_ref(const shared_ref<R> &other) noexcept :
         _ptr(other.ptr())
      {}

      shared_ref() : _ptr(new mutable_type)
      {
         static_assert(decltype(shared_ref::test_constructible<>(0))::value,
                       "Referenced type is not default constructible") ;
      }

      template<typename A1, typename = decltype(new mutable_type(std::declval<A1>()))>
      explicit shared_ref(A1 &&p1) :
         _ptr(new mutable_type(p1)) {}

      template<typename A1, typename A2, typename...Args,
               typename = decltype(new mutable_type(std::declval<A1>(), std::declval<A2>(), std::declval<Args>()...))>

      shared_ref(A1 &&p1, A2 &&p2, Args &&...args) :
         _ptr(new mutable_type(std::forward<A1>(p1), std::forward<A2>(p2), std::forward<Args>(args)...))
      {}

      explicit shared_ref(const smartptr_type &ptr) :
         _ptr(PCOMN_ENSURE_ARG(ptr))
      {}
      explicit shared_ref(smartptr_type &&ptr) :
         _ptr(std::move(PCOMN_ENSURE_ARG(ptr)))
      {}

      type &get() const noexcept { return *_ptr ; }
      operator type &() const noexcept { return get() ; }
      type *operator->() const noexcept { return &get() ; }
      type &operator*() const noexcept { return get() ; }

      const smartptr_type &ptr() const noexcept { return _ptr ; }
      operator const smartptr_type &() const noexcept { return ptr() ; }

      shared_ref &operator=(const shared_ref &) noexcept = default ;

      template<typename R>
      std::enable_if_t<std::is_convertible<R*, element_type*>::value,
                       shared_ref &>
      operator=(const shared_ref<R> &other) noexcept
      {
         _ptr = other.ptr() ;
         return *this ;
      }

      /// Call the Callable object, reference to which is stored.
      /// @note Available only if the stored reference points to a Callable object.
      ///
      template<typename... Args>
      std::result_of_t<reference(Args...)>
      operator()(Args&&... args) const
      {
         return get()(std::forward<Args>(args)...) ;
      }

      template<typename A>
      auto operator[](A &&a) const -> decltype(this->get()[std::forward<A>(a)])
      {
         return get()[std::forward<A>(a)] ;
      }

      intptr_t instances() const noexcept { return _ptr.use_count() ; }
      intptr_t use_count() const noexcept { return _ptr.use_count() ; }

      void swap(shared_ref &other) noexcept { pcomn_swap(_ptr, other._ptr) ; }

   private:
      smartptr_type _ptr ;
} ;

template<class T> class sptr_wrapper_tag ;

/***************************************************************************//**
 sptr_wrapper<T> is a wrapper around a smartpointer of type T

 sptr_wrapper<T> is implicitly convertible to (T::element_type *) and designed for
 use as a bound plain pointer argument in closures and std::bind
 (like e.g. std::bind(&Foo::bar,sptr(pcomn::shared_intrusive_ptr<Foo>>(new Foo))))
*******************************************************************************/
template<class T>
class sptr_wrapper : private sptr_wrapper_tag<T> {
   public:
      typedef T smartptr_type ;
      typedef typename T::element_type element_type ;

      explicit sptr_wrapper(const T &p) noexcept : _ptr(p) {}

      /// Get the value of the stored smartpointer as a plain pointer
      element_type *get() const noexcept { return _ptr.get() ; }
      /// Get the value of the stored smartpointer as a plain pointer
      operator element_type *() const noexcept { return get() ; }
      /// Get the stored smartpointer
      const smartptr_type &ptr() const noexcept { return _ptr ; }

      void swap(sptr_wrapper &other) noexcept { other._ptr.swap(_ptr) ; }
   private:
      T _ptr ;
} ;

/// Create a smartpointer wrapper from passed smartpointer @a p
template<typename T>
inline sptr_wrapper<T> sptr(const T &p) noexcept { return sptr_wrapper<T>(p) ; }

template<typename T>
inline void swap(sptr_wrapper<T> &x, sptr_wrapper<T> &y) noexcept { x.swap(y) ; }

/*******************************************************************************
 Define swap function and tags for smartpointers
*******************************************************************************/
#define PCOMN_SPTR_DEF(PTemplate)                                  \
   template<typename T>                                            \
   inline void swap(PTemplate<T> &lhs, PTemplate<T> &rhs) noexcept \
   {                                                               \
      lhs.swap(rhs) ;                                              \
   }                                                               \
   template<typename T> class sptr_wrapper_tag<PTemplate<T> > {}

PCOMN_SPTR_DEF(shared_intrusive_ptr) ;
PCOMN_SPTR_DEF(shared_ref) ;

#undef PCOMN_SPTR_DEF
#undef PCOMN_SPTR_RELOP

/*******************************************************************************
 Debug output
*******************************************************************************/
template<class R>
inline std::ostream &operator<<(std::ostream &os, const shared_intrusive_ptr<R> &ptr)
{
   return os << '(' << ptr.get() << ", " << ptr.instances() << ')' ;
}

template<typename T>
inline std::ostream &operator<<(std::ostream &os, const shared_ref<T> &ref)
{
   return os << ref.get() ;
}

/*******************************************************************************
 Backward compatibility
*******************************************************************************/
template<typename T> using PTDirectSmartPtr = shared_intrusive_ptr<T> ;
template<typename T> using PTSmartRef       = shared_ref<T> ;
template<typename T> using PTRefLease       = ref_lease<T> ;

} // end of namespace pcomn


#endif /* PCOMN_SMARTPTR_H */
