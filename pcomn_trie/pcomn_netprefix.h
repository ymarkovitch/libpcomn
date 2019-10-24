/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
#ifndef __PCOMN_NETPREFIX_H
#define __PCOMN_NETPREFIX_H
/*******************************************************************************
 FILE         :   pcomn_netprefix.h
 COPYRIGHT    :   Yakov Markovitch, 2019

 DESCRIPTION  :

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   21 Oct 2019
****************************************************************************//**
 @file
*******************************************************************************/
#include <pcomn_netaddr.h>
#include <pcomn_meta.h>
#include <pcomn_tuple.h>
#include <pcomn_integer.h>
#include <pcomn_vector.h>

namespace pcomn {

/***************************************************************************//**
 A data structure for fast membership check of a network address against
 a set of prefixes.

 @note Does not allow to extract prefix value itself, only to answer membership
 questions ("does the address match any member of the prefix set").
*******************************************************************************/
class shortest_netprefix_set {
public:
    shortest_netprefix_set(const ipv4_subnet *begin, const ipv4_subnet *end) ;

    shortest_netprefix_set() = default ;
    shortest_netprefix_set(shortest_netprefix_set&&) = default ;
    shortest_netprefix_set &operator=(shortest_netprefix_set&&) = default ;

    /// Check is an addr starts with any of the prefixes in the set.
    bool is_member(ipv4_addr addr) const ;

    /// STL set<> interface.
    unsigned count(ipv4_addr addr) const { return is_member(addr) ; }

private:
    // {descendant_array,leaf_array}
    typedef triple<uint64_t> node_type ;

    static constexpr uint64_t children_bits(const node_type &node)
    {
        return std::get<0>(node) ;
    }
    static constexpr uint64_t leaves_bits(const node_type &node)
    {
        return std::get<1>(node) ;
    }

    static const node_type *child(const node_type *node, size_t n)
    {
        NOXCHECK(n < bitop::bitcount(children_bits(*node))) ;
        return node + uint32_t(std::get<2>(*node)) + n ;
    }

private:
    std::vector<node_type> _nodes ;

private:
    shortest_netprefix_set(std::true_type, std::vector<ipv4_subnet> &&data) ;

    std::vector<ipv4_subnet> &prepare_source_data(std::vector<ipv4_subnet> &) ;
    std::vector<node_type> compile_nodes(const simple_slice<ipv4_subnet> &) ;
    std::vector<node_type> pack_nodes(const simple_slice<node_type> &) ;

} ;

/*******************************************************************************
 Global functions
*******************************************************************************/
namespace detail {
template<typename T, unsigned count>
struct bittuple_extract {
    static constexpr unsigned bitsize() { return sizeof(T)*8 ; }
    PCOMN_STATIC_CHECK(is_integer<T>::value && count && count < bitsize()) ;

    typedef unsigned type ;

    static unsigned extract(T v, unsigned ndx, unsigned basepos)
    {
        typedef std::make_unsigned_t<T> mask_type ;

        static constexpr unsigned rbitsize = (bitsize() - count) ;
        static constexpr mask_type mask = ~(uint64_t(-1LL) << count) << rbitsize ;

        const unsigned startpos = basepos + count*ndx ;
        NOXCHECK(startpos < bitsize()) ;
        return mask_type((v << startpos) & mask) >> rbitsize ;
    }
} ;

template<unsigned count>
struct bittuple_extract<binary128_t, count> {
    PCOMN_STATIC_CHECK(count && count < 8) ;

    typedef unsigned type ;

    static unsigned extract(const binary128_t &v, unsigned ndx, unsigned basepos)
    {
        static constexpr unsigned rbitsize = 8 - count ;
        static constexpr unsigned mask = 0xffu >> rbitsize ;

        const unsigned startpos = basepos + count*ndx ;
        NOXCHECK(startpos <= 127) ;
        const uint8_t msb_ndx = startpos/8 ;
        const uint8_t lsb_ndx = (startpos + (count - 1))/8 ;

        if (lsb_ndx >= sizeof(v))
            return uint8_t(v.octet(msb_ndx) << (startpos - (126 - count))) >> rbitsize ;

        const uint16_t word = (v.octet(msb_ndx) << 8) | v.octet(lsb_ndx) ;

        return (word >> (8*msb_ndx + (16 - count) - startpos)) & mask ;
    }
} ;

template<typename T, unsigned count>
using bittuple_extract_t = typename bittuple_extract<T, count>::type ;

} // end of namespace pcomn::detail

/// The family of functions to extract n-bit chunks from 32-bit, 64-bit, and 128-bit
/// values.
/// Facilitate implementation of various tries (prefix trees): critbit, poptrie, qp-trie.
/**{@*/
template<unsigned count, typename T>
inline detail::bittuple_extract_t<T, count> bittuple(T value, unsigned ndx, unsigned basepos)
{
    return detail::bittuple_extract<T, count>::extract(value, ndx, basepos) ;
}

template<unsigned count, typename T>
inline detail::bittuple_extract_t<T, count> bittuple(T value, unsigned ndx)
{
    return bittuple<count>(value, ndx, 0) ;
}
/**}@*/

} // end of namespace pcomn

#endif /* __PCOMN_NETPREFIX_H */
