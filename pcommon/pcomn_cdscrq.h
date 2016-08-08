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
      constexpr crqslot_tag(bool unsafe, uintptr_t index) :
         crqslot_tag(index & ndx_bits | ((uintptr_t)unsafe << unsafe_bit_pos))
      {}

      constexpr uintptr_t ndx() const { return _tag & ndx_bits ; }
      constexpr bool is_unsafe() const { return _tag & unsafe_bit ; }
      constexpr bool is_empty() const { return !(_tag & value_bit) ; }

      bool test_and_set(unsigned bitpos, std::memory_order order)
      {
         NOXCHECK(bitpos >= ndx_bits && bitpos < bitsizeof(_tag)) ;

         const uintptr_t bit = uintptr_t(1) << bitpos ;
         return
            atomic_op::bit_or(&_tag, bit, order) & bit ;
      }

      uintptr_t _tag ;   /** */
} ;

struct alignas(2*sizeof(void*)) crqslot_data : public crqslot_tag {

      using crqslot_tag::crqslot_tag ;
      explicit constexpr crqslot_data(crqslot_tag tag, void *d = nullptr) :
         crqslot_tag(tag), _data(d)
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

      crq_slot() { new (this->_data) value_type ; }

      crq_slot(bool unsafe, uintptr_t index) :
         crqslot_data(unsafe, index)
      {
         new (this->_data) value_type ;
      }

      crq_slot(bool unsafe, uintptr_t index, value_type &v) :
         crq_slot(unsafe, index)
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

      static crq *make_crq(size_t capacity, uintptr_t initndx) ;

      size_t capacity() const { return _capacity ; }

      size_t memsize() const
      {
         return sizeof *this + _capacity * sizeof *_slot_ring - sizeof *_slot_ring ;
      }

      size_t initndx() const { return _initndx ; }

      size_t modulo() const { return _modmask + 1 ; }

      bool empty() const { return _tail.ndx() <= _head ; }

      size_t pos(uintptr_t ndx) const
      {
         const size_t index = ndx & _modmask ;
         NOXCHECK(index < capacity()) ;
         return index ;
      }

      bool enqueue(value_type &value) ;

      std::pair<T, bool> dequeue() ;

      uintptr_t head_fetch_and_next(std::memory_order order = std::memory_order_relaxed)
      {
         uintptr_t result ;
         while ((result = atomic_op::postdec(&_head, std::memory_order_relaxed) & _modmask) >= capacity()) ;
         fence_after_atomic(order) ;
         return result ;
      }

      crqslot_tag tail_fetch_and_next(std::memory_order order = std::memory_order_relaxed)
      {
         crqslot_tag result ;
         while ((result._tag = atomic_op::postdec(&_tail._tag, std::memory_order_relaxed) & _modmask) >= capacity()) ;
         fence_after_atomic(order) ;
         return result ;
      }

   private:
      typedef cdsnode_nextptr<crq<T>> ancestor ;

      crq(size_t capacity, uintptr_t initndx) ;

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

      void fix_tail() ;

      bool starving() const { return false ; }

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
crq<T>::crq(size_t capac, size_t startndx) :
   _capacity(capac),
   _initndx(startndx),
   _modmask(bitop::log2ceil(capacity()) - 1)
{
   NOXCHECK(capacity()) ;

   PCOMN_VERIFY(memsize() % sys::pagesize() == 0) ;

   // Fill the slots
   using std::swap ;
   slot_type dummy (false, initndx()) ;
   swap(_slot_ring[0], dummy) ;
   for (size_t i = 1 ; i < capacity() ; ++i)
      new (_slot_ring + i) slot_type(false, initndx() + i) ;
}

template<typename T>
std::pair<T, bool> crq<T>::dequeue()
{
   while (true)
   {
      const uintptr_t head_ndx = head_fetch_and_next(std::memory_order_acq_rel) ;
      slot_type * const slot = _slot_ring + pos(head_ndx) ;

      crqslot_tag slot_tag = *slot ;

      while (head_ndx <= slot_tag.ndx())
      {
         if (slot_tag.is_empty())
         {
            slot_type empty_data {slot_tag.is_unsafe(), head_ndx} ;
            // Head_index >= slot_index and value is empty, our enqueuer is late: try empty transition
            if (cas2(slot, &empty_data, {slot_tag.is_unsafe(), head_ndx + modulo()}))
               break ;
            else
            {
               // After cas2 empty_data contains actual tag
               slot_tag = empty_data ;
               // Cannot make empty transition, something changed, maybe our enqueuer
               // managed it on time? Try again with the same head index
               continue ;
            }
         }

         void * const data = slot->_data ;
         // Slot is nonempty
         crqslot_data expected {slot_tag, data} ;

         if (slot_tag.ndx() != head_ndx)
         {
            // Actually head_ndx > slot_tag.ndx() here (see while condition above).
            // This means we are ahead of enquers for k full rounds (k >= 1).
            // Mark node unsafe to prevent future enqueue.
            if (cas2(slot, &expected, crqslot_data({true, head_ndx}, data)))
               // Get new head index
               break ;
         }

         slot_type new_empty_data {slot_tag.is_unsafe(), head_ndx + modulo()} ;
         // Here head_ndx == slot_tag.ndx() and the slot is nonempty.
         // Try dequeue transition,
         if (cas2(slot, &expected, new_empty_data))
         {
            using namespace std ;
            slot_type &slot_content = *static_cast<slot_type *>(&expected) ;

            pair<value_type, bool> result ;
            result.second = true ;

            swap(result.first, slot_content.value()) ;
            destroy_slot_value(slot_content) ;

            return move(result) ;
         }
      }

      // FAILED to dequeue, need new head index.
      // Check for empty.
      if (_tail.ndx() <= head_ndx + 1)
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
   crqslot_tag &newtag = newval ;

   // We use unsafe_bit in tail index to mark the CRQ queue as closed and throw a
   // tantrum.
   for (crqslot_tag tail ; !(tail = tail_fetch_and_next()).unsafe() ;)
   {
      const uintptr_t tail_ndx = tail.ndx() ;
      slot_type * const slot = _slot_ring + pos(tail_ndx) ;
      crqslot_data slot_data = *slot ;

      newtag = crqslot_tag(true, tail_ndx) ;

      if (slot_data.is_empty() && slot_data.ndx() <= tail.ndx() &&
          (!slot_data.is_unsafe() || _head <= tail_ndx) &&
          // Here is the enqueue transition
          cas2(slot, &slot_data, newval))
      {
         destroy_slot_value(*static_cast<slot_type *>(&slot_data)) ;
         return true ;
      }

      const uintptr_t head_ndx = _head ;

      // Check is full or starving
      if (tail_ndx - head_ndx >= modulo() || starving())
      {
         // Close the CRQ
         _tail.test_and_set(crqslot_tag::unsafe_bit_pos, std::memory_order_seq_cst) ;
         // Need not to check loop condition: we've just explicitly set the unsafe bit
         break ;
      }
   }

   using namespace std ;

   swap(newval.value(), value) ;
   destroy_slot_value(newval) ;

   return false ;
}

template<typename T>
void crq<T>::fix_tail()
{
   while (true)
   {
      const uintptr_t   head = atomic_op::load(&_head, std::memory_order_seq_cst) ;
      const crqslot_tag tail = atomic_op::load(&_tail, std::memory_order_relaxed) ;

      if (_tail.ndx() != tail.ndx())
         continue ;


      if (head <= _tail.ndx() /* Nothing to do, already OK */ ||
          // Bump the tail to catch up with the head
          atomic_op::cas(&_tail, tail, crqslot_tag(tail.is_unsafe(), head), std::memory_order_seq_cst))
         break ;
   }
}

} // end of namespace pcomn

#endif /* __PCOMN_CDSCRQ_H */
