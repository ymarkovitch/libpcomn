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
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcomn_meta.h>
#include <pcomn_integer.h>
#include <pcomn_assert.h>
#include <pcomn_atomic.h>
#include <pcommon.h>

#include <memory>

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
/**

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

      explicit hazard_pointer(element_type *ptr) : _ptr(ptr)
      {
         mark_hazard() ;
      }

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
struct alignas(16) hazard_registry {

      static_assert(Log2 <= 3,
                    "The capacity of hazard_registry cannot exceed 63 pointers, "
                    "Log2 template argument is too big (Log2 > 3)") ;

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
class hazard_storage {
   public:
      typedef hazard_registry<Log2> registry_type ;

      explicit hazard_storage(unsigned thread_maxcount = HAZARD_DEFAULT_THREADCOUNT) ;
      ~hazard_storage() ;

      /// Get the maximum count of registered threads
      size_t capacity() const { return _capacity ; }

      /// Allocate a hazard registry.
      /// @note thread-safe
      registry_type *allocate_slot() ;

      /// Release a hazard registry.
      /// @note thread-safe
      void release_slot(registry_type *) ;

   private:
      const size_t _capacity ; /* Capacity of the storage (in hazard_registry objects) */

      const std::unique_ptr<uint64_t[]> _map ; /* Registry slots map */
      std::atomic<size_t>               _map_ubound ;
      registry_type *                   _thread_registries ; /* Registry slots */

      uint64_t *map_data() const { return _map.get() ; }
      size_t map_size() const { return _capacity/sizeof(*map_data()) ; }
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
      typedef hazard_storage<bitop::ct_log2floor<traits_type::thread_capacity>::value> storage_type ;

      /// Get the hazard pointer manager for this thread + tags
      static hazard_manager &manager() ;
      /// Get the global hazard pointer storage for this tag
      static storage_type &storage() { return _storage ; }

   private:
      static storage_type _storage ; /* Global hazard pointer storage for tag_type */
} ;

/*******************************************************************************
 hazard_storage
*******************************************************************************/
template<unsigned Log2>
hazard_storage<Log2>::hazard_storage(unsigned thread_maxcount) :
   _capacity(thread_maxcount)
{}

template<unsigned Log2>
hazard_storage<Log2>::~hazard_storage()
{
   delete [] _thread_registries ;
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
