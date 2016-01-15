/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_IVECTOR_H
#define __PCOMN_IVECTOR_H
/*******************************************************************************
 FILE         :   pcomn_ivector.h
 COPYRIGHT    :   Yakov Markovitch, 1996-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   STL-like vector of pointers with the "object owning" logic

 CREATION DATE:   24 Jul 1996
*******************************************************************************/
#include <pcomn_def.h>
#include <pcomn_utils.h>

#include <vector>
#include <algorithm>

namespace pcomn {

/*******************************************************************************
                  template<class T> class ivector
*******************************************************************************/
template<class T>
class ivector : private std::vector<T *> {
      typedef std::vector<T *> ancestor ;
   public:
      typedef typename ancestor::value_type value_type ;
      typedef typename ancestor::reference reference ;
      typedef typename ancestor::const_reference const_reference ;
      typedef typename ancestor::iterator iterator ;
      typedef typename ancestor::const_iterator const_iterator ;
      typedef typename ancestor::const_reverse_iterator const_reverse_iterator ;
      typedef typename ancestor::reverse_iterator reverse_iterator ;
      typedef typename ancestor::difference_type difference_type ;
      typedef typename ancestor::size_type size_type ;

      using ancestor::begin ;
      using ancestor::end ;
      using ancestor::rbegin ;
      using ancestor::rend ;

      //
      // Capacity.
      //
      using ancestor::size ;
      using ancestor::max_size ;
      using ancestor::capacity ;
      using ancestor::empty ;
      using ancestor::reserve ;

      //
      // Element access.
      //
      using ancestor::at ;
      using ancestor::front ;
      using ancestor::back ;

      ivector() :
         _owns(false)
      {}

      explicit ivector(size_type n, bool owns = false) :
         ancestor (n, 0),
         _owns(owns)
      {}

      ivector(const ivector<T>& x, bool owns = false) :
         ancestor (x),
         _owns(owns)
      {}

      ~ivector()
      {
         detach (begin(), end()) ;
      }

      ivector<T>& operator = (const ivector<T>& x) ;

      bool owns_elements() const { return _owns ; }
      bool owns_elements(bool owns) { return xchange(_owns, owns) ; }

      void resize (size_type new_size, value_type value = NULL)
      {
         if (new_size < size())
            erase(begin() + new_size, end()) ;
         else
            ancestor::resize (new_size, value) ;
      }
      //
      // Modifiers.
      //
      using ancestor::push_back ;
      using ancestor::insert ;
      using ancestor::swap ;

      void pop_back()
      {
         detach(this->end()-1) ;
         ancestor::pop_back() ;
      }

      void assign (const_iterator first, const_iterator last)
      {
         erase(this->begin(), this->end()) ;
         insert(this->begin(), first, last) ;
      }

      void assign (size_type n)
      {
         erase(this->begin(), this->end()) ;
         insert(this->begin(), n, NULL) ;
      }

      void erase (iterator position)
      {
         detach (position) ;
         ancestor::erase((iterator)position) ;
         *this->end() = NULL ;
      }

      void erase(iterator first, iterator last)
      {
         detach(first, last) ;
         iterator e = this->end() ;
         iterator i = std::copy ((iterator)last, e, (iterator)first) ;
         std::fill(i, e, (value_type)NULL) ;
         ancestor::erase(i, e) ;
      }

      void clear()
      {
         detach(begin(), end()) ;
         ancestor::clear() ;
      }

      reference operator[](int ndx) { return ancestor::operator[](ndx) ; }
      const_reference operator[](int ndx) const { return ancestor::operator[](ndx) ; }

      void swap(ivector &other)
      {
         ancestor::swap(other) ;
         std::swap(_owns, other._owns) ;
      }

   private:
      bool _owns ;

   protected:
      void detach (iterator first, iterator last)
      {
         if (_owns)
            while(last != first)
               delete *--last ;
      }

      void detach (iterator iter)
      {
         if (_owns)
            delete *iter ;
      }
} ;

/*******************************************************************************
 swap
*******************************************************************************/
PCOMN_DEFINE_SWAP(ivector<T>, template<typename T>) ;

/*******************************************************************************
 Comparator functors for indirect-containers
*******************************************************************************/
template<typename Ptr>
struct i_less : public std::binary_function<const Ptr, const Ptr, bool> {
   bool operator () (const Ptr &p1, const Ptr &p2) const { return *p1 < *p2 ; }
} ;

template<typename Ptr>
struct i_equal : public std::binary_function<const Ptr, const Ptr, bool> {
   bool operator () (const Ptr &p1, const Ptr &p2) const { return p1 == p2 || *p1 == *p2 ; }
} ;

template<typename Ptr>
struct i_compare : public std::binary_function<const Ptr, const Ptr, int> {
   int operator () (const Ptr &p1, const Ptr &p2) const
      { return i_less<Ptr>() (p1, p2) ? -1 : i_less<Ptr>() (p2, p1) ; }
} ;

} // end of namespace pcomn

#endif /* __PCOMN_IVECTOR_H */
