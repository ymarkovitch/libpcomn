/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_MEMMGR_H
#define __PCOMN_MEMMGR_H
/*******************************************************************************
 FILE         :   pcomn_memmgr.h
 COPYRIGHT    :   Yakov Markovitch, 1995-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Memory management - custom heaps

 CREATION DATE:   28 Apr 1995
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcomn_assert.h>

#include <algorithm>
#include <new>

namespace pcomn {

/*******************************************************************************
                     class PMStdAllocator
*******************************************************************************/
struct PMStdAllocator {
      static void *allocate(size_t size) { return ::operator new(size) ; }
      static void deallocate(void *ptr) { ::operator delete(ptr) ; }
} ;

/*******************************************************************************
                     class PTMMemBlockList
*******************************************************************************/
template<class Alloc = PMStdAllocator>
class PTMMemBlockList {
   private:
      class Node ;
      friend class Node ;

      class Node {

         public:
            // Takes pointer to the next list node
            Node (Node *nxt) : _next(nxt) {}

            // Only list node can allocate new list nodes
            static void *operator new(size_t sz, size_t extra, PTMMemBlockList &blocklist)
            {
               return blocklist._allocator.allocate(sz+extra) ;
            }

            static void operator delete(void *, size_t, PTMMemBlockList &) {}

            Node *_next ;
      } ;

   public:
      typedef Alloc allocator_type ;

      // Constructor. Creates empty memory block list.
      // Parameters:
      //    blocksize   -  size of single block.
      //
      explicit PTMMemBlockList(size_t blocksize = 8192, const allocator_type &a = allocator_type()) :
         _allocator(a),
         _current(NULL),
         _blocksize(blocksize),
         _blockcount(0)
      {
         NOXCHECK(blocksize != 0) ;
      }

      ~PTMMemBlockList()
      {
         free_to() ;
      }

      char *current() const
      {
         return reinterpret_cast<char *>(_current) ;
      }

      unsigned blockcount() const { return _blockcount ; }
      size_t blocksize() const { return _blocksize ; }

      // MSVC++ 7.1 ICEs in release mode on PTMMemBlockList::allocate
      #if PCOMN_WORKAROUND(_MSC_VER, < 1400)
      #pragma optimize("", off)
      #endif
      /// Allocate memory block and add it to the chain of allocated blocks.
      unsigned allocate(size_t minsize)
      {
         _current = new (std::max(minsize, _blocksize), *this) Node(_current) + 1 ;
         return ++_blockcount ;
      }

      void free_to (unsigned bookmark = 0) ;

   private:
      allocator_type _allocator ;
      Node *         _current ;
      const size_t   _blocksize ;
      unsigned       _blockcount ;
} ;


template <class Alloc>
void PTMMemBlockList<Alloc>::free_to(unsigned bookmark)
{
   NOXPRECONDITION(_blockcount >= bookmark) ;

   for (Node *temp ; _blockcount > bookmark ; --_blockcount)
   {
      temp = _current - 1 ;
      _current = temp->_next ;

      temp->~Node() ;
      _allocator.deallocate(temp) ;
   }
}

/*******************************************************************************
                     class PTMMemStack

 Managed memory stack.  Implements mark and release style memory management,
 using the allocator Alloc.
*******************************************************************************/
template <class Alloc> class PTMMarker ;

template<class Alloc = PMStdAllocator>
class PTMMemStack {
   public:
      friend class PTMMarker<Alloc>;

      typedef Alloc allocator_type ;

      explicit PTMMemStack(size_t blksize = 8192, const allocator_type &a = allocator_type()) :
         _data(blksize, a),
         _blkoffs(_data.blocksize())
      {
         NOXCHECK(blksize != 0) ;
      }

      ~PTMMemStack()
      {}

      void *allocate(size_t size)
      {
         size = std::max<size_t>(1, size) ;

         if (size > _data.blocksize() - _blkoffs)
         {
            // We needn't check result of _data.allocate() - if something is wrong,
            // there'll be thrown an exception of type bad_alloc.
            _data.allocate(size) ;
            _blkoffs = 0 ;
         }
         void *temp = _data.current() + _blkoffs ;
         _blkoffs += size ;
         return temp ;
      }

   private:
      PTMMemBlockList<Alloc>  _data ;
      size_t                  _blkoffs ;
} ;

/*******************************************************************************
                     template <class Alloc>
                     class PTMMarker

 Provides the mark for PTMMemStack
*******************************************************************************/
template <class Alloc = PMStdAllocator>
class PTMMarker {
   public:
      typedef PTMMemStack<Alloc> memstack_type ;

      PTMMarker(memstack_type &mstack) :
         _memstack(&mstack),
         _bookmark(mstack._data.blockcount()),
         _blkoffs(mstack._blkoffs)
      {}

      ~PTMMarker()
      {
         if (_memstack)
         {
            NOXCHECK(_bookmark < _memstack->_data.blockcount() ||
                     _bookmark == _memstack->_data.blockcount() &&
                     _blkoffs <= _memstack->_blkoffs) ;
            _memstack->_data.free_to(_bookmark) ;
            _memstack->_blkoffs = _blkoffs ;
         }
      }

      void release() { _memstack = NULL ; }

   private:
      memstack_type *_memstack ;
      const size_t   _bookmark ;
      const size_t   _blkoffs ;
} ;

/******************************************************************************/
/** Managed single-size block allocator (pool) with O(1) allocation/deallocation
 amortized time.

 Allocates blocks of equal size from chunks. When an allocated block is freed, it is
 returned to the chunk; chunks are never freed, so this is a kind of "pool".
*******************************************************************************/
template<class Alloc = PMStdAllocator>
class PTMMemBlocks {
   public:
      typedef Alloc allocator_type ;

      explicit PTMMemBlocks(size_t itemsize, unsigned items_per_chunk = 0,
                            const allocator_type &a = allocator_type()) :

         _freelist(NULL),
         _itemsize(std::max(itemsize, sizeof(void *))),
         _memstack(items_per_chunk
                   ? (items_per_chunk * _itemsize)
                   : std::max((int)(4096/_itemsize), 1) * _itemsize,
                   a)
      {}

      ~PTMMemBlocks()
      {}

      void *allocate(size_t size)
      {
         NOXPRECONDITION(size <= _itemsize) ; (void)size ;

         if (!_freelist)
            return _memstack.allocate(_itemsize) ;
         else
         {
            void *temp = _freelist ;
            _freelist = *(void **)temp ;
            return temp ;
         }
      }

      void deallocate(void *block)
      {
         *(void **)block = _freelist ;
         _freelist = block ;
      }

   protected:
      void *               _freelist ;
      const size_t         _itemsize ;
      PTMMemStack<Alloc>   _memstack ;
} ;

/******************************************************************************/
/** Managed heap (pool) for objects of type T.
*******************************************************************************/
template<typename T, class Alloc = PMStdAllocator>
class PTMemPool {
   public:
      typedef Alloc allocator_type ;

      explicit PTMemPool(unsigned items_per_chunk = 0, const allocator_type &a = allocator_type()) :
         _heap(sizeof(T), items_per_chunk, a)
      {}

      void *allocate() { return _heap.allocate(sizeof(T)) ; }
      void deallocate(void *item) { _heap.deallocate(item) ; }
      void destroy(T *item)
      {
         if (!item)
            return ;
         item->~T() ;
         _heap.deallocate(item) ;
      }
   private:
      PTMMemBlocks<Alloc> _heap ;
} ;

typedef PTMMemBlocks<PMStdAllocator>   PMStdMemBlocks ;
typedef PTMMemBlocks<PMStdAllocator>   PMMemBlocks ;

} // end of namespace pcomn

template<class Alloc>
inline void *operator new(size_t sz, pcomn::PTMMemStack<Alloc>& m)
{
   return m.allocate(sz) ;
}

template<class Alloc>
inline void *operator new(size_t sz, pcomn::PTMMemBlocks<Alloc> &heap)
{
   return heap.allocate(sz) ;
}

template<class Alloc>
inline void operator delete(void *data, pcomn::PTMMemBlocks<Alloc> &heap)
{
   if (data)
      heap.deallocate(data) ;
}

template<typename T, class Alloc>
void *operator new(size_t sz, pcomn::PTMemPool<T, Alloc> &heap)
{
   if (sz != sizeof(T))
      throw std::bad_alloc() ;
   return heap.allocate() ;
}

template<typename T, class Alloc>
inline void operator delete(void *data, pcomn::PTMemPool<T, Alloc> &heap)
{
   if (data)
      heap.deallocate(data) ;
}

#endif /* __PCOMN_MEMMGR_H */
