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
#include "pcomn_function.h"
#include "pcomn_strslice.h"

#include <thread>
#include <memory>
#include <pthread.h>

namespace pcomn {

/***************************************************************************//**
 Represents a POSIX thread with interface compatible with std::thread but allows
 to specify various pthread specifics, like affinity, stack size, etc.

 @note Also allows to automatically rejoin on destruction, like std::jthread,
 depending on construction flags.
*******************************************************************************/
class pthread {
    PCOMN_NONCOPYABLE(pthread) ;
    PCOMN_NONASSIGNABLE(pthread) ;

    template<typename F, typename... Args>
    using valid_callable = std::enable_if_t<is_callable_as<valtype_t<F>, void, Args...>::value> ;

public:
    struct id ;

    enum Flags : unsigned {
        F_DEFAULT   = 0, /**< Default behaviour */
        F_AUTOJOIN  = 1  /**< Automatically rejoin on destruction, if in joinable state */
    } ;

    pthread() noexcept = default ;
    pthread(pthread &&other) noexcept { swap(other) ; }

    template<typename F, typename... Args, typename = valid_callable<F, Args...>>
    pthread(Flags flags, const strslice &name, F &&callable, Args &&... args) ;

    template<typename F, typename... Args, typename = valid_callable<F, Args...>>
    pthread(Flags flags, F &&callable, Args &&... args) :
        pthread(flags, {}, std::forward<F>(callable), std::forward<Args>(args)...)
    {}

    template<typename F, typename... Args, typename = valid_callable<F, Args...>>
    explicit pthread(F &&callable, Args &&... args) :
        pthread(F_DEFAULT, {}, std::forward<F>(callable), std::forward<Args>(args)...)
    {}

    ~pthread() { finalize() ; }

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
        std::swap(_flags, other._flags) ;
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

        static id this_thread() { return id(pthread_self()) ; }

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
    Flags   _flags ;
    id      _id ;

private:
    // Thread function state
    struct thread_state ;
    typedef std::unique_ptr<thread_state> thread_state_ptr ;

    pthread(Flags flags, const strslice &name, thread_state_ptr &&state) :
        _flags(flags),
        _id(start_native_thread(std::move(state), name))
    {}

    // If the state is not joinable, no-op.
    // Otherwise, if autojoin is enabled, join; if disabled, aborts the process.
    void finalize() ;
    void ensure_running(const char *attempted_action) ;

    static id start_native_thread(thread_state_ptr &&, const strslice &name = {}) ;

private:
    struct thread_state {
        virtual void run() = 0 ;
        virtual ~thread_state() = default ;

        char _name[16] = {} ; /* Thread name, if specified */
    } ;

    template<typename F, typename... Args>
    struct state_data final : thread_state {

        template<typename A0, typename... An>
        state_data(A0 &&fn, An &&...args) :
            _function(std::forward<A0>(fn)),
            _args(std::forward<An>(args)...)
        {}

    private:
        ~state_data() final = default ;

        template<size_t... I>
        void invoke_function(std::index_sequence<I...>)
        {
            _function(std::get<I>(_args)...) ;
        }

        void run() final { invoke_function(std::index_sequence_for<Args...>()) ; }

    private:
        F                     _function ;
        std::tuple<Args...>   _args ;
    } ;
} ;

/*******************************************************************************
 pthread
*******************************************************************************/
PCOMN_DEFINE_FLAG_ENUM(pthread::Flags) ;

template<typename F, typename... Args, typename>
pthread::pthread(Flags flags, const strslice &name, F &&callable, Args &&... args) :
    pthread(flags, name, thread_state_ptr(new state_data<std::decay_t<F>, std::decay_t<Args>...>
                                          (std::forward<F>(callable),
                                           std::forward<Args>(args)...)))
{}

/*******************************************************************************
 Global functions
*******************************************************************************/
PCOMN_DEFINE_RELOP_FUNCTIONS(, pthread::id) ;
PCOMN_DEFINE_SWAP(pthread) ;

/// Set current thread name (handy for debugging/monitoring).
void set_thread_name(const strslice &name) ;

/// Set thread name for specified thread (handy for debugging/monitoring).
void set_thread_name(const std::thread &, const strslice &name) ;

/// Set thread name for specified thread (handy for debugging/monitoring).
void set_thread_name(const pthread &, const strslice &name) ;

/// Get the name for the current thread (handy for debugging/monitoring).
std::string get_thread_name() noexcept ;

/// Get the count of threads in the current process.
/// @note The return value is never 0.
size_t get_threadcount() noexcept ;

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
