/*-*- tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_hashtable.cc
 COPYRIGHT    :   Yakov Markovitch, 1999-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Hash table implementation

 CREATION DATE:   10 Dec 1999
*******************************************************************************/
#include <pcomn_diag.h>
#include <pcomn_algorithm.h>
#include <pcomn_function.h>
#include <pcomn_numeric.h>
#include <algorithm>

namespace pcomn {

const int P_LONG_BITS = 8*sizeof(unsigned long) ;

/*******************************************************************************
 hashtable_entry
*******************************************************************************/
template <class Key, class Value>
hashtable_entry<Key, Value>::hashtable_entry(const key_type &key, const value_type &value) :
   ancestor(key, value)
{}

template <class Key, class Value>
hashtable_entry<Key, Value>::~hashtable_entry()
{}

/*******************************************************************************
 hashtable
*******************************************************************************/
template <class Key, class Value, class Hash, class Comparator, class Locker>
inline void hashtable<Key, Value, Hash, Comparator, Locker>::create_buckets(size_t initsize)
{
   _create_buckets(dprime_ubound(initsize)) ;
}

template <class Key, class Value, class Hash, class Comparator, class Locker>
void hashtable<Key, Value, Hash, Comparator, Locker>::_create_buckets(size_t initsize)
{
   // Allocate one spare bucket to provide the end sentry bucket
   _buckets = new bucket_list_node[initsize + 1] ;
   _virgins = _capacity = initsize ;
   // Short circuit the end sentry bucket
   _buckets_end()->next(_buckets_end()) ;
}

/*******************************************************************************
 hashtable
*******************************************************************************/
template <class Key, class Value, class Hash, class Comparator, class Locker>
hashtable<Key, Value, Hash, Comparator, Locker>::hashtable(size_t initsize) :
   _hasher(),
   _comparator()
{
   _init(initsize) ;
}

template <class Key, class Value, class Hash, class Comparator, class Locker>
hashtable<Key, Value, Hash, Comparator, Locker>::hashtable(const Hash &hasher,
                                                           const Comparator &comparator,
                                                           size_t initsize) :
   _hasher(hasher),
   _comparator(comparator)
{
   _init(initsize) ;
}

template <class Key, class Value, class Hash, class Comparator, class Locker>
hashtable<Key, Value, Hash, Comparator, Locker>::hashtable(const hashtable &src) :
   _hasher(src._hasher),
   _comparator(src._comparator)
{
   read_lock _rlock (src.lock()) ;
   size_t initsize (src.size()) ;
   _init(initsize) ;
   try {
      for (const_iterator iter (const_cast<hashtable &>(src)) ; _size != initsize ; ++_size, ++iter)
      {
         const _entry_t &srcentry (*iter._entry()) ;
         bucket_list_node * const bucket = _buckets + location(srcentry.hash_value()) ;
         ::new (_alloc.allocate()) _entry_t(srcentry, bucket) ;
         _touch_bucket(bucket) ;
      }
   }
   catch(...) {
      _destroy_hashtable() ;
   }
}

template <class Key, class Value, class Hash, class Comparator, class Locker>
void hashtable<Key, Value, Hash, Comparator, Locker>::_destroy_hashtable()
{
   TRACEPX(PCOMN_Hashtbl, DBGL_ALWAYS, (void *)this << ": destroying hash table."
           << " size=" << size() << " buckets=" << capacity()) ;
   _clear() ;
   bucket_list_node *buckets = _buckets_start() ;
   _buckets = NULL ;
   _virgins = _size = _capacity = 0 ;
   delete [] buckets ;
}

template <class Key, class Value, class Hash, class Comparator, class Locker>
bucket_list_node *hashtable<Key, Value, Hash, Comparator, Locker>::_closest(bucket_list_node *bucket)
{
   NOXCHECK(bucket) ;
   // We have to find the closest entry, i.e. the closest item in the list
   // that is actually an entry with data (i.e. an object of type _entry_t).
   // Empty buckets and touched entries are not such entries; they are buckets
   // (<bucket_list_node>s) from the _buckets vector.
   while(bucket->is_empty_bucket() || bucket->touched())
      bucket = bucket->next() ;
   return bucket ;
}

template <class Key, class Value, class Hash, class Comparator, class Locker>
size_t hashtable<Key, Value, Hash, Comparator, Locker>::clear(size_t initsize)
{
   write_lock _lock(*this) ;
   size_t sz = size() ;
   _clear() ;
   _size = 0 ;
   create_buckets(initsize) ;
   return sz ;
}

template <class Key, class Value, class Hash, class Comparator, class Locker>
void hashtable<Key, Value, Hash, Comparator, Locker>::_init(size_t initsize)
{
   write_lock _lock(*this) ;
   _buckets = NULL ;
   _capacity = _size = _virgins = 0 ;
   create_buckets(initsize) ;
   TRACEPX(PCOMN_Hashtbl, DBGL_ALWAYS, (void *)this << ": hash table created. required_size="
           << initsize << " capacity=" << capacity()) ;
}

template <class Key, class Value, class Hash, class Comparator, class Locker>
void hashtable<Key, Value, Hash, Comparator, Locker>::_clear()
{
   for (bucket_list_node *bucket = _buckets_start(), *end = _buckets_end() ; bucket != end ; ++bucket)
      while (!bucket->is_empty_bucket())
         _destroy(static_cast<_entry_t *>(bucket->remove_next())) ;
}

template <class Key, class Value, class Hash, class Comparator, class Locker>
void hashtable<Key, Value, Hash, Comparator, Locker>::_resize()
{
   size_t newsize = dprime_ubound(2*size()) ;
   TRACEPX(PCOMN_Hashtbl, DBGL_HIGHLEV, (void *)this << ": resizing hash table."
           << " size=" << size() << " buckets=" << capacity()
           << " virgins=" << _virgins << " newsize=" << newsize) ;

   if (newsize != _capacity)
   {
      // Yes, we need real resizing
      bucket_list_node *old_buckets = _buckets_start() ;
      bucket_list_node *bucket = _buckets ;
      bucket_list_node *end_buckets = bucket + capacity() ;

      for (_create_buckets(newsize) ; bucket != end_buckets ; ++bucket)
         while (!bucket->is_empty_bucket())
         {
            bucket_list_item *item = static_cast<bucket_list_item *>(bucket->remove_next()) ;
            bucket_list_node *new_bucket = _buckets + location(item->hash_value()) ;
            new_bucket->insert_next(item) ;
            _touch_bucket(new_bucket) ;
         }
      delete [] old_buckets ;
   }
   else
      // Needn't resizing at all, just clean up touched info
      for (bucket_list_node *node = _buckets, *end = _buckets + capacity() ;
           node != end ;
           ++node)
         if (node->is_empty_bucket())
            _virgins += node->touch(false) ;
}

template <class Key, class Value, class Hash, class Comparator, class Locker>
bool hashtable<Key, Value, Hash, Comparator, Locker>::_insert(const key_type &key,
                                                              const value_type &value,
                                                              iterator &entry)
{
   std::pair<_entry_t *, unsigned long> result (_find_entry(key)) ;
   bool doesnt_exist = !result.first ;

   if (doesnt_exist)
   {
      TRACEPX(PCOMN_Hashtbl, DBGL_NORMAL, (void *)this << ": inserting entry with key <" << key << "> into bucket "
              << location(result.second) << " size=" << size() << " buckets=" << capacity() << " virgins=" << _virgins) ;

      bucket_list_node *bucket = _buckets + location(result.second) ;
      // New entry is inserted at front of the list to increase probability of hitting the mark instantly
      // during next searches.
      result.first = ::new (_alloc.allocate()) _entry_t(key, value, result.second, bucket) ;
      _touch_bucket(bucket) ;
      ++_size ;
      // Check if number of virgins dropped below acceptable threshold
      _check_resize() ;
   }

   entry = iterator(result.first) ;

   return doesnt_exist ;
}

template <class Key, class Value, class Hash, class Comparator, class Locker>
typename hashtable<Key, Value, Hash, Comparator, Locker>::_entry_found_t
hashtable<Key, Value, Hash, Comparator, Locker>::_find_entry(const key_type &key)
{
   unsigned long hashval = hash(key) ;
   size_t loc = location(hashval) ;

   for (bucket_iterator item (bucket_begin(loc)), end (bucket_end(loc)) ; item != end ; ++item)
   {
      _entry_t *entry = item._entry() ;
      if (entry->hash_value() == hashval && equal(key, entry->key()))
         return _entry_found_t(entry, hashval) ;
   }
   return _entry_found_t(0, hashval) ;
}

template <class Key, class Value, class Hash, class Comparator, class Locker>
typename hashtable<Key, Value, Hash, Comparator, Locker>::_entry_t *
hashtable<Key, Value, Hash, Comparator, Locker>::_remove(iterator iter)
{
   NOXCHECK(iter && !iter._is_end()) ;

   TRACEPX(PCOMN_Hashtbl, DBGL_NORMAL, (void *)this << ": removing <" << iter->key() << '>') ;

   bucket_iterator prev (*this, location(iter), false) ;
   for (bucket_iterator next (prev) ; ++next != iter ; ++prev)
      NOXCHECK(!next._is_end()) ;
   return _remove_next_entry(prev) ;
}

template <class Key, class Value, class Hash, class Comparator, class Locker>
typename hashtable<Key, Value, Hash, Comparator, Locker>::_entry_t *
hashtable<Key, Value, Hash, Comparator, Locker>::_remove(const key_type &key)
{
   TRACEPX(PCOMN_Hashtbl, DBGL_NORMAL, (void *)this << ": trying to remove <" << key << ">...") ;

   unsigned long hashval = hash(key) ;
   size_t loc = location(hashval) ;

   bucket_iterator prev (_buckets + loc) ;
   bucket_iterator end (bucket_end(loc)) ;
   for (bucket_iterator next (prev) ; ++next != end ; ++prev)
      if (next._entry()->hash_value() == hashval && equal(next->key(), key))
         return _remove_next_entry(prev) ;

   return NULL ;
}

template <class Key, class Value, class Hash, class Comparator, class Locker>
bool hashtable<Key, Value, Hash, Comparator, Locker>::_destroy(_entry_t *entry)
{
   if (entry)
   {
      _alloc.deallocate(pcomn::destroy(entry)) ;
      return true ;
   }
   return false ;
}

template <class Key, class Value, class Hash, class Comparator, class Locker>
bool hashtable<Key, Value, Hash, Comparator, Locker>::_rebind(const key_type &key,
                                                              const value_type &value,
                                                              iterator &entry)
{
   bool found = !_insert(key, value, entry) ;
   if (found)
      entry->value(value) ;
   return found ;
}

template <class Key, class Value, class Hash, class Comparator, class Locker>
bool hashtable<Key, Value, Hash, Comparator, Locker>::_rebind(const key_type &key, const value_type &value,
                                                              value_type &oldval, iterator &entry)
{
   bool found = !_insert(key, value, entry) ;
   if (found)
   {
      pcomn_swap(oldval, entry->value()) ;
      entry->value(value) ;
   }
   return found ;
}

} // End of namespace pcomn
