/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_IMMUTABLESTR_H
#define __PCOMN_IMMUTABLESTR_H
/*******************************************************************************
 FILE         :   pcomn_immutablestr.h
 COPYRIGHT    :   Yakov Markovitch, 2006-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   The immutable string template.
                  Mostly interface-compatible with std::basic_string, aside from
                  the absense of mutating operations
                  insert(), erase(), replace(), resize(), push_back(), operator +=()
                  as well as capacity() and get_allocator().
                  While all the absent mutating operations could be implemented as
                  returning new string, these are intentionally omitted to avoid
                  semantic clash.

                  Please note that string storage is Loki::flex_string compliant.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   14 Nov 2006
*******************************************************************************/
#include <pcomn_meta.h>
#include <pcomn_metafunctional.h>
#include <pcomn_atomic.h>
#include <pcomn_function.h>
#include <pcomn_algorithm.h>
#include <pcomn_strmanip.h>
#include <pcomn_except.h>

#include <utility>
#include <iterator>
#include <algorithm>
#include <string>
#include <iostream>
#include <limits>

namespace pcomn {

/*******************************************************************************
 Forward declarations
*******************************************************************************/
template<typename Char, class CharTraits, class Storage>
class shared_string ;
template<typename Char, class CharTraits, class Storage>
class immutable_string ;
template<typename Char, class CharTraits, class Storage>
class mutable_strbuf ;

/*******************************************************************************
                     template<typename Char,
                              class AtomicThreadingPolicy>
                     struct refcounted_strdata
*******************************************************************************/
template<typename Char, class AtomicThreadingPolicy = ::pcomn::atomic_op::SingleThreaded>
struct refcounted_strdata {
      long     _refcount ;
      size_t   _size ;
      Char     _begin[1] ;

      static const refcounted_strdata<Char, AtomicThreadingPolicy> zero ;

      typedef AtomicThreadingPolicy atomic_policy ;

      union alignment {
            long     _1 ;
            size_t   _2 ;
            Char     _3 ;
      } ;

      size_t size() const { return _size ; }
      Char *begin() { return _begin + 0 ; }
      Char *end() { return _begin + _size ; }
      const Char *begin() const { return _begin + 0 ; }
      const Char *end() const { return _begin + _size ; }
} ;

template<typename Char, class AtomicThreadingPolicy>
const refcounted_strdata<Char, AtomicThreadingPolicy>
refcounted_strdata<Char, AtomicThreadingPolicy>::zero = { 0, 0, {0} } ;

/*******************************************************************************
                     template <typename Char,
                               class AtomicThreadingPolicy,
                               class Allocator>
                     class refcounted_storage
*******************************************************************************/
/** Reference-counted storage for pcomn::shared_string.
 This storage shall always provide space for at least one additional
 character after requested size, namely value_type() (0 in most cases) in order
 to preserve C string compatibility.
 I.e. both storage.end() + 1 is a valid past-the-end iterator, access to both
 *storage.end() is always allowed and produces value_type().
*******************************************************************************/
template <typename Char,
          class AtomicThreadingPolicy = atomic_op::SingleThreaded,
          class Allocator = typename std::basic_string<Char>::allocator_type>
class refcounted_storage : public Allocator {
      typedef Allocator                                     ancestor ;
      typedef AtomicThreadingPolicy                         atomic_policy ;
      typedef refcounted_storage<Char, AtomicThreadingPolicy, Allocator> self_type ;
      friend class mutable_strbuf<Char, std::char_traits<Char>, self_type> ;

   protected:
      typedef refcounted_strdata<Char, atomic_policy> data_type ;

   public:
      typedef Char                              value_type ;
      typedef Allocator                         allocator_type ;
      typedef typename Allocator::pointer       iterator ;
      typedef typename Allocator::const_pointer const_iterator ;
      typedef typename Allocator::size_type     size_type ;

      refcounted_storage() :
         ancestor(),
         _data(const_cast<value_type *>(data_type::zero.begin()))
      {}

      refcounted_storage(const refcounted_storage &source) :
         ancestor(source.get_allocator()),
         _data(source._data)
      {
         incref(str_data()) ;
      }

      refcounted_storage(const allocator_type &a) :
         ancestor(a),
         _data(const_cast<value_type *>(data_type::zero.begin()))
      {}

      refcounted_storage(const value_type *source, const size_type len,
                         const allocator_type &a = allocator_type()) :
         allocator_type(a),
         _data(const_cast<value_type *>(data_type::zero.begin()))
      {
         if (len)
         {
            size_type capacity = len ;
            raw_copy(source, source + len,
                     _data = create_str_data(capacity)->begin()) ;
         }
      }

      refcounted_storage(const size_type len, value_type c,
                         const allocator_type &a = allocator_type()) :
         allocator_type(a),
         _data(const_cast<value_type *>(data_type::zero.begin()))
      {
         if (len)
         {
            size_type capacity = len ;
            data_type * const d = create_str_data(capacity) ;
            pcomn::raw_fill(d->begin(), d->end(), c) ;
            _data = d->begin() ;
         }
      }

      refcounted_storage &operator=(const refcounted_storage &source)
      {
         if (_data != source._data)
         {
            incref(const_cast<data_type &>(source.str_data())) ;
            do_decref() ;
            _data = source._data ;
         }
         return *this ;
      }

      ~refcounted_storage()
      {
         do_decref() ;
      }

      size_type size() const { return size_type(str_data().size()) ; }
      size_type capacity() const { return size() ; }

      size_type max_size() const { return allocator_type::max_size(); }

      void swap(refcounted_storage &rhs) { std::swap(_data, rhs._data) ; }

      const value_type *begin() const { return _data ; }
      const value_type *end() const { return str_data().end() ; }

      const value_type *c_str() const { return begin() ; }
      const value_type *data() const { return begin() ; }
      allocator_type get_allocator() const { return *this; }

   protected:
      static size_type allocated_size(size_type char_count)
      {
         return sizeof(aligner) * aligner_count(char_count) ;
      }

      static size_type allocated_count(size_type requested_count)
      {
         return
            (requested_count * sizeof(aligner) - offsetof(data_type, _begin))
            / sizeof(value_type) ;
      }

      // count gets actually allocated memory
      void *do_alloc(size_type &char_count) const
      {
         NOXCHECK(char_count) ;
         const size_type allocated_items = aligner_count(char_count) ;
         void * const allocated = actual_allocator(*this).allocate(allocated_items, 0) ;
         char_count = allocated_count(allocated_items) ;
         return allocated ;
      }

      void do_dealloc(data_type *d) const
      {
         // Zero stings are _never_ deleted, they are static
         NOXCHECK(d->_size) ;
         actual_allocator(*this)
            .deallocate(reinterpret_cast<aligner *>(d), aligner_count(d->_size)) ;
      }

      void clear()
      {
         do_decref() ;
         _data = const_cast<value_type *>(data_type::zero.begin()) ;
      }

   private:
      value_type *_data ;

      typedef typename data_type::alignment aligner ;
      typedef typename allocator_type::template rebind<aligner>::other actual_allocator ;

      // char_count gets actually allocated memory
      data_type *create_str_data(size_type &char_count) const
      {
         NOXCHECK(char_count) ;
         const size_type requested_count = char_count ;
         // Don't forget place for trailing zero
         size_type actual_size = char_count + 1 ;
         data_type * const d = reinterpret_cast<data_type *>(do_alloc(actual_size)) ;
         char_count = actual_size - 1 ;
         d->_refcount = 1 ;
         d->_size = requested_count ;
         // Trailing zero (C string compatible)
         *d->end() = value_type() ;
         return d ;
      }

      data_type &str_data()
      {
         return *reinterpret_cast<data_type *>
            (reinterpret_cast<char *>(_data) - offsetof(data_type, _begin)) ;
      }
      const data_type &str_data() const { return const_cast<self_type *>(this)->str_data() ; }

      static long incref(data_type &data)
      {
         // Please note that all zero-length strings are static by design
         // and never increfed/decrefed.
         // While we could avoid checking for empty strings by making initial
         // static empty string refcount equal to 1 and then counting empty
         // string(s) references the same way as for nonempty, actually checking
         // _is_ an optimization since allows to avoid cache flushing by avoiding
         // write operations.
         return data._size ? atomic_policy::inc(data._refcount) : 0 ;
      }

      static size_type aligner_count(size_type count)
      {
         return
            (sizeof(data_type)
             + (std::max(count, static_cast<size_type>(1)) - 1) * sizeof(value_type)
             + sizeof(aligner) - 1)
            /  sizeof(aligner) ;
      }

      void do_decref()
      {
         data_type *d = &str_data() ;
         if (d->_size && !atomic_policy::dec(d->_refcount))
            do_dealloc(d) ;
      }
} ;

/*******************************************************************************
                     template<typename Char,
                              class Traits,
                              class Storage>
                     class shared_string
*******************************************************************************/
/** The common base for pcomn::immutable_string and pcomn::mutable_strbuf.
  Implements all constant methods of std::basic_string and thus is
  interface-compatible with the latter by read operations.
*******************************************************************************/
template<typename Char,
         class CharTraits = std::char_traits<Char>,
         class Storage = refcounted_storage<Char> >
class shared_string : protected Storage {
   public:
      typedef Storage      storage_type ;
      typedef Char         value_type ;
      typedef Char         char_type ;
      typedef CharTraits   traits_type ;

      typedef size_t            size_type ;
      typedef const char_type * pointer ;
      typedef const char_type * const_pointer ;

      typedef const char_type * iterator ;
      typedef const char_type * const_iterator;
      typedef std::reverse_iterator<iterator>         reverse_iterator ;
      typedef std::reverse_iterator<const_iterator>   const_reverse_iterator ;

      static const size_type npos = static_cast<size_type>(-1) ;

   public:
      using storage_type::begin ;
      using storage_type::end ;
      using storage_type::data ;
      using storage_type::c_str ;

      const_reverse_iterator rbegin() const { return reverse_iterator(end()) ; }
      const_reverse_iterator rend() const { return reverse_iterator(begin()) ; }

      size_type max_size() const { return storage_type::max_size() ; }
      size_type size() const { return storage_type::size() ; }
      size_type length() const { return storage_type::size() ; }
      bool empty() const { return !storage_type::size() ; }

      char_type operator[](size_type pos) const
      {
         NOXCHECK(pos <= size()) ;
         return *(begin() + pos) ;
      }

      char_type at(size_type pos) const
      {
         if (pos >= size())
            bad_pos(pos) ;
         return (*this)[pos] ;
      }

      size_type find(const shared_string &str, size_type pos = 0) const
      {
         return find(str.data(), pos, str.length()) ;
      }

      size_type find(const value_type *str, size_type pos, size_type n) const
      {
         const size_type sz = size() ;
         if (pos <= sz)
         {
            const_iterator current (begin() + pos) ;
            const const_iterator finish(end()) ;
            do if (traits_type::compare(current, str, n) == 0)
                  return finish - current ;
            while (current++ <= finish) ;
         }
         return npos ;
      }

      size_type find(const value_type *str, size_type pos = 0) const
      {
         return find(str, pos, traits_type::length(str)) ;
      }

      size_type find(value_type c, size_type pos = 0) const
      {
         const size_type sz = size() + 1 ;
         if (pos < sz)
         {
            const char_type * const start = c_str() ;
            const char_type * const found = traits_type::find(c_str() + pos, sz - pos, c) ;
            if (found)
               return found - start ;
         }
         return npos ;
      }

      size_type rfind(const shared_string &src, size_type pos = npos) const
      {
         return rfind(src.data(), pos, src.length()) ;
      }

      size_type rfind(const value_type *s, size_type pos, size_type n) const
      {
         const size_type sz = size() ;
         if (n > sz)
            return npos ;
         const size_type startpos = std::min(pos, sz - n) ;
         if (n == 0)
            return startpos ;

         const const_iterator start (begin()) ;
         const_iterator current (start + startpos) ;
         for ( ; !traits_type::eq(*current, *s) || traits_type::compare(&*current, s, n) != 0 ; --current)
            if (current == start)
               return npos ;
         return current - start ;
      }

      size_type rfind(const value_type *s, size_type pos = npos) const
      {
         return rfind(s, pos, traits_type::length(s)) ;
      }

      size_type rfind(value_type c, size_type pos = npos) const
      {
         const size_type startpos = std::min(pos, size()) ;
         const const_iterator start (begin()) ;
         const_iterator current (start + startpos) ;
         while (!traits_type::eq(*current, c))
         {
            if (current == start)
               return npos ;
            --current ;
         }
         return current - start ;
      }

      size_type find_first_of(const shared_string &str, size_type pos = 0) const
      {
         return find_first_of(str.data(), pos, str.size()) ;
      }

      size_type find_first_of(const value_type *s, size_type pos, size_type n) const
      {
         return __find_first<identity<const char_type *> >(s, pos, n) ;
      }

      size_type find_first_of(const value_type *s, size_type pos = 0) const
      {
         return find_first_of(s, pos, traits_type::length(s)) ;
      }

      size_type find_first_of(value_type c, size_type pos = 0) const
      {
         return find(c, pos) ;
      }

      size_type find_last_of (const shared_string &str, size_type pos = npos) const
      {
         return find_last_of(str.data(), pos, str.length());
      }

      size_type find_last_of(const value_type* s, size_type pos, size_type n) const
      {
         return __find_last<identity<const char_type *> >(s, pos, n) ;
      }

      size_type find_last_of (const value_type* s, size_type pos = npos) const
      {
         return find_last_of(s, pos, traits_type::length(s));
      }

      size_type find_last_of(value_type c, size_type pos = npos) const
      {
         return rfind(c, pos) ;
      }

      size_type find_first_not_of(const shared_string &str, size_type pos = 0) const
      {
         return find_first_not_of(str.data(), pos, str.size());
      }

      size_type find_first_not_of(const value_type *s, size_type pos, size_type n) const
      {
         return __find_first<std::logical_not<const char_type *> >(s, pos, n) ;
      }

      size_type find_first_not_of(const value_type* s, size_type pos = 0) const
      {
         return find_first_not_of(s, pos, traits_type::length(s));
      }

      size_type find_first_not_of(value_type c, size_type pos = 0) const
      {
         return find_first_not_of(&c, pos, 1);
      }

      size_type find_last_not_of(const shared_string &str, size_type pos = npos) const
      {
         return find_last_not_of(str.data(), pos, str.length());
      }

      size_type find_last_not_of(const value_type* s, size_type pos, size_type n) const
      {
         return __find_last<std::logical_not<const char_type *> >(s, pos, n) ;
      }

      size_type find_last_not_of(const value_type* s, size_type pos = npos) const
      {
         return find_last_not_of(s, pos, traits_type::length(s)) ;
      }

      size_type find_last_not_of(value_type c, size_type pos = npos) const
      {
         return rfind(c, pos) == npos ;
      }

      int compare(const shared_string &str) const
      {
         // While we can call compare(0, size(), str), we'd better optimize
         // to avoid lots of checks
         if (data() == str.data())
            return 0 ;

         const size_type lsz = size() ;
         const size_type rsz = str.size() ;
         const int intermed = traits_type::compare(data(), str.data(), std::min(lsz, rsz)) ;
         return intermed ? intermed
            : lsz > rsz ? 1
            : lsz < rsz ? -1
            : 0 ;
      }

      int compare(size_type pos1, size_type n1, const shared_string &str) const
      {
         return compare(pos1, n1, str.data(), str.size()) ;
      }

      int compare(size_type pos1, size_type n1, const value_type *s) const
      {
         return compare(pos1, n1, s, traits_type::length(s)) ;
      }

      int compare(size_type pos1, size_type n1, const value_type *s, size_type n2) const
      {
         const size_type sz = size() ;
         if (pos1 > sz) bad_pos(pos1) ;
         n1 = std::min(n1, sz - pos1) ;
         const int intermed = traits_type::compare(data() + pos1, s, std::min(n1, n2)) ;
         return intermed ? intermed
            : n1 > n2 ? 1
            : n1 < n2 ? -1
            : 0 ;
      }

      int compare(size_type pos1, size_type n1, const shared_string &str,
                  size_type pos2, size_type n2) const
      {
         if (pos2 > str.size()) str.bad_pos(pos2) ;
         return compare(pos1, n1, str.data() + pos2, std::min(n2, str.size() - pos2)) ;
      }

      int compare(const value_type *s) const
      {
         const size_type s_sz = traits_type::length(s) ;
         const int intermed = traits_type::compare(c_str(), s, s_sz + 1) ;
         return !intermed ? static_cast<int>(size() - s_sz) : intermed ;
      }

   protected:
      ~shared_string() {}

      shared_string() :
         storage_type()
      {}

      shared_string(const shared_string &str) :
         storage_type(str)
      {}

      shared_string(const shared_string &str, size_type pos, size_type n)
      {
         const size_type srcsize = str.size() ;
         ensure_le<std::out_of_range>(pos, srcsize, "String position is out of range") ;
         const size_type newsize = std::min(n, srcsize - pos) ;
         if (newsize)
            if (newsize == srcsize)
               *static_cast<storage_type *>(this) = static_cast<const storage_type &>(str) ;
            else
            {
               storage_type tmp (str.data() + pos, newsize,
                                 *static_cast<const typename storage_type::allocator_type *>(this)) ;
               storage_type::swap(tmp) ;
            }
      }

      shared_string(const value_type *source, size_type len) :
         storage_type(source, len)
      {}

      shared_string(size_type n, char_type c) :
         storage_type(n, c)
      {}

      void assign(const shared_string &src)
      {
         storage_type::operator=(src) ;
      }

      void swap(shared_string &other) { storage_type::swap(other) ; }

   private:
      template<class Predicate>
      size_type __find_first(const value_type *s, size_type pos, size_type n) const
      {
         Predicate predicate ;
         if (pos > size() || !n)
            return npos ;
         const const_iterator start (begin()) ;
         const const_iterator finish (end()) ;
         for (const_iterator current (start + pos) ; current != finish ; ++current)
            if (predicate(traits_type::find(s, n, *current)))
               return current - start ;
         return npos ;
      }

      template<class Predicate>
      size_type __find_last(const value_type *s, size_type pos, size_type n) const
      {
         Predicate predicate ;
         const size_type sz = size() ;
         const size_type startpos = std::min(pos, sz) ;
         if (!startpos || !n)
            return npos ;

         const const_iterator finish (begin()) ;
         const_iterator current (finish + (startpos - 1)) ;
         while (!predicate(traits_type::find(s, n, *current)))
         {
            if (current == finish)
               return npos ;
            --current ;
         }
         return current - finish ;
      }

      void bad_pos(size_type pos) const ;

      void operator=(pcomn::Instantiate) ;
} ;

template<typename Char, class CharTraits, class Storage>
void shared_string<Char, CharTraits, Storage>::bad_pos(size_type pos) const
{
   char buf[96] ;
   sprintf(buf, "Position %u is out of range for shared string of size %u.", pos, size()) ;
   throw std::out_of_range(buf) ;
}

/*******************************************************************************
 mutable_strbuf_ref - proxy reference for mutable_strbuf copying.
*******************************************************************************/
template<typename Char, class CharTraits, class Storage>
class mutable_strbuf ;

template<typename Char, class CharTraits, class Storage>
struct mutable_strbuf_ref {
      mutable_strbuf_ref(mutable_strbuf<Char, CharTraits, Storage> &rval) :
         _ref(rval)
		{}

      mutable_strbuf<Char, CharTraits, Storage> &_ref ;
} ;

/*******************************************************************************
                     template<typename Char,
                              class Traits,
                              class Storage>
                     class mutable_strbuf
*******************************************************************************/
/** The mutable string buffer with move-only (no copy) logic.
*******************************************************************************/
template<typename Char,
         class CharTraits = std::char_traits<Char>,
         class Storage = refcounted_storage<Char> >
class mutable_strbuf : public shared_string<Char, CharTraits, Storage> {
      typedef shared_string<Char, CharTraits, Storage>      ancestor ;
      typedef typename ancestor::data_type                  data_type ;

      friend class immutable_string<Char, CharTraits, Storage> ;

      PCOMN_NONASSIGNABLE(mutable_strbuf) ;
      mutable_strbuf(const mutable_strbuf &) ;
   public:
      typedef typename ancestor::storage_type   storage_type ;
      typedef typename ancestor::char_type      char_type ;
      typedef typename ancestor::traits_type    traits_type ;
      typedef typename ancestor::value_type     value_type ;
      typedef typename ancestor::size_type      size_type ;

      typedef char_type *        pointer ;
      typedef const char_type *  const_pointer ;

      typedef char_type *        iterator ;
      typedef const char_type *  const_iterator;
      typedef std::reverse_iterator<iterator>         reverse_iterator ;
      typedef std::reverse_iterator<const_iterator>   const_reverse_iterator ;

      typedef mutable_strbuf<char_type, traits_type, storage_type>      class_type ;
      typedef mutable_strbuf_ref<char_type, traits_type, storage_type>  ref_type ;

      using ancestor::npos ;
      using ancestor::data ;
      using ancestor::begin ;
      using ancestor::end ;
      using ancestor::rbegin ;
      using ancestor::rend ;

      mutable_strbuf() :
         _capacity(0)
      {}

      mutable_strbuf(mutable_strbuf &src) :
         ancestor(),
         _capacity(0)
      {
         swap(src) ;
      }

      mutable_strbuf(const ref_type &src) :
         ancestor(),
         _capacity(0)
      {
         swap(src._ref) ;
      }

      template<typename S>
      mutable_strbuf(const S &s, PCOMN_ENABLE_CTR_IF_STRCHAR(S, char_type)) :
         ancestor(str::cstr(s), str::len(s)),
         _capacity(allocated_capacity(ancestor::size()))
      {}

      template<typename S>
      mutable_strbuf(const S &s, size_type from_pos,
                     typename enable_if_strchar<S, char_type, size_type>::type length) :
         ancestor(str::cstr(s) + ensure_le<std::out_of_range>(from_pos,
                                                              (size_type)str::len(s),
                                                              "String position is out of range"),
                  std::min(str::len(s) - from_pos, length)),
         _capacity(allocated_capacity(ancestor::size()))
      {}

      template<typename C, size_type n>
      mutable_strbuf(C (&s)[n], size_type from_pos, size_type length) :
         ancestor(s + ensure_lt<std::out_of_range>(from_pos, n, "String position is out of range"),
                  length == npos
                  ? (std::find(s + std::min(n, from_pos), s + n, 0) - s) - from_pos
                  : length)
      {}

      value_type *data() { return const_cast<value_type *>(ancestor::data()) ; }

      iterator begin() { return const_cast<iterator>(ancestor::begin()) ; }
      iterator end() { return const_cast<iterator>(ancestor::end()) ; }

      reverse_iterator rbegin() { return reverse_iterator(end()) ; }
      reverse_iterator rend() { return reverse_iterator(begin()) ; }

      operator ref_type() { return ref_type(*this) ; }

      size_type capacity() const { return _capacity ; }

      void swap(mutable_strbuf &src)
      {
         ancestor::swap(src) ;
         std::swap(_capacity, src._capacity) ;
      }

      template<typename InputIterator>
      std::enable_if_t<(!std::is_same<InputIterator, iterator>::value &&
                        !std::is_same<InputIterator, const_iterator>::value &&
                        !std::numeric_limits<InputIterator>::is_integer),
                       mutable_strbuf &>
      append(InputIterator input, size_type n)
      {
         if (!n)
            return *this ;
         const size_type newsize = this->size() + n ;
         data_type &data = reserve(newsize) ;
         value_type *current = data.end() ;
         // Just to make sure that if InputIterator throws an exception, all
         // invariants will be OK
         const value_type safety_value = *input ;
         value_type * const safety_point = current ;
         for(value_type * const end = current + n ; ++current != end ; *current = *++input) ;
         data._size = newsize ;
         *data.end() = value_type() ;
         *safety_point = safety_value ;
         return *this ;
      }

      template<typename C>
      std::enable_if_t<std::is_same<C, value_type>::value, mutable_strbuf &>
      append(const C *input, size_type n)
      {
         if (n)
            pcomn::raw_copy(input, input + n, expand(n)) ;
         return *this ;
      }

      mutable_strbuf &append(size_type n, char_type c)
      {
         if (n)
         {
            char_type *start = expand(n) ;
            pcomn::raw_fill(start, start + n, c) ;
         }
         return *this ;
      }

      mutable_strbuf &resize(const size_type n, value_type c)
      {
         const int increment = n - this->size() ;
         if (increment > 0)
            append((size_type)increment, c) ;
         else if (increment)
            if (!n)
               clear() ;
            else
            {
               data_type &data = storage_type::str_data() ;
               data._size = n ;
               *data.end() = value_type() ;
            }
         return *this ;
      }

      template<typename S>
      typename enable_if_strchar<S, char_type, mutable_strbuf &>::type
      operator+=(const S &rhs)
      {
         return append(str::cstr(rhs), str::len(rhs)) ;
      }

      mutable_strbuf &operator+=(char_type rhs)
      {
         const size_type newsize = this->size() + 1 ;
         data_type &data = reserve(newsize) ;
         char_type *d = data.end() ;
         *d = rhs ;
         *++d = value_type() ;
         data._size = newsize ;
         return *this ;
      }

      template<typename S>
      std::enable_if_t<is_string_or_char<S, char_type>::value, mutable_strbuf>
      operator+(const S &rhs)
      {
         return ref_type(mutable_strbuf(*this) += rhs) ;
      }

   protected:
      void clear()
      {
         ancestor::clear() ;
         _capacity = 0 ;
      }

   private:
      size_t _capacity ;

      data_type &reserve(const size_type requested_capacity)
      {
         NOXCHECK(this->storage_type::_data &&
                  (!storage_type::str_data()._size || storage_type::str_data()._refcount == 1)) ;
         if (requested_capacity > _capacity)
            recapacitate(requested_capacity) ;
         return storage_type::str_data() ;
      }

      char_type *expand(const size_type n)
      {
         const size_type newsize = this->size() + n ;
         data_type &data = reserve(newsize) ;
         char_type * const gap = data.end() ;
         data._size = newsize ;
         *data.end() = value_type() ;
         return gap ;
      }

      void recapacitate(const size_type requested_capacity) ;

      static size_type allocated_capacity(size_type requested_count)
      {
         return !!requested_count *
            (mutable_strbuf::allocated_count(storage_type::aligner_count(requested_count + 1)) - 1) ;
      }
} ;

template<typename C, class Traits, class Storage>
void mutable_strbuf<C, Traits, Storage>::recapacitate(const size_type requested_capacity)
{
   NOXCHECK(requested_capacity > _capacity) ;
   // _capacity is always odd (by design, due to alignement + trailing 0)
   size_type actual_capacity =
      std::max(_capacity + (_capacity + 1) / 2, requested_capacity) + 1 ;
   data_type *old_data = &storage_type::str_data() ;
   data_type *new_data = static_cast<data_type *>(this->do_alloc(actual_capacity)) ;
   memcpy(new_data, old_data, this->allocated_size(old_data->_size + 1)) ;
   this->storage_type::_data = new_data->_begin ;
   // "-1" stands for the trailing zero
   _capacity = actual_capacity - 1 ;
   new_data->_refcount = 1 ;
   if (old_data->_size)
   {
      NOXCHECK(old_data->_refcount == 1) ;
      this->do_dealloc(old_data) ;
   }
}

/*******************************************************************************
                     template<typename Char,
                              class Traits,
                              class Storage>
                     class immutable_string
*******************************************************************************/
template<typename Char,
         class CharTraits = std::char_traits<Char>,
         class Storage = refcounted_storage<Char> >
class immutable_string : public shared_string<Char, CharTraits, Storage> {
      typedef shared_string<Char, CharTraits, Storage>   ancestor ;
      typedef mutable_strbuf<Char, CharTraits, Storage>  strbuf ;
   public:
      typedef typename ancestor::storage_type   storage_type ;
      typedef typename ancestor::char_type      char_type ;
      typedef typename ancestor::traits_type    traits_type ;
      typedef typename ancestor::value_type     value_type ;
      typedef typename ancestor::size_type      size_type ;

      typedef typename ancestor::const_iterator iterator ;
      typedef typename ancestor::const_iterator const_iterator ;
      typedef typename ancestor::const_reverse_iterator reverse_iterator ;
      typedef typename ancestor::const_reverse_iterator const_reverse_iterator ;

      using ancestor::npos ;

      immutable_string() {}

      immutable_string(const immutable_string &str) :
         ancestor(str)
      {}

      template<typename S>
      immutable_string(const S &s, PCOMN_ENABLE_CTR_IF_STRCHAR(S, char_type)) :
         ancestor(str::cstr(s), str::len(s))
      {}

      immutable_string(const immutable_string &str,
                       size_type from_pos,
                       size_type length) :
         ancestor(str, from_pos, length)
      {}

      template<typename S>
      immutable_string(const S &s, size_type from_pos,
                       typename enable_if_strchar<S, char_type, size_type>::type length) :
         ancestor(str::cstr(s) + ensure_le<std::out_of_range>(from_pos,
                                                              (size_type)str::len(s),
                                                              "String position is out of range"),
                  std::min(str::len(s) - from_pos, length))
      {}

      immutable_string(const char_type *s, size_type from_pos, size_type length) :
         ancestor(s + from_pos, length == npos ? str::len(s + from_pos) : length)
      {}

      template<typename C, size_type n>
      immutable_string(C (&s)[n], size_type from_pos, size_type length) :
         ancestor(s + ensure_lt<std::out_of_range>(from_pos, n, "String position is out of range"),
                  length == npos
                  ? (std::find(s + std::min(n, from_pos), s + n, 0) - s) - from_pos
                  : length)
      {}

      immutable_string(const char_type *begin, const char_type *end) :
         ancestor(begin, end - begin)
      {}

      immutable_string(strbuf &src)
      {
         from_strbuf(src) ;
      }
      immutable_string(const typename strbuf::ref_type &src)
      {
         from_strbuf(src._ref) ;
      }

      /* FIXME: Not implemented
         template<class InputIterator>
         immutable_string(InputIterator begin, InputIterator end) ;
      */

      immutable_string(size_type n, char_type c) :
         ancestor(n, c)
      {}

      immutable_string &operator=(const immutable_string &src)
      {
         ancestor::assign(src) ;
         return *this ;
      }

      immutable_string &operator=(char_type c)
      {
         immutable_string tmp (1, c) ;
         swap(tmp) ;
         return *this ;
      }

      immutable_string &operator=(strbuf &buf)
      {
         this->clear() ;
         from_strbuf(buf) ;
         return *this ;
      }

      immutable_string &operator=(const typename strbuf::ref_type &buf)
      {
         this->clear() ;
         from_strbuf(buf._ref) ;
         return *this ;
      }

      template<typename S>
      typename enable_if_strchar<S, char_type, immutable_string &>::type
      operator=(const S &src)
      {
         immutable_string tmp (str::cstr(src), 0, str::len(src)) ;
         swap(tmp) ;
         return *this ;
      }

      // This specialization is necessary for MSVC++ 8.0 (VS2005) to prevent
      // ICE when attempting to assign string literal to immutable string
      // (e.g. foostring = "Hello, world!", where foostring is immutable_string<char>).
      template<size_t n>
      immutable_string &operator=(const char_type (&src)[n])
      {
         immutable_string tmp (src + 0, 0, npos) ;
         swap(tmp) ;
         return *this ;
      }

      immutable_string substr(size_type pos = 0, size_type n = npos) const
      {
         return immutable_string(*this, pos, n);
      }

      void swap(immutable_string &other) { ancestor::swap(other) ; }

      template<typename T>
      immutable_string &assign(const T &src)
      {
         return *this = src ;
      }

      template<typename T, typename U>
      immutable_string &assign(T a1, U a2)
      {
         immutable_string tmp (a1, a2) ;
         swap(tmp) ;
         return *this ;
      }

      immutable_string &assign(const immutable_string &src, size_type pos, size_type n)
      {
         return *this = immutable_string(src, pos, n) ;
      }

      template<typename R>
      std::enable_if_t<is_string_or_char<R, char_type>::value, strbuf>
      operator+(const R &rhs) const
      {
         return typename strbuf::ref_type(strbuf(*this) += rhs) ;
      }
   private:
      void from_strbuf(strbuf &src)
      {
         NOXCHECK(!this->size()) ;
         ancestor::swap(src) ;
         src._capacity = 0 ;
      }
} ;

/*******************************************************************************
 swap specializations for immutable strings and mutable stringbufs
*******************************************************************************/
PCOMN_DEFINE_SWAP(immutable_string<P_PASS(C, T, S)>, template<typename C, class T, class S>) ;
PCOMN_DEFINE_SWAP(mutable_strbuf<P_PASS(C, T, S)>, template<typename C, class T, class S>) ;

/*******************************************************************************
 istring
 iwstring
*******************************************************************************/
typedef immutable_string<char>    istring ;
typedef immutable_string<wchar_t> iwstring ;

typedef mutable_strbuf<char>     mstrbuf ;
typedef mutable_strbuf<wchar_t>  mwstrbuf ;

/*******************************************************************************
 Stream output
*******************************************************************************/
template<typename Char, class Traits, class Storage>
inline std::basic_ostream<Char, Traits> &
operator<<(std::basic_ostream<Char, Traits> &os,
           const shared_string<Char, Traits, Storage> &str)
{
   return os << str.c_str() ;
}

template<class StreamTraits, class Traits, class Storage>
inline std::basic_ostream<char, StreamTraits> &
operator<<(std::basic_ostream<char, StreamTraits> &os,
           const shared_string<wchar_t, Traits, Storage> &str)
{
   const wchar_t *begin = str.c_str() ;
   return narrow_output(os, begin, begin + str.size()) ;
}

/*******************************************************************************
 String traits for immutable strings
*******************************************************************************/
template<typename Char, class Traits, class Storage>
struct string_traits<shared_string<Char, Traits, Storage> > :
   public stdstring_traits<shared_string<Char, Traits, Storage>, true> {} ;

template<typename Char, class Traits, class Storage>
struct string_traits<immutable_string<Char, Traits, Storage> > :
   public stdstring_traits<immutable_string<Char, Traits, Storage>, true> {} ;

/*******************************************************************************
 shared_string comparison
*******************************************************************************/
#define STR_DEFINE_RELOPS(type)       \
DEFINE_RELOP_##type(==)               \
DEFINE_RELOP_##type(!=)               \
DEFINE_RELOP_##type(<)                \
DEFINE_RELOP_##type(<=)               \
DEFINE_RELOP_##type(>)                \
DEFINE_RELOP_##type(>=)

#define DEFINE_RELOP_STR_STR(op)                           \
template<class Char, class Traits, class S1, class S2>            \
inline bool operator op(const pcomn::shared_string<Char, Traits, S1> &lhs,  \
                        const pcomn::shared_string<Char, Traits, S2> &rhs)  \
{                                                                 \
   return lhs.compare(rhs) op 0 ;                                 \
}

#define DEFINE_RELOP_STR_CSTR(op)                                       \
template<class Str, class Char, class Traits, class S>                  \
inline typename pcomn::enable_if_other_string<pcomn::shared_string<Char, Traits, S>, Str, \
bool>::type operator op(const pcomn::shared_string<Char, Traits, S> &lhs, const Str &rhs) \
{                                                                       \
   return lhs.compare(pcomn::str::cstr(rhs)) op 0 ;                     \
}

#define DEFINE_RELOP_CSTR_STR(op)                                       \
template<class Str, class Char, class Traits, class S>                  \
inline typename pcomn::enable_if_other_string<pcomn::shared_string<Char, Traits, S>, Str, \
bool>::type operator op(const Str &lhs, const pcomn::shared_string<Char, Traits, S> &rhs) \
{                                                                       \
   return -rhs.compare(pcomn::str::cstr(lhs)) op 0 ;                    \
}

STR_DEFINE_RELOPS(STR_STR)
STR_DEFINE_RELOPS(STR_CSTR)
STR_DEFINE_RELOPS(CSTR_STR)

#undef STR_DEFINE_RELOPS
#undef DEFINE_RELOP_STR_CSTR
#undef DEFINE_RELOP_CSTR_STR
#undef DEFINE_RELOP_STR_STR

/*******************************************************************************
 Concatenation
*******************************************************************************/
template<typename L, class C, class Traits, class S>
inline typename pcomn::enable_if_other_string<pcomn::shared_string<C, Traits, S>, L,
L>::type operator+(const L &lhs, const pcomn::immutable_string<C, Traits, S> &rhs)
{
   typedef pcomn::mutable_strbuf<C, Traits, S> mbuf_type ;
   return typename mbuf_type::ref_type(mbuf_type(lhs) += rhs) ;
}

/*******************************************************************************
 Case conversion
*******************************************************************************/
namespace str {

template<typename Converter, typename C>
inline mutable_strbuf<C> &convert_inplace(mutable_strbuf<C> &buf, Converter converter,
                                          size_t offs = 0, size_t size = std::string::npos)
{
   return detail::convert_inplace_stdstr(buf, converter, offs, size) ;
}

template<typename C>
inline mutable_strbuf<C> &to_lower_inplace(mutable_strbuf<C> &s,
                                           size_t offs = 0, size_t size = std::string::npos)
{
   return convert_inplace(s, ctype_traits<C>::tolower, offs, size) ;
}

template<typename C>
inline mutable_strbuf<C> &to_upper_inplace(mutable_strbuf<C> &s,
                                           size_t offs = 0, size_t size = std::string::npos)
{
   return convert_inplace(s, ctype_traits<C>::toupper, offs, size) ;
}

template<typename C>
inline immutable_string<C> to_lower(const immutable_string<C> &s)
{
   mutable_strbuf<C> buf (s) ;
   return to_lower_inplace(buf) ;
}

template<typename C>
inline immutable_string<C> to_upper(const immutable_string<C> &s)
{
   mutable_strbuf<C> buf (s) ;
   return to_upper_inplace(buf) ;
}

} // end of namespace pcomn::str

/*******************************************************************************
 reader & writer
*******************************************************************************/
namespace io {

/// String writer for mutable_strbuf.
template<typename Char, class Traits, class Storage>
struct writer<mutable_strbuf<Char, Traits, Storage> > :
         iodevice<mutable_strbuf<Char, Traits, Storage> &>  {

      static ssize_t write(mutable_strbuf<Char, Traits, Storage> &buffer,
                           const Char *from, const Char *to)
      {
         const ssize_t sz = to - from ;
         NOXCHECK(sz >= 0);
         buffer.append(from, sz) ;
         return sz ;
      }
} ;
} // end of namespace pcomn::io

} // end of namespace pcomn

#endif /* __PCOMN_IMMUTABLESTR_H */
