/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
#include <pcomn_stacktrace.h>
#include <pcomn_ssafe.h>
#include <pcomn_string.h>
#include <pcomn_strnum.h>

#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <ucontext.h>

namespace pcomn {

/*******************************************************************************
 Signal handler
*******************************************************************************/
// A global variable, when set to nonzero forces skipping IsDebuggerPresent() check
// in several places in the code, making possible do debug (the most of the)
// print_state_with_debugger() itself.
//
// Set it to 1 directly from GDB, like 'set pcomn::debug_debugger_backtrace=1', and most
// of the print_state_with_debugger() and gdb_print_state() will not be skipped under GDB;
// particularly this enables debugging of create_tempscript()
//
volatile int debug_debugger_backtrace = false ;

static bool     backtrace_enabled = false ;
static int      backtrace_fd = STDERR_FILENO ;
static time_t   backtrace_time = 0 ;
static const char backtrace_msgprefix[] = "\n------ " ;
static const char backtrace_msgsuffix[] = " ------\n\n" ;

static __noreturn void backtrace_handler(int, siginfo_t *info, void *ctx) ;

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

static void printerror(const char *errtext)
{
    write(STDERR_FILENO, errtext, strlen(errtext)) ;
}

/*******************************************************************************
 Backtracing signal handler registration
*******************************************************************************/
extern "C" int set_backtrace_on_coredump(int traceout_fd = -1) ;

extern "C" int set_backtrace_on_coredump(int traceout_fd)
{
    static bool loaded = false ;

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
static void print_state_with_debugger(const void *errsp, const void *errpc) ;
static void gdb_print_state(const char *tempscript_filename) ;
static int create_tempscript(pid_t guilty_thread, const void *frame_sp, const void *frame_pc,
                             char *filename_buf, size_t filename_bufsz) ;
static bool wait_n_kill(pid_t child, time_t timeout) ;

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

    print_memmaps() ;
    print_state_with_debugger(errsp, errpc) ;

    forward_signal() ;
    _exit(EXIT_FAILURE) ;
}

static void print_memmaps()
{
    const int memmaps_fd = open("/proc/self/smaps", O_RDONLY) ;
    if (memmaps_fd < 0)
        return ;

    auto print_boundary = []
    {
        char rfcdate[RFC3339_FULL] ;
        putmsg("Memory maps by /proc/self/smaps ") ;
        puterror(ssafe_rfc3339_gmtime(backtrace_time, rfcdate)) ;
        puterror("\n\n") ;
    } ;

    print_boundary() ;

    char buf[1024] ;
    for (ssize_t n ; (n = read(memmaps_fd, buf, sizeof(buf))) > 0 && write(backtrace_fd, buf, n) == n ;) ;
    close(memmaps_fd) ;

    putmsg("END") ;
    puterror(backtrace_msgsuffix) ;
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
define pretty_run
    echo \n\n------\n
    if $argc == 1
        $arg0
    end
    if $argc == 2
        $arg0 $arg1
    end
    if $argc == 3
        $arg0 $arg1 $arg2
    end
    if $argc == 4
        $arg0 $arg1 $arg2 $arg3
    end
    if $argc == 5
        $arg0 $arg1 $arg2 $arg3 $arg4
    end
    if $argc == 6
        $arg0 $arg1 $arg2 $arg3 $arg5
    end
    echo ------\n
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
    bufstr_ostream<8192> script ;

    script
        << define_run_command <<
        "set filename-display basename\n"
        //"set print frame-arguments all\n"
        //"set print entry-values both\n"
        "set scheduler-locking on\n"

        "handle SIGPIPE pass nostop\n"

        // Switch to the thread the signal came from
        "py [t.switch() for t in gdb.selected_inferior().threads() if t.ptid[1]==" << guilty_thread << "]\n"
        "pretty_run thread\n"
        "pretty_run info sharedlibrary\n"
        "pretty_run info threads\n"
        "pretty_run thread apply all backtrace\n"
        "pretty_run thread apply all disassemble\n"
        "pretty_run thread apply all info all-registers\n"
        "pretty_run thread apply all backtrace full\n"
        "pretty_run shell uname -a\n"
        "pretty_run shell df -lh\n"
        "pretty_run show environment\n"
        "pretty_run backtrace full\n" ;

    if (frame_sp && frame_pc)
    {
        script <<
            "frame " << frame_sp << ' ' << frame_pc << '\n' <<
            "info locals\n"
            "info all-registers\n"
            "disassemble\n"
            ;
    }

    script <<
        "detach\n"
        "quit\n"
        ;

    write(script_fd, script.str(), script.end() - script.begin()) ;

    return script_fd ;
}

} // end of namespace pcomn

#include <iostream>
#include <pcomn_regex.h>

using namespace pcomn ;

void test_rx(const regex &exp, const char *str)
{
    reg_match sub[36] ;
    for (reg_match *first = sub, *last = exp.match(str, sub) ; first != last ; ++first)
        std::cout << strslice(str, *first) << std::endl ;
}

const char *read_rx(char *str)
{
    return std::cin.getline(str, 1024*1024) ? str : nullptr ;
}

static struct {
    char inbuf[64] ;
    char *buf = inbuf ;
} data ;

int main(int argc, char *argv[])
{
    backtrace_time = time(NULL) ;

    set_backtrace_on_coredump() ;

    char localbuf1[RFC3339_FULL] ;
    char localbuf2[RFC3339_DATETIME] ;
    char localbuf3[RFC3339_DATE] ;

    char gmbuf1[RFC3339_FULL] ;
    char gmbuf2[RFC3339_DATETIME] ;
    char gmbuf3[RFC3339_DATE] ;

    std::cout
        << ssafe_rfc3339_localtime(backtrace_time, localbuf1) << '\n'
        << ssafe_rfc3339_localtime(backtrace_time, localbuf2) << '\n'
        << ssafe_rfc3339_localtime(backtrace_time, localbuf3) << "\n\n"
        << ssafe_rfc3339_gmtime(backtrace_time, gmbuf1) << '\n'
        << ssafe_rfc3339_gmtime(backtrace_time, gmbuf2) << '\n'
        << ssafe_rfc3339_gmtime(backtrace_time, gmbuf3) << '\n'
        << std::endl ;

    if (argc != 2)
    {
        std::cout << "Usage: " << *argv << " <regexp>" << std::endl ;
        return 255 ;
    }

    const regex exp (argv[1]) ;
    read_rx(data.inbuf) ;
    test_rx(exp, data.buf) ;
    return 0 ;
}
