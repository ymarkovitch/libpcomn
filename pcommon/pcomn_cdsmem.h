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
class concurrent_freestack {
   public:
      concurrent_freestack(block_allocator &alloc, unsigned maxsz) ;
      concurrent_freestack(block_allocator &alloc, const unsigned *maxsz) ;

      block_allocator &allocator() const ;

   private:
      block_allocator &_allocator ;
} ;

/******************************************************************************/
/**
*******************************************************************************/
class freestack_ring final : public block_allocator {
   public:
      freestack_ring(block_allocator &alloc, unsigned ring_size, unsigned stack_maxsz) ;

      block_allocator &allocator() const ;


   protected:
      void *allocate_block() override
      {
         return sys::pagealloc() ;
      }

      void free_block(void *block) override
      {
         return sys::pagefree(block) ;
      }
   private:
      block_allocator &_allocator ;
      unsigned         _stack_maxsz ;
} ;

/*******************************************************************************
 freestack_ring
*******************************************************************************/
inline
void *freestack_ring::allocate_block()
{
   return sys::pagealloc() ;
}

inline
void freestack_ring::free_block(void *block)
{
   return sys::pagefree(block) ;
}

} // end of namespace pcomn

#endif /* __PCOMN_CDSMEM_H */
