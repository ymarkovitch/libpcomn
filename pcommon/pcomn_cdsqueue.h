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

      T &value() { return _value ; }
} ;

/******************************************************************************/
/** Node of list-based "dual" queue or stack, i.e. the lockless queue/stack with
 lock-based wait for a thread that attempts to pop from an empty queue/stack.
*******************************************************************************/
template<typename T>
struct dualq_node : cdsnode_nextptr<dualq_node<T>> {
   public:
      promise_lock _reqlock ;     /* Request lock: for normal node, initially unlocked */
      T *          _valptr = {} ; /* &_valstor: normal node;
                                   * nullptr:   unfulfilled request;
                                   * other:     fulfilled request */

      std::aligned_storage_t<sizeof(T), alignof(T)> _valstor ;

   public:
      /// Create unfulfilled request.
      constexpr dualq_node() : _reqlock(true) {}

      /// Create normal node with initialized value.
      template<typename... Args>
      dualq_node(std::piecewise_construct_t, Args &&...args) : _reqlock(false)
      {
         _valptr = new (value_storage()) T(std::forward<Args>(args)...) ;
      }

      ~dualq_node()
      {
         if (_valptr == value_storage())
         {
            _valptr = tag_ptr(_valptr) ;
            destroy(value_storage()) ;
         }
      }

      T *value_storage() { return static_cast<T *>(static_cast<void *>(&_valstor)) ; }

      T &value() { return *_valptr ; }

      bool is_request_node() const
      {
         return static_cast<const void *>(untag_ptr(_valptr)) != &_valstor ;
      }

      /// Get node address by the value address.
      static dualq_node *node(T *value)
      {
         // Hack to avoid using offsetof macro
         static const std::aligned_storage_t<sizeof(dualq_node), alignof(dualq_node)> dummy {} ;
         static const ptrdiff_t valstor_offset = pdiff(&((dualq_node *)&dummy)->_valstor, &dummy) ;

         return static_cast<dualq_node *>(padd<void>(value, -valstor_offset)) ;
      }

      bool fulfill_request(dualq_node *realizer)
      {
         NOXCHECK(is_request_node()) ;
         NOXCHECK(realizer) ;
         NOXCHECK(!realizer->is_request_node()) ;

         return atomic_op::cas(&_valptr, (T *)nullptr, realizer->_valptr) ;
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
      using typename ancestor::node_hazard_ptr ;

      bool empty() const
      {
         return
            atomic_op::load(&_head, std::memory_order_relaxed) ==
            atomic_op::load(&_tail, std::memory_order_relaxed) ;
      }

      void push(const value_type &value)
      {
         self().emplace(value) ;
      }
      void push(value_type &&value)
      {
         self().emplace(std::move(value)) ;
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

      /// Get the tail; if tail is falling behind, i.e. the queue is in the middle of
      /// enqueuing of a node by someone else, help to wag the tail and return NULL.
      node_hazard_ptr ensure_consistent_tail()
      {
         for (;;)
         {
            node_hazard_ptr tail (&this->_tail) ;

            // When a tail is still the tail and the queue is not in the middle of enqueuing of
            // a node by someone else, tail->_next shall be NULL.
            if (const node_hazard_ptr tail_next {tail->_next})
               // Tail is not pointing to the last node, help someone else who is now
               // in the middle of enqueuing to swing the tail.
               atomic_op::cas(&this->_tail, tail.get(), tail_next.get(), std::memory_order_release) ;
            else
               return std::move(tail) ;
         }
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
      using ancestor::node_finalizer ;
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

      using ancestor::push ;
      using ancestor::push_back ;

      template<typename... Args>
      void emplace(Args &&...args)
      {
         ancestor::emplace(std::piecewise_construct, std::forward<Args>(args)...) ;
      }

      value_type pop() ;

      bool try_pop(value_type &result) ;

      /// If the queue is nonempty, pop and return its head; otherwise, return a default
      /// value constructed from passsed arguments.
      ///
      /// @return A pair where the first item is the value (popped or default), the
      /// second item is true if the actual value returned, false if the default.
      template<typename... Args>
      std::pair<value_type, bool> pop_default(Args &&...defargs) ;

      bool empty() const
      {
         return ancestor::empty() || this->_tail->is_request_node() ;
      }

   private:
      void push_node(node_type *new_node) noexcept ;
      node_hazard_ptr pop_node(bool lock_if_empty) ;

      using ancestor::make_node ;
      using ancestor::retire_node ;
      using ancestor::node_finalizer ;

      bool is_request_node(node_type *node) const
      {
         return node != this->_head && node->is_request_node() ;
      }

      void finalize_popped_head(node_type *popped_head)
      {
         node_type * const value_node = node_type::node(popped_head->_valptr) ;

         // Call the destructor but do _not_ retire/deallocate:
         // this node (maybe) becomes a dummy node.
         this->destroy_node(popped_head) ;

         if (value_node != popped_head)
            // value_node was a request fulfillment node
            this->delete_node(value_node) ;
      } ;
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
bool concurrent_dynqueue<T, A>::try_pop(value_type &result)
{
   const node_hazard_ptr popped_head = pop_node() ;
   const bool retval = !!popped_head ;
   if (retval)
      // While finalizing, call the destructor but do _not_ retire/deallocate:
      // this node (maybe) becomes a dummy node.
      result = std::move
         (node_finalizer(popped_head.get(), [this](node_type *n){ this->destroy_node(n) ; })->value()) ;
   return retval ;
}

template<typename T, typename A>
template<typename... Args>
auto concurrent_dynqueue<T, A>::pop_default(Args &&...defargs) -> std::pair<value_type, bool>
{
   const node_hazard_ptr popped_head = pop_node() ;
   if (!popped_head)
      // Return default value
      return {std::piecewise_construct,
            std::forward_as_tuple(std::forward<Args>(defargs)...),
            std::forward_as_tuple(false)} ;

   // Return head value and finalize popped node in one fell swoop
   return
      {std::move(node_finalizer(popped_head.get(), [this](node_type *n){ this->destroy_node(n) ; })->value()), true} ;
}

template<typename T, typename A>
void concurrent_dynqueue<T, A>::push_node(node_type *new_node) noexcept
{
   NOXCHECK(new_node) ;
   NOXCHECK(!new_node->_next) ;

   // Hold the pushed node safe until enqueue is finished
   const node_hazard_ptr node_guard (new_node) ;
   // Keep trying until successful enqueue
   while(!this->enqueue_node(this->ensure_consistent_tail().get(), new_node)) ;
}

template<typename T, typename A>
auto concurrent_dynqueue<T, A>::pop_node() -> node_hazard_ptr
{
   // Keep trying until successful dequeue
   for(;;)
   {
      node_hazard_ptr head {&this->_head} ;
      node_type *current_head = head.get() ;

      if (current_head == this->_tail)
      {
         if (!current_head->_next)
            // The queue is empty
            return nullptr ;
         this->ensure_consistent_tail() ;
      }

      node_hazard_ptr next (current_head->_next) ;
      if (this->retire_head(head))
         return std::move(next) ;
   }
}

/*******************************************************************************
 concurrent_dualqueue
*******************************************************************************/
template<typename T, typename A>
auto concurrent_dualqueue<T, A>::pop() -> value_type
{
   const node_hazard_ptr popped_head = pop_node(true) ;
   NOXCHECK(popped_head) ;
   return
      std::move(node_finalizer(popped_head.get(), [this](node_type *n) { finalize_popped_head(n) ; })->value()) ;
}

template<typename T, typename A>
bool concurrent_dualqueue<T, A>::try_pop(value_type &result)
{
   const node_hazard_ptr popped_head = pop_node(false) ;
   const bool retval = !!popped_head ;
   if (retval)
      result = std::move
         (node_finalizer(popped_head.get(), [this](node_type *n) { finalize_popped_head(n) ; })
          ->value()) ;
   return retval ;
}

template<typename T, typename A>
template<typename... Args>
auto concurrent_dualqueue<T, A>::pop_default(Args &&...defargs) -> std::pair<value_type, bool>
{
   const node_hazard_ptr popped_head = pop_node(false) ;
   if (!popped_head)
      return {std::piecewise_construct,
            std::forward_as_tuple(std::forward<Args>(defargs)...),
            std::forward_as_tuple(false)} ;

   // Return head value and finalize popped node in one fell swoop
   return {std::move(node_finalizer(popped_head.get(),
                                    [this](node_type *n) { finalize_popped_head(n) ; })->value()), true} ;
}

template<typename T, typename A>
void concurrent_dualqueue<T, A>::push_node(node_type *new_node) noexcept
{
   NOXCHECK(new_node) ;
   NOXCHECK(!new_node->_next) ;
   NOXCHECK(!new_node->is_request_node()) ;

   // Hold the pushed node safe until enqueue is finished
   const node_hazard_ptr node (new_node) ;

   // Keep trying until successful enqueue
   for(;;)
   {
      const node_hazard_ptr tail (this->ensure_consistent_tail()) ;
      node_type * const current_tail = tail.get() ;

      if (!is_request_node(current_tail))
         // The queue is either empty or consists of data nodes.
         if (this->enqueue_node(current_tail, new_node))
            break ;
         else
            continue ;

      // The queue consists of requests.
      // Try to fulfill a request at the head.
      node_hazard_ptr head (&this->_head) ;
      node_hazard_ptr front (head->_next) ;
      node_type *front_request ;

      if (atomic_op::load(&this->_head) != head || (front_request = front.get()) == nullptr || !is_request_node(front_request))
         continue ;

      const bool front_fulfilled = front_request->fulfill_request(new_node) ;

      if (front_fulfilled)
         // Request is just fulfilled by us, let the requester go
         front_request->_reqlock.unlock() ;

      // Attempt to remove the old head.
      this->retire_head(head) ;

      if (front_fulfilled)
         break ;
   }
}

template<typename T, typename A>
auto concurrent_dualqueue<T, A>::pop_node(bool lock_if_empty) -> node_hazard_ptr
{
   node_type *new_request_node = nullptr ;
   // Keep trying until successful dequeue
   for(;;)
   {
      node_hazard_ptr head {&this->_head} ;
      node_type *current_head = head.get() ;

      node_hazard_ptr tail {current_head != this->_tail
            ? node_hazard_ptr(&this->_tail)
            : this->ensure_consistent_tail()} ;
      node_type *current_tail = tail.get() ;

      if (current_head == current_tail || current_tail->is_request_node())
      {
         if (!lock_if_empty)
            // The only place in pop_node that returns null
            return nullptr ;

         // Needn't head any more
         head.reset() ;

         // No data nodes in the queue (queue is empty or consists of requests)
         if (!new_request_node)
            new_request_node = this->make_node() ;

         node_hazard_ptr request {new_request_node} ;
         // Enqueue new request
         if (!this->enqueue_node(current_tail, new_request_node))
            continue ;

         // Wait until someone else fulfilled our request
         new_request_node->_reqlock.wait() ;

         // Assume the old tail is the current head
         if (this->atomic_pop_head(current_tail, std::memory_order_release))
            this->retire_node(current_tail) ;

         NOXCHECK(new_request_node->_valptr) ;
         NOXCHECK(new_request_node->is_request_node()) ;

         // Return fulfilled request
         return std::move(request) ;
      }

      // The queue consists of (at least one) data node(s).
      // Needn't tail any more.
      tail.reset() ;

      node_hazard_ptr front {head->_next} ;

      if (this->retire_head(head))
      {
         NOXCHECK(!front->is_request_node()) ;
         this->delete_node(new_request_node) ;
         return std::move(front) ;
      }
   }
}

} // end of namespace pcomn

#endif /* __PCOMN_CDSQUEUE_H */
