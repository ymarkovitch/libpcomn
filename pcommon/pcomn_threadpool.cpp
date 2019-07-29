/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_threadpool.cpp
 COPYRIGHT    :   Yakov Markovitch, 2018-2019. All rights reserved.

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

threadpool::~threadpool() { stop(true) ; }

void threadpool::flush_task_queue() { _task_queue = decltype(_task_queue)() ; }

} // end of namespace pcomn
