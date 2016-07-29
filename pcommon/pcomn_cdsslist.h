/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_CDSSLIST_H
#define __PCOMN_CDSSLIST_H
/*******************************************************************************
 FILE         :   pcomn_cdsslist.h
 COPYRIGHT    :   Yakov Markovitch, 2016

 DESCRIPTION  :   Concurrent lock-free singly-linked list

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   14 Jun 2016
*******************************************************************************/
#include <pcomn_cdsbase.h>

namespace pcomn {

/******************************************************************************/
/** Node of lockless singly-linked list.
*******************************************************************************/
template<typename T>
struct cdsslist_node : cdsnode_nextptr<cdsslist_node<T>> {
      cdsslist_node() = default ;

      template<typename... Args>
      cdsslist_node(Args &&...args) : _value(std::forward<Args>(args)...) {}

      T &value() { return _value ; }
      const T &value() const { return _value ; }

      /// Unique list node ID.
      /// Node IDs are unique per program run per node type.
      int64_t id() const { return _id ; }

      struct reference {
            constexpr reference() = default ;
            reference(const cdsslist_node &n) : _id(n.id()) {}

            explicit operator bool() const { return !!_id ; }

            friend bool operator==(const reference &x, const reference &y)
            {
               return x._id == y._id ;
            }

            friend bool operator!=(const reference &x, const reference &y)
            {
               return !(x == y) ;
            }

         private:
            int64_t _id = 0 ;
      } ;

   private:
      const int64_t  _id    = new_id() ;
      T              _value = {} ;

      static std::atomic<uint32_t> _id_range = {0} ;
      static thread_local int64_t  _next_id  = {0} ;

      static constexpr const bits_per_range = 16 ;

      static int64_t new_id()
      {
         if (!(_next_id & (1UL << bits_per_range)))
            _next_id = ((int64_t)_id_range.fetch_add(1, std::memory_order_relaxed) << bits_per_range) + 1 ;
         return _next_id++ ;
      }

      PCOMN_NONCOPYABLE(cdsslist_node) ;
      PCOMN_NONASSIGNABLE(cdsslist_node) ;
} ;

/******************************************************************************/
/** Lock-free singly-linked list.
 Implemented as Harris&Michael singly-linked list with hazard pointers.
*******************************************************************************/
template<typename T, typename Alloc = std::allocator<T>>
class concurrent_slist : public concurrent_container<T, cdsslist_node<T>, Alloc> {
      typedef concurrent_container<T, cdsslist_node<T>, Alloc> ancestor ;
   public:
      using typename ancestor::value_type ;
      using typename ancestor::node_type ;
      using typename ancestor::node_allocator_traits ;
      using typename ancestor::node_hazard_ptr ;

      typedef typename node_type::reference node_reference ;

      concurrent_slist() = default ;

      /// The destructor removes and @em immediately destroys and deallocates all
      /// the remaining list nodes.
      ///
      /// No hazard pointers and safe memory reclamation here: lists are assumed
      /// to be destructed in exclusive mode, it is illegal to concurrently access
      /// the list being destroyed.
      ~concurrent_slist() ;

      bool empty() const ;

      void push_back(const value_type &value) ;
      void push_back(value_type &&value) ;

      /// Alias for push() to allow for std::back_insert_iterator
      void push_back(const value_type &value) { push(value) ; }
      /// @overload
      void push_back(value_type &&value) { push(std::move(value)) ; }

      template<typename Compare>
      node_reference insert(const value_type &value, Compare comp) ;
      template<typename Compare>
      node_reference insert(value_type &&value, Compare comp) ;

      bool erase(node_reference) ;

      bool pop(node_reference, value_type &store) ;

      std::pair<value_type, bool> pop(node_reference) ;

      template<struct UnaryPredicate>
      size_t remove_if(UnaryPredicate pred) ;

      template<struct UnaryPredicate>
      std::pair<value_type, node_reference> find_if(UnaryPredicate pred) ;

   private:
      typename node_type::nextptr_type _dummy_node[2] ;

      node_type *_head = static_cast<node_type *>(_dummy_node + 0) ;
      node_type *_tail = static_cast<node_type *>(_dummy_node + 1) ;

   private:
      void retire_node(node_type *node)
      {
         if (node != _dummy_node + 0 && node != _dummy_node + 1)
            ancestor::retire_node(node) ;
      }

      bool enqueue_node(node_type *old_tail, node_type *new_node)
      {
         // Attempt to link the new node at the end of the list.
         if (!old_tail || !atomic_op::cas(&old_tail->_next, (node_type *)nullptr, new_node, std::memory_order_release))
            return false ;

         // Enqueue is done (visible), try to swing the tail to the appended node.
         // We can safely ignore the result of this CAS: if successful - we've swung the tail,
         // if not - some other thread has already helped us.
         atomic_op::cas(&this->_tail, old_tail, new_node, std::memory_order_relaxed) ;
         return true ;
      }

      bool atomic_pop_head(node_type *head, std::memory_order order = std::memory_order_release)
      {
         return atomic_op::cas(&this->_head, head, head->_next, std::memory_order_release) ;
      }

      bool retire_head(node_hazard_ptr &head, std::memory_order order = std::memory_order_release)
      {
         node_type * const current_head = head.get() ;

         if (atomic_pop_head(current_head, order))
         {
            head.reset() ;
            retire_node(current_head) ;
            return true ;
         }
         return false ;
      }
} ;

/*******************************************************************************
 cdsslist_node<T>
*******************************************************************************/
template<typename T>
std::atomic<uint32_t> cdsslist_node<T>::_id_range ;
template<typename T>
thread_local int64_t cdsslist_node<T>::_next_id ;


/*******************************************************************************
 concurrent_slist<T,N,Alloc,Q>
*******************************************************************************/
template<typename T, typename A>
template<typename V, typename C>
auto concurrent_slist<T,A>::insert(V &&value, C &&compare) -> node_reference
{
   const value_type *vp = &value ;
   node_hazard_ptr new_node_hazard ;
   node_type *new_node = nullptr ;
   unipair<node_hazard_ptr> between ;

   do {
      between = search(*vp, std::forward<C>(compare)) ;
      if (!between.second)
      {
         new_node_hazard.reset() ;
         this->delete_node(new_node) ;
         // Failure
         return {} ;
      }

      if (!new_node_hazard)
      {
         new_node_hazard = node_hazard_ptr(new_node = this->make_node(std::forward<V>(value))) ;
         vp = new_node->value() ;
      }

      new_node->_next = between.second.get() ;
   }
   while (!atomic_op::cas(&between.first->_next, new_node->_next, new_node)) ;

   // Success
   return {*new_node} ;
}

template<typename T, typename A>
template<typename C>
auto concurrent_slist<T,A>::remove_single(C &&compare) -> node_hazard_ptr
{
   unipair<node_hazard_ptr> between ;
   node_hazard_ptr right_node_next ;

   do {
      between = search(*vp, std::forward<C>(compare)) ;
      if (!between.second)
         // Nothing found
         return {} ;

      right_node_next = node_hazard_ptr(between.second->_next) ;
      if (!is_marked_reference(right_node_next))
         if (CAS (&(right_node.next), /*C3*/
                  right_node_next, get_marked_reference (right_node_next)))
            break;
   } while (true); /*B4*/
   if (!CAS (&(left_node.next), right_node, right_node_next)) /*C4*/
      right_node = search (right_node.key, &left_node);
   return true;
}

template<typename T, typename A>
template<typename C>
auto concurrent_slist<T,A>::remove_multiple(C &&compare) -> std::pair<node_hazard_ptr, size_t>
{
   unipair<node_hazard_ptr> between ;
   node_hazard_ptr right_node_next ;

   do {
      between = search(*vp, std::forward<C>(compare)) ;
      if (!between.second)
         // No nodes to delete
         return {} ;

      right_node_next = right_node.next;
      if (!is_marked_reference(right_node_next))
         if (CAS (&(right_node.next), /*C3*/
                  right_node_next, get_marked_reference (right_node_next)))
            break;
   } while (true); /*B4*/
   if (!CAS (&(left_node.next), right_node, right_node_next)) /*C4*/
      right_node = search (right_node.key, &left_node);
   return true;
}

}

#endif /* __PCOMN_CDSSLIST_H */
