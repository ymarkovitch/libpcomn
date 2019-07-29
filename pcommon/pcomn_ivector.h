/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_IVECTOR_H
#define __PCOMN_IVECTOR_H
/*******************************************************************************
 FILE         :   pcomn_ivector.h
 COPYRIGHT    :   Yakov Markovitch, 1996-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   STL-like vector of pointers with the "object owning" logic

 CREATION DATE:   24 Jul 1996
*******************************************************************************/
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

      ivector(const ivector &x, bool owns = false) :
         ancestor (x),
         _owns(owns)
      {}

      ivector(ivector &&) = default ;

      ~ivector()
      {
         detach (begin(), end()) ;
      }

      ivector &operator=(const ivector &) ;
      ivector &operator=(ivector &&) = default ;

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
template<typename = void> struct i_less ;
template<typename = void> struct i_equal ;

template<typename T>
struct i_less {
      typedef bool result_type ;
      typedef valtype_t<T> arg_type ;

      template<typename P1, typename P2>
      auto operator()(const P1 &p1, const P2 &p2) const->decltype(*p1 < *p2)
      {
         PCOMN_STATIC_CHECK(std::is_convertible<decltype(*p1), arg_type>() ||
                            std::is_convertible<decltype(*p2), arg_type>()) ;
         return *p1 < *p2 ;
      }
      template<typename P>
      auto operator()(const P &p1, const arg_type &p2) const->decltype(*p1 < p2) { return *p1 < p2 ; }
      template<typename P>
      auto operator()(const arg_type &p1, const P &p2) const->decltype(p1 < *p2) { return p1 < *p2 ; }
} ;

template<> struct i_less<void> {
      typedef bool result_type ;
      typedef bool is_transparent ;

      template<typename P1, typename P2>
      auto operator()(const P1 &p1, const P2 &p2) const->decltype(*p1 < *p2) { return *p1 < *p2 ; }
      template<typename V, typename P>
      auto operator()(const V &v, const P &p)     const->decltype(v < *p) { return v < *p ; }
      template<typename P, typename V>
      auto operator()(const P &p, const V &v)     const->decltype(*p < v) { return *p < v ; }
} ;

template<typename T>
struct i_equal {
      typedef bool result_type ;
      typedef valtype_t<T> arg_type ;

      template<typename P1, typename P2>
      auto operator()(const P1 &p1, const P2 &p2) const->decltype(*p1 == *p2)
      {
         PCOMN_STATIC_CHECK(std::is_convertible<decltype(*p1), arg_type>() ||
                            std::is_convertible<decltype(*p2), arg_type>()) ;
         return *p1 == *p2 ;
      }
      template<typename P>
      auto operator()(const P &p1, const arg_type &p2) const->decltype(*p1 == p2) { return *p1 == p2 ; }
      template<typename P>
      auto operator()(const arg_type &p1, const P &p2) const->decltype(p1 == *p2) { return p1 == *p2 ; }
} ;

template<> struct i_equal<void> {
      typedef bool result_type ;
      typedef bool is_transparent ;

      template<typename P1, typename P2>
      auto operator()(const P1 &p1, const P2 &p2) const->decltype(*p1 == *p2) { return *p1 == *p2 ; }
      template<typename V, typename P>
      auto operator()(const V &v, const P &p)     const->decltype(v == *p) { return v == *p ; }
      template<typename P, typename V>
      auto operator()(const P &p, const V &v)     const->decltype(*p == v) { return *p == v ; }
} ;

} // end of namespace pcomn

#endif /* __PCOMN_IVECTOR_H */
