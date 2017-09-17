/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 * Copyright 2013 Google Inc.       All Rights Reserved.
 * Copyright 2017 Yakov Markovitch. All Rights Reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*******************************************************************************/
#define UNW_LOCAL_ONLY
#include <pcomn_stacktrace.h>
#include <pcommon.h>

#include <malloc.h>
#include <cxxabi.h>
#include <string.h>
#include <algorithm>
#include <string>
#include <vector>
#include <limits>

#include <unistd.h>
#include <signal.h>

#include <sys/mman.h>

#include <syscall.h>
#include <libunwind.h>
#include <elfutils/libdw.h>
#include <elfutils/libdwfl.h>
#include <dwarf.h>

namespace pcomn {

static inline strslice safe_strslice(const char *s)
{
    return s ? strslice(s) : strslice() ;
}

/*******************************************************************************
 frame_resolver
*******************************************************************************/
class frame_resolver {
public:
    frame_resolver() = default ;

    resolved_frame &resolve(resolved_frame &) ;

private:
    struct end_dwfl {void operator()(Dwfl *dwfl) { if (dwfl) dwfl_end(dwfl) ; }} ;

    std::unique_ptr<Dwfl_Callbacks> _dwfl_callbacks ;
    std::unique_ptr<Dwfl, end_dwfl> _dwfl_handle ;
    bool                            _initialized = false ;

private:
    bool init() ;

    Dwfl *session() { return _dwfl_handle.get() ; }

    template<typename Callback>
    static bool depth_first_search_by_pc(Dwarf_Die *parent, Dwarf_Addr pc, Callback &&callback) ;
} ;

/*******************************************************************************
 stack_trace
*******************************************************************************/
__noinline stack_trace::stack_trace(void *from, size_t maxdepth)
{
    maxdepth = !from
        ? std::min(_stacktrace.max_size(), maxdepth)
        : std::min(_stacktrace.max_size() - 8, maxdepth) + 8 ;

    load_thread_info() ;
    unwind(maxdepth) ;

    if (!from || _stacktrace.empty())
        return ;

    const auto pc = std::find(_stacktrace.begin(), _stacktrace.end(), from) ;
    if (pc != _stacktrace.end())
        skip(pc - _stacktrace.begin()) ;
    _stacktrace.resize(std::min(_stacktrace.size(), _skip + maxdepth)) ;
    _begin = _stacktrace.begin() + std::min(_skip,  _stacktrace.size()) ;
}

stack_trace::stack_trace(const stack_trace &other) :
    _thread_id(other._thread_id),
    _skip(other._skip),
    _stacktrace(other._stacktrace),
    _begin(_stacktrace.begin() + (other._begin - other.begin()))
{}

void stack_trace::load_thread_info()
{
    const size_t id = syscall(SYS_gettid) ;
    // Dont show thread ID for the maint thread
    _thread_id = id != (size_t)getpid() ? id : 0 ;
}

void stack_trace::unwind(size_t maxdepth)
{
    NOXCHECK(maxdepth <= _stacktrace.max_size() - _stacktrace.size()) ;
    if (!maxdepth)
        return ;

    unw_context_t ctx ;
    unw_cursor_t cursor ;

    // Initialize cursor to current frame for local unwinding.
    unw_getcontext(&ctx) ;
    unw_init_local(&cursor, &ctx) ;

    // Unwind frames one by one, going up the frame stack.
    for (unw_word_t pc ; maxdepth && unw_step(&cursor) > 0 && unw_get_reg(&cursor, UNW_REG_IP, &pc) == 0 && pc ; --maxdepth)
    {
        _stacktrace.push_back((frame)pc) ;
    }
}

/*******************************************************************************
 resolved_frame
*******************************************************************************/
resolved_frame &resolved_frame::reset(stack_trace::frame pc)
{
    _frame = pc ;
    _object_function = _object_filename = {} ;
    _source = {} ;
    for (source_loc &inliner: _inliners) inliner = {} ;
    _inliners.clear() ;
    return *this ;
}

strslice &resolved_frame::init_member(strslice &dest, const strslice &src)
{
    if (!src ||
        xinrange(src.begin(), _memory.begin(), _memory.end()) &&
        xinrange(src.end(), _memory.begin(), _memory.end()))
    {
        dest = src ;
    }
    else
    {
        // Only members of itself are allowed
        NOXCHECK(xinrange((uintptr_t)&dest, (uintptr_t)this, (uintptr_t)(this + 1))) ;

        const char * const start = _memory.end() ;
        _memory.write(src.begin(), src.size()) ;
        dest = {start, _memory.end()} ;
    }
    return dest ;
}

/*******************************************************************************
 frame_resolver
*******************************************************************************/
static inline const char *find_call_file(Dwarf_Die *die)
{
    Dwarf_Sword file_idx = 0 ;

    dwarf_formsdata(dwarf_attr(die, DW_AT_call_file, &as_mutable(Dwarf_Attribute())), &file_idx);
    if (!file_idx)
        return nullptr ;

    Dwarf_Die buf ;
    Dwarf_Die *cudie = dwarf_diecu(die, &buf, 0, 0) ;
    if (!cudie)
        return nullptr ;

    Dwarf_Files *files = nullptr ;
    dwarf_getsrcfiles(cudie, &files, &as_mutable(size_t())) ;

    return files ? dwarf_filesrc(files, file_idx, 0, 0) : nullptr ;
}

static bool is_pc_in_entity(Dwarf_Addr pc, Dwarf_Die *entity)
{
    Dwarf_Addr low, high ;

    // continuous range
    if (dwarf_hasattr(entity, DW_AT_low_pc) && dwarf_hasattr(entity, DW_AT_high_pc))
    {
        if (dwarf_lowpc(entity, &low) != 0)
            return false ;

        if (dwarf_highpc(entity, &high) != 0)
        {
            Dwarf_Attribute attr ;
            Dwarf_Word      value ;
            if (dwarf_formudata(dwarf_attr(entity, DW_AT_high_pc, &attr), &value) != 0)
                return false ;

            high = low + value ;
        }
        return pc >= low && pc < high ;
    }

    // non-continuous range.
    Dwarf_Addr base;
    for (ptrdiff_t offset = 0 ; (offset = dwarf_ranges(entity, offset, &base, &low, &high)) > 0 ; )
        if (pc >= low && pc < high)
            return true ;

    return false;
}

static Dwarf_Die *find_function_entity_by_pc(Dwarf_Die *parent, Dwarf_Addr pc, Dwarf_Die *result)
{
    if (dwarf_child(parent, result) != 0)
        return nullptr ;

    Dwarf_Die *die = result ;
    do {
        switch (dwarf_tag(die))
        {
            case DW_TAG_subprogram:
            case DW_TAG_inlined_subroutine:
                if (is_pc_in_entity(pc, die))
                    return result ;
        } ;
        bool declaration = false ;
        Dwarf_Attribute attr_mem;
        dwarf_formflag(dwarf_attr(die, DW_AT_declaration, &attr_mem), &declaration);
        if (!declaration)
        {
            // let's be curious and look deeper in the tree,
            // function are not necessarily at the first level, but
            // might be nested inside a namespace, structure etc.
            Dwarf_Die die_mem ;
            if (find_function_entity_by_pc(die, pc, &die_mem))
            {
                *result = die_mem;
                return result;
            }
        }
    }
    while (dwarf_siblingof(die, result) == 0) ;
    return nullptr ;
}

/*******************************************************************************
 frame_resolver
*******************************************************************************/
bool frame_resolver::init()
{
    if (_initialized)
        return true ;

    _dwfl_callbacks.reset(new Dwfl_Callbacks()) ;
    _dwfl_callbacks->find_elf = &dwfl_linux_proc_find_elf ;
    _dwfl_callbacks->find_debuginfo = &dwfl_standard_find_debuginfo ;

    _dwfl_handle.reset(dwfl_begin(_dwfl_callbacks.get())) ;
    _initialized = true ;

    if (!_dwfl_handle)
        return false ;

    // ...from the current process.
    dwfl_report_begin(session()) ;
    const int r = dwfl_linux_proc_report(session(), getpid()) ;
    dwfl_report_end(session(), NULL, NULL) ;

    return r >= 0 ;
}

resolved_frame &frame_resolver::resolve(resolved_frame &fframe)
{
    fframe.reset() ;

    if (!init() || !session())
        return fframe ;

    const Dwarf_Addr pc = (Dwarf_Addr)fframe.frame() ;

    // Find the module (binary object) that contains the frame's address by using
    // the address ranges of all the currently loaded binary object (no debug info
    // required).
    Dwfl_Module *mod = dwfl_addrmodule(session(), pc) ;
    if (mod)
    {
        // now that we found it, lets get the name of it, this will be the
        // full path to the running binary or one of the loaded library.
        if (const char * const module_name = dwfl_module_info(mod, 0, 0, 0, 0, 0, 0, 0))
            fframe.object_filename(module_name) ;

        // We also look after the name of the symbol, equal or before this
        // address. This is found by walking the symtab. We should get the
        // symbol corresponding to the function (mangled) containing the
        // address. If the code corresponding to the address was inlined,
        // this is the name of the out-most inliner function.

        if (const char * const sym_name = dwfl_module_addrname(mod, pc))
            fframe.object_function(PCOMN_DEMANGLE(sym_name)) ;
    }

    // Attempt to find the source file and line number for the address.
    // Look into .debug_aranges for the address, map it to the location of the
    // compilation unit DIE in .debug_info, and return it.
    Dwarf_Addr mod_bias = 0;
    Dwarf_Die *cudie = dwfl_module_addrdie(mod, pc, &mod_bias) ;

    if (!cudie)
    {
        // Clang does not generate the section .debug_aranges, so
        // dwfl_module_addrdie will fail early. Clang doesn't either set
        // the lowpc/highpc/range info for every compilation unit.
        //
        // So for every compilation unit we will iterate over every single
        // DIEs. Normally functions should have a lowpc/highpc/range, which
        // we will use to infer the compilation unit.

        // This is probably badly inefficient.
        for (Dwarf_Die diebuf ; ((cudie = dwfl_module_nextcu(mod, cudie, &mod_bias)) &&
                                 !find_function_entity_by_pc(cudie, pc - mod_bias, &diebuf)) ; ) ;
    }

    if (!cudie)
        // Cannot find
        return fframe ;

    // Now we have a compilation unit DIE and this function will be able
    // to load the corresponding section in .debug_line (if not already
    // loaded) and hopefully find the source location mapped to our
    // address.
    if (Dwarf_Line *srcloc = dwarf_getsrc_die(cudie, pc - mod_bias))
    {
        int line = 0 ;
        dwarf_lineno(srcloc, &line) ;
        fframe.source_location(safe_strslice(dwarf_linesrc(srcloc, 0, 0)), line) ;
    }

    // Traverse inlined functions depth-first
    depth_first_search_by_pc(cudie, pc - mod_bias, [&](Dwarf_Die *die)
    {
        switch (dwarf_tag(die))
        {
            case DW_TAG_subprogram:
                if (const char * const name = dwarf_diename(die))
                    fframe.source_function(PCOMN_DEMANGLE(name)) ;
                break ;

            case DW_TAG_inlined_subroutine:
            {
                if (fframe._inliners.size() == fframe._inliners.max_size())
                    break ;

                fframe._inliners.push_back({}) ;
                resolved_frame::source_loc &location = fframe._inliners.back() ;

                Dwarf_Word line = 0 ;
                fframe.init_member(location._function, safe_strslice(dwarf_diename(die))) ;
                fframe.init_member(location._filename, safe_strslice(find_call_file(die))) ;
                dwarf_formudata(dwarf_attr(die, DW_AT_call_line, &as_mutable(Dwarf_Attribute())), &line) ;
                location._line = line ;
                break ;
            }
        }
    }) ;

    if (!fframe.source().function())
        // fallback.
        fframe.source_function(fframe.object_function()) ;

    return fframe ;
}

template<typename Callback>
bool frame_resolver::depth_first_search_by_pc(Dwarf_Die *parent, Dwarf_Addr pc, Callback &&callback)
{
    Dwarf_Die buf ;
    Dwarf_Die *die = &buf ;
    if (dwarf_child(parent, die) != 0)
        return false ;

    bool branch_has_pc = false;
    do {
        bool declaration = false;
        Dwarf_Attribute attr_mem;
        dwarf_formflag(dwarf_attr(die, DW_AT_declaration, &attr_mem), &declaration);

        if (!declaration)
            // Walk down the tree: functions are not necessarily at the top level, they
            // may be nested inside a namespace, struct, function, inlined function etc.
            branch_has_pc = depth_first_search_by_pc(die, pc, std::forward<Callback>(callback)) ;

        if (branch_has_pc || (branch_has_pc = is_pc_in_entity(pc, die)))
            std::forward<Callback>(callback)(die) ;
    }
    while (dwarf_siblingof(die, die) == 0) ;

    return branch_has_pc ;
}

/*******************************************************************************
 Signal handler
*******************************************************************************/

} // end of namespace pcomn
