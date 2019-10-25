/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_netprefix.cpp
 COPYRIGHT    :   Yakov Markovitch, 2019

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

shortest_netprefix_set::shortest_netprefix_set(std::true_type, std::vector<ipv4_subnet> &&data) :
    _depth(pack_nodes(compile_nodes(prepare_source_data(data), std::array<unsigned,trie_maxdepth>().data()))),

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

std::vector<ipv4_subnet> &shortest_netprefix_set::prepare_source_data(std::vector<ipv4_subnet> &v)
{
    // Sort the source array by subnet_addr() and then retain only unique shortest
    // prefixes, i.e. for every two subnets N1 and N2, if N1 is a prefix of N2, drop N2.

    std::transform(v.begin(), v.end(), v.begin(), std::mem_fn(&ipv4_subnet::subnet)) ;
    pcomn::sort(v) ;
    pcomn::unique(v, [](const ipv4_subnet &x, const ipv4_subnet &y)
    {
        return x.pfxlen() <= y.pfxlen() && x.subnet_addr() == (y.subnet_addr() & x.netmask()) ;
    }) ;

    return v ;
}

shortest_netprefix_set::node_type *
shortest_netprefix_set::append_node(node_type *current_node, uint8_t hexad, bool is_leaf)
{
    NOXCHECK(hexad < 64) ;
    const uint64_t node_bit = 1ULL << hexad ;

    // The bit must be unset
    NOXCHECK((current_node->children_bits()|current_node->leaves_bits()) < node_bit) ;

    current_node->_children[is_leaf] |= node_bit ;

    if (is_leaf)
        return current_node ;

    NOXCHECK(inrange(current_node, pbegin(_nodes), pend(_nodes))) ;

    const unsigned current_node_ndx = current_node - pbegin(_nodes) ;

    node_type *new_node = &extend_container(_nodes, 1).back() ;
    current_node = _nodes.data() + current_node_ndx ;

    const unsigned new_node_offs = new_node - current_node ;

    if (!current_node->_first_child_offs)
        current_node
            ->_first_child_offs = new_node_offs ;
    else
        current_node
            ->child_at_compilation_stage(current_node->children_count())
            ->_next_node_offs = new_node_offs ;

    return new_node ;
}

unsigned *shortest_netprefix_set::compile_nodes(const simple_slice<ipv4_subnet> &source, unsigned *count_per_level)
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

    _nodes.resize(1) ;

    node_type *node = _nodes.data() ;
    unsigned *current_level_count = count_per_level ;

    for (const ipv4_subnet &v: source)
    {
        NOXCHECK(v.pfxlen()) ;

        const unsigned leaf_level = (v.pfxlen() - 1)/6 ;
        const auto addr = v.subnet_addr().ipaddr() ;

        for (unsigned level = 0 ; level <= leaf_level ; ++level)
        {
            const bool is_leaf = level == leaf_level ;
            node = append_node(node, bittuple<6>(addr, level), is_leaf) ;

            *++current_level_count += !is_leaf ;
        }
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
            node_type &packed_node = nodes[*level_offs] ;
            packed_node = *srcnode ;
            packed_node._next_node_offs = 0 ;
            ++*level_offs ;

            if (srcnode->children_bits())
            {
                node_type *child = srcnode->child_at_compilation_stage(0) ;
                packed_node._first_child_offs = *++level_offs ;

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

} // end of namespace pcomn
