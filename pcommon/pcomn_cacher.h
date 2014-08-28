/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_CACHER_H
#define __PCOMN_CACHER_H
/*******************************************************************************
 FILE         :   pcomn_cacher.h
 COPYRIGHT    :   Yakov Markovitch, 2009-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Generic LRU cacher templates.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   8 Oct 2009
*******************************************************************************/
/** @file
    Generic LRU cacher.
*******************************************************************************/
#include <pcomn_syncobj.h>
#include <pcomn_hashclosed.h>
#include <pcomn_incdlist.h>
#include <pcomn_function.h>
#include <pcomn_alloca.h>

#include <algorithm>
#include <functional>
#include <memory>

namespace pcomn {

/******************************************************************************/
/** Generic cacher template.

 Implements LRU eviction
*******************************************************************************/
template<typename Value,
         typename ExtractKey = pcomn::identity<Value>,
         typename Hash = pcomn::hash_fn<PCOMN_NOREF_RETTYPE(ExtractKey(Value))>,
         typename Pred = std::equal_to<PCOMN_NOREF_RETTYPE(ExtractKey(Value))> >
class cacher {
   public:
      typedef PCOMN_NOREF_RETTYPE(ExtractKey(Value)) key_type ;
      typedef Value       value_type ;
      typedef Hash        hasher ;
      typedef Pred        key_equal ;
      typedef ExtractKey  key_extract ;

      explicit cacher(size_t szlimit = (size_t)-1) :
         _szlimit(szlimit)
      {}

      explicit cacher(const hasher &hf, size_t szlimit = (size_t)-1) :
         _szlimit(szlimit),
         _cache(-1, hf)
      {}

      cacher(const hasher &hf, const key_equal &eq, size_t szlimit = (size_t)-1) :
         _szlimit(szlimit),
         _cache(-1, hf, eq)
      {}

      ~cacher() ;

      /// Get an item from cache.
      /// @param key          Cached entry key.
      /// @param found_item   Buffer to place the found cache entry to.
      /// @param touch        Whether to move the accessed element to the head of the LRU
      /// list.
      /// @note If there is no cache entry with name @a key found, @a found_item is left
      /// unchanged.
      /// @return true, if the item is present in the cache; false otherwise.
      bool get(const key_type &key, value_type &found_item, bool touch = true) const
      {
         PCOMN_SCOPE_LOCK (guard, _lock) ;
         return get_unlocked(key, &found_item, touch) ;
      }

      /// Get an item from cache.
      /// @param key          Cached entry key.
      /// @param touch        Whether to move the accessed element to the head of the LRU
      /// list.
      /// @return An item from the cache, or a default-constructed value_type object, if
      /// an item with @a key is not present in the cache.
      value_type get(const key_type &key, bool touch = true) const
      {
         value_type result ;
         get(key, result, touch) ;
         return result ;
      }

      /// Put a value into the cache, if there is no value with the same key in the cache
      /// yet
      bool put(const value_type &new_item, bool touch = true)
      {
         return put_locked(new_item, NULL, touch) ;
      }

      bool put(const value_type &new_item, value_type &found_item, bool touch = true)
      {
         return put_locked(new_item, &found_item, touch) ;
      }

      /// Replace a value in the cache; if there is no value with such key, insert the value
      bool replace(const value_type &new_item) ;

      bool exists(const key_type &key) const
      {
         PCOMN_SCOPE_LOCK (guard, _lock) ;
         return get_unlocked(key, NULL, false) ;
      }

      /// Discard an item from the cache
      /// @param key          Cached entry key.
      /// @return true, if the item was present in the cache and has been deleted; false
      /// otherwise.
      bool erase(const key_type &key) ;

      /// Discard a set of items from the cache
      ///
      /// @return the count of discarded items
      ///
      template<typename InputIterator>
      size_t erase(InputIterator begin, InputIterator end) ;

      /// Erase all elements from the cache
      size_t clear()
      {
         size_t count ;
         lru_list removed_entries ;
         {
            PCOMN_SCOPE_LOCK(guard, _lock) ;

            count = _cache.size() ;
            _lru.swap(removed_entries) ;
            _cache.clear() ;
         }
         // Destroy entries outside of a critical section
         destroy_entries(removed_entries) ;
         return count ;
      }

      /// Get the current cache size
      size_t size() const ;

      /// Get the cache size limit
      size_t size_limit() const { return _szlimit ; }

      /// Set the cache size limit.
      ///
      /// If the new size limit is less than size(), removes the extra items from the
      /// cache.
      /// @return Current cache size, i.e. the count of remaining cache entries.
      size_t set_size_limit(size_t count) ;

      /// Get all cacher keys in LRU order (more precise, in "reverse LRU" - recently used
      /// first).
      /// @note Avoid excessive use of this function: the cache stay locked while it is
      /// being traversed.
      template<typename OutputIterator>
      OutputIterator keys(OutputIterator out) const
      {
         key_vector result ;
         retrieve_keys(result) ;
         return std::copy(result.begin(), result.end(), out) ;
      }

   private:
      typedef std::vector<typename std::remove_cv<typename std::remove_reference<key_type>::type>::type> key_vector ;

      /*************************************************************************
       Cache entry: a node of an LRU list containing data_type item
      *************************************************************************/
      struct entry_type {
            entry_type() : _value() {}

            explicit entry_type(const value_type &v) : _value(v) {}

            const value_type &value() const { return _value ; }
            value_type &value() { return _value ; }

            mutable incdlist_node   _node ;
         private:
            value_type              _value ;

            PCOMN_NONCOPYABLE(entry_type) ;
            PCOMN_NONASSIGNABLE(entry_type) ;
      } ;

      // Declare the inclusive list of entry_type nodes
      typedef incdlist<entry_type, &entry_type::_node> lru_list ;

      typedef typename std::result_of<key_extract(value_type)>::type key_result ;

      struct entry_key_extract : std::unary_function<const entry_type *, key_result> {
            entry_key_extract() {}

            entry_key_extract(const key_extract &extract) :
               extract_key(extract)
            {}

            key_result operator()(const entry_type *entry) const
            {
               NOXCHECK(entry) ;
               return extract_key(entry->value()) ;
            }

            key_extract extract_key ;
      } ;

      typedef closed_hashtable<entry_type *, entry_key_extract, hasher, key_equal> cache_data ;

#ifdef PCOMN_MD_MT
      typedef std::recursive_mutex lock_type ;
#else
      typedef PVoidMutex lock_type ;
#endif

   private:
      mutable lock_type _lock ;
      size_t            _szlimit ; /* Cache size (item count) limit; by reaching this
                                    * limit, the cache starts evicting entries */
      mutable lru_list  _lru ;
      cache_data        _cache ;

      bool put_locked(const value_type &item, value_type *found_item, bool touch) ;

      bool get_unlocked(const key_type &key, value_type *found_item, bool touch) const ;
      void insert_unlocked(const value_type &item, value_type *found_item)
      {
         _lru.push_front(**_cache.insert(new entry_type(item)).first) ;
         if (found_item)
            *found_item = _lru.front().value() ;
      }
      entry_type *remove_unlocked(const key_type &key)
      {
         entry_type *erased = NULL ;
         if (_cache.erase(key, erased))
         {
            NOXCHECK(erased) ;
            _lru.erase(*erased) ;
         }
         return erased ;
      }

      void handle_existing_entry(const entry_type *entry, value_type *found_item, bool touch) const
      {
         NOXCHECK(entry) ;
         if (touch)
            // The entry is _the_ most recent used one, move it to the head of the LRU list
            _lru.push_front(*const_cast<entry_type *>(entry)) ;

         if (found_item)
            *found_item = entry->value() ;
      }

      entry_type **cleanup_cache(entry_type **discarded) noexcept ;

      // Get count of items to remove
      size_t cleanup_required() const
      {
         const size_t cachesz = _cache.size() ;
         return cachesz > size_limit()
            ? std::max<size_t>(cachesz/3, cachesz - std::max<size_t>(size_limit(), 1) + 1)
            : 0 ;
      }

      key_vector &retrieve_keys(key_vector &result) const ;

      static void destroy_entries(lru_list &lru)
      {
         while(!lru.empty())
            delete &lru.back() ;
      }
} ;

/*******************************************************************************
 cacher
*******************************************************************************/
template<typename V, typename X, typename H, typename P>
cacher<V, X, H, P>::~cacher() { destroy_entries(_lru) ; }

template<typename V, typename X, typename H, typename P>
bool cacher<V, X, H, P>::get_unlocked(const key_type &key, value_type *found_item, bool touch) const
{
   typename cache_data::const_iterator entry (_cache.find(key)) ;

   if (entry == _cache.end())
      return false ;

   handle_existing_entry(*entry, found_item, touch) ;
   return true ;
}

template<typename V, typename X, typename H, typename P>
bool cacher<V, X, H, P>::put_locked(const value_type &item, value_type *found_item, bool touch)
{
   entry_type **begin_discarded, **end_discarded ;
   {
      PCOMN_SCOPE_LOCK (guard, _lock) ;

      auto item_iter = _cache.find(_cache.key_get().extract_key(item)) ;
      if (item_iter != _cache.end())
      {
         // Already in the cache
         handle_existing_entry(*item_iter, found_item, touch) ;
         return false ;
      }

      P_FAST_BUFFER(discarded, entry_type *, cleanup_required(), 64*KiB) ;
      begin_discarded = discarded ;
      end_discarded = cleanup_cache(discarded) ;

      if (size_limit())
         insert_unlocked(item, found_item) ;
      else if (found_item)
         *found_item = item ;
   }
   std::for_each(begin_discarded, end_discarded, std::default_delete<entry_type>()) ;

   return true ;
}

template<typename V, typename X, typename H, typename P>
bool cacher<V, X, H, P>::replace(const value_type &item)
{
   entry_type **begin_discarded, **end_discarded ;
   bool erased ;
   {
      PCOMN_SCOPE_LOCK (guard, _lock) ;
      P_FAST_BUFFER(discarded, entry_type *, cleanup_required() + 1, 64*KiB) ;

      begin_discarded = discarded ;
      *discarded = remove_unlocked(_cache.key_get().extract_key(item)) ;
      erased = !!*discarded ;
      discarded += erased ;
      end_discarded = cleanup_cache(discarded) ;

      insert_unlocked(item, NULL) ;
   }
   std::for_each(begin_discarded, end_discarded, std::default_delete<entry_type>()) ;

   return erased ;
}

template<typename V, typename X, typename H, typename P>
bool cacher<V, X, H, P>::erase(const key_type &key)
{
   entry_type *erased ;
   {
      PCOMN_SCOPE_LOCK (guard, _lock) ;
      erased = remove_unlocked(key) ;
   }
   delete erased ;
   return !!erased ;
}

template<typename V, typename X, typename H, typename P>
template<typename InputIterator>
__noinline size_t cacher<V, X, H, P>::erase(InputIterator begin, InputIterator end)
{
   _lock.lock() ;
   if (_cache.empty() || begin == end)
   {
      _lock.unlock() ;
      return 0 ;
   }

   P_FAST_BUFFER(discarded, entry_type *, _cache.size(), 64*KiB) ;
   entry_type **next_discarded = discarded ;
   do {
      *next_discarded = remove_unlocked(*begin) ;
      next_discarded += !!*next_discarded ;
   }
   while(++begin != end) ;

   _lock.unlock() ;

   // We removed entries both from the hashtable and LRU list, so we can delete them
   // outside the critical section
   const size_t discarded_count = next_discarded - discarded ;
   std::for_each(discarded, next_discarded, std::default_delete<entry_type>()) ;

   return discarded_count ;
}

template<typename V, typename X, typename H, typename P>
size_t cacher<V, X, H, P>::size() const
{
   PCOMN_SCOPE_LOCK (guard, _lock) ;
   return _cache.size() ;
}

template<typename V, typename X, typename H, typename P>
__noinline size_t cacher<V, X, H, P>::set_size_limit(size_t limit)
{
   size_t newsz ;
   entry_type **begin_discarded, **end_discarded ;
   {
      PCOMN_SCOPE_LOCK(guard, _lock) ;

      _szlimit = limit ;

      const size_t cleanup_count = cleanup_required() ;
      NOXCHECK(cleanup_count <= _cache.size()) ;

      P_FAST_BUFFER(discarded, entry_type *, cleanup_count, 64*KiB) ;

      begin_discarded = discarded ;
      end_discarded = cleanup_cache(begin_discarded) ;

      NOXCHECK(end_discarded - begin_discarded <= (long)cleanup_count) ;

      newsz = _cache.size() ;
      NOXCHECK(newsz <= _szlimit) ;
   }
   std::for_each(begin_discarded, end_discarded, std::default_delete<entry_type>()) ;

   return newsz ;
}

template<typename V, typename X, typename H, typename P>
auto cacher<V, X, H, P>::cleanup_cache(entry_type **discarded) noexcept -> entry_type **
{
   for (size_t remove_count = cleanup_required() ; remove_count ; --remove_count)
   {
      NOXCHECK(!_lru.empty()) ;
      entry_type * const entry = &_lru.back() ;
      NOXVERIFY(_cache.erase_value(entry)) ;
      _lru.erase(*entry) ;
      *discarded++ = entry ;
   }

   return discarded ;
}

template<typename V, typename X, typename H, typename P>
auto cacher<V, X, H, P>::retrieve_keys(key_vector &result) const -> key_vector &
{
   result.clear() ;

   PCOMN_SCOPE_LOCK (guard, _lock) ;
   result.reserve(size()) ;
   for (typename lru_list::const_iterator i (_lru.begin()), e (_lru.end()) ; i != e ; ++i)
      result.push_back(_cache.key_get().extract_key(i->value())) ;
   return result ;
}

} // end of namespace pcomn

#endif /* __PCOMN_CACHER_H */
