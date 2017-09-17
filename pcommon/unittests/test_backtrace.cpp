/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
#include <pcomn_platform.h>
#include <pcomn_assert.h>
#include <pcommon.h>

#include <valgrind/valgrind.h>

#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <execinfo.h>
#include <ucontext.h>

#include <libelf.h>
#include <gelf.h>

extern "C" ssize_t ssafe_read(const char *name, char *buf, size_t bufsize) noexcept
{
    if (!name || !buf && bufsize)
    {
        errno = EINVAL ;
        return -1 ;
    }
    const int fd = open(name, O_RDONLY) ;
    if (fd < 0)
        return -1 ;

    const ssize_t result = bufsize ? read(fd, buf, bufsize) : 0 ;
    if (bufsize)
        buf[std::min<ssize_t>(std::max<ssize_t>(result, 1), bufsize) - 1] = 0 ;

    close(fd) ;
    return result ;
}

inline const char *ssafe_reads(const char *name, char *buf, size_t bufsize) noexcept
{
    ssafe_read(name, buf, bufsize) ;
    return buf ? buf : "" ;
}

template<size_t n>
inline const char *ssafe_reads(const char *name, char (&buf)[n]) noexcept
{
    return ssafe_reads(name, buf, n) ;
}

extern "C" const char *ssafe_progname(char *buf, size_t bufsize) noexcept
{
    static const char empty[] = {0} ;
    if (!buf || !bufsize)
    {
        if (!buf)
            errno = EINVAL ;
        return empty ;
    }
    readlink("/proc/self/exe", memset(buf, 0, bufsize), bufsize - 1) ;
    return buf ;
}

template<size_t n>
inline const char *ssafe_progname(char (&buf)[n]) noexcept
{
    return ssafe_progname(buf, n) ;
}

static bool is_valgrind_present()
{
    return RUNNING_ON_VALGRIND ;
}

static bool are_symbols_available()
{
    if (elf_version(EV_CURRENT) == EV_NONE)
        return false ;

    char self_path[PATH_MAX] ;
    if (!*ssafe_progname(self_path))
        return false ;

    // Open self to obtain ELF descriptor
    const int elf_fd = open(name_buf, O_RDONLY) ;
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
            gelf_getshdr(scn, &shdr) ;

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
 Signal handler
*******************************************************************************/
static bool backtrace_enabled = false ;
static int  backtrace_fd = STDERR_FILENO ;

static __noreturn void backtrace_handler(int, siginfo_t *info, void *ctx) ;

static void puterror(const char *errtext)
{
    write(backtrace_fd, errtext, strlen(errtext)) ;
}

static void printerror(const char *errtext)
{
    write(STDERR_FILENO, errtext, strlen(errtext)) ;
}

/*******************************************************************************
 Backtracing signal handler registration
*******************************************************************************/
extern "C" int set_backtrace_on_coredump(int traceout_fd)
{
    if (loaded)
    {
        const bool was_enabled = backtrace_enabled ;
        backtrace_fd = traceout_fd < 0 ? STDERR_FILENO : traceout_fd ;
        backtrace_enabled = true ;
        return was_enabled ;
    }

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
    static bool loaded = false ;

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
 Backtracing signal handler
*******************************************************************************/
static void print_memmaps() ;
static void print_state_by_debugger() ;
static pid_t gdb_print_state() ;
static int create_tempscript(char *filename_buf, size_t filename_bufsz) ;
static bool wait_n_kill(pid_t child) ;

static
#ifdef PCOMN_COMPILER_GNU
__attribute__((no_sanitize_address))
#endif
__noreturn void backtrace_handler(int, siginfo_t *info, void *ctx)
{
    ucontext_t *uctx = (ucontext_t*)ctx ;

    void * const erraddr =
        #ifdef REG_RIP // x86_64
        reinterpret_cast<void*>(uctx->uc_mcontext.gregs[REG_RIP])
        #elif defined(__aarch64__)
        reinterpret_cast<void*>(uctx->uc_mcontext.pc)
        #elif defined(__ppc__) || defined(__powerpc) || defined(__powerpc__) || defined(__POWERPC__)
        reinterpret_cast<void*>(uctx->uc_mcontext.regs->nip)
        #else
        #   error "Unsupported CPU architecture"
        #endif
        ;

    auto forward_signal = [&]
    {
        psiginfo(info, nullptr) ;
        // Try to forward the signal.
        raise(info->si_signo) ;
        // Terminate the process immediately.
        puterror("\nFATAL: cannot forward signal") ;
        if (is_debugger_present())
        {
            puterror(" give way to a debugger.\n") ;
            DebugBreak() ;
        }
        puterror(", exiting immediately.\n") ;
        _exit(EXIT_FAILURE) ;
    } ;

    stack_trace st (erraddr) ;
    // Print!

    print_memmaps() ;
    print_state_by_debugger() ;

    forward_signal() ;
    _exit(EXIT_FAILURE) ;
}

static void print_memmaps()
{
    int mem_map_fd = open("/proc/self/smaps", O_RDONLY) ;
    if (mem_map_fd >= 0) {
        char buf[1024];
        dprintf(fd, "\n*** Memory usage map (by /proc/self/smaps):\n");
        while(1) {
            int n = read(mem_map_fd, &buf, sizeof(buf));
            if (n < 1)
                break;
            if (write(fd, &buf, n) != n)
                break;
        }
        close(mem_map_fd);
    }
}

static void print_state_by_debugger()
{
    if (is_valgrind_present())
    {
        puterror("**** Running under Valgrind, skipping state printing by gdb\n") ;
        return ;
    }
    if (is_debugger_present())
    {
        puterror("**** Already under debugger, skipping state printing by gdb\n") ;
        return ;
    }

    // Get current thread's TID
    const pid_t tid = syscall(SYS_gettid) ;

    char tempscript_filename[PATH_MAX] ;
    const int tempscript_fd = create_tempscript(tempscript_filename, sizeof tempscript_filename) ;
    if (tempscript_fd < 0)
    {
        puterror("**** FAILURE: Cannot create temporary GDB script for printing state\n") ;
        return ;
    }

    // Fork/exec GDB, attach GDB to this process and run the prepared script,
    // wait for GDB to exit (kill 9 on timeout).
    gdb_print_state() ;

    unlink(tempscript_filename) ;
    close(tempscript_fd) ;
}

static void gdb_print_state()
{
    // Avoid ECHILD from waitpid()
    signal(SIGCHLD, SIG_DFL) ;

    char self_pidstr[16] ;
    char self_progpath[PATH_MAX] ;

    numtostr(getpid(), self_pidstr) ;
    ssafe_progname(self_progpath) ;

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
    for(fd = getdtablesize() ; fd > STDERR_FILENO ; close(fd--)) ;

    // Create a new session
    setsid() ;
    setpgid(0, 0) ;
    // Exec GDB
    execlp("gdb", "gdb", "-q", "-se", self_progpath, "-n", NULL) ;

    // Were the exec successful, we wouldn't get here: the execlp shouldn't return.
    // If we _are_ here, something went awry.
    char errbuf[128] = "" ;
    strerror_r(errno, errbuf, sizeof errbuf) ;
    dprintf(STDERR_FILENO, "\n*** FAILURE: GDB launch failed, cannot print state: %s\n", errbuf) ;
    fsync(STDERR_FILENO) ;
    _exit(EXIT_FAILURE) ;
}

static bool wait_n_kill(pid_t child, time_t timeout)
{
    // Wait for the child to exit, but not forever
    pid_t result = 0 ;
    const timespec t1s = {1, 0} ;
    for (timespec interval = t1s ; !(result = waitpid(gdb_pid, NULL, WNOHANG)) && timeout-- ; interval = t1s)
        for (int still_safe = 100 ; still_safe && nanosleep(&interval, &interval) != 0 ; --still_safe) ;

    if (result <= 0)
        // Timeout
        kill(gdb_pid, SIGKILL) ;

    return result > 0 ;
}

static int create_tempscript(char *filename_buf, size_t filename_bufsz)
{
    // pcomn::bufstr_ostream is signal-safe, as long as we output strings, strslices,
    // and integral types.
    bufstr_ostream<8192> script ;
    script
        <<
        "set confirm off\n"
        "set width 132\n"
        "set prompt =============================================================================\\n\n"
        "attach " << getpid() << '\n'
        <<
        "handle SIGPIPE pass nostop\n"
        "interrupt\n"
        "set scheduler-locking on\n"
        // Switch to the thread the signal came from
        "py [t.switch() for t in gdb.selected_inferior().threads() if t.ptid[1]==" << tid << "]\n"
        <<
        "info sharedlibrary\n"
        "info threads\n"
        "thread apply all backtrace\n"
        "thread apply all disassemble\n"
        "thread apply all info all-registers\n"
        "thread apply all backtrace full\n"
        "shell uname -a\n"
        "shell df -lh\n"
        "show environment\n"
        <<
        "frame %d\n"
        <<
        "thread\n"
        "backtrace\n"
        "info all-registers\n"
        "disassemble\n"
        "backtrace full\n"
        "detach\n"
        "quit\n"
        ;
    write(script_fd, str(), script.end() - script.begin()) ;
    return script_fd ;
}
