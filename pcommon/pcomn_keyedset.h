/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_KEYEDSET_H
#define __PCOMN_KEYEDSET_H
/*******************************************************************************
 FILE         :   pcomn_keyedset.h
 COPYRIGHT    :   Yakov Markovitch, 2014-2016

 DESCRIPTION  :

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   19 Sep 2014
*******************************************************************************/
#include <pcomn_utils.h>

#include <map>
#include <unordered_map>

namespace pcomn {

template<typename Key, typename Value, typename Compare>
using std_map = std::map<Key, Value, Compare> ;

template<typename Key, typename Value, typename Compare>
using std_multimap = std::multimap<Key, Value, Compare> ;

template<typename Key, typename Value, typename Hash, typename Equal>
using std_unordered_map = std::unordered_map<Key, Value, Hash, Equal> ;

template<typename Key, typename Value, typename Hash, typename Equal>
using std_unordered_multimap = std::unordered_multimap<Key, Value, Hash, Equal> ;

/******************************************************************************/
/**
*******************************************************************************/
template<typename Value, typename ExtractKey>
struct keyval_adapter {
      typedef ExtractKey   key_extract ;
      typedef Value        value_type ;
      typedef typename std::result_of<ExtractKey(Value)>::type key_type ;
      typedef typename std::remove_reference<key_type>::type   keyval_type ;

      template<typename Compare>
      struct comparator : std::binary_function<keyval_adapter, keyval_adapter, bool> {
            comparator() = default ;
            comparator(const Compare &actual) : _compare(actual) {}

            bool operator()(const keyval_adapter &x, const keyval_adapter &y) const
            {
               return _compare(x.key(), y.key()) ;
            }

            const key_compare &base() const { return _compare ; }
         private:
            Compare _compare ;
      } ;

      template<typename Hash>
      struct hasher : std::unary_function<keyval_adapter, size_t> {
            hasher() = default ;
            hasher(const Hash &actual) : _hash(actual) {}

            size_t operator()(const keyval_adapter &k) const { return _hash(k.key()) ; }
         private:
            Hash _hash ;
      } ;

      keyval_adapter(const key_type &key) { _ptr.set<0>(&key) ; }

      key_type key() const
      {
         if (const keyval_type * const key = _ptr.get<0>())
            return *key ;
         return _extract_key(_ptr.get<1>()) ;
      }

   private:
      tagged_ptr_union<const keyval_type *, const value_type *> _ptr ;

      static const key_extract _extract_key ;

   private:
      PCOMN_NONCOPYABLE(keyval_adapter) ;
      PCOMN_NONASSIGNABLE(keyval_adapter) ;

      typedef std::pair<keyval_adapter<value_type, key_extract>, value_type> item_type ;

   protected:
      keyval_adapter()
      {
         _ptr.set<1>(reinterpret_cast<const value_type *>
                     (reinterpret_cast<char *>(this) + (offsetof(item_type, second) - offsetof(item_type, first)))) ;
      }
} ;

template<typename Value, typename ExtractKey>
const keyval_adapter<Value, ExtractKey>::key_extract
keyval_adapter<Value, ExtractKey>::_extract_key = {} ;

/*******************************************************************************

*******************************************************************************/
template<typename Value, typename ExtractKey,
         typename Compare = std::less<typename noref_result_of<ExtractKey(Value)>::type>,
         template <typename, typename, typename> class Container = std_map>
class keyed_ordered_set {
      typedef keyval_adapter<Value, ExtractKey>                    base_key_type ;
      typedef template typename base_key_type::comparator<Compare> base_key_compare ;
      typedef Container<base_key_type, Value, base_key_compare>    base_map_type ;
   public:
      typedef Value value_type ;
      typedef typename base_key_type::keyval_type key_type ;

      keyed_ordered_set() = default ;
      keyed_ordered_set(const keyed_ordered_set &) = default ;
      keyed_ordered_set(keyed_ordered_set &&) = default ;

      explicit keyed_ordered_set(const Compare &c) : _storage(base_key_compare(c)) {}

      keyed_ordered_set(initializer_list<value_type> values) : _storage(values) {}

      template<typename InputIterator>
      keyed_ordered_set(InputIterator first, InputIterator last, const Compare &comp) :
         _storage(first, last, base_key_compare(c))
      {}

      template<typename InputIterator>
      keyed_ordered_set(InputIterator first, InputIterator last) :
         _storage(first, last, base_key_compare())
      {}

      keyed_ordered_set &operator=(const keyed_ordered_set &) = default ;
      keyed_ordered_set &operator=(keyed_ordered_set &&) = default ;

      keyed_ordered_set &operator=(initializer_list<value_type> values)
      {
         keyed_ordered_set newset (values) ;
         swap(newset) ;
         return *this ;
      }

      key_compare key_comp() const { return storage().key_comp().base() ; }

      iterator begin() const noexcept { return storage().begin() ; }
      iterator end() const noexcept { return storage().end() ; }

      reverse_iterator rbegin() const noexcept { return storage().rbegin() ; }
      reverse_iterator rend() const noexcept { return storage().rend() ; }

      iterator cbegin() const noexcept { return storage().cbegin() ; }
      iterator cend() const noexcept { return storage().cend() ; }

      reverse_iterator crbegin() const noexcept { return storage().crbegin() ; }
      reverse_iterator crend() const noexcept { return storage().crend() ; }

      bool empty() const noexcept { return storage().empty() ; }
      size_type size() const noexcept { return storage().size() ; }

      size_type max_size() const noexcept { return storage().max_size() ; }

      void swap(keyed_ordered_set &other) { storage().swap(other.storage()) ; }

      template<typename... Args>
      std::pair<iterator, bool> emplace(Args&&... args) ;

      template<typename... Args>
      iterator	emplace_hint(const_iterator pos, Args &&... args) ;

      std::pair<iterator, bool> insert(const value_type &x) ;
      std::pair<iterator, bool> insert(value_type &&x) ;

      iterator insert(const_iterator position, const value_type &x) ;
      iterator insert(const_iterator position, value_type &&x) ;

      template<typename InputIterator>
      void insert(InputIterator first, InputIterator last) ;
      void insert(initializer_list<value_type>) ;

      iterator erase(const_iterator position) ;
      size_type erase(const key_type &x) ;
      iterator erase(const_iterator first, const_iterator last) ;

      void clear() { storage().clear() ; }

      size_type count(const key_type &x) const ;

      iterator find(const key_type &x) ;
      const_iterator find(const key_type &x) const ;

      iterator lower_bound(const key_type &x) ;
      const_iterator lower_bound(const key_type &x) const ;

      iterator upper_bound(const key_type &x) ;
      const_iterator upper_bound(const key_type &x) const ;

      std::pair<iterator, iterator> equal_range(const key_type &x) ;
      std::pair<const_iterator, const_iterator> equal_range(const key_type &x) const ;

   private:
      base_map_type _container ;

      base_map_type &container() { return _container ; }
      const base_map_type &container() const { return _container ; }
} ;

template<typename Value, typename ExtractKey,
         typename Hash = std::hash<typename noref_result_of<ExtractKey(Value)>::type>,
         typename Equal = std::equal_to<typename noref_result_of<ExtractKey(Value)>::type>,
         template <typename, typename, typename, typename> class Container = std_unordered_map>
class keyed_unordered_set {
} ;

} // end of pcomn namespace

#endif /* __PCOMN_KEYEDSET_H */
