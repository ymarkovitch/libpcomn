/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_CDSBASE_H
#define __PCOMN_CDSBASE_H
/*******************************************************************************
 FILE         :   pcomn_cdsbase.h
 COPYRIGHT    :   Yakov Markovitch, 2018-2020. All rights reserved.

 DESCRIPTION  :   Base definintions and helpers for implementing concurrent data
                  structures.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   21 Jun 2016
*******************************************************************************/
/** @file
 Base definintions and helpers for implementing concurrent data structures.
*******************************************************************************/
#include <pcomn_atomic.h>
#include <pcomn_hazardptr.h>
#include <pcomn_meta.h>

#include <memory>

namespace pcomn {

/******************************************************************************/
/** Base class for concurrent nonblocking containers.

 Provides basic allocation logic: controls the object of the allocator class
 it is parameterized with is either stateless (is_empty) or is_always_equal,
 which is necessary as lifetime of the memory allocated for a container element can
 exceed lifetime of the container itself, due to hazard pointers machinery.
*******************************************************************************/
template<typename T, typename Node, typename Allocator = std::allocator<T>>
class concurrent_container : private Allocator::template rebind<Node>::other {
      PCOMN_NONCOPYABLE(concurrent_container) ;
      PCOMN_NONASSIGNABLE(concurrent_container) ;

      // The allocator must be "stateless per container"
      static_assert(
         #ifdef PCOMN_COMPILER_CXX17
         std::allocator_traits<Allocator>::is_always_equal,
         #else
         std::is_empty<Allocator>::value,
         #endif
         "Allocator class must be empty or invariant: "
         "no allocator state per concurrent container object allowed.") ;

   public:
      typedef T value_type ;
   protected:
      typedef Allocator allocator_type ;
      typedef Node      node_type ;

      typedef typename allocator_type::template rebind<node_type>::other   node_allocator_type ;
      typedef std::allocator_traits<node_allocator_type>                   node_allocator_traits ;

      typedef hazard_ptr<node_type> node_hazard_ptr ;
      template<typename D>
      using node_safeptr = std::unique_ptr<node_type, D> ;

      static void delete_node(node_allocator_type &a, node_type *node)
      {
         if (node)
         {
            node_allocator_traits::destroy(a, node) ;
            node_allocator_traits::deallocate(a, node, 1) ;
         }
      }

      void delete_node(node_type *node)
      {
         if (node)
            delete_node(node_allocator(), node) ;
      }
      void destroy_node(node_type *node)
      {
         node_allocator_traits::destroy(node_allocator(), node) ;
      }

      struct node_dealloc : private std::reference_wrapper<node_allocator_type> {
            using std::reference_wrapper<node_allocator_type>::reference_wrapper ;

            void operator()(node_type *node) const
            {
               node_allocator_traits::deallocate(this->get(), node, 1) ;
            }
      } ;
      struct node_deleter : private std::reference_wrapper<node_allocator_type> {
            using std::reference_wrapper<node_allocator_type>::reference_wrapper ;

            void operator()(node_type *node) const { delete_node(this->get(), node) ; }
      } ;

      concurrent_container() = default ;

      /// Get the node allocator for this container.
      node_allocator_type &node_allocator() noexcept
      {
         return *static_cast<node_allocator_type *>(this) ;
      }

      /// Get the hazard manager for this thread
      static typename node_hazard_ptr::manager_type &hazards()
      {
         return node_hazard_ptr::manager() ;
      }

      /// Allocate an empty node for this container.
      node_type *allocate_node()
      {
         return node_allocator().allocate(1) ;
      }

      /// Immediately deallocate a node (but @see retire_node()).
      void deallocate_node(node_type *node)
      {
         node_allocator().deallocate(node, 1) ;
      }

      /// Mark a node for pending deallocation.
      ///
      /// The actual deallocation will take place at some moment after all hazard pointer
      /// to the passed node have been invalidated.
      void retire_node(node_type *node)
      {
         // This node will be actually deallocated some after there are no hazard pointers
         // pointed to it.
         hazards().mark_for_cleanup(node, node_dealloc(node_allocator())) ;
      }

      /// Allocate and construct a node.
      template<typename... Args>
      node_type *make_node(Args &&... args)
      {
         node_allocator_type &allocator = node_allocator() ;
         node_safeptr<node_dealloc> p {allocate_node(), node_dealloc(allocator)} ;

         node_allocator_traits::construct(allocator, p.get(), std::forward<Args>(args)...) ;
         return p.release() ;
      }

      template<typename F>
      node_safeptr<F> node_finalizer(node_type *node, F &&finalizer)
      {
         return node_safeptr<F>(node, std::forward<F>(finalizer)) ;
      }
} ;

/******************************************************************************/
/** The base class of singly-linked list node for various kinds of lockless containers.
*******************************************************************************/
template<typename N>
struct cdsnode_nextptr {
      PCOMN_NONCOPYABLE(cdsnode_nextptr) ;
      PCOMN_NONASSIGNABLE(cdsnode_nextptr) ;

      typedef N node_type ;
      typedef cdsnode_nextptr nextptr_type ;

      constexpr cdsnode_nextptr(node_type *next = nullptr) : _next(next) {}

      node_type *_next ;
} ;

} // end of namespace pcomn

#endif /* __PCOMN_CDSBASE_H */
