/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_CDSMEM_H
#define __PCOMN_CDSMEM_H
/*******************************************************************************
 FILE         :   pcomn_cdsmem.h
 COPYRIGHT    :   Yakov Markovitch, 2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Memory management for concurrent data structures.

 CREATION DATE:   18 Aug 2016
*******************************************************************************/
#include <pcomn_hazardptr.h>
#include <pcomn_atomic.h>
#include <pcomn_integer.h>
#include <pcomn_sys.h>

#include <algorithm>
#include <thread>
#include <cstddef>
#include <new>

namespace pcomn {

/******************************************************************************/
/**
*******************************************************************************/
class block_allocator {
   public:
      virtual ~block_allocator() = default ;

      /// Get the size of allocated blocks
      size_t size() const { return _size ; }

      /// Get the alignment of allocated blocks
      size_t alignment() const { return _alignment ; }

      void *allocate()
      {
         return allocate_block() ;
      }

      void deallocate(void *block)
      {
         if (!block)
            return ;

         PCOMN_THROW_IF((uintptr_t)block & (alignment() - 1), std::invalid_argument,
                        "Pointer %p with invalid alignment passed to the block deallocation method that requires alignment at least %zu",
                        block, alignment()) ;

         free_block(block) ;
      }

   protected:
      explicit block_allocator(size_t sz, size_t align = 0) :
         _size(sz),
         _alignment(align ? align : sz)
      {
         PCOMN_THROW_IF(!_size || !bitop::tstpow2(_alignment) || (_size & (_alignment - 1)), std::invalid_argument,
                        "Invalid size or alignment specified for a block allocator. size:%zu alignment:%zu",
                        size(), alignment()) ;
      }

      virtual void *allocate_block() = 0 ;
      virtual void free_block(void *) = 0 ;

   private:
      const size_t _size ;
      const size_t _alignment ;

      PCOMN_NONASSIGNABLE(block_allocator) ;
} ;

/******************************************************************************/
/**
*******************************************************************************/
class malloc_block_allocator : public block_allocator {
      typedef block_allocator ancestor ;
   public:
      explicit malloc_block_allocator(size_t size) :
         ancestor(!size ? 0 : ((size + (std_align() - 1)) & (std_align() - 1)),
                  std_align())
      {}

      malloc_block_allocator(size_t size, size_t align) :
         ancestor(size, std::max(std_align(), align))
      {}

   protected:
      void *allocate_block() override
      {
         return alignment() == std_align()
            ? malloc(size())
            : sys::alloc_aligned(alignment(), size()) ;
      }

      void free_block(void *block) override
      {
         alignment() == std_align() ? free(block) : sys::free_aligned(block) ;
      }

   private:
      static constexpr size_t std_align() { return alignof(std::max_align_t) ; }
} ;

/******************************************************************************/
/**
*******************************************************************************/
class singlepage_allocator : public block_allocator {
      typedef block_allocator ancestor ;
   public:
      singlepage_allocator() : ancestor(sys::pagesize()) {}

   protected:
      void *allocate_block() override
      {
         return sys::pagealloc() ;
      }

      void free_block(void *block) override
      {
         return sys::pagefree(block) ;
      }
} ;

/******************************************************************************/
/**
*******************************************************************************/
class alignas(PCOMN_CACHELINE_SIZE) concurrent_freestack {

      PCOMN_NONCOPYABLE(concurrent_freestack) ;
      PCOMN_NONASSIGNABLE(concurrent_freestack) ;

      template<typename T>
      using hazard = hazard_ptr<T, concurrent_freestack> ;

      enum : unsigned {
         COUNT_BITS = 24 // Bitsize of the stack item count (16M max)
      } ;
   public:
      explicit concurrent_freestack(unsigned maxsz) :
         _intmaxsz(validate_maxsize(maxsz)),
         _maxsize(_intmaxsz)
      {}

      explicit concurrent_freestack(const unsigned *maxsz) :
         _intmaxsz(0),
         _maxsize(validate_maxsize(*PCOMN_ENSURE_ARG(maxsz)))
      {}

      static constexpr unsigned max_size_limit() { return (1U << COUNT_BITS) - 1 ; }

      unsigned max_size() const { return std::min((unsigned)_maxsize, max_size_limit()) ; }

      size_t size() const
      {
         return atomic_op::load(&_head._counter, std::memory_order_acquire)._count ;
      }

      bool empty() const
      {
         return !atomic_op::load(&_head._top, std::memory_order_acquire) ;
      }

      void *pop() ;
      bool push(void *p) ;

      size_t trim(unsigned sz, block_allocator &allocator)
      {
         const size_t current_sz = size() ;
         size_t trimmed = 0 ;

         if (sz < current_sz)
            for (const size_t diff = current_sz - sz ; trimmed < diff ; ++trimmed)
               ;
         return trimmed ;
      }

   private:
      static const unsigned &validate_maxsize(const unsigned &maxsz)
      {
         return ensure_le<std::length_error>
            (maxsz, max_size_limit(), "Implementation-defined concurrent_freestack maxsize limits exceeded") ;
      }

      // Place both the stack size and top pointer generation into a single 64-bit to
      // allow for atomic insertion/deletion and size increment/decrement.
      struct counter {
            uint64_t _count      : COUNT_BITS ;
            uint64_t _generation : 64 - COUNT_BITS ;
      } ;
      struct block { block *_next ; } ;
      struct alignas(16) head {
            block  *_top ;
            counter _counter ;
      } ;
      PCOMN_STATIC_CHECK(sizeof(counter) == sizeof(uint64_t)) ;
      PCOMN_STATIC_CHECK(sizeof(head) == 16) ;

   private:
      const unsigned            _intmaxsz ;
      const volatile unsigned & _maxsize ;
      head                      _head = {} ;
} ;

/******************************************************************************/
/**
*******************************************************************************/
template<class Pool = concurrent_freestack>
class concurrent_freepool_ring final : public block_allocator {
      typedef block_allocator ancestor ;
   public:
      typedef Pool pool_type ;

      concurrent_freepool_ring(block_allocator &alloc, unsigned free_maxsize, unsigned ring_size = -1) ;
      ~concurrent_freepool_ring() ;

      block_allocator &allocator() const { return _allocator ; }

      unsigned ringsize() const { return _pools_count ; }

      static constexpr unsigned max_ringsize() { return 512 ; }

      unsigned max_size() const { return _pool_maxsz * _pools_count ; }

      void max_size(unsigned newmaxsize)
      {
         atomic_op::store(&_pool_maxsz, calc_pool_maxsz(newmaxsize), std::memory_order_seq_cst) ;
      }

      /// Debug interface
      std::vector<unsigned> pool_sizes() const
      {
         std::vector<unsigned> result (ringsize()) ;
         std::transform(_pools, _pools + ringsize(), result.begin(),
                        [](const pool_type &s) { return s.size() ; }) ;
         return result ;
      }

   protected:
      void *allocate_block() override ;
      void free_block(void *block) override ;

   private:
      block_allocator & _allocator ;
      const unsigned    _pools_count ;
      unsigned          _pool_maxsz ;
      pool_type *       _pools ;

      unsigned next_poolndx() const
      {
         static thread_local unsigned short random_ndx[max_ringsize()] ;
         static thread_local size_t next = max_ringsize() + 1 ;
         static constexpr const size_t mask = max_ringsize() - 1 ;
         if (next > max_ringsize())
            // Generate random index sequence
            ;
         const size_t i = next ;
         next = (next + 1) & mask ;
         return random_ndx[i] & (ringsize() - 1) ;
      }

      unsigned calc_pool_maxsz(unsigned maxsz) const
      {
         return (maxsz + ringsize() - 1) & (ringsize() - 1) ;
      }
} ;

/******************************************************************************/
/**
*******************************************************************************/
template<size_t size, size_t alignment, typename Tag = void, class Pool = concurrent_freestack>
struct concurrent_global_blocks {

      static block_allocator &blocks() ;
      static void *allocate() { return blocks().allocate() ; }
      static void deallocate(void *blk) { blocks().deallocate(blk) ; }
} ;

/*******************************************************************************
 concurrent_freestack
*******************************************************************************/
inline __noinline
void *concurrent_freestack::pop()
{
   head oldhead = {} ;
   do {
      hazard<block> top {&_head._top} ;
      oldhead._top = top.get() ;
      if (!oldhead._top)
         break ;

      counter c = _head._counter ;
      oldhead._counter = c ;
      --c._count ;
      ++c._generation ;
      const head newhead = {oldhead._top->_next, c} ;
      if (atomic_op::cas2_strong(&_head, &oldhead, newhead))
         return oldhead._top ;
   }
   while (oldhead._top) ;

   return nullptr ;
}

inline __noinline
bool concurrent_freestack::push(void *p)
{
   if (!p)
      return false ;

   NOXCHECK(((uintptr_t)p & (sizeof(void*) - 1)) == 0) ;

   block * const newtop = static_cast<block *>(p) ;
   counter c = _head._counter ;

   while (c._count < max_size())
   {
      hazard<block> top {&_head._top} ;

      newtop->_next = top.get() ;

      head oldhead = {newtop->_next, c} ;
      ++c._count ;
      ++c._generation ;
      const head newhead = {newtop, c} ;
      if (atomic_op::cas2_strong(&_head, &oldhead, newhead))
         return true ;
      c = oldhead._counter ;
   }
   return false ;
}

/*******************************************************************************
 concurrent_freepool_ring
*******************************************************************************/
template<typename P>
concurrent_freepool_ring<P>::concurrent_freepool_ring(block_allocator &alloc, unsigned free_maxsize, unsigned ring_size) :
   ancestor(alloc.size(), alloc.alignment()),
   _allocator(alloc),

   // Round up ring size to the power of 2
   _pools_count(bitop::log2ceil
                 (ring_size == (unsigned)-1
                  ? midval(1U, max_ringsize(), std::thread::hardware_concurrency())
                  : ensure_le<std::length_error>(ring_size, max_ringsize(),
                                                 "Implementation-defined freepool ring size limits exceeded"))),

   _pool_maxsz(calc_pool_maxsz(free_maxsize)),
   _pools(ringsize() ? sys::alloc_aligned<pool_type>(ringsize()) : nullptr)
{
   if (ringsize() && !_pools)
      throw_exception<bad_alloc_msg>(sys::strlasterr()) ;

   try {
      std::for_each(_pools, _pools + ringsize(), [this](pool_type &p) { return new (&p) pool_type(&_pool_maxsz) ; }) ;
   }
   catch (...) {
      sys::free_aligned(_pools) ;
      _pools = nullptr ;
      throw ;
   }
}

template<typename P>
concurrent_freepool_ring<P>::~concurrent_freepool_ring()
{
   if (!_pools)
      return ;

   std::for_each(_pools, _pools + ringsize(), destroy_ref<pool_type>) ;
   sys::free_aligned(_pools) ;
}

template<typename P>
void *concurrent_freepool_ring<P>::allocate_block()
{
   return allocator().allocate() ;
}

template<typename P>
void concurrent_freepool_ring<P>::free_block(void *)
{
}

} // end of namespace pcomn

#endif /* __PCOMN_CDSMEM_H */
