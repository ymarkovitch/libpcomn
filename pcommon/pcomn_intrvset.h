/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
#ifndef __PCOMN_INTRVSET_H
#define __PCOMN_INTRVSET_H
/*******************************************************************************
 FILE         :   pcomn_intrvset.h
 COPYRIGHT    :   Yakov Markovitch, 2019

 DESCRIPTION  :   Intervals, iterators over interval sequences, and disjoint sets.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   25 Jan 2019
*******************************************************************************/
/** @file Intervals, iterators over interval sequences, and disjoint sets.
*******************************************************************************/
#include <pcomn_utils.h>
#include <pcomn_iterator.h>
#include <pcomn_calgorithm.h>
#include <pcomn_algorithm.h>
#include <pcomn_flgout.h>
#include <pcomn_function.h>
#include <pcomn_integer.h>

namespace pcomn {

/// Interval of indices
typedef unipair<int> ndx_interval ;

/******************************************************************************/
/** Tags for result of interval_combination
*******************************************************************************/
enum IntervalCombination : uint8_t {
    INTRV_UNIVERSAL = 0,                /**< Universal interval */
    INTRV_1         = 1,                /**< The result is from the first interval */
    INTRV_2         = 2,                /**< The result is from the second interval */
    INTRV_BOTH      = INTRV_1|INTRV_2   /**< The result is from intersection */
} ;

/***************************************************************************//**
 An open discrete interval

 open_interval::endpoint() is not included into the interval itself (i.e. this is
 like an end-iterator), so the whole possible values range is T_MIN:(T_MAX-1)
*******************************************************************************/
template<typename T>
struct open_interval {

    PCOMN_STATIC_CHECK(is_integer<T>() ||
                       std::is_pointer<T>() && !std::is_void<std::remove_cv_t<std::remove_pointer_t<T>>>()) ;

    typedef T       value_type ;
    typedef size_t  size_type ;

    typedef count_iterator<value_type> iterator ;
    typedef iterator const_iterator ;

    typedef std::numeric_limits<value_type> boundary_limits ;

    constexpr open_interval() = default ;

    open_interval(value_type startpoint, value_type endpoint) : _data {startpoint, endpoint}
    {
        // This is open_interval invariant; we don't check this in release
        // mode since it is expensive
        NOXCHECK(startpoint <= endpoint) ;
    }

    explicit open_interval(value_type point) : open_interval(point, point + 1) {}

    template<typename U, typename = instance_if_t<std::is_convertible<T, value_type>::value>>
    open_interval(const unipair<U> &range) :
        open_interval(range.first, range.second)
    {}

    template<typename I, I startpoint, I endpoint>
    constexpr open_interval(std::integral_constant<I, startpoint>,
                            std::integral_constant<I, endpoint>) :
        _data{startpoint, endpoint}
    {
        PCOMN_STATIC_CHECK(startpoint <= endpoint) ;
    }

    template<typename I, I point>
    constexpr open_interval(std::integral_constant<I, point> p) :
        open_interval(p, std::integral_constant<I, point+1>())
    {
        PCOMN_STATIC_CHECK(point < std::numeric_limits<I>::max()) ;
    }

    constexpr value_type startpoint() const { return _data[0] ; }
    constexpr value_type endpoint() const { return _data[1] ; }

    /// For a valid interval there must be startpoint() <= endpoint()
    constexpr bool is_valid() const { return startpoint() <= endpoint() ; }

    constexpr bool empty() const { return startpoint() == endpoint() ; }
    explicit constexpr operator bool() const { return !empty() ; }

    constexpr size_type size() const { return endpoint() - startpoint() ; }

    const_iterator begin() const { return const_iterator{startpoint()} ; }
    const_iterator end() const { return const_iterator{endpoint()} ; }

    constexpr const value_type *data() const { return _data ; }

    constexpr bool intersects(const open_interval &other) const
    {
        return !(startpoint() >= other.endpoint() || other.startpoint() >= endpoint()) ;
    }

    constexpr bool contains(const open_interval &inner) const
    {
        return startpoint() <= inner.startpoint() && inner.endpoint() <= endpoint() ;
    }

    friend constexpr bool operator==(const open_interval &x, const open_interval &y)
    {
        return x._data[0] == y._data[0] && x._data[1] == y._data[1] ;
    }

    friend constexpr bool operator<(const open_interval &x, const open_interval &y)
    {
        return x.startpoint() < y.startpoint() ;
    }

private:
    value_type _data[2] = {} ;
} ;

PCOMN_DEFINE_RELOP_FUNCTIONS(constexpr, open_interval) ;

/***************************************************************************//**
 Random-access iterator over sorted sequence of interval boundaries, which
 represents any two adjacent boundaries as an interval
*******************************************************************************/
template<typename T>
struct interval_boundary_iterator :
        std::iterator<std::random_access_iterator_tag, const open_interval<T>> {
public:
    typedef open_interval<T> interval_type ;
    typedef typename interval_type::value_type boundary_type ;

    constexpr interval_boundary_iterator() = default ;

    explicit constexpr interval_boundary_iterator(const boundary_type *boundary) :
        _current(boundary) {}

    constexpr const interval_type *operator->() const { return current() ; }
    constexpr const interval_type &operator*() const { return *current() ; }

    interval_boundary_iterator &operator++()
    {
        NOXCHECK(_current) ;
        ++_current ;
        return *this ;
    }
    interval_boundary_iterator &operator--()
    {
        NOXCHECK(_current) ;
        --_current ;
        return *this ;
    }
    PCOMN_DEFINE_POSTCREMENT_METHODS(interval_boundary_iterator) ;

    interval_boundary_iterator &operator+=(difference_type i)
    {
        NOXCHECK(_current) ;
        _current += i ;
        return *this ;
    }

    interval_boundary_iterator &operator-=(difference_type i)
    {
        NOXCHECK(_current) ;
        _current -= i ;
        return *this ;
    }

    friend bool operator==(const interval_boundary_iterator &x, const interval_boundary_iterator &y)
    {
        return x._current == y._current ;
    }
    friend bool operator<(const interval_boundary_iterator &x, const interval_boundary_iterator &y)
    {
        return x._current < y._current ;
    }
    friend ptrdiff_t operator-(const interval_boundary_iterator &x, const interval_boundary_iterator &y)
    {
        return x._current - y._current ;
    }

private:
    constexpr const interval_type *current() const
    {
        PCOMN_STATIC_CHECK(2*sizeof *_current == sizeof(interval_type)) ;
        return reinterpret_cast<const interval_type *>(_current) ;
    }

private:
    const boundary_type *_current = {} ;
} ;

PCOMN_DEFINE_NONASSOC_ADDOP_FUNCTIONS(, interval_boundary_iterator, ptrdiff_t) ;

PCOMN_DEFINE_RELOP_FUNCTIONS(, interval_boundary_iterator) ;

/***************************************************************************//**
 Combination of two intervals holds information about intersection, union,
 and symmetric difference of two intervals at the same time.
*******************************************************************************/
template<typename T>
class interval_combination {
public:
    typedef open_interval<T>                interval_type ;
    typedef interval_boundary_iterator<T>   iterator ;
    typedef iterator                        const_iterator ;
    typedef typename interval_type::value_type boundary_type ;

    constexpr interval_combination() : _data{}, _size(0), _valid{} {}

    interval_combination(const interval_type &i1, const interval_type &i2) : _valid(0)
    {
        NOXCHECK(i1 && i2) ;

        const boundary_type data[4] = {
            i1.startpoint(), i1.endpoint(),
            i2.startpoint(), i2.endpoint()
        } ;
        unsigned index[] = {0, 1, 2, 3} ;
        // Handcrufted ordering
        // Note that initially data[0] <= data[1] and data[2] <= data[3], due to
        // open_interval invariant
        if (data[2] < data[0])
            std::swap(index[2], index[0]) ;
        if (data[3] < data[1])
            std::swap(index[3], index[1]) ;

        unsigned * const ndxend =
            std::unique(std::begin(index), std::end(index), [&](unsigned x, unsigned y){ return data[x] == data[y] ;}) ;

        NOXCHECK(ndxend != index) ;

        _data[0] = data[index[0]] ;
        _data[1] = data[index[1]] ;
        _data[2] = data[index[2]] ;
        _data[3] = data[index[3]] ;

        _size = ndxend - index ;
        if (_size == 1)
        {
            _tags[0] = INTRV_BOTH ;
            _data[3] = _data[2] = _data[1] ;
        }
        else
        {
            _tags[0] = (IntervalCombination)((index[0] >> 1) + 1) ;
            _tags[1] = (IntervalCombination)((index[3] >> 1) + 1) ;
            if (_size == 2)
                _data[3] = _data[2] ;
            else if (!(index[1] & 1))
                _tags[1] = INTRV_BOTH ;
        }
    }

    /// Get the combination result part
    const interval_type &interval(unsigned ndx) const
    {
        NOXCHECK(ndx < size()) ;
        return *(begin() + ndx) ;
    }

    IntervalCombination tag(unsigned ndx) const
    {
        NOXCHECK(ndx < size()) ;
        return _tags[ndx] ;
    }

    /// Same as interval()
    const interval_type &operator[](unsigned ndx) const
    {
        return interval(ndx) ;
    }

    constexpr size_t size() const { return _size ; }

    constexpr explicit operator bool() const { return !!_valid ; }

    const_iterator begin() const { return const_iterator(_data) ; }
    const_iterator end() const { return const_iterator(_data + size()) ; }

private:
    boundary_type           _data[4] ;
    size_t                  _size ;
    union {
        IntervalCombination _tags[3] ;
        unsigned            _valid ;
    } ;

private:
    void dummy_check() const
    {
        PCOMN_STATIC_CHECK(sizeof _tags <= sizeof _valid) ;
        PCOMN_STATIC_CHECK(2*sizeof *_data == sizeof(interval_type)) ;
    }
} ;

/***************************************************************************//**
 The set of disjoint intervals, which partition a "universal interval"

 All intervals in the set are nonempty
*******************************************************************************/
template<typename T>
class disjoint_partition {
public:
    typedef interval_boundary_iterator<T> iterator ;
    typedef iterator                      const_iterator ;
    typedef open_interval<T>              value_type ;
    typedef open_interval<T>              interval_type ;

    typedef typename interval_type::value_type      boundary_type ;
    typedef typename interval_type::boundary_limits boundary_limits ;
    typedef std::vector<boundary_type>              boundary_vector ;

    disjoint_partition() = default ;

    /// Copy constructor, O(source.size())
    disjoint_partition(const disjoint_partition &source) = default ;

    /// Move constructor, O(1)
    disjoint_partition(disjoint_partition &&other) :
        _universal(other._universal)
    {
        other._boundaries.swap(_boundaries) ;
        other._universal = {} ;
    }

    /// Create a disjoint partition for a set of possibly intersecting open intervals
    /// @note Empty intervals are skipped
    ///
    template<typename InputIterator>
    disjoint_partition(InputIterator begin, InputIterator end) :
        disjoint_partition(std::vector<interval_type>(begin, end), nullptr)
    {}

    /// Create a disjoint partition for a set of possibly intersecting open intervals and
    /// a universal interval
    ///
    /// The following must hold:
    ///   @li !universal.empty()
    ///   @li every interval in (begin, end) is either empty or completely inside @a
    ///   universal
    ///
    /// @param universal A "universal interval", overlapping the whole partition
    /// @note Empty intervals are skipped
    ///
    template<typename InputIterator>
    disjoint_partition(InputIterator begin, InputIterator end, const interval_type &universal) :
        disjoint_partition(std::vector<interval_type>(begin, end), &universal)
    {}

    /// Merge two partitions into a new consolidated partition
    ///
    /// The universal interval of the merged partition must be the same
    ///
    disjoint_partition(const disjoint_partition &p1, const disjoint_partition &p2) ;

    disjoint_partition(std::vector<interval_type> &&source) :
        disjoint_partition(std::move(source), nullptr)
    {}

    disjoint_partition(std::vector<interval_type> &&source, const interval_type &universal) :
        disjoint_partition(std::move(source), &universal)
    {}

    template<typename F, typename=std::enable_if_t<is_callable<F, interval_type(interval_type)>::value>>
    disjoint_partition(disjoint_partition &&source, F interval_convertor) ;

    disjoint_partition(boundary_vector &&) ;

    disjoint_partition &operator=(disjoint_partition &) = default ;
    disjoint_partition &operator=(disjoint_partition &&other)
    {
        disjoint_partition(std::move(other)).swap(*this) ;
        return *this ;
    }

    void swap(disjoint_partition &other)
    {
        other._boundaries.swap(_boundaries) ;
        pcomn_swap(_universal, other._universal) ;
    }

    bool empty() const { return _boundaries.empty() ; }

    /// Get the count of intervals in the partition
    size_t size() const
    {
        NOXCHECK(_boundaries.size() != 1) ;
        return std::max<size_t>(_boundaries.size(), 1) - 1 ;
    }

    const_iterator begin() const { return const_iterator(_boundaries.data()) ; }
    const_iterator end() const { return const_iterator(_boundaries.data() + size()) ; }

    const interval_type &front() const { return *begin() ; }
    const interval_type &back() const
    {
        NOXCHECK(_boundaries.size() >= 2) ;
        return *const_iterator(_boundaries.data() + (_boundaries.size() - 2)) ;
    }

    /// Get an interval at @a ndx
    const interval_type &operator[](size_t ndx) const
    {
        NOXCHECK(ndx < size()) ;
        return *(begin() + ndx) ;
    }

    /// Find an interval overlapping the given coordinate
    ///
    /// @return Iterator pointing to the interval.
    ///
    const_iterator find(boundary_type boundary) const
    {
        return inrange(boundary, universal_interval().startpoint(), universal_interval().endpoint())
            ? const_iterator(std::upper_bound(bbegin(), bend(), boundary) - 1)
            : end() ;
    }

    /// Find a sequence of intervals overlapped by the parameter
    ///
    /// @param overlapping  @em Must lie inside universal_interval(), or be an empty
    /// interval.
    ///
    /// @return Iterator range pointing intervals which are overlapped by the given one.
    /// @note Returns an empty range if and only if @a overlapping is empty
    ///
    unipair<const_iterator> overlapped_range(const interval_type &overlapping) const ;

    /// Same as overlapped_range(), but returns interval of indices instead of iterators
    ///
    ndx_interval overlapped_ndxrange(const interval_type &overlapping) const
    {
        const auto &r = overlapped_range(overlapping) ;
        return {int(r.first - begin()), int(r.second - begin())} ;
    }

    /// Get the "universal interval" overlapping the whole partition
    ///
    /// The universal interval is [front().startpoint(), back().endpoint()) for
    /// a nonempty partition, or an empty interval for an empty partition
    ///
    interval_type universal_interval() const { return _universal ; }

    boundary_type boundary(size_t ndx) const
    {
        NOXCHECK(ndx < _boundaries.size()) ;
        return _boundaries[ndx] ;
    }

    friend bool operator==(const disjoint_partition &x, const disjoint_partition &y)
    {
        return x._boundaries == y._boundaries ;
    }
    friend bool operator!=(const disjoint_partition &x, const disjoint_partition &y)
    {
        return !(x == y) ;
    }

private:
    interval_type   _universal ; /* Universal interval, overlaps the whole partition */
    boundary_vector _boundaries ;

private:
    // If universal is NULL, calculate overlapping interval from source
    disjoint_partition(std::vector<interval_type> &&source, const interval_type *universal) ;

    const boundary_type *bbegin() const { return pbegin(_boundaries) ; }
    const boundary_type *bend() const { return pend(_boundaries) ; }

    static const interval_type &ensure_interval_range(const interval_type &interval,
                                                      const interval_type &universal,
                                                      const char *msg)
    {
        if (!universal.contains(interval))
            throw_interval_range(interval, universal, msg) ;
        return interval ;
    }

    const interval_type &ensure_interval_range(const interval_type &interval, const char *msg) const
    {
        return ensure_interval_range(interval, universal_interval(), msg) ;
    }

    static __noreturn void throw_interval_range(const interval_type &invalid,
                                                const interval_type &universal,
                                                const char *) ;

} ;

PCOMN_DEFINE_SWAP(disjoint_partition) ;

/*******************************************************************************
 Interval algorithms
*******************************************************************************/
template<typename ForwardIterator>
inline
ForwardIterator coalesce_intervals(ForwardIterator first, ForwardIterator last)
{
    return adjacent_coalesce
        (first, last,
         [](const open_interval &x, const open_interval &y)
         {
             return x.endpoint() >= y.startpoint() ;
         },
         [](const open_interval &x, const open_interval &y)
         {
             return open_interval(x.startpoint(), std::max(x.endpoint(), y.endpoint())) ;
         }) ;
}

template<size_t suffix, typename T = int>
inline
open_interval augment_interval(const open_interval &src, const unipair<T> &data)
{
    PCOMN_STATIC_CHECK(std::is_integral<T>::value) ;
    PCOMN_STATIC_CHECK(suffix <= bitsizeof(T)) ;
    PCOMN_STATIC_CHECK(suffix < bitsizeof(boundary_t) - 2) ;

    static constexpr const boundary_t mask = (uint64_t)~boundary_t() << suffix ;

    return {(src.startpoint() << suffix) | (data.first &~ mask),
            (src.endpoint()   << suffix) | (data.second &~ mask)} ;
}

template<size_t suffix, typename T = int>
inline
std::pair<open_interval, unipair<T>> decompose_interval(const open_interval &src)
{
    PCOMN_STATIC_CHECK(std::is_integral<T>::value) ;
    PCOMN_STATIC_CHECK(suffix <= bitsizeof(T)) ;
    PCOMN_STATIC_CHECK(suffix < bitsizeof(boundary_t) - 2) ;

    static constexpr const boundary_t mask    = (uint64_t)~boundary_t() << suffix ;
    static constexpr const unsigned int_shift = bitsizeof(int) - suffix ;

    const T x = (src.startpoint() &~ mask) << int_shift ;
    const T y = (src.endpoint() &~ mask) << int_shift ;

    return {{src.startpoint() >> suffix, src.endpoint() >> suffix},
            {x >> int_shift, y >> int_shift}} ;
}

/// Get the complement of an interval @a subtrahend in the interval @a minuend
///
/// @return If @a subtrahend splits @a minuend in two, both result.first and
/// result.second are nonempty; if completely overlaps, both are empty; otherwise,
/// result.first is nonempty and result.second is empty, no matter which part of @a
/// minuend is overlapped.
template<typename T>
inline
unipair<open_interval<T>> interval_complement(const open_interval<T> &minuend,
                                              const open_interval<T> &subtrahend)
{
    typedef typename open_interval<T>::value_type boundary_type ;

    unipair<unipair<boundary_type>> result {
        {minuend.startpoint(), subtrahend.startpoint()},
        {subtrahend.endpoint(), minuend.endpoint()}
    } ;

    if (result.first.second <= result.first.first)
        if (result.second.first >= result.second.second)
            result = {} ;
        else
        {
            result.first = result.second ;
            result.second.first = result.second.second ;
        }
    else if (result.second.first > result.second.second)
        result.second.first = result.second.second ;

    return result ;
}

/*******************************************************************************
 disjoint_partition
*******************************************************************************/
template<typename T>
template<typename F, typename>
disjoint_partition<T>::disjoint_partition(disjoint_partition &&source, F convert_interval) :
    disjoint_partition(std::move(source))
{
    if (empty())
        return ;

    boundary_vector::iterator dest = _boundaries.begin() ;
    open_interval converted ;
    for (const open_interval &source: *this)
    {
        converted = convert_interval(source) ;
        if (!converted.empty())
            *dest++ = converted.startpoint() ;
    }

    if (dest == _boundaries.begin())
    {
        swap_clear(_boundaries) ;
        _universal = {} ;
    }
    else
    {
        *dest++ = converted.endpoint() ;
        truncate_container(_boundaries, dest).shrink_to_fit() ;
        _universal = {_boundaries.front(), _boundaries.back()} ;
    }
}

/*******************************************************************************
 Debug output
*******************************************************************************/
std::ostream &operator<<(std::ostream &, const open_interval &) ;

std::ostream &operator<<(std::ostream &, const interval_combination &) ;

std::ostream &operator<<(std::ostream &, const disjoint_partition &) ;

} // end of namespace pcomn

PCOMN_DEFINE_ENUM_ELEMENTS(pcomn, IntervalCombination,
                           4,
                           INTRV_UNIVERSAL,
                           INTRV_1,
                           INTRV_2,
                           INTRV_BOTH) ;

#endif /* __PCOMN_INTRVSET_H */
