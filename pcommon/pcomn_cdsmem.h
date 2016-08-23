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
#include <new>

namespace pcomn {

/******************************************************************************/
/**
*******************************************************************************/
class block_allocator {
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

         PCOMN_THROW_IF(static_cast<uintptr_t>(block) & (alignment() - 1), std::invalid_argument,
                        "Pointer %p with invalid alignment passed to the block deallocation method that requires alignment at least %zu",
                        block, alignment) ;

         free_block(block) ;
      }

   protected:
      explicit block_allocator(size_t sz, size_t align = 0) :
         _size(sz),
         _alignment(align ? align : sz)
      {
         PCOMN_THROW_IF(!_size || bitop::bitcount(_alignment) != 1 || (_size & (_alignment - 1)), std::invalid_argument,
                        "Invalid size or alignment specified for a block allocator. size:%zu alignment:zu",
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
         ancestor((size + (std_align() - 1)) & (std_align() - 1), std_align())
      {}

      malloc_block_allocator(size_t size, size_t align) :
         ancestor(size, std::max(std_align(), align))
      {}

   protected:
      void *allocate_block() override
      {
         return alignment() == std_align()
            ? malloc(size())
            : alloc_aligned(alignment(), size()) ;
      }

      void free_block(void *block) override
      {
         #ifndef PCOMN_PL_MS
         free(block) ;
         #else
         alignment() == std_align() ? free(block) : _aligned_free(block) ;
         #endif
      }

   private:
      static constexpr std_align() { return sizeof(std::max_align_t) ; }

      static void *alloc_aligned(size_t align, size_t sz)
      {
         return
          #ifndef PCOMN_PL_MS
          aligned_alloc(align, sz)
          #else
          _aligned_malloc(sz, align)
          #endif
            ;
      }
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
      template<typename T>
      using phazard = hazard_ptr<T, concurrent_freestack> ;

   public:
      explicit concurrent_freestack(unsigned maxsz) :
         _intmaxsz(maxsz),
         _maxsize(_intmaxsz)
      {}

      explicit concurrent_freestack(const unsigned *maxsz) :
         _intmaxsz(0),
         _maxsize(PCOMN_ENSURE_ARG(*maxsz))
      {}

      block_allocator &allocator() const { return _allocator ; }

      size_t size() const
      {
         return atomic_op::load(&_top._counter, std::memory_order_acquire)._size ;
      }

      void *allocate()
      {
         head oldhead = {} ;
         do {
            phazard<block> top {&_head._top} ;
            oldhead._top = top ;
            if (!oldhead._top)
               break ;

            counter c = _head._counter ;
            oldhead._counter = c ;
            --c._size ;
            ++c._generation ;
            const head newhead = {oldhead._top->_next, c} ;
            if (atomic_op::cas2_strong(&_head, &oldhead, newhead))
               return oldhead._ptr ;
         }
         while (oldhead._top) ;

         return nullptr ;
      }

      bool deallocate(void *p)
      {
         if (!p)
            return true ;

         NOXCHECK((static_cast<uintptr_t>(p) & (sizeof(void*) - 1)) == 0) ;

         block * const newtop = static_cast<block *>(p) ;
         counter c = _head._counter ;

         while (c._size < maxsize())
         {
            phazard<block> top {&_head._top} ;

            newtop->_next = top ;

            head oldhead = {newtop->_next, c} ;
            ++c._size ;
            ++c._generation ;
            const head newhead = {newtop, c} ;
            if (atomic_op::cas2_strong(&_head, &oldhead, newhead))
               return true ;
            c = oldhead._counter ;
         }
         return false ;
      }

      size_t trim(unsigned sz, block_allocator &allocator)
      {
         const size_t current_sz = size() ;
         size_t trimmed = 0 ;

         if (sz < current_sz)
            for (const size_t diff = current_sz - sz ; trimmed < diff ; ++trimmed)
               ;
      }

   private:
      struct counter {
            uint64_t _size      :24 ;
            uint64_t _generation:40 ;
      } ;
      struct block {
            block *_next ;
      } ;
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
class freestack_ring final : public block_allocator {
   public:
      freestack_ring(block_allocator &alloc, unsigned ring_size, unsigned free_maxsize) :
         _allocator(alloc),
         _stacks_count(ring_size),
         _stack_maxsz(calc_stack_maxsz(free_maxsize)),
         _stacks(std::allocator<concurrent_freestack>::allocate(_stacks_count))
      {
         for (unsigned i = 0 ; i < _stacks_count ; ++i)
            std::allocator<concurrent_freestack>::construct(&stacks[i], &_stack_maxsz) ;
      }

      block_allocator &allocator() const { return _allocator ; }

      unsigned ringsize() const { return _stacks_count ; }

      static constexpr unsigned maxringsize() { return 512 ; }

      unsigned maxsize() { return _stack_maxsz * _stacks_count ; }

      void maxsize(unsigned newmaxsize)
      {
         atomic_op::store(&_stack_maxsz, calc_stack_maxsz(newmaxsize), std::memory_order_seq_cst) ;
      }

      /// Debug interface
      std::vector<unsigned> stack_sizes() const
      {
         std::vector<unsigned> result (ringsize()) ;
         std::transform(_stacks.get(), _stacks.get() + ringsize(),
                        [](const concurrent_freestack &s) { return s.size() ; }, result.begin()) ;
         return result ;
      }

   protected:
      void *allocate_block() override ;
      void free_block(void *block) override ;

   private:
      block_allocator &_allocator ;
      const unsigned   _stacks_count ;
      unsigned         _stack_maxsz ;
      const std::unique_ptr<concurrent_freestack[]> _stacks ;

      unsigned next_stackndx() const
      {
         static thread_local unsigned short random_ndx[maxringsize()] ;
         static thread_local size_t next = maxringsize() + 1 ;
         static constexpr const size_t mask = maxringsize() - 1 ;
         if (next > maxringsize())
            // Generate random index sequence
            ;
         const size_t i = next ;
         next = (next + 1) & mask ;
         return random_ndx[i] & (ringsize() - 1) ;
      }
} ;

/*******************************************************************************
 freestack_ring
*******************************************************************************/
inline
void *freestack_ring::allocate_block()
{
   return allocator().allocate_block() ;
}

inline
void freestack_ring::free_block(void *block)
{
   return sys::pagefree(block) ;
}

} // end of namespace pcomn

#endif /* __PCOMN_CDSMEM_H */
