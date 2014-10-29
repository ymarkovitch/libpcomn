/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_POOL_H
#define __PCOMN_POOL_H
/*******************************************************************************
 FILE         :   pcomn_pool.h
 COPYRIGHT    :   Yakov Markovitch, 2010-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Generic pools.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   17 May 2010
*******************************************************************************/
/** @file
    Generic pools.
*******************************************************************************/
#include <pcomn_syncobj.h>
#include <pcomn_atomic.h>
#include <pcomn_hashclosed.h>
#include <pcomn_incdlist.h>
#include <pcomn_calgorithm.h>

#include <algorithm>
#include <functional>
#include <memory>

namespace pcomn {

/******************************************************************************/
/** Keyed pool with LRU eviction
*******************************************************************************/
template<typename Key, typename Value,
         typename Hash = pcomn::hash_fn<Key>, typename Pred = std::equal_to<Key> >
class keyed_pool {
   public:
      typedef Key          key_type ;
      typedef Value        value_type ;
      typedef value_type   mapped_type ;
      typedef Hash         hasher ;
      typedef Pred         key_equal ;

      /// Create a keyed pool with specified size limit, hasher, and key equality
      /// predicate
      /// @param szlimit
      /// @param hf
      /// @param keq
      explicit keyed_pool(size_t szlimit,
                          const hasher &hf = hasher(),
                          const key_equal &keq = key_equal()) :
         _size(0),
         _size_limit(szlimit),
         _emptykey_count(0),
         _data(-1, hf, keq)
      {}

      ~keyed_pool() { clear() ; }

      /// Erase all items from the pool
      /// @return Count of erased items
      __noinline size_t clear() ;

      /// Put an item into the pool.
      ///
      void put(const key_type &key, const value_type &value)
      {
         cleanup(LOCKED) ;
         PCOMN_SCOPE_LOCK (pool_guard, _lock) ;
         put_unlocked(key, value) ;
      }

      /// Move an item into the pool.
      ///
      void checkin(const key_type &key, value_type &value)
      {
         cleanup(LOCKED) ;
         PCOMN_SCOPE_LOCK (pool_guard, _lock) ;
         checkin_unlocked(key, value) ;
      }

      /// Extract an item from the pool.
      ///
      /// Deletes the exrtacted item from the pool.
      /// @param key          Item key.
      /// @param found_item   Buffer to place the found pooled item to.
      /// @note If there is no item with @a key found, @a found_item is left unchanged.
      /// @return true, if the item was found and extracted; false otherwise.
      bool checkout(const key_type &key, value_type &found_item)
      {
         PCOMN_SCOPE_LOCK (pool_guard, _lock) ;
         return checkout_unlocked(key, found_item) ;
      }

      /// Discard an item with specified key from the pool.
      /// @return The count of discarded items
      size_t erase(const key_type &key) ;

      /// Get the current pool size
      size_t size() const { return _size ; }

      /// Get the current pool key count
      size_t key_count() const
      {
         PCOMN_SCOPE_LOCK (pool_guard, _lock) ;
         return _data.size() ;
      }

      /// Get the pool size limit
      size_t size_limit() const { return _size_limit ; }

      size_t keysize_limit() const ;

      /// Set the pool size limit.
      ///
      /// If the new size limit is less than size(), removes the extra items from the
      /// pool.
      /// @return Current pool size, i.e. the count of remaining cache entries.
      size_t set_size_limit(size_t count) ;

      /// Get all pool keys together with item counts
      /// @note This is a debugging function, it has complexity O(n). Avoid using this
      /// function in production code.
      template<typename OutputIterator>
      OutputIterator keys(OutputIterator out) const
      {
         std::vector<std::pair<key_type, size_t> > result ;
         retrieve_keys(result) ;
         return std::copy(result.begin(), result.end(), out) ;
      }

   private:
      /*************************************************************************
       Value entry: a node of an LRU list containing data_type item
      *************************************************************************/
      struct value_entry {
            value_entry() : _value() {}
            explicit value_entry(const value_type &v) : _value(v) {}

            const value_type &value() const { return _value ; }
            value_type &value() { return _value ; }

            mutable incdlist_node   _entry_node ;
            mutable incdlist_node   _lru_node ;
         private:
            value_type _value ;

            PCOMN_NONCOPYABLE(value_entry) ;
            PCOMN_NONASSIGNABLE(value_entry) ;
      } ;

      typedef incdlist_managed<value_entry, &value_entry::_entry_node> item_list ;

      /*************************************************************************
       Key entry: a head node of a list containing pool items
      *************************************************************************/
      struct key_entry {
            explicit key_entry(const key_type &k) : _key(k) {}

            const key_type &key() const { return _key ; }

            const item_list &items() const { return _items ; }
            item_list &items() { return _items ; }
         private:
            const key_type _key ;
            item_list      _items ;

            PCOMN_NONCOPYABLE(key_entry) ;
            PCOMN_NONASSIGNABLE(key_entry) ;
      } ;

      typedef incdlist<value_entry, &value_entry::_lru_node> lru_list ;

      typedef closed_hashtable<key_entry *, extract_key<key_entry *>, hasher, key_equal> pool_data ;
      typedef std::recursive_mutex lock_type ;

   private:
      mutable lock_type _lock ;
      size_t            _size ;
      size_t            _size_limit ; /* Item count limit; by reaching this
                                       * the pool starts evicting entries */
      size_t            _emptykey_count ;

      lru_list    _lru ;
      pool_data   _data ;

      enum Locking { UNLOCKED, LOCKED } ;

      bool checkout_unlocked(const key_type &key, value_type &item) ;
      void checkin_unlocked(const key_type &key, value_type &item) ;
      void put_unlocked(const key_type &key, const value_type &item) ;

      void checkout_from_entry(key_entry *entry, value_type &result) ;

      void cleanup(Locking) ;
      void cleanup_emptykeys() ;

      void retrieve_keys(std::vector<std::pair<key_type, size_t> > &result) const ;

      key_entry *provide_entry(const key_type &key, std::unique_ptr<key_entry> &guard) ;
      void save_entry(key_entry *entry, value_entry *data, std::unique_ptr<key_entry> &guard) ;
} ;

/*******************************************************************************
 keyed_pool
*******************************************************************************/
template<typename K, typename V, typename H, typename P>
size_t keyed_pool<K, V, H, P>::clear()
{
   PCOMN_SCOPE_LOCK (pool_guard, _lock) ;
   if (_data.empty())
      return 0 ;
   const size_t oldsize = _size ;
   // The hashtable of pointers won't itself delete objects those pointers point to
   clear_icontainer(_data) ;
   NOXCHECK(_lru.empty()) ;
   _size = 0 ;
   _emptykey_count = 0 ;
   swap_clear(_data) ;
   return oldsize ;
}

template<typename K, typename V, typename H, typename P>
inline void keyed_pool<K, V, H, P>::checkout_from_entry(key_entry *entry, value_type &result)
{
   NOXCHECK(entry) ;
   NOXCHECK(!entry->items().empty()) ;

   pcomn_swap(entry->items().front().value(), result) ;
   if (!entry->items().front()._entry_node.is_only())
      entry->items().pop_front() ;
   else
   {
      // This is the last item with the given key in the pool, remove the key itself
      key_entry *deleted_entry = NULL ;
      _data.erase(entry->key(), deleted_entry) ;
      NOXCHECK(deleted_entry == entry) ;
      delete deleted_entry ;
   }
   --_size ;
}

template<typename K, typename V, typename H, typename P>
bool keyed_pool<K, V, H, P>::checkout_unlocked(const key_type &key, value_type &found_item)
{
   const typename pool_data::iterator entry (_data.find(key)) ;

   if (entry == _data.end() || (*entry)->items().empty())
      return false ;

   checkout_from_entry(*entry, found_item) ;
   return true ;
}

template<typename K, typename V, typename H, typename P>
inline typename keyed_pool<K, V, H, P>::key_entry *
keyed_pool<K, V, H, P>::provide_entry(const key_type &key, std::unique_ptr<key_entry> &guard)
{
   const typename pool_data::iterator ientry (_data.find(key)) ;
   key_entry *result ;

   if (ientry != _data.end())
      result = *ientry ;
   else
      guard.reset(result = new key_entry(key)) ;
   return result ;
}

template<typename K, typename V, typename H, typename P>
inline void keyed_pool<K, V, H, P>::save_entry(key_entry *entry, value_entry *data, std::unique_ptr<key_entry> &guard)
{
   NOXCHECK(entry && data && (!guard || guard.get() == entry)) ;

   entry->items().push_front(*data) ;
   _lru.push_back(*data) ;

   if (guard)
   {
      // There is a guard for the entry, hence the entry is new
      NOXVERIFY(_data.insert(entry).second) ;
      guard.release() ;
   }
   ++_size ;
}

template<typename K, typename V, typename H, typename P>
void keyed_pool<K, V, H, P>::put_unlocked(const key_type &key, const value_type &item)
{
   cleanup(UNLOCKED) ;
   if (_size >= _size_limit)
      return ;

   std::unique_ptr<key_entry> entry_guard ;
   key_entry * const entry = provide_entry(key, entry_guard) ;
   // Using plain pointer is safe, none of the following operations can throw exception
   // before the new value_entry is "hooked" into the corresponding key_entry
   value_entry * const data = new value_entry(item) ;

   save_entry(entry, data, entry_guard) ;
}

template<typename K, typename V, typename H, typename P>
void keyed_pool<K, V, H, P>::checkin_unlocked(const key_type &key, value_type &item)
{
   cleanup(UNLOCKED) ;
   if (_size >= _size_limit)
   {
      swap_clear(item) ;
      return ;
   }

   std::unique_ptr<value_entry> data (new value_entry) ;
   pcomn_swap(data->value(), item) ;

   std::unique_ptr<key_entry> entry_guard ;
   key_entry * const entry = provide_entry(key, entry_guard) ;

   save_entry(entry, data.release(), entry_guard) ;
}

template<typename K, typename V, typename H, typename P>
size_t keyed_pool<K, V, H, P>::erase(const key_type &key)
{
   PCOMN_SCOPE_LOCK (pool_guard, _lock) ;
   key_entry *erased = NULL ;
   if (!_data.erase(key, erased))
      return 0 ;

   NOXCHECK(erased) ;
   const size_t erased_count = erased->items().size() ;
   NOXCHECK(erased_count && erased_count <= _size) ;
   delete erased ;
   _size -= erased_count ;
   return erased_count ;
}

template<typename K, typename V, typename H, typename P>
size_t keyed_pool<K, V, H, P>::set_size_limit(size_t limit)
{
   PCOMN_SCOPE_LOCK (pool_guard, _lock) ;
   _size_limit = limit ;
   cleanup(UNLOCKED) ;
   return size() ;
}

template<typename K, typename V, typename H, typename P>
void keyed_pool<K, V, H, P>::retrieve_keys(std::vector<std::pair<key_type, size_t> > &result) const
{
   swap_clear(result) ;

   PCOMN_SCOPE_LOCK (pool_guard, _lock) ;
   result.reserve(size()) ;
   for (typename pool_data::const_iterator i (_data.begin()), e (_data.end()) ; i != e ; ++i)
      result.push_back(std::pair<key_type, size_t>((*i)->key(), (*i)->items().size())) ;
}

template<typename K, typename V, typename H, typename P>
inline void keyed_pool<K, V, H, P>::cleanup_emptykeys()
{
   if (_emptykey_count <= _data.size()/2)
      return ;
   typedef typename pool_data::iterator data_iterator ;
   for (data_iterator i = _data.begin(), e = _data.end() ; i != e ;)
   {
      const data_iterator d (i++) ;
      if ((*d)->items().empty())
         _data.erase(d) ;
   }
   _emptykey_count = 0 ;
}

template<typename K, typename V, typename H, typename P>
void keyed_pool<K, V, H, P>::cleanup(Locking locking)
{
   // Move all stale pool items into a local list and destroy them out of the lock scope,
   // in order to not to hold a lock during items destruction. This can be important if
   // pool items have potentially long destruction time (e.g., must close files, etc.)
   if (locking)
      _lock.lock() ;

   cleanup_emptykeys() ;
   if (!_size || _size < _size_limit)
   {
      if (locking)
         _lock.unlock() ;
      return ;
   }
   item_list erasable ;
   for (const size_t final = _size_limit - midval<size_t>(1, _size_limit, _size_limit/4) ; _size > final ; --_size)
   {
      value_entry &entry = _lru.front() ;
      if (entry._entry_node.is_only())
         ++_emptykey_count ;
      erasable.push_back(entry) ;
      _lru.remove(entry) ;
   }

   if (locking)
      _lock.unlock() ;

   // Here, out of lock scope, the destructor of 'erasable' will be called; since
   // erasable is a managed list, it will delete all its contents
}

} // end of namespace pcomn

#endif /* __PCOMN_POOL_H */
