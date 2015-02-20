/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
#ifndef __BGRAPH_TRAITS_H
#define __BGRAPH_TRAITS_H
/*******************************************************************************
 FILE         :   bgraph_traits.h
 COPYRIGHT    :   Yakov Markovitch, 2015

 DESCRIPTION  :   Typedefs over boost::graph_traits

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   20 Feb 2015
*******************************************************************************/
/** @file
  Typedefs over boost::graph_traits to make for more concise and clean programming
*******************************************************************************/
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/properties.hpp>
#include <boost/property_map/property_map.hpp>

namespace pcomn {
using namespace bgl = boost ;

/*******************************************************************************
 Graph traits dependent types
*******************************************************************************/
template<class G> using
vertex_descriptor_t     = typename bgl::graph_traits<G>::vertex_descriptor ;

template<class G> using
edge_descriptor_t       = typename bgl::graph_traits<G>::edge_descriptor ;

template<class G> using
adjacency_iterator_t    = typename bgl::graph_traits<G>::adjacency_iterator ;

template<class G> using
out_edge_iterator_t     = typename bgl::graph_traits<G>::out_edge_iterator ;

template<class G> using
in_edge_iterator_t      = typename bgl::graph_traits<G>::in_edge_iterator ;

template<class G> using
edge_iterator_t         = typename bgl::graph_traits<G>::edge_iterator ;

template<class G> using
vertex_iterator_t       = typename bgl::graph_traits<G>::vertex_iterator ;

template<class G> using
directed_category_t     = typename bgl::graph_traits<G>::directed_category ;

template<class G> using
edge_parallel_category_t = typename bgl::graph_traits<G>::edge_parallel_category ;

template<class G> using
traversal_category_t    = typename bgl::graph_traits<G>::traversal_category ;

template<class G> using
vertices_size_type_t    = typename bgl::graph_traits<G>::vertices_size_type ;

template<class G> using
edges_size_type_t       = typename bgl::graph_traits<G>::edges_size_type ;

template<class G> using
degree_size_type_t      = typename bgl::graph_traits<G>::degree_size_type ;

/*******************************************************************************
 Property types
*******************************************************************************/
template<class G> using
edge_property_t     = typename bgl::edge_property_type<G>::type ;

template<class G> using
vertex_property_t   = typename bgl::vertex_property_type<G>::type ;

template<class G> using
graph_property_t    = typename bgl::graph_property_type<G>::type ;

}

#endif /* __BGRAPH_TRAITS_H */
