/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_incdlist.cpp
 COPYRIGHT    :   Yakov Markovitch, 1998-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Doubly-linked inclusive list implementation.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   24 Nov 1998
*******************************************************************************/
#include <pcomn_incdlist.h>

namespace pcomn {
/*******************************************************************************
 PDList::Node
*******************************************************************************/
size_t PDList::Node::count(const Node *start, const Node *finish)
{
   size_t sz ;
   for (sz = 0 ; start != finish ; start = start->next)
      ++sz ;

   return sz ;
}

size_t PDList::Node::desintegrate(Node *start, Node *finish, Node::destructor *dtr)
{
   Node *cur ;
   Node *first = start->prev ;
   size_t cnt = 0 ;

   while(start != finish)
   {
      cur = start ;
      start = cur->next ;
      cur->postremove() ;
      if (dtr)
         dtr(cur) ;
      ++cnt ;
   }
   start->prev = first ;
   first->next = start ;

   return cnt ;
}

size_t PDList::Node::desintegrate(Node *start, size_t n, Node::destructor *dtr)
{
   Node *cur ;
   Node *first = start->prev ;
   size_t cnt = 0 ;

   while(cnt < n  && (cur = start->next) != start)
   {
      start->postremove() ;
      if (dtr)
         dtr(start) ;
      start = cur ;
      ++cnt ;
   }
   start->prev = first ;
   first->next = start ;

   return cnt ;
}

} // end of namespace pcomn
