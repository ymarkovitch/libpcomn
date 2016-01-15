/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_MXMUTEX_H
#define __PCOMN_MXMUTEX_H
/*******************************************************************************
 FILE         :   pcomn_mxmutex.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Multiplexed mutex.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   28 Aug 2008
*******************************************************************************/
/** @file
  Multiplexed mutex provides synchronization based on a value of a key, like a keyed
  mutex, but, in contrast to the keyed mutex, two different key values can contend.
*******************************************************************************/
#include <pcomn_syncobj.h>
#include <pcomn_hash.h>
#include <pcomn_primenum.h>
#include <pcomn_integer.h>

namespace pcomn {
namespace detail { template<class, size_t, typename, typename> struct mxlock_guard ; }

/******************************************************************************/
/** Multiplexed mutex: in contrast to the keyed mutex, two different key values can
 contend.
*******************************************************************************/
template<class Mutex, size_t sz,
         typename Key, typename Hash = pcomn::hash_fn_raw<Key> >
class PTMxMutex {
      typedef Mutex single_lock ;
      friend detail::mxlock_guard<Mutex, sz, Key, Hash> ;
   public:
      typedef Key    key_type ;
      typedef Hash   hasher ;

      static const size_t ratio = pow2_primeceil<bitop::ct_log2ceil<sz>::value >::value ;

      explicit PTMxMutex(const hasher &h = hasher()) : _hasher(h) {}

      void lock(const key_type &key) { slock(key).lock() ; }
      bool try_lock(const key_type &key) { return slock(key).try_lock() ; }

      void lock_shared(const key_type &key) { slock(key).lock_shared() ; }
      bool try_lock_shared(const key_type &key) { return slock(key).try_lock_shared() ; }

      void unlock(const key_type &key) { slock(key).unlock() ; }
      void unlock_shared(const key_type &key) { slock(key).unlock_shared() ; }

      static size_t capacity() { return ratio ; }

   private:
      const hasher   _hasher ;
      single_lock    _locks[ratio] ;

      single_lock &slock(const key_type &key) { return _locks[_hasher(key) % ratio] ; }
} ;

template<class Mutex, size_t sz, typename Key, typename Hash>
const size_t PTMxMutex<Mutex, sz, Key, Hash>::ratio ;

namespace detail {

template<class M, size_t sz, typename K, typename H>
struct mxlock_guard {
      typedef PTMxMutex<M, sz, K, H> lock_type ;
      typedef typename lock_type::key_type      key_type ;
      typedef typename lock_type::single_lock   single_lock ;
   protected:
      constexpr mxlock_guard() : _lock() {}
      explicit mxlock_guard(lock_type &lock, const key_type &key) : _lock(&lock.slock(key)) {}

      void lock() const { slock().lock() ; }
      bool try_lock() const { return slock().try_lock() ; }

      void lock_shared() const { slock().lock_shared() ; }
      bool try_lock_shared() const { return slock().try_lock_shared() ; }

      void unlock() const
      {
         if (this->_lock)
            this->_lock->unlock() ;
      }

      void unlock_shared() const
      {
         if (this->_lock)
            this->_lock->unlock_shared() ;
      }

   protected:
      single_lock *_lock ;

   private:
      single_lock &slock() const
      {
         NOXCHECK(this->_lock) ;
         return *this->_lock ;
      }
} ;
} // end of namespace pcomn::detail

/******************************************************************************/
/** pcomn::shared_lock specialization for a multiplexed mutex.
*******************************************************************************/
template<class M, size_t sz, typename K, typename H>
class shared_lock<PTMxMutex<M, sz, K, H> > : public detail::mxlock_guard<M, sz, K, H> {
      typedef detail::mxlock_guard<M, sz, K, H> ancestor ;
   public:
      using typename ancestor::lock_type ;
      using typename ancestor::key_type ;

      shared_lock(lock_type &lock, const key_type &key) : ancestor(lock, key) { this->lock_shared() ; }
      ~shared_lock() { this->unlock_shared() ; }

      void lock() const { this->lock_shared() ; }
      bool try_lock() const { return this->try_lock_shared() ; }
      void unlock() const { this->unlock_shared() ; }

   protected:
      shared_lock() {}
} ;
} // end of namespace pcomn

namespace std {

/******************************************************************************/
/** std::lock_guard specialization for a multiplexed mutex.
*******************************************************************************/
template<class M, size_t sz, typename K, typename H>
class lock_guard<pcomn::PTMxMutex<M, sz, K, H> > : public pcomn::detail::mxlock_guard<M, sz, K, H> {
      typedef pcomn::detail::mxlock_guard<M, sz, K, H> ancestor ;
   public:
      using typename ancestor::lock_type ;
      using typename ancestor::key_type ;

      lock_guard(lock_type &lock, const key_type &key) : ancestor(lock, key) { this->lock() ; }
      ~lock_guard() { this->unlock() ; }

      using ancestor::lock ;
      using ancestor::try_lock ;
      using ancestor::unlock ;
   protected:
      constexpr lock_guard() {}
} ;

} // end of namespace pcomn


#endif /* __PCOMN_MXMUTEX_H */
