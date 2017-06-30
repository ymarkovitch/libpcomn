/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
#ifndef __SHELL_CMDARG_H
#define __SHELL_CMDARG_H
/*******************************************************************************
 FILE         :   shell_cmdarg.h
 COPYRIGHT    :   Yakov Markovitch, 2009-2017. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Specialized command-line arguments for pcomn_shell library

 CREATION DATE:   3 Jul 2009
*******************************************************************************/
/** @file
 The most common command-line arguments for daemons and utilities
*******************************************************************************/
#include <pcomn_cmdline/cmdext.h>
#include <pcomn_platform.h>
#include <algorithm>
#include <stdio.h>

namespace pcomn {
namespace sh {

/******************************************************************************/
/** This argument prints the version information for the command on stdout and exits with
 zero code (EXIT_SUCCESS).
*******************************************************************************/
class ArgVersion : public CmdArg {
    typedef CmdArg ancestor ;
public:
    /// Constructor.
    /// @param version_format Either format string or the full version string; if this
    /// argument contains exactly one '%' character, it is considered a format string and
    /// printf'ed with the command name as argument; otherwise, it is printed as-is.
    explicit ArgVersion(const std::string &version_format, char optchar = '\0') :
        ancestor(optchar, "version", "", "Output version information and exit", 0),
        _version_format(version_format)
    {}

    ArgVersion(const std::string &version_format,
               char optchar, const char *keyword, const char *descritpion, unsigned flags = 0) :
        ancestor(optchar, keyword, "", descritpion, flags),
        _version_format(version_format)
    {}

    void print_version(const CmdLine &cmd) const
    {
        if (std::count(_version_format.begin(), _version_format.end(), '%') != 1)
            fputs(_version_format.c_str(), stdout) ;
        else
            fprintf(stdout, _version_format.c_str(), cmd.name()) ;
        fputs("\n", stdout) ;
    }

    int operator() (const char *&, CmdLine &cmd)
    {
        print_version(cmd) ;
        cmd.quit(0) ;
        return 0 ;
    }

private:
    const std::string _version_format ;
} ;

} // end of namespace pcomn::sh
} // end of namespace pcomn

#endif /* __SHELL_CMDARG_H */
