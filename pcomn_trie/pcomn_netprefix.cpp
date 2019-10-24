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

namespace pcomn {

/*******************************************************************************
 shortest_netprefix_set
*******************************************************************************/
shortest_netprefix_set::shortest_netprefix_set(std::true_type, std::vector<ipv4_subnet> &&data)
{
    compile_nodes(prepare_source_data(data)) ;
    pack_nodes() ;
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
            ->child_at_compilation_stage(bitop::bitcount(current_node->children_bits()))
            ->_next_node_offs = new_node_offs ;

    return new_node ;
}

void shortest_netprefix_set::compile_nodes(const simple_slice<ipv4_subnet> &source)
{
    if (source.empty())
        return ;

    _nodes.resize(1) ;
    if (!source.front().pfxlen())
    {
        // "Star" or "any" subnet: all the leaves are set to 1 at the zero level (all the
        // addresses will match).
        _nodes.front().set_leaves(~uint64_t()) ;
        return ;
    }

    node_type *node = _nodes.data() ;
    for (const ipv4_subnet &v: source)
    {
        NOXCHECK(v.pfxlen()) ;

        const unsigned leaf_level = (v.pfxlen() - 1)/6 ;
        const auto addr = v.subnet_addr().ipaddr() ;

        for (unsigned level = 0 ; level <= leaf_level ; ++level)
        {
            node = append_node(node, bittuple<6>(addr, level), level == leaf_level) ;
        }
    }
}

} // end of namespace pcomn
