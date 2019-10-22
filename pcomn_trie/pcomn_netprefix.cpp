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
typedef triple<uint64_t> shortest_netprefix_node ;

shortest_netprefix_set::shortest_netprefix_set(std::true_type, std::vector<ipv4_subnet> &&data) :
    _nodes(pack_nodes(compile_nodes(prepare_source_data(data))))
{
    PCOMN_STATIC_CHECK((std::is_same_v<node_type, shortest_netprefix_node>)) ;
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

static inline const shortest_netprefix_node *unpacked_next(const shortest_netprefix_node *node)
{
    const uint32_t v = std::get<2>(*node) >> 32 ;
    NOXCHECK(v) ;
    return node + (v - 1) ;
}

static inline shortest_netprefix_node *append_child_bit(shortest_netprefix_node *node, unsigned n)
{
    NOXCHECK(n < 64) ;

    const uint64_t child_bit = 1ULL << n ;
    uint64_t &cbits = std::get<0>(*node) ;

    NOXCHECK(!(cbits & (-child_bit & child_bit))) ;

    cbits |= child_bit ;
    return node ;
}

std::vector<shortest_netprefix_set::node_type>
shortest_netprefix_set::compile_nodes(const simple_slice<ipv4_subnet> &source)
{
    if (source.empty())
        return {} ;

    std::vector<node_type> result ;
    return result ;
}

} // end of namespace pcomn
