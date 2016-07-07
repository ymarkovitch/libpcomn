/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
/*******************************************************************************
 FILE         :   shell_repl.cpp
 COPYRIGHT    :   Yakov Markovitch, 2009-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Interactive command-line shell implementation.

 CREATION DATE:   5 Jul 2009
*******************************************************************************/
#include "shell_repl.h"

#include <pcomn_path.h>
#include <pcomn_except.h>
#include <pcomn_safeptr.h>
#include <pcomn_string.h>
#include <pcomn_sys.h>
#include <pcomn_trace.h>

#include <iostream>
#include <algorithm>
#include <stdexcept>

#include <stdlib.h>
#include <stdio.h>

#ifndef PCOMN_PL_WINDOWS
/*******************************************************************************
 Windows
*******************************************************************************/
#include <readline/readline.h>
#include <readline/history.h>
#include <syslog.h>

static inline bool is_probably_interactive()
{
    const unsigned mode = pcomn::sys::filestat(fileno(stdin)).st_mode ;
    return S_ISCHR(mode) || S_ISFIFO(mode) ;
}

#else
/*******************************************************************************
 Nix
*******************************************************************************/
#include <editline/readline.h>
#include <stdio.h>
#include <io.h>

#define openlog(name, level, facility) ((void)0)
static inline int rl_initialize() { return using_history() ; }

static inline bool is_probably_interactive()
{
    return isatty(fileno(stdout)) ;
}
#endif

namespace pcomn {
namespace sh {

namespace { struct quit_shell{} ; }

/*******************************************************************************
 CmdContext
*******************************************************************************/
CmdContext::CmdContext(const char *cmdname, const char *description, const char *version_format) :

    _description(PCOMN_ENSURE_ARG(description)),
    _interactive(is_probably_interactive()),
    _print_version(version_format ? version_format : ""),
    _cmdline(path::posix::split(PCOMN_ENSURE_ARG(cmdname)).second.begin())
{
    _cmdline.description(_description.c_str()) ;
    _cmdline.usage_level(CmdLine::NO_USAGE) ;

    if (version_format && *version_format)
        append(_print_version) ;
}

void CmdContext::initlog(int syslog_facility)
{
    diag_inittrace(PCOMN_BUFPRINTF(256, "%s.trace.ini", name())) ;
    openlog(name(), LOG_PID, syslog_facility) ;
}

void CmdContext::parse_cmdline(CmdLineArgIter &args) throw()
{
    try {
        do_parse_cmdline(args) ;
    }
    catch (const std::exception &x)
    {
        if (!(_cmdline.flags() & CmdLine::QUIET))
            _cmdline.error() << x.what() << std::endl ;
        exit(EXIT_USAGE) ;
    }
}

void CmdContext::do_parse_cmdline(CmdLineArgIter &args)
{
    _cmdline.parse(args) ;
}

/*******************************************************************************
 ShellContext
*******************************************************************************/
ShellContext::~ShellContext()
{
    if (!_history_file.empty())
        write_history(_history_file.c_str()) ;
}

void ShellContext::init_readline()
{
    if (!is_interactive())
        return ;

    rl_initialize() ;
    rl_readline_name = name() ;

    if (const char * const home = getenv("HOME"))
    {
        _history_file = path::abspath<std::string>
            (std::string(home).append("/.").append(name()).append(".history")) ;

        using_history() ;
        read_history(_history_file.c_str()) ;
    }
}

/*******************************************************************************
 BasicShell
*******************************************************************************/
BasicShell::BasicShell(const std::string &prompt) :
    _prompt(prompt),
    _commands("", "shell")
{
    commands()
        .append("quit", &BasicShell::exit_shell, *this, "Exit shell") ;
}

BasicShell::~BasicShell()
{}

void BasicShell::run()
{
    for (malloc_ptr<char> line ; (line.reset(readline(str::cstr(prompt()))), line) ; )
        if (*line)
        {
            add_history(line.get()) ;
            if (!exec_command(line.get()))
                break ;
        }
    std::cout << std::endl ;
}

bool BasicShell::exec_command(const char *line) throw()
{
    struct local { static void no_quit(int) { throw parse_error() ; } } ;

    string_vector args ;
    if (split_args(line, args).empty())
        return true ;

    try {
        cmdl::ArgIter<string_vector::const_iterator> argv (args.begin(), args.end()) ;

        CmdLine cmdline ("") ;
        cmdline.quit_handler(local::no_quit) ;
        commands().exec(cmdline, argv, Command::ABBREV) ;
    }
    catch(const quit_shell &) { return false ; }
    // Handle nonlocal quit from the command-line parser
    catch(const parse_error &) {}
    catch(const command_exception &x)
    {
        std::cerr << x.what() << std::endl ;
    }
    catch(const std::exception &e)
    {
        std::cerr << STDEXCEPTOUT(e) << std::endl ;
    }

    return true ;
}

void BasicShell::quit(CmdLine &cmdline, CmdLineArgIter &argv)
{
    cmdline.parse(argv) ;
    throw quit_shell() ;
}

} // end of namespace pcomn::sh
} // end of namespace pcomn
