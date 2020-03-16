/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_netprefix.cpp
 COPYRIGHT    :   Yakov Markovitch, 2019-2020

 DESCRIPTION  :   Network address prefix tables.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   21 Oct 2019
*******************************************************************************/
#include "pcomn_netprefix.h"
#include "pcomn_calgorithm.h"

#include <numeric>

namespace pcomn {

/*******************************************************************************
 shortest_netprefix_set
*******************************************************************************/
static constexpr size_t trie_maxdepth = (256+5)/6 ;

const shortest_netprefix_set::node_type shortest_netprefix_set::_nomatch_root ;
const shortest_netprefix_set::node_type shortest_netprefix_set::_anymatch_root (~uint64_t()) ;

template<typename Subnet>
shortest_netprefix_set::shortest_netprefix_set(std::false_type, const simple_cslice<Subnet> &subnets) :
    shortest_netprefix_set(std::true_type(), std::vector<Subnet>(subnets.begin(), subnets.end()))
{}

template<typename Subnet>
shortest_netprefix_set::shortest_netprefix_set(std::true_type, std::vector<Subnet> &&data) :
    _nodes(),
    _depth(pack_nodes(compile_nodes(make_simple_slice(prepare_source_data(data)),
                                    std::array<unsigned,trie_maxdepth>().data()))),

    _root(_nodes.size() ? _nodes.data() :
          _depth        ? &_anymatch_root :
                          &_nomatch_root)
{
    NOXCHECK(_root != &_anymatch_root || _depth == 1) ;
}

shortest_netprefix_set::shortest_netprefix_set(shortest_netprefix_set &&other) :
    _nodes(std::move(other._nodes)),
    _depth(other._depth),
    _root(other._root)
{
    other._root = &_nomatch_root ;
    other._depth = 0 ;
}

template<typename Subnet>
std::vector<Subnet> &shortest_netprefix_set::prepare_source_data(std::vector<Subnet> &v)
{
    // Sort the source array by subnet_addr() and then retain only unique shortest
    // prefixes, i.e. for every two subnets N1 and N2, if N1 is a prefix of N2, drop N2.

    std::transform(v.begin(), v.end(), v.begin(), std::mem_fn(&Subnet::subnet)) ;
    pcomn::sort(v) ;
    pcomn::unique(v, [](const Subnet &x, const Subnet &y)
    {
        return x.pfxlen() <= y.pfxlen() && x.subnet_addr() == Subnet(y.addr(), x.pfxlen()).subnet_addr() ;
    }) ;

    return v ;
}

shortest_netprefix_set::node_type *
shortest_netprefix_set::append_node(node_type *current_node, uint8_t hexad, uint16_t prefix_bitcount)
{
    NOXCHECK(hexad < 64) ;
    NOXCHECK(prefix_bitcount) ;

    const uint64_t node_bit = 1ULL << hexad ;

    // The bit must be unset in both children and leaves.
    NOXCHECK((current_node->children_bits()|current_node->leaves_bits()) < node_bit) ;

    if (prefix_bitcount <= 6)
    {
        // Leaf node
        const uint8_t children_bitcount = 1ULL << (6 - prefix_bitcount) ;
        NOXCHECK(hexad + children_bitcount <= 64) ;

        current_node->_children[true] |= ((1ULL << children_bitcount) - 1) << hexad ;
        return current_node ;
    }

    NOXCHECK(inrange(current_node, pbegin(_nodes), pend(_nodes))) ;

    current_node->_children[false] |= node_bit ;

    const unsigned current_node_ndx = current_node - pbegin(_nodes) ;

    node_type * const new_node = &extend_container(_nodes, 1).back() ;
    current_node = _nodes.data() + current_node_ndx ;

    if (!current_node->_first_child_offs)
    {
        current_node->_first_child_offs = new_node - current_node ;
    }
    else
    {
        node_type * const last_child =
            current_node->child_at_compilation_stage(current_node->children_count()) ;
        last_child->_next_node_offs = new_node - last_child ;
    }

    return new_node ;
}

template<typename Subnet>
unsigned *shortest_netprefix_set::compile_nodes(const simple_slice<Subnet> &source, unsigned *count_per_level)
{
    memset(count_per_level, 0, trie_maxdepth*sizeof(*count_per_level)) ;

    if (source.empty())
        return count_per_level ;

    // There is always 1 at the zero level.
    *count_per_level = 1 ;

    if (!source.front().pfxlen())
    {
        // "Star" or "any" subnet: all the leaves are set to 1 at the zero level (all the
        // addresses will match).
        return count_per_level ;
    }

    // Insert the empty root, we'll add all the nodes to it.
    _nodes.resize(1) ;

    for (const Subnet &v: source)
    {
        NOXCHECK(v.pfxlen()) ;

        const auto addr = v.subnet_addr() ;

        // Start from the root.
        node_type *node = _nodes.data() ;
        unsigned *current_level_count = count_per_level ;
        unsigned level = 0 ;
        int prefix_tail = v.pfxlen() ;

        do
        {
            node = append_node(node, bittuple<6>(addr, level), prefix_tail) ;
            prefix_tail -= 6 ;

            const bool not_leaf = prefix_tail > 0 ;
            *++current_level_count += not_leaf ;
            ++level ;
        }
        while(prefix_tail > 0) ;
    }
    return count_per_level ;
}

size_t shortest_netprefix_set::pack_nodes(const unsigned *count_per_level)
{
    NOXCHECK(count_per_level[0] <= 1) ;

    const size_t depth =
        std::find(count_per_level, count_per_level + trie_maxdepth, 0) - count_per_level ;

    if (depth < 2)
        // At most one node (the root), no need to pack
        return depth ;

    std::vector<node_type> packed_nodes (_nodes.size()) ;
    unsigned offs_per_level[trie_maxdepth + 1] ;

    std::partial_sum(count_per_level, count_per_level + depth, offs_per_level + 1) ;
    offs_per_level[0] = 0 ;

    NOXCHECK(offs_per_level[depth] == _nodes.size()) ;

    packed_nodes[0] = _nodes[0] ;

    NOXCHECK(packed_nodes.front()._next_node_offs == 0) ;

    struct {
        void put_node(node_type *srcnode, unsigned *level_offs) const
        {
            const unsigned this_ndx = *level_offs ;
            ++*level_offs ;

            node_type &packed_node = nodes[this_ndx] ;
            packed_node = *srcnode ;
            packed_node._next_node_offs = 0 ;

            if (srcnode->children_bits())
            {
                node_type *child = srcnode->child_at_compilation_stage(0) ;

                const unsigned packed_child_ndx = *++level_offs ;
                packed_node._first_child_offs = packed_child_ndx - this_ndx ;

                do put_node(child, level_offs) ;
                while(child = child->sibling_at_compilation_stage()) ;
            }
        }
        node_type *nodes ;

    } local = {packed_nodes.data()} ;

    local.put_node(_nodes.data(), offs_per_level) ;

    std::swap(_nodes, packed_nodes) ;

    return depth ;
}

template<typename Addr>
bool shortest_netprefix_set::is_member(const Addr &addr) const
{
    constexpr unsigned maxlevels = (8*sizeof(addr)+5)/6 ;

    // Start from the root.
    const node_type *node = _root ;
    unsigned level = 0 ;

    do {
        const uint64_t level_bit = 1ULL << bittuple<6>(addr, level) ;

        if (!(node->children_bits() & level_bit))
            return node->leaves_bits() & level_bit ;

        node = node->child(bitop::popcount(node->children_bits() & (level_bit-1))) ;
    }
    while(++level < maxlevels) ;

    PCOMN_DEBUG_FAIL("must never be here") ;
    return false ;
}

/*******************************************************************************
 ipaddr_prefix_set<Addr>
*******************************************************************************/
template<typename Addr>
bool ipaddr_prefix_set<Addr>::is_member(const addr_type &addr) const
{
    return ancestor::is_member(addr) ;
}

/*******************************************************************************
 Explicitly instantiate ipaddr_prefix_set IPv4 and IPv6 variants to ensure
 instantiation of the respective shortest_netprefix_set template members.
*******************************************************************************/
template class ipaddr_prefix_set<ipv4_addr> ;
template class ipaddr_prefix_set<ipv6_addr> ;

} // end of namespace pcomn
