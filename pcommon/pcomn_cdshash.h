/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_CDSHASH_H
#define __PCOMN_CDSHASH_H
/*******************************************************************************
 FILE         :   pcomn_cdshash.h
 COPYRIGHT    :   Yakov Markovitch, 2016

 DESCRIPTION  :   Lock-free concurrent hash table

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   14 Jun 2016
*******************************************************************************/
/** @file
 Lock-free concurrent hash table
*******************************************************************************/
#include <pcomn_cdsslist.h>

#include <new>

namespace pcomn {

/*******************************************************************************

*******************************************************************************/
template<typename T,
         typename ExtractKey = pcomn::identity,
         typename Hash = pcomn::hash_fn<noref_result_of_t<ExtractKey(T)>>,
         typename Pred = std::equal_to<noref_result_of_t<ExtractKey(T)>>>
class concurrent_hashtable {
      PCOMN_NONCOPYABLE(concurrent_hashtable) ;
      PCOMN_NONASSIGNABLE(concurrent_hashtable) ;

      struct node_value {
            // Constructor for a dummy node (directly pointed to from the corresponding bucket)
            explicit node_value(uint64_t bucket_num) :
               _key(make_dummy_key(bucket_num))
            {}

            template<typename... Args>
            node_value(uint64_t value_hash, std::piecewise_construct_t, Args && ...value_ctor_args) :
               _key(make_regular_key(value_hash))
            {
               ::new (&value()) T(std::forward<Args>(value_ctor_args)...) ;
            }

            uint64_t key() const { return _key ; }

            T &value() { return reinterpret_cast<T *>(&_value) ; }
            const T &value() const { return reinterpret_cast<const T *>(&_value) ; }

            static uint64_t make_regular_key(uint64_t value_hash) ;
            static uint64_t make_dummy_key(uint64_t bucket_num) ;

         private:
            const uint64_t          _key ;
            std::aligned_union_t<T> _value ;
      } ;

      typedef concurrent_slist<node_value> chaining_list ;

   public:
      typedef T            value_type ;

      typedef ExtractKey   key_extract ; /**< Functor calculating key from value:
                                              key_extract()(value_type) -> key_type */
      typedef Hash         hasher ;      /**< Functor calculating hash value from key:
                                              hasher()(value_type)->uint64_t */
      typedef Pred         key_equal ;   /**< Key comparator:
                                              key_equal()(key_type,key_type)->bool */

      typedef noref_result_of_t<key_extract(value_type)> key_type ;

      typedef size_t       size_type ;

      /************************************************************************/
      /** No iterators, only markers
      *************************************************************************/
      class marker final : chaining_list::marker {
            friend class concurrent_hashtable<value_type, key_extract, hasher, key_equal> ;
            typedef chaining_list::marker ancestor ;

         public:
            constexpr marker() = default ;

            friend bool operator==(const marker &x, const marker &y)
            {
               return static_cast<const ancestor &>(x) == static_cast<const ancestor &>(y) ;
            }
            friend bool operator!=(const marker &x, const marker &y)
            {
               return !(lhs == rhs) ;
            }

         private:
            // Allow implicit conversion
            marker(const ancestor &m) : ancestor(m) {}
      } ;

   public:
      explicit concurrent_hashtable(size_type initsize = 0) ;

      explicit concurrent_hashtable(std::pair<size_type, double> size_n_load) :
         concurrent_hashtable(size_n_load, {}, {}, {})
      {}

      concurrent_hashtable(std::pair<size_type, double> size_n_load,
                       const hasher &hash_fn, const key_equal &key_eq = {},
                       const key_extract &key_xtr = {}) ;

      concurrent_hashtable(std::pair<size_type, double> size_n_load,
                       const key_equal &key_eq, const key_extract &key_xtr = {}) :
         concurrent_hashtable(size_n_load, {}, key_eq, key_xtr)
      {}

      concurrent_hashtable(std::pair<size_type, double> size_n_load,
                       const key_extract &key_xtr) :
         concurrent_hashtable(size_n_load, {}, {}, key_xtr)
      {}

      ~concurrent_hashtable() ;

      concurrent_hashtable &operator=(concurrent_hashtable &&other)
      {
         if (&other != this)
            concurrent_hashtable(std::move(other)).swap(*this) ;
         return *this ;
      }

      const hasher &hash_function() const { return _hasher ; }

      const key_equal &key_eq() const { return _key_eq ; }

      const key_extract &key_get() const { return _key_get ; }

      void erase(iterator) ;
      size_type erase(const key_type &key) ;
      size_type erase_value(const value_type &value)
      {
         return erase_item(key_get()(value), nullptr) ;
      }

      std::pair<value_type, bool> pop(const key_type &key) ;
      bool pop(const key_type &key, value_type &popped_value) ;

      /// Erase all containter items.
      /// @returns The count of erased items.
      size_t clear() ;

      std::pair<iterator, bool> insert(const value_type &value) ;
      std::pair<iterator, bool> replace(const value_type &value) ;
      std::pair<iterator, bool> replace(const value_type &value, value_type &oldvalue) ;

      iterator find(const key_type &key) ;
      iterator find_value(const value_type &value) ;

      /// Get the value of the element with key equivalent to @a key.
      ///
      /// If no such element exists, an exception of type std::out_of_range is
      /// thrown. Note that in contrast to STL associative containers, this returns
      /// result by @em value instead of by @em reference, due to concurrent nature
      /// of the container.
      ///
      value_type at(const key_type &key) const ;

      /// Get the count of items in the container.
      /// @note In the presence on concurrent write access the returned value is
      /// transient.
      size_type size() const ;

      /// Indicate if the container has no elements.
      /// @note Generally on concurrent write access, i.e. if there are some other
      /// threads are inserting or erasing items, the returned value is transient.
      bool empty() const ;

      /// Get the number of elements with specified key.
      /// @note In this implementation may take only 0 and 1.
      size_type count(const key_type &key) const ;

      size_type value_count(const value_type &value) const ;
      {
         return count(key_get()(value)) ;
      }

      size_type max_size() const ;
      double max_load_factor() const ;
      double load_factor() const ;

   private:
      // While in many (the most) cases those classes are empty there is no much sense in
      // EBO here
      hasher            _hasher  {} ;
      key_equal         _key_eq  {} ;
      key_extract       _key_get {} ;
      float             _max_load_factor = PCOMN_CLOSED_HASH_LOAD_FACTOR ;

      chaining_list     _chain ;

      bool keys_equal(const value_type &value, const key_type &key) const
      {
         return !!key_eq()(key_get()(value), key) ;
      }
      size_t bucket_ndx(const key_type &key) const
      {
         return hash_function()(key) % bucket_count() ;
      }
} ;

int insert(so_key_t key)
{
   node = new node(so_regularkey(key));
   bucket = key Z size;

   if (T[bucket] == UNINITIALIZED)

      initialize_bucket(bucket);
   if (!list_insert(&(T[bucket]), node)) {
      delete_node(node);
      return 0;
   }

   csize = size;
   if (fetch-and-inc(&count) / csize > MAX_LUAD)

      CAS(&size, csize, 2 * csize);
   return 1;
}

int find(so_key_t key) {

  Sl: bucket = key Z size;

   if (T[bucket] == UNINITIALIZED)

      initialize_bucket(bucket);

   return list_find(&(T[bucket]),
                    so_regularkey(key));

   int delete(so_key_t key) {

      bucket = key Z size;

      if (T[bucket] == UNINITIALIZED)

         initialize_bucket(bucket);

      if (!list_delete(&(T[bucket]),
                       so_regularkey(key)))

         return 0;

      fetch-and-dec(&count);

      return 1;

   }

   void initialize_bucket(uint bucket) {
      parent = GET_PARENT(bucket);
      if (T[parent] == UNINITIALIZED)
         initialize_bucket(parent);
      dummy = new node(so_dummykey(bucket));
      if (!list_insert(&(T[parent]), dummy)) {
         delete dummy;
         dummy = cur;
      }
      T[bucket] = dummy;


} // end of namespace pcomn

#endif /* __PCOMN_CDSHASH_H */
