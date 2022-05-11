/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
#pragma once
/*******************************************************************************
 FILE         :   pcomn_memringbuf.h
 COPYRIGHT    :   Yakov Markovitch, 2022
 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   10 May 2022
*******************************************************************************/
/** @file
 Fixed-size single-threaded ring memory buffer.

 @note Currently supports only Linux
*******************************************************************************/
#include "pcomn_platform.h"
#ifndef PCOMN_PL_LINUX
#error This platform does not support pcomn::memory_ring_buffer, it is supported only on Linux.
#endif
#include <pcomn_meta.h>
#include <pcommon.h>

#include <new>

namespace pcomn {

/***************************************************************************//**
 Power-of-2-pages-sized non-resizable single-threaded ring memory buffer with
 guaranteed contiguous suballocation.

 One can (sub)allocate the memory from this buffer at its back and deallocate both
 at the front *and* at the back, i.e. use it both as a memory queue and a stack.

 Due to virtual memory allocation trick, free (yet unallocated) volume can always
 be allocated as a *linear buffer*, i.e. there is no wraparound boundary.

 The buffer memory is allocated by mmap. The capacity of the buffer is specified
 in the constructor and automatically rounded up to the closest cpu_page_size*power_of_2;
 the buffer cannot be resized.

 There are the following member functions to allocate memory from the buffer and
 return it back to the buffer:
    - allocate()
    - dealloc_head()
    - dealloc_tail()

 No concurrency supported. Simple, fast.
*******************************************************************************/
class memory_ring_buffer {
public:
    /// Create a zero-capacity  buffer.
    constexpr memory_ring_buffer() noexcept = default ;

    /// Create a memory buffer with capacity rounded up to power of 2 CPU pages (or 0
    /// for zero).
    ///
    /// The whole allocated capacity is then available for suballocation with
    /// memory_ring_buffer::allocate();  memory_ring_buffer::capacity() will return
    /// the *actual* size of the buffer.
    ///
    /// @note 0->0, 3->4096, 9000->16384, etc.
    ///
    explicit memory_ring_buffer(size_t capacity_bytes) ;

    /// A move constructor: the source object becomes zero-capacity.
    /// No (de)allocation or memory copy.
    ///
    memory_ring_buffer(memory_ring_buffer &&other) noexcept :
        _capacity_mask(std::exchange(other._capacity_mask, 0)),
        _pushoffs(std::exchange(other._pushoffs, 0)),
        _popoffs(std::exchange(other._popoffs, 0)),
        _memory(std::exchange(other._memory, nullptr))
    {}

    ~memory_ring_buffer() ;

    /// Get a pointer to the start of the ring memory.
    /// Can be used as a ring identifier as it doesn't change since construction until
    /// destruction.
    ///
    const void *ringmem() const noexcept { return memory() ; }

    /// Get the size of memory alocated from the ring.
    constexpr size_t allocated_size() const noexcept { return _pushoffs - _popoffs ; }

    /// Get the whole ring capacity, in bytes.
    /// @note Always the power of 2 or 0.
    constexpr size_t capacity() const noexcept { return _capacity_mask + 1 ; }

    /// Get the available ring capacity (bot yet allocated), in bytes.
    constexpr size_t available_capacity() const noexcept
    {
        return capacity() - allocated_size() ;
    }

    /// Indicate if the ring is empty.
    constexpr bool empty() const noexcept { return _pushoffs == _popoffs ; }

    /// Indicate if the ring is full.
    /// @note For zero-capacity ring both empty() and full() are always true.
    constexpr bool full() const noexcept { return allocated_size() == capacity() ; }

    /// Allocate the specified memory amount from the back of the ring.
    /// The allocation is not aligned.
    /// @note Can be seen as "uninitialized push back".
    ///
    void *allocate(size_t bytes)
    {
        if (void * const allocated_start = allocate(bytes, std::nothrow))
            return allocated_start ;
        allocation_failed(bytes) ;
    }

    void *allocate(size_t bytes, const std::nothrow_t &) noexcept
    {
        if (bytes > available_capacity())
            return nullptr ;

        void * const allocated_start = unsafe_memptr(_pushoffs) ;
        _pushoffs += bytes ;
        return allocated_start ;
    }

    /// Deallocate specified count of bytes at the head of the queue.
    /// @return The new head of the queue.
    const void *dealloc_head(size_t dealloc_bytes)
    {
        NOXCHECK(dealloc_bytes < allocated_size()) ;

        _popoffs = std::min(_pushoffs, _popoffs + dealloc_bytes) ;
        return unsafe_memptr(_popoffs) ;
    }

    const void *dealloc_tail(size_t dealloc_bytes)
    {
        NOXCHECK(dealloc_bytes < allocated_size()) ;
        _pushoffs -= std::min(dealloc_bytes, allocated_size()) ;
    }

    void swap(memory_ring_buffer &other) noexcept
    {
        using std::swap ;
        swap(_capacity_mask, other._capacity_mask) ;
        swap(_pushoffs, other._pushoffs) ;
        swap(_popoffs, other._popoffs) ;
        swap(_memory, other._memory) ;
    }

private:
    size_t      _capacity_mask = -1LL ;
    uint64_t    _pushoffs = 0 ;
    uint64_t    _popoffs = 0 ;
    uint8_t *   _memory = nullptr ;

private:
    // Get the address of the start of the ring.
    uint8_t *memory() const noexcept { return _memory ; }

    constexpr size_t ring_pos(size_t offset) const noexcept
    {
        return offset & _capacity_mask ;
    }

    uint8_t *unsafe_memptr(size_t offset) const noexcept
    {
        return memory() + ring_pos(offset) ;
    }

    uint8_t *memptr(size_t offset) const noexcept
    {
        NOXCHECK(!empty()) ;
        return unsafe_memptr(offset) ;
    }

    __noreturn void allocation_failed(size_t bytes) const ;
} ;

/*******************************************************************************
 Global function
*******************************************************************************/
PCOMN_DEFINE_SWAP(memory_ring_buffer) ;

} // end of namespace pcomn
