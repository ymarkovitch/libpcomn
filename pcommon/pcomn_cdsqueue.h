/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_CDSQUEUE_H
#define __PCOMN_CDSQUEUE_H
/*******************************************************************************
 FILE         :   pcomn_cdsqueue.h
 COPYRIGHT    :   Yakov Markovitch, 2016

 DESCRIPTION  :   Concurrent wait-free queue

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   14 Jun 2016
*******************************************************************************/
#include <pcomn_cdsbase.h>
#include <pcomn_atomic.h>
#include <pcomn_hazardptr.h>
#include <pcomn_meta.h>

namespace pcomn {
namespace detail {

template<typename T>
struct dynqueue_node {
      PCOMN_NONCOPYABLE(dynqueue_node) ;
      PCOMN_NONASSIGNABLE(dynqueue_node) ;

      template<typename... Args>
      dynqueue_node(Args &&...args) : _value(std::forward<Args>(args)...) {}

      dynqueue_node *_next ;
      T              _value ;
} ;
}

/******************************************************************************/
/** Lock-free dynamic-memory FIFO queue.

 @note Implemented as Michael&Scott queue. While Ladan-Mozes/Shavit queue
 is better due single CAS on both ends (M&S requires single CAS at pop, 2 CASes
 at push), it needs a @em new (unique) dummy node every time the queue gots empty,
 which @em is the case when average push attempts rate is lower than pop attempts rate.
*******************************************************************************/
template<typename T, typename Allocator = std::allocator<T>>
class concurrent_dynqueue :
         private concurrent_container<T, detail::dynqueue_node<T>, Allocator> {

      PCOMN_NONCOPYABLE(concurrent_dynqueue) ;
      PCOMN_NONASSIGNABLE(concurrent_dynqueue) ;

      typedef concurrent_container<T, detail::dynqueue_node<T>, Allocator> ancestor ;

      using typename ancestor::node_type ;
      using typename ancestor::node_hazard_ptr ;

      using typename ancestor::allocator_type ;
      using typename ancestor::node_allocator_type ;
      using typename ancestor::node_allocator_traits ;
      using typename ancestor::value_allocator_type ;
      using typename ancestor::node_dealloc ;

   public:
      typedef T value_type ;

      concurrent_dynqueue() = default ;

      void push(const value_type &value)
      {
         emplace(value) ;
      }
      void push(value_type &&value)
      {
         emplace(std::move(value)) ;
      }

      template<typename... Args>
      void emplace(Args &&...args)
      {
         push_node(make_node(std::forward<Args>(args)...)) ;
      }

      bool pop(value_type &result) ;
      template<typename... Args>
      std::pair<value_type, bool> pop_default(Args &&...defargs) ;

      bool empty() const { return _head == _tail ; }

   private:
      node_type *_head = nullptr ;
      node_type *_tail = nullptr ;

   private:
      // Allocate and construct a queue node
      template<typename... Args>
      node_type *make_node(Args &&... args)
      {
         node_allocator_type &allocator = this->node_allocator() ;
         std::unique_ptr<node_type, node_dealloc> p {this->allocate_node(), node_dealloc(allocator)} ;

         node_allocator_traits::construct(allocator, p.get(), std::forward<Args>(args)...) ;
         return p.release() ;
      }

      void destroy_popped_node(node_type *popped_node)
      {
         NOXCHECK(popped_node) ;
         node_allocator_traits::destroy(this->node_allocator(), popped_node) ;
         this->retire_node(popped_node) ;
      }

      void push_node(node_type *new_node) noexcept ;
      node_type *pop_node() ;
} ;

/*******************************************************************************
 concurrent_dynqueue
*******************************************************************************/
template<typename T, typename A>
bool concurrent_dynqueue<T, A>::pop(value_type &result)
{
   node_type *popped_head = pop_node() ;
   if (!popped_head)
      return false ;

   result = std::move(popped_head->_value) ;
   destroy_popped_node(popped_head) ;

   return true ;
}

template<typename T, typename A>
template<typename... Args>
auto concurrent_dynqueue<T, A>::pop_default(Args &&...defargs) -> std::pair<value_type, bool>
{
   node_type *popped_head = pop_node() ;
   if (!popped_head)
      return
      {std::piecewise_construct,
            std::forward_as_tuple(std::forward<Args>(defargs)...),
            std::forward_as_tuple(false)} ;

   std::pair<value_type, bool> result {std::move(popped_head->_value), true} ;
   destroy_popped_node(popped_head) ;

   return std::move(result) ;
}

template<typename T, typename A>
void concurrent_dynqueue<T, A>::push_node(node_type *new_node) noexcept
{
   NOXCHECK(new_node) ;
   NOXCHECK(!new_node->next) ;

   // Hold the pushed node safe until enqueue is finished
   const node_hazard_ptr node (new_node) ;
   node_type *old_tail ;

   // Keep trying until successful enqueue
   for(;;)
   {
      const node_hazard_ptr tail (&_tail) ;
      old_tail = tail ;

      // When a tail is still the tail and the queue is not in the middle of enqueuing of
      // a node by someone else, tail->_next shall be NULL.
      if (const node_hazard_ptr tail_next = tail->_next)
      {
         // Tail is not pointing to the last node, help someone else who is now
         // in the middle of enqueuing to swing the tail.
         atomic_op::cas(&_tail, old_tail, tail_next.get(), std::memory_order_release) ;
         continue ;
      }

      // Attempt to link the new node at the end of the list.
      if (atomic_op::cas(&tail->_next, (node_type *)nullptr, new_node, std::memory_order_release))
         break ;
   }
   // Enqueue is done (visible), try to swing the tail to the appended node.
   // We can safely ignore the result of this CAS: if successful - we've swung the tail,
   // if not - some other thread has already helped us.
   atomic_op::cas(&_tail, old_tail, new_node, std::memory_order_relaxed) ;
}

template<typename T, typename A>
inline auto concurrent_dynqueue<T, A>::pop_node() -> node_type *
{
   // Keep trying until successful dequeue
   for(;;)
   {
      const node_hazard_ptr head (&_head) ;
      const node_hazard_ptr next (head->_next) ;

      if (head == _tail)
      {
         if (!next)
            // The queue is empty
            return nullptr ;

         // The queue is in the middle of enqueuing of the first node, tail is falling
         // behind, help to advance it.
         atomic_op::cas(&_tail, head.get(), next.get(), std::memory_order_release) ;
         continue ;
      }

      if (atomic_op::cas(&_head, head.get(), next.get(), std::memory_order_release))
         return head.get() ;
   }
}

} // end of namespace pcomn

#endif /* __PCOMN_CDSQUEUE_H */
