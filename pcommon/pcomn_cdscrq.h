/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_CDSCRQ_H
#define __PCOMN_CDSCRQ_H
/*******************************************************************************
 FILE         :   pcomn_cdscrq.h
 COPYRIGHT    :   Yakov Markovitch, 2016

 DESCRIPTION  :   Nonblocking concurrent array-based ring queue.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   4 Aug 2016
*******************************************************************************/
#include <pcomn_cdsbase.h>
#include <pcomn_sys.h>
#include <pcomn_integer.h>

#include <algorithm>
#include <new>

namespace pcomn {

/*******************************************************************************

*******************************************************************************/
struct crqslot_tag {
      /// Reserve 3 most significant bits for flags.
      /// Reserving @em most significant bits will allow increment index simply
      /// incrementing the whole tag w/o masking.
      static constexpr const uintptr_t ndx_bits = (uintptr_t(1) << (bitsizeof(uintptr_t) - 3)) - 1 ;

      static constexpr const uintptr_t unsafe_bit_pos   = bitsizeof(uintptr_t) - 1 ;
      static constexpr const uintptr_t value_bit_pos    = bitsizeof(uintptr_t) - 2 ;
      static constexpr const uintptr_t reserved_bit_pos = bitsizeof(uintptr_t) - 3 ;

      /// The most significant bit of the tag is "unsafe" bit (i.e. 0 is safe, 1 is unsafe).
      static constexpr const uintptr_t unsafe_bit   = uintptr_t(1) << unsafe_bit_pos ;
      static constexpr const uintptr_t value_bit    = uintptr_t(1) << value_bit_pos ;
      /// For future use
      static constexpr const uintptr_t reserved_bit = uintptr_t(1) << reserved_bit_pos ;

      constexpr crqslot_tag() : crqslot_tag(0) {}
      constexpr explicit crqslot_tag(uintptr_t tag) : _tag(tag) {}
      constexpr crqslot_tag(bool safe, uintptr_t index) :
         crqslot_tag(index & ndx_bits | ((uintptr_t)!safe << unsafe_bit_pos))
      {}

      constexpr uintptr_t ndx() const { return _tag & ndx_bits ; }
      constexpr bool is_safe() const { return !(_tag & unsafe_bit) ; }
      constexpr bool is_empty() const { return !(_tag & value_bit) ; }

      bool test_and_set(unsigned bitpos, std::memory_order order)
      {
         NOXCHECK(bitpos < bitsizeof(_tag) && ((uintptr_t)1 << bitpos) > ndx_bits) ;

         const uintptr_t bit = uintptr_t(1) << bitpos ;
         return
            atomic_op::bit_or(&_tag, bit, order) & bit ;
      }

      crqslot_tag &set_ndx(uintptr_t index)
      {
         set_flags_masked(_tag, index, ndx_bits) ;
         return *this ;
      }

      uintptr_t _tag ;   /** */
} ;

struct alignas(2*sizeof(void*)) crqslot_data : crqslot_tag {

      using crqslot_tag::crqslot_tag ;

      explicit constexpr crqslot_data(crqslot_tag tag = {}) : crqslot_tag(tag) {}
      constexpr crqslot_data(crqslot_tag tag, void *d) :
         crqslot_tag(tag._tag | crqslot_tag::value_bit), _data(d)
      {}

      void *_data = nullptr ;
} ;

PCOMN_STATIC_CHECK(is_atomic2<crqslot_data>::value) ;

/******************************************************************************/
/** CRQ slot
*******************************************************************************/
template<typename T>
struct alignas(PCOMN_CACHELINE_SIZE) crq_slot : public crqslot_data {

      static_assert(sizeof(T) <= sizeof(void*), "CRQ value type is too big, maximum is sizeof(void*)") ;
      static_assert(std::is_default_constructible<T>::value, "CRQ value type must have default constructor") ;
      static_assert(is_trivially_swappable<T>::value, "CRQ value type must be trivially swappable") ;

      typedef T value_type ;

      crq_slot() { new (&this->_data) value_type ; }

      crq_slot(bool safe, uintptr_t index) :
         crqslot_data(safe, index)
      {
         new (&this->_data) value_type ;
      }

      crq_slot(bool safe, uintptr_t index, value_type &v) :
         crq_slot(safe, index)
      {
         using std::swap ;
         swap(v, value()) ;
         this->_tag |= crqslot_tag::value_bit ;
      }

      value_type &value() { return *static_cast<value_type *>(static_cast<void *>(&_data)) ; }
} ;

/******************************************************************************/
/** CRQ
*******************************************************************************/
template<typename T>
struct crq : cdsnode_nextptr<crq<T>> {

      typedef crq_slot<T>                    slot_type ;
      typedef typename slot_type::value_type value_type ;

      ~crq() { destroy_slots(std::bool_constant<std::is_trivially_destructible<value_type>::value>()) ; }

      static crq *make_crq(uintptr_t initndx, size_t capacity_request = 1) ;

      size_t capacity() const { return _capacity ; }

      size_t memsize() const { return memsize(capacity()) ; }

      size_t initndx() const { return _initndx ; }

      size_t modulo() const { return _modmask + 1 ; }

      bool empty() const { return _tail.ndx() <= _head ; }

      size_t pos(uintptr_t ndx) const
      {
         const size_t index = (ndx - initndx()) & _modmask ;
         NOXCHECK(index < capacity()) ;
         return index ;
      }

      bool enqueue(value_type &value) ;

      bool enqueue(value_type &&value)
      {
         return enqueue(static_cast<value_type &>(value)) ;
      }

      std::pair<T, bool> dequeue() ;

      uintptr_t head_fetch_and_next(std::memory_order order = std::memory_order_relaxed)
      {
         return fetch_and_next(&_head, order) ;
      }

      crqslot_tag tail_fetch_and_next(std::memory_order order = std::memory_order_relaxed)
      {
         return crqslot_tag(fetch_and_next(&_tail._tag, order)) ;
      }

      static void operator delete(void *data) { sys::pagefree(data) ; }

   private:
      typedef cdsnode_nextptr<crq<T>> ancestor ;

      crq(uintptr_t initndx, size_t capacity) ;

      static void *operator new(size_t sz)
      {
         NOXCHECK(sz <= sys::pagesize()) ;
         (void)sz ;
         void * const page = sys::pagealloc() ;
         if (!page)
            throw_exception<bad_alloc_msg>(sys::strlasterr()) ;
         return page ;
      }

      static size_t memsize(size_t capacity)
      {
         return sizeof(crq<T>) + capacity * sizeof *_slot_ring - sizeof *_slot_ring ;
      }

      static void fence_after_atomic(std::memory_order order)
      {
         (void)order ;
         #ifndef PCOMN_PL_X86
         std::atomic_thread_fence(order) ;
         #endif
      }

      bool cas2(crqslot_data *target, crqslot_data *expected, const crqslot_data &desired,
                std::memory_order order = std::memory_order_acq_rel)
      {
         const bool success = atomic_op::cas2_weak(target, expected, desired, std::memory_order_relaxed) ;
         fence_after_atomic(order) ;
         return success ;
      }

      static void destroy_slot_value(slot_type &slot) noexcept
      {
         slot.value().~value_type() ;
      }

      uintptr_t fetch_and_next(uintptr_t *mark, std::memory_order order) const
      {
         uintptr_t result ;

         do result = atomic_op::postinc(mark, std::memory_order_relaxed) ;
         while (((result - initndx()) & _modmask) >= capacity()) ;

         fence_after_atomic(order) ;
         return result ;
      }

      void fix_tail() ;

      bool starving() const { return false ; }

      static void destroy_slots(std::true_type) {}
      void destroy_slots(std::false_type)
      {
         for (slot_type *s = _slot_ring, *e = s + capacity() ; s != e ; ++s)
            s->value().~value_type() ;
      }

   private:
      const size_t _capacity ;
      const size_t _initndx ;
      const size_t _modmask ;

      // Fields are on distinct cache lines.
      alignas(PCOMN_CACHELINE_SIZE) uintptr_t   _head ; /* Head index */
      alignas(PCOMN_CACHELINE_SIZE) crqslot_tag _tail ; /* Tail index and CRQ state */

      slot_type _slot_ring[1] ; /* Array of _capacity nodes, initially every node == <SAFE;ndx;EMPTY> */
} ;

/*******************************************************************************
 crq
*******************************************************************************/
template<typename T>
crq<T>::crq(size_t startndx, size_t actual_capacity) :
   _capacity(actual_capacity),
   _initndx(startndx),
   _modmask(((size_t)1 << bitop::log2ceil(capacity())) - 1),
   _head(initndx()),
   _tail(initndx())
{
   NOXCHECK(capacity()) ;

   PCOMN_VERIFY((memsize() & sys::pagemask()) == 0) ;

   // Fill the slots
   using std::swap ;
   slot_type dummy (true, initndx()) ;
   swap(_slot_ring[0], dummy) ;
   for (size_t i = 1 ; i < capacity() ; ++i)
      new (_slot_ring + i) slot_type(true, initndx() + i) ;
}

template<typename T>
std::pair<T, bool> crq<T>::dequeue()
{
   while (true)
   {
      const uintptr_t xhead      = head_fetch_and_next(std::memory_order_acq_rel) ;
      crqslot_data * const slot  = _slot_ring + pos(xhead) ;
      crqslot_data slot_data     = *slot ;

      while (xhead >= slot_data.ndx())
      {
         if (slot_data.is_empty())
         {
            const crqslot_data new_empty_data {slot_data.is_safe(), xhead + modulo()} ;

            // head_index <= slot_index and value is empty, our enqueuer is late: try empty transition
            if (cas2(slot, &slot_data, new_empty_data))
               // Empty transition succesful, get the new head index, try again
               break ;

            // Cannot make empty transition, something changed, maybe our enqueuer
            // managed it on time? Try again with the same head index.
            // Note thay after cas2 slot_data contains actual tag.
            continue ;
         }

         // ============================================
         // === If we are here, the slot is nonempty ===
         // ============================================

         if (xhead > slot_data.ndx())
            // We are ahead of enqueuers for k full rounds (k >= 1).
            // Mark node unsafe to prevent future enqueue.
            if (cas2(slot, &slot_data, crqslot_data({false, slot_data.ndx()}, slot_data._data)))
               // Node marked unsafe, get the new head index, try again.
               break ;
            else
               continue ;

         // ============================================
         // === If we are here, this is our slot. ======
         // === Try dequeue transition.           ======
         // ============================================

         slot_type new_empty_data {slot_data.is_safe(), xhead + modulo()} ;
         if (cas2(slot, &slot_data, new_empty_data))
         {
            slot_type &slot_content = *static_cast<slot_type *>(&slot_data) ;

            std::pair<value_type, bool> result ;
            result.second = true ;

            pcomn_swap(result.first, slot_content.value()) ;
            destroy_slot_value(slot_content) ;

            return std::move(result) ;
         }
      }

      // FAILED to dequeue, need new head index.
      // Check for empty.
      if (_tail.ndx() <= xhead + 1)
      {
         fix_tail() ;
         return {} ;
      }
   }
}

template<typename T>
bool crq<T>::enqueue(value_type &value)
{
   slot_type newval {true, 0, value} ;

   // We use unsafe_bit in tail index to mark the CRQ queue as closed and throw a
   // tantrum.
   for (crqslot_tag tail ; (tail = tail_fetch_and_next()).is_safe() ;)
   {
      const intptr_t xtail = tail.ndx() ;
      crqslot_data * const slot = _slot_ring + pos(xtail) ;
      NOXCHECK(slot < _slot_ring + capacity()) ;
      crqslot_data slot_data = *slot ;

      if (slot_data.is_empty())
      {
         newval.set_ndx(xtail) ;

         if ((intptr_t)slot_data.ndx() <= xtail &&
             (slot_data.is_safe() || (intptr_t)_head <= xtail) &&
             // Here is the enqueue transition
             cas2(slot, &slot_data, newval))
         {
            destroy_slot_value(*static_cast<slot_type *>(&slot_data)) ;
            return true ;
         }
      }

      const intptr_t xhead = _head ;

      // Check is full or starving
      if (xtail - xhead >= (intptr_t)capacity() || starving())
      {
         // Close the CRQ
         _tail.test_and_set(crqslot_tag::unsafe_bit_pos, std::memory_order_seq_cst) ;
         // Need not to check loop condition: we've just explicitly set the unsafe bit
         break ;
      }
   }

   pcomn_swap(newval.value(), value) ;
   destroy_slot_value(newval) ;

   return false ;
}

template<typename T>
void crq<T>::fix_tail()
{
   while (true)
   {
      const uintptr_t   head = atomic_op::load(&_head, std::memory_order_acquire) ;
      const crqslot_tag tail = atomic_op::load(&_tail, std::memory_order_relaxed) ;

      if (_tail.ndx() != tail.ndx())
         continue ;


      if (head <= _tail.ndx() /* Nothing to do, already OK */ ||
          // Bump the tail to catch up with the head
          atomic_op::cas(&_tail, tail, crqslot_tag(tail.is_safe(), head), std::memory_order_release))
         break ;
   }
}

template<typename T>
crq<T> *crq<T>::make_crq(uintptr_t initndx, size_t)
{
   const size_t msize = sys::pagesize() ;
   const size_t actual_capacity = (msize - sizeof(crq<T>)) / sizeof *_slot_ring + 1 ;

   NOXCHECK(memsize(actual_capacity) == sys::pagesize()) ;

   return new crq<T>(initndx, actual_capacity) ;
}

} // end of namespace pcomn

#endif /* __PCOMN_CDSCRQ_H */
