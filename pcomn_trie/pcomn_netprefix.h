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

    shortest_netprefix_set(shortest_netprefix_set &&) ;

    shortest_netprefix_set &operator=(shortest_netprefix_set &&other)
    {
        std::swap(as_mutable(shortest_netprefix_set()), other) ;
        return *this ;
    }

    /// Check is an addr starts with any of the prefixes in the set.
    bool is_member(ipv4_addr addr) const ;

    /// STL set<> interface.
    unsigned count(ipv4_addr addr) const { return is_member(addr) ; }

private:
    // {{descendant_array,leaves_array}, {subnodes_begin,}}
    struct node_type {

        constexpr node_type() = default ;
        explicit constexpr node_type(uint64_t leavebits) :
            _children{0, leavebits}
        {}

        constexpr uint64_t children_bits() const { return _children[0] ; }
        constexpr uint64_t leaves_bits() const { return _children[1] ; }

        uint64_t children_count() const { return bitop::bitcount(children_bits()) ; }

        const node_type *child(size_t n) const
        {
            NOXCHECK(n < bitop::bitcount(children_bits())) ;
            NOXCHECK(_first_child_offs) ;

            return this + _first_child_offs + n ;
        }

        node_type *child_at_compilation_stage(size_t n)
        {
            node_type *head = const_cast<node_type *>(child(0)) ;
            while(n--)
                head += head->_next_node_offs ;
            return head ;
        }

        node_type *sibling_at_compilation_stage()
        {
            node_type * const next = this + _next_node_offs ;
            return _next_node_offs ? next : nullptr ;
        }

        void set_leaves(uint64_t bits) { _children[1] = bits ; }

        uint64_t _children[2] = {} ;      /* Bitarrays for child nodes and leaves */
        uint32_t _first_child_offs = 0 ;
        uint32_t _next_node_offs = 0 ;   /* Relevant only to compilation stage */
    } ;

    static const node_type _nomatch_root ;
    static const node_type _anymatch_root ;

private:
    std::vector<node_type> _nodes ;
    size_t                 _depth = 0 ;
    const node_type *      _root = &_nomatch_root ;

private:
    shortest_netprefix_set(std::true_type, std::vector<ipv4_subnet> &&data) ;

    std::vector<ipv4_subnet> &prepare_source_data(std::vector<ipv4_subnet> &) ;
    unsigned *compile_nodes(const simple_slice<ipv4_subnet> &, unsigned *count_per_level) ;
    // Returns trie depth
    size_t pack_nodes(const unsigned *count_per_level) ;

    node_type *append_node(node_type *current_node, uint8_t hexad, bool is_leaf) ;
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
