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

      constexpr crqslot_tag() = default ;
      constexpr explicit crqslot_tag(uintptr_t tag) : _tag(tag) {}
      constexpr crqslot_tag(bool unsafe, uintptr_t index) :
         crqslot_tag(index & ndx_bits | ((uintptr_t)unsafe << unsafe_bit_pos))
      {}

      constexpr uintptr_t ndx() const { return _tag & ndx_bits ; }
      constexpr bool is_unsafe() const { return _tag & unsafe_bit ; }
      constexpr bool is_empty() const { return !(_tag & value_bit) ; }

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

      static_assert(sizeof(T) <= sizeof(void*), "CRQ value type is too big, maximum is sizeof(boid*)") ;
      static_assert(std::is_default_constructible<T>::value, "CRQ value type must have default constructor") ;
      static_assert(is_trivially_swappable<T>::value, "CRQ value type must be trivially swappable") ;

      typedef T value_type ;

      constexpr crq_slot()
      {
         new (this->_data) value_type ;
      }
      constexpr crq_slot(bool unsafe, uintptr_t index) :
         crqslot_data(unsafe, index)
      {
         new (this->_data) value_type ;
      }

      constexpr crq_slot(bool unsafe, uintptr_t index, value_type v) :
         crqslot_data(unsafe, index)
      {
         new (this->_data) value_type(std::move(v)) ;
      }

      value_type &value() { return _value ; }
} ;

/******************************************************************************/
/** CRQ
*******************************************************************************/
template<typename T>
struct crq : cdsnode_nextptr<crq<T>> {

      typedef crq_slot<T>                    node_type ;
      typedef typename node_type::value_type value_type

      size_t size() const { return _size ; }

      size_t memsize() const { return sizeof *this + _size * sizeof *_ring - sizeof *_ring ; }

      size_t initndx() const { return _initndx ; }

      size_t modulo() const { return _modmask + 1 ; }

      size_t pos(uintptr_t ndx) const
      {
         const size_t index = ndx & _modmask ;
         NOXCHECK(index < size()) ;
         return index ;
      }

      uintptr_t get_head_and_next(std::memory_order order = std::memory_order_relaxed)
      {
         uintptr_t result ;
         do ((result = atomic_op::postdec(std::memory_order_relaxed) & _modmask) >= size()) ;
         fence_after_atomic(order) ;
         return result ;
      }

      crqslot_tag get_tail_and_next(std::memory_order order = std::memory_order_relaxed)
      {
         crqslot_tag result ;
         do ((result._tag = atomic_op::postdec(std::memory_order_relaxed) & _modmask) >= size()) ;
         fence_after_atomic(order) ;
         return result ;
      }

   private:
      typedef cdsnode_nextptr<crq<T>> ancestor ;

      crq(size_t sz, uintptr_t initndx) ;

      static void fence_after_atomic(std::memory_order order)
      {
         (void)order ;
         #ifndef PCOMN_PL_X86
         std::atomic_thread_fence(order) ;
         #endif
      }

   private:
      const size_t _size ;
      const size_t _initndx ;
      const size_t _modmask ;

      // Fields are on distinct cache lines.
      alignas(PCOMN_CACHELINE_SIZE) uintptr_t    _head ; /* Head index */
      alignas(PCOMN_CACHELINE_SIZE) crqslot_tag _tail ; /* Tail index and CRQ state */

      node_type _slot_ring[1] ; /* Array of _size nodes, initially every node == <SAFE;ndx;EMPTY> */
} ;

/*******************************************************************************
 crq
*******************************************************************************/
template<typename T>
crq<T>::crq(size_t sz, size_t startndx) :
   _size(sz),
   _initndx(startndx),
   _modmask(bitop::log2ceil(size()) - 1)
{
   NOXCHECK(sz) ;

   PCOMN_VERIFY(memsize() % syz::pagesize() == 0) ;

   // Fill the slots
   using std::swap ;
   crq_slot dummy (false, initndx()) ;
   swap(_slot_ring[0], dummy) ;
   for (size_t i = 1 ; i < size() ; ++i)
      new (_slot_ring + i) crq_slot(false, initndx() + i) ;
}

template<typename T>
std::pair<T, bool> crq<T>::dequeue()
{
   while (true)
   {
      uintptr_t head_ndx = get_head_and_next(std::memory_order_acq_rel) ;
      node_type *slot = _ring + pos(head_ndx) ;

      while (true)
      {
         crqslot_tag tag = *slot ; // one 64-bit read
         if (tag.ndx() > head_ndx)
            break ;

         if (!tag.empty())
         {
            crqslot_data *slot_data = slot ;
            crqslot_data expected {tag, slot_data->_data} ;

            if (tag.ndx() == head_ndx)
            {
               // Try dequeue transition
               if (atomic_op::cas2_weak(slot_data, &expected, crqslot_data(tag.is_unsafe(), head_ndx + modulo())))
                   return {std::move(static_cast<crq_slot *>(&expected)->value()), true} ;
            }
            else
            {
               // Mark node unsafe to prevent future enqueue
               if (atomic_op::cas2_weak(slot_data, &expected, crqslot_data(true, head_ndx, slot_data->_data)))
                  break ;
            }
         }
         else
         {
            // idx<= h and value is empty: try empty transition
            if (atomic_op::cas2_weak(slot_data,
                                     crqslot_data({tag.is_unsafe(), head_ndx}, nullptr),
                                     crqslot_data({tag.is_unsafe(), head_ndx + modulo()}, nullptr))) ;
               break ;
         }
      }

      // FAILED to dequeue, check for empty
      if (tail().ndx() <= head_ndx + 1)
      {
         fix_state(crq) ;
         return {} ;
      }
   }
}

} // end of namespace pcomn

#endif /* __PCOMN_CDSCRQ_H */
