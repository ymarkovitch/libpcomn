/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_SIMPLEMATRIX_H
#define __PCOMN_SIMPLEMATRIX_H
/*******************************************************************************
 FILE         :   pcomn_simplematrix.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Simple (of constant, constructor-given size) vector and matrix
                  classes

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   4 May 2000
*******************************************************************************/
#include <pcommon.h>
#include <pcomn_assert.h>
#include <pcomn_meta.h>
#include <pcomn_function.h>
#include <pcomn_iterator.h>

#include <algorithm>
#include <vector>
#include <memory>

#include <limits.h>

namespace pcomn {

/*******************************************************************************
                     template<class T>
                     class simple_slice
*******************************************************************************/
template<typename T>
class simple_slice {
      typedef std::remove_const_t<T> mutable_value_type ;
   public:
      typedef T value_type ;
      typedef T * iterator ;
      typedef iterator const_iterator ;
      typedef T & reference ;
      typedef reference const_reference ;

      constexpr simple_slice() : _start(), _finish() {}

      template<typename U>
      simple_slice(const simple_slice<U> &src, std::enable_if_t<std::is_same<T, std::add_const_t<U> >::value, Instantiate> = {}) :
         _start(const_cast<T *>(src.begin())), _finish(const_cast<T *>(src.end()))
      {}

      constexpr simple_slice(value_type *start, value_type *finish) :
         _start(start), _finish(finish) {}

      constexpr simple_slice(value_type *start, size_t sz) :
         _start(start), _finish(_start + sz) {}

      template<size_t n>
      constexpr simple_slice(value_type (&data)[n]) :
         _start(data), _finish(data + n) {}

      simple_slice(const std::vector<mutable_value_type> &src) :
         _start(const_cast<T *>(&*src.begin())), _finish(const_cast<T *>(&*src.end()))
      {}

      /// Get the count of slice elements
      constexpr size_t size() const { return _finish - _start ; }

      /// Indicate that the slice is empty
      constexpr bool empty() const { return !size() ; }

      simple_slice operator()(ptrdiff_t from, ptrdiff_t to = INT_MAX) const
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

      iterator begin() const { return _start ; }
      iterator end() const { return _finish ; }

      value_type &front() const { return *_start ; }
      value_type &back() const { return *(_finish-1) ; }

      value_type &operator[] (ptrdiff_t ndx) const { return _start[ndx] ; }

      void swap(simple_slice &other)
      {
         std::swap(_start, other._start) ;
         std::swap(_finish, other._finish) ;
      }

   private:
      value_type *_start ;
      value_type *_finish ;
} ;

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
      { return assign((const const_slice &)src) ; }

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

      explicit static_vector(size_t size = 0) :
         _size(size)
      {
         NOXPRECONDITION(size <= maxsize) ;
      }

      static_vector(size_t size, const value_type &init) :
         _size(size)
      {
         NOXPRECONDITION(size <= maxsize) ;
         std::fill(begin(), end(), init) ;
      }

      static_vector(const simple_slice<const T> &src) :
         _size(src.size())
      {
         NOXPRECONDITION(size() <= maxsize) ;
         std::copy(src.begin(), src.end(), begin()) ;
      }

      static_vector(const static_vector &src) :
         _size(src.size())
      {
         std::copy(src.begin(), src.end(), begin()) ;
      }

      static_vector(const T *start, const T *finish) :
         _size(finish - start)
      {
         NOXPRECONDITION(size() <= maxsize) ;
         std::copy(start, finish, begin()) ;
      }

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
         return _data[_size++] = item ;
      }
      value_type pop_back()
      {
         NOXPRECONDITION(size()) ;
         return _data[_size--] ;
      }

      template<typename InputIterator>
      void append(InputIterator sta, InputIterator fin)
      {
         for (iterator i = end(), e = i + maxsize ; i != e && sta != fin ; ++i, ++_size, ++sta)
            *i = *sta ;
         NOXCHECK(sta == fin) ;
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
} ;

/******************************************************************************/
/** Statically-sized stack of POD items, which does not use dynamic memory

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


/*******************************************************************************
                     template<class T>
                     class simple_column
*******************************************************************************/
template<class T>
class simple_column {
   public:
      template<class V>
      class base_iterator : public std::iterator<std::random_access_iterator_tag, T, ptrdiff_t> {
         public:
            typedef std::iterator<std::random_access_iterator_tag, T, ptrdiff_t> ancestor ;
            typedef V   value_type  ;
            typedef V & reference ;

            base_iterator() :
               _iter(NULL),
               _step(0)
            {}

            base_iterator(value_type *start, int step, int num) :
               _iter(start + step * num),
               _step(step)
            {}

            base_iterator &operator+=(int diff)
            {
               _iter += diff * _step ;
               return *this ;
            }

            base_iterator &operator-=(int diff)
            {
               _iter -= diff * _step ;
               return *this ;
            }

            base_iterator &operator++() { return *this += 1 ; }
            base_iterator &operator--() { return *this -= 1 ; }
            base_iterator operator++(int)
            {
               base_iterator tmp(*this) ;
               ++*this ;
               return tmp ;
            }
            base_iterator operator--(int)
            {
               base_iterator tmp(*this) ;
               --*this ;
               return tmp ;
            }

            base_iterator operator+ (int diff) const { return base_iterator<V>(*this) += diff ; }
            base_iterator operator- (int diff) const { return base_iterator<V>(*this) -= diff ; }
            int operator- (const base_iterator &other) const { return (_iter - other._iter)/_step ; }
            bool operator== (const base_iterator &i2) const { return _iter == i2._iter ; }
            bool operator!= (const base_iterator &i2) const { return _iter != i2._iter ; }
            bool operator< (const base_iterator &i2) const { return _iter < i2._iter ; }
            bool operator<= (const base_iterator& i2) const { return !(i2 < *this) ; }
            bool operator> (const base_iterator& i2) const { return i2 < *this ; }
            bool operator>= (const base_iterator& i2) const { return !(*this < i2) ; }

            reference operator*() const
            {
               return *_iter ;
            }

         private:
            value_type * _iter ;
            int          _step ;
      } ;

      typedef T value_type ;
      typedef value_type & reference ;
      typedef const value_type & const_reference ;
      typedef base_iterator<value_type> iterator ;
      typedef base_iterator<const value_type> const_iterator ;

      simple_column() :
         _start(NULL),
         _size(0),
         _step(0)
      {}

      simple_column(T *begin, int size, int step) :
         _start(begin),
         _size(size),
         _step(step)
      {}

      iterator begin() { return iterator(_start, _step, 0) ; }
      iterator end() { return iterator(_start, _step, _size) ; }

      const_iterator begin() const { return const_iterator(_start, _step, 0) ; }
      const_iterator end() const { return const_iterator(_start, _step, _size) ; }

      value_type &operator[] (int ndx)
      {
         return _start[ndx * _step] ;
      }

      const value_type &operator[] (int ndx) const
      {
         return _start[ndx * _step] ;
      }

      int size() const
      {
         return _size ;
      }

   private:
      T       *_start ;
      int      _size ;
      int      _step ;
} ;


/*******************************************************************************
                     template<class T>
                     class matrix_slice
*******************************************************************************/
template<typename T>
class matrix_slice {
   public:
      typedef T                              item_type ;
      typedef std::add_const_t<item_type>    const_item_type ;

      typedef simple_slice<item_type>        row_type ;
      typedef simple_slice<const_item_type>  const_row_type ;
      typedef simple_column<item_type>       column_type ;

   private:
      template<bool c>
      using iterator_descriptor = std::iterator<std::random_access_iterator_tag,
                                                std::conditional_t<c, const_row_type, row_type>,
                                                ptrdiff_t, void, void> ;

   public:
      template<bool c>
      class byrow_iterator : iterator_descriptor<c> {
            friend byrow_iterator<!c> ;
            typedef std::conditional_t<c, const matrix_slice, matrix_slice> container_type ;
         public:
            using typename iterator_descriptor<c>::value_type ;

            constexpr byrow_iterator() : _matrix(), _pos() {}

            byrow_iterator(container_type &matrix, ptrdiff_t pos = 0) :
               _matrix(&matrix),
               _pos(pos)
            {}

            byrow_iterator(const byrow_iterator<!c> &src, std::enable_if_t<c, Instantiate> = {}) :
               _matrix(src._matrix),
               _pos(src._pos)
            {}

            byrow_iterator &operator+=(ptrdiff_t diff)
            {
               _pos += diff ;
               return *this ;
            }
            byrow_iterator &operator-=(ptrdiff_t diff) { return *this += -diff ; }

            byrow_iterator &operator++() { return *this += 1 ; }
            byrow_iterator &operator--() { return *this -= 1 ; }
            byrow_iterator operator++(int)
            {
               byrow_iterator tmp(*this) ;
               operator++() ;
               return tmp ;
            }
            byrow_iterator operator--(int)
            {
               byrow_iterator tmp(*this) ;
               operator--() ;
               return tmp ;
            }
            byrow_iterator operator+ (ptrdiff_t diff) const { return byrow_iterator(*this) += diff ; }
            byrow_iterator operator- (ptrdiff_t diff) const { return byrow_iterator(*this) -= diff ; }
            ptrdiff_t operator-(const byrow_iterator &iter) const { return _pos - iter._pos ; }

            bool operator==(const byrow_iterator &i2) const { return _pos == i2._pos ; }
            bool operator<(const byrow_iterator &i2) const { return _pos < i2._pos ; }

            PCOMN_DEFINE_RELOP_METHODS(byrow_iterator) ;

            value_type operator*() const { return (*_matrix)[_pos] ; }

         private:
            container_type *  _matrix ;
            ptrdiff_t         _pos ;
      } ;

   public:
      typedef byrow_iterator<false> iterator ;
      typedef byrow_iterator<true>  const_iterator ;

      matrix_slice() = default ;

      matrix_slice(item_type *init, size_t rows, size_t cols) :
         _rows(rows),
         _cols(cols),
         _data(init)
      {
         NOXCHECK(init || empty()) ;
      }

      size_t size() const { return _rows ; }
      size_t rows() const { return _rows ; }
      size_t columns() const { return _cols ; }
      bool empty() const { return !(_rows | _cols) ; }

      unipair<size_t> dim() const { return {_rows, _cols} ; }

      row_type       row(int ndx)       { return {_data + _cols * ndx, columns()} ; }
      const_row_type row(int ndx) const { return {_data + _cols * ndx, columns()} ; }

      row_type operator[](int ndx) { return row(ndx) ; }
      const_row_type operator[](int ndx) const { return row(ndx) ; }

      const column_type column(int num) const { return column_type(_data + num, _rows, columns()) ; }
      column_type column(int num) { return column_type(_data + num, _rows, columns()) ; }

      iterator begin() { return {*this, 0} ; }
      iterator end() { return {*this, size()} ; }

      const_iterator begin() const { return {*this, 0} ; }
      const_iterator end() const { return {*this, size()} ; }

      const_iterator cbegin() const { return begin() ; }
      const_iterator cend() const { return end() ; }

      item_type *data() { return _data ; }
      const item_type *data() const { return _data ; }

      matrix_slice &fill(const T &init)
      {
         std::fill_n(_data, _rows * _cols, init) ;
         return *this ;
      }

   protected:
      item_type *reset(item_type *init, size_t rows, size_t cols)
      {
         item_type *old_data = _data ;
         _cols = cols ;
         _rows = rows ;
         _data = init ;
         return old_data ;
      }

   private:
      int   _rows = 0 ;
      int   _cols = 0 ;
      T *   _data = nullptr ;
} ;


/*******************************************************************************
                     template<class T>
                     class simple_matrix
*******************************************************************************/
template<typename T>
class simple_matrix : public matrix_slice<T> {
      typedef matrix_slice<T> ancestor ;
   public:
      simple_matrix() = default ;

      template<typename U>
      simple_matrix(const matrix_slice<U> &src,
                    std::enable_if_t<std::is_convertible<U, T>::value, Instantiate> = {}) ;

      simple_matrix(const simple_matrix &src) :
         simple_matrix(static_cast<const ancestor &>(src))
      {}

      simple_matrix(simple_matrix &&src) { swap(src) ; }

      simple_matrix(size_t rows, size_t cols) :
         ancestor(_new_data(rows, cols), rows, cols)
      {}

      simple_matrix(size_t rows, size_t cols, const T &init) :
         ancestor(_new_data(rows, cols), rows, cols)
      {
         this->fill(init) ;
      }

      ~simple_matrix()
      {
         delete [] this->data() ;
      }

      simple_matrix &reset(size_t rows, size_t cols)
      {
         delete [] ancestor::reset(_new_data(rows, cols), rows, cols) ;
         return *this ;
      }

      void swap(simple_matrix &other)
      {
         if (&other != this)
            pcomn_swap(static_cast<ancestor &>(other), *static_cast<ancestor *>(this)) ;
      }

      simple_matrix &operator=(const ancestor &src)
      {
         if (&src != this)
            *this = std::move(simple_matrix(src)) ;
         return *this ;
      }
      simple_matrix &operator=(const simple_matrix &src)
      {
         return *this = static_cast<const ancestor &>(src) ;
      }
      simple_matrix &operator=(simple_matrix &&src)
      {
         simple_matrix tmp ;
         tmp.swap(src) ;
         swap(tmp) ;
         return *this ;
      }

   private:
      static T *_new_data(size_t rows, size_t cols)
      {
         size_t sz = rows * cols ;
         return sz ? new T[sz] : nullptr ;
      }
} ;

PCOMN_DEFINE_SWAP(simple_matrix<T>, template<typename T>) ;

/*******************************************************************************
 simple_matrix
*******************************************************************************/
template<typename T>
template<typename U>
simple_matrix<T>::simple_matrix(const matrix_slice<U> &src,
                                std::enable_if_t<std::is_convertible<U, T>::value, Instantiate>) :
   ancestor(_new_data(src.rows(), src.columns()), src.rows(), src.columns())
{
   std::copy(src.data(), src.data() + src.rows() * src.columns(), this->data()) ;
}

/*******************************************************************************
 simple_slice
*******************************************************************************/
template<typename T>
inline const simple_slice<T> make_simple_slice(T *b, T *e)
{
   return simple_slice<T>(b, e) ;
}

template <typename T>
inline simple_slice<const T> make_simple_slice(const std::vector<T> &v)
{
   return simple_slice<const T>(v) ;
}

template <typename T, size_t maxsize>
inline simple_slice<const T> make_simple_slice(const static_vector<T, maxsize> &v)
{
   return v(0) ;
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
inline std::enable_if_t<std::is_convertible<S, simple_slice<std::add_const_t<T> > >::value, bool>
operator==(const simple_slice<T> &x, const S &y)
{
   const simple_slice<std::add_const_t<T> > &sy = y ;
   return x == sy ;
}

template<typename T, typename S>
inline std::enable_if_t<std::is_convertible<S, simple_slice<std::add_const_t<T> > >::value, bool>
operator==(const S &x, const simple_slice<T> &y)
{
   const simple_slice<std::add_const_t<T> > &sx = x ;
   return sx == y ;
}

} // end of namespace pcomn

#endif /* __SIMPLEMATRIX_H */
