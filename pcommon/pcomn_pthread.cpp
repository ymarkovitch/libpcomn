/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_pthread.cpp
 COPYRIGHT    :   Yakov Markovitch, 2020

 DESCRIPTION  :   pcomn::pthread implementation

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   28 Jan 2020
*******************************************************************************/
#include "pcomn_pthread.h"
#include "pcomn_omanip.h"

#include <stdio.h>

namespace pcomn {

/*******************************************************************************
 pthread
*******************************************************************************/
inline
void pthread::ensure_running(const char *attempted_action)
{
    if (joinable())
        return ;

    throw_exception<std::system_error>
        (std::make_error_code(std::errc::invalid_argument),
         string_cast("Attempt to ", attempted_action, " a pcomn::pthread in non-joinable state")) ;
}

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

pthread::id pthread::start_native_thread(thread_state_ptr &&state_ptr, const strslice &name)
{
    PCOMN_VERIFY(state_ptr) ;

    const auto exec_native_thread_function = [](void *sp) -> void*
    {
        const thread_state_ptr state (static_cast<thread_state *>(sp)) ;
        if (*state->_name)
            set_thread_name(state->_name) ;

        state->run() ;
        return NULL ;
    } ;

    pthread_t phandle {} ;

    strslicecpy(state_ptr->_name, name) ;
    PCOMN_ENSURE_ENOERR(pthread_create(&phandle, nullptr, exec_native_thread_function, state_ptr.get()),
                        "pthread_create") ;

    state_ptr.release() ;

    return id(phandle) ;
}

/*******************************************************************************
 Global functions
*******************************************************************************/
static __noinline void thread_setname(pthread_t pt, const strslice &name)
{
    char namebuf[16] ;
    strslicecpy(namebuf, name) ;

    PCOMN_ENSURE_ENOERR(pthread_setname_np(pt, namebuf), "pthread_setname_np") ;
}

void set_thread_name(const strslice &name)
{
    thread_setname(pthread_self(), name) ;
}

/// Set thread name for specified thread (useful for debugging/monitoring).
void set_thread_name(const std::thread &th, const strslice &name)
{
    if (th.joinable())
        thread_setname((pthread_t)const_cast<std::thread &>(th).native_handle(), name) ;
}

/// Set thread name for specified thread (useful for debugging/monitoring).
void set_thread_name(const pthread &th, const strslice &name)
{
    if (th.joinable())
        thread_setname(th.native_handle(), name) ;
}

std::string get_thread_name() noexcept
{
    char namebuf[16] = {} ;
    pthread_getname_np(pthread_self(), namebuf, sizeof namebuf) ;
    return namebuf ;
}

size_t get_threadcount() noexcept
{
    FILE *status = fopen("/proc/self/status", "r") ;
    if (!status)
        return 1 ;

    unsigned result ;
    int linesize ;

    while (fscanf(status, "Threads: %u\n", &(result = 1)) == 0 &&
           fscanf(status, "%*[^\n]\n%n", &(linesize = 0)) >= 0 &&
           linesize > 0) ;

    fclose(status) ;
    return result ;
}

std::ostream &operator<<(std::ostream &os, const pthread::id &v)
{
    return !v
        ? (os << "NON_RUNNING_PTHREAD")
        : (os << ohex(v.native_handle())) ;
}

} // namespace pcomn
