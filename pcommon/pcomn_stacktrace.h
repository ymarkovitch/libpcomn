/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
#ifndef __PCOMN_STACKTRACE_H
#define __PCOMN_STACKTRACE_H
/*******************************************************************************
 FILE         :   pcomn_stacktrace.h
 COPYRIGHT    :   2013 Google Inc.       All Rights Reserved.
                  2017 Yakov Markovitch. All Rights Reserved.

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*******************************************************************************/
#include <pcomn_simplematrix.h>
#include <pcomn_strslice.h>
#include <iostream>

namespace pcomn {
/***************************************************************************//**
 Object that stores the call stack trace.
*******************************************************************************/
class stack_trace final {
    constexpr static size_t _MAXDEPTH = 32 ;
public:
    typedef void *frame ;

    explicit stack_trace(void *addr, size_t depth = -1) ;
    explicit stack_trace(size_t depth = -1) : stack_trace(nullptr, depth) {}

    stack_trace(const stack_trace &other) :
        _thread_id(other._thread_id),
        _skip(other._skip),
        _stacktrace(other._stacktrace),
        _begin(_stacktrace.begin() + (other._begin - other.begin()))
    {}

    size_t size() const { return _stacktrace.end() - _begin ; }

    const frame *begin() const { return _begin ; }
    const frame *end() const { return _stacktrace.end() ; }

    unsigned thread_id() const { return _thread_id ; }

    constexpr static size_t maxdepth() { return _MAXDEPTH ; }

private:
    size_t              _thread_id = 0 ;
    size_t              _skip = 0 ;
    static_vector<frame, _MAXDEPTH + 8> _stacktrace ;
    const frame *        _begin = _stacktrace.begin() ;

private:
    void skip(size_t levels) { _skip = levels ; }
    void load_thread_info() ;
    void unwind(size_t maxdepth) ;
} ;

/*******************************************************************************
 resolved_frame

 @note PC is the value of the Program Counter register.
*******************************************************************************/
class resolved_frame final {
    PCOMN_NONCOPYABLE(resolved_frame) ;
    PCOMN_NONASSIGNABLE(resolved_frame) ;
public:
    static const size_t NAMES_MAXMEM = 4*KiB ;

    struct source_loc {
        friend resolved_frame ;

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

    resolved_frame() = default ;
    resolved_frame(stack_trace::frame unresolved) ;

    stack_trace::frame frame() const { return _frame ; }
    const strslice &object_filename() const { return _object_filename ; }
    const strslice &object_function() const { return _object_function ; }
    const source_loc &source() const { return _source ; }

    explicit operator bool() const { return !!frame() ; }

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

    resolved_frame &reset(stack_trace::frame pc) ;

    resolved_frame &reset() { return reset(_frame) ; }

private:
    stack_trace::frame _frame = nullptr ;

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

} // end of namespace pcomn

#endif /* __PCOMN_STACKTRACE_H */
