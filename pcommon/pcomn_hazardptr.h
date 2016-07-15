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

  - hazard_ptr\<Element, Tag\> refers to @em  thread-local hazard_manager\<Tag\>,
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
#include <pcomn_calgorithm.h>
#include <pcommon.h>

#include <memory>
#include <vector>
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
class hazard_ptr ;

template<typename>
class hazard_manager ;

template<unsigned>
class hazard_registry ;

template<unsigned>
class hazard_storage ;

class cleanup_holder ;

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
/** A per-thread hazard pointer registry
*******************************************************************************/
template<unsigned Log2 = 0>
class alignas(PCOMN_CACHELINE_SIZE) hazard_registry {

      static_assert(Log2 <= 3,
                    "The capacity of hazard_registry cannot exceed 62 pointers, "
                    "Log2 template argument is too big (Log2 > 3)") ;

      PCOMN_NONCOPYABLE(hazard_registry) ;
      PCOMN_NONASSIGNABLE(hazard_registry) ;

      template<typename>
      friend class hazard_manager ;
      friend hazard_storage<Log2> ;

      // Container of pending deallocations
      typedef std::vector<cleanup_holder> cleanup_container ;

   public:
      constexpr hazard_registry() = default ;
      ~hazard_registry() = default ;

      /// The maximum count of hazard pointers per thread
      static constexpr size_t capacity()
      {
         return (8 << (int)Log2) - (sizeof _occupied + sizeof _pending_retired)/sizeof(void*) ;
      }

      /// Register a hazard pointer.
      /// No more than max_size pointers may be registered at the same time.
      ///
      /// @param ptr A hazard pointer.
      /// @return Hazard pointer index, which must be passed to unregister_hazard(); on
      /// attempt to register more than max_size pointers, returns 0xBADDCA11.
      ///
      /// @note The returned index is valid only for the calling thread, hazard_ptr
      /// objects can @em not be passed between threads.
      ///
      int register_hazard(const void *ptr)
      {
         NOXCHECK(ptr) ;

         const unsigned slot = bitop::rzcnt(~_occupied) ;
         if (slot >= capacity())
            return HAZARD_BADCALL ;

         _hazard[slot] = ptr ;
         // Memory barrier
         atomic_op::store(&_occupied, _occupied | slotbit(slot), std::memory_order_release) ;
         return slot ;
      }

      /// Unregister a hazard pointer.
      /// @param slot A slot index previously returned from register_hazard() call.
      ///
      void unregister_hazard(int slot)
      {
         const uint64_t slotmask = slotbit(slot) ;
         if ((size_t)slot >= capacity() || !(_occupied & slotmask))
            PCOMN_FAIL("Invalid hazard pointer slot index passed to unregister_hazard") ;

         // The following operations need not be atomic and do not require memory barriers
         _occupied &= ~slotmask ;
         _hazard[slot] = nullptr ;
      }

      const void *hazard(int slot) const
      {
         NOXCHECK((size_t)slot < capacity()) ;
         return _hazard[slot] ;
      }

      static constexpr const hazard_registry zero = {} ;

   private:
      uint64_t             _occupied = 0 ;
      cleanup_container *  _pending_retired = nullptr ; /* Pointers marked for cleanup */
      const void *         _hazard[capacity()] = {} ;   /* Currently marked hazards */

      cleanup_container &pending_retired()
      {
         NOXCHECK(_pending_retired) ;
         return *_pending_retired ;
      }

      static constexpr uint64_t slotbit(size_t pos) { return (uint64_t)1 << pos ; }
} ;

template<unsigned L>
constexpr const hazard_registry<L> hazard_registry<L>::zero ;

/******************************************************************************/
/** Hazard pointer marks a non-null pointer to a node of some lock-free dynamic
 object as intended to be further accessed by the current thread without
 furhter validation.

 @note Objects of this class are movable, enabling to return them from functions,
 but they must @em never be passed between threads.
*******************************************************************************/
template<typename E, typename T = void>
class hazard_ptr {
      PCOMN_NONCOPYABLE(hazard_ptr) ;
      PCOMN_NONASSIGNABLE(hazard_ptr) ;
   public:
      typedef E                        element_type ;
      typedef T                        tag_type ;
      typedef hazard_manager<tag_type> manager_type ;
      typedef typename manager_type::registry_type registry_type ;

      /// The default constructor creates a hazard pointer with nullptr value.
      constexpr hazard_ptr() = default ;
      /// The same as the default constructor.
      constexpr hazard_ptr(nullptr_t, manager_type & = *(manager_type *)nullptr) {}

      hazard_ptr(hazard_ptr &&other) noexcept :
         _registry(other._registry),
         _slot(other._slot)
      {
         other.zero_itself() ;
      }

      template<typename U, typename = std::enable_if_t<std::is_convertible<U *, T *>::value, void>>
      hazard_ptr(hazard_ptr<U, T> &&other) noexcept :
         _registry(other._registry),
         _slot(other._slot)
      {
         other.zero_itself() ;
      }

      /// Mark object at @a ptr as being accessed to prevent its retirement.
      ///
      /// When the pointer is marked as being in a hazard state, the collection of memory
      /// the pointer points to is prevented until the pointer is unmarked by hazard_ptr
      /// destructor or hazard_ptr::reset.
      explicit hazard_ptr(element_type *ptr, manager_type &m = manager()) :
         _registry(&m.registry()),
         _slot(mark_hazard(ptr))
      {}

      /// Load a pointer @and mark it as being at hazard in one atomic operation.
      explicit hazard_ptr(element_type **pptr, manager_type &m = manager()) :
         hazard_ptr(*PCOMN_ENSURE_ARG(pptr), m)
      {
         // The hazard is already marked by the target constructor, check its consistency
         for (element_type *p ; *this && (p = atomic_op::load(pptr, std::memory_order_relaxed)) != get() ; mark_hazard(p))
            unmark_hazard() ;
      }

      explicit hazard_ptr(std::atomic<element_type *> *pptr, manager_type &m = manager()) :
         hazard_ptr(reinterpret_cast<element_type **>(pptr), m)
      {}  ;

      /// Unmark a hazard pointer, inform other threads the object pointed to is no more
      /// in use by the current thread.
      ~hazard_ptr()
      {
         unmark_hazard() ;
      }

      hazard_ptr &operator=(hazard_ptr &&other) noexcept
      {
         move_assign(other) ;
         return *this ;
      }

      template<typename U>
      std::enable_if_t<std::is_convertible<U *, T *>::value, hazard_ptr &>
      operator=(hazard_ptr<U, T> &&other) noexcept
      {
         move_assign(other) ;
         return *this ;
      }

      element_type *get() const
      {
         return static_cast<element_type *>(const_cast<void *>(registry().hazard(_slot))) ;
      }
      element_type *operator->() const { return get() ; }
      element_type &operator*() const { return *get() ; }

      explicit operator bool() const { return !!get() ; }

      friend bool operator==(const hazard_ptr &x, const hazard_ptr &y) { return x.get() == y.get() ; }
      friend bool operator==(element_type *x, const hazard_ptr &y) { return x == y.get() ; }
      friend bool operator==(const hazard_ptr &x, element_type *y) { return y == x ; }
      friend bool operator!=(const hazard_ptr &x, const hazard_ptr &y) { return !(x == y) ; }
      friend bool operator!=(element_type *x, const hazard_ptr &y) { return !(x == y) ; }
      friend bool operator!=(const hazard_ptr &x, element_type *y) { return !(x == y) ; }

      /// Mark the pointer as safe for reclaim.
      /// Since after this call the plain pointer this object has held is eventually
      /// invalid, so no return value.
      void reset() noexcept
      {
         unmark_hazard() ;
         zero_itself() ;
      }

      /// Get the default thread-local hazard pointer manager
      static manager_type &manager()
      {
         return hazard_manager<T>::manager() ;
      }

   private:
      registry_type *_registry = &zero_registry() ;
      int            _slot = 0 ; /* Slot index in hazard registry */

      static constexpr registry_type &zero_registry()
      {
         return *const_cast<registry_type *>(&registry_type::zero) ;
      }

      registry_type &registry() const
      {
         NOXCHECK(_registry) ;
         return *_registry ;
      }

      void zero_itself()
      {
         _registry = &zero_registry() ;
         _slot = 0 ;
      }

      int mark_hazard(element_type *hazard)
      {
         if (hazard)
            return registry().register_hazard(hazard) ;
         zero_itself() ;
         return _slot ;
      }

      void unmark_hazard()
      {
         if (_registry != &registry_type::zero)
            registry().unregister_hazard(_slot) ;
      }

      template<typename U>
      void move_assign(hazard_ptr<U, T> &&other) noexcept
      {
         if (&other == this)
            return ;
         std::swap(_registry, other._registry) ;
         std::swap(_slot, other._slot) ;
         other.reset() ;
      }

} ;

/******************************************************************************/
/**
   Movable, Swappable
*******************************************************************************/
class cleanup_holder {

      template<typename T, typename C>
      struct cleanup_forwarder {
            static_assert(std::is_convertible<C, std::function<void(T*)>>::value,
                          "Pointer cleanup function is not callable with T *") ;

            typedef C cleanup_function ;

            cleanup_forwarder(const cleanup_function &fn) : _cleanup(fn) {}
            cleanup_forwarder(cleanup_function &&fn) : _cleanup(std::move(fn)) {}

            void operator()(void *s) { _cleanup(static_cast<T *>(s)) ; }

         private:
            cleanup_function _cleanup ;
      } ;

   public:
      template<typename T, typename C>
      cleanup_holder(T *data, C &&cleanup_fn) noexcept :
         _data(data),
         _cleanup(cleanup_forwarder<T, C>(std::forward<C>(cleanup_fn)))
      {}

      template<typename T>
      explicit cleanup_holder(T *data) noexcept :
         cleanup_holder(data, std::default_delete<T>())
      {}

      ~cleanup_holder() { cleanup() ; }

      /// The cleanup holder is not copyable, only movable
      cleanup_holder(cleanup_holder &&other) noexcept :
         _data(xchange(other._data, nullptr)),
         _cleanup(std::move(other._cleanup))
      {}

      cleanup_holder &operator=(cleanup_holder &&other) noexcept
      {
         if (&other != this)
         {
            _data = xchange(other._data, nullptr) ;
            _cleanup = std::move(other._cleanup) ;
         }
         return *this ;
      }

      operator void *() const { return _data ; }

      void cleanup()
      {
         if (void * const d = release())
            _cleanup(d) ;
      }

      void *release() { return xchange(_data, nullptr) ; }

      void swap(cleanup_holder &other) noexcept
      {
         using std::swap ;
         swap(_data, other._data) ;
         swap(_cleanup, other._cleanup) ;
      }

   private:
      void *                        _data ;
      std::function<void(void *)>   _cleanup ;
} ;

PCOMN_DEFINE_SWAP(cleanup_holder) ;

/******************************************************************************/
/** Lockless storage for hazard registries.

 Cannot be placed onto the stack, the only method to create hazard_storage object
 is with new_hazard_storage() call.
*******************************************************************************/
template<unsigned Log2 = 0>
class alignas(PCOMN_CACHELINE_SIZE) hazard_storage {

      PCOMN_NONCOPYABLE(hazard_storage) ;
      PCOMN_NONASSIGNABLE(hazard_storage) ;

      template<typename>
      friend class hazard_manager ;

      // Bitmap of free/allocated registries
      typedef basic_bitvector<uint64_t> slots_bitmap ;

   public:
      typedef hazard_registry<Log2> registry_type ;

      /// Constructor function.
      /// The actual constructor is private, to disable placing hazard_storage onto the
      /// stack.
      template<unsigned L>
      friend std::unique_ptr<hazard_storage<L>> new_hazard_storage(unsigned) ;

      void operator delete(void *ptr) { deallocate_storage(ptr) ; }

      /// Get the maximum count of registered threads
      size_t capacity() const { return _slots_map.size() ; }

      /// Allocate an empty hazard registry slot from this storage.
      /// @note thread-safe, lock-free
      registry_type &allocate_slot() ;

      /// Release a hazard registry.
      /// @note thread-safe, wait-free
      void release_slot(registry_type *) ;

   private:
      const slots_bitmap      _slots_map ;  /* Map of free/allocated registry slots */
      registry_type * const   _registries ; /* Registry slots */
      std::atomic<size_t>     _top_alloc ;

   private:
      typedef typename registry_type::cleanup_container cleanup_container ;
      // There must be Bloom filter, actually
      typedef std::vector<const void *> hazard_filter ;

      explicit hazard_storage(unsigned slotcount) ;

      // Allocate memory enough to place sequentially the hazard_storage object, registry
      // slots map data, and registry slots themselves, accounting for alignment.
      void *operator new(size_t sz, int thread_maxcount) { return allocate_storage(thread_maxcount) ; }
      void operator delete(void *ptr, int) { deallocate_storage(ptr) ; }

      // Given requested slots count, get the actual count aligned to slots_bitmap's
      // bitcount granularity.
      static constexpr size_t allocated_slotcount(unsigned requested_count)
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
      static void *allocate_storage(unsigned thread_maxcount) ;

      // Deallocate meory region allocated with allocate(); ignore nullptr
      static void deallocate_storage(void *ptr) ;

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

      registry_type &init_registry(registry_type &slot)
      {
         NOXCHECK(is_slotaddr_valid(&slot)) ;
         // No hazards may be marked at this point
         NOXCHECK(!slot._occupied) ;
         if (!slot._pending_retired)
            slot._pending_retired = new cleanup_container ;
         return slot ;
      }

      void fini_registry(registry_type &slot)
      {
         if (retire_pending(slot.pending_retired(), &slot) == 0)
         {
            // All pending entries are successfully cleaned up
            delete slot._pending_retired ;
            slot._pending_retired = nullptr ;
         }
      }

      bool is_slotaddr_valid(const registry_type *slot) const
      {
         return xinrange(slot, slots(), slots() + capacity()) ;
      }

      // Gather marked hazards from the whole storage except from skip_slot, then call
      // the cleanup functor for each entry in pending for which there is no hazards;
      // remove those entries from pending.
      // Return the count of remaining pending entries.
      // skip_slot may be NULL.
      size_t retire_pending(cleanup_container &pending, registry_type *skip_slot) noexcept ;

      // Flush those pending pointers that are not present in gathered hazards
      static cleanup_container &retire_safe(cleanup_container &pending,
                                            hazard_filter &observed_hazards) ;

      // Append (presumably currently marked) hazard to a filter
      static void note_hazard(const void *hazard, hazard_filter &observed_hazards)
      {
         if (!hazard)
            return ;
         observed_hazards.reserve(32) ;
         observed_hazards.push_back(hazard) ;
      }

      cleanup_container *attempt_inherit_pending(size_t slot_index)
      {
         cleanup_container *inherited = nullptr ;
         if (slots_map().cas(slot_index, false, true, std::memory_order_acquire))
         {
            inherited = xchange((slots() + slot_index)->_pending_retired, nullptr) ;
            slots_map().set(slot_index, false, std::memory_order_release) ;
         }
         return inherited ;
      }

      // Factor out hazard filter manipulation into the set of hazard_storage member
      // function to allow in future to change hazard_filter type (Bloom filter?)
      hazard_filter prepare_hazard_filter()
      {
         return hazard_filter() ;
      }

      // Complete filter building
      static hazard_filter &complete_hazard_filter(hazard_filter &observed_hazards)
      {
         return pcomn::sort(observed_hazards) ;
      }

      static bool is_hasard_observed(void *ptr, hazard_filter &observed_hazards)
      {
         return ptr && std::binary_search(observed_hazards.begin(), observed_hazards.end(), ptr) ;
      }
} ;

/******************************************************************************/
/** The manager of hazard pointers.

 There is a manager per tag per thread.

 @note The @a Tag template parameter is @em not the element type of hazard pointer,
 this is @em tag type instead.
*******************************************************************************/
template<typename Tag>
class hazard_manager {
      template<typename E, typename U>
      friend class hazard_ptr ;

      static constexpr size_t storage_capacity_bits()
      {
         return bitop::ct_log2floor<hazard_traits<Tag>::thread_capacity>::value ;
      }
   public:
      typedef hazard_storage<storage_capacity_bits()> storage_type ;
      typedef typename storage_type::registry_type    registry_type ;

      /// Create a hazard manager and allocate a hazard registry slot in the specified
      /// storage.
      explicit hazard_manager(storage_type &hazards) ;

      ~hazard_manager() ;

      /// Get the hazard registry allocated for this manager.
      registry_type &registry() { return _registry ; }
      /// @overload
      const registry_type &registry() const { return _registry ; }

      /// Get the thread-local hazard pointer manager
      static hazard_manager &manager() ;

      /// Register an object for posponed reclamation and provide reclaiming function.
      template<typename U, typename F>
      std::enable_if_t<std::is_convertible<F, std::function<void(U*)>>::value, void>
      mark_for_cleanup(U *object, F &&reclaimer)
      {
         if (!object)
            return ;
         pending_retired().emplace_back(object, std::forward<F>(reclaimer)) ;
         probably_retire_pending() ;
      }

      /// Register an object for posponed reclamation using std::default_deleter as
      /// a reclaiming function.
      template<typename U>
      void mark_for_cleanup(U *object)
      {
         mark_for_cleanup(object, std::default_delete<U>()) ;
      }

   private:
      storage_type &    _storage ;
      registry_type &   _registry ;

      static std::atomic<storage_type *> _global_storage ; /* Global hazard_storage for tag_type */

   private:
      typedef typename registry_type::cleanup_container cleanup_container ;

      /// Atomically allocate and set _global_storage, if not yet allocated.
      static storage_type &ensure_global_storage() ;

      /// Get the global hazard pointer storage for this tag.
      storage_type &storage() { return _storage ; }
      const storage_type &storage() const { return _storage ; }

      cleanup_container &pending_retired() { return _registry.pending_retired() ; }
      const cleanup_container &pending_retired() const { return _registry.pending_retired() ; }

      void probably_retire_pending()
      {
         if (pending_threshold_exceeded())
            storage().retire_pending(pending_retired(), &registry()) ;
      }

      bool pending_threshold_exceeded() const
      {
         const size_t pending_count = pending_retired().size() ;
         return
            pending_count > 8 &&
            pending_count > storage()._top_alloc.load(std::memory_order_relaxed) ;
      }
} ;

/*******************************************************************************
 hazard_storage
*******************************************************************************/
template<unsigned Log2>
hazard_storage<Log2>::hazard_storage(unsigned requested_slotcount) :

   // Slots map starts immediately past the end of hazard_storage object itself
   _slots_map(static_cast<typename slots_bitmap::element_type *>(raw_data()),
              slots_bitmap::cellndx(allocated_slotcount(requested_slotcount))),

   // Array of registries starts after the slots map, accounting for alignment
   _registries(static_cast<registry_type *>(padd<void>(raw_data(), slotdata_offset(slots_map().size()))))
{
   PCOMN_VERIFY(requested_slotcount > 0) ;
}

/// Constructor function.
/// The actual constructor is private, to disable placing hazard_storage onto the
/// stack.
template<unsigned L>
std::unique_ptr<hazard_storage<L>> new_hazard_storage(unsigned thread_maxcount)
{
   typedef hazard_storage<L> storage ;
   if (!thread_maxcount)
      thread_maxcount = HAZARD_DEFAULT_THREADCOUNT ;

   return std::unique_ptr<storage>(new ((int)thread_maxcount) storage(thread_maxcount)) ;
}

template<unsigned L>
void *hazard_storage<L>::allocate_storage(unsigned thread_maxcount)
{
   NOXCHECK(thread_maxcount != 0) ;
   const size_t slot_count = allocated_slotcount(thread_maxcount) ;
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
void hazard_storage<L>::deallocate_storage(void *ptr)
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
auto hazard_storage<L>::allocate_slot() -> registry_type &
{
   using namespace std ;
   bool failed_attempts ;
   do
   {
      failed_attempts = false ;
      // Search the slots map for 0 bit (i.e. for a free slot position)
      for (auto p = slots_map().begin_positional(false_type()), e = slots_map().end_positional(false_type()) ; p != e ; ++p)
      {
         const size_t freepos = *p ;
         if (slots_map().cas(freepos, false, true, memory_order_acquire))
         {
            atomic_op::check_and_swap(&_top_alloc, [=](size_t old){ return freepos + 1 > old ; }, freepos + 1,
                                      memory_order_acquire) ;
            // Success
            return
               init_registry(_registries[freepos]) ;
         }
         failed_attempts = true ;
      }
   }
   while (failed_attempts) ;
   // No failed attempts and no successes - all slots are occupied
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

   // This operation is allowed to be nonatomic: some arbitrary bit sequences can be
   // transiently recognized as hazard pointers, this is absolutely harmless.
   fini_registry(*slot) ;
   // Atomically mark the slot as free, ensure all observers after this point also see
   // slot memory zeroed away.
   slots_map().set(slotpos, false, std::memory_order_release) ;
}

template<unsigned L>
size_t hazard_storage<L>::retire_pending(cleanup_container &pending, registry_type *skip_slot) noexcept
{
   using namespace std ;

   registry_type *slot = slots() ;
   registry_type *const top = slot + _top_alloc.load(memory_order_relaxed) ;
   hazard_filter observed_hazards = prepare_hazard_filter() ;

   for (auto allocstat = slots_map().begin() ; slot != top ; ++slot, ++allocstat)
   {
      if (slot == skip_slot)
         continue ;

      else if (*allocstat)
         // Gather hazards marked by this registry
         for_each(bitop::bitpos_begin(slot->_occupied),
                  bitop::bitpos_end(slot->_occupied),
                  [&](unsigned slotndx) { note_hazard(slot->hazard(slotndx), observed_hazards) ; }) ;

      else if (slot->_pending_retired)
         // Some pointers remain pending after slot deallocation, help other threads that
         // were unable to retire all its pending pointers when finalizing itself
         if (cleanup_container * const inherited = attempt_inherit_pending(slot - slots()))
         {
            // Not completely exception-safe: adding items for inherited to pending may,
            // in principle, entail bad_alloc; ignore this near zero possibility,
            // terminate if worst comes to worst (retire_pending is noexcept)
            pending.insert(pending.end(),
                           make_move_iterator(inherited->begin()),
                           make_move_iterator(inherited->end())) ;
            delete inherited ;
         }
   }
   complete_hazard_filter(observed_hazards) ;
   return
      retire_safe(pending, observed_hazards).
      size() ;
}

template<unsigned L>
typename hazard_storage<L>::cleanup_container &
hazard_storage<L>::retire_safe(cleanup_container &pending, hazard_filter &observed_hazards)
{
   pending.erase
      (std::partition(pending.begin(), pending.end(), [&](void *p)
      {
         return is_hasard_observed(p, observed_hazards) ;
      }),
      pending.end()) ;

   return pending ;
}

/*******************************************************************************
 hazard_manager
*******************************************************************************/
template<typename T>
std::atomic<typename hazard_manager<T>::storage_type *> hazard_manager<T>::_global_storage ;

template<typename T>
hazard_manager<T>::hazard_manager(storage_type &hazards) :
   _storage(hazards),
   _registry(storage().allocate_slot())
{}

template<typename T>
hazard_manager<T>::~hazard_manager()
{
   storage().release_slot(&_registry) ;
}

template<typename T>
hazard_manager<T> &hazard_manager<T>::manager()
{
   thread_local hazard_manager thread_manager (ensure_global_storage()) ;
   return thread_manager ;
}

template<typename T>
auto hazard_manager<T>::ensure_global_storage() -> storage_type &
{
   storage_type *s = _global_storage.load(std::memory_order_relaxed) ;
   if (!s)
   {
      auto new_s = new_hazard_storage<storage_capacity_bits()>(0) ;
      if (_global_storage.compare_exchange_strong(s, new_s.get(), std::memory_order_release))
         return *new_s.release() ;
   }
   return *s ;
}

} // end of namespace pcomn

#endif /* __PCOMN_HAZARDPTR_H */
