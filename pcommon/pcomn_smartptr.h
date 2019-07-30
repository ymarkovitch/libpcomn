/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_SMARTPTR_H
#define __PCOMN_SMARTPTR_H
/*******************************************************************************
 FILE         :   pcomn_smartptr.h
 COPYRIGHT    :   Yakov Markovitch, 1994-2019. All rights reserved.
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
inline active_counter<C, init> *inc_ref(active_counter<C, init> *counted)
{
   counted && counted->inc() ;
   return counted ;
}

template<typename C, intptr_t init>
inline active_counter<C, init> *dec_ref(active_counter<C, init> *counted)
{
   counted && counted->dec() ;
   return counted ;
}

template<typename Refcounted>
inline void assign_ref(Refcounted *&target, Refcounted *source)
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
inline void clear_ref(Refcounted *&target)
{
   if (target)
   {
      dec_ref(target) ;
      target = nullptr ;
   }
}

template<typename T> struct refcount_policy ;

/******************************************************************************/
/** Basic policy for intrusive reference-counted pointers
*******************************************************************************/
template<typename T, typename C, C (T::*counter)>
struct refcount_basic_policy {

      static void threshold_action(T *ptr) { delete ptr ; }

      static C instances(const T *ptr)
      {
         NOXCHECK(ptr) ;
         return atomic_op::load(&(ptr->*counter), std::memory_order_consume) ;
      }

      static C inc_ref(T *ptr)
      {
         NOXCHECK(ptr) ;
         return atomic_op::preinc(&(ptr->*counter)) ;
      }
      static C dec_ref(T *ptr)
      {
         NOXCHECK(ptr) ;
         const C result = atomic_op::predec(&(ptr->*counter)) ;
         if (!result)
            refcount_policy<T>::threshold_action(ptr) ;
         return result ;
      }
} ;

/*******************************************************************************
                     class PRefBase
*******************************************************************************/
class PRefBase {
   protected:
      virtual ~PRefBase() {}
      static void self_destroy(PRefBase *object) { delete object ; }
} ;

/******************************************************************************/
/** Reference counter: the base class for reference counted objects.
*******************************************************************************/
template<typename C = std::atomic<intptr_t>>
class PTRefCounter : public PRefBase, public active_counter<C> {
      typedef active_counter<C> ancestor ;
   public:
      using typename ancestor::count_type ;
      typedef refcount_policy<PTRefCounter<C>> refcount_policy_type ;

      /// Alias for instances(), added to match std::shared_ptr insterface
      count_type use_count() const { return instances() ; }

      /// Get current instance counter value.
      count_type instances() const { return this->count() ; }

   protected:
      /// The default constructor creates object with zero counter.
      PTRefCounter() = default ;

      /// Copy constructor.
      /// Creates object with zeroed counter, same as default constructor.
      /// In fact, does nothing about the copying.
      PTRefCounter (const PTRefCounter &) : ancestor() {}

      /// Destructor does nothing, declared for inheritance protection and debugging purposes.
      ~PTRefCounter() = default ;

      /// Deletes "this" (itself); overrides pure virtual active_counter::dec_action().
      count_type dec_action(count_type threshold) override
      {
         self_destroy(this) ;
         return threshold ;
      }

      /// Does nothing, provided in order to implement pure virtual function of
      /// active_counter.
      count_type inc_action(count_type threshold) override
      {
         return threshold ;
      }
} ;

typedef PTRefCounter<> PRefCount ;

/******************************************************************************/
/** Intrusive reference-counted pointer policy for objects based on
 PTRefCounter (or PRefCount)
*******************************************************************************/
template<typename C>
struct refcount_policy<PTRefCounter<C>> {
      typedef typename PTRefCounter<C>::count_type count_type ;

      static count_type instances(const PTRefCounter<C> *counted)
      {
         NOXCHECK(counted) ;
         return counted->instances() ;
      }
      static count_type inc_ref(PTRefCounter<C> *counted)
      {
         NOXCHECK(counted) ;
         return counted->inc() ;
      }
      static count_type dec_ref(PTRefCounter<C> *counted)
      {
         NOXCHECK(counted) ;
         return counted->dec() ;
      }
} ;

/*******************************************************************************

*******************************************************************************/
namespace detail {
template<typename T> struct refcount_policy_ {
      typedef typename T::refcount_policy_type type ;
} ;
template<typename U> struct refcount_policy_<refcount_policy<U> > {
      typedef refcount_policy<U> type ;
} ;
}

/******************************************************************************/
/** Intrusive reference-counted shared pointer.

 Intrusive shared pointer requires the pointed object to provide reference counting
 logic. This can be done in one of the two possible ways:

  - The template parameter T is (directly or indirectly) derived from the PTRefCounter
    instance.
  - There is a specialization of refcount_policy template for the class T.
*******************************************************************************/
template<typename T>
class shared_intrusive_ptr {
      // SFINAE selector for refcount policy
      // If refcount_policy<T> is a complete type, use it;
      // otherwise, use T::refcount_policy_type
      template<typename U>
      static refcount_policy<typename std::remove_cv<U>::type> *
      policy(U *, std::is_convertible<decltype((void)refcount_policy<typename std::remove_cv<U>::type>::inc_ref(nullptr)), void> * = nullptr) ;
      static T *policy(...) ;
   public:
      typedef T element_type ;
      typedef typename detail::refcount_policy_<std::remove_pointer_t<decltype(policy(autoval<T *>()))> >::type refcount_policy_type ;

      template<typename U>
      friend class shared_intrusive_ptr ;

      constexpr shared_intrusive_ptr() : _object() {}
      constexpr shared_intrusive_ptr(nullptr_t) : shared_intrusive_ptr() {}

      shared_intrusive_ptr(element_type *object) :
         _object(object) { inc_ref() ; }

      shared_intrusive_ptr(const shared_intrusive_ptr &src) :
         shared_intrusive_ptr(src.get())
      {}

      shared_intrusive_ptr(shared_intrusive_ptr &&src) : _object(src._object)
      { src._object = nullptr ; }

      template<typename U, typename = instance_if_t<std::is_convertible<U*, element_type*>::value> >
      shared_intrusive_ptr(const shared_intrusive_ptr<U> &src) :
         shared_intrusive_ptr(src.get())
      {}

      template<typename U, typename = instance_if_t<std::is_convertible<U*, element_type*>::value> >
      shared_intrusive_ptr(shared_intrusive_ptr<U> &&src) : _object(src._object)
      { src._object = nullptr ; }

      ~shared_intrusive_ptr() { dec_ref() ; }

      constexpr element_type *get() const { return _object ; }
      constexpr element_type *operator->() const { return get() ; }
      constexpr element_type &operator*() const { return *get() ; }

      constexpr explicit operator bool() const { return !!get() ; }

      /// Get the number of different intrusive_sptr instances (this included) managing
      /// the current object
      ///
      intptr_t instances() const { return _object ? refcount_policy_type::instances(_object) : 0 ; }

      /// Alias for instances(), added to match std::shared_ptr insterface
      intptr_t use_count() const { return instances() ; }

      void reset() { shared_intrusive_ptr().swap(*this) ; }

      void swap(shared_intrusive_ptr &other) noexcept
      {
         std::swap(_object, other._object) ;
      }

      shared_intrusive_ptr &operator=(const shared_intrusive_ptr &other)
      {
         assign_element(other.get()) ;
         return *this ;
      }

      template<typename U, typename = instance_if_t<std::is_convertible<U*, element_type*>::value>>
      shared_intrusive_ptr &operator=(const shared_intrusive_ptr<U> &other)
      {
         assign_element(other.get()) ;
         return *this ;
      }

      shared_intrusive_ptr &operator=(element_type *other)
      {
         assign_element(other) ;
         return *this ;
      }

   private:
      std::remove_const_t<element_type> * mutable_object() const
      {
         return const_cast<std::remove_const_t<element_type> *>(_object) ;
      }

      void inc_ref() const { if (_object) refcount_policy_type::inc_ref(mutable_object()) ; }
      void dec_ref() { if (_object) refcount_policy_type::dec_ref(mutable_object()) ; }

      template<class E>
      void assign_element(E *other_element)
      {
         typedef typename shared_intrusive_ptr<E>::refcount_policy_type other_policy_type ;
         typedef std::remove_const_t<E>                                 other_mutable_element ;

         other_mutable_element *other_object = const_cast<other_mutable_element *>(other_element) ;

         if (_object != other_object)
         {
            if (other_object)
               other_policy_type::inc_ref(other_object) ; /* First we must increment counter of source object
                                                           * due to possibility of (indirect) removal of all
                                                           * remaining references to that one as a result of
                                                           * dec_ref() call. */
            dec_ref() ;
            _object = other_object ;
         }
      }

   private:
      element_type *_object ;

} ;

/// Allows to cast smartpointer to a base class to a smartpointer to derived the same way
/// static_cast allows for plain pointers.
/// Casting derived -> base happens automatically.
template<class T, class U>
inline shared_intrusive_ptr<T> sptr_cast(const shared_intrusive_ptr<U> &src)
{
   return shared_intrusive_ptr<T>(static_cast<T *>(src.get())) ;
}

/*******************************************************************************
                     template<class T>
                     class ref_lease
*******************************************************************************/
template<class T>
struct ref_lease  {
      explicit ref_lease(T *guarded) : _guarded(guarded) { inccount(guarded) ; }
      ~ref_lease() { deccount(_guarded) ; }

   private:
      T * const _guarded ;

      template<typename C, intptr_t init>
      static void inccount(active_counter<C, init> *counter)
      {
         if (counter) counter->inc_passive() ;
      }
      template<typename C, intptr_t init>
      static void deccount(active_counter<C, init> *counter)
      {
         if (counter) counter->dec_passive() ;
      }
      PCOMN_NONCOPYABLE(ref_lease) ;
} ;

/*******************************************************************************
 Operators ==, !=, <
*******************************************************************************/
#define PCOMN_SPTR_RELOP(Template, op)                                  \
   template<typename T>                                                 \
   inline bool operator op(const typename Template<T>::element_type *lhs, \
                           const Template<T> &rhs)                      \
   {                                                                    \
      return lhs op rhs.get() ;                                         \
   }                                                                    \
                                                                        \
   template<typename T>                                                 \
   inline bool operator op(const Template<T> &lhs,                      \
                           const typename Template<T>::element_type *rhs) \
   {                                                                    \
      return lhs.get() op rhs ;                                         \
   }                                                                    \
   template<typename T>                                                 \
   inline bool operator op(nullptr_t, const Template<T> &rhs)           \
   {                                                                    \
      return nullptr op rhs.get() ;                                     \
   }                                                                    \
                                                                        \
   template<typename T>                                                 \
   inline bool operator op(const Template<T> &lhs, nullptr_t)           \
   {                                                                    \
      return lhs.get() op nullptr ;                                     \
   }                                                                    \
                                                                        \
   template<typename T, typename U>                                     \
   inline decltype((T *)nullptr == (U *)nullptr)                        \
   operator op(const Template<T> &lhs, const Template<U> &rhs)          \
   {                                                                    \
      return lhs.get() op rhs.get() ;                                   \
   }

PCOMN_SPTR_RELOP(shared_intrusive_ptr, ==) ;
PCOMN_SPTR_RELOP(shared_intrusive_ptr, !=) ;
PCOMN_SPTR_RELOP(shared_intrusive_ptr, <) ;


/******************************************************************************/
/** Smart reference: template class like a smartpointer that constructs its pointee
 object and thus is never NULL.
*******************************************************************************/
template<typename T>
class shared_ref {
      typedef typename std::remove_cv<T>::type mutable_type ;

      template<typename... Args, typename = decltype(new mutable_type(std::declval<Args>()...))>
      static std::true_type test_constructible(int) ;
      template<typename...>
      static std::false_type test_constructible(...) ;
   public:

      typedef T type ;
      typedef T element_type ;
      typedef element_type &reference ;

      typedef std::conditional_t<std::is_base_of<PRefBase, T>::value,
                                 shared_intrusive_ptr<type>,
                                 std::shared_ptr<type> >
      smartptr_type ;

      shared_ref(const shared_ref &other) = default ;

      template<typename R, typename = instance_if_t<std::is_convertible<R*, element_type*>::value> >
      shared_ref(const shared_ref<R> &other) :
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

      type &get() const { return *_ptr ; }
      operator type &() const { return get() ; }
      type *operator->() const { return &get() ; }
      type &operator*() const { return get() ; }

      const smartptr_type &ptr() const { return _ptr ; }
      operator const smartptr_type &() const { return ptr() ; }

      shared_ref &operator=(const shared_ref &) = default ;

      template<typename R>
      std::enable_if_t<std::is_convertible<R*, element_type*>::value,
                       shared_ref &>
      operator=(const shared_ref<R> &other)
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

      intptr_t instances() const { return _ptr.use_count() ; }
      intptr_t use_count() const { return _ptr.use_count() ; }

      void swap(shared_ref &other) { pcomn_swap(_ptr, other._ptr) ; }

   private:
      smartptr_type _ptr ;
} ;

template<class T> class sptr_wrapper_tag ;

/******************************************************************************/
/** sptr_wrapper<T> is a wrapper around a smartpointer of type T

 sptr_wrapper<T> is implicitly convertible to (T::element_type *) and designed for
 use as a bound plain pointer argument in closures and std::bind
 (like e.g. std::bind(&Foo::bar,sptr(pcomn::shared_intrusive_ptr<Foo>>(new Foo))))
*******************************************************************************/
template<class T>
class sptr_wrapper : private sptr_wrapper_tag<T> {
   public:
      typedef T smartptr_type ;
      typedef typename T::element_type element_type ;

      explicit sptr_wrapper(const T &p) : _ptr(p) {}

      /// Get the value of the stored smartpointer as a plain pointer
      element_type *get() const { return _ptr.get() ; }
      /// Get the value of the stored smartpointer as a plain pointer
      operator element_type *() const { return get() ; }
      /// Get the stored smartpointer
      const smartptr_type &ptr() const { return _ptr ; }

      void swap(sptr_wrapper &other) { other._ptr.swap(_ptr) ; }
   private:
      T _ptr ;
} ;

/// Create a smartpointer wrapper from passed smartpointer @a p
template<typename T>
inline sptr_wrapper<T> sptr(const T &p) { return sptr_wrapper<T>(p) ; }

template<typename T>
inline void swap(sptr_wrapper<T> &x, sptr_wrapper<T> &y) { x.swap(y) ; }

/*******************************************************************************
 Define swap function and tags for smartpointers
*******************************************************************************/
#define PCOMN_SPTR_DEF(PTemplate)                                 \
   template<typename T>                                           \
   inline void swap(PTemplate<T> &lhs, PTemplate<T> &rhs)         \
   {                                                              \
      lhs.swap(rhs) ;                                             \
   }                                                              \
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
