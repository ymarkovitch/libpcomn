/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
#ifndef __PCOMN_STACKTRACE_H
#define __PCOMN_STACKTRACE_H
/*******************************************************************************
 FILE         :   pcomn_stacktrace.h
 COPYRIGHT    :   2013 Google Inc.                 All Rights Reserved.
                  2016 Leo Yuriev (leo@yuriev.ru). All Rights Reserved.
                  2017 Yakov Markovitch.           All Rights Reserved.

                  See LICENSE for information on usage/redistribution.
*******************************************************************************/
#include <pcomn_simplematrix.h>
#include <pcomn_strslice.h>
#include <iostream>

namespace pcomn {

class stack_trace ;
class resolved_iptr ;
class iptr_resolver ;

enum InstructionPtrDetails {
    IPTR_FUNCTION,
    IPTR_LOCATION,
    IPTR_FULLINFO
} ;

/***************************************************************************//**
 Object that stores the call stack trace.
*******************************************************************************/
class stack_trace final {
    constexpr static size_t _MAXDEPTH = 32 ;
public:
    typedef void *iptr ;

    explicit stack_trace(const void *addr, int depth = -1) ;
    explicit stack_trace(int depth = -1) : stack_trace(nullptr, depth) {}

    stack_trace(const stack_trace &other) ;

    stack_trace &operator=(const stack_trace &other)
    {
        _thread_id = other._thread_id ;
        _skip = other._skip ;
        _stacktrace = other._stacktrace ;
        _begin = calc_begin(other) ;
        return *this ;
    }

    size_t size() const { return _stacktrace.end() - _begin ; }

    const iptr *begin() const { return _begin ; }
    const iptr *end() const { return _stacktrace.end() ; }

    unsigned thread_id() const { return _thread_id ; }

    constexpr static size_t maxdepth() { return _MAXDEPTH ; }

    resolved_iptr *resolve(resolved_iptr *begin, resolved_iptr *end,
                            InstructionPtrDetails = IPTR_FUNCTION) const ;

private:
    size_t              _thread_id = 0 ;
    size_t              _skip = 0 ;
    static_vector<iptr, _MAXDEPTH + 8> _stacktrace ;
    const iptr *        _begin = _stacktrace.begin() ;

private:
    void skip(size_t levels) { _skip = levels ; }
    void load_thread_info() ;
    void unwind(size_t maxdepth) ;
    const iptr *calc_begin(const stack_trace &other) const
    {
        return _stacktrace.begin() + (other._begin - other.begin()) ;
    }
} ;

/*******************************************************************************
 resolved_iptr

 @note PC is the value of the Program Counter register.
*******************************************************************************/
class resolved_iptr final {
    PCOMN_NONCOPYABLE(resolved_iptr) ;
    PCOMN_NONASSIGNABLE(resolved_iptr) ;
    friend iptr_resolver ;
public:
    static const size_t NAMES_MAXMEM = 4*KiB ;

    struct source_loc {
        friend resolved_iptr ;
        friend iptr_resolver ;

        constexpr source_loc() = default ;

        constexpr const strslice &function() const { return _function ; }
        constexpr const strslice &filename() const { return _filename ; }
        constexpr unsigned line() const { return _line ; }

        constexpr explicit operator bool() const { return !!_function ; }
        constexpr bool has_sourcefile_info() const { return !!_filename ; }

    private:
        strslice _function ;
        strslice _filename ;
        unsigned _line = 0 ;
    } ;

    /// Create an empty object with nullptr iptr and empty source location (all names
    /// are empty, line number is 0).
    resolved_iptr() = default ;

    /// Resolve a iptr for a specified program counter value.
    ///
    /// If @a pc is valid (points inside the code of some function), at least
    /// object_function() will be resolved.
    ///
    /// @note It is safe to pass invalid PC, including NULL.
    ///
    resolved_iptr(stack_trace::iptr pc, InstructionPtrDetails detail_level) ;

    stack_trace::iptr addr() const { return _iptr ; }
    const strslice &object_filename() const { return _object_filename ; }
    const strslice &object_function() const { return _object_function ; }
    const source_loc &source() const { return _source ; }

    bool is_resolved() const { return !!object_function() ; }

    explicit operator bool() const { return !!addr() ; }

    strslice &object_filename(const strslice &newname)
    {
        return init_member(_object_filename, newname) ;
    }

    strslice &object_function(const strslice &newname)
    {
        return init_member(_object_function, newname) ;
    }

    strslice &source_location(const strslice &new_filename, unsigned line)
    {
        _source._line = line ;
        return init_member(_source._filename, new_filename) ;
    }

    strslice &source_function(const strslice &newname)
    {
        return init_member(_source._function, newname) ;
    }

    resolved_iptr &reset(stack_trace::iptr pc) ;

    resolved_iptr &reset() { return reset(_iptr) ; }

private:
    stack_trace::iptr _iptr = nullptr ;

    strslice _object_filename ; /* The object file with the function PC points to. */

    strslice _object_function ; /* The non-inlined function PC is in; not necessarily
                                   the same as the source function, which can be inlined. */
    source_loc _source ; /* The source file location; the filename can be empty and
                            line/column can be 0 if this information can't be deduced,
                            e.g. there is no debug information in the binary object. */

    static_vector<source_loc, 8> _inliners ; /* The list of optional "inliners": the
                                                source locations where the location the
                                                trace PC points to is inlined from. */
    bufstr_ostream<NAMES_MAXMEM>  _memory ;

private:
    strslice &init_member(strslice &dest, const strslice &src) ;
} ;

/*******************************************************************************
 Global functions
*******************************************************************************/
std::ostream &operator<<(std::ostream &os, const stack_trace &) ;
std::ostream &operator<<(std::ostream &os, const resolved_iptr::source_loc &) ;
std::ostream &operator<<(std::ostream &os, const resolved_iptr &) ;

bool is_valgrind_present() noexcept ;
bool are_symbols_available() noexcept ;

/// A global variable, when set to nonzero forces skipping IsDebuggerPresent() check
/// in several places of the signal handler code, making possible do debug (the most of
/// the) print_state_with_debugger() itself.
///
/// Set it to 1 directly from GDB, like 'set pcomn::debug_debugger_backtrace=1', and most
/// of the print_state_with_debugger() and gdb_print_state() won't be skipped under GDB;
/// particularly this enables debugging of create_tempscript().
///
/// @note default is 0
///
extern volatile int debug_debugger_backtrace ;

extern "C" int enable_dump_on_abend(int traceout_fd = -1) ;


} // end of namespace pcomn

#endif /* __PCOMN_STACKTRACE_H */
