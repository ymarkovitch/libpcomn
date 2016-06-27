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
#include <pcomn_syncobj.h>

namespace pcomn {
namespace detail {

template<typename T>
struct dynqueue_node ;

template<typename T>
struct dynqueue_node_nextptr {
      PCOMN_NONCOPYABLE(dynqueue_node_nextptr) ;
      PCOMN_NONASSIGNABLE(dynqueue_node_nextptr) ;

      constexpr dynqueue_node_nextptr(dynqueue_node<T> *next = nullptr) : _next(next) {}

      dynqueue_node<T> *_next ;
} ;

template<typename T>
struct dynqueue_node : dynqueue_node_nextptr<T> {
      template<typename... Args>
      dynqueue_node(Args &&...args) : _value(std::forward<Args>(args)...) {}

      T _value ;
} ;

template<typename T>
struct dualqueue_node : dynqueue_node_nextptr<T> {
      template<typename... Args>
      dualqueue_node(Args &&...args) : _value(std::forward<Args>(args)...) {}

      T _value ;
} ;
}

/******************************************************************************/
/** Lock-free dynamic-memory list-based FIFO queue.

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

      /// The destructor pops and @em immediately destroys and deallocates all nodes
      /// remaining in the queue.
      ///
      /// No hazard pointers and safe memory reclamation here: it is assumed a queue is
      /// destructed in exclusive mode, it is illegal to concurrently access the queue
      /// being destroyed.
      ~concurrent_dynqueue() ;

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

      /// Alias for push() to allow for std::back_insert_iterator
      void push_back(const value_type &value) { push(value) ; }
      /// @overload
      void push_back(value_type &&value) { push(std::move(value)) ; }

      bool pop(value_type &result) ;

      template<typename... Args>
      std::pair<value_type, bool> pop_default(Args &&...defargs) ;

      bool empty() const { return _head == _tail ; }

   private:
      detail::dynqueue_node_nextptr<T> _dummy_node = {nullptr} ;

      node_type *_head = static_cast<node_type *>(&_dummy_node) ;
      node_type *_tail = static_cast<node_type *>(&_dummy_node) ;

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

      void retire_node(node_type *node)
      {
         if (node != &_dummy_node)
            ancestor::retire_node(node) ;
      }

      void push_node(node_type *new_node) noexcept ;
      node_hazard_ptr pop_node() ;
} ;

/******************************************************************************/
/** Lock-free dynamic-memory FIFO queue.

 @note Implemented as Michael&Scott queue. While Ladan-Mozes/Shavit queue
 is better due single CAS on both ends (M&S requires single CAS at pop, 2 CASes
 at push), it needs a @em new (unique) dummy node every time the queue gots empty,
 which @em is the case when average push attempts rate is lower than pop attempts rate.
*******************************************************************************/
template<typename T, typename Allocator = std::allocator<T>>
class concurrent_dualqueue :
         private concurrent_container<T, detail::dynqueue_node<T>, Allocator> {

      PCOMN_NONCOPYABLE(concurrent_dualqueue) ;
      PCOMN_NONASSIGNABLE(concurrent_dualqueue) ;

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

      concurrent_dualqueue() = default ;

      /// The destructor pops and @em immediately destroys and deallocates all nodes
      /// remaining in the queue.
      ///
      /// No hazard pointers and safe memory reclamation here: it is assumed a queue is
      /// destructed in exclusive mode, it is illegal to concurrently access the queue
      /// being destroyed.
      ~concurrent_dualqueue() ;

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

      /// Alias for push() to allow for std::back_insert_iterator
      void push_back(const value_type &value) { push(value) ; }
      /// @overload
      void push_back(value_type &&value) { push(std::move(value)) ; }

      bool pop(value_type &result) ;

      template<typename... Args>
      std::pair<value_type, bool> pop_default(Args &&...defargs) ;

      bool empty() const { return _head == _tail ; }

   private:
      detail::dynqueue_node_nextptr<T> _dummy_node = {nullptr} ;

      node_type *_head = static_cast<node_type *>(&_dummy_node) ;
      node_type *_tail = static_cast<node_type *>(&_dummy_node) ;

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

      void retire_node(node_type *node)
      {
         if (node != &_dummy_node)
            ancestor::retire_node(node) ;
      }

      void push_node(node_type *new_node) noexcept ;
      node_hazard_ptr pop_node() ;
} ;

/*******************************************************************************
 concurrent_dynqueue
*******************************************************************************/
template<typename T, typename A>
concurrent_dynqueue<T, A>::~concurrent_dynqueue()
{
   if (_tail == &_dummy_node)
   {
      NOXCHECK(_tail == _head) ;
      return ;
   }
   node_type *node = _head->_next ;
   auto &allocator = this->node_allocator() ;
   if (_head != &_dummy_node)
      node_allocator_traits::deallocate(allocator, _head, 1) ;
   else
      NOXCHECK(node) ;

   for ( ; node ; node = _head)
   {
      _head = node->_next ;
      node_allocator_traits::destroy(allocator, node) ;
      node_allocator_traits::deallocate(allocator, node, 1) ;
   }
}

template<typename T, typename A>
bool concurrent_dynqueue<T, A>::pop(value_type &result)
{
   node_hazard_ptr popped_head = pop_node() ;
   if (!popped_head)
      return false ;

   result = std::move(popped_head->_value) ;
   // Call the destructor but do _not_ retire/deallocate:
   // this node (maybe) becomes a dummy node.
   node_allocator_traits::destroy(this->node_allocator(), popped_head.get()) ;

   return true ;
}

template<typename T, typename A>
template<typename... Args>
auto concurrent_dynqueue<T, A>::pop_default(Args &&...defargs) -> std::pair<value_type, bool>
{
   node_hazard_ptr popped_head = pop_node() ;
   if (!popped_head)
      return
      {std::piecewise_construct,
            std::forward_as_tuple(std::forward<Args>(defargs)...),
            std::forward_as_tuple(false)} ;

   std::pair<value_type, bool> result {std::move(popped_head->_value), true} ;
   node_allocator_traits::destroy(this->node_allocator(), popped_head.get()) ;

   return std::move(result) ;
}

template<typename T, typename A>
void concurrent_dynqueue<T, A>::push_node(node_type *new_node) noexcept
{
   NOXCHECK(new_node) ;
   NOXCHECK(!new_node->_next) ;

   // Hold the pushed node safe until enqueue is finished
   const node_hazard_ptr node (new_node) ;
   node_type *old_tail ;

   // Keep trying until successful enqueue
   for(;;)
   {
      const node_hazard_ptr tail (&_tail) ;
      old_tail = tail.get() ;

      // When a tail is still the tail and the queue is not in the middle of enqueuing of
      // a node by someone else, tail->_next shall be NULL.
      if (const node_hazard_ptr tail_next {tail->_next})
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
inline auto concurrent_dynqueue<T, A>::pop_node() -> node_hazard_ptr
{
   // Keep trying until successful dequeue
   for(;;)
   {
      node_hazard_ptr head (&_head) ;
      node_hazard_ptr next (head->_next) ;

      node_type *head_ptr = head.get() ;

      if (head_ptr == _tail)
      {
         if (!next)
            // The queue is empty
            return nullptr ;

         // The queue is in the middle of enqueuing of the first node, tail is falling
         // behind, help to advance it.
         atomic_op::cas(&_tail, head_ptr, next.get(), std::memory_order_release) ;
         continue ;
      }

      if (atomic_op::cas(&_head, head_ptr, next.get(), std::memory_order_release))
      {
         head.reset() ;
         retire_node(head_ptr) ;
         return std::move(next) ;
      }
   }
}

/*******************************************************************************
 concurrent_dualqueue
*******************************************************************************/
template<typename T, typename A>
concurrent_dualqueue<T, A>::~concurrent_dualqueue()
{
   if (_tail == &_dummy_node)
   {
      NOXCHECK(_tail == _head) ;
      return ;
   }
   node_type *node = _head->_next ;
   auto &allocator = this->node_allocator() ;
   if (_head != &_dummy_node)
      node_allocator_traits::deallocate(allocator, _head, 1) ;
   else
      NOXCHECK(node) ;

   for ( ; node ; node = _head)
   {
      _head = node->_next ;
      node_allocator_traits::destroy(allocator, node) ;
      node_allocator_traits::deallocate(allocator, node, 1) ;
   }
}

template<typename T, typename A>
bool concurrent_dualqueue<T, A>::pop(value_type &result)
{
   node_hazard_ptr popped_head = pop_node() ;
   if (!popped_head)
      return false ;

   result = std::move(popped_head->_value) ;
   // Call the destructor but do _not_ retire/deallocate:
   // this node (maybe) becomes a dummy node.
   node_allocator_traits::destroy(this->node_allocator(), popped_head.get()) ;

   return true ;
}

template<typename T, typename A>
template<typename... Args>
auto concurrent_dualqueue<T, A>::pop_default(Args &&...defargs) -> std::pair<value_type, bool>
{
   node_hazard_ptr popped_head = pop_node() ;
   if (!popped_head)
      return
      {std::piecewise_construct,
            std::forward_as_tuple(std::forward<Args>(defargs)...),
            std::forward_as_tuple(false)} ;

   std::pair<value_type, bool> result {std::move(popped_head->_value), true} ;
   node_allocator_traits::destroy(this->node_allocator(), popped_head.get()) ;

   return std::move(result) ;
}

template<typename T, typename A>
void concurrent_dualqueue<T, A>::push_node(node_type *new_node) noexcept
{
   NOXCHECK(new_node) ;
   NOXCHECK(!new_node->_next) ;

   // Hold the pushed node safe until enqueue is finished
   const node_hazard_ptr node (new_node) ;
   node_type *old_tail ;

   // Keep trying until successful enqueue
   for(;;)
   {
      const node_hazard_ptr tail (&_tail) ;
      old_tail = tail.get() ;

      // When a tail is still the tail and the queue is not in the middle of enqueuing of
      // a node by someone else, tail->_next shall be NULL.
      if (const node_hazard_ptr tail_next {tail->_next})
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
inline auto concurrent_dualqueue<T, A>::pop_node() -> node_hazard_ptr
{
   // Keep trying until successful dequeue
   for(;;)
   {
      node_hazard_ptr head (&_head) ;
      node_hazard_ptr next (head->_next) ;

      node_type *head_ptr = head.get() ;

      if (head_ptr == _tail)
      {
         if (!next)
            // The queue is empty
            return nullptr ;

         // The queue is in the middle of enqueuing of the first node, tail is falling
         // behind, help to advance it.
         atomic_op::cas(&_tail, head_ptr, next.get(), std::memory_order_release) ;
         continue ;
      }

      if (atomic_op::cas(&_head, head_ptr, next.get(), std::memory_order_release))
      {
         head.reset() ;
         retire_node(head_ptr) ;
         return std::move(next) ;
      }
   }
}

} // end of namespace pcomn

#endif /* __PCOMN_CDSQUEUE_H */
