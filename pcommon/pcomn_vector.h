/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_VECTOR_H
#define __PCOMN_VECTOR_H
/*******************************************************************************
 FILE         :   pcomn_vector.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Simple (of constant, constructor-given size) vector classes.
                  Vector view (simple slice).

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   4 May 2000
*******************************************************************************/
#include <pcommon.h>
#include <pcomn_assert.h>
#include <pcomn_meta.h>
#include <pcomn_function.h>
#include <pcomn_iterator.h>
#include <pcomn_hash.h>

#include <algorithm>
#include <vector>
#include <memory>
#include <initializer_list>

#include <limits.h>

namespace pcomn {

/***************************************************************************//**
 Non-owning reference to a part (range) of a contiguous memory array,
 "unowning subvector"
*******************************************************************************/
template<typename T>
class simple_slice {
      template<typename P, typename U=P> struct enable_if_ptr_compat ;

      template<typename P, typename U>
      struct enable_if_ptr_compat<P*,U*> :
         std::enable_if<(std::is_convertible_v<P*,T*> &&
                         (std::is_same_v<std::remove_cv_t<P>, std::remove_cv_t<T>> ||
                          std::is_pointer_v<P> && std::is_pointer_v<T>))>
      {} ;

   public:
      typedef T value_type ;

      typedef value_type * iterator ;
      typedef iterator     const_iterator ;
      typedef value_type & reference ;
      typedef reference    const_reference ;

      constexpr simple_slice() = default ;
      constexpr simple_slice(const simple_slice &) = default ;

      constexpr simple_slice(value_type *start, value_type *finish) :
         _start(start), _finish(finish) {}

      constexpr simple_slice(value_type *start, size_t sz) :
         _start(start), _finish(_start + sz) {}

      template<size_t n>
      constexpr simple_slice(value_type (&data)[n]) :
         _start(data), _finish(data + n) {}

      template<typename V,
               typename = typename enable_if_ptr_compat<decltype(std::declval<V>().data()),
                                                        decltype(&*std::declval<V>().end())>::type>
      constexpr simple_slice(V &&src) :
         _start(std::forward<V>(src).data()),
         _finish(vector_end(std::forward<V>(src), is_pointer_t<decltype(src.begin())>()))
      {}

      #if PCOMN_WORKAROUND(__GNUC_VER__, >= 900)
      GCC_DIAGNOSTIC_PUSH_IGNORE(init-list-lifetime)
      #endif

      constexpr simple_slice(std::initializer_list<std::remove_cv_t<value_type>> init) :
         _start(init.begin()), _finish(init.end())
      {}

      #if PCOMN_WORKAROUND(__GNUC_VER__, >= 900)
      GCC_DIAGNOSTIC_POP()
      #endif

      /// Get the count of slice elements
      constexpr size_t size() const { return _finish - _start ; }

      /// Indicate that the slice is empty
      constexpr bool empty() const { return !size() ; }
      constexpr explicit operator bool() const { return !empty() ; }

      constexpr value_type *data() const { return _start ; }

      constexpr iterator begin() const { return _start ; }
      constexpr iterator end() const { return _finish ; }

      value_type &front() const { return *_start ; }
      value_type &back() const { return *(_finish-1) ; }

      value_type &operator[] (ptrdiff_t ndx) const { return _start[ndx] ; }

      simple_slice operator()(ptrdiff_t from, ptrdiff_t to) const
      {
         const ptrdiff_t sz = size() ;
         if (from < 0)
            from += sz ;
         if (to < 0)
            to += sz ;
         to = std::min(sz, std::max((ptrdiff_t)0, to)) ;
         from = std::min(sz, std::max((ptrdiff_t)0, from)) ;
         return from >= to
            ? simple_slice()
            : simple_slice(_start + from, _start + to) ;
      }

      simple_slice operator()(ptrdiff_t from) const
      {
         const ptrdiff_t sz = size() ;
         from = std::min(sz, std::max((ptrdiff_t)0, from < 0 ? from + sz : from)) ;
         return from >= sz
            ? simple_slice()
            : simple_slice(_start + from, end()) ;
      }

      void swap(simple_slice &other)
      {
         std::swap(_start, other._start) ;
         std::swap(_finish, other._finish) ;
      }

      std::vector<std::remove_cv_t<value_type>> to_vector() const
      {
         return std::vector<std::remove_cv_t<value_type>>(begin(), end()) ;
      }

      explicit operator std::vector<std::remove_cv_t<value_type>>() const
      {
         return to_vector() ;
      }

   private:
      value_type *_start   = nullptr ;
      value_type *_finish  = nullptr ;

   private:
      template<typename V>
      static value_type *vector_end(V &&v, std::true_type) { return std::forward<V>(v).end() ; }
      template<typename V>
      static value_type *vector_end(V &&v, std::false_type)
      {
         return std::forward<V>(v).data() + std::forward<V>(v).size() ;
      }
} ;

/// Slice of a constant array
template<typename T>
using simple_cslice = simple_slice<const T> ;

/******************************************************************************/
/** Non-resizable vector with dynamic allocation of storage at construction, with
 STL random-access container interface

 Copy-constructible, move-constructible, copy-assignable, move-assignable.
*******************************************************************************/
template<typename T>
class simple_vector {
      typedef std::remove_const_t<T>            mutable_value_type ;
      typedef std::add_const_t<T>               const_value_type ;
      typedef simple_slice<T>                   slice_type ;
      typedef simple_slice<mutable_value_type>  mutable_slice ;
      typedef simple_slice<const_value_type>    const_slice ;
   public:
      typedef T value_type ;
      typedef T * iterator ;
      typedef const T * const_iterator ;
      typedef T & reference ;
      typedef const T & const_reference ;

      constexpr simple_vector() = default ;
      explicit simple_vector(size_t size) : _base(size ? new mutable_value_type[size] : nullptr, size) {}

      simple_vector(size_t size, const value_type &init) : simple_vector(size)
      {
         std::fill(const_cast<mutable_value_type *>(begin()), const_cast<mutable_value_type *>(end()), init) ;
      }

      template<typename FwIterator>
      simple_vector(FwIterator begin, std::enable_if_t<is_iterator<FwIterator, std::forward_iterator_tag>::value, FwIterator> end) :
         simple_vector(end == begin ? 0 : std::distance(begin, end))
      {
         this->copy(begin, end) ;
      }

      simple_vector(const simple_vector &src) : simple_vector(src.begin(), src.end()) {}

      simple_vector(simple_vector &&src) : _base(src.begin(), src.end()) { slice_type().swap(src._base) ; }

      template<typename U>
      simple_vector(const simple_slice<U> &src) : simple_vector(src.begin(), src.end()) {}
      template<typename U>
      simple_vector(const simple_vector<U> &src) : simple_vector(src.begin(), src.end()) {}
      template<size_t n, typename U>
      simple_vector(U (&data)[n]) : simple_vector(data + 0, data + n) {}

      simple_vector(std::initializer_list<value_type> init) : simple_vector(init.begin(), init.end()) {}

      ~simple_vector() { clear() ; }

      /// Get the count of vector elements
      constexpr size_t size() const { return _base.size() ; }
      /// Indicate that the slice is empty
      constexpr bool empty() const { return !size() ; }

      void swap(simple_vector &other) { _base.swap(other._base) ; }

      void clear()
      {
         const iterator data = begin() ;
         _base = {} ;
         delete[] data ;
      }

      simple_vector &operator=(const simple_vector &src) { return assign(src.base()) ; }
      simple_vector &operator=(simple_vector &&src)
      {
         simple_vector(std::move(src)).swap(*this) ;
         return *this ;
      }

      simple_vector &operator=(const simple_vector
                               <std::conditional_t<std::is_const<value_type>::value, mutable_value_type, const_value_type> > &src)
      {
         return assign((const const_slice &)src) ;
      }

      simple_vector &operator=(const const_slice &src) { return assign(src) ; }
      simple_vector &operator=(const mutable_slice &src) { return assign(src) ; }

      iterator begin() { return _base.begin() ; }
      const_iterator begin() const { return _base.begin() ; }
      iterator end() { return _base.end() ; }
      const_iterator end() const { return _base.end() ; }

      value_type &front() { return _base.front() ; }
      const value_type &front() const { return _base.front() ; }
      value_type &back() { return _base.back() ; }
      const value_type &back() const { return _base.back() ; }

      value_type &operator[](ptrdiff_t ndx) { return _base[ndx] ; }
      const value_type &operator[](ptrdiff_t ndx) const { return _base[ndx] ; }

      slice_type operator()(ptrdiff_t from, ptrdiff_t to = INT_MAX)
      {
         return base()(from, to) ;
      }
      const_slice operator()(ptrdiff_t from, ptrdiff_t to = INT_MAX) const
      {
         return const_base()(from, to) ;
      }

      operator const const_slice &() const { return const_base() ; }
      operator const slice_type &() { return base() ; }

      friend bool operator==(const simple_vector &x, const simple_vector &y) { return x._base == y._base ; }
      friend bool operator!=(const simple_vector &x, const simple_vector &y) { return !(x == y) ; }

   protected:
      template<typename U>
      simple_vector &assign(const simple_slice<U> &src)
      {
         if (src.begin() != this->begin())
         {
            simple_vector().swap(*this) ;
            *this = std::move(simple_vector(src)) ;
         }
         return *this ;
      }

      const const_slice &const_base() const
      {
         return *reinterpret_cast<const const_slice *>(&base()) ;
      }

      slice_type &base() const { return const_cast<simple_vector *>(this)->_base ; }

   private:
      template<typename ForwardIterator>
      mutable_value_type *copy(ForwardIterator from, ForwardIterator to)
      {
         return std::copy(from, to, const_cast<mutable_value_type *>(begin())) ;
      }

   private:
      slice_type _base ;
} ;

/*******************************************************************************
                     template<class T>
                     class simple_ivector
*******************************************************************************/
template<typename T>
class simple_ivector : public simple_vector<T *> {
      typedef simple_vector<T *> ancestor ;
   public:
      typedef typename ancestor::value_type     value_type ;
      typedef typename ancestor::iterator       iterator ;
      typedef typename ancestor::const_iterator const_iterator ;

      simple_ivector() = default ;
      simple_ivector(size_t size, T *init = nullptr) : ancestor(size, init) {}

      simple_ivector(const simple_slice<const value_type> &src, bool owns = false) : ancestor(src), _owns(owns) {}
      simple_ivector(const simple_ivector &src, bool owns = false) : ancestor(src), _owns(owns) {}

      simple_ivector(const_iterator begin, const_iterator end, bool owns = false) : ancestor(begin, end), _owns(owns) {}

      ~simple_ivector() { _detach() ; }

      simple_ivector &operator=(const simple_ivector &src) { return assign(src) ; }

      bool owns_elements() const { return _owns ; }
      bool owns_elements(bool owns)
      {
         bool old = _owns ;
         _owns = owns ;
         return old ;
      }

      void clear()
      {
         _detach() ;
         std::fill(this->begin(), this->end(), (T *)0) ;
      }

      void swap(simple_ivector<T *> &other)
      {
         std::swap(_owns, other._owns) ;
         ancestor::swap(other) ;
      }

   protected:
      simple_ivector &assign(const simple_slice<const value_type> &src)
      {
         if (&src != this)
         {
            _detach() ;
            ancestor::assign(src) ;
         }
         return *this ;
      }

   private:
      bool _owns = false ;

      void _detach()
      {
         if (owns_elements())
            std::for_each(this->begin(), this->end(), std::default_delete<T>()) ;
      }
} ;

/*******************************************************************************
                     template<typename T, size_t maxsz>
                     class static_vector
*******************************************************************************/
template<typename T, size_t maxsize>
class static_vector {
   public:
      typedef T         value_type ;
      typedef T *       iterator ;
      typedef const T * const_iterator ;
      typedef T &       reference ;
      typedef const T & const_reference ;
      typedef size_t    size_type ;

      explicit static_vector(size_t size = 0) : _size(size)
      {
         NOXPRECONDITION(size <= maxsize) ;
      }

      static_vector(const static_vector &src) : _size(src.size())
      {
         std::copy(src.begin(), src.end(), mutable_data()) ;
      }

      static_vector(size_t size, const value_type &init) : static_vector(size)
      {
         std::fill(mutable_data(), mutable_data() + size, init) ;
      }

      template<typename I>
      static_vector(I start, std::enable_if_t<is_iterator<I, std::random_access_iterator_tag>::value, I> finish) :
         static_vector(finish - start)
      {
         std::copy(start, finish, mutable_data()) ;
      }

      static_vector(const simple_slice<const T> &src) :
         static_vector(src.begin(), src.end())
      {}

      size_t size() const { return _size ; }

      size_t max_size() const { return maxsize ; }

      /// Indicate that the slice is empty
      bool empty() const { return !size() ; }

      simple_slice<const T> operator()(ptrdiff_t from, ptrdiff_t to = INT_MAX) const
      {
         return simple_slice<const T>(begin(), end())(from, to) ;
      }

      simple_slice<T> operator()(ptrdiff_t from, ptrdiff_t to = INT_MAX)
      {
         return simple_slice<T>(begin(), end())(from, to) ;
      }

      operator simple_slice<const T>() const { return {begin(), end()} ; }
      operator simple_slice<T>() { return {begin(), end()} ; }

      iterator begin() { return _data ; }
      iterator end() { return _data + _size ; }

      const_iterator begin() const  { return _data ; }
      const_iterator end() const { return _data + _size ; }

      value_type &front() { return *_data ; }
      value_type &back() { return _data[_size - 1] ; }

      const value_type &front() const { return *_data ; }
      const value_type &back() const { return _data[_size - 1] ; }

      value_type *data() { return _data ; }
      const value_type *data() const { return _data ; }

      value_type &operator[] (ptrdiff_t ndx)
      {
         NOXPRECONDITION(ndx < (ptrdiff_t)size()) ;
         return _data[ndx] ;
      }
      const value_type &operator[] (ptrdiff_t ndx) const
      {
         NOXPRECONDITION(ndx < (ptrdiff_t)size()) ;
         return _data[ndx] ;
      }

      value_type &push_back(const value_type &item)
      {
         NOXPRECONDITION(size() < maxsize) ;
         value_type &r = _data[_size] = item ;
         ++_size ;
         return r ;
      }
      value_type pop_back()
      {
         NOXPRECONDITION(size()) ;
         return std::move(_data[_size--]) ;
      }

      template<typename... Args>
      value_type &emplace_back(Args && ...args)
      {
         NOXPRECONDITION(size() < maxsize) ;
         value_type &r = _data[_size] = value_type(std::forward<Args>(args)...) ;
         ++_size ;
         return r ;
      }

      template<typename InputIterator>
      void append(InputIterator sta, InputIterator fin)
      {
         for (iterator i = end(), e = i + maxsize ; i != e && sta != fin ; ++i, ++_size, ++sta)
            *i = *sta ;
         NOXCHECK(sta == fin) ;
      }

      void clear() { _size = 0 ; }

      void resize(size_t newsize)
      {
         NOXPRECONDITION(newsize <= size()) ;
         _size = std::min(newsize, _size) ;
      }

      void swap(static_vector &vec)
      {
         size_t len = std::max(size(), vec.size()) ;
         std::swap(_size, vec._size) ;
         std::swap_ranges(begin(), begin()+len, vec.begin()) ;
      }
   private:
      size_t   _size ;
      T        _data[maxsize] ;

      std::remove_cv_t<T> *mutable_data() { return (std::remove_cv_t<T> *)(_data + 0) ; }
} ;

/***************************************************************************//**
 Statically-sized stack of POD items, does not use dynamic memory.

 Quite speed-effective (in fact, equal to C), but can be used for POD data ONLY
 since it doesn't care of constructors, destructors, etc.
*******************************************************************************/
template<class T, size_t max_size>
class static_stack {
   public:
      typedef T value_type ;
      typedef T & reference ;
      typedef const T & const_reference ;

      static_stack() : _top(_data) {}

      static_stack(const static_stack<T, max_size> &src) :
         _data(_data),
         _top(_data + src.size())
      {}

      const T &top() const
      {
         NOXPRECONDITION(!empty()) ;
         return *(_top - 1) ;
      }

      T &top()
      {
         NOXPRECONDITION(!empty()) ;
         return *(_top - 1) ;
      }

      const T &top(unsigned depth) const
      {
         NOXPRECONDITION(_top - depth > _data) ;
         return *(_top - ++depth) ;
      }

      T &top(unsigned depth)
      {
         NOXPRECONDITION(_top - depth > _data) ;
         return *(_top - ++depth) ;
      }

      T &push(const T &item)
      {
         NOXPRECONDITION(!full()) ;
         *_top = item ;
         return *_top++ ;
      }

      void pop()
      {
         NOXPRECONDITION(!empty()) ;
         --_top ;
      }

      void pop(unsigned num)
      {
         _top -= std::min(num, (unsigned)size()) ;
      }

      // size()   -  get the number of vector elements
      //
      size_t size() const { return _top - _data ; }

      static size_t capacity() { return max_size ; }

      // empty()  -  return true if the stack is empty
      //
      bool empty() const { return _top == _data ; }

      bool full() const { return _top == _data + max_size ; }

      void swap(static_stack<T, max_size> &other)
      {
         size_t len = std::max(size(), other.size()) ;
         std::swap_ranges(_data, _data+len, other._data) ;
         std::swap(_top, other._top) ;
      }
   private:
      T  _data[max_size] ;
      T *_top ;
} ;

/***************************************************************************//**
 Movable, non-copyable sorted set of trivially copyable items.
 Avoids dynamic allocation in the single-item case.
*******************************************************************************/
template<typename T>
class trivial_set {
      static_assert(std::is_trivially_copyable<T>::value, "trivial_set<> item type must be trivially copyable, but is not") ;
      typedef pcomn::simple_slice<T> value_set ;
   public:
      typedef T value_type ;

      typedef typename value_set::const_iterator iterator ;
      typedef iterator const_iterator ;

      trivial_set() : _item_single() {}
      trivial_set(trivial_set &&other) : trivial_set() { swap(other) ; }

      template<typename InputIterator>
      trivial_set(InputIterator b, InputIterator e) : trivial_set()
      {
         construct(b, e, pcomn::is_iterator<InputIterator, std::forward_iterator_tag>()) ;
      }

      /// Conversion constructor, implicit
      trivial_set(value_type single) :
         _item_single(single),
         _item_set(&_item_single, &_item_single + 1)
      {}

      ~trivial_set() { clear() ; } ;

      trivial_set &operator=(trivial_set &&other)
      {
         if (&other != this)
         {
            swap(other) ;
            trivial_set().swap(other) ;
         }
         return *this ;
      }

      void swap(trivial_set &other)
      {
         if (&other == this)
            return ;
         using std::swap ;
         swap(_item_set, other._item_set) ;
         swap(_item_single, other._item_single) ;
         if (_item_set.begin() == &other._item_single)
            value_set(&_item_single, &_item_single + 1).swap(_item_set) ;
         if (other._item_set.begin() == &_item_single)
            value_set(&other._item_single, &other._item_single + 1).swap(other._item_set) ;
      }

      const_iterator begin() const { return _item_set.begin() ; }
      const_iterator end() const { return _item_set.end() ; }
      size_t size() const { return _item_set.size() ; }
      bool empty() const { return !size() ; }

      bool has_member(const value_type &member) const
      {
         switch (size())
         {
            case 0: return false ;
            case 1: return member == _item_single ;
         }
         return std::binary_search(begin(), end(), member) ;
      }

      /// If only one item in the set
      const value_type &front() const { return _item_single ; }

      std::pair<const_iterator, bool> insert(const value_type &value) ;

      friend bool operator==(const trivial_set &x, const trivial_set &y)
      {
         const size_t xsz = x.size() ;
         if (y.size() != xsz)
            return false ;
         switch (xsz)
         {
            case 0: return true ;
            case 1: return x._item_single == y._item_single ;
         }
         return std::equal(x.begin(), x.end(), y.begin()) ;
      }

      friend bool operator!=(const trivial_set &x, const trivial_set &y) { return !(x == y) ; }

   private:
      value_type _item_single ;
      value_set  _item_set ;

   private:
      template<typename ForwardIterator>
      void construct(ForwardIterator b, ForwardIterator e, std::true_type)
      {
         switch (const size_t sz = std::distance(b, e))
         {
            case 0: break ;
            default:
            {
               value_type * const items = new value_type[sz] ;
               value_type * end = std::copy(b, e, items) ;
               std::sort(items, end) ;
               end = std::unique(items, end) ;
               if (end - items <= 1)
                  delete [] items ;
               else
               {
                  value_set(items, end).swap(_item_set) ;
                  break ;
               }
            }
            case 1:
               _item_single = *b ;
               value_set(&_item_single, &_item_single + 1).swap(_item_set) ;
               break ;
         }
      }

      template<typename InputIterator>
      void construct(InputIterator &b, InputIterator &e, std::false_type)
      {
         std::vector<value_type> v (b, e) ;
         construct(v.begin(), v.end(), std::true_type()) ;
      }

      void clear()
      {
         if (_item_set.begin() != &_item_single)
            delete [] _item_set.begin() ;
      }
} ;

template<typename T>
std::pair<typename trivial_set<T>::const_iterator, bool> trivial_set<T>::insert(const value_type &value)
{
   trivial_set nset (value) ;
   if (nset.empty())
      return {} ;

   if (empty())
   {
      swap(nset) ;
      return {begin(), true} ;
   }

   auto found = std::lower_bound(_item_set.begin(), _item_set.end(), value) ;
   if (found != _item_set.end() && *found == value)
      return {found, false} ;

   value_type * const items = new value_type[size() + 1] ;
   value_type * const valiter = std::copy(_item_set.begin(), found, items) ;
   *valiter = value ;
   value_set new_items (items, std::copy(found, _item_set.end(), valiter + 1)) ;
   clear() ;
   _item_set = new_items ;
   _item_single = new_items.front() ;

   return {valiter, true} ;
}

PCOMN_DEFINE_SWAP(trivial_set<T>, template<typename T>) ;


/***************************************************************************//**
 Contiguous view of a container with random-access iterator.
*******************************************************************************/
template<typename T>
struct container_slice {
      /// The sequence type
      typedef T container_type ;

      typedef decltype(std::begin(std::declval<container_type>()))       iterator ;
      typedef decltype(std::begin(std::declval<const container_type>())) const_iterator ;

      typedef iterator_difference_t<iterator>         difference_type ;
      typedef std::make_unsigned_t<difference_type>   size_type ;

      container_slice(container_type &c) : _container(&c) {}
      container_slice(container_type &c, iterator from, iterator to) :
         _container(&c),
         _range(size_type(from - c.begin()), size_type(to - c.begin()))
      {
         NOXCHECK(to >= from) ;
         NOXCHECK(from >= c.begin()) ;
      }

      iterator begin() const { return iter_at(_range.first) ; }
      iterator end() const { return iter_at(_range.second) ; }

      size_type size() const { return range_size(_range) ; }

   private:
      container_type *     _container ;
      unipair<size_type>   _range ;

   private:
      container_type &container() const { return *_container ; }

      iterator iter_at(size_type offset) const
      {
         NOXCHECK(offset <= container().size()) ;
         return std::begin(container()) + offset ;
      }
} ;

/*******************************************************************************
 simple_slice
*******************************************************************************/
template<typename T>
inline simple_slice<T> make_simple_slice(T *b, T *e)
{
   return simple_slice<T>(b, e) ;
}

template<typename V>
inline auto make_simple_slice(V &&v) -> simple_slice<std::remove_reference_t<decltype(*v.data())>>
{
   return {std::forward<V>(v)} ;
}

template<typename V>
inline auto make_simple_cslice(V &&v) -> simple_cslice<std::remove_reference_t<decltype(*v.data())>>
{
   return {std::forward<V>(v)} ;
}

template<typename T>
inline constexpr simple_cslice<T> make_simple_slice(std::initializer_list<T> v) { return {v.begin(), v.end()} ; }
template<typename T>
inline constexpr simple_cslice<T> make_simple_cslice(std::initializer_list<T> v) { return make_simple_slice(v) ; }

template<typename T, typename U>
inline simple_slice<T> cat_slices(T *dest, const simple_slice<U> &src)
{
   return {dest, std::copy(src.begin(), src.end(), dest)} ;
}

template<typename T, typename U, typename V>
inline simple_slice<T> cat_slices(T *dest, const simple_slice<U> &src1, const simple_slice<V> &src2)
{
   return {dest, std::copy(src2.begin(), src2.end(),
                           std::copy(src1.begin(), src1.end(),
                                     dest))} ;
}

/*******************************************************************************
 swap specializations
*******************************************************************************/
PCOMN_DEFINE_SWAP(static_vector<P_PASS(T, maxsize)>, template<typename T, size_t maxsize>) ;
PCOMN_DEFINE_SWAP(simple_slice<T>, template<typename T>) ;
PCOMN_DEFINE_SWAP(simple_vector<T>, template<typename T>) ;
PCOMN_DEFINE_SWAP(simple_ivector<T *>, template<typename T>) ;

/***************************************************************************//**
 simple_slice hash functor
*******************************************************************************/
template<typename T>
struct hash_fn<simple_slice<T>> : hash_fn_sequence<simple_slice<T>> {} ;

/*******************************************************************************
 Slice equality comparison
*******************************************************************************/
template<typename T, typename U>
inline std::enable_if_t<is_same_unqualified<T, U>::value, bool>
operator==(const simple_slice<T> &x, const simple_slice<U> &y)
{
   return
      x.size() == y.size() && (x.begin() == y.begin() || std::equal(x.begin(), x.end(), y.begin())) ;
}

template<typename T, typename U>
inline std::enable_if_t<is_same_unqualified<T, U>::value, bool>
operator!=(const simple_slice<T> &x, const simple_slice<U> &y)
{
   return !(x == y) ;
}

template<typename T, typename S>
inline std::enable_if_t<std::is_convertible_v<S, simple_cslice<T>>, bool>
operator==(const simple_slice<T> &x, const S &y)
{
   const simple_cslice<T> &sy = y ;
   return x == sy ;
}

template<typename T, typename S>
inline std::enable_if_t<std::is_convertible_v<S, simple_cslice<T>>, bool>
operator==(const S &x, const simple_slice<T> &y)
{
   const simple_cslice<T> &sx = x ;
   return sx == y ;
}

template<typename T, typename S>
inline std::enable_if_t<std::is_convertible_v<S, simple_cslice<T>>, bool>
operator!=(const simple_slice<T> &x, const S &y) { return !(x == y) ; }

template<typename T, typename S>
inline std::enable_if_t<std::is_convertible_v<S, simple_cslice<T>>, bool>
operator!=(const S &x, const simple_slice<T> &y) { return y != x ; }

/*******************************************************************************
 pbegin()/pend()
*******************************************************************************/
PCOMN_DEFINE_PRANGE(simple_slice<T>, template<typename T>) ;
PCOMN_DEFINE_PRANGE(simple_vector<T>, template<typename T>) ;
PCOMN_DEFINE_PRANGE(static_vector<P_PASS(T, maxsize)>, template<typename T, size_t maxsize>) ;

} // end of namespace pcomn

#endif /* __PCOMN_VECTOR_H */
