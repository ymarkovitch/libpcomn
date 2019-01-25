/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_intrvset.cpp
 COPYRIGHT    :   Yakov Markovitch, 2019

 DESCRIPTION  :   Intervals, iterators over interval sequences, and disjoint sets.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   25 Jan 2019
*******************************************************************************/
#include <pcomn_intrvset.h>
#include <pcomn_calgorithm.h>
#include <pcomn_except.h>

namespace pcomn {

template<typename T>
static inline const open_interval &ensure_valid_interval(const open_interval<T> &interval, const char *msg)
{
    ensure<std::invalid_argument>(interval.is_valid(),
                                  msg, ": ", interval.startpoint(), "..", interval.endpoint()) ;
    return interval ;
}

/*******************************************************************************
 disjoint_partition
*******************************************************************************/
template<typename T>
disjoint_partition<T>::disjoint_partition(std::vector<interval_type> &&source, const interval_type *universal) :

    _universal(universal
               ? ensure_valid_interval(*universal, "Invalid universal interval passed to disjoint_partition constructor")
               : interval_type(boundary_limits::min(), boundary_limits::max()))

{
    // Remove empty intervals and check if nonempty intervals are all inside the universal
    source.erase(std::remove_if(source.begin(), source.end(),
                                [](const interval_type &interval)
                                {
                                    return ensure_valid_interval
                                        (interval, "Invalid interval passed to disjoint_partition constructor")
                                        .empty() ;
                                }),
                 source.end()) ;

    if (source.empty())
    {
        _universal = {} ;
        return ;
    }

    const size_t interval_count = source.size() ;
    // Interpret interval vector as a vector of splitting points in a universal interval
    boundary_type *begin_source = const_cast<boundary_type *>((source.data() + 0)->data()) ;
    boundary_type *end_source =   const_cast<boundary_type *>((source.data() + interval_count)->data()) ;

    // Sort and remove identical splitting points
    std::sort(begin_source, end_source) ;
    end_source = std::unique(begin_source, end_source) ;

    NOXCHECK(begin_source != end_source) ;

    const interval_type overlap (*begin_source, *(end_source - 1)) ;

    if (!universal)
    {
        // The source sequence of intervals implicitly specifies the universal interval
        _universal = overlap ;
        _boundaries = boundary_vector(begin_source, end_source) ;
    }
    else
    {
        ensure_interval_range(overlap, universal_interval(), "The union of source intervals") ;

        const unsigned startp = overlap.startpoint() > universal_interval().startpoint() ;
        const unsigned endp = overlap.endpoint() < universal_interval().endpoint() ;

        _boundaries.reserve(end_source - begin_source + startp + endp) ;
        if (startp)
            _boundaries.push_back(universal_interval().startpoint()) ;
        _boundaries.insert(_boundaries.end(), begin_source, end_source) ;
        if (endp)
            _boundaries.push_back(universal_interval().endpoint()) ;
    }

    NOXCHECK(_boundaries.size() > 1) ;
    NOXCHECK(universal_interval()) ;
    NOXCHECK(interval_type(_boundaries.front(), _boundaries.back()) == universal_interval()) ;
}

template<typename ForwardIterator>
static inline open_interval ensure_valid_partition(ForwardIterator begin, ForwardIterator end)
{
    if (begin == end)
        return {} ;
    size_t count = 1 ;
    const auto sp = *begin ;
    ForwardIterator next = begin ;

    for (++next ; next != end ; ++next, ++begin, ++count)
        ensure<std::invalid_argument>(*begin < *next, "Invalid interval boundaries sequence, prev >= next") ;

    ensure<std::invalid_argument>(count > 1, "Interval boundaries sequence of length 1 is not allowed") ;

    return {sp, *begin} ;
}

template<typename T>
disjoint_partition::disjoint_partition(boundary_vector &&source) :
    _universal(ensure_valid_partition(source.begin(), source.end())),
    _boundaries(std::move(source))
{}

template<typename T>
auto disjoint_partition<T>::overlapped_range(const interval_type &overlapping) const -> unipair<const_iterator>
{
    const boundary_t * const startp =
        std::upper_bound(bbegin(), bend(),
                         ensure_interval_range(overlapping, "Invalid argument to overlapped_range").startpoint()) ;

    return
    {const_iterator(startp - 1), const_iterator(std::lower_bound(startp, bend(), overlapping.endpoint()))} ;
}

template<typename T>
__noreturn __cold
void disjoint_partition<T>::throw_interval_range(const interval_type &invalid, const interval_type &universal,
                                                 const char *msg)
{
    throw_exception<std::invalid_argument>(msg, ": ", invalid, " is out of its universal interval: ", universal) ;
}

/*******************************************************************************
 Debug output
*******************************************************************************/
std::ostream &operator<<(std::ostream &os, const open_interval &v)
{
    os << '[' << v.startpoint() << ':' ;
    if (v)
        os << v.endpoint() ;
    return os << ']' ;
}

std::ostream &operator<<(std::ostream &os, const interval_combination &v)
{
    if (!v)
        return os << "([:])" ;
    os << '(' ;
    for (unsigned i = 0 ; i < v.size() ; ++i)
    {
        if (i)
            os << ' ' ;
        os << oenum(v.tag(i)) << v[i] ;
    }
    return os << ')' ;
}

std::ostream &operator<<(std::ostream &os, const disjoint_partition &v)
{
    return os << v.universal_interval() << ":(" << ocontdelim(v, "") << ')' ;
}

} // end of namespace pcomn
