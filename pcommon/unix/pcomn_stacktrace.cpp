/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_stacktrace.cpp
 COPYRIGHT    :   2013 Google Inc.                 All Rights Reserved.
                  2016 Leo Yuriev (leo@yuriev.ru). All Rights Reserved.
                  2017 Yakov Markovitch.           All Rights Reserved.

                  See LICENSE for information on usage/redistribution.
*******************************************************************************/
#define UNW_LOCAL_ONLY
#include <pcomn_stacktrace.h>
#include <pcomn_strslice.h>
#include <pcomn_strnum.h>
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
#include <sys/types.h>
#include <sys/wait.h>

#include <ucontext.h>
#include <syscall.h>
#include <libunwind.h>

#include <libelf.h>
#include <elfutils/libdw.h>
#include <elfutils/libdwfl.h>
#include <dwarf.h>

#include <valgrind/valgrind.h>

namespace pcomn {

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
__noinline stack_trace::stack_trace(const void *from, size_t maxdepth)
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
        fframe.source_location(ssafe_strslice(dwarf_linesrc(srcloc, 0, 0)), line) ;
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
                fframe.init_member(location._function, ssafe_strslice(dwarf_diename(die))) ;
                fframe.init_member(location._filename, ssafe_strslice(find_call_file(die))) ;
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
 Global functions
*******************************************************************************/
bool is_valgrind_present() noexcept
{
    return RUNNING_ON_VALGRIND ;
}

bool are_symbols_available() noexcept
{
    if (elf_version(EV_CURRENT) == EV_NONE)
        return false ;

    char self_path[PATH_MAX] ;
    if (!*ssafe_progname(self_path))
        return false ;

    // Open self to obtain ELF descriptor
    const int elf_fd = open(self_path, O_RDONLY) ;
    if (elf_fd < 0)
        return false ;

    // Initialize elf pointer to examine file contents.
    if (Elf * const elf = elf_begin(elf_fd, ELF_C_READ, NULL))
    {
        // Iterate through ELF sections until .symtab section is found
        for (Elf_Scn *section = nullptr ; section = elf_nextscn(elf, section) ;)
        {
            GElf_Shdr shdr ;
            // Retrieve the section header
            gelf_getshdr(section, &shdr) ;

            if (shdr.sh_type == SHT_SYMTAB)
            {
                // A header of a section that holds a symbol table found,
                // there are symbols present.
                elf_end(elf) ;
                close(elf_fd) ;
                return true ;
            }
        }
        elf_end(elf) ;
    }

    close(elf_fd) ;
    return false ;
}

/*******************************************************************************
 ************                Signal handler                         ***********
********************************************************************************/
// A global variable, when set to nonzero forces skipping IsDebuggerPresent() check
// in several places in the code, making possible do debug (the most of the)
// print_state_with_debugger() itself.
//
// Set it to 1 directly from GDB, like 'set pcomn::debug_debugger_backtrace=1', and most
// of the print_state_with_debugger() and gdb_print_state() will not be skipped under GDB;
// particularly this enables debugging of create_tempscript()
//
volatile int debug_debugger_backtrace = false ;

/*******************************************************************************
 Registrator of a backtracing signal handler.
*******************************************************************************/
static bool     backtrace_enabled = false ;
static int      backtrace_fd = STDERR_FILENO ;
static time_t   backtrace_time = 0 ;

static __noreturn void backtrace_handler(int, siginfo_t *info, void *ctx) ;

extern "C" int enable_dump_on_abend(int traceout_fd)
{
    static bool loaded = false ;

    if (loaded)
    {
        const bool was_enabled = backtrace_enabled ;
        backtrace_fd = traceout_fd < 0 ? STDERR_FILENO : traceout_fd ;
        backtrace_enabled = true ;
        return was_enabled ;
    }

    auto printerror = [](const char *errtext)
    {
        write(STDERR_FILENO, errtext, strlen(errtext)) ;
    } ;

    static const int coredump_signals[] =
        {
            // Signals for which the default action is "Core".
            SIGABRT,    // Abort signal from abort(3)
            SIGBUS,     // Bus error (bad memory access)
            SIGFPE,     // Floating point exception
            SIGILL,     // Illegal Instruction
            SIGIOT,     // IOT trap. A synonym for SIGABRT
            SIGQUIT,    // Quit from keyboard
            SIGSEGV,    // Invalid memory reference
            SIGSYS,     // Bad argument to routine (SVr4)
            SIGTRAP,    // Trace/breakpoint trap
            SIGUNUSED,  // Synonymous with SIGSYS
            SIGXCPU,    // CPU time limit exceeded (4.2BSD)
            SIGXFSZ,    // File size limit exceeded (4.2BSD)
        } ;

    static constexpr size_t ALTSTACK_SIZE = 8*MiB ;

    // Never deallocated
    static char * const stack_storage =
        (char *)mmap(nullptr, ALTSTACK_SIZE, PROT_WRITE|PROT_READ, MAP_PRIVATE|MAP_ANONYMOUS|MAP_STACK, -1, 0) ;

    if (!stack_storage)
        printerror("\nWARINING: Cannot allocate alternate stack for signal handlers.\n"
                   "Stack trace on fatal signals will be unavailable.\n") ;
    else if (!loaded)
    {
        stack_t ss ;
        ss.ss_sp = stack_storage ;
        ss.ss_size = ALTSTACK_SIZE ;
        ss.ss_flags = 0 ;
        loaded = sigaltstack(&ss, 0) == 0 ;

        if (!loaded)
            printerror("\nWARINING: Cannot switch to alternate stack for signal handlers.\n"
                       "Stack trace on fatal signals will be unavailable.\n") ;
    }
    if (!loaded)
        return false ;

    for (const int coresig: coredump_signals)
    {
        struct sigaction action ;
        memset(&action, 0, sizeof action) ;
        action.sa_flags = (SA_SIGINFO | SA_ONSTACK | SA_NODEFER | SA_RESETHAND) ;
        sigfillset(&action.sa_mask) ;
        sigdelset(&action.sa_mask, coresig) ;
        action.sa_sigaction = &backtrace_handler ;

        if (sigaction(coresig, &action, 0) < 0)
        {
            printerror(PCOMN_BUFPRINTF
                       (256, "\nCannot register signal handler for signal #%d.\nStack trace on signal %d will be unavailable.\n",
                        coresig, coresig)) ;
        }
    }
    backtrace_fd = traceout_fd < 0 ? STDERR_FILENO : traceout_fd ;
    backtrace_enabled = true ;
    return true ;
}

/*******************************************************************************
 Signal handler utility functions
*******************************************************************************/
static const char backtrace_msgprefix[] = "\n------ " ;
static const char backtrace_msgsuffix[] = " ------\n\n" ;

static void puterror(const char *errtext)
{
    write(backtrace_fd, errtext, strlen(errtext)) ;
}

static __noinline void putmsg(const char *message)
{
    puterror(backtrace_msgprefix) ;
    puterror(message) ;
}

static __noinline void putstrerror(const char *errtext)
{
    char errbuf[128] = "" ;
    strerror_r(errno, errbuf, sizeof errbuf - 1) ;
    if (errtext)
        putmsg(errtext) ;
    puterror(strcat(errbuf, "\n")) ;
}

static void print_state_with_debugger(const void *errsp, const void *errpc) ;
static void gdb_print_state(const char *tempscript_filename) ;
static int create_tempscript(pid_t guilty_thread, const void *frame_sp, const void *frame_pc,
                             char *filename_buf, size_t filename_bufsz) ;
static bool wait_n_kill(pid_t child, time_t timeout) ;
static unipair<const void *> context_frame(const ucontext_t *uctx) ;

/*******************************************************************************
 Backtracing signal handler
*******************************************************************************/
static
#ifdef PCOMN_COMPILER_GNU
__attribute__((no_sanitize_address))
#endif
__noreturn void backtrace_handler(int, siginfo_t *info, void *ctx)
{
    const void *errsp, *errpc ;
    std::tie(errsp, errpc) = context_frame((ucontext_t*)ctx) ;

    auto forward_signal = [&]
    {
        putmsg("Forwarding signal\n") ;

        psiginfo(info, nullptr) ;
        // Try to forward the signal.
        raise(info->si_signo) ;
        // Terminate the process immediately.
        putmsg("FATAL: cannot forward signal") ;
        if (IsDebuggerPresent())
        {
            puterror(" give way to a debugger.\n") ;
            DebugBreak() ;
        }
        puterror(", exiting immediately.\n") ;
        _exit(EXIT_FAILURE) ;
    } ;

    backtrace_time = time(NULL) ;

    stack_trace st (errpc) ;
    // Print!

    print_state_with_debugger(errsp, errpc) ;

    forward_signal() ;
    _exit(EXIT_FAILURE) ;
}

static void print_state_with_debugger(const void *sp, const void *pc)
{
    if (is_valgrind_present())
    {
        putmsg("Running under Valgrind, skipping state printing by gdb\n") ;
        return ;
    }
    if (IsDebuggerPresent() && !debug_debugger_backtrace)
    {
        putmsg("Already under debugger, skipping state printing by gdb\n") ;
        return ;
    }

    // Get current thread's TID
    const pid_t tid = syscall(SYS_gettid) ;

    char tempscript_filename[PATH_MAX] ;
    const int tempscript_fd = create_tempscript(tid, sp, pc, tempscript_filename, sizeof tempscript_filename) ;
    if (tempscript_fd < 0)
    {
        putstrerror("FAILURE: Cannot create temporary GDB script for printing state: ") ;
        return ;
    }

    // Fork/exec GDB, attach GDB to this process and run the prepared script,
    // wait for GDB to exit (kill 9 on timeout).
    gdb_print_state(tempscript_filename) ;

    unlink(tempscript_filename) ;
    close(tempscript_fd) ;
}

static void gdb_print_state(const char *tempscript_filename)
{
    char self_pidstr[16] ;
    char self_progpath[PATH_MAX] ;

    numtostr(getpid(), self_pidstr) ;
    ssafe_progname(self_progpath) ;

    if (IsDebuggerPresent())
        return ;

    // Avoid ECHILD from waitpid()
    signal(SIGCHLD, SIG_DFL) ;

    if (const pid_t gdb_pid = fork())
    {
        if (gdb_pid > 0)
            wait_n_kill(gdb_pid, 42) ;
        else ;
        return ;
    }

    // From this point on, we're executing the code of the new (forked) process.

    // Set both the stderr and stdout to backtrace fd and close all others
    dup2(backtrace_fd, STDOUT_FILENO) ;
    dup2(backtrace_fd, STDERR_FILENO) ;
    for(int fd = getdtablesize() ; fd > STDERR_FILENO ; close(fd--)) ;

    // Create a new session
    setsid() ;
    setpgid(0, 0) ;
    // Exec GDB
    execlp("gdb", "gdb", "-batch", "-p", self_pidstr, "-s", self_progpath, "-n", "-x", tempscript_filename, NULL) ;

    // Were the exec successful, we wouldn't get here: the execlp shouldn't return.
    // If we _are_ here, something went awry.
    putstrerror("FAILURE: GDB launch failed, cannot print state: ") ;
    fsync(STDERR_FILENO) ;
    _exit(EXIT_FAILURE) ;
}

static unipair<const void *> context_frame(const ucontext_t *uctx)
{
    const void * const errpc =
        #ifdef REG_RIP // x86_64
        reinterpret_cast<void*>(uctx->uc_mcontext.gregs[REG_RIP])
        #elif defined(__aarch64__)
        reinterpret_cast<void*>(uctx->uc_mcontext.pc)
        #else
        #   error "Unsupported CPU architecture"
        #endif
        ;

    const void * const errsp =
        #ifdef REG_RSP // x86_64
        reinterpret_cast<void*>(uctx->uc_mcontext.gregs[REG_RSP])
        #elif defined(__aarch64__)
        reinterpret_cast<void*>(uctx->uc_mcontext.sp)
        #endif
        ;
    return {errsp, errpc} ;
}

static bool wait_n_kill(pid_t child, time_t timeout)
{
    // Wait for the child to exit, but not forever
    pid_t result = 0 ;
    const timespec t1s = {1, 0} ;
    for (timespec interval = t1s ; !(result = waitpid(child, NULL, WNOHANG)) && timeout-- ; interval = t1s)
        for (int still_safe = 100 ; still_safe && nanosleep(&interval, &interval) != 0 ; --still_safe) ;

    if (result <= 0)
        // Timeout
        kill(child, SIGKILL) ;

    return result > 0 ;
}

static const char define_run_command[] =
R"(

py
import datetime, sys, os

# Avoid output buffering
sys.stdout = sys.stderr

class Abend(gdb.Command):
    def __init__(self):
        super(Abend, self).__init__ ('abend-header', gdb.COMMAND_OBSCURE, gdb.COMPLETE_NONE)
        Abend.now  = staticmethod(lambda m=datetime.datetime.now: m().replace(microsecond=0))
        Abend.just = staticmethod(lambda s: s.ljust(32))
        Abend.pretty_delimiter = '----------------'
        Abend.pretty_width = 132

    def invoke(self, argv, from_tty):
        Abend.invocation_time = Abend.now().replace(microsecond=0)
        Abend.invocation_name = os.readlink("/proc/%s/exe" % (gdb.selected_inferior().pid,))
        print '\n%s\n%s %s %s' % ('#' * Abend.pretty_width, Abend.just('###### START ABEND DUMP'),
                                  Abend.invocation_time, Abend.invocation_name)

class AbendFooter(gdb.Command):
    def __init__(self):
        super (AbendFooter, self).__init__ ('abend-footer', gdb.COMMAND_OBSCURE, gdb.COMPLETE_NONE)

    def invoke(self, argv, from_tty):
        print '\n%s %s\n%s' % (Abend.just('###### END ABEND DUMP STARTED AT'), Abend.invocation_time, '#' * Abend.pretty_width)

class PrettyRun(gdb.Command):
    def __init__ (self):
        super (PrettyRun, self).__init__ ('pretty-run', gdb.COMMAND_OBSCURE, gdb.COMPLETE_NONE)

    def invoke(self, argv, from_tty):
        print '\n' + ('%s START %s ' % (Abend.pretty_delimiter, argv)).ljust(Abend.pretty_width, '-') ;
        gdb.execute(argv)
        gdb.execute('printf ""')
        print ('%s END %s' % (Abend.pretty_delimiter, argv)).ljust(Abend.pretty_width, '-') ;

Abend()
AbendFooter()
PrettyRun()
end

define print_mmaps
    py print '\n%s Memory maps from /proc/self/smaps' % Abend.pretty_delimiter
    py gdb.execute('shell cat /proc/%s/smaps' % gdb.selected_inferior().pid)
    py print Abend.pretty_delimiter
end
)" ;

static int create_tempscript(pid_t guilty_thread, const void *frame_sp, const void *frame_pc,
                             char *result_filename, size_t filename_bufsz)
{
    const char name_template[] = "/tmp/gdbscriptXXXXXX" ;
    if (filename_bufsz)
        *result_filename = 0 ;
    if (filename_bufsz < sizeof name_template)
    {
        errno = EINVAL ;
        return -1 ;
    }
    strcpy(result_filename, name_template) ;
    // Create a temporary file
    const int script_fd = mkstemp(result_filename) ;
    if (script_fd < 0)
        return script_fd ;

    // pcomn::bufstr_ostream _is_ signal-safe, as long as we output strings, strslices,
    // and integral types.
    bufstr_ostream<16384> script ;

    (script
     << define_run_command <<

     "set filename-display basename\n"
     "set print frame-arguments all\n"
     "set print entry-values both\n"
     "set scheduler-locking on\n"

     "handle SIGPIPE pass nostop\n"

     // Print the header
     "abend-header\n"

     // Switch to the thread the signal came from
     "py [t.switch() for t in gdb.selected_inferior().threads() if t.ptid[1]==" << guilty_thread << "]\n"
     "pretty-run thread\n"
     "pretty-run backtrace full\n"
        )
        ;

    if (frame_sp && frame_pc)
        (script <<

         "# Select the signal source frame\n"
         "select-frame " << frame_sp << '\n' <<

         "# May happen the frame is a special trampoline created for signal delivery,\n"
         "# then the actual frame is the next directly above (older).\n"
         "py gdb.selected_frame().type()==gdb.SIGTRAMP_FRAME and gdb.selected_frame().older().select()\n"

         "pretty-run info frame\n"
         "pretty-run info locals\n"
         "pretty-run info all-registers\n"
         "pretty-run disassemble\n")
            ;

    (script <<
     "pretty-run info sharedlibrary\n"
     "pretty-run info threads\n"
     "pretty-run thread apply all backtrace\n"
     "pretty-run thread apply all info registers\n"
     "pretty-run thread apply all disassemble\n"
     "pretty-run thread apply all backtrace full\n"

     // Print memory maps
     "print_mmaps\n"

     "pretty-run shell uname -a\n"
     "pretty-run shell df -lh\n"
     "pretty-run show environment\n"

      // Print the footer
      "abend-footer\n")
        ;


    script <<
        "detach\n"
        "quit\n"
        ;

    write(script_fd, script.str(), script.end() - script.begin()) ;

    return script_fd ;
}

} // end of namespace pcomn
