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

namespace pcomn {

/***************************************************************************//**
 Power-of-2-pages-sized non-resizable single-threaded ring memory buffer with
 guaranteed contiguous suballocation.

 One can (sub)allocate the memory from this buffer at its back and deallocate both
 at the front *and* at the back, i.e. use it both as a memory queue and a stack.
 Due to virtual memory allocation trick, free (yet unallocated) volume can always
 be allocated as a linear buffer, i.e. there is no wraparound boundary.

 The buffer memory is allocated by mmap. The capacity of the buffer is specified
 in the constructor and automatically rounded up to the closest cpu_page_size*power_of_2;
 the buffer cannot be resized.

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
        _pushcnt(std::exchange(other._pushcnt, 0)),
        _popcnt(std::exchange(other._popcnt, 0)),
        _data(std::exchange(other._data, 0))
    {}

    ~memory_ring_buffer() ;

    /// Get a pointer to the start of the ring memory.
    /// Can be used as a ring identifier as it doesn't change since construction until
    /// destruction.
    ///
    const value_type *ringmem() const { return _data ; }

    /// Get the size of memory alocated from the buffer.
    constexpr size_t allocated_size() const noexcept { return _pushcnt - _popcnt ; }

    /// Get the buffer capacity.
    /// @note Always the power of 2 or 0.
    constexpr size_t capacity() const noexcept { return _capacity._capacity_mask + 1 ; }

    /// Indicate if the ring is empty (no items).
    constexpr bool empty() const noexcept { return _pushcnt == _popcnt ; }

    /// Indicate if the ring is full.
    /// @note For zero-capacity ring both empty() and full() are always true.
    constexpr bool full() const noexcept { return size() == capacity() ; }

    const value_type &back() const { return item(_pushcnt - 1) ; }
    value_type &back() { return item(_pushcnt - 1) ; }

    void pop_front()
    {
        std::destroy_at(&front()) ;
        ++_popcnt ;
    }

    void pop_front(size_t count)
    {
        NOXCHECK(count < size()) ;

        while (count--)
            std::destroy_at(unsafe_item(_popcnt++)) ;
    }

    template<typename... Args>
    value_type &emplace_back(Args &&...args)
    {
        NOXCHECK(!full()) ;

        value_type &pushed_value = *new (unsafe_item(_pushcnt)) value_type(std::forward<Args>(args)...) ;
        ++_pushcnt ;
        return pushed_value ;
    }

    value_type &push_back(const value_type &value)
    {
        return emplace_back(value) ;
    }

    value_type &push_back(value_type &value)
    {
        return emplace_back(std::move(value)) ;
    }

    void swap(memory_ring_buffer &other) noexcept
    {
        using namespace std ;
        _capacity.swap(other._capacity) ;
        swap(_pushcnt, other._pushcnt) ;
        swap(_popcnt, other._popcnt) ;
    }

private:
    typedef std::allocator_traits<allocator_type> alloc_traits ;

    struct capacity_data : public allocator_type {
        constexpr capacity_data(size_t capacity = 0) noexcept :
            _capacity_mask(round2z(capacity) - 1)
        {}

        capacity_data(size_t capacity, allocator_type &&alloc) noexcept :
            allocator_type(std::move(alloc)),
            _capacity_mask(round2z(capacity) - 1)
        {}

        capacity_data(capacity_data &&other) noexcept :
            allocator_type(static_cast<allocator_type&&>(other)),
            _capacity_mask(std::exchange(other._capacity_mask, -1LL))
        {}

        void swap(capacity_data &other) noexcept
        {
            using namespace std ;
            swap(*static_cast<allocator_type*>(this), *static_cast<allocator_type*>(&other)) ;
            swap(_capacity_mask, other._capacity_mask) ;
        }

        size_t _capacity_mask = -1LL ;
    } ;
private:
    capacity_data _capacity ;
    uint64_t      _pushcnt = 0 ; /* Overall pushed items count */
    uint64_t      _popcnt = 0 ;  /* Overall popped items count */
    value_type *  _data = nullptr ;

private:

    const value_type *ring_data() const noexcept ;
    value_type *ring_data() noexcept ;

    constexpr size_t ring_pos(size_t idx) const noexcept
    {
        return idx & _capacity._capacity_mask ;
    }

    value_type *unsafe_item(size_t idx) noexcept
    {
        return ring_data() + ring_pos(idx) ;
    }

    value_type &item(size_t idx) noexcept
    {
        NOXCHECK(!empty()) ;
        return *unsafe_item(idx) ;
    }

    const value_type &item(size_t idx) const noexcept
    {
        return as_mutable(*this).item(idx) ;
    }
} ;

/*******************************************************************************
 Global function
*******************************************************************************/
PCOMN_DEFINE_SWAP(memory_ring_buffer) ;

} // end of namespace pcomn
