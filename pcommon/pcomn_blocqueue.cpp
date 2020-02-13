/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_blocqueue.cpp
 COPYRIGHT    :   Yakov Markovitch, 2020

 DESCRIPTION  :   Blocking concurrent queues.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   13 Feb 2020
*******************************************************************************/
#include "pcomn_blocqueue.h"

namespace pcomn {
/*******************************************************************************
 blocqueue_controller
*******************************************************************************/
void blocqueue_controller::change_capacity(unsigned new_capacity)
{
    ensure_range<std::out_of_range>(new_capacity, (size_t)1, max_capacity(),
                                    "Invalid capactity specified for blocking_queue") ;

    PCOMN_SCOPE_LOCK(caplock, _capmutex) ;
    ensure_open() ;

    const int32_t diff = (int32_t)new_capacity - (int32_t)_capacity ;
    if (!diff)
        return ;

    change_data_capacity(new_capacity) ;

    if (diff > 0)
        // Capacity increased, raise the available empty slots count.
        slots(EMPTY).release(diff) ;

    else
        // Capacity reduced: in principle, we need to take (acquire) the difference
        // from the empty slots but this can block, so we borrow.
        slots(EMPTY).borrow(-diff) ;
}

void blocqueue_controller::close(bool close_both_ends)
{
}

} // end of namespace pcomn
