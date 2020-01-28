/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_pthread.cpp
 COPYRIGHT    :   Yakov Markovitch, 2020

 DESCRIPTION  :   pcomn::pthread implementation

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   28 Jan 2020
*******************************************************************************/
#include "pcomn_pthread.h"

namespace pcomn {

/*******************************************************************************
 pthread
*******************************************************************************/
void pthread::join()
{
    ensure_running("join") ;
    PCOMN_ENSURE_ENOERR(pthread_join(native_handle(), NULL), "pthread_join") ;
    // Reset the id, joinable() will be false.
    _id = {} ;
}

void pthread::detach()
{
    ensure_running("detach") ;
    PCOMN_ENSURE_ENOERR(pthread_detach(native_handle()), "pthread_detach") ;
    // Reset the id, joinable() will be false.
    _id = {} ;
}

void pthread::finalize()
{
    if (!joinable())
        return ;

    if (_flags & F_AUTOJOIN)
        join() ;
    else
        PCOMN_FAIL("Attempt to destroy running pcomn::pthread with disabled autojoin") ;
}

pthread::id pthread::start_native_thread(thread_state_ptr &&state_ptr)
{
    PCOMN_VERIFY(state_ptr) ;

    const auto exec_native_thread_function = [](void *sp) -> void*
    {
        const thread_state_ptr state (static_cast<thread_state *>(sp)) ;
        state->run() ;
        return NULL ;
    } ;

    pthread_t phandle {} ;

    PCOMN_ENSURE_ENOERR(pthread_create(&phandle, nullptr, exec_native_thread_function, state_ptr.get()),
                        "pthread_create") ;

    state_ptr.release() ;

    return id(phandle) ;
}

} // namespace pcomn
