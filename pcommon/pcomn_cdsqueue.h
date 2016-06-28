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

template<typename> struct dynq_node ;
template<typename> struct dualq_node ;

/******************************************************************************/
/** Node of list-based lockless queue.
*******************************************************************************/
template<typename T>
struct dynq_node : cdsnode_nextptr<dynq_node<T>> {
      template<typename... Args>
      dynq_node(Args &&...args) : _value(std::forward<Args>(args)...) {}

      T _value ;
} ;

/******************************************************************************/
/** Node of list-based "dual" queue or stack, i.e. the lockless queue/stack with
 lock-based wait for a thread that attempts to pop from an empty queue/stack.
*******************************************************************************/
template<typename T>
struct dualq_node : cdsnode_nextptr<dualq_node<T>> {
   public:
      promise_lock _reqlock ;    /* Request lock: for normal node, initially unlocked */
      T *          _value = {} ; /* &_valstor: normal node;
                                  * nullptr:   unfulfilled request;
                                  * other:     fulfilled request */

      std::aligned_storage_t<sizeof(T), alignof(T)> _valstor ;

   public:
      /// Create unfulllfilled request.
      constexpr dualq_node() : _reqlock(true) {}

      /// Create normal node with initialized value.
      template<typename... Args>
      dualq_node(std::piecewise_construct_t, Args &&...args) : _reqlock(false)
      {
         _value = new (value_storage()) T(std::forward<Args>(args)...) ;
      }

      ~dualq_node()
      {
         if (xchange(_value, nullptr) == value_storage())
            destroy(value_storage()) ;
      }

      T *value_storage() { return static_cast<T *>(static_cast<void *>(&_valstor)) ; }

      bool is_request_node() const { return _value != value_storage() ; }

      /// Get node address by the value address.
      static dualq_node *node(T *value)
      {
         return static_cast<dualq_node *>(padd(value, -offsetof(dualq_node, _valstor))) ;
      }

      bool fulfill_request(dualq_node *realizer, std::memory_order order)
      {
         NOXCHECK(is_request_node()) ;
         NOXCHECK(realizer) ;
         NOXCHECK(!realizer->is_request_node()) ;

         return atomic_op::cas(&_value, nullptr, realizer->_value, order) ;
      }
} ;
}

/******************************************************************************/
/** Base class for lock-free Michael&Scott FIFO queue implementation.
*******************************************************************************/
template<typename T, typename N, typename Alloc, typename Q>
class cdsqueue_base : public concurrent_container<T, N, Alloc> {
      typedef concurrent_container<T, N, Alloc> ancestor ;
   public:
      using typename ancestor::value_type ;
      using typename ancestor::node_type ;
      using typename ancestor::node_allocator_traits ;

      bool empty() const
      {
         return
            atomic_op::load(&_head, std::memory_order_relaxed) ==
            atomic_op::load(&_tail, std::memory_order_relaxed) ;
      }

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
         self().push_node(self().make_node(std::forward<Args>(args)...)) ;
      }

      /// Alias for push() to allow for std::back_insert_iterator
      void push_back(const value_type &value) { push(value) ; }
      /// @overload
      void push_back(value_type &&value) { push(std::move(value)) ; }

   protected:
      typename node_type::nextptr_type _dummy_node = {nullptr} ;

      node_type *_head = static_cast<node_type *>(&_dummy_node) ;
      node_type *_tail = static_cast<node_type *>(&_dummy_node) ;

   protected:
      cdsqueue_base() = default ;

      /// The destructor pops and @em immediately destroys and deallocates all nodes
      /// remaining in the queue.
      ///
      /// No hazard pointers and safe memory reclamation here: it is assumed a queue is
      /// destructed in exclusive mode, it is illegal to concurrently access the queue
      /// being destroyed.
      ~cdsqueue_base() ;

      void retire_node(node_type *node)
      {
         if (node != &_dummy_node)
            ancestor::retire_node(node) ;
      }

   private:
      Q &self() { return *static_cast<Q *>(this) ; }
} ;

/******************************************************************************/
/** Lock-free dynamic-memory list-based FIFO queue.

 @note Implemented as Michael&Scott queue. While Ladan-Mozes/Shavit queue
 is better due single CAS on both ends (M&S requires single CAS at pop, 2 CASes
 at push), it needs a @em new (unique) dummy node every time the queue gots empty,
 which @em is the case when average push attempts rate is lower than pop attempts rate.
*******************************************************************************/
template<typename T, typename A = std::allocator<T>>
class concurrent_dynqueue : cdsqueue_base<T, detail::dynq_node<T>, A, concurrent_dynqueue<T, A>> {

      typedef cdsqueue_base<T, detail::dynq_node<T>, A, concurrent_dynqueue<T, A>> ancestor ;
      friend ancestor ;

      using typename ancestor::node_type ;
      using typename ancestor::node_hazard_ptr ;
      using typename ancestor::node_allocator_type ;
      using typename ancestor::node_allocator_traits ;

   public:
      using typename ancestor::value_type ;

      concurrent_dynqueue() = default ;

      /// Pop and @em immediately destroy and deallocate all remaining nodes.
      ///
      /// No hazard pointers and safe memory reclamation here: it is assumed a queue is
      /// destructed in exclusive mode, it is illegal to concurrently access the queue
      /// being destroyed.
      ~concurrent_dynqueue() = default ;

      using ancestor::push ;
      using ancestor::push_back ;
      using ancestor::emplace ;
      using ancestor::empty ;

      bool pop(value_type &result) ;

      /// If the queue is nonempty, pop and return its head; otherwise, return a default
      /// value constructed from passsed arguments.
      ///
      /// @return A pair where the first item is the value (popped or default), the
      /// second item is true if the actual value returned, false if the default.
      template<typename... Args>
      std::pair<value_type, bool> pop_default(Args &&...defargs) ;

   private:
      void push_node(node_type *new_node) noexcept ;
      node_hazard_ptr pop_node() ;

      using ancestor::make_node ;
      using ancestor::retire_node ;
} ;

/******************************************************************************/
/** Lock-free dynamic-memory FIFO queue.

 @note Implemented as Michael&Scott queue. While Ladan-Mozes/Shavit queue
 is better due single CAS on both ends (M&S requires single CAS at pop, 2 CASes
 at push), it needs a @em new (unique) dummy node every time the queue gots empty,
 which @em is the case when average push attempts rate is lower than pop attempts rate.
*******************************************************************************/
template<typename T, typename A = std::allocator<T>>
class concurrent_dualqueue : cdsqueue_base<T, detail::dualq_node<T>, A, concurrent_dualqueue<T, A>> {

      typedef cdsqueue_base<T, detail::dualq_node<T>, A, concurrent_dualqueue<T, A>> ancestor ;
      friend ancestor ;

      using typename ancestor::node_type ;
      using typename ancestor::node_hazard_ptr ;
      using typename ancestor::node_allocator_type ;
      using typename ancestor::node_allocator_traits ;

   public:
      using typename ancestor::value_type ;

      concurrent_dualqueue() = default ;

      /// Pop and @em immediately destroy and deallocate all remaining nodes.
      ///
      /// No hazard pointers and safe memory reclamation here: it is assumed a queue is
      /// destructed in exclusive mode, it is illegal to concurrently access the queue
      /// being destroyed.
      ~concurrent_dualqueue() = default ;

      value_type pop() ;

      bool try_pop(value_type &result) ;

      /// If the queue is nonempty, pop and return its head; otherwise, return a default
      /// value constructed from passsed arguments.
      ///
      /// @return A pair where the first item is the value (popped or default), the
      /// second item is true if the actual value returned, false if the default.
      template<typename... Args>
      std::pair<value_type, bool> pop_default(Args &&...defargs) ;

   private:
      void push_node(node_type *new_node) noexcept ;
      node_hazard_ptr pop_node() ;

      using ancestor::make_node ;
      using ancestor::retire_node ;
} ;

/*******************************************************************************
 cdsqueue_base
*******************************************************************************/
template<typename T, typename N, typename A, typename Q>
cdsqueue_base<T, N, A, Q>::~cdsqueue_base()
{
   if (_tail == &_dummy_node)
   {
      NOXCHECK(empty()) ;
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

/*******************************************************************************
 concurrent_dynqueue
*******************************************************************************/
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
      const node_hazard_ptr tail (&this->_tail) ;
      old_tail = tail.get() ;

      // When a tail is still the tail and the queue is not in the middle of enqueuing of
      // a node by someone else, tail->_next shall be NULL.
      if (const node_hazard_ptr tail_next {tail->_next})
      {
         // Tail is not pointing to the last node, help someone else who is now
         // in the middle of enqueuing to swing the tail.
         atomic_op::cas(&this->_tail, old_tail, tail_next.get(), std::memory_order_release) ;
         continue ;
      }

      // Attempt to link the new node at the end of the list.
      if (atomic_op::cas(&tail->_next, (node_type *)nullptr, new_node, std::memory_order_release))
         break ;
   }
   // Enqueue is done (visible), try to swing the tail to the appended node.
   // We can safely ignore the result of this CAS: if successful - we've swung the tail,
   // if not - some other thread has already helped us.
   atomic_op::cas(&this->_tail, old_tail, new_node, std::memory_order_relaxed) ;
}

template<typename T, typename A>
inline auto concurrent_dynqueue<T, A>::pop_node() -> node_hazard_ptr
{
   // Keep trying until successful dequeue
   for(;;)
   {
      node_hazard_ptr head (&this->_head) ;
      node_hazard_ptr next (head->_next) ;

      node_type *head_ptr = head.get() ;

      if (head_ptr == this->_tail)
      {
         if (!next)
            // The queue is empty
            return nullptr ;

         // The queue is in the middle of enqueuing of the first node, tail is falling
         // behind, help to advance it.
         atomic_op::cas(&this->_tail, head_ptr, next.get(), std::memory_order_release) ;
         continue ;
      }

      if (atomic_op::cas(&this->_head, head_ptr, next.get(), std::memory_order_release))
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

} // end of namespace pcomn

#endif /* __PCOMN_CDSQUEUE_H */
