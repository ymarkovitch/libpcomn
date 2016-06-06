/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_HAZARDPTR_H
#define __PCOMN_HAZARDPTR_H
/*******************************************************************************
 FILE         :   pcomn_hazardptr.h
 COPYRIGHT    :   Yakov Markovitch, 2016, All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Hazard pointers for lockless concurrent data structures

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   20 May 2016
*******************************************************************************/
/** @file
 Implementation of hazard pointers for lockless concurrent data structures.

  - hazard_pointer\<Element, Tag\> refers to @em  thread-local hazard_manager\<Tag\>,
    i.e all hazard pointers\<Element\> with the same tag in a thread refer to the same
    thread-local hazard_manager\<Tag\> in that thread.

  - hazard_manager\<Tag\> refers to @em global hazard_storage\<hazard_traits\<Tag\>\>

  - hazard_storage\<hazard_traits\<Tag\>\>
    hazard_registry\<log2(hazard_traits\<Tag\>::thread_capacity\>
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcomn_meta.h>
#include <pcomn_integer.h>
#include <pcomn_assert.h>
#include <pcomn_atomic.h>
#include <pcomn_bitvector.h>
#include <pcommon.h>

#include <memory>
#include <new>

#include <stdlib.h>
#include <string.h>

#ifdef PCOMN_PL_MS
// For _aligned_malloc/_aligned_free
#include <malloc.h>
#endif

namespace pcomn {

/*******************************************************************************
 Constants
*******************************************************************************/
constexpr int32_t HAZARD_BADCALL = 0xBADDCA11 ;

constexpr size_t HAZARD_DEFAULT_CAPACITY = 7 ;

constexpr size_t HAZARD_DEFAULT_THREADCOUNT = 128 ;

/*******************************************************************************
 Forward declarations
*******************************************************************************/
template<typename, typename>
class hazard_pointer ;

template<typename>
class hazard_manager ;

template<unsigned>
class hazard_registry ;

template<unsigned>
class hazard_storage ;

template<unsigned L>
std::unique_ptr<hazard_storage<L>> new_hazard_storage(unsigned thread_maxcount = 0) ;

/******************************************************************************/
/** Describes various hazard pointer policies
*******************************************************************************/
template<size_t Capacity>
struct hazard_policy {
      static constexpr size_t thread_capacity = Capacity ;
} ;

/******************************************************************************/
/** Policy type for hazard_manager
*******************************************************************************/
template<typename T>
struct hazard_traits ;

template<>
struct hazard_traits<void> : hazard_policy<HAZARD_DEFAULT_CAPACITY> {} ;

/******************************************************************************/
/** Hazard pointer marks a non-null pointer to a node of some lock-free dynamic
 object as intended to be further accessed by the current thread without
 furhter validation.

 @note Objects of this class are movable, enabling to return them from functions,
 but they must @em never be passed between threads.
*******************************************************************************/
template<typename E, typename T = void>
class hazard_pointer {
      PCOMN_NONCOPYABLE(hazard_pointer) ;
      PCOMN_NONASSIGNABLE(hazard_pointer) ;
   public:
      typedef E                        element_type ;
      typedef T                        tag_type ;
      typedef hazard_traits<tag_type>  traits_type ;

      constexpr hazard_pointer() = default ;

      hazard_pointer(hazard_pointer &&other) : _ptr(other._ptr)
      {
         other._ptr = nullptr ;
      }

      template<typename U, typename = std::enable_if_t<std::is_convertible<U *, T *>::value, void>>
      hazard_pointer(hazard_pointer<U, T> &&other) : _ptr(other._ptr)
      {
         other._ptr = nullptr ;
      }

      /// Mark @a ptr as a hazard pointer.
      explicit hazard_pointer(element_type *ptr) : _ptr(ptr)
      {
         mark_hazard() ;
      }

      /// Unmark a hazard pointer, inform other threads the object pointed to is no more
      /// in use by the current thread.
      ~hazard_pointer()
      {
         unmark_hazard() ;
      }

      element_type *get() const { return _ptr ; }
      element_type *operator->() const { return get() ; }
      element_type &operator*() const { return *get() ; }

      explicit operator bool() const { return !!get() ; }

      /// Mark the pointer as safe for reclaim.
      /// Since after this call the plain pointer this object has held is eventually
      /// invalid, so no return value.
      void reset()
      {
         unmark_hazard() ;
         _ptr = nullptr ;
      }

   private:
      element_type *_ptr = nullptr ;
      int32_t       _slot = HAZARD_BADCALL ; /* Slot index in hazard registry */

      void mark_hazard()
      {
         if (_ptr) ;
      }
      void unmark_hazard()
      {
      }
} ;

/******************************************************************************/
/** A per-thread hazard pointer registry
*******************************************************************************/
template<unsigned Log2 = 0>
class alignas(PCOMN_CACHELINE_SIZE) hazard_registry {

      static_assert(Log2 <= 3,
                    "The capacity of hazard_registry cannot exceed 63 pointers, "
                    "Log2 template argument is too big (Log2 > 3)") ;

   public:
      constexpr hazard_registry() : _occupied{}, _hazard{} {}
      ~hazard_registry() = default ;

      /// The maximum count of hazard pointers per thread
      static constexpr size_t capacity() { return  (8 << (int)Log2) - 1 ; }

      /// Register a hazard pointer.
      /// No more than max_size pointers may be registered at the same time.
      ///
      /// @param ptr A hazard pointer.
      /// @return Hazard pointer index, which must be passed to unregister_hazard(); on
      /// attempt to register more than max_size pointers, returns 0xBADDCA11.
      ///
      /// @note The returned index is valid only for the calling thread, hazard_pointer
      /// objects can @em not be passed between threads.
      ///
      int register_hazard(const void *ptr)
      {
         NOXCHECK(ptr) ;

         int slot = HAZARD_BADCALL ;
         const uint64_t slotmask = (uint64_t)1 << slot ;
         _hazard[slot] = ptr ;
         // Memory barrier
         atomic_op::store(&_occupied, _occupied | slotmask, std::memory_order_release) ;
         return slot ;
      }

      /// Unregister a hazard pointer.
      /// @param slot A slot index previously returned from register_hazard() call.
      ///
      void unregister_hazard(int slot)
      {
         const uint64_t slotmask = (uint64_t)1 << slot ;
         if ((unsigned)slot >= std::size(_hazard) || !(_occupied & slotmask))
            PCOMN_FAIL("Invalid hazard pointer slot index passed to unregister_hazard") ;

         // The following operations need not be atomic and do not require memory barriers
         _occupied &= ~slotmask ;
         _hazard[slot] = nullptr ;
      }

   private:
      uint64_t    _occupied ;
      const void *_hazard[capacity()] ;
} ;

/******************************************************************************/
/** Lockless storage for hazard registries.

 Cannot be placed onto the stack, the only method to create hazard_storage object
 is with new_hazard_storage() call.
*******************************************************************************/
template<unsigned Log2 = 0>
class alignas(PCOMN_CACHELINE_SIZE) hazard_storage {

      PCOMN_NONCOPYABLE(hazard_storage) ;
      PCOMN_NONASSIGNABLE(hazard_storage) ;

      typedef basic_bitvector<uint64_t> slots_bitmap ;

   public:
      typedef hazard_registry<Log2> registry_type ;

      void operator delete(void *ptr) { deallocate(ptr) ; }

      /// Constructor function.
      /// The actual constructor is private, to disable placing hazard_storage onto the
      /// stack.
      template<unsigned L>
      friend std::unique_ptr<hazard_storage<L>>
      new_hazard_storage(unsigned thread_maxcount)
      {
         typedef hazard_storage<L> storage ;
         if (!thread_maxcount)
            thread_maxcount = HAZARD_DEFAULT_THREADCOUNT ;

         return std::unique_ptr<storage>(new ((int)thread_maxcount) storage(thread_maxcount)) ;
      }

      /// Get the maximum count of registered threads
      size_t capacity() const { return _slots_map.size() ; }

      /// Allocate a hazard registry.
      /// @note thread-safe
      registry_type *allocate_slot() ;

      /// Release a hazard registry.
      /// @note thread-safe
      void release_slot(registry_type *) ;

   private:
      const slots_bitmap      _slots_map ;  /* Registry slots bitmap */
      registry_type * const   _registries ; /* Registry slots */

   private:
      explicit hazard_storage(unsigned slotcount) ;

      // Allocate memory enough to place sequentially the hazard_storage object, registry
      // slots map data, and registry slots themselves, accounting for alignment.
      void *operator new(size_t sz, int thread_maxcount) { return allocate(thread_maxcount) ; }
      void operator delete(void *ptr, int) { deallocate(ptr) ; }

      // Given requested slots count, get the actual count aligned to slots_bitmap's
      // bitcount granularity.
      static constexpr size_t slotcount(unsigned requested_count)
      {
         return
            (requested_count + slots_bitmap::bits_per_element() - 1)
            / slots_bitmap::bits_per_element()
            * slots_bitmap::bits_per_element() ;
      }

      // Allocate continuous memory regison aligned to cacheline size and sufficient for
      // continuous placement of hazard_storage object itself, the slot bitmap, and
      // registry slots themselvels.
      // The allocated memory must be filled with zeros.
      static void *allocate(unsigned thread_maxcount) ;

      // Deallocate meory region allocated with allocate(); ignore nullptr
      static void deallocate(void *ptr) ;

      const slots_bitmap &slots_map() const { return _slots_map ; }

      registry_type *slots() { return _registries ; }
      const registry_type *slots() const { return _registries ; }

      // The raw data area immediately follows the memory cuupied by hazard_storage
      // object itself
      void *raw_data() { return this + 1 ; }

      // Offset in bytes from the end of hazard_storage object memory to the start of
      // registry slots memory
      static constexpr size_t slotdata_offset(size_t registry_slotcount)
      {
         return
            (slots_bitmap::cellndx(registry_slotcount) * sizeof(typename slots_bitmap::element_type) +
             alignof(registry_type) - 1)/alignof(registry_type)*alignof(registry_type) ;
      }
} ;

/******************************************************************************/
/** The manager of hazard pointers.

 There is a manager per tag per thread
*******************************************************************************/
template<typename T>
class hazard_manager {
      template<typename E, typename U>
      friend class hazard_pointer ;
   public:
      typedef T                        tag_type ;
      typedef hazard_traits<tag_type>  traits_type ;

      explicit hazard_manager(size_t thread_maxcount) ;

      /// Register an object for posponed reclamation and provide reclaiming function.
      template<typename U, typename F>
      std::enable_if_t<std::is_convertible<U, std::function<void(U*)>>::value, void>
      mark_for_cleanup(U *object, F reclaimer)
      {
      }

   private:
      typedef hazard_storage<bitop::ct_log2floor<traits_type::thread_capacity>::value> pointer_storage ;

      /// Get the hazard pointer manager for this thread + tags
      static hazard_manager &manager() ;
      /// Get the global hazard pointer storage for this tag
      static pointer_storage &storage() { return _storage ; }

   private:
      static pointer_storage _storage ; /* Global hazard pointer storage for tag_type */
} ;

/*******************************************************************************
 hazard_storage
*******************************************************************************/
template<unsigned Log2>
hazard_storage<Log2>::hazard_storage(unsigned requested_slotcount) :

   // Slots map starts immediately past the end of hazard_storage object itself
   _slots_map(static_cast<typename slots_bitmap::element_type *>(raw_data()),
              slots_bitmap::cellndx(slotcount(requested_slotcount))),

   // Array of registries starts after the slots map, accounting for alignment
   _registries(static_cast<registry_type *>(padd<void>(raw_data(), slotdata_offset(slots_map().size()))))
{
   PCOMN_VERIFY(requested_slotcount > 0) ;
}

template<unsigned L>
void *hazard_storage<L>::allocate(unsigned thread_maxcount)
{
   NOXCHECK(thread_maxcount != 0) ;
   const size_t slot_count = slotcount(thread_maxcount) ;
   const size_t memsize =
      sizeof(hazard_storage) +
      slotdata_offset(slot_count) +
      sizeof(registry_type) * slot_count ;

   void * const mem =
#ifndef PCOMN_PL_MS
      aligned_alloc(alignof(hazard_storage), memsize)
#else
      _aligned_malloc(memsize, alignof(hazard_storage))
#endif
      ;
   // Check and zero-fill
   return memset(ensure_nonzero<std::bad_alloc>(mem), 0, memsize) ;
}

template<unsigned L>
void hazard_storage<L>::deallocate(void *ptr)
{
   if (!ptr)
      return ;
#ifndef PCOMN_PL_MS
   free(ptr) ;
#else
   _aligned_free(ptr) ;
#endif
}

template<unsigned L>
typename hazard_storage<L>::registry_type *hazard_storage<L>::allocate_slot()
{
   do
   {
      bool failed_attempts = false ;
      // Search the slots map for 0 bit (i.e. free slot position)
      for (auto p = slots_map().begin_positional(), e = end_positional() ; p != e ; ++p)
      {
      }
   }
   while (failed_attempts) ;

   PCOMN_THROWF(implimit_error,
                "attempt to allocate more than %u hazard registry slots. "
                "Reduce thread count or increase hazard storage capacity.", (unsigned)capacity()) ;
}

template<unsigned L>
void hazard_storage<L>::release_slot(registry_type *slot)
{
   PCOMN_VERIFY(slot != nullptr) ;
   PCOMN_ENSURE(!((uintptr_t)slot & (alignof(*slot) - 1)),
                "Invalid alignment of a hazard registry pointer") ;

   const size_t slotpos = slot - slots() ;

   PCOMN_ENSURE(slotpos < capacity(),
                "Attempt to release a hazard registry pointer that does not belong to the storage.") ;

   memset(slot, 0, sizeof *slot) ;
   slots_map().set(slotpos, false, std::memory_order_release) ;
}

/*******************************************************************************
 hazard_manager
*******************************************************************************/
template<typename T>
hazard_manager<T> &hazard_manager<T>::manager()
{
   thread_local hazard_manager thread_manager ;
   return thread_manager ;
}

} // end of namespace pcomn

#endif /* __PCOMN_HAZARDPTR_H */
