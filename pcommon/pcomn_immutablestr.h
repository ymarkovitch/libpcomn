/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_IMMUTABLESTR_H
#define __PCOMN_IMMUTABLESTR_H
/*******************************************************************************
 FILE         :   pcomn_immutablestr.h
 COPYRIGHT    :   Yakov Markovitch, 2006-2018. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   The immutable string template.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   14 Nov 2006
*******************************************************************************/
/** @file
 The immutable reference-counted string template.

 Mostly interface-compatible with std::basic_string, except for the absense of mutating
 operations insert(), erase(), replace(), resize(), push_back(), operator +=().
*******************************************************************************/
#include <pcomn_meta.h>
#include <pcomn_atomic.h>
#include <pcomn_counter.h>
#include <pcomn_function.h>
#include <pcomn_algorithm.h>
#include <pcomn_except.h>
#include <pcomn_string.h>

#include <utility>
#include <iterator>
#include <iostream>
#include <limits>

namespace pcomn {

/*******************************************************************************
 Forward declarations
*******************************************************************************/
template<typename C>
class shared_string ;
template<typename C>
class immutable_string ;
template<typename C>
class mutable_strbuf ;

/*******************************************************************************
 istring
 iwstring
*******************************************************************************/
typedef immutable_string<char>    istring ;
typedef immutable_string<wchar_t> iwstring ;

typedef mutable_strbuf<char>     mstrbuf ;
typedef mutable_strbuf<wchar_t>  mwstrbuf ;

/***************************************************************************//**
 Reference-counted storage for both the immutable_string and mutable_strbuf
*******************************************************************************/
template<typename C>
struct refcounted_strdata {
      active_counter_base<std::atomic<intptr_t>> _refcount ;
      size_t   _size ;
      C        _begin[1] ;

      static const refcounted_strdata<C> zero ;

      union alignment {
            intptr_t _1 ;
            size_t   _2 ;
            C     _3 ;
      } ;

      size_t size() const { return _size ; }
      C *begin() { return _begin + 0 ; }
      C *end() { return _begin + _size ; }
      const C *begin() const { return _begin + 0 ; }
      const C *end() const { return _begin + _size ; }
} ;

/***************************************************************************//**
 Reference-counted storage for pcomn::shared_string.

 This storage shall always provide space for at least one additional
 character after requested size, namely value_type() (0 in most cases) in order
 to preserve C string compatibility.
 I.e. both storage.end() + 1 is a valid past-the-end iterator, access to both
 *storage.end() is always allowed and produces value_type().
*******************************************************************************/
template<typename C, class Allocator = typename std::basic_string<C>::allocator_type>
class refcounted_storage {
      typedef refcounted_storage<C, Allocator>  self_type ;

      template<typename>
      friend class mutable_strbuf ;

   protected:
      typedef refcounted_strdata<C> data_type ;

   public:
      typedef C                                 value_type ;
      typedef Allocator                         allocator_type ;
      typedef typename Allocator::pointer       iterator ;
      typedef typename Allocator::const_pointer const_iterator ;
      typedef typename Allocator::size_type     size_type ;

      refcounted_storage() noexcept :
         _data(const_cast<value_type *>(data_type::zero.begin()))
      {}

      refcounted_storage(const refcounted_storage &source) noexcept :
         _data(source._data)
      {
         incref(str_data()) ;
      }

      refcounted_storage(refcounted_storage &&source) noexcept
      {
         swap(source) ;
      }

      refcounted_storage(const allocator_type &) :
         _data(const_cast<value_type *>(data_type::zero.begin()))
      {}

      refcounted_storage(const value_type *source, size_type len,
                         const allocator_type & = allocator_type()) ;

      refcounted_storage(size_type len, value_type c,
                         const allocator_type & = allocator_type()) ;

      ~refcounted_storage() noexcept { do_decref() ; }

      refcounted_storage &operator=(const refcounted_storage &source)
      {
         if (_data == source._data)
            return *this ;

         incref(const_cast<data_type &>(source.str_data())) ;
         do_decref() ;
         _data = source._data ;

         return *this ;
      }

      refcounted_storage &operator=(refcounted_storage &&source) noexcept
      {
         if (_data != source._data)
            refcounted_storage(std::move(source)).swap(*this) ;
         return *this ;
      }

      size_type size() const noexcept { return size_type(str_data().size()) ; }
      size_type capacity() const noexcept { return size() ; }
      size_type max_size() const noexcept { return allocator_type::max_size() ; }

      void swap(refcounted_storage &rhs) noexcept { std::swap(_data, rhs._data) ; }

      const value_type *begin() const noexcept { return _data ; }
      const value_type *end() const noexcept { return str_data().end() ; }

      const value_type *c_str() const noexcept { return begin() ; }
      const value_type *data() const noexcept { return begin() ; }

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

      // Sets actually allocated memory size into char_count.
      void *do_alloc(size_type &char_count) const ;

      void do_dealloc(data_type *d) const noexcept
      {
         // Zero stings are _never_ deleted, they are static
         NOXCHECK(d->_size) ;
         actual_allocator().deallocate(reinterpret_cast<aligner *>(d), aligner_count(d->_size)) ;
      }

      void clear()
      {
         do_decref() ;
         _data = const_cast<value_type *>(data_type::zero.begin()) ;
      }

   private:
      value_type *_data ;

   private:
      typedef typename data_type::alignment aligner ;
      typedef typename allocator_type::template rebind<aligner>::other actual_allocator ;

      // Sets actually allocated memory size into char_count.
      data_type *create_str_data(size_type &char_count) const ;

      data_type &str_data()
      {
         return *reinterpret_cast<data_type *>
            (reinterpret_cast<char *>(_data) - offsetof(data_type, _begin)) ;
      }
      const data_type &str_data() const { return const_cast<self_type *>(this)->str_data() ; }

      static intptr_t incref(data_type &data)
      {
         // Please note that all zero-length strings are static by design
         // and never increfed/decrefed.
         // While we could avoid checking for empty strings by making initial
         // static empty string refcount equal to 1 and then counting empty
         // string(s) references the same way as for nonempty, actually checking
         // _is_ an optimization since allows to avoid cache flushing by avoiding
         // write operations.
         return data._size ? data._refcount.inc_passive() : 0 ;
      }

      static size_type aligner_count(size_type count)
      {
         return
            (sizeof(data_type)
             + (std::max(count, static_cast<size_type>(1)) - 1) * sizeof(value_type)
             + sizeof(aligner) - 1)
            /  sizeof(aligner) ;
      }

      void do_decref() noexcept
      {
         data_type *d = &str_data() ;
         if (d->_size && !d->_refcount.dec_passive())
            do_dealloc(d) ;
      }
} ;

/***************************************************************************//**
 The common base for pcomn::immutable_string and pcomn::mutable_strbuf.

 Implements all constant methods of std::basic_string so is interface-compatible
 with it by read operations.
*******************************************************************************/
template<typename C>
class shared_string : protected refcounted_storage<C> {
   public:
      typedef refcounted_storage<C> storage_type ;
      typedef C                     value_type ;
      typedef C                     char_type ;
      typedef std::char_traits<C>   traits_type ;

      typedef size_t            size_type ;
      typedef const char_type * pointer ;
      typedef const char_type * const_pointer ;

      typedef const char_type * iterator ;
      typedef const char_type * const_iterator;
      typedef std::reverse_iterator<iterator>         reverse_iterator ;
      typedef std::reverse_iterator<const_iterator>   const_reverse_iterator ;

      static constexpr size_type npos = static_cast<size_type>(-1) ;

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

      explicit operator std::basic_string<char_type>() const { return {begin(), end()} ; }

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

      size_type find_first_of(const value_type *s, size_type pos, size_type n) const ;
      size_type find_last_of(const value_type* s, size_type pos, size_type n) const ;
      size_type find_first_not_of(const value_type *s, size_type pos, size_type n) const ;
      size_type find_last_not_of(const value_type* s, size_type pos, size_type n) const ;

      size_type find_first_of(const shared_string &str, size_type pos = 0) const
      {
         return find_first_of(str.data(), pos, str.size()) ;
      }

      size_type find_first_of(const value_type *s, size_type pos = 0) const
      {
         return find_first_of(s, pos, traits_type::length(s)) ;
      }

      size_type find_first_of(value_type c, size_type pos = 0) const
      {
         return find(c, pos) ;
      }

      size_type find_last_of(const shared_string &str, size_type pos = npos) const
      {
         return find_last_of(str.data(), pos, str.length());
      }

      size_type find_last_of(const value_type* s, size_type pos = npos) const
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
      shared_string() = default ;
      ~shared_string() = default ;

      shared_string(const shared_string &) = default ;
      shared_string(shared_string &&) = default ;

      shared_string(const value_type *source, size_type len) : storage_type(source, len) {}

      shared_string(size_type n, char_type c) : storage_type(n, c) {}

      shared_string(const shared_string &str, size_type pos, size_type n)
      {
         const size_type srcsize = str.size() ;
         ensure_le<std::out_of_range>(pos, srcsize, "String position is out of range") ;
         const size_type newsize = std::min(n, srcsize - pos) ;

         if (!newsize) return ;

         if (newsize == srcsize)
         {
            *static_cast<storage_type *>(this) = static_cast<const storage_type &>(str) ;
            return ;
         }

         storage_type tmp (str.data() + pos, newsize) ;
         storage_type::swap(tmp) ;
      }

      void assign(const shared_string &src) { storage_type::operator=(src) ; }
      void assign(shared_string &&src) noexcept { storage_type::operator=(std::move(src)) ; }

      void swap(shared_string &other) noexcept { storage_type::swap(other) ; }

   private:
      template<class Predicate>
      size_type __find_first(const value_type *s, size_type pos, size_type n) const ;

      template<class Predicate>
      size_type __find_last(const value_type *s, size_type pos, size_type n) const ;

      __noreturn void bad_pos(size_type pos) const ;

      void operator=(pcomn::Instantiate) ;
} ;

/*******************************************************************************
 mutable_strbuf_ref - proxy reference for mutable_strbuf copying.
*******************************************************************************/
template<typename C>
struct mutable_strbuf_ref {
      mutable_strbuf_ref(mutable_strbuf<C> &rval) : _ref(rval) {}

      mutable_strbuf<C> &_ref ;
} ;

/***************************************************************************//**
 The mutable string buffer with move-only (no copy) logic.
*******************************************************************************/
template<typename C>
class mutable_strbuf : public shared_string<C> {
      typedef shared_string<C>               ancestor ;
      typedef typename ancestor::data_type   data_type ;

      friend class immutable_string<C> ;

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

      typedef mutable_strbuf_ref<char_type> ref_type ;

      using ancestor::npos ;
      using ancestor::data ;
      using ancestor::begin ;
      using ancestor::end ;
      using ancestor::rbegin ;
      using ancestor::rend ;

      mutable_strbuf() = default ;
      mutable_strbuf(mutable_strbuf &src) { swap(src) ; }
      mutable_strbuf(const ref_type &src) { swap(src._ref) ; }

      template<typename S, typename=enable_if_strchar_t<S, char_type>>
      mutable_strbuf(const S &s) :
         ancestor(str::cstr(s), str::len(s)),
         _capacity(allocated_capacity(ancestor::size()))
      {}

      template<typename S>
      mutable_strbuf(const S &s, size_type from_pos,
                     enable_if_strchar_t<S, char_type, size_type> length) :
         ancestor(str::cstr(s) + ensure_le<std::out_of_range>(from_pos,
                                                              (size_type)str::len(s),
                                                              "String position is out of range"),
                  std::min(str::len(s) - from_pos, length)),
         _capacity(allocated_capacity(ancestor::size()))
      {}

      template<size_type n>
      mutable_strbuf(const char_type (&s)[n], size_type from_pos, size_type length) :
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
      std::enable_if_t<(!std::is_same_v<InputIterator, iterator> &&
                        !std::is_same_v<InputIterator, const_iterator> &&
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

      template<typename Z>
      std::enable_if_t<std::is_same<C, value_type>::value, mutable_strbuf &>
      append(const Z *input, size_type n)
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
         {
            if (!n)
               clear() ;
            else
            {
               data_type &data = storage_type::str_data() ;
               data._size = n ;
               *data.end() = value_type() ;
            }
         }
         return *this ;
      }

      template<typename S, typename=enable_if_strchar_t<S, char_type>>
      mutable_strbuf &operator+=(const S &rhs)
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

      template<typename S, typename=std::enable_if_t<is_string_or_char<S, char_type>::value>>
      mutable_strbuf operator+(const S &rhs)
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
      size_t _capacity = 0 ;

   private:
      data_type &reserve(size_type requested_capacity)
      {
         NOXCHECK(this->storage_type::_data &&
                  (!storage_type::str_data()._size || storage_type::str_data()._refcount.count() == 1)) ;
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

      void recapacitate(size_type requested_capacity) ;

      static size_type allocated_capacity(size_type requested_count)
      {
         return !!requested_count *
            (mutable_strbuf::allocated_count(storage_type::aligner_count(requested_count + 1)) - 1) ;
      }
} ;

/***************************************************************************//**
 The immutable reference-counted string read-interface compatible
 with std::basic_string.
*******************************************************************************/
template<typename C>
class immutable_string : public shared_string<C> {
      typedef shared_string<C>   ancestor ;
      typedef mutable_strbuf<C>  strbuf ;
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

      immutable_string() = default ;
      immutable_string(const immutable_string &) = default ;
      immutable_string(immutable_string &&) = default ;

      immutable_string(size_type n, char_type c) : ancestor(n, c) {}

      template<typename S, enable_if_strchar_t<S, char_type, nullptr_t> = nullptr>
      immutable_string(const S &s) :
         ancestor(str::cstr(s), str::len(s))
      {}

      immutable_string(const immutable_string &str, size_type from_pos, size_type length) :
         ancestor(str, from_pos, length)
      {}

      template<typename S>
      immutable_string(const S &s, size_type from_pos,
                       enable_if_strchar_t<S, char_type, size_type> length) :
         ancestor(str::cstr(s) + ensure_le<std::out_of_range>(from_pos,
                                                              (size_type)str::len(s),
                                                              "String position is out of range"),
                  std::min(str::len(s) - from_pos, length))
      {}

      immutable_string(const char_type *s, size_type from_pos, size_type length) :
         ancestor(s + from_pos, length == npos ? str::len(s + from_pos) : length)
      {}

      template<size_type n>
      immutable_string(const char_type (&s)[n], size_type from_pos, size_type length) :
         ancestor(s + ensure_lt<std::out_of_range>(from_pos, n, "String position is out of range"),
                  length == npos
                  ? (std::find(s + std::min(n, from_pos), s + n, 0) - s) - from_pos
                  : length)
      {}

      immutable_string(const char_type *begin, const char_type *end) : ancestor(begin, end - begin) {}

      immutable_string(strbuf &src) { from_strbuf(src) ; }

      immutable_string(const typename strbuf::ref_type &src) { from_strbuf(src._ref) ; }

      /* FIXME: Not implemented
         template<class InputIterator>
         immutable_string(InputIterator begin, InputIterator end) ;
      */

      immutable_string &operator=(const immutable_string &src)
      {
         ancestor::assign(src) ;
         return *this ;
      }

      immutable_string &operator=(immutable_string &&src) noexcept
      {
         ancestor::assign(std::move(src)) ;
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

      template<typename S, typename=enable_if_strchar_t<S, char_type>>
      immutable_string &operator=(const S &src)
      {
         immutable_string tmp (str::cstr(src), 0, str::len(src)) ;
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

      template<typename R, typename=std::enable_if_t<is_string_or_char<R, char_type>::value>>
      strbuf operator+(const R &rhs) const
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
PCOMN_DEFINE_SWAP(immutable_string<C>, template<typename C>) ;
PCOMN_DEFINE_SWAP(mutable_strbuf<C>,   template<typename C>) ;

/*******************************************************************************
 Decalre explicit instantiations.
*******************************************************************************/
extern template class immutable_string<char> ;
extern template class immutable_string<wchar_t> ;

extern template class mutable_strbuf<char> ;
extern template class mutable_strbuf<wchar_t> ;

/*******************************************************************************
 Stream output
*******************************************************************************/
template<typename C>
inline std::basic_ostream<C> &
operator<<(std::basic_ostream<C> &os, const shared_string<C> &str)
{
   return os << str.c_str() ;
}

template<class StreamTraits>
inline std::basic_ostream<char, StreamTraits> &
operator<<(std::basic_ostream<char, StreamTraits> &os, const shared_string<wchar_t> &str)
{
   const wchar_t *begin = str.c_str() ;
   return narrow_output(os, begin, begin + str.size()) ;
}

/*******************************************************************************
 String traits for immutable strings
*******************************************************************************/
template<typename C>
struct string_traits<shared_string<C>> : stdstring_traits<shared_string<C>, true> {} ;

template<typename C>
struct string_traits<immutable_string<C>> : stdstring_traits<immutable_string<C>, true> {} ;

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

#define DEFINE_RELOP_STR_STR(op)                       \
template<typename C>                                   \
inline bool operator op(const pcomn::shared_string<C> &x, const pcomn::shared_string<C> &y)  \
{                                                                 \
   return x.compare(y) op 0 ;                                     \
}

#define DEFINE_RELOP_STR_CSTR(op)                                       \
template<typename S, typename C>                                        \
inline pcomn::enable_if_other_string_t<pcomn::shared_string<C>, S, bool> \
operator op(const pcomn::shared_string<C> &x, const S &y)               \
{                                                                       \
   return x.compare(pcomn::str::cstr(y)) op 0 ;                         \
}

#define DEFINE_RELOP_CSTR_STR(op)                                       \
template<typename S, typename C>                                        \
inline pcomn::enable_if_other_string_t<pcomn::shared_string<C>, S, bool> \
operator op(const S &x, const pcomn::shared_string<C> &y)               \
{                                                                       \
   return -y.compare(pcomn::str::cstr(x)) op 0 ;                        \
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
template<typename L, class C>
inline enable_if_other_string_t<pcomn::shared_string<C>, L, L>
operator+(const L &lhs, const pcomn::immutable_string<C> &rhs)
{
   typedef mutable_strbuf<C> mbuf_type ;
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
} // end of namespace pcomn

#endif /* __PCOMN_IMMUTABLESTR_H */
