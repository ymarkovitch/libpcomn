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
        NOXCHECK(n < bitop::bitcount(children_bits(*n))) ;
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
/// The family of functions to extract n-bit chunks from 32-bit, 64-bit, and 128-bit
/// values.
/// Facilitate implementation of various tries (prefix trees): critbit, poptrie, qp-trie.
/**{@*/
template<unsigned count, typename T>
unsigned bittuple(T value, unsigned basepos, unsigned ndx) ;

template<unsigned count, typename T>
inline unsigned bittuple(T value, unsigned ndx)
{
    return bittuple<count,T>(value, 0, ndx) ;
}

template<> inline
unsigned bittuple<6,uint32_t>(uint32_t v, unsigned basepos, unsigned ndx)
{
    const unsigned startpos = basepos + 6U*ndx ;
    NOXCHECK(startpos <= 31) ;
    return ((v << startpos) & 0xfc000000U) >> 26U ;
}

template<> inline
unsigned bittuple<6,uint64_t>(uint64_t v, unsigned basepos, unsigned ndx)
{
    const unsigned startpos = basepos + 6U*ndx ;
    NOXCHECK(startpos <= 63) ;
    return ((v << startpos) & 0xfc00000000000000ULL) >> 58U ;
}

template<> inline
unsigned bittuple<6,binary128_t>(binary128_t v, unsigned basepos, unsigned ndx)
{
    const unsigned startpos = basepos + 6U*ndx ;
    NOXCHECK(startpos <= 127) ;
    const uint8_t msb = startpos/8 ;
    const uint8_t lsb = (startpos + 5)/8 ;

    if (lsb >= sizeof(v))
        return ((v.octet(msb) << (startpos - 120)) & 0xfc) >> 2U ;
    return {} ;
}
/**}@*/

} // end of namespace pcomn

#endif /* __PCOMN_NETPREFIX_H */
