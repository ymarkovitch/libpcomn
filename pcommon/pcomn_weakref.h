/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_WEAKREF_H
#define __PCOMN_WEAKREF_H
/*******************************************************************************
 FILE         :   pcomn_weakref.h
 COPYRIGHT    :   Yakov Markovitch, 2003-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Weak references.

 CREATION DATE:   7 Apr 2003
*******************************************************************************/
#include <pcomn_smartptr.h>
#include <pcomn_platform.h>
#include <stdexcept>

/*******************************************************************************
 Weak reference API
 Provides a host of classes that allow a programmer to make weakly-referenceable
 objects.
 This suite does not force using of multiple inheritance for the classes made
 weakly-referenceable, thus making it possible to work with specific single-root
 hierarchies like VCL library.
 To make particular class weakly-referenceable, place anywhere in its declaration
 (preferably in the private part) PCOMN_WEAK_REFERENCEABLE call with the name
 of the class itself as a parameter:

 class MyClass {
    public:
       ...
    private:
       PCOMN_WEAK_REFERENCEABLE(MyClass) ; // Don't forget a semicolon!
 } ;

 Then, to get a weak reference to any object of MyClass or descendants thereof,
 construct an object of type PTWeakReference<MyClass>, passing it a pointer to
 MyClass:

 MyClass *my = new MyClass ;
 PTWeakReference<MyClass> weak_my (my) ;

 You as well may assign pointers:

 MyClass *my = new MyClass ;
 PTWeakReference<MyClass> weak_my (my) ;
 MyClass *my1 = new MyClass ;

 weak_my = my1 ;
 weak_my = NULL ;

 Derived classes are handled properly:

 class MyDerived : MyClass {
 } ;

 MyDerived *my = new MyDerived ;
 PTWeakReference<MyDerived> weak_derived (my) ;
 PTWeakReference<MyClass> weak_my (my) ;

 weak_my = weak_derived ; // That's OK, but the other way around end up in
                          // a compiler error, just as expected
*******************************************************************************/

namespace pcomn {

namespace wref_passive {
template<class Referent>
class PTWeakRefBase ;
template<class Referent>
class PTWeakRefHolder ;
template<class Referent>
class PTWeakRefProxy ;
}

namespace wref_active {
template<class Referent>
class PTWeakRefBase ;
template<class Referent>
class PTWeakRefHolder ;
template<class Referent>
class PTWeakRefProxy ;
}

template<class Referent, template<class R> class Reference>
class PTWeakReference ;

#define PCOMN_WEAK_REFERENCEABLE(type)                         \
   public:                                                     \
      typedef type refself_type ;                              \
   private:                                                    \
      pcomn::wref::PTWeakRefHolder<type > _weak_ref_holder ;   \
      friend class pcomn::wref::PTWeakRefBase<type >

// Yes, this is a very inelegant way, magic like policy template parameters are
// much cooler; but for the moment it works and, unlike the latter method, doesn't
// force compiler to blow out. So, until better times...
#ifndef PCOMN_WEAKREF_ACTIVE
namespace wref = wref_passive ;
#else
namespace wref = wref_active ;
#endif

using namespace wref ;

/*******************************************************************************
                     class object_deleted
 PTWeakReference throws object_deleted on attempt to access already deleted
 weakly-referenced object
*******************************************************************************/
class object_deleted : public std::runtime_error {
  public:
      object_deleted() :
         std::runtime_error("weakly-referenced object no longer exists")
      {}
} ;

/*******************************************************************************
 *
 * Passive weak references
 *
 ******************************************************************************/
namespace wref_passive {

/*******************************************************************************
                     template<class Referent>
                     class wref_passive::PTWeakRefProxy
*******************************************************************************/
template<class Referent>
class PTWeakRefProxy {
   public:
      friend class PTWeakRefBase<Referent> ;
   public:
      explicit PTWeakRefProxy(Referent *referent = NULL) :
         _referent(referent)
         // Don't touch _generation!
      {}

      Referent *referent() const { return _referent ; }
      unsigned long generation() const { return _generation ; }

      void *operator new(size_t) ;
      void operator delete(void *, size_t) ;

   private:
      Referent *     _referent ;
      unsigned long  _generation ;

      struct MemCell {
            MemCell  *     next ;
            unsigned long  generation ;
      } ;

      struct MemChunk {
            ~MemChunk() { delete next ; }
            static MemCell *cellalloc()
            {
               MemCell *first = cell() ;
               if (MemCell *result = first->next)
               {
                  first->next = result->next ;
                  return result ;
               }
               MemChunk *chunk = heap() ;
               if (chunk->full())
               {
                  chunk = chunk->next = new MemChunk ;
                  chunk->init() ;
               }
               return chunk->memory + chunk->lastndx++ ;
            }

            static void cellfree(MemCell *mem)
            {
               if (!mem)
                  return ;
               MemCell *first = cell() ;
               ++mem->generation ;
               mem->next = first->next ;
               first->next = mem ;
            }

         private:
            enum { Size = 125 } ;

            MemChunk *  next ;
            int         lastndx ;
            MemCell     memory[Size] ;

            void init() { memset(this, 0, sizeof *this) ; }
            bool full() const { return lastndx == Size ; }
            static MemChunk *heap() ;
            static MemCell * cell() ;
      } ;

} ;

template<class Referent>
void *PTWeakRefProxy<Referent>::operator new(size_t sz)
{
   NOXCHECK(sz == sizeof (MemCell)) ;
   PCOMN_USE(sz) ;
   return MemChunk::cellalloc() ;
}

template<class Referent>
void PTWeakRefProxy<Referent>::operator delete(void *mem, size_t)
{
   return MemChunk::cellfree(static_cast<MemCell *>(mem)) ;
}

template<class Referent>
typename PTWeakRefProxy<Referent>::MemChunk *PTWeakRefProxy<Referent>::MemChunk::heap()
{
   static MemChunk first ;
   static MemChunk *last = &first ;
   if (last->next)
      last = last->next ;
   return last ;
}

template<class Referent>
typename PTWeakRefProxy<Referent>::MemCell *PTWeakRefProxy<Referent>::MemChunk::cell()
{
   static MemCell first ;
   return &first ;
}

/*******************************************************************************
                     template<class Referent>
                     class wref_passive::PTWeakRefBase
*******************************************************************************/
template<class Referent>
class PTWeakRefBase {
   public:
      typedef PTWeakRefProxy<Referent> proxy_type ;
      typedef Referent                 referent_type ;

   protected:
      PTWeakRefBase() :
         _proxy(null())
      {}

      proxy_type *proxy() const { return _proxy ; }

      void set_proxy(const PTWeakRefBase &source) const
      {
         _proxy = source._proxy ;
      }

      void set_proxy(const referent_type *source) const
      {
         _proxy = source ?
            static_cast<const PTWeakRefBase<Referent> *>(&holder(source))->get_proxy() : null() ;
      }

      static PTWeakRefProxy<Referent> *null() ;

      static const PTWeakRefHolder<Referent> &holder(const referent_type *self)
      {
         return self->_weak_ref_holder ;
      }

      static referent_type *referent(const PTWeakRefHolder<Referent> *self)
      {
         return reinterpret_cast<referent_type*>((char *)self -
                                                 PCOMN_NONCONST_OFFSETOF(referent_type, _weak_ref_holder)) ;
      }

   private:
      mutable proxy_type *_proxy ;

      proxy_type *get_proxy() const
      {
         if (!_proxy->referent())
            _proxy = new proxy_type(referent(static_cast<const PTWeakRefHolder<Referent> *>(this))) ;
         NOXCHECK(_proxy) ;
         return _proxy ;
      }
} ;

template<class Referent>
PTWeakRefProxy<Referent> *PTWeakRefBase<Referent>::null()
{
   static PTWeakRefProxy<Referent> result ;
   return &result ;
}

/*******************************************************************************
                     template<class Referent>
                     class wref_passive::PTWeakRefHolder
*******************************************************************************/
template<class Referent>
class PTWeakRefHolder : public PTWeakRefBase<Referent> {
      typedef PTWeakRefBase<Referent> ancestor ;
      typedef typename ancestor::referent_type referent_type ;
      friend referent_type ;

      PTWeakRefHolder() {}
      ~PTWeakRefHolder()
      {
         if (this->proxy()->referent())
            delete this->proxy() ;
      }
      PTWeakRefHolder<Referent> &operator=(const PTWeakRefHolder &) { return *this ; }
} ;

/*******************************************************************************
                     template<class Referent>
                     class wref_passive::PTReference
*******************************************************************************/
template<class Referent>
class PTReference : public PTWeakRefBase<Referent> {
      typedef PTWeakRefBase<Referent> ancestor ;
      typedef typename ancestor::proxy_type proxy_type ;
   protected:
      PTReference() :
         _tag(0)
      {}

      PTReference(const PTReference &source) :
         ancestor(source),
         _tag(source._tag)
      {}

      Referent *unsafe() const
      {
         proxy_type *p = this->proxy() ;
         return
            (Referent *)(-(intptr_t)!(p->generation() ^ _tag) &
                         (intptr_t)p->referent()) ;
      }

      template<typename Source>
      void set_proxy(const Source &source)
      {
         ancestor::set_proxy(source) ;
         _tag = this->proxy()->generation() ;
      }

   private:
      unsigned long _tag ; /* When the reference is actual, its tag is equal to proxie's */
} ;

} ;// end of wref_passive

/*******************************************************************************
 *
 * Active weak references
 *
 ******************************************************************************/
namespace wref_active {

/*******************************************************************************
                     template<class Referent>
                     class wref_active::PTWeakRefProxy
*******************************************************************************/
template<class Referent>
class PTWeakRefProxy {
      friend class PTWeakRefBase<Referent> ;
   public:
      PTWeakRefProxy(Referent *referent) :
         _referent(referent),
         _refcount(0)
      {}

      Referent *referent() const { return _referent ; }

   private:
      Referent *  _referent ;
      int         _refcount ;

      PTWeakRefProxy() :
         _referent(NULL),
         _refcount(1)
      {}

      void reset() { _referent = NULL ; }
      void incref() { ++_refcount ; }
      void decref()
      {
         NOXCHECK(_refcount) ;
         if (_refcount-- == 1)
            delete this ;
      }

} ;

/*******************************************************************************
                     template<class Referent>
                     class wref_active::PTWeakRefBase
*******************************************************************************/
template<class Referent>
class PTWeakRefBase {
   public:
      typedef PTWeakRefProxy<Referent> proxy_type ;
      typedef Referent                referent_type ;

   protected:
      PTWeakRefBase() :
         _proxy(null())
      {
         proxy()->incref() ;
      }

      PTWeakRefBase(const PTWeakRefBase &base) :
         _proxy(base.proxy())
      {
         proxy()->incref() ;
      }

      ~PTWeakRefBase()
      {
         proxy()->decref() ;
      }

      proxy_type *proxy() const { return _proxy ; }

      void reset() { proxy()->reset() ; }

      void set_proxy(const PTWeakRefBase &source) const
      {
         set_proxy(source.proxy()) ;
      }

      void set_proxy(const referent_type *source) const
      {
         set_proxy(source ?
                   static_cast<const PTWeakRefBase<Referent> *>(&holder(source))->get_proxy() :
                   null()) ;
      }

      static PTWeakRefProxy<Referent> *null() ;

      static const PTWeakRefHolder<Referent> &holder(const referent_type *self)
      {
         return self->_weak_ref_holder ;
      }

      static referent_type *referent(const PTWeakRefHolder<Referent> *self)
      {
         return reinterpret_cast<referent_type*>((char *)self -
                                                 PCOMN_NONCONST_OFFSETOF(referent_type, _weak_ref_holder)) ;
      }

   private:
      mutable proxy_type *_proxy ;

      PTWeakRefBase &operator=(const PTWeakRefBase &) ;

      void set_proxy(proxy_type *source) const
      {
         proxy_type *tmp = _proxy ;
         source->incref() ; /* We must increment counter of source object first, due to
                           * possibility of (indirect) removal of all remaining
                           * references to that one as a result of decref(target) call.
                           */
         _proxy = source ;
         tmp->decref() ;
      }

      proxy_type *get_proxy() const
      {
         if (!_proxy->referent())
            _create_proxy() ;
         NOXCHECK(_proxy) ;
         return _proxy ;
      }

      void _create_proxy() const ;
} ;

template<class Referent>
PTWeakRefProxy<Referent> *PTWeakRefBase<Referent>::null()
{
   static PTWeakRefProxy<Referent> result ;
   return &result ;
}

template<class Referent>
void PTWeakRefBase<Referent>::_create_proxy() const
{
   NOXCHECK(_proxy && !_proxy->referent()) ;
   set_proxy(new proxy_type(referent(static_cast<const PTWeakRefHolder<Referent> *>(this)))) ;
} ;


/*******************************************************************************
                     template<class Referent>
                     class wref_active::PTWeakRefHolder
*******************************************************************************/
template<class Referent>
class PTWeakRefHolder : public PTWeakRefBase<Referent> {
      typedef PTWeakRefBase<Referent> ancestor ;
      typedef typename ancestor::referent_type referent_type ;
      friend referent_type ;

      PTWeakRefHolder() {}
      ~PTWeakRefHolder() { ancestor::reset() ; }
      PTWeakRefHolder<Referent> &operator=(const PTWeakRefHolder &) { return *this ; }
} ;

/*******************************************************************************
                     template<class Referent>
                     class wref_active::PTReference
*******************************************************************************/
template<class Referent>
class PTReference : public PTWeakRefBase<Referent> {
   protected:
      PTReference() {}

      PTReference(const PTReference &source) :
         PTWeakRefBase<Referent>(source)
      {}

      Referent *unsafe() const
      {
         return this->proxy()->referent() ;
      }
} ;

} // end of wref_active

/*******************************************************************************
                     template<class T>
                     class PTWeakReference
*******************************************************************************/
template<class Referent, template<class R> class Reference = PTReference>
class PTWeakReference : public Reference<typename Referent::refself_type> {
      typedef Reference<typename Referent::refself_type> ancestor ;

   public:
      typedef Referent          referent_type ;
      typedef referent_type *   pointer ;

      PTWeakReference() {}

      PTWeakReference(const PTWeakReference &source) :
         ancestor(source)
      {}

      PTWeakReference(const referent_type *referent)
      {
         this->set_proxy(referent) ;
      }

      template<class Other>
      PTWeakReference(const PTWeakReference<Other, Reference> &source)
      {
         set_derived_proxy(source, (Other *)0) ;
      }

      PTWeakReference &operator=(const PTWeakReference &source)
      {
         this->set_proxy(source) ;
         return *this ;
      }

      PTWeakReference &operator=(const referent_type *source) ;

      template<class Other>
      PTWeakReference &operator=(const PTWeakReference<Other, Reference> &source)
      {
         set_derived_proxy(source, (Other *)0) ;
         return *this ;
      }

      referent_type *operator->() const { return safe() ; }
      referent_type &operator*() const { return *safe() ; }

      referent_type *unsafe() const
      {
         return static_cast<referent_type *>(ancestor::unsafe()) ;
      }

      operator unspecified_bool(PTWeakReference)() const
      {
         return reinterpret_cast<unspecified_bool(PTWeakReference) >(unsafe()) ;
      }

   protected:
      referent_type *safe() const ;

   private:
      void set_derived_proxy(const ancestor &source, referent_type *)
      {
         this->set_proxy(source) ;
      }
} ;

template<class Referent, template<class R> class Reference>
typename PTWeakReference<Referent, Reference>::pointer PTWeakReference<Referent, Reference>::safe() const
{
   if (pointer result = unsafe())
      return result ;
   throw object_deleted() ;
}

template<class Referent, template<class R> class Reference>
PTWeakReference<Referent, Reference> &
PTWeakReference<Referent, Reference>::operator=(const referent_type *source)
{
   this->set_proxy(source) ;
   return *this ;
}

#define PCOMN_WREF_RELOP(op)                                      \
template<class L, class R, template<class> class Reference>       \
inline bool operator op(const L *lhs,                             \
                        const PTWeakReference<R, Reference> &rhs) \
{                                                                 \
   return lhs op rhs.unsafe() ;                                   \
}                                                                 \
                                                                  \
template<class L, class R, template<class> class Reference>       \
inline bool operator op(const PTWeakReference<L, Reference> &lhs, \
                        const R *rhs)                             \
{                                                                 \
   return lhs.unsafe() op rhs ;                                   \
}                                                                 \
                                                                  \
template<class L, class R, template<class> class Reference>       \
inline bool operator op(const PTWeakReference<L, Reference> &lhs, \
                        const PTWeakReference<R, Reference> &rhs) \
{                                                                 \
   return lhs.unsafe() op rhs.unsafe() ;                          \
}

PCOMN_WREF_RELOP(==) ;
PCOMN_WREF_RELOP(!=) ;
PCOMN_WREF_RELOP(<) ;

#undef PCOMN_WREF_RELOP

} // end of namespace pcomn

using pcomn::PTWeakReference ;

#endif /* __PCOMN_WEAKREF_H */
