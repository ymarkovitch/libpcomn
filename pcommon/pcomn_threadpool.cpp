/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_threadpool.cpp
 COPYRIGHT    :   Yakov Markovitch, 2018-2020. All rights reserved.

 DESCRIPTION  :   A resizable threadpool.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   6 Nov 2018
*******************************************************************************/
#include "pcomn_threadpool.h"

namespace pcomn {

/*******************************************************************************
 threadpool
*******************************************************************************/
threadpool::threadpool() = default ;

threadpool::~threadpool() { stop() ; }

unsigned threadpool::clear_queue()
{
    return _task_queue.try_pop_some(std::numeric_limits<int32_t>::max()).size() ;
}

} // end of namespace pcomn
