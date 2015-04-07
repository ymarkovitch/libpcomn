/*-*- mode: c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
#ifndef __SHELL_REPL_H
#define __SHELL_REPL_H
/*******************************************************************************
 FILE         :   shell_repl.h
 COPYRIGHT    :   Yakov Markovitch, 2009-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Interactive command-line shell.

 CREATION DATE:   5 Jul 2009
*******************************************************************************/
/** @file
  Interactive command-line shell, implements REPL (Read-Execute-Print Loop).
*******************************************************************************/
#include <pcomn_shell/shell_cmdline.h>
#include <pcomn_unistd.h>

namespace pcomn {
namespace sh {

/*******************************************************************************
                            class Verbosity
*******************************************************************************/
class Verbosity {
public:
    Verbosity(int level = 0) : _verbosity(level) {}

    int verbosity() const { return _verbosity ; }
    void set_verbosity(int level) { _verbosity = level ; }

    int logfd(int minlevel = 0, int fd = STDOUT_FILENO) const
    {
        return verbosity() >= minlevel ? fd : -1 ;
    }
private:
    int _verbosity ;
} ;

/******************************************************************************/
/** Base class of command-line contexts for handling servers (daemons) command line
*******************************************************************************/
class CmdContext : public Verbosity {
public:
    virtual ~CmdContext() {}

    /// Get the command name
    const char *name() const { return _cmdline.name() ; }

    /// Check whether the command started in interactive mode
    ///
    /// Interactive mode assumes the command started in foreground and stdin is a tty or
    /// a pipe.
    virtual bool is_interactive() const { return _interactive ; }

    void initlog(int syslog_facility) ;

    void parse_cmdline(int argc, char * const argv[]) throw()
    {
        CmdArgvIter args (argc, argv) ;
        parse_cmdline(args) ;
    }

    void parse_cmdline(CmdLineArgIter &args) throw() ;

    CmdContext &append(CmdArg &arg)
    {
        _cmdline.append(arg) ;
        return *this ;
    }

protected:
    CmdContext(const char *cmdname, const char *description, const char *version_format) ;

    // Parse the command line and handle common arguments (like '--interactive')
    virtual void do_parse_cmdline(CmdLineArgIter &args) ;

    const CmdLine &cmdline() const { return _cmdline ; }
    CmdLine &cmdline() { return _cmdline ; }

private:
    const std::string   _cmdpath ;
    const std::string   _description ;
    const bool          _interactive ;

    ArgVersion          _print_version ;
    CmdLine             _cmdline ;
} ;

/*******************************************************************************

*******************************************************************************/
class ShellContext : public CmdContext {
    typedef CmdContext ancestor ;
public:
    ~ShellContext() ;

    /// Initialize 'readline' (resp. 'editline') and 'history' interactive input
    /// libraries.
    ///
    /// If is_interactive() is false, the call to this method is dummy.
    void init_readline() ;

protected:
    ShellContext(const char *cmdname, const char *description, const char *version_format) :
        ancestor(cmdname, description, version_format)
    {}

private:
    std::string _history_file ;
} ;

/******************************************************************************/
/** Interactive command-line shell.

 Provides a foundation for building specialized shells. Implements REPL
 (Read-Execute-Print Loop), as well as 'help' command, and more.
*******************************************************************************/
class BasicShell {
    PCOMN_NONCOPYABLE(BasicShell) ;
    PCOMN_NONASSIGNABLE(BasicShell) ;
public:
    BasicShell(const std::string &prompt = "> ") ;
    virtual ~BasicShell() ;

    void run() ;

    /// @return true - continue running REPL loop, false - exit the shell
    bool exec_command(const char *line) throw() ;

    /// Get the prompt
    virtual const std::string &prompt() const { return _prompt ; }

    /// Set the prompt
    /// @note This method can be called from a command handler, allowing to change the
    /// prompt dynamically.
    virtual void set_prompt(const std::string &p) { _prompt = p ; }

    CommandSuite &commands() { return _commands ; }

protected:
    /// @note Must not return, should either throw an exception (quit_shell), or call
    /// exit(), or abort()
    virtual void quit(CmdLine &, CmdLineArgIter &) ;

private:
    std::string     _prompt ;
    CommandSuite    _commands ;

    int exit_shell(CmdLine &cmdl, CmdLineArgIter &argv)
    {
        quit(cmdl, argv) ;
        return 0 ;
    }
} ;

} // end of namespace pcomn::sh
} // end of namespace pcomn

#endif /* __SHELL_REPL_H */
