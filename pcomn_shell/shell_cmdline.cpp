/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
/*******************************************************************************
 FILE         :   shell_cmdline.cpp
 COPYRIGHT    :   Yakov Markovitch, 2009-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Command-line argument handling for daemons and utilities

 CREATION DATE:   5 Jul 2009
*******************************************************************************/
#include <pcomn_shell/shell_cmdline.h>

#include <pcomn_except.h>
#include <pcomn_safeptr.h>
#include <pcomn_string.h>

#include <iostream>
#include <iomanip>
#include <iterator>
#include <string>
#include <algorithm>
#include <stdexcept>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <sysexits.h>

namespace pcomn {
namespace sh {

/*******************************************************************************
 CommandSuite
*******************************************************************************/
static const int PRINT_LMARGIN = 2 ;
static const int PRINT_MAXCOLS = 79 ;

CommandSuite::CommandSuite(const std::string &description,
                           const std::string &suite_name) :
    ancestor(description, 0),
    _suite_name(suite_name)
{
    append("help", &CommandSuite::help, *this,
           !suite_name.empty() ? "Describe " + suite_name + " subcommands" : std::string("Describe commands"),
           FLONGOPT) ;
}

CommandSuite::~CommandSuite()
{}

CommandSuite &CommandSuite::append(const std::string &name, const handler_fn &command,
                                   const std::string &description,
                                   unsigned mode_flags)
{
    if (command)
        append(name, command_ptr(new IndividualCommand(command, description, mode_flags))) ;
    return *this ;
}

CommandSuite &CommandSuite::append(const std::string &name, const command_ptr &command)
{
    if (command)
    {
        command_ptr newcmd (command) ;
        pcomn_swap(_commands[name], newcmd) ;
    }
    return *this ;
}

CommandSuite::value_type CommandSuite::get_command(const std::string &name) const
{
    const cmd_map::const_iterator result (_commands.find(name)) ;
    return result == _commands.end() ? value_type() : *result ;
}

CommandSuite::value_type CommandSuite::get_by_abbrev(const std::string &name) const
{
    const cmd_map::const_iterator &found = _commands.lower_bound(name) ;

    if (found == _commands.end() || !str::startswith(found->first, name))
        return value_type() ;

    cmd_map::const_iterator next = found ;
    return
        (name == found->first || ++next == _commands.end() || !str::startswith(next->first, name))
        ? *found : value_type(name, cmd_map::value_type::second_type()) ;
}

void CommandSuite::print_synopsis(std::ostream &os) const
{
    if (_suite_name.empty())
        return ;

    const std::string &full = description() ;
    const strslice &brief = split_description(full).first ;

    os << "Usage: " << _suite_name << " <subcommand> [OPTIONS] [ARGS]\n" ;
    if (!brief.empty())
        os << brief << '\n' ;
    os << "Type '" << _suite_name << " <subcommand> --help' for help on a specific subcommand.\n" ;
}

void CommandSuite::print_subcommands(std::ostream &os) const
{
    static const unsigned INDENT = 2 ;

    os << "\nAvailable subcommands:\n" ;
    for (cmd_map::const_iterator i = _commands.begin(), e = _commands.end() ; i != e ; ++i)
        os << std::setw(INDENT) << "" << i->first << "\n" ;
}

void CommandSuite::print_description(std::ostream &os) const
{
    const std::string &full = description() ;
    const strslice &longdesc = split_description(full).second ;
    if (!longdesc.empty())
        CmdLine::strindent(os << '\n', PRINT_MAXCOLS, 0, "", 0, longdesc.begin()) ;
}

int CommandSuite::help(CmdLine &, CmdLineArgIter &)
{
    CmdLine cmd ("help") ;
    cmd.description("Describe the usage of this program.") ;

    print_synopsis(std::cout) ;
    print_subcommands(std::cout) ;
    print_description(std::cout) ;

    std::cout << std::flush ;

    return 0 ;
}

CommandSuite::value_type
CommandSuite::extract_command(const CmdLine &cmdline, CmdLineArgIter &argv, unsigned flags) const
{
    const bool is_subcommand = cmdline.name() && *cmdline.name() ;
    const char * const name = argv() ;
    if (!name || !*name)
        throw command_exception(is_subcommand ? "No subcommand specified" : "Empty command name") ;

    const bool as_longopt = name[0] == '-' && name[1] == '-' && name[2] ;
    const char * const cmdname = as_longopt ? name + 2 : name ;

    const value_type &cmd = (flags & ABBREV) ? get_by_abbrev(cmdname) : get_command(cmdname) ;

    if (!cmd.second || // Command not found
        // Command is specified as a long option (--help) while isn't allowed
        as_longopt && !(cmd.second->mode() & FLONGOPT) ||
        // Command is found by abbreviated name while isn't allowed
        (cmd.second->mode() & FNOABBREV) && (flags & ABBREV) && cmd.first != cmdname)
    {
        const char * const format = cmd.first.empty() || as_longopt && !(cmd.second->mode() & FLONGOPT)
            ? "Unknown %s "
            : "Ambiguous %s name " ;
        throw unknown_command
            (name, PCOMN_BUFPRINTF(64, format, as_longopt ? "option" : (is_subcommand ? "subcommand" : "command"))) ;
    }

    return cmd ;
}

int CommandSuite::exec(CmdLine &cmdline, CmdLineArgIter &argv, unsigned flags) const
{
    try {
        const value_type &cmd = extract_command(cmdline, argv, flags) ;

        cmdline.name(str::cstr(cmd.first)) ;
        cmdline.usage_level(CmdLine::NO_USAGE) ;
        cmdline.description(str::cstr(cmd.second->description())) ;

        return cmd.second->exec(cmdline, argv, flags) ;
    }
    catch(const command_exception &x)
    {
        if (!(flags & ERREXIT))
            throw ;
        std::cerr << x.what() << std::endl ;
        exit(EXIT_USAGE) ;
    }
    catch(const std::exception &x)
    {
        if (!(flags & ERREXIT))
            throw ;
        std::cerr << STDEXCEPTOUT(x) << std::endl;
        exit(EXIT_FAILURE) ;
    }

    // Even though we never come here, return something to pacify the compiler
    return 0 ;
}

/*******************************************************************************
 IndividualCommand
*******************************************************************************/
int IndividualCommand::exec(CmdLine &cmdline, CmdLineArgIter &argv, unsigned) const
{
    return handler()(cmdline, argv) ;
}

/*******************************************************************************
 Global functions
*******************************************************************************/
string_vector &split_args(const char *begin, const char *end, string_vector &result)
{
    string_vector ret ;
    const char *e = begin;
    const char *b ;
    while ((b = find_if(e, end, std::bind1st(std::not_equal_to<char>(), ' '))) != end)
    {
        const bool is_quote(*b == '"' || *b == '\'' || *b == '`') ;
        if (is_quote)
        {
            const char delim(*(b++));
            e = std::find(b, end, delim);
        }
        else
            e = std::find(b, end, ' ');
        ret.push_back(std::string(b, e));
        if (is_quote)
            ++e;
    }
    pcomn_swap(ret, result) ;
    return result ;
}

strslice_pair split_description(const strslice &description)
{
    const char *eop = (const char *)memchr(description.begin(), '\n', description.size()) ;

    if (!eop)
        // Only brief
        return strslice_pair(description, strslice()) ;
    else if (eop && ++eop != description.end() && *eop++ == '\n')
        // Brief and detailed
        return strslice_pair(strslice(description.begin(), eop-2),
                             strslice(eop, description.end())) ;
    else
        // Only detailed
        return strslice_pair(strslice(), description) ;
}

} // end of namespace pcomn::sh
} // end of namespace pcomn
