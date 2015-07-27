/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_HASHCLOSED_H
#define __PCOMN_HASHCLOSED_H
/*******************************************************************************
 FILE         :   pcomn_hashclosed.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2015. All rights reserved.

 DESCRIPTION  :   Exteremely simple closed hashing hashtable implementation, optimized
                  for small data items, especially POD data.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   21 Apr 2008
*******************************************************************************/
/** @file
 Exteremely simple closed hashing hashtable implementation.

 Very efficient storing small POD types, especially pointers, where it is far more space
 efficient than std::unordered_set.

 Uses linear probing as a collision resolution strategy: this is simple, has quite
 satisfactory performance and allows more efficient item deletion algorithme than
 other more complex strategies (e.g. double-hashing).
*******************************************************************************/
#include <pcomn_meta.h>
#include <pcomn_math.h>
#include <pcomn_function.h>
#include <pcomn_except.h>
#include <pcomn_hash.h>

#include <functional>

#include <utility>
#include <functional>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <limits>

#include <string.h>
#include <math.h>

constexpr const float PCOMN_CLOSED_HASH_LOAD_FACTOR = 0.75 ;

namespace pcomn {

struct closed_hashitem_base {
      enum State {
         Valid   = 0,
         Empty   = 1,
         Deleted = 2,
         End     = 3
      } ;
} ;

/******************************************************************************/
/** Closed hashtable item.
*******************************************************************************/
template<typename Value>
struct closed_hashtable_item : closed_hashitem_base {
      typedef Value value_type ;

      closed_hashtable_item() :
         _state(Empty),
         _value()
      {}

      State state() const { return (State)_state ; }
      void set_state(State newstate)
      {
         NOXCHECK((unsigned)newstate <= End) ;
         _state = newstate ;
      }

      const value_type &value() const { return _value ; }
      void set_value(const value_type &v)
      {
         _value = v ;
         _state = Valid ;
      }

      bool is_available() const { return ((_state >> 1) + _state) & 1 ; }

   private:
      uint8_t   _state ;
      Value     _value ;
} ;

/******************************************************************************/
/** Specialization of hashtable_item for pointers; uses no additional space,
 @p sizeof(bucket)==sizeof(void).
*******************************************************************************/
template<typename Value>
struct closed_hashtable_item<Value *> : closed_hashitem_base {
      typedef Value * value_type ;

      closed_hashtable_item() :
         _value((value_type)(int)Empty)
      {}

      State state() const
      {
         return is_valid(_value) ? Valid : (State)(uintptr_t)_value ;
      }
      void set_state(State newstate)
      {
         if (newstate > Valid)
            _value = (value_type)(uintptr_t)newstate ;
         else if (!is_valid(_value))
            _value = NULL ;
      }

      const value_type &value() const { return _value ; }
      void set_value(value_type v)
      {
         ensure<std::invalid_argument>(is_valid(v) || !v, "Attempt to insert an invalid pointer value.") ;
         _value = v ;
      }

      bool is_available() const
      {
         return (uintptr_t)_value <= End && ((((uintptr_t)_value >> 1) + (uintptr_t)_value) & 1) ;
      }

   private:
      value_type _value ;

      static bool is_valid(value_type v) { return !!((uintptr_t)v & ~(uintptr_t)7) ; }
} ;

/******************************************************************************/
/** Closed hash table, particularly efficient for storing objects of small POD types.

 Uses linear probing for collision resolution, .
*******************************************************************************/
template<typename Value,
         typename ExtractKey = pcomn::identity,
         typename Hash = pcomn::hash_fn<noref_result_of_t<ExtractKey(Value)> >,
         typename Pred = std::equal_to<noref_result_of_t<ExtractKey(Value)> > >
class closed_hashtable {
      // Only TriviallyCopyable items are allowed!
      PCOMN_STATIC_CHECK(std::is_trivially_copyable<Value>::value || std::is_literal_type<Value>::value) ;

      PCOMN_NONASSIGNABLE(closed_hashtable) ;

      typedef closed_hashtable_item<Value> bucket_type ;

   public:
      typedef noref_result_of_t<ExtractKey(Value)> key_type ;
      typedef Value       value_type ;
      typedef Hash        hasher ;
      typedef Pred        key_equal ;
      typedef ExtractKey  key_extract ;

      typedef size_t      size_type ;
      typedef ptrdiff_t   difference_type ;

      template<bool IsConstant>
      class basic_iterator :
         public std::iterator<std::forward_iterator_tag, Value, ptrdiff_t, const Value *, const Value &>
      {
            friend class closed_hashtable<Value, ExtractKey, Hash, Pred> ;
            typedef std::conditional_t<IsConstant, const bucket_type *, bucket_type *> item_pointer ;

         public:
            typedef ptrdiff_t    difference_type ;
            typedef Value        value_type ;
            typedef const Value *pointer ;
            typedef const Value &reference ;

            basic_iterator() :
               _current(NULL)
            {}

            basic_iterator(const basic_iterator<false> &other) :
               _current(other._current)
            {}

            reference operator*() const { return _current->value() ; }
            pointer operator->() const { return &(operator*()) ; }

            basic_iterator &operator++()
            {
               ++_current ;
               nearest() ;
               return *this ;
            }
            basic_iterator operator++(int)
            {
               basic_iterator prev (*this) ;
               ++*this ;
               return prev ;
            }

            friend bool operator==(const basic_iterator<IsConstant> &lhs, const basic_iterator<IsConstant> &rhs)
            {
               return lhs._current == rhs._current ;
            }
            friend bool operator!=(const basic_iterator<IsConstant> &lhs, const basic_iterator<IsConstant> &rhs)
            {
               return !(lhs == rhs) ;
            }

            friend std::ostream &operator<<(std::ostream &os, const basic_iterator &v)
            {
               return os << "{_current:" << v._current << '}' ;
            }
         private:
            item_pointer _current ;

            explicit basic_iterator(item_pointer ptr):
               _current(ptr)
            {
               if (_current)
                  nearest() ;
            }

            item_pointer nearest()
            {
               NOXCHECK(_current) ;
               while (_current->is_available())
                  ++_current ;
               return _current ;
            }
      } ;

   public:
      typedef basic_iterator<false> iterator ;
      typedef basic_iterator<true>  const_iterator ;

      explicit closed_hashtable(size_type initsize = 0) ;

      explicit closed_hashtable(const std::pair<size_type, double> &size_n_load) ;

      closed_hashtable(const closed_hashtable &other) :
         _basic_state{other.hash_function(), other.key_eq(), {}, other.max_load_factor()},
         _bucket_container(other.size(), _basic_state)
      {
         copy_buckets(other.begin_buckets(), other.end_buckets()) ;
      }

      closed_hashtable(closed_hashtable &&other) : closed_hashtable(0) { swap(other) ; }

      closed_hashtable(const std::pair<size_type, double> &size_n_load,
                       const hasher &hf, const key_equal &keq = {}, const key_extract &kex = {}) :
         _basic_state{hf, keq, kex, size_n_load.second},
         _bucket_container(size_n_load.first, _basic_state)
      {}

      closed_hashtable(const std::pair<size_type, double> &size_n_load,
                       const key_extract &kx, const key_equal &eq = {}) :
         _basic_state{{}, eq, kx, size_n_load.second},
         _bucket_container(size_n_load.first, _basic_state)
      {}

      ~closed_hashtable() { container().clear(_basic_state) ; }

      closed_hashtable &operator=(closed_hashtable &&other)
      {
         if (&other != this)
            closed_hashtable(std::move(other)).swap(*this) ;
         return *this ;
      }

      const hasher &hash_function() const { return _basic_state._hasher ; }

      const key_equal &key_eq() const { return _basic_state._key_eq ; }

      const key_extract &key_get() const { return _basic_state._key_get ; }

      void swap(closed_hashtable &other)
      {
         if (&other == this)
            return ;
         _basic_state.swap(other._basic_state) ;
         _bucket_container.swap(other._bucket_container) ;
      }

      void erase(iterator it) { erase_bucket(it._current) ; }

      size_type erase(const key_type &key) { return erase_item(key, NULL) ; }

      size_type erase(const key_type &key, value_type &erased_value)
      {
         return erase_item(key, &erased_value) ;
      }

      size_type erase_value(const value_type &value)
      {
         return erase_item(key_get()(value), NULL) ;
      }

      std::pair<iterator, bool> insert(const value_type &value)
      {
         if (container().overloaded(_basic_state))
            expand() ;

         bucket_type *place = find_available_bucket(key_get()(value)) ;

         NOXCHECK(place && (place->is_available() || keys_equal(place->value(), key_get()(value)))) ;

         const bool has_place = place->is_available() ;
         if (has_place)
            container().put_value(place, value, _basic_state) ;

         return std::pair<iterator, bool>(iterator(place), has_place) ;
      }

      std::pair<iterator, bool> replace(const value_type &value)
      {
         const std::pair<iterator, bool> result = insert(value) ;
         if (!result.second)
            const_cast<bucket_type *>(result.first._current)->set_value(value) ;
         return result ;
      }

      std::pair<iterator, bool> replace(const value_type &value, value_type &oldvalue)
      {
         const std::pair<iterator, bool> result = insert(value) ;
         if (!result.second)
         {
            oldvalue = *result.first ;
            const_cast<bucket_type *>(result.first._current)->set_value(value) ;
         }
         return result ;
      }

      template<typename InputIterator>
      void insert(InputIterator b, InputIterator e)
      {
         for ( ; b != e ; ++b)
            insert(*b) ;
      }

      template<typename InputIterator>
      void replace(InputIterator b, InputIterator e)
      {
         for ( ; b != e ; ++b)
            replace(*b) ;
      }

      iterator find(const key_type &key)
      {
         return find_iterator<iterator>(key) ;
      }

      const_iterator find(const key_type &key) const
      {
         return find_iterator<const_iterator>(key) ;
      }

      iterator find_value(const value_type &value)
      {
         return find_iterator<iterator>(key_get()(value)) ;
      }

      const_iterator find_value(const value_type &value) const
      {
         return find_iterator<const_iterator>(key_get()(value)) ;
      }

      size_type bucket_count() const { return container().bucket_count(_basic_state) ; }

      size_type size() const { return container().valid_count(_basic_state) ; }

      size_type max_size() const { return bucket_count() ; }

      double max_load_factor() const { return _basic_state._max_load_factor ; }

      double load_factor() const
      {
         const size_type bc = bucket_count() ;
         return bc ? (double)size()/bc : 1.0 ;
      }

      bool empty() const { return !size() ; }

      void reserve(size_type n) ;

      void clear() { container().clear(_basic_state) ; }

      size_type count(const key_type &key) const
      {
         return (size_type)!!find_bucket(key) ;
      }

      size_type value_count(const value_type &value) const
      {
         return count(key_get()(value)) ;
      }

      iterator begin() { return empty() ? end() : iterator(begin_buckets()) ; }
      const_iterator begin() const { return empty() ? end() : const_iterator(begin_buckets()) ; }
      const_iterator cbegin() const { return begin() ; }

      iterator end() { return iterator(end_buckets()) ; }
      const_iterator end() const { return const_iterator(end_buckets()) ; }
      const_iterator cend() const { return end() ; }

      friend std::ostream &operator<<(std::ostream &os, const closed_hashtable &v)
      {
         return os << "{size:" << v.size()
                   << (v._basic_state.is_static_buckets() ? " static_buckets:" : " buckets:") << v.bucket_count()
                   << " occupied:" << v.occupied_count()
                   << " buckptr:" << v.begin_buckets() << '}' ;
      }

   private:
      template<bool UseStaticBuckets, nullptr_t> union combined_buckets ;

      /*************************************************************************
        Common state for both dynamic and static buckets.
      *************************************************************************/
      struct basic_state {
            // Interface. Keep these members together: in most cases, those classes are empty
            // and object have size of 1 byte and don't require alignment, thus keeping them
            // together we optimizing both on size and locality.
            hasher            _hasher  {} ;
            key_equal         _key_eq  {} ;
            key_extract       _key_get {} ;
            uint8_t           _embed_count = 0 ; /* Nonzero value indicates "small table optimization" */
            float             _max_load_factor = PCOMN_CLOSED_HASH_LOAD_FACTOR ;

            basic_state() = default ;

            basic_state(const hasher &h, const key_equal &keq = {}, const key_extract &kex = {}) :
               _hasher(h), _key_eq(keq), _key_get(kex) {}

            basic_state(const hasher &h, const key_equal &keq, const key_extract &kex, double max_load) :
               _hasher(h), _key_eq(keq), _key_get(kex),
               // Set max load factor bounds to float values exactly representable in
               // binary
               _max_load_factor(max_load <= 0 ? PCOMN_CLOSED_HASH_LOAD_FACTOR : midval<float>(0.125, 0.875, (float)max_load))
            {}

            bool is_static_buckets() const { return !!_embed_count ; }
            size_t static_size() const
            {
               NOXCHECK(is_static_buckets()) ;
               return _embed_count - 1 ;
            }

            void swap(basic_state &other)
            {
               pcomn_swap(_hasher, other._hasher) ;
               pcomn_swap(_key_eq, other._key_eq) ;
               pcomn_swap(_key_get, other._key_get) ;
               pcomn_swap(_embed_count, other._embed_count) ;
               pcomn_swap(_max_load_factor, other._max_load_factor) ;
            }
      } ;

      /*************************************************************************
        Dynamically allocated buckets
      *************************************************************************/
      struct dynamic_buckets {
            size_type     _bucket_count ;
            size_type     _valid_count ;
            size_type     _occupied_count ;
            bucket_type * _buckets ;

            bucket_type *begin_buckets() const { return _buckets ; }
            bucket_type *end_buckets() const { return begin_buckets() + _bucket_count ; }

            void init(size_type initsize, double max_load_factor)
            {
               reset_members() ;
               if (initsize)
                  create_buckets((size_type)ceil((double)initsize/max_load_factor)) ;
            }

            void reset(size_type bucketcount)
            {
               NOXCHECK(bucketcount) ;
               reset_members() ;
               create_buckets(bucketcount) ;
            }

            void create_buckets(size_type bucketcount)
            {
               NOXCHECK(!_buckets) ;
               _bucket_count = 0 ;
               _buckets = new bucket_type[bucketcount + 1] ;
               _bucket_count = bucketcount ;
               _buckets[bucketcount].set_state(bucket_type::End) ;
            }

            void clear()
            {
               bucket_type *buckets = _buckets ;
               reset_members() ;
               delete [] buckets ;
            }

            void put_value(bucket_type *bucket, const value_type &value)
            {
               const bool newly_occupied = bucket->state() == bucket_type::Empty ;
               bucket->set_value(value) ;
               if (newly_occupied)
                  ++_occupied_count ;
               ++_valid_count ;
            }

            bool overloaded(double max_load_factor) const
            {
               return !_bucket_count || (double)_occupied_count/_bucket_count >= max_load_factor ;
            }
         private:
            void reset_members()
            {
               _valid_count = _occupied_count = 0 ;
               _buckets = NULL ;
               _bucket_count = 0 ;
            }
      } ;

      /*************************************************************************
        Statically allocated buckets
      *************************************************************************/
      struct static_buckets {
            enum { _bucket_count =
                   sizeof(bucket_type) <= sizeof(void *) ? 4 : sizeof(dynamic_buckets)/sizeof(bucket_type) } ;

            bucket_type *begin_buckets() const { return (bucket_type *)(void *)&_bucketmem ; }
            bucket_type *end_buckets() const { return begin_buckets() + _bucket_count ; }

         private:
            void *_bucketmem[(sizeof(bucket_type) * (_bucket_count + 1) + sizeof(void *) - 1)/sizeof(void *)] ;
      } ;

      /*************************************************************************
        Specialization of combined_buckets for purely dynamically-allocated backets
      *************************************************************************/
      template<nullptr_t dummy> union combined_buckets<false, dummy> {
            dynamic_buckets _dyn ;

            combined_buckets(size_type buckcount) { _dyn.reset(buckcount) ; }

            combined_buckets(size_type initsize, basic_state &st)
            {
               initsize = midval<ssize_t>(0, std::numeric_limits<ssize_t>::max(), initsize) ;

               st._embed_count = 0 ;
               _dyn.init(initsize, st._max_load_factor) ;
            }

            void put_value(bucket_type *bucket, const value_type &value, basic_state &)
            {
               _dyn.put_value(bucket, value) ;
            }

            size_t valid_count(const basic_state &) const { return _dyn._valid_count ; }
            size_type occupied_count(const basic_state &) const { return _dyn._occupied_count ; }
            size_type bucket_count(const basic_state &) const { return _dyn._bucket_count ; }

            size_t inc_valid(basic_state &, int increment) { return _dyn._valid_count += increment ; }
            size_t inc_occupied(const basic_state &, int increment) { return _dyn._occupied_count += increment ; }

            bucket_type *begin_buckets(const basic_state &) const { return _dyn._buckets ; }
            bucket_type *end_buckets(const basic_state &st) const { return begin_buckets(st) + _dyn._bucket_count ; }

            bool overloaded(const basic_state &st) const { return _dyn.overloaded(st._max_load_factor) ; }

            void clear(const basic_state &) { _dyn.clear() ; }

            void swap(combined_buckets &other)
            {
               std::swap(_dyn._bucket_count, other._dyn._bucket_count) ;
               std::swap(_dyn._valid_count, other._dyn._valid_count) ;
               std::swap(_dyn._occupied_count, other._dyn._occupied_count) ;
               std::swap(_dyn._buckets, other._dyn._buckets) ;
            }
      } ;

      /*************************************************************************
        Specialization of combined_buckets for actual combination of static
        and dynamic buckets
      *************************************************************************/
      template<nullptr_t dummy> union combined_buckets<true, dummy> {
            dynamic_buckets _dyn ;
            static_buckets  _stat ;

            combined_buckets(size_type buckcount) { _dyn.reset(buckcount) ; }

            combined_buckets(size_type initsize, basic_state &st)
            {
               initsize = midval<ssize_t>(0, std::numeric_limits<ssize_t>::max(), initsize) ;
               st._embed_count = initsize <= _stat._bucket_count ;
               if (!st.is_static_buckets())
                  _dyn.init(initsize, st._max_load_factor) ;
               else
               {
                  std::uninitialized_fill(_stat.begin_buckets(), _stat.end_buckets(), bucket_type()) ;
                  _stat.end_buckets()->set_state(bucket_type::End) ;
               }
            }

            void put_value(bucket_type *bucket, const value_type &value, basic_state &st)
            {
               if (!st.is_static_buckets())
                  _dyn.put_value(bucket, value) ;
               else
               {
                  NOXCHECK(bucket >= _stat.begin_buckets() && bucket < _stat.end_buckets()) ;
                  NOXCHECK(st.static_size() < _stat._bucket_count) ;
                  bucket->set_value(value) ;
                  ++st._embed_count ;
               }
            }

            size_t valid_count(const basic_state &st) const
            {
               return st.is_static_buckets() ? st.static_size() : _dyn._valid_count ;
            }

            size_t inc_valid(basic_state &st, int increment)
            {
               if (st.is_static_buckets())
                  return st._embed_count = (int)st._embed_count + increment ;
               return _dyn._valid_count += increment ;
            }

            size_type occupied_count(const basic_state &st) const
            {
               return st.is_static_buckets() ? st.static_size() : _dyn._occupied_count ;
            }

            size_t inc_occupied(const basic_state &st, int increment)
            {
               if (!st.is_static_buckets())
                  return _dyn._occupied_count += increment ;
               return st.static_size() ;
            }

            size_type bucket_count(const basic_state &st) const
            {
               return st.is_static_buckets() ? (size_type)_stat._bucket_count : _dyn._bucket_count ;
            }

            bucket_type *begin_buckets(const basic_state &st) const
            {
               return st.is_static_buckets() ? _stat.begin_buckets() : _dyn.begin_buckets() ;
            }
            bucket_type *end_buckets(const basic_state &st) const
            {
               return st.is_static_buckets() ? _stat.end_buckets() : _dyn.end_buckets() ;
            }

            bool overloaded(const basic_state &st)
            {
               return !st.is_static_buckets()
                  ? _dyn.overloaded(st._max_load_factor)
                  : st.static_size() == _stat._bucket_count ;
            }

            void clear(basic_state &st)
            {
               if (st.is_static_buckets())
                  st._embed_count = 1 ;
               else
                  _dyn.clear() ;
            }

            void swap(combined_buckets &other)
            {
               // Interpret _bucket_container as POD data. Since _bucket_container contains at
               // least one pointer, its aligned at least to the pointer size boundary.
               std::swap_ranges((void **)this,
                                (void **)this + (sizeof *this)/sizeof(void **),
                                (void **)&other) ;
            }
      } ;

      // Use static buckets optimization only if there is a place for at least 4 buckets
      typedef
      combined_buckets<(sizeof(dynamic_buckets)/sizeof(bucket_type) >= 4), nullptr> buckets_container ;

   private:
      basic_state       _basic_state ;
      buckets_container _bucket_container ;

      buckets_container &container() { return _bucket_container ; }
      const buckets_container &container() const { return _bucket_container ; }

      bucket_type *begin_buckets() const { return container().begin_buckets(_basic_state) ; }
      bucket_type *end_buckets() const { return container().end_buckets(_basic_state) ; }

      size_type occupied_count() const { return container().occupied_count(_basic_state) ; }

      bool keys_equal(const value_type &value, const key_type &key) const
      {
         return !!key_eq()(key_get()(value), key) ;
      }
      size_t bucket_ndx(const key_type &key) const { return hash_function()(key) % bucket_count() ; }

      void expand(size_type reserve_count = 0) ;

      bucket_type *find_available_bucket(const key_type &key) const
      {
         NOXCHECK(occupied_count() <= bucket_count()) ;
         NOXCHECK(size() < bucket_count()) ;

         bucket_type * const first_found = begin_buckets() + bucket_ndx(key) ;
         bucket_type *bucket = first_found ;
         bucket_type *result = NULL ;
         do
            switch (bucket->state())
            {
               case bucket_type::Valid:
                  if (!keys_equal(bucket->value(), key)) break ;
               case bucket_type::Empty:
                  return bucket ;

               case bucket_type::Deleted:
                  // Save position of first found deleted slot: if we'll not find a
                  // valid bucket with requested key, this slot will be used as a
                  // result, since it is a first appropriate available place for the key.
                  if (!result)
                     result = bucket ;
                  break ;
               default:
                  PCOMN_DEBUG_FAIL("Invalid bucket state while searching available bucket") ;
            }
         while ((bucket = next(bucket)) != first_found) ;
         return result ;
      }

      bucket_type *find_bucket(const key_type &key) const
      {
         if (empty())
            return NULL ;
         bucket_type * const begin = begin_buckets() ;
         bucket_type *bucket = begin + bucket_ndx(key) ;
         bucket_type * const end = bucket ;
         do
            switch (bucket->state())
            {
               case bucket_type::Valid:
                  if (keys_equal(bucket->value(), key))
                     return bucket ;
                  break ;
               case bucket_type::Empty:
                  return NULL ;

               default: break ;
            }
         while ((bucket = next(bucket)) != end) ;
         return NULL ;
      }

      template<typename Iterator>
      Iterator find_iterator(const key_type &key) const
      {
         bucket_type * const bucket = find_bucket(key) ;
         return Iterator(bucket ? bucket : end_buckets()) ;
      }

      // Advance bucket pointer cyclically forward
      bucket_type *next(bucket_type *current) const
      {
         bucket_type * const begin = begin_buckets() ;
         return begin + (current - begin + 1) % bucket_count() ;
      }
      bucket_type *prev(bucket_type *current) const
      {
         // Modulo division in C is broken for negative numbers, from the mathematics POV
         return current - begin_buckets() ? --current : end_buckets() - 1 ;
      }

      size_type erase_item(const key_type &key, value_type *erased_value)
      {
         bucket_type * const found (find_bucket(key)) ;
         if (!found)
            return 0 ;
         if (erased_value)
            *erased_value = found->value() ;
         erase_bucket(found) ;
         return 1 ;
      }

      void erase_bucket(bucket_type *bucket)
      {
         NOXCHECK(bucket >= begin_buckets() && bucket < end_buckets()) ;
         NOXCHECK(bucket->state() == bucket_type::Valid) ;
         bucket_type *pnext = next(bucket) ;
         container().inc_valid(_basic_state, -1) ;
         if (pnext->state() != bucket_type::Empty)
            bucket->set_state(bucket_type::Deleted) ;
         else
            collect_buckets(bucket, pnext) ;
      }

      // Garbage collection; maintains invariant that a bucket with Empty state is never
      // prepended with Deleted buckets, i.e. only Deleted or Valid bucket may follow
      // a Deleted one.
      void collect_buckets(bucket_type *bucket, bucket_type *boundary)
      {
         NOXCHECK(bucket >= begin_buckets() && bucket < end_buckets()) ;
         NOXCHECK(bucket->state() != bucket_type::Empty) ;
         do
         {
            bucket->set_state(bucket_type::Empty) ;
            container().inc_occupied(_basic_state, -1) ;
         }
         while((bucket = prev(bucket)) != boundary && bucket->state() == bucket_type::Deleted) ;
      }

      void copy_buckets(const bucket_type *begin, const bucket_type *end) ;
} ;

/*******************************************************************************
 closed_hashtable
*******************************************************************************/
template<typename V, typename X, typename H, typename P>
closed_hashtable<V, X, H, P>::closed_hashtable(size_type initsize) :
   _bucket_container(initsize, _basic_state)
{}

template<typename V, typename X, typename H, typename P>
closed_hashtable<V, X, H, P>::closed_hashtable(const std::pair<size_type, double> &size_n_load) :
   _basic_state{{}, {}, {}, size_n_load.second},
   _bucket_container(size_n_load.first, _basic_state)
{}

template<typename V, typename X, typename H, typename P>
void closed_hashtable<V, X, H, P>::copy_buckets(const bucket_type *begin,
                                                const bucket_type *end)
{
   for ( ; begin != end ; ++begin)
      if (begin->state() == bucket_type::Valid)
         insert(begin->value()) ;
      else
         NOXCHECK(begin->is_available()) ;
}

template<typename V, typename X, typename H, typename P>
void closed_hashtable<V, X, H, P>::expand(size_type reserve_count)
{
   NOXCHECK(reserve_count <= std::numeric_limits<size_type>::max()/2) ;
   NOXCHECK(!_basic_state.is_static_buckets() || _basic_state.static_size()) ;

   size_type new_bucket_count = std::max<size_type>(bucket_count(), 4) ;
   for (new_bucket_count *= 2 ; new_bucket_count < reserve_count ; new_bucket_count *= 2) ;

   const bool was_static = _basic_state.is_static_buckets() ;

   buckets_container other_container (new_bucket_count) ;
   other_container.swap(container()) ;

   const bucket_type *begin = other_container.begin_buckets(_basic_state) ;
   const bucket_type *end = other_container.end_buckets(_basic_state) ;
   _basic_state._embed_count = 0 ;
   copy_buckets(begin, end) ;

   if (!was_static)
      other_container._dyn.clear() ;
}

/*******************************************************************************

*******************************************************************************/
template<typename V, typename E, typename H, typename P>
inline bool find_keyed_value(const closed_hashtable<V, E, H, P> &dict,
                             const typename closed_hashtable<V, E, H, P>::key_type &key,
                             V &value)
{
   const typename closed_hashtable<V, E, H, P>::const_iterator found (dict.find(key)) ;
   if (found == dict.end())
      return false ;
   value = *found ;
   return true ;
}

template<typename V, typename E, typename H, typename P>
inline V get_keyed_value(const closed_hashtable<V, E, H, P> &dict,
                         const typename closed_hashtable<V, E, H, P>::key_type &key,
                         const V &default_value)
{
   const typename closed_hashtable<V, E, H, P>::const_iterator found (dict.find(key)) ;
   return found == dict.end() ? default_value : *found ;
}

template<typename V, typename E, typename H, typename P>
inline V get_keyed_value(const closed_hashtable<V, E, H, P> &dict,
                         const typename closed_hashtable<V, E, H, P>::key_type &key)
{
   return get_keyed_value(dict, key, V()) ;
}


} // end of namespace pcomn

namespace std {
template<typename V, typename X, typename H, typename P>
inline void swap(pcomn::closed_hashtable<V, X, H, P> &lhs, pcomn::closed_hashtable<V, X, H, P> &rhs)
{
   lhs.swap(rhs) ;
}
} ;

#endif /* __PCOMN_HASHCLOSED_H */
