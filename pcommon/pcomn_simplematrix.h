/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_SIMPLEMATRIX_H
#define __PCOMN_SIMPLEMATRIX_H
/*******************************************************************************
 FILE         :   pcomn_simplematrix.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2014. All rights reserved.
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

#include <iterator>
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
   public:
      typedef T value_type ;
      typedef T * iterator ;
      typedef const T * const_iterator ;
      typedef T & reference ;
      typedef const T & const_reference ;

      enum { is_const_value = std::is_const<value_type>::value } ;

      simple_slice() :
         _start(NULL),
         _finish(NULL)
      {}

      simple_slice(const simple_slice<typename std::remove_const<value_type>::type> &src) :
         _start(const_cast<T *>(src.begin())),
         _finish(const_cast<T *>(src.end()))
      {}

      simple_slice(value_type *start, value_type *finish) :
         _start(start),
         _finish(finish)
      {}

      simple_slice(value_type *start, size_t sz) :
         _start(start),
         _finish(_start + sz)
      {}

      template<size_t n>
      simple_slice(value_type (&data)[n]) :
         _start(data),
         _finish(data + n)
      {}

      simple_slice(typename std::conditional<is_const_value,
                   const std::vector<typename std::remove_const<value_type>::type>,
                   std::vector<value_type> >::type &src) :
         _start(&*src.begin()),
         _finish(&*src.end())
      {}

      /// Get the count of slice elements
      size_t size() const { return _finish - _start ; }

      /// Indicate that the slice is empty
      bool empty() const { return !size() ; }

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

      iterator begin() { return _start ; }
      iterator end() { return _finish ; }

      const_iterator begin() const { return _start ; }
      const_iterator end() const { return _finish ; }

      value_type &front() { return *_start ; }
      value_type &back() { return *(_finish-1) ; }

      const value_type &front() const { return *_start ; }
      const value_type &back() const { return *(_finish-1) ; }

      value_type &operator[] (ptrdiff_t ndx) { return _start[ndx] ; }
      const value_type &operator[] (ptrdiff_t ndx) const { return _start[ndx] ; }

      void swap(simple_slice<T> &other)
      {
         std::swap(_start, other._start) ;
         std::swap(_finish, other._finish) ;
      }

   protected:
      T *_start ;
      T *_finish ;
} ;

/*******************************************************************************
                     template<class T>
                     class simple_vector
*******************************************************************************/
template<class T>
class simple_vector : public simple_slice<T> {
      typedef simple_slice<T> ancestor ;
   public:
      typedef typename ancestor::value_type value_type ;
      typedef typename ancestor::iterator iterator ;
      typedef typename ancestor::const_iterator const_iterator ;

      explicit simple_vector(size_t size = 0) :
         ancestor(size ? new T[size] : NULL, size)
      {}

      simple_vector(size_t size, const value_type &init) :
         ancestor(size ? new T[size] : NULL, size)
      {
         std::fill(this->_start, this->_finish, init) ;
      }

      simple_vector(const ancestor &src) :
         ancestor(src.size() ? new T[src.size()] : NULL, (T *)NULL)
      {
         this->_finish = copy(src) ;
      }

      simple_vector(const simple_vector<T> &src) :
         ancestor(src.size() ? new T[src.size()] : NULL, (T *)NULL)
      {
         this->_finish = copy(src) ;
      }

      simple_vector(const T *begin, const T *end) :
         ancestor(end == begin ? NULL : new T[end - begin], (T *)NULL)
      {
         this->_finish = copy(begin, end) ;
      }

      template<typename ForwardIterator>
      simple_vector(ForwardIterator begin, ForwardIterator end, int) :
         ancestor(end == begin ? NULL : new T[std::distance(begin, end)], (T *)NULL)
      {
         this->_finish = copy(begin, end) ;
      }

      ~simple_vector()
      {
         delete[] this->_start ;
      }

      void swap(simple_vector &other) { ancestor::swap(other) ; }

      simple_vector<T> &operator= (const simple_slice<T> &src) { return assign(src) ; }
      simple_vector<T> &operator= (const simple_vector<T> &src) { return assign(src) ; }

   protected:
      simple_vector<T> &assign(const simple_slice<T> &src)
      {
         if (&src != this)
         {
            delete[] this->_start ;
            this->_start = this->_finish = NULL ;
            size_t sz = src.size() ;
            this->_start = sz ? new T[sz] : NULL ;
            this->_finish = copy(src) ;
         }
         return *this ;
      }

   private:
      template<class RandomAccessIterator>
      T *copy (RandomAccessIterator begin, RandomAccessIterator end)
      {
         return std::copy(begin, end, this->_start) ;
      }

      T *copy(const simple_slice<T> &src)
      {
         return copy(src.begin(), src.end()) ;
      }
} ;

/*******************************************************************************
                     template<class T>
                     class simple_ivector
*******************************************************************************/
template<class T>
class simple_ivector : public simple_vector<T *> {
      typedef simple_vector<T *> ancestor ;
   public:
      typedef typename ancestor::value_type     value_type ;
      typedef typename ancestor::iterator       iterator ;
      typedef typename ancestor::const_iterator const_iterator ;

      explicit simple_ivector(size_t size = 0) :
         ancestor(size, 0)
      {}

      simple_ivector(size_t size, T *init) :
         ancestor(size, init),
         _owns(false)
      {}

      simple_ivector(const simple_slice<T *> &src, bool owns = false) :
         ancestor(src),
         _owns(owns)
      {}

      simple_ivector(const simple_ivector<T *> &src, bool owns = false) :
         ancestor(src),
         _owns(owns)
      {}

      simple_ivector(const_iterator begin, const_iterator end, bool owns = false) :
         ancestor(begin, end),
         _owns(owns)
      {}

      ~simple_ivector() { _detach() ; }

      simple_ivector<T> &operator= (const simple_slice<T *> &src) { return assign(src) ; }
      simple_ivector<T> &operator= (const simple_ivector<T *> &src) { return assign(src) ; }

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
      simple_vector<T> &assign(const simple_slice<T *> &src)
      {
         if (&src != this)
         {
            _detach() ;
            ancestor::assign(src) ;
         }
         return *this ;
      }

   private:
      bool _owns ;

      void _detach()
      {
         if (owns_elements())
            std::for_each(this->begin(), this->end(), std::default_delete<value_type>()) ;
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

/*******************************************************************************
                     template<class T, size_t max_size>
                     class static_stack
 A statically-sized stack of POD items. Never uses dynamic memory allocation,
 quite speed-effective (in fact, equal to C), but can be used for POD data ONLY
 since it doesn't care of constructors, destructors, etc.
*******************************************************************************/
template<class T, size_t max_size>
class static_stack {
   public:
      typedef T value_type ;
      typedef T & reference ;
      typedef const T & const_reference ;

      static_stack() :
         _top(_data)
      {}

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
template<class T>
class matrix_slice {
   public:
      typedef T                 item_type ;
      typedef simple_slice<T>   value_type ;
      typedef simple_column<T>  column_type ;
      typedef value_type        row_type ;
      typedef row_type          reference ;
      typedef const row_type    const_reference ;
      class iterator ;
      class const_iterator ;

      class iterator :
         public std::iterator<std::random_access_iterator_tag, value_type, ptrdiff_t> {

            friend class const_iterator ;

         public:
            iterator(matrix_slice<T> &matrix, int pos = 0) :
               _matrix(&matrix),
               _pos(pos)
            {}

            iterator &operator+=(int diff)
            {
               _pos += diff ;
               return *this ;
            }
            iterator &operator-=(int diff) { return *this += -diff ; }

            iterator &operator++() { return *this += 1 ; }
            iterator &operator--() { return *this += -1 ; }
            iterator operator++(int)
            {
               iterator tmp(*this) ;
               operator++() ;
               return tmp ;
            }
            iterator operator--(int)
            {
               iterator tmp(*this) ;
               operator--() ;
               return tmp ;
            }
            iterator operator+ (int diff) const { return iterator(*this) += diff ; }
            iterator operator- (int diff) const { return iterator(*this) -= diff ; }
            int operator- (const iterator &iter) const { return _pos - iter._pos ; }
            bool operator== (const iterator &i2) const { return _pos == i2._pos ; }
            bool operator!= (const iterator &i2) const { return _pos != i2._pos ; }
            bool operator< (const iterator &i2) const { return _pos < i2._pos ; }
            bool operator<= (const iterator& i2) const { return ( !(i2 < *this)) ; }
            bool operator> (const iterator& i2) const { return i2 < *this ; }
            bool operator>= (const iterator& i2) const { return !(*this < i2) ; }

            value_type operator*() const
            {
               return (*_matrix)[_pos] ;
            }

         private:
            matrix_slice<T> *_matrix ;
            int               _pos ;
      } ;

      class const_iterator :
         public std::iterator<std::random_access_iterator_tag, value_type, ptrdiff_t> {

         public:
            const_iterator(const matrix_slice<T> &matrix, int pos = 0) :
               _matrix(&matrix),
               _pos(pos)
            {}

            const_iterator(const iterator &source) :
               _matrix(source._matrix),
               _pos(source._pos)
            {}

            const_iterator &operator+=(int diff)
            {
               _pos += diff ;
               return *this ;
            }
            const_iterator &operator-=(int diff) { return *this += -diff ; }

            const_iterator &operator++() { return *this += 1 ; }
            const_iterator &operator--() { return *this += -1 ; }
            const_iterator operator++(int)
            {
               const_iterator tmp(*this) ;
               operator++() ;
               return tmp ;
            }
            const_iterator operator--(int)
            {
               const_iterator tmp(*this) ;
               operator--() ;
               return tmp ;
            }
            const_iterator operator+ (int diff) const { return const_iterator(*this) += diff ; }
            const_iterator operator- (int diff) const { return const_iterator(*this) -= diff ; }
            int operator- (const const_iterator &iter) const { return _pos - iter._pos ; }
            bool operator== (const const_iterator &i2) const { return _pos == i2._pos ; }
            bool operator!= (const const_iterator &i2) const { return _pos != i2._pos ; }
            bool operator< (const const_iterator &i2) const { return _pos < i2._pos ; }
            bool operator<= (const const_iterator& i2) const { return ( !(i2 < *this)) ; }
            bool operator> (const const_iterator& i2) const { return i2 < *this ; }
            bool operator>= (const const_iterator& i2) const { return !(*this < i2) ; }

            const value_type operator*() const
            {
               return (*_matrix)[_pos] ;
            }

         private:
            const matrix_slice<T> *_matrix ;
            int               _pos ;
      } ;

      matrix_slice() :
         _rows(0),
         _cols(0),
         _data(NULL)
      {}

      matrix_slice(item_type *init, size_t rows, size_t cols) :
         _rows(rows),
         _cols(cols),
         _data(init)
      {}

      size_t size() const { return _rows ; }
      size_t rows() const { return _rows ; }
      size_t columns() const { return _cols ; }
      bool empty() const { return _rows * _cols  == 0 ; }

      std::pair<size_t, size_t> dim() const { return std::pair<size_t, size_t>(_rows, _cols) ; }

      row_type operator[](int ndx)
      {
         return row_type(_data + _cols * ndx, _cols) ;
      }

      const row_type operator[](int ndx) const
      {
         return row_type(_data + _cols * ndx, _cols) ;
      }

      const row_type row(int ndx) const { return row_type(_data + _cols * ndx, _cols) ; }
      row_type row(int ndx) { return row_type(_data + _cols * ndx, _cols) ; }

      const column_type column(int num) const { return column_type(_data + num, _rows, _cols) ; }
      column_type column(int num) { return column_type(_data + num, _rows, _cols) ; }

      iterator begin() { return iterator(*this) ; }
      iterator end() { return iterator(*this, size()) ; }

      const_iterator begin() const { return const_iterator(*this) ; }
      const_iterator end() const { return const_iterator(*this, size()) ; }

      item_type *data() { return _data ; }
      const item_type *data() const { return _data ; }

      matrix_slice<T> &fill(const T &init)
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
      int   _rows ;
      int   _cols ;
      T *   _data ;
} ;


/*******************************************************************************
                     template<class T>
                     class simple_matrix
*******************************************************************************/
template<class T>
class simple_matrix : public matrix_slice<T> {
      typedef matrix_slice<T> ancestor ;
   public:
      simple_matrix() {}

      simple_matrix(const simple_matrix<T> &src) :
         ancestor(_new_data(src.rows(), src.columns()), src.rows(), src.columns())
      {
         std::copy(src.data(), src.data() + src.rows() * src.columns(), this->data()) ;
      }

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

      simple_matrix<T> &reset(size_t rows, size_t cols)
      {
         delete [] ancestor::reset(_new_data(rows, cols), rows, cols) ;
         return *this ;
      }

      simple_matrix<T> &operator=(const simple_matrix<T> &src) ;

   private:
      static T *_new_data(size_t rows, size_t cols)
      {
         size_t sz = rows * cols ;
         return sz ? new T[sz] : NULL ;
      }
} ;

/*******************************************************************************
 simple_matrix
*******************************************************************************/
template<class T>
simple_matrix<T> &simple_matrix<T>::operator=(const simple_matrix<T> &src)
{
   ancestor new_obj (_new_data(src.rows(), src.columns()), src.rows(), src.columns()) ;
   std::copy(src.data(), src.data() + src.rows() * src.columns(), new_obj.data()) ;
   T *tmp = this->data() ;
   *static_cast<ancestor *>(this) = new_obj ;
   delete [] tmp ;
   return *this ;
}

/*******************************************************************************
 simple_slice
*******************************************************************************/
template <class T>
inline bool operator==(const simple_slice<T> &x, const simple_slice<T> &y)
{
   return
      &x == &y || x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin()) ;
}

template <class T>
inline bool operator!=(const simple_slice<T> &x, const simple_slice<T> &y)
{
   return !(x == y) ;
}

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

template <typename T>
inline simple_slice<T> cat_slices(T *dest, const simple_slice<T> &src1, const simple_slice<T> &src2)
{
   return simple_slice<T>(dest,
                          std::copy(src2.begin(),
                                    src2.end(),
                                    std::copy(src1.begin(),
                                              src1.end(),
                                              dest))) ;
}

/*******************************************************************************
 swap specializations
*******************************************************************************/
PCOMN_DEFINE_SWAP(static_vector<P_PASS(T, maxsize)>, template<typename T, size_t maxsize>) ;
PCOMN_DEFINE_SWAP(simple_slice<T>, template<typename T>) ;
PCOMN_DEFINE_SWAP(simple_vector<T>, template<typename T>) ;
PCOMN_DEFINE_SWAP(simple_ivector<T *>, template<typename T>) ;

} // end of namespace pcomn

#endif /* __SIMPLEMATRIX_H */
