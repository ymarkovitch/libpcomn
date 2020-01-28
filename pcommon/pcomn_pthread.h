/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
#ifndef __PCOMN_PTHREAD_H
#define __PCOMN_PTHREAD_H
/*******************************************************************************
 FILE         :   pcomn_pthread.h
 COPYRIGHT    :   Yakov Markovitch, 2020
 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   28 Jan 2020
*******************************************************************************/
/** @file

 Thread class which is (mostly) source-compatible with std::thread but supports
 Linux-pthreads-specifics, like e.g. to specify CPU affinity in the constructor.
*******************************************************************************/
#include "pcomn_platform.h"

#ifndef PCOMN_PL_LINUX
#error pcomn::pthread is supported only on Linux.
#endif

#include "pcomn_syncobj.h"
#include "pcomn_hash.h"

#include <thread>
#include <memory>
#include <pthread.h>

namespace pcomn {

/*******************************************************************************

*******************************************************************************/
class pthread {
    PCOMN_NONCOPYABLE(pthread) ;
    PCOMN_NONASSIGNABLE(pthread) ;
public:
    struct id ;

    pthread() noexcept = default ;
    pthread(pthread &&other) noexcept { swap(other) ; }

    template<typename F, typename... Args,
             typename = disable_if_t<std::is_same<valtype_t<F>, pthread>::value>>
    explicit pthread(F &&callable, Args &&... args) ;

    ~pthread() ;

    bool joinable() const noexcept { return !!_id ; }

    /// @note Move-assigning to itself is safe and no-op.
    pthread &operator=(pthread &&other) noexcept
    {
        finalize() ;
        // Note that after the finalize() call the state is `!joinable()`,
        // which is equivalent to default-constructed state.
        swap(other) ;

        return *this ;
    }

    void swap(pthread &other) noexcept
    {
        std::swap(_id, other._id) ;
        std::swap(_autojoin, other._autojoin) ;
    }

    void join() ;
    void detach() ;

    id get_id() const noexcept { return _id ; }

    /***********************************************************************//**
     Thread identifier.
    ***************************************************************************/
    struct id {
        friend pthread ;
    public:
        constexpr id() noexcept = default ;

        explicit constexpr id(pthread_t ph) noexcept : _handle(ph) {}

        id(const std::thread &th) noexcept :
            id(const_cast<std::thread &>(th).native_handle())
        {}

        id(const pthread &th) noexcept : id(th._id._handle) {}

        constexpr pthread_t native_handle() const { return _handle ; }

        constexpr explicit operator bool() const { return !!_handle ; }

        size_t hash() const { return valhash((uint64_t)_handle) ; }

        friend bool operator==(id x, id y) noexcept
        {
            return x._handle == y._handle ;
        }

        friend bool operator<(id x, id y) noexcept
        {
            return x._handle < y._handle ;
        }

        friend std::ostream &operator<<(std::ostream &, const id &) ;

    private:
        pthread_t _handle = {} ;
    } ;

    /// Get thread's native handle
    pthread_t native_handle() const { return _id._handle ; }

private:
    id      _id ;
    bool    _autojoin = false ;

private:
    // Thread function state
    struct thread_state ;
    typedef std::unique_ptr<thread_state> thread_state_ptr ;

    // If the state is not joinable, no-op.
    // Otherwise, if autojoin is enabled, join; if disabled, aborts the process.
    void finalize() ;
    void ensure_running(const char *attempted_action) ;
    void start_native_thread(thread_state_ptr &&) ;
} ;

/*******************************************************************************
 Global functions
*******************************************************************************/
PCOMN_DEFINE_RELOP_FUNCTIONS(, pthread::id) ;
PCOMN_DEFINE_SWAP(pthread) ;

inline std::ostream &operator<<(std::ostream &os, const pthread::id &v)
{
    if (!v)
        return os << "NON_RUNNING_PTHREAD" ;
    return os << v._handle ;
}

} // end of namespace pcomn

namespace std {
/***************************************************************************//**
 std::hash specialization for pthread::id.
*******************************************************************************/
template<>
struct hash<pcomn::pthread::id> : pcomn::hash_fn_member<pcomn::pthread::id> {} ;

} // end of namespace std

#endif /* __PCOMN_PTHREAD_H */
