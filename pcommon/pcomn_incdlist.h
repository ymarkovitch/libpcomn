/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_INCDLIST_H
#define __PCOMN_INCDLIST_H
/*******************************************************************************
 FILE         :   pcomn_incdlist.h
 COPYRIGHT    :   Yakov Markovitch, 1998-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Template for the doubly-linked inclusive list.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   24 Nov 1998
*******************************************************************************/
/** @file
  Doubly-linked inclusive list.

  @code
  struct ListItem {
      ListItem() ;
      ~ListItem() ;

      PCOMN_INCLIST_DECLARE(item_list) ;
   private:
      pcomn::incdlist_node _listnode ;
  } ;

  PCOMN_INCLIST_DEFINE(ListItem, item_list, _listnode, pcomn::incdlist) ;
  @endcode
*******************************************************************************/
#include <pcommon.h>
#include <pcomn_assert.h>
#include <iterator>
#include <stddef.h>

#define PCOMN_INCLIST_DECLARE(listtype) \
   struct listtype ; friend struct listtype

#define PCOMN_INCLIST_DEFINE(nodetype, listtype, nodemember, list_template)   \
   struct nodetype::listtype : list_template< nodetype, &nodetype::nodemember> {}

namespace pcomn {

/******************************************************************************/
/** Doubly-linked inclusive list.
 The fundamental difference between inclusive and non-inclusive list is that every data
 item of an inclusive list contains a list node (i.e. the node is a member of a data
 item), whereas for non-inclusive list this is the other way around, i.e. the _data_item_
 is a node member.
*******************************************************************************/
class PDList {
      PCOMN_NONASSIGNABLE(PDList) ;
   public:
      struct Node ;
      friend struct Node ;

      struct Node
      {
            friend class PDList ;
            typedef void (destructor)(Node *node) ;


            Node() :
               next(this),
               prev(this)
            {}

            /// The copy constructor.
            /// Doesn't copy list membership.
            Node(const Node &) :
               next(this),
               prev(this)
            {}

            ~Node()
            {
               // Removing itself from the list.
               // If we are already have been removed, then list == NULL and remove() call is
               // simply ignored.
               remove() ;
            }

            /// Assignment is no-op, since node assignment shall never cause carrying the
            /// node into another list.
            Node &operator= (const Node &) { return *this ; }

            /// Remove itself from the list.
            Node *remove()
            {
               preremove() ;
               postremove() ;
               return this ;
            }

            /// Append the node before another node.
            /// Ignores attemps to append a node before itself.
            Node *prepend(Node *element)
            {
               element->preremove() ;

               Node *prevn = prev ;
               Node *nextn = prevn->next ;

               element->next  = nextn ;
               element->prev  = prevn ;
               prevn->next    = nextn->prev = element ;

               return this ;
            }

            /// Append a node after the given node.
            /// Ignores attempts to append a node after itself.
            Node *append(Node *element)
            {
               next->prepend(element) ;
               return this ;
            }

            /// Indicates whether this node is standalone (not in any list).
            bool is_standalone() const { return next == this && prev == this ; }

            /// Count the number of nodes in a list slice.
            static _PCOMNEXP size_t count(const Node *start, const Node *finish) ;

            /// Remove a node range from the list.
            /// Disconnects all nodes in a range. After call to this function
            /// all nodes from @a start to @a finish become self-connected (single).
            static _PCOMNEXP size_t desintegrate(Node *start, Node *finish, destructor *dtr = NULL) ;

            static _PCOMNEXP size_t desintegrate(Node *start, size_t n, destructor *dtr = NULL) ;

            Node *next ;
            Node *prev ;

         protected:
            void preremove()
            {
               prev->next = next ;
               next->prev = prev ;
            }

            void postremove()
            {
               next = prev = this ;
            }
      } ;

      /// Indicate whether the list is empty.
      bool empty() const { return _zero.is_standalone() ; }

      /// Get the number of items in a list.
      /// Has linear time complexity.
      size_t size() const { return Node::count(first(), last()) ; }

   protected:
      Node              _zero ; /* The "zero" empty node */
      Node::destructor *_ndtr ; /* The function, which is called then node destruction is
                                 * necessary. Can be NULL. */

      PDList(Node::destructor *ndtr = NULL) :
         _ndtr(ndtr)
      {}

      PDList(const PDList &lst) :
         _ndtr(lst._ndtr)
      {}

      ~PDList()
      {
         erase(first(), last()) ;
      }

      Node *first()
      {
         return _zero.next ;
      }

      Node *last()
      {
         return &_zero ;
      }

      const Node *first() const
      {
         return _zero.next ;
      }

      const Node *last() const
      {
         return &_zero ;
      }

      void swap(PDList &lst)
      {
         if (&lst != this)
         {
            Node *f = first() ;
            lst.first()->prepend(&_zero) ;
            f == &_zero ? lst._zero.remove() : f->prepend(&lst._zero) ;
         }
      }

      // Returns the count of erased nodes.
      size_t erase(Node *start, Node *finish)
      {
         return Node::desintegrate(start, finish, _ndtr) ;
      }

      size_t erase(Node *start, size_t n)
      {
         return Node::desintegrate(start, n, _ndtr) ;
      }

      Node *insert(Node *position, Node *element)
      {
         return position->prepend(element) ;
      }
} ;


/*******************************************************************************
                     class incdlist_node
 Node header for an element of the doubly-linked list.
*******************************************************************************/
class incdlist_node : public PDList::Node {
   public:
      bool is_only() const
      {
         return next == prev && next != (const PDList::Node *)this ;
      }
} ;

/******************************************************************************/
/** Doubly-linked (bidirectional) inclusive list.
 In contrast with std::list, list data elements include bookkeeping list node
 data, not vice versa.
*******************************************************************************/
template<class T, incdlist_node T::*N>
class incdlist : private PDList {
      typedef PDList ancestor ;
   public:
      typedef incdlist<T, N>        self_type ;
      typedef T                     value_type ;
      typedef T &                   reference ;
      typedef const T &             const_reference ;
      typedef ptrdiff_t             difference_type ;

      class                         iterator ;
      class                         const_iterator ;
      friend class                  iterator ;
      friend class                  const_iterator ;


      /// An iterator by an inclusive list can be constructed from any list
      /// item (even in absense of the list object itself).
      class iterator :
         public std::iterator<std::bidirectional_iterator_tag, T, difference_type> {

         friend class incdlist<T, N> ;
         friend class const_iterator ;

         protected:
            Node *node ;

            iterator (Node * x) :
               node(x)
            {}


         public:
            iterator() :
               node(NULL)
            {}

            /// Create an iterator to an item from the item itself; introduces implicit
            /// conversion
            iterator(reference r) :
               node(incdlist<T, N>::node(&r))
            {}

            bool operator== (const iterator& x) const { return node == x.node; }
            bool operator!= (const iterator& x) const { return node != x.node; }

            reference operator*() const { return *incdlist<T, N>::object(node) ; }
            value_type *operator->() const { return incdlist<T, N>::object(node) ; }

            iterator& operator++()
            {
               node = node->next ;
               return *this ;
            }

            iterator& operator--()
            {
               node = node->prev ;
               return *this ;
            }

            PCOMN_DEFINE_POSTCREMENT_METHODS(iterator) ;
      } ;

      class const_iterator :
         public std::iterator<std::bidirectional_iterator_tag, T, difference_type> {

         friend class incdlist<T, N> ;

         protected:
            const Node *node ;

            const_iterator (const Node *x) :
               node(x)
            {}

         public:
            const_iterator() :
               node(NULL)
            {}

            const_iterator(const iterator& x) :
               node(x.node)
            {}

            /// Create an iterator to an item from the item itself; introduces implicit
            /// conversion
            const_iterator (const_reference r) :
               node(incdlist<T, N>::node(&r))
            {}

            bool operator==(const const_iterator& x) const { return node == x.node ; }
            bool operator!= (const const_iterator& x) const { return node != x.node ; }

            const_reference operator*() const { return *incdlist<T, N>::object(node) ; }
            const value_type *operator->() const { return incdlist<T, N>::object(node) ; }

            const_iterator& operator++()
            {
               node = node->next ;
               return *this ;
            }

            const_iterator& operator--()
            {
               node = node->prev ;
               return *this ;
            }

            PCOMN_DEFINE_POSTCREMENT_METHODS(const_iterator) ;
      } ;

      typedef std::reverse_iterator<const_iterator> const_reverse_iterator ;
      typedef std::reverse_iterator<iterator>       reverse_iterator ;

      using ancestor::size ;
      using ancestor::empty ;

      explicit incdlist(bool owns = false) :
         ancestor(owns ? node_destructor : NULL)
      {}

      // push_pack() -  insert <elem> at the back of the list.
      // If element is a member of some other incdlist, it is removed from that one.
      //
      void push_back(value_type &elem) { last()->prepend(node(&elem)) ; }

      // push_front() -  insert <elem> at the front of the list.
      // If element is a member of some other incdlist, it is removed from that one.
      //
      void push_front(value_type &elem) { first()->prepend(node(&elem)) ; }

      void pop_front() { erase(begin()) ; }

      void pop_back() { erase(last_element()) ; }

      size_t erase(const iterator &start, const iterator &finish)
      {
         return ancestor::erase(start.node, finish.node) ;
      }

      size_t erase(const iterator &start, size_t n = 1)
      {
         return ancestor::erase(start.node, n) ;
      }

      iterator insert(const iterator &where, const value_type &element)
      {
         return node(&*where)->prepend(node(&element)) ;
      }

      size_t flush() { return erase(begin(), end()) ; }

      static iterator insert_after(const value_type &where, const value_type &element)
      {
         return node(&where)->append(node(&element)) ;
      }
      static void remove(value_type &elem) { node(&elem)->remove() ; }

      // swap()   -  swap the contents of two lists.
      //
      incdlist &swap(incdlist &another)
      {
         ancestor::swap(another) ;
         return *this ;
      }

      iterator begin() { return first() ; }
      iterator end() { return last() ; }

      const_iterator begin() const { return first() ; }
      const_iterator end() const { return last() ; }

      reference front() { return *begin() ; }
      const_reference front() const { return *begin() ; }

      reference back() { return *last_element() ; }
      const_reference back() const { return *last_element() ; }

      bool owns() const { return !!_ndtr ; }
      bool owns(bool nv)
      {
         bool oldv = owns() ;
         _ndtr = nv ? node_destructor : NULL ;
         return oldv ;
      }

   private:
      /// Get the pointer to the list node for the given data object.
      static Node *node(const value_type *value)
      {
         return const_cast<Node *>(static_cast<const Node *>(&(value->*N))) ;
      }

      /// Get the pointer to the data object for given node.
      static value_type *object(const Node *node)
      {
         // A hack that calculates "back offset" from a member to its
         // enclosing object.
         // To avoid GCC complaints, we don't use NULL as a "straw pointer".
         // I suppose, 256 holds for _any_ alignment requirements.
         NOXCHECK(node) ;
         return
            reinterpret_cast<value_type *>
            (reinterpret_cast<char *>(const_cast<Node *>(node)) -
             (reinterpret_cast<char *>(&(reinterpret_cast<value_type *>(256)->*N)) - 256)) ;
      }

      static void node_destructor(Node *n) { delete object(n) ; }

      iterator last_element() { return last()->prev ; }
      const_iterator last_element() const { return last()->prev ; }
} ;


/*******************************************************************************
          template<class T, incdlist_node T::*N>
          class incdlist_managed

 Derived from the incdlist and holds all it properties except owns its
 hodes (elements) by default. I.e. nodes are deleted during list destruction
 (if opposite is not set).
*******************************************************************************/
template<class T, incdlist_node T::*N>
class incdlist_managed : public incdlist<T, N> {
   public:
      typedef incdlist<T, N> ancestor ;
      typedef typename ancestor::iterator               iterator ;
      typedef typename ancestor::const_iterator         const_iterator ;
      typedef typename ancestor::reverse_iterator       reverse_iterator ;
      typedef typename ancestor::const_reverse_iterator const_reverse_iterator ;

      incdlist_managed() :
         ancestor(true)
      {}
} ;

} // end of namespace pcomn

#endif /* __PCOMN_INCDLIST_H */
