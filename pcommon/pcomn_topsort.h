/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_TOPSORT_H
#define __PCOMN_TOPSORT_H
/*******************************************************************************
 FILE         :   pcomn_topsort.h
 COPYRIGHT    :   Yakov Markovitch, 2002-2017. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Topological sorting algorithm

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   5 Apr 2002
*******************************************************************************/
#include <vector>
#include <utility>
#include <algorithm>

namespace pcomn {

/*******************************************************************************
                     class topological_sorter
 This class provides means for topological sorting of graph nodes.
 Nodes represented by positive integers. Arcs represented by pairs <int, int>.
 The scheme of work with topological_sorter is as follows:
 - create topological_sorter object;
 - add all the graph arcs using push() method
 - call sort(), which will sort and copy to the output iterator all nodes
   that don't violate partial ordering constraints;
 - to check whether where is some constraints violations (i.e. loops),
   call violations() method, which returns the number of nodes, violating
   constraints;
 - to get violating nodes (i.e. nodes that are involved in loops) call
   get_violations method;
*******************************************************************************/
class topological_sorter {
   public:
      topological_sorter() :
         _watermark(NULL),
         _violations(0)
      {}

      unsigned size() const { return _source.size() ; }

      unsigned violations() const { return _violations ; }

      void push(int predecessor, int successor = -1)
      {
         if (predecessor < 0)
         {
            std::swap(predecessor, successor) ;
            if (predecessor < 0)
               throw std::out_of_range("The node number is out of range") ;
         }
         _set_size(std::max(predecessor, successor)) ;
         if (successor >= 0)
         {
            ++_source[successor].first ;
            _add_predecessor(predecessor, successor) ;
         }
      }

      void push(const std::pair<int, int> &arc)
      {
         push(arc.first, arc.second) ;
      }

      template<class ForwardIterator>
      ForwardIterator sort(ForwardIterator output) ;

      template<class ForwardIterator>
      ForwardIterator get_violations(ForwardIterator begin, ForwardIterator end) const ;

   private:
      std::vector<std::pair<int, int> > _source ;
      std::vector<std::pair<int, int> > _successors ;
      int *_watermark ;
      int  _violations ;

      void _set_size(int appended_item)
      {
         if (++appended_item > (int)size())
            _source.resize(appended_item, std::pair<int, int>(0, -1)) ;
      }

      void _add_predecessor(int predecessor, int successor)
      {
         std::pair<int, int> &item = _source[predecessor] ;
         _successors.push_back(std::pair<int, int>(successor, item.second)) ;
         item.second = _successors.size() - 1 ;
      }

      void _remove_predecessor(int beginning_of_chain)
      {
         for (int successor = _source[beginning_of_chain].second ; successor >= 0 ;)
         {
            const std::pair<int, int> &item = _successors[successor] ;
            successor = item.second ;
            if (!--_source[item.first].first)
               _output(item.first) ;
         }
      }

      void _output(int item) { *_watermark++ = item ; }

      void _outqueue_init()
      {
         int sz = size() ;
         for(int current = 0 ; current < sz ; ++current)
            if (!_source[current].first)
               // No predecessors, output immediately
               _output(current) ;
      }
} ;

template<class ForwardIterator>
ForwardIterator topological_sorter::sort(ForwardIterator output)
{
   unsigned sz = size() ;
   int *outqueue, *bottom ;
   _watermark = bottom = outqueue = new int[sz] ;
   _outqueue_init() ;
   while (bottom != _watermark)
   {
      *output = *bottom ;
      _remove_predecessor(*bottom++) ;
      ++output ;
   }
   _violations = outqueue + sz - bottom ;
   delete [] outqueue ;
   return output ;
}

template<class ForwardIterator>
ForwardIterator topological_sorter::get_violations(ForwardIterator begin, ForwardIterator end) const
{
   for (int vn = _violations, item = 0 ; vn && begin != end ; --vn, ++begin)
   {
      while(!_source[item].first) ++item ;
      *begin = item++ ;
   }
   return begin ;
}

} // end of namespace pcomn

#endif /* __PCOMN_TOPSORT_H */
