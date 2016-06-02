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

#if defined(PCOMN_PL_POSIX)
#include <unistd.h>
#include <sys/mman.h>
#elif defined(PCOMN_PL_MS)
#include <windows.h>
#else
#error The platform is not supported by pcomn_hazardptr.
#endif

namespace pcomn {

/*******************************************************************************
 Constants
*******************************************************************************/
constexpr int32_t HAZARD_BADCALL = 0xBADDCA11 ;

constexpr size_t HAZARD_DEFAULT_CAPACITY = 7 ;

constexpr size_t HAZARD_DEFAULT_THREADCOUNT = 128 ;

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
      // Use 4K as a guesstimate: there is no 32- or 64-bit CPU with memory page which is
      // not multiple of 4K bytes
      static_assert(!(4*KiB % sizeof(hazard_registry)),
                    "The sizeof(hazard_registry) must be page size divizor") ;

   public:
      constexpr hazard_registry() : _occupied{}, _hazard{} {}

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
*******************************************************************************/
template<unsigned Log2 = 0>
class alignas(PCOMN_CACHELINE_SIZE) hazard_storage {
   public:
      typedef hazard_registry<Log2> registry_type ;

      explicit hazard_storage(unsigned thread_maxcount = HAZARD_DEFAULT_THREADCOUNT) :
         hazard_storage(alloc_slotmem(thread_maxcount ? std::max(thread_maxcount, 2U) : HAZARD_DEFAULT_THREADCOUNT))
      {}

      ~hazard_storage() ;

      /// Get the maximum count of registered threads
      size_t capacity() const { return _slots_map.size() ; }

      /// Allocate a hazard registry.
      /// @note thread-safe
      registry_type *allocate_slot() ;

      /// Release a hazard registry.
      /// @note thread-safe
      void release_slot(registry_type *) ;

   private:
      registry_type * const             _registries ; /* Registry slots */
      const std::unique_ptr<uint64_t[]> _map_data ;   /* Registry slots bitmap data */
      const basic_bitvector<uint64_t>   _slots_map ;  /* Registry slots bitmap */

      alignas(PCOMN_CACHELINE_SIZE) std::atomic<size_t> _map_ubound = {} ; /* Upper bound of allocated slots */

   private:
      explicit hazard_storage(const std::pair<void *, size_t> &allocated) ;

      static std::pair<void *, size_t> alloc_slotmem(size_t bytes) ;
      static void free_slotmem(void *) ;

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
hazard_storage<Log2>::hazard_storage(const std::pair<void *, size_t> &allocated) :
   _registries(static_cast<registry_type *>(ensure_nonzero<std::bad_alloc>(allocated.first)))
{
   NOXCHECK(capacity() >= 2) ;
}

template<unsigned Log2>
hazard_storage<Log2>::~hazard_storage()
{
   PCOMN_STATIC_CHECK(std::is_trivially_destructible<registry_type>::value) ;
   free_slotmem(_registries) ;
}

#ifdef PCOMN_PL_POSIX
template<unsigned Log2>
__noinline std::pair<void *, size_t> hazard_storage<Log2>::alloc_slotmem(size_t bytes)
{
   NOXCHECK(bytes) ;
   static size_t pagesz ;
   if (!pagesz)
      pagesz = sysconf(_SC_PAGESIZE) ;

   const size_t allocsz = (bytes + pagesz - 1)/pagesz*pagesz ;
   void * const mem = mmap(nullptr, allocsz, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) ;
   return {ensure_nonzero<std::bad_alloc>(mem), allocsz} ;
}

template<unsigned Log2>
__noinline void hazard_storage<Log2>::free_slotmem(void *memptr)
{
   PCOMN_VERIFY(memptr != nullptr) ;
   munmap(memptr, capacity() * sizeof(registry_type)) ;
}
#else
#error Windows version is not implemented yet
#endif

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
