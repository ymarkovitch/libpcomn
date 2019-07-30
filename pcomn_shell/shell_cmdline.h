/*-*- mode: c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
#ifndef __SHELL_CMDLINE_H
#define __SHELL_CMDLINE_H
/*******************************************************************************
 FILE         :   shell_cmdline.h
 COPYRIGHT    :   Yakov Markovitch, 2009-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Command-line argument handling for DS daemons and utilities

 CREATION DATE:   5 Jul 2009
*******************************************************************************/
/** @file
  Command-line argument handling for DS daemons and utilities
*******************************************************************************/
#include <pcomn_shell/shell_cmdarg.h>

#include <pcomn_strslice.h>
#include <pcomn_smartptr.h>
#include <pcomn_meta.h>
#include <pcomn_function.h>

#include <string>
#include <vector>
#include <map>

/// Declare command implementor member function in a class derived from
/// pcomn::sh::CommandSuite
#define PCOMN_DECL_SHCMD(name) int shcmd_##name(CmdLine &, CmdLineArgIter &)

/// Append a command implemented as a member function.
#define PCOMN_ADD_SHCMD(name, desc, ...) (this->append(#name, &pcomn::valtype_t<decltype(*this)>::shcmd_##name, *this, (desc) , ##__VA_ARGS__))

#define PCOMN_REG_SHCMD(name, desc, ...)                                \
    PCOMN_DECL_SHCMD(name) ;                                            \
    pcomn::sh::CommandSuite::register_command _reg__##name {  PCOMN_ADD_SHCMD(name, (desc), ##__VA_ARGS__) }

namespace pcomn {
namespace sh {

/*******************************************************************************
 Forward declarations
*******************************************************************************/
class Command ;
class CommandSuite ;
class IndividualCommand ;

typedef shared_intrusive_ptr<Command>       CommandP ;
typedef shared_intrusive_ptr<const Command> CommandCP ;

/*******************************************************************************
 CLI exceptions
*******************************************************************************/

/******************************************************************************/
/** Base exception for all pcomn::sh excetions
*******************************************************************************/
struct command_exception : std::runtime_error {
    command_exception(const std::string &msg) :
        std::runtime_error(msg)
    {}
} ;

/******************************************************************************/
/** Invalid argument exception (invalid format or value)
*******************************************************************************/
struct cmdarg_error : public command_exception {
    typedef command_exception ancestor ;
public:
    cmdarg_error(const std::string &msg = std::string()) : ancestor(msg) {}
} ;

/******************************************************************************/
/** Invalid argument format
*******************************************************************************/
struct parse_error : public cmdarg_error {
    typedef cmdarg_error ancestor ;
public:
    parse_error(const std::string &msg = std::string()) : ancestor(msg) {}
} ;

/******************************************************************************/
/** The command is not a member of a command suite
*******************************************************************************/
class unknown_command : public command_exception {
    typedef command_exception ancestor ;
public:
    unknown_command(const std::string &cmdname,
                    const std::string &msgprefix = "Unknown command ") :
        ancestor(cmdname),
        _msg(std::string(msgprefix).append("'").append(cmdname).append("'"))
    {}

    ~unknown_command() throw() {}

    const char *name() const throw() { return ancestor::what() ; }

    const char *what() const throw() { return _msg.c_str() ; }
private:
    const std::string _msg ;
} ;

/******************************************************************************/
/** Shell command
*******************************************************************************/
struct Command : PRefCount {

    /// Command handler; may return an integer value that the Command does not interpret,
    /// simply retrurning it from exec(); this value may be used as e.g. exit code
    typedef std::function<int(CmdLine &, CmdLineArgIter &)> handler_fn ;

    /// Command exec flags, passed to exec() member function
    enum ExecFlags {
        ABBREV  = 0x0001,   /**< Allow to use nonambiguous command name abbreviations */
        ERREXIT = 0x0002,   /**< Call exit() if command name or options are invalid */
        QUIET   = 0x0003
    } ;

    /// Command mode flags persistent flags
    enum ModeFlags {
        FLONGOPT    = 0x0001,   /**< Allow to use as long option (e.g. --help) */
        FNOABBREV   = 0x0002    /**< Don't attempt to resolve an abbreviated command */
    } ;

    /// Execute either an individual command or a command from a command suite
    /// @param cmdline    A commandline object
    /// @param argv       The set of arguments; in case of a command suite the first argument
    /// may specify an individual command
    /// @param exec_flags ORed ExecFlags values
    ///
    /// @return The value returned by the corresponding handler_fn
    virtual int exec(CmdLine &cmdline, CmdLineArgIter &argv, unsigned exec_flags = 0) const = 0 ;

    const std::string &description() const { return _description ; }

    /// Get command mode flags (ORed values from ModeFlags enum)
    unsigned mode() const { return _mode ; }

protected:
    Command(const std::string &description, unsigned mode_flags) :
        _mode(mode_flags),
        _description(description)
    {}

private:
    const unsigned    _mode ;
    const std::string _description ;

    PCOMN_NONASSIGNABLE(Command) ;
    PCOMN_NONCOPYABLE(Command) ;
} ;

/******************************************************************************/
/** A set of shell commands, connected by the common domain/purpose.
*******************************************************************************/
class CommandSuite : public Command {
    typedef Command ancestor ;
public:
    typedef std::pair<const std::string, CommandP> value_type ;

    /// Construct a command suite
    /// @param suite_name The name of command suite: if this parameter is nonempty, it is
    /// assumed that the command suite describes subcommands of a command with name
    /// @a suite_name: all messages will mention "subcommands"; otherwise, all messages
    /// will mention "commands".
    explicit CommandSuite(const std::string &description = std::string(),
                          const std::string &suite_name = std::string()) ;

    ~CommandSuite() ;

    /// Parse a command line and execute a command from the command suite
    ///
    /// The first call to @a argv must return the name of the command to run. If
    /// cmdline.name() is nonempty, the command is considered "subcommand" (like
    /// e.g. "update" in "svn update").
    int exec(CmdLine &cmdline, CmdLineArgIter &argv, unsigned exec_flags = 0) const ;

    /// Append a command handler
    CommandSuite &append(const std::string &name, const handler_fn &command,
                         const std::string &description,
                         unsigned mode_flags = 0) ;

    CommandSuite &append(const std::string &name, const CommandP &command) ;

    template<typename T, typename P, typename R>
    CommandSuite &append(const std::string &name,
                         R (T::*method)(CmdLine &, CmdLineArgIter &), P &object,
                         const std::string &description,
                         unsigned mode_flags = 0)
    {
        namespace _ = std::placeholders ;
        append(name, std::bind(method, &object, _::_1, _::_2), description, mode_flags) ;
        return *this ;
    }

    value_type get_command(const std::string &name) const ;

    template<typename OutputIterator>
    OutputIterator get_by_abbrev(const std::string &prefix, OutputIterator out) const
    {

        for (cmd_map::const_iterator i (_commands.lower_bound(prefix)), e = _commands.end() ;
             i != e && str::startswith(i->first, prefix) ; *out = *i, ++i, ++out) ;
        return out ;
    }

    /// Get the command by the first few letters of the command name.
    ///
    /// The abbreviation must be unambiguous.
    /// @return (name, command), if @a prefix unambiguously defines the command;
    /// ("",NULL), if such command doesn't exist; ("Command prefix is ambiguous", NULL) if
    /// @a prefix is ambiguous.
    value_type get_by_abbrev(const std::string &prefix) const ;

    const std::string &suite_name() const { return _suite_name ; }
    void set_suite_name(const strslice &name) { name.stdstring().swap(_suite_name) ; }

protected:
    int help(CmdLine &, CmdLineArgIter &) ;

    struct register_command { register_command(CommandSuite &) {} } ;

private:
    typedef std::map<std::string, CommandP> cmd_map ;

    std::string _suite_name ;
    cmd_map     _commands ;

    value_type extract_command(const CmdLine &cmdline, CmdLineArgIter &argv, unsigned flags) const ;
    void print_synopsis(std::ostream &) const ;
    void print_subcommands(std::ostream &) const ;
    void print_description(std::ostream &) const ;
} ;

/******************************************************************************/
/** "Concrete" executable shell command.
*******************************************************************************/
class IndividualCommand : public Command {
    typedef Command ancestor ;
public:
    /// Create a command with a specified handler and descritpion
    explicit IndividualCommand(const handler_fn &handler = handler_fn(),
                               const std::string &description = {},
                               unsigned mode_flags = 0) :
        ancestor(description, mode_flags),
        _handler(handler)
    {}

    explicit IndividualCommand(const handler_fn &handler = handler_fn(),
                               unsigned mode_flags = 0) :
        ancestor({}, mode_flags),
        _handler(handler)
    {}

    /// Get the command handler
    const handler_fn &handler() const { return _handler ; }

    /// Parse a command line and execute the command
    ///
    /// @return The value returned by the handler
    /// @note The @a exec_flags parameter value is ignored
    int exec(CmdLine &cmdline, CmdLineArgIter &argv, unsigned exec_flags = 0) const ;

private:
    handler_fn  _handler ;
} ;

/******************************************************************************/
/** Execute commands over a comand suite from a line stream
*******************************************************************************/
class CommandStream {
    PCOMN_NONCOPYABLE(CommandStream) ;
    PCOMN_NONASSIGNABLE(CommandStream) ;
public:
    explicit CommandStream(CommandSuite &suite) ;

    /// Execute a single command specified as a string
    CommandStream &exec_line(strslice line) ;

    /// Execute a series of commands from a file specified by the name
    ///
    /// Opens @a filename for reading in the text mode and executes it line-by-line,
    /// then closes the file.
    CommandStream &exec_from(const strslice &filename) ;

    /// Execute a series of commands from std::istream
    CommandStream &exec_from(std::istream &is) ;
    /// @overload
    CommandStream &exec_from(std::istream &&is) { return exec_from(*static_cast<std::istream *>(&is)) ; }

    template<typename InputIterator>
    CommandStream &exec_from(InputIterator begin, InputIterator end)
    {
        std::for_each(begin, end, bind_thisptr(&CommandStream::exec_line, this)) ;
        return *this ;
    }

    template<typename InputIterator>
    std::enable_if_t<is_iterator<InputIterator>::value, CommandStream &>
    exec_from(const unipair<InputIterator> &lines)
    {
        return exec_from(std::begin(lines), std::end(lines)) ;
    }

    CommandSuite &commands() { return _commands ; }
    const CommandSuite &commands() const { return _commands ; }

    unsigned linenum() const { return _linenum ; }
    void set_linenum(unsigned num) { _linenum = num ; }

    const char *filename() const { return _filename.c_str() ; }
    void set_filename(const strslice &fn) { _filename = fn.stdstring() ; }

private:
    CommandSuite &  _commands ;
    std::string     _filename ; /* For error messages */
    unsigned        _linenum ;  /* Last executed line number */
} ;

/*******************************************************************************
 Global functions
*******************************************************************************/
/// Split an argument string into an std::string vector.
string_vector &split_args(const char *begin, const char *end, string_vector &result) ;

/// @overload
inline string_vector &split_args(const strslice &str, string_vector &result)
{
    return split_args(str.begin(), str.end(), result) ;
}
!
/// Split a description into "brief" and "long" parts.
///
/// The split is made by the first '\n' iff it is directly followed by the second
/// '\n', i.e. after the first paragraph but only if it ends with two end-of-lines in a
/// row. If there is no such paragraph, the complete description is considered "long"
/// (i.e., the  function returns ("", description)).
unipair<strslice> split_description(const strslice &description) ;

} // end of namespace pcomn::sh
} // end of namespace pcomn

#endif /* __SHELL_CMDLINE_H */
