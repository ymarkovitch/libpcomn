/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_HASHTABLE_H
#define __PCOMN_HASHTABLE_H
/*******************************************************************************
 FILE         :   pcomn_hashtable.h
 COPYRIGHT    :   Yakov Markovitch, 1999-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Hash table template.

 CREATION DATE:   27 Dec 1999
*******************************************************************************/
/** @file
 Thread-safe hash table.

 @par
 This hash table is ACE's hashtable-like both in its class interface and ideology, as
 opposite to STL's unordered_map.
 @par
 It is threadsafe-enabled and works in threading environment efficiently, while not
 compromising single-threaded performance.
*******************************************************************************/
#include <pcommon.h>
#include <pcomn_utils.h>
#include <pcomn_memmgr.h>
#include <pcomn_hash.h>

#include <string.h>

namespace pcomn {

/*******************************************************************************
                     template <class Key, class Value>
                     class hashtable_entry
 An entry in the hash table.
*******************************************************************************/
template <class Key, class Value>
class hashtable_entry : public std::pair<const Key, Value> {
      typedef std::pair<const Key, Value> ancestor ;
   public:
      typedef hashtable_entry<Key, Value> self_type ;
      typedef Key    key_type ;
      typedef Value  value_type ;

      const key_type &key() const { return this->first ; }

      const value_type &value() const { return this->second ; }
      value_type &value() { return this->second ; }
      value_type &value(const value_type &newval) { return this->second = newval ; }

   protected:
      hashtable_entry(const key_type &id, const value_type &val) ;
      ~hashtable_entry() ;

   private:
      void *operator new(size_t) = delete ;
} ;

/*******************************************************************************
                     class bucket_list_node
*******************************************************************************/
class bucket_list_node {
      PCOMN_NONASSIGNABLE(bucket_list_node) ;
      PCOMN_NONCOPYABLE(bucket_list_node) ;
   public:
      bucket_list_node() :
         _next(this + 1)
      {}

      explicit bucket_list_node(bucket_list_node *next) :
         _next(next)
      {}

      bucket_list_node *next() const
      {
         return (bucket_list_node *)((intptr_t)_next & ~(intptr_t)1) ;
      }

      void next(bucket_list_node *next_node)
      {
          const intptr_t was_touched = touched() ;
         _next = (bucket_list_node *)((intptr_t)next_node | was_touched) ;
      }

      // touched()   -  return 1 if a bucket has been touched, 0 otherwize
      //
      bool touched() const { return (intptr_t)_next & 1 ; }

      bool touch(bool onoff)
      {
         bucket_list_node * const before = _next ;
         _next = (bucket_list_node *)
            (onoff ? ((intptr_t)before | (intptr_t)1) : ((intptr_t)before &~ (intptr_t)1)) ;
         return before != _next ;
      }

      bucket_list_node *remove_next()
      {
         NOXCHECK(!is_empty_bucket()) ;
         bucket_list_node *next_node = next() ;
         next(next_node->_next) ;
         return next_node ;
      }

      bucket_list_node *insert_next(bucket_list_node *new_node)
      {
         NOXPRECONDITION(new_node) ;

         bucket_list_node *next_node = next() ;
         next(new_node) ;
         new_node->next(next_node) ;
         return next_node ;
      }

      bool is_empty_bucket() const { return next() == this + 1 ; }

   protected:
      bucket_list_node(bucket_list_node *prev, bool) :
         _next(prev->next())
      {
         prev->next(this) ;
      }

   private:
      bucket_list_node *_next ;
} ;

/*******************************************************************************
                     class bucket_list_item
*******************************************************************************/
class bucket_list_item : public bucket_list_node {
   public:
      unsigned long hash_value() const { return _hash ; }

   protected:
      bucket_list_item(bucket_list_node *prev, unsigned long hash) :
         bucket_list_node(prev, true),
         _hash(hash)
      {}

   private:
      const unsigned long _hash ; /* Keeping hash value, we trade speed for memory, because
                                   * rehashing during auto-resizes could be quite expensive.
                                   */
} ;

/*******************************************************************************
      template<Key, Value, Hasher, Comparator, Lock>
      class hashtable

 Defined a hash that efficiently associates <Key>s with <Value>s.
 Keys are hashed through the Hasher object and compared is through the
 Comparator object.
*******************************************************************************/
template<class Key, class Value,
         class Hasher      = pcomn::hash_fn<Key>,
         class Comparator  = std::equal_to<Key>,
         class Locker      = PVoidMutex>
class hashtable : public Locker {
      class _entry_t ;
      class bucket_iterator ;
      friend class bucket_iterator ;

   public:
      typedef Key    key_type ;
      typedef Value  value_type ;
      typedef Locker lock_type ;

      typedef hashtable<key_type, value_type, Hasher, Comparator, lock_type> hashtable_type ;
      typedef std::lock_guard<lock_type>  write_lock ;
      typedef shared_lock<lock_type> read_lock ;

      class entry_type : public hashtable_entry<key_type, value_type> {
            friend class _entry_t ;
            entry_type(const Key &key, const Value &value) :
               hashtable_entry<key_type, value_type>(key, value)
            {}
            ~entry_type() {}
      } ;

      class iterator ;
      friend class iterator ;
      // I simply have no time to write real const_iterators, so there is some cheating...
      typedef iterator const_iterator ;

      typedef const entry_type &const_reference ;
      typedef entry_type &      reference ;

      typedef size_t size_type ;
      typedef int    difference_type ;

   private:
      class _entry_t : public bucket_list_item, public entry_type {
         public:
            _entry_t(const Key &key, const Value &value, unsigned long hash, bucket_list_node *head) :
               bucket_list_item(head, hash),
               entry_type(key, value)
            {}

            _entry_t(const _entry_t &src, bucket_list_node *head) :
               bucket_list_item(head, src.hash_value()),
               entry_type(src)
            {}
            // Do-nothing operator delete intented for use as stub
            // (so that expression 'delete object ;' would mean 'call destructor and don't free memory')
            //
            void operator delete(void *) {}
      } ;

      class bucket_iterator : public std::iterator<std::bidirectional_iterator_tag, entry_type> {
#if PCOMN_WORKAROUND(PCOMN_COMPILER_GNU, > 0)
            friend class hashtable<key_type, value_type, Hasher, Comparator, lock_type> ;
#else
            friend typename hashtable::hashtable_type ;
#endif
         public:
            typedef typename hashtable::hashtable_type hashtable_type ;

            bucket_iterator() :
               _next(NULL)
            {}

            // Contructor.
            // Creates a bucket iterator positioned at the beginning of a bucket
            //
            bucket_iterator(hashtable_type &table, unsigned loc) :
               _next(table._buckets[loc].next())
            {
               NOXCHECK(loc < table.capacity()) ;
            }

            bucket_iterator(hashtable_type &table, unsigned loc, bool end) :
               _next(table._buckets + (loc + end))
            {
               NOXCHECK(loc < table.capacity()) ;
            }

            bucket_iterator &operator++()
            {
               _next = _next->next() ;
               return *this ;
            }

            bucket_iterator operator++(int)
            {
               iterator tmp(*this) ;
               ++*this ;
               return tmp ;
            }

            bool operator==(const bucket_iterator &rhs) const
            {
               return _pointed() == rhs._pointed() ;
            }
            bool operator!=(const bucket_iterator &rhs) const
            {
               return !(*this == rhs) ;
            }

            entry_type& operator*() const { return *_entry() ; }
            entry_type *operator->() const { return _entry() ; }

            explicit operator bool() const { return !!_pointed() ; }

         protected:
            explicit bucket_iterator(bucket_list_node *node) :
               _next(node)
            {}

            void next(bucket_list_node *node) { _next = node ; }
            bucket_list_node *next() const  { return _next ; }

         private:
            bucket_list_node *_next ;

            bool _is_end() const { return _pointed() == _pointed()->next() ; }
            bucket_list_node *_pointed() const { return next() ; }
            _entry_t *_entry() const
            {
               NOXCHECK(_pointed() && !_pointed()->is_empty_bucket() && !_is_end()) ;
               return static_cast<_entry_t *>(_pointed()) ;
            }
      } ;


   public:
      // Constructor.
      // Initialize a <hashtable> with size <size> (in fact, with some prime
      // number size no less than size given).
      //
      explicit hashtable(size_t size = 0) ;
      hashtable(const Hasher &hasher, const Comparator &comparator, size_t size = 0) ;

      hashtable(const hashtable &) ;

      ~hashtable()
      {
         write_lock _lock(*this) ;
         _destroy_hashtable() ;
      }

      void swap(hashtable &other)
      {
         if (&other == this)
            return ;
         write_lock _lock_self(*this) ;
         write_lock _lock_source(other) ;
         _swap(other) ;
      }

      hashtable &operator=(const hashtable &other)
      {
         if (&other == this)
            return ;
         hashtable_type tmp (other) ;
         write_lock _lock_self(*this) ;
         _swap(other) ;
      }

      lock_type &lock() const { return *const_cast<hashtable_type *>(this) ; }


      /*************************************************************************
                           class hashtable::iterator
      *************************************************************************/
      /** Forward iterator for the pcomn::hashtable.
       This class doesn't perform any internal locking of the hashtable it
       is iterating upon since locking is inherently inefficient and/or
       error-prone within an STL-style iterator.
       If locking required, use explicitly std::lock_guard or std::lock_guard on the
       hashtable's internal lock, which is a base class of pcomn::hashtable.
      *************************************************************************/
      class iterator : public bucket_iterator {
         public:
            // Contructor.
            // If <at_end> == false, the iterator constructed is positioned
            // at the head of the map, it is positioned at the end otherwise.
            //
            explicit iterator(hashtable_type &table, bool at_end = false) :
               bucket_iterator(at_end || !table.size() ?
                               table._buckets_end() : hashtable_type::_closest(table._buckets_start()))
            {}

            // This constructor is deliberately non-explicit, for we actually want implicit
            // conversion of entry_type
            iterator(entry_type *entry) :
               bucket_iterator((_entry_t *)entry)
            {}

            iterator() {}

            iterator &operator++()
            {
               this->next(hashtable_type::_closest(this->next()->next())) ;
               return *this ;
            }

            iterator operator++(int)
            {
               iterator tmp(*this) ;
               ++*this ;
               return tmp ;
            }
      } ;

      typedef std::pair<iterator, bool> ret_type ;

      // insert() -  associate <id> with <val>.
      // If <id> is already in the table then the <hashtable_entry> is not changed.
      // Returns:
      //    On success, ret_type::first is equal to the entry inserted, ret_type::second is true.
      //    If <id> is already in the table, ret_type::first is equal to the entry found, ret_type::second is false.
      //
      ret_type insert(const key_type &id, const value_type &val)
      {
         ret_type result ;
         write_lock _lock(*this) ;

         result.second = _insert(id, val, result.first) ;
         return result ;
      }

      // try_insert() - associate <id> with <val> iff <id> is not in the table.
      // The difference with insert() is that if <id> is already in the table,
      // try_insert() assigns <id>'s corresponding value to the <val> parameter.
      // Returns:
      //    On success, ret_type::first is equal to the entry inserted,
      //    ret_type::second is true.
      //    If <id> is already in the table, both ret_type::first and <val>
      //    are equal to the entry found, ret_type::second is false.
      //
      ret_type try_insert(const key_type &id, value_type &val)
      {
         ret_type result ;
         write_lock _lock(*this) ;

         result.second = _insert(id, val, result.first) ;
         if (!result.second)
            val = result.first->value() ;
         return result ;
      }

      // replace()  -  reassociate <id> with <val>.
      // If <id> is not in the table then behaves just like <insert>.
      // Otherwise, associates that <id> with the <val> given.
      // Returns:
      //    If <id> not in the table, ret_type::first is equal to the entry
      //    inserted, ret_type::second is false.
      //    In <id> is already in the table, ret_type::first is equal to the
      //    entry found, ret_type::second is true.
      //
      ret_type replace(const key_type &id, const value_type &val)
      {
         ret_type result ;
         write_lock _lock(*this) ;

         result.second = _rebind(id, val, result.first) ;
         return result ;
      }

      // replace()  -  reassociate <id> with <val>.
      // If <id> is not in the table then behaves just like <insert>.
      // Otherwise, associates that <id> with the <val> given and stores the old value of
      // <val> into the <old_val> parameter
      // Returns:
      //    In case of insertion, ret_type::first is equal to the entry inserted, ret_type::second is false.
      //    In case of replacement, ret_type::first is equal to the entry found, ret_type::second is true.
      //
      ret_type replace(const key_type &id, const value_type &val, value_type &old_val)
      {
         ret_type result ;
         write_lock _lock(*this) ;

         result.second = _rebind(id, val, old_val, result.first) ;
         return result ;
      }

      // find()   -  locate <id>.
      // If found, returns <entry>; if not, returns NULL.
      //
      iterator find(const key_type &id)
      {
         write_lock _lock(*this) ;
         return _find(id) ;
      }

      // find()   -  locate <id>.
      // If found, returns <entry>; if not, returns NULL.
      //
      const_iterator find(const key_type &id) const
      {
         read_lock _lock(lock()) ;
         return const_cast<hashtable_type *>(this)->_find(id) ;
      }

      // find()   -  locate <key> and pass out corresponding <value>.
      // If <key> found, return true and assign corresponding value to <value>;
      // If not found, returns false and leaves <value> unchanged.
      //
      bool find(const key_type &key, value_type &value) const
      {
         read_lock _lock(lock()) ;
         const _entry_t *entry = const_cast<hashtable_type *>(this)->_find_entry(key).first ;
         if (!entry)
            return false ;
         value = entry->value() ;
         return true ;
      }

      bool has_key(const key_type &key) const
      {
         read_lock _lock(lock()) ;
         return !!_find_entry(key).first ;
      }

      // erase()  -  remove the <key> from the table.
      // Don't return the <val> to the caller (this is useful for collections where the
      // <val>s are *not* dynamically allocated...)
      // Returns:
      //    true if <key> was in the table, false otherwise.
      //
      bool erase(const key_type &key)
      {
         write_lock _lock(*this) ;
         return _destroy(_remove(key)) ;
      }

      // erase()  -  remove the <key> from the table.
      // Passes out the corresponding value to <value> to allow the caller to
      // deallocate.
      // Returns:
      //    true if <key> was in the table, false otherwise.
      //
      bool erase(const key_type &key, value_type &value)
      {
         write_lock _lock(*this) ;
         _entry_t *removed = _remove(key) ;
         if (removed)
         {
            value = removed->value() ;
            return _destroy(removed) ;
         }
         return false ;
      }

      // erase()  -  remove the <entry> from the table.
      //
      bool erase(const iterator &entry)
      {
         write_lock _lock(*this) ;
         return _destroy(_remove(entry)) ;
      }

      // clear()  -  remove all hashtable entries, reinitialize hashtable
      // Parameter 'initsize' is analogous to 'size' in the constructor.
      // Returns the number of erased entries.
      //
      size_t clear(size_t initsize = 0) ;

      // size()   -  get the current size of the table.
      // Value returned can be more than capacity() due to possibility of more
      // than one entry per bucket.
      //
      size_t size() const { return _size ; }

      // capacity()   -  get the exact number of buckets.
      // The result can be _less_ than returned by size()
      //
      size_t capacity() const { return _capacity ; }

      bool empty() const { return !size() ; }

      iterator begin() { return iterator(*this) ; }
      iterator end() { return iterator(*this, true) ; }

      const_iterator begin() const { return iterator(*const_cast<hashtable_type *>(this)) ; }
      const_iterator end() const { return iterator(*const_cast<hashtable_type *>(this), true) ; }

   protected:
      /*
       * The following methods do the actual work.
       */

      // equal()  -  returns true if <id1> == <id2>, else false.
      // This is defined as a separate method to facilitate template specialization.
      //
      bool equal(const key_type &id1, const key_type &id2) const
      {
         return _comparator(id1, id2) ;
      }

      // hash()   -  compute the hash value of the <id>.
      // This is defined as a separate method to facilitate template specialization.
      //
      unsigned long hash(const key_type &id) const
      {
         return _hasher(id) ;
      }

      size_t location(unsigned long hash_value) const { return hash_value % capacity() ; }
      size_t location(const iterator &iter) const { return location(iter._entry()->hash_value()) ; }

      /*
       * The following methods assume locks are held by private methods.
       */

      // _insert()   -  performs insert of <id,val> pair.
      // Must be called with locks held.
      //
      bool _insert(const key_type &id, const value_type &val, iterator &entry) ;

      // Performs rebind.  Must be called with locks held.
      bool _rebind(const key_type &id, const value_type &val, iterator &entry) ;

      // Performs rebind.  Must be called with locks held.
      bool _rebind(const key_type &id, const value_type &val, value_type &old_val, iterator &entry) ;

      // _find()  -  performs a find using <key>.
      // Must be called with locks held.
      //
      iterator _find(const key_type &key) { return iterator(_find_entry(key).first) ; }

      // _remove()   -  removes an entry with key given.
      // Must be called with locks held.
      //
      _entry_t *_remove(const key_type &key) ;

      // _remove()   -  removes an entry.
      // Must be called with locks held.
      //
      _entry_t *_remove(iterator iter) ;

      bool _destroy(_entry_t *entry) ;

      // create_buckets() - resize the map.
      // Must be called with locks held.  Note, that this method should never be
      // called more than once or else all the hashing will get screwed up as the size will change.
      //
      void create_buckets(size_t size) ;

      bucket_iterator bucket_begin(unsigned loc) { return bucket_iterator(*this, loc) ; }
      bucket_iterator bucket_end(unsigned loc) { return bucket_iterator(*this, loc, true) ; }

      /*******************************************************************************
                           class hashtable::entry_allocator
       Memory allocator for hash table entries.
      *******************************************************************************/
      class entry_allocator {
         public:
            // For the hash table bucket list to work properly, the allocator
            // must allocate memory aligned to even boundary, to ensure the
            // least significant bit (LSb) of a pointer to allocated memory is 0
            // It is so because the hash table uses LSb of a bucket pointer to
            // indicate whether the bucket has been touched, i.e. at least once
            // used to hold pointer to key/value pair.
            //
            entry_allocator() :
               _heap(sizeof(_entry_t), 11)
            {
               NOXCHECK(!(sizeof(_entry_t) & 1U)) ;
            }

            _entry_t *allocate() { return static_cast<_entry_t *>(_heap.allocate(sizeof(_entry_t))) ; }
            void deallocate(_entry_t *p) { _heap.free(p) ; }
         private:
            PMMemBlocks _heap ;
      } ;

      entry_allocator   _alloc ;       /* Allocator for the hash table entries */
      const Hasher      _hasher ;      /* Functor used for key hashing. */
      const Comparator  _comparator ;  /* Function object used for comparing keys. */

   private:
      bucket_list_node *_buckets ;  /* Array of singly-linked lists of <hashtable_entry>s
                                     * that hash to that bucket */
      size_t            _capacity ; /* The number of buckets size in a hash table. */
      size_t            _size ;     /* Current number of entries in the table (note that this can be
                                     * larger than <_capacity> due to the bucket chaining). */
      size_t            _virgins ;  /* Current number of buckets that have never been occupied */

      bucket_list_node *_buckets_start() { return _buckets ; }
      bucket_list_node *_buckets_end() { return _buckets + _capacity ; }
      // Get closest nonempty bucket (not _next_ closest, so if the backet
      // passed as a parameter is not empty, it will be returned as a return value)
      static bucket_list_node *_closest(bucket_list_node *bucket) ;

      typedef std::pair<_entry_t *, unsigned long> _entry_found_t ;
      // _find_entry()  -  get an entry that corresponds to a given key
      // Returns pair<entry_found, hash_value>.
      // If entry not found, result.first is NULL
      //
      _entry_found_t _find_entry(const key_type &key) ;

      void _init(size_t initsize) ;
      void _clear() ;
      void _destroy_hashtable() ;
      void _create_buckets(size_t realsize) ;
      void _touch_bucket(bucket_list_node *bucket) { _virgins -= bucket->touch(true) ; }
      _entry_t *_remove_next_entry(bucket_iterator &prev)
      {
         _entry_t *result = static_cast<_entry_t *>(prev._pointed()->remove_next()) ;
         --_size ;
         return result ;
      }

      // _check_resize() - check if number of virgins dropped below acceptable threshold.
      //                   If so, resize the table appropriatedly
      void _check_resize()
      {
         // There may be overflow problem with a hash table of capacity > 1.6G entries
         if (_virgins * 3 <= _capacity)
            _resize() ;
      }

      void _resize() ;

      void _swap(hashtable &other)
      {
         std::swap(_buckets, other._buckets) ;
         std::swap(_capacity, other._capacity) ;
         std::swap(_size, other._size) ;
         std::swap(_virgins, other._virgins) ;
      }
} ;

} // End of namespace pcomn

#include <pcomn_hashtable.cc>

#endif /* __PCOMN_HASHTABLE_H */
