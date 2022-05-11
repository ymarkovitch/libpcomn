/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
#pragma once
/*******************************************************************************
 FILE         :   pcomn_fixringbuf.h
 COPYRIGHT    :   Yakov Markovitch, 2022
 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   6 May 2022
*******************************************************************************/
/** @file Power-of-2-sized non-resizable single-threaded ring buffer.
*******************************************************************************/
#include <pcomn_meta.h>
#include <pcomn_bitops.h>

#include <new>
#include <utility>

namespace pcomn {

/***************************************************************************//**
 Power-of-2-sized non-resizable single-threaded ring buffer.

 The capacity of the buffer is specified in the constructor, automatically rounded
 up to the closest power of 2 and cannot be changed.

 No concurrency supported. Simple, fast.

 @tparam T     The type of the ring items: does not require copyablity/movability.
 @tparam Alloc An allocator that is used to acquire the ring memory in the constructor
 and release in the destructor; void (the default) designates using of the global
 operator new().

 @note `Alloc` is *not* used to create/destroy items, the ring buffer always uses
 placement new for construction and std::destroy_at for destruction of items.
*******************************************************************************/
template<typename T, typename Alloc=void>
class fixed_ring_buffer {
public:
    typedef T value_type ;
    typedef std::conditional_t<std::is_void<Alloc>::value, std::allocator<T>, Alloc> allocator_type ;

    /**@{**********************************************************************/
    /** @name Iterators.
     The iterator is a random-access iterator.

     Both the iterator inself and the result of its derefencing are stable with
     respect to any ring modification (of course deleting the item referenced by
     the iterator *does* invalidate the iterator).

     Also provides operator bool, which is false for a default-constructed iterator
     and true otherwise - this particularly means that it is true for non-dereferenceable
     but valid end() iterator.
    ***************************************************************************/
    struct const_iterator {
        typedef std::random_access_iterator_tag iterator_category ;
        typedef T                               value_type ;
        typedef ptrdiff_t                       difference_type ;
        typedef value_type *                    pointer ;
        typedef value_type &                    reference ;

        constexpr const_iterator() noexcept = default ;

        constexpr explicit operator bool() noexcept { return _ring ; }

        reference operator*() const { return _ring->item(_idx) ; }
        pointer operator->() const { return &**this ; }

        const_iterator &operator++() noexcept
        {
            NOXCHECK(_ring) ;
            ++_idx ;
            return *this ;
        }
        const_iterator &operator--() noexcept
        {
            NOXCHECK(_ring) ;
            --_idx ;
            return *this ;
        }

        const_iterator &operator+=(ptrdiff_t diff) noexcept
        {
            NOXCHECK(_ring || !diff) ;
            _idx += diff ;
            return *this ;
        }
        const_iterator &operator-=(ptrdiff_t diff) noexcept
        {
            NOXCHECK(_ring || !diff) ;
            _idx -= diff ;
            return *this ;
        }

        PCOMN_DEFINE_POSTCREMENT_METHODS(const_iterator) ;
        PCOMN_DEFINE_NONASSOC_ADDOP_METHODS(const_iterator, ptrdiff_t) ;

        friend ptrdiff_t operator-(const const_iterator &x, const const_iterator &y) noexcept
        {
            NOXCHECK(x._ring == y._ring) ;
            return x._idx - y._idx ;
        }

        friend bool operator==(const const_iterator &x, const const_iterator &y) noexcept
        {
            NOXCHECK(x._ring == y._ring) ;
            return x._idx == y._idx ;
        }

        friend bool operator<(const const_iterator &x, const const_iterator &y) noexcept
        {
            NOXCHECK(x._ring == y._ring) ;
            return x._idx < y._idx ;
        }

        PCOMN_DEFINE_RELOP_FUNCTIONS(friend, const_iterator) ;

    private:
        const fixed_ring_buffer *_ring = nullptr ;
        uint64_t                 _idx = 0 ;

    private:
        friend fixed_ring_buffer ;

        template<bool is_end>
        const_iterator(const fixed_ring_buffer &ring, std::bool_constant<is_end>) noexcept :
            _ring(&ring),
            _idx(is_end ? ring._pushcnt : ring._popcnt)
        {}
    } ;

    // Non-constant iterator: all the same as a constant, except for dereferencing.
    struct iterator : const_iterator {

        typedef value_type *pointer ;
        typedef value_type &reference ;

        constexpr iterator() noexcept = default ;

        reference operator*() const { return const_cast<reference>(const_iterator::operator*()) ; }
        pointer operator->() const { return const_cast<pointer>(const_iterator::operator->()) ; }

        iterator &operator++() noexcept
        {
            const_iterator::operator++() ;
            return *this ;
        }
        iterator &operator--() noexcept
        {
            const_iterator::operator--() ;
            return *this ;
        }

        iterator &operator+=(ptrdiff_t diff) noexcept
        {
            const_iterator::operator+=(diff) ;
            return *this ;
        }
        iterator &operator-=(ptrdiff_t diff) noexcept
        {
            const_iterator::operator-=(diff) ;
            return *this ;
        }

        PCOMN_DEFINE_POSTCREMENT_METHODS(iterator) ;
        PCOMN_DEFINE_NONASSOC_ADDOP_METHODS(iterator, ptrdiff_t) ;

    private:
        friend fixed_ring_buffer ;

        template<bool is_end>
        iterator(fixed_ring_buffer &ring, std::bool_constant<is_end> isend) noexcept :
            const_iterator(ring, isend)
        {}
    } ;
    /**@}*/

    /***************************************************************************
     Members
    ***************************************************************************/

    /// Create a zero-capacity ring buffer.
    constexpr fixed_ring_buffer() noexcept = default ;

    /// Create a ring buffer with capacity rounded up to power of 2 (or 0 for zero).
    explicit fixed_ring_buffer(size_t capac) ;

    /// A move constructor: the source object becomes zero-capacity.
    /// No (de)allocations or items copy.
    ///
    fixed_ring_buffer(fixed_ring_buffer &&other) noexcept :
        _capacity(std::move(other._capacity)),
        _pushcnt(std::exchange(other._pushcnt, 0)),
        _popcnt(std::exchange(other._popcnt, 0)),
        _data(std::exchange(other._data, nullptr))
    {}

    ~fixed_ring_buffer() ;

    /// Get a pointer to the start of the ring items memory.
    /// Can be used as a ring identifier as it doesn't change since construction until
    /// destruction.
    ///
    /// @note Do *not* dereference, use only as a tag: it is not named `data()` for a
    /// reason.
    const value_type *ringmem() const { return _data ; }

    /// Convert a pointer to supposed ring's item to a ring iterator.
    ///
    /// The passed argument may be any pointer, including pointer to an object which
    /// is not inside the ring, or even nullptr.
    ///
    /// If `item` does not point into the ring, this function returns
    /// a default-constructed iterator, which evaluated as `false` in bool context, so
    /// that `foo.member(bar)` is true if `bar` is inside the ring's memory (but not
    /// necessary valid item).
    ///
    /// If `item` points into the ring *and* points to an item between begin() and
    /// end(), this function returns a dereferenceable iterator.
    ///
    /// Otherwise, returns end().
    ///
    iterator member(value_type *item)
    {
        if (!item | !xinrange(_data, _data + capacity()))
            return {} ;
        const uint64_t i = item - _data ;
        return {} ;
    }

    /// @overload
    const_iterator member(const value_type *item) const
    {
        return as_ptr_mutable(this)->member(as_ptr_mutable(item)) ;
    }

    /// Get the current count of items in the ring.
    constexpr size_t size() const noexcept { return _pushcnt - _popcnt ; }

    /// Get the ring capacity.
    /// @note Always the power of 2 or 0.
    constexpr size_t capacity() const noexcept { return _capacity._capacity_mask + 1 ; }

    /// Indicate if the ring is empty (no items).
    constexpr bool empty() const noexcept { return _pushcnt == _popcnt ; }

    /// Indicate if the ring is full.
    /// @note For zero-capacity ring both empty() and full() are always true.
    constexpr bool full() const noexcept { return size() == capacity() ; }

    const_iterator begin() const noexcept { return cbegin() ; }
    const_iterator end() const noexcept { return cend() ; }

    iterator begin() noexcept { return {*this, std::false_type()} ; }
    iterator end() noexcept { return {*this, std::true_type()} ; }

    const_iterator cbegin() const noexcept { return {*this, std::false_type()} ; }
    const_iterator cend() const noexcept { return {*this, std::true_type()} ; }

    const value_type &front() const noexcept { return item(_popcnt) ; }
    value_type &front() noexcept { return item(_popcnt) ; }

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

    void swap(fixed_ring_buffer &other) noexcept
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
 fixed_ring_buffer
*******************************************************************************/
template<typename T, typename A>
fixed_ring_buffer<T,A>::fixed_ring_buffer(size_t capac) :
    _capacity(capac),
    _data(alloc_traits::allocate(_capacity, capacity()))
{}

template<typename T, typename A>
fixed_ring_buffer<T,A>::~fixed_ring_buffer()
{
    if (const size_t c = capacity())
    {
        std::destroy(begin(), end()) ;
        alloc_traits::deallocate(_capacity, _data, c) ;
    }
}

/*******************************************************************************
 Global function
*******************************************************************************/
PCOMN_DEFINE_SWAP(fixed_ring_buffer<P_PASS(T,A)>, template<typename T, typename A>) ;

} // end of namespace pcomn
