/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_SIMPLEMATRIX_H
#define __PCOMN_SIMPLEMATRIX_H
/*******************************************************************************
 FILE         :   pcomn_simplematrix.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Simple non-resizable matrix class

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   4 May 2000
*******************************************************************************/
#include "pcomn_vector.h"

namespace pcomn {

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

/******************************************************************************/
/** A matrix over a contiguous memory range, does not own its memory
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
      class byrow_iterator : public iterator_descriptor<c> {
            friend byrow_iterator<!c> ;
            typedef std::conditional_t<c, const matrix_slice, matrix_slice> container_type ;
         public:
            using typename iterator_descriptor<c>::value_type ;

            constexpr byrow_iterator() : _matrix(), _pos() {}

            byrow_iterator(container_type &matrix, ptrdiff_t pos = 0) :
               _matrix(&matrix),
               _pos(pos)
            {}

            template<bool other, typename = instance_if_t<((int)other < (int)c)> >
            byrow_iterator(const byrow_iterator<other> &src) :
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

      matrix_slice(item_type *data, size_t rows, size_t cols) :
         _rows((int)rows),
         _cols((int)cols),
         _data(data)
      {
         NOXCHECK(_cols || !_rows) ;
         NOXCHECK(data || empty()) ;
      }

      matrix_slice(item_type *data, size_t cols, std::initializer_list<std::initializer_list<T> > init) ;

      matrix_slice(const matrix_slice<std::remove_const_t<item_type> > &other) :
         _rows((int)other.rows()),
         _cols((int)other.columns()),
         _data(const_cast<matrix_slice<std::remove_const_t<item_type> > &>(other).data())
      {}

      size_t size() const { return _rows ; }
      size_t rows() const { return _rows ; }
      size_t columns() const { return _cols ; }
      bool empty() const { return !_rows ; }

      /// Get both matrix dimensions {rows, columns}
      unipair<size_t> dim() const { return {_rows, _cols} ; }

      row_type       row(ptrdiff_t ndx)       { return {_data + _cols * ndx, columns()} ; }
      const_row_type row(ptrdiff_t ndx) const { return {_data + _cols * ndx, columns()} ; }

      row_type operator[](ptrdiff_t ndx) { return row(ndx) ; }
      const_row_type operator[](ptrdiff_t ndx) const { return row(ndx) ; }

      const column_type column(ptrdiff_t num) const { return column_type(_data + num, _rows, columns()) ; }
      column_type column(ptrdiff_t num) { return column_type(_data + num, _rows, columns()) ; }

      iterator begin() { return {*this, 0} ; }
      iterator end() { return {*this, size()} ; }

      const_iterator begin() const { return {*this, 0} ; }
      const_iterator end() const { return {*this, (ptrdiff_t)size()} ; }

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
 matrix_internal_storage
*******************************************************************************/
template<typename T, bool resizable>
struct matrix_internal_storage {
   protected:
      static void delete_data(T *data)
      {
         delete [] data ;
      }
      static T *new_data(size_t rows, size_t cols)
      {
         size_t sz = rows * cols ;
         return sz ? new T[sz] : nullptr ;
      }
      void resize_data(size_t, size_t) ;
} ;

template<typename T>
struct matrix_internal_storage<T, true> {
   protected:
      static void delete_data(T *) {}
      T *new_data(size_t rows, size_t cols) ;
      T *resize_data(size_t rows, size_t cols)
      {
         _data.resize(rows * cols) ;
         return _data.size() ? _data.data() : nullptr ;
      }
   private:
      std::vector<T> _data ;
} ;

template<typename T>
T *matrix_internal_storage<T, true>::new_data(size_t rows, size_t cols)
{
   _data.clear() ;
   _data.shrink_to_fit() ;
   if (const size_t sz = rows * cols)
   {
      _data.resize(sz) ;
      return _data.data() ;
   }
   return nullptr ;
}

/******************************************************************************/
/** A memory-owning matrix over a contiguous memory range
*******************************************************************************/
template<typename T, bool resizable = false>
class simple_matrix : matrix_internal_storage<T, resizable>, public matrix_slice<T>  {
      typedef matrix_slice<T> ancestor ;
      typedef matrix_internal_storage<T, resizable> storage ;
   public:
      using typename ancestor::item_type ;
      using typename ancestor::const_item_type ;

      simple_matrix() = default ;

      /// Copy constructor (O(n))
      simple_matrix(const simple_matrix &src) :
         simple_matrix(static_cast<const ancestor &>(src))
      {}

      /// Move constructor (O(1))
      simple_matrix(simple_matrix &&src) { swap(src) ; }

      simple_matrix(const matrix_slice<std::remove_const_t<item_type> > &other) :
         simple_matrix(other.data(), other.rows(), other.columns(), std::true_type())
      {}

      simple_matrix(const matrix_slice<const_item_type> &other) :
         simple_matrix(other.data(), other.rows(), other.columns(), std::true_type())
      {}

      simple_matrix(size_t rows, size_t cols) :
         ancestor(storage::new_data(rows, cols), rows, cols)
      {}

      simple_matrix(size_t rows, size_t cols, const T &init) :
         ancestor(storage::new_data(rows, cols), rows, cols)
      {
         this->fill(init) ;
      }

      simple_matrix(size_t cols, std::initializer_list<std::initializer_list<T> > init) ;

      ~simple_matrix() { storage::delete_data(this->data()) ; }

      simple_matrix &reset(size_t rows, size_t cols)
      {
         storage::delete_data(ancestor::reset(storage::new_data(rows, cols), rows, cols)) ;
         return *this ;
      }

      simple_matrix &resize(size_t rows)
      {
         ancestor::reset(storage::resize_data(rows, this->columns()), rows, this->columns()) ;
         return *this ;
      }

      void swap(simple_matrix &other)
      {
         if (&other != this)
         {
            pcomn_swap(static_cast<ancestor &>(other), *static_cast<ancestor *>(this)) ;
            pcomn_swap(static_cast<storage &>(other), *static_cast<storage *>(this)) ;
         }
      }

      /// Copy assignment (O(n))
      simple_matrix &operator=(const ancestor &src)
      {
         if (&src != this)
            *this = std::move(simple_matrix(src)) ;
         return *this ;
      }

      /// Copy assignment (O(n))
      simple_matrix &operator=(const simple_matrix &src)
      {
         return *this = static_cast<const ancestor &>(src) ;
      }

      /// Move assignment (O(1))
      simple_matrix &operator=(simple_matrix &&src)
      {
         simple_matrix tmp ;
         tmp.swap(src) ;
         swap(tmp) ;
         return *this ;
      }

   private:
      simple_matrix(const_item_type *other_data, size_t rows, size_t cols, std::true_type) ;
} ;

PCOMN_DEFINE_SWAP(simple_matrix<P_PASS(T, r)>, template<typename T, bool r>) ;

/*******************************************************************************
 matrix_slice
*******************************************************************************/
template<typename T>
matrix_slice<T>::matrix_slice(item_type *data, size_t cols, std::initializer_list<std::initializer_list<T> > init) :
   matrix_slice(data, init.size(), cols)
{
   unsigned rownum = 0 ;
   for (const auto &r: init)
   {
      ensure_eq<std::invalid_argument>
         (r.size(), cols, "Item count mismatch in the initializer of a matrix row") ;
      std::copy(r.begin(), r.end(), row(rownum).begin()) ;
      ++rownum ;
   }
}

/*******************************************************************************
 simple_matrix
*******************************************************************************/
template<typename T, bool r>
simple_matrix<T, r>::simple_matrix(const_item_type *src_data, size_t rows, size_t cols, std::true_type) :
   ancestor(storage::new_data(rows, cols), rows, cols)
{
   std::copy(src_data, src_data + rows * cols, this->data()) ;
}

template<typename T, bool r>
simple_matrix<T, r>::simple_matrix(size_t cols, std::initializer_list<std::initializer_list<T> > init)
   try : ancestor(storage::new_data(init.size(), cols), cols, init) {}
catch(...)
{
   storage::delete_data(this->data()) ;
}

/*******************************************************************************
 Debug output
*******************************************************************************/
template<typename T>
inline std::ostream &operator<<(std::ostream &os, const matrix_slice<T> &v)
{
   return os << '{' << PCOMN_TYPENAME(T) << '@' << v.data()
             << '[' << v.rows() << ']' << '[' << v.columns() << "]}" ;
}

} // end of namespace pcomn

#endif /* __SIMPLEMATRIX_H */
