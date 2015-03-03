/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_KEYEDMUTEX_H
#define __PCOMN_KEYEDMUTEX_H
/*******************************************************************************
 FILE         :   pcomn_keyedmutex.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Keyed mutex.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   28 Aug 2008
*******************************************************************************/
/** @file
  Keyed mutexes - mutexes, providing synchronization based on a value of a key.
*******************************************************************************/
#include <pcomn_hashclosed.h>
#include <pcomn_algorithm.h>
#include <pcomn_hash.h>
#include <pcomn_memmgr.h>
#include <pcomn_numeric.h>
#include <pcomn_syncobj.h>

#include <new>
#include <memory>
#include <mutex>
#include <condition_variable>

namespace pcomn {

/// @cond
namespace detail {
template<typename Key> struct slock ;
template<typename Key> struct srwlock ;

template<class Key, class Bucket>
inline void destroy_single_lock(slock<Key> *value, Bucket &owner, size_t maxpoolsize)
{
   std::condition_variable * const c = xchange(value->_writer_condvar, nullptr) ;
   value->~slock<Key>() ;
   owner._lockheap.deallocate(value) ;
   owner.checkin_condvar(c, maxpoolsize) ;
}

template<class Key, class Bucket>
inline void destroy_single_lock(srwlock<Key> *value, Bucket &owner, size_t maxpoolsize)
{
   std::condition_variable * const wcond = value->_writer_condvar ;
   std::unique_ptr<std::condition_variable> rcond (value->_reader_condvar) ;
   value->_writer_condvar = value->_reader_condvar = NULL ;

   value->~slock<Key>() ;
   owner._lockheap.deallocate(value) ;

   owner.checkin_condvar(wcond, maxpoolsize) ;
   owner.checkin_condvar(rcond.release(), maxpoolsize) ;
}
} // end of namespace pcomn::detail
/// @endcond

/******************************************************************************/
/** Base class for both PTKeyedMutex and PTKeyedRWMutex
******************************************************************************/
template<typename Key, typename Hash, typename Pred, class SingleLock>
class PTKeyedMutexManager {
      PCOMN_NONCOPYABLE(PTKeyedMutexManager) ;
      PCOMN_NONASSIGNABLE(PTKeyedMutexManager) ;

   public:
      typedef Key    key_type ;
      typedef Hash   hasher ;
      typedef Pred   key_equal ;

   protected:
      typedef SingleLock single_lock ;

      PTKeyedMutexManager(unsigned multiplexing, unsigned keypool_size) :
         _keypool_size(std::max(keypool_size, 1u)),
         _multiplexing(std::max(multiplexing, 1u)),
         _buckets(new bucket[_multiplexing])
      {}

      ~PTKeyedMutexManager() ;

      struct key_selector : std::unary_function<const single_lock *, key_type> {
            const key_type &operator()(const single_lock *value) const
            {
               NOXCHECK(value) ;
               return value->_key ;
            }
      } ;

      typedef pcomn::closed_hashtable<single_lock *, key_selector, hasher, key_equal> lock_set ;

      struct bucket {
            std::mutex              _mutex ;
            lock_set                _lockset ;
            std::vector<std::condition_variable *> _condpool ;
            PMMemBlocks             _lockheap ;

            typedef std::lock_guard<std::mutex> guard ;

            bucket() :
               _lockheap(sizeof(single_lock), 8)
            {}

            ~bucket()
            {
               _mutex.lock() ;
               NOXCHECK(_lockset.empty()) ;
               // Cleanup the keypool
               std::vector<std::condition_variable *> tmp ;
               tmp.swap(_condpool) ;
               std::for_each(tmp.begin(), tmp.end(), pcomn::destroy<std::condition_variable>) ;
               _mutex.unlock() ;
            }

            single_lock *create_lock(const key_type &key)
            {
               return new (_lockheap.allocate(sizeof(single_lock))) single_lock(key) ;
            }

            void destroy_lock(single_lock *value, size_t maxpoolsize)
            {
               if (value)
                  detail::destroy_single_lock(value, *this, maxpoolsize) ;
            }

            // Get a conditional variable from the pool or create a new, if the pool
            // is empty
            std::condition_variable *checkout_condvar()
            {
               if (_condpool.empty())
                  return new std::condition_variable ;

               std::condition_variable *result = _condpool.back() ;
               _condpool.pop_back() ;

               return result ;
            }

            void checkin_condvar(std::condition_variable *condvar, size_t maxpoolsize)
            {
               if (!condvar)
                  return ;

               std::unique_ptr<std::condition_variable> pguard (condvar) ;
               if (_condpool.size() < maxpoolsize)
               {
                  // We still have free place in the pool, place the lock where
                  _condpool.push_back(condvar) ;
                  pguard.release() ;
               }
            }
      } ;

      bucket &select_bucket(const key_type &key)
      {
         bucket * const buckets = _buckets.get() ;
         return buckets[_multiplexing == 1 ? 0 : (buckets->_lockset.hash_function()(key) % _multiplexing)] ;
      }

      struct bucket_lock ;
      friend struct bucket_lock ;

      struct bucket_lock {
            bucket_lock(PTKeyedMutexManager &kmutex, const key_type &key) :
               _bucket(kmutex.select_bucket(key)),
               _bucket_lock(_bucket._mutex),
               _lock(NULL),
               _keypool_size(kmutex._keypool_size)
            {}

            ~bucket_lock() { _bucket.destroy_lock(_lock, _keypool_size) ; }

            bucket &       _bucket ;
            const typename bucket::guard _bucket_lock ;
            single_lock *  _lock ;
            const size_t   _keypool_size ;
      }  ;

   private:
      const size_t                     _keypool_size ; /* Keypool size per multilexed bucket */
      const size_t                     _multiplexing ; /* Multiplexing ratio */
      const std::unique_ptr<bucket[]>  _buckets ;
} ;

/// @cond
namespace detail {

/*******************************************************************************
                     struct detail::slock
 Single-lock object (see SingleLock template parameter of PTKeyedMutexManager) for
 PTKeyedMutex
*******************************************************************************/
template<typename Key>
struct slock {
    slock() :
         _key(),
         _queue_size(0),
         _writer_condvar()
      {}

      explicit slock(const Key &key) :
         _key(key),
         _queue_size(0),
         _writer_condvar()
      {}

      std::condition_variable &writer_condvar() const
      {
         NOXCHECK(_writer_condvar) ;
         return *_writer_condvar ;
      }

      const Key   _key ;
      unsigned    _queue_size ; /* This number is odd when the key is locked */
      std::condition_variable *  _writer_condvar ;
} ;

/*******************************************************************************
                     struct detail::srwlock
 Single-lock object (see SingleLock template parameter of PTKeyedMutexManager) for
 PTKeyedRWMutex
*******************************************************************************/
template<typename Key>
struct srwlock : slock<Key> {
      srwlock() :
         _nreaders(0),
         _npending_readers(0),
         _reader_condvar()
      {}

      explicit srwlock(const Key &key) :
         slock<Key>(key),
         _nreaders(0),
         _npending_readers(0),
         _reader_condvar()
      {}

      std::condition_variable &reader_condvar() const
      {
         NOXCHECK(_reader_condvar) ;
         return *_reader_condvar ;
      }

      unsigned    _nreaders ;
      unsigned    _npending_readers ;
      std::condition_variable *_reader_condvar ;
} ;

} // end of namespace pcomn::detail
/// @endcond

/******************************************************************************/
/** Keyed mutex - a mutex, providing synchronization based on a value of a key.

 Single PTKeyedMutex<Key> object provides synchronization on arbitrary number of unique
 keys of type Key. When you have an object @p keyed_mutex of type PTKeyedMutex<Key> and a
 thread that wishes to synchronize based on a unique Key value you call
 @p keyed_mutex.lock(key). Any other thread will block on @p keyed_mutex.lock(key)
 until the first thread calls @p keyed_mutex.unlock(key).

 If key1!=key2, @p keyed_mutex.lock(key1) and @p keyed_mutex.unlock(key2) will
 @em not contend, i.e. PTKeyedMutex object can be viewed as a set of "uniquely named"
 mutexes.
*******************************************************************************/
template<typename Key,
         typename Hash = pcomn::hash_fn<Key>,
         typename Pred = std::equal_to<Key> >

class PTKeyedMutex : private PTKeyedMutexManager<Key, Hash, Pred, detail::slock<Key> > {

      typedef PTKeyedMutexManager<Key, Hash, Pred, detail::slock<Key> > ancestor ;
   public:
      typedef typename ancestor::key_type    key_type ;
      typedef typename ancestor::hasher      hasher ;
      typedef typename ancestor::key_equal   key_equal ;

      PTKeyedMutex(unsigned multiplexing, unsigned keypool_size) :
         ancestor(multiplexing, keypool_size)
      {}

      explicit PTKeyedMutex(unsigned estimated_keycount) :
         ancestor(estimated_keycount <= 16 ? 1 : dprime_ubound((estimated_keycount + 15u)/16u), // multiplexing
                  std::max(estimated_keycount/16u, 1u)*16u) // keypool_size
      {}

      void lock(const key_type &key) { acquire_lock(key, true) ; }

      bool try_lock(const key_type &key) { return acquire_lock(key, false) ; }

      bool unlock(const key_type &key) ;

   private:
      typedef typename ancestor::single_lock single_lock ;
      typedef typename ancestor::bucket_lock bucket_lock ;
      typedef typename ancestor::lock_set    lock_set ;

      bool acquire_lock(const key_type &key, bool allow_wait) ;
} ;


/******************************************************************************/
/** Keyed reader/writer mutex.
*******************************************************************************/
template<typename Key,
         typename Hash = pcomn::hash_fn<Key>,
         typename Pred = std::equal_to<Key> >

class PTKeyedRWMutex : private PTKeyedMutexManager<Key, Hash, Pred, detail::srwlock<Key> > {

      typedef PTKeyedMutexManager<Key, Hash, Pred, detail::srwlock<Key> > ancestor ;
   public:
      typedef typename ancestor::key_type    key_type ;
      typedef typename ancestor::hasher      hasher ;
      typedef typename ancestor::key_equal   key_equal ;

      PTKeyedRWMutex(unsigned multiplexing, unsigned keypool_size) :
         ancestor(multiplexing, 2*keypool_size)
      {}

      explicit PTKeyedRWMutex(unsigned estimated_keycount) :
         ancestor(estimated_keycount <= 16 ? 1 : dprime_ubound((estimated_keycount + 15u)/16u), // multiplexing
                  std::max(estimated_keycount/16u, 1u)*32u) // keypool_size
      {}

      void lock_shared(const key_type &key) { acquire_rlock(key, true) ; }

      void lock(const key_type &key) { acquire_wlock(key, true) ; }

      bool try_lock_shared(const key_type &key) { return acquire_rlock(key, false) ; }

      bool try_lock(const key_type &key) { return acquire_wlock(key, false) ; }

      bool unlock(const key_type &key) ;

   private:
      typedef typename ancestor::single_lock single_lock ;
      typedef typename ancestor::bucket_lock bucket_lock ;
      typedef typename ancestor::lock_set    lock_set ;

      bool acquire_rlock(const key_type &key, bool allow_wait) ;
      bool acquire_wlock(const key_type &key, bool allow_wait) ;
} ;

/******************************************************************************/
/** Base lock guard for a keyed mutex
*******************************************************************************/
template<typename L>
class keyed_lock_guard {
      PCOMN_NONCOPYABLE(keyed_lock_guard) ;
      PCOMN_NONASSIGNABLE(keyed_lock_guard) ;
   public:
      typedef L                     lock_type ;
      typedef typename L::key_type  key_type ;

      lock_type *release()
      {
         lock_type *retval = _lock ;
         _lock = NULL ;
         return retval ;
      }

      lock_type *unlock()
      {
         lock_type *retval = release() ;
         if (retval)
            retval->unlock(_key) ;
         return retval ;
      }

   protected:
      keyed_lock_guard(lock_type *lock, const key_type &key) : _lock(lock), _key(key) {}
      ~keyed_lock_guard() { if (_lock) _lock->unlock(_key) ; }

      lock_type * _lock ;
      key_type    _key ;
} ;

/******************************************************************************/
/** shared_lock specialization for a keyed mutex.
*******************************************************************************/
template<typename K, typename H, typename P>
class shared_lock<PTKeyedRWMutex<K, H, P> > : public keyed_lock_guard<PTKeyedRWMutex<K, H, P> > {
      typedef keyed_lock_guard<PTKeyedRWMutex<K, H, P> > ancestor ;
   public:
      typedef typename ancestor::key_type    key_type ;
      typedef typename ancestor::lock_type   lock_type ;

      shared_lock(lock_type &lock, const key_type &key) :
         ancestor(&lock, key)
      {
         this->_lock->lock_shared(key) ;
      }
} ;

/*******************************************************************************
 PTKeyedMutexManager<Key>
*******************************************************************************/
template<typename Key, typename Hash, typename Pred, class SingleLock>
PTKeyedMutexManager<Key, Hash, Pred, SingleLock>::~PTKeyedMutexManager()
{}

/*******************************************************************************
 PTKeyedMutex<Key>
*******************************************************************************/
template<typename Key, typename Hash, typename Pred>
bool PTKeyedMutex<Key, Hash, Pred>::acquire_lock(const key_type &key, bool allow_wait)
{
   // Acquire bucket mutex
   bucket_lock locked (*this, key) ;

   // We optimize for low contention, hence
   // - create new lock object,
   // - attempt to insert to lockset
   // - use existent if failed (contention present)
   // instead of
   // - attempt to find in lockset
   // - create new lock object and insert, if failed (no contention)
   // The latter approach wastes a search in lockset if there is no contention;
   // the former wastes a creation of a new empty single-lock object if there _is_.
   locked._lock = locked._bucket.create_lock(key) ;

   NOXCHECK(locked._lock && locked._lock->_key == key && !locked._lock->_queue_size) ;

   const std::pair<typename lock_set::iterator, bool>
      inserted (locked._bucket._lockset.insert(locked._lock)) ;

   single_lock &keylock = **inserted.first ;
   if (inserted.second)
   {
      // Prevent it from destroying
      locked._lock = NULL ;

      NOXCHECK(!keylock._queue_size) ;
      keylock._queue_size = 1 ;
      // No contention, proceed
      return true ;
   }

   // We have a contention and will probably wait, so check if we are allowed to wait.
   if (!allow_wait)
      return false ;

   NOXCHECK(key == keylock._key) ;

   if (!keylock._writer_condvar)
   {
      NOXCHECK(keylock._queue_size == 1) ;
      keylock._writer_condvar = locked._bucket.checkout_condvar() ;
      NOXCHECK(keylock._writer_condvar) ;
   }

   keylock._queue_size += 2 ;

   // Wait on the condition variable until the keyed area is unlocked, or proceed
   // immediately if there is nobody inside the keyed area
   while (keylock._queue_size & 1)
      keylock.writer_condvar().wait(locked._bucket._mutex) ;

   NOXCHECK(keylock._queue_size) ;

   // Now keylock._queue_size is even. Make it odd (mark as locked) and proceed.
   --keylock._queue_size ;

   return false ;
}

template<typename Key, typename Hash, typename Pred>
bool PTKeyedMutex<Key, Hash, Pred>::unlock(const key_type &key)
{
   bucket_lock locked (*this, key) ;
   lock_set &lockset = locked._bucket._lockset ;

   const typename lock_set::iterator found = lockset.find(key) ;

   // There was no lock() at all for this key!
   if (unlikely(found == lockset.end()))
      return false ;

   single_lock &keylock = **found ;

   NOXCHECK(key == keylock._key) ;
   NOXCHECK(keylock._queue_size & 1) ;

   if (--keylock._queue_size)
      // More waiters in the queue
      keylock.writer_condvar().notify_one() ;

   else
   {
      // It was the last keylock owner
      lockset.erase(found) ;
      // 'locked._lock' will be destroyed by the destructor of 'locked'
      locked._lock = &keylock ;
   }

   return true ;
}

/*******************************************************************************
 PTKeyedRWMutex<Key>
*******************************************************************************/
#if 0
#define PCOMN_TRACE_KEYEDRWMUTEX(rw)                               \
   (printf(rw ". nreaders=%u, npending_readers=%u, qsize=%u\n",    \
           keylock._nreaders, keylock._npending_readers, keylock._queue_size) && \
    fflush(stdout))
#else
#define PCOMN_TRACE_KEYEDRWMUTEX(rw) ((void)0)
#endif

template<typename Key, typename Hash, typename Pred>
bool PTKeyedRWMutex<Key, Hash, Pred>::acquire_rlock(const key_type &key, bool allow_wait)
{
   // Acquire bucket mutex
   bucket_lock locked (*this, key) ;

   locked._lock = locked._bucket.create_lock(key) ;

   NOXCHECK(locked._lock && locked._lock->_key == key && !locked._lock->_queue_size) ;

   const std::pair<typename lock_set::iterator, bool>
      inserted (locked._bucket._lockset.insert(locked._lock)) ;

   single_lock &keylock = **inserted.first ;
   if (inserted.second)
   {
      // This is the first reader and there is no writers, proceed

      // Prevent it from destroying
      locked._lock = NULL ;

      NOXCHECK(!keylock._queue_size) ;
      NOXCHECK(!keylock._nreaders) ;
      NOXCHECK(!keylock._npending_readers) ;

      keylock._nreaders = 1 ;

      PCOMN_TRACE_KEYEDRWMUTEX("R enter new") ;

      return true ;
   }

   if (!keylock._queue_size || keylock._nreaders && !(keylock._npending_readers & 1))
   {
      NOXCHECK(!(keylock._queue_size & 1)) ;
      // Either the queue is empty or there are already readers there, let's join to the
      // crowd!
      ++keylock._nreaders ;

      PCOMN_TRACE_KEYEDRWMUTEX("R enter immediately") ;

      return true ;
   }

   // We have a contention and will probably wait, so check if we are allowed to wait.
   if (!allow_wait)
      return false ;

   NOXCHECK(key == keylock._key) ;

   if (!keylock._reader_condvar)
   {
      NOXCHECK(keylock._queue_size) ;
      keylock._reader_condvar = locked._bucket.checkout_condvar() ;
      NOXCHECK(keylock._reader_condvar) ;
   }

   NOXCHECK((keylock._npending_readers & 1) || keylock._nreaders == 0 && keylock._queue_size) ;

   keylock._npending_readers += 2 ;

   // Wait on the condition variable until there is only readers in the keyed area
   do
      keylock.reader_condvar().wait(locked._bucket._mutex) ;
   while ((keylock._npending_readers & 1) || (keylock._queue_size & 1)) ;

   ++keylock._nreaders ;
   keylock._npending_readers -= 2 ;

   PCOMN_TRACE_KEYEDRWMUTEX("R enter after wait") ;

   // Here "false" means "there was contention"
   return false ;
}

template<typename Key, typename Hash, typename Pred>
bool PTKeyedRWMutex<Key, Hash, Pred>::acquire_wlock(const key_type &key, bool allow_wait)
{
   bucket_lock locked (*this, key) ;

   locked._lock = locked._bucket.create_lock(key) ;

   NOXCHECK(locked._lock && locked._lock->_key == key && !locked._lock->_queue_size) ;

   const std::pair<typename lock_set::iterator, bool>
      inserted (locked._bucket._lockset.insert(locked._lock)) ;

   single_lock &keylock = **inserted.first ;
   if (inserted.second)
   {
      // Prevent it from destroying
      locked._lock = NULL ;

      NOXCHECK(!keylock._queue_size) ;
      keylock._queue_size = 1 ;

      PCOMN_TRACE_KEYEDRWMUTEX("W enter new") ;

      // No contention, proceed
      return true ;
   }

   if (!allow_wait)
      return false ;

   NOXCHECK(key == keylock._key) ;

   if (!keylock._writer_condvar)
   {
      NOXCHECK(keylock._queue_size == 1 || !keylock._queue_size && keylock._nreaders) ;
      keylock._writer_condvar = locked._bucket.checkout_condvar() ;
      NOXCHECK(keylock._writer_condvar) ;
   }

   keylock._queue_size += 2 ;

   // Wait on the condition variable until the keyed area is unlocked, or proceed
   // immediately if there is nobody inside the keyed area
   while (keylock._nreaders ||
          (keylock._queue_size & 1) ||
          keylock._npending_readers && !(keylock._npending_readers & 1))

      keylock.writer_condvar().wait(locked._bucket._mutex) ;

   // Now keylock._queue_size is even. Make it odd (mark as locked) and proceed.
   --keylock._queue_size ;
   // Let readers again
   keylock._npending_readers &= ~1u ;

   PCOMN_TRACE_KEYEDRWMUTEX("W enter") ;

   return false ;
}

template<typename Key, typename Hash, typename Pred>
bool PTKeyedRWMutex<Key, Hash, Pred>::unlock(const key_type &key)
{
   bucket_lock locked (*this, key) ;
   lock_set &lockset = locked._bucket._lockset ;

   const typename lock_set::iterator found = lockset.find(key) ;

   // There was no lock() at all for this key!
   if (unlikely(found == lockset.end()))
      return false ;

   single_lock &keylock = **found ;

   NOXCHECK(key == keylock._key) ;

   // Are we reader or writer?
   if (!keylock._nreaders)
   {
      // We are the writer
      NOXCHECK(keylock._queue_size & 1) ;
      // Unlock
      --keylock._queue_size ;

      PCOMN_TRACE_KEYEDRWMUTEX("W exit") ;

      if (keylock._npending_readers)
      {
         NOXCHECK(!(keylock._npending_readers & 1)) ;
         // Allow readers run
         keylock.reader_condvar().notify_all() ;
      }
      else if (keylock._queue_size)
         // More waiting writers in the queue; allow outstanding writers run
         keylock.writer_condvar().notify_one() ;
      else
      {
         // It was the last keylock owner
         lockset.erase(found) ;
         // Let the locked_bucket destructor to delete this keylock
         locked._lock = &keylock ;
      }
   }
   else
   {
      // We are the reader. Can/need we unlock waiting writers?
      --keylock._nreaders ;

      PCOMN_TRACE_KEYEDRWMUTEX("R exit") ;

      if (keylock._queue_size)
      {
         // There are waiting writers
         NOXCHECK(!(keylock._queue_size & 1)) ;

         if (keylock._nreaders)
            // Give writers a chance
            keylock._npending_readers |= 1 ;
         else
            keylock.writer_condvar().notify_one() ;
      }
      else if (!keylock._nreaders)
      {
         // It was the last keylock owner
         lockset.erase(found) ;
         // Let the locked_bucket destructor to delete this keylock
         locked._lock = &keylock ;
      }
   }

   return true ;
}
} // end of namespace pcomn

namespace std {
/******************************************************************************/
/** std::lock_guard specialization for a keyed mutex.
*******************************************************************************/
template<typename K, typename H, typename P>
class lock_guard<pcomn::PTKeyedMutex<K, H, P> > : public pcomn::keyed_lock_guard<pcomn::PTKeyedMutex<K, H, P> > {
      typedef pcomn::keyed_lock_guard<pcomn::PTKeyedMutex<K, H, P> > ancestor ;
   public:
      using typename ancestor::key_type ;
      using typename ancestor::lock_type ;

      lock_guard(lock_type &lock, const key_type &key) : ancestor(&lock, key)
      {
         this->_lock->lock(key) ;
      }
} ;

/******************************************************************************/
/** std::lock_guard specialization for a keyed shared mutex.
*******************************************************************************/
template<typename K, typename H, typename P>
class lock_guard<pcomn::PTKeyedRWMutex<K, H, P> > : public pcomn::keyed_lock_guard<pcomn::PTKeyedRWMutex<K, H, P> > {
      typedef pcomn::keyed_lock_guard<pcomn::PTKeyedRWMutex<K, H, P> > ancestor ;
   public:
      using typename ancestor::key_type ;
      using typename ancestor::lock_type ;

      lock_guard(lock_type &lock, const key_type &key) : ancestor(&lock, key)
      {
         this->_lock->lock(key) ;
      }
} ;
} // end of namespace std

#endif /* __PCOMN_KEYEDMUTEX_H */
