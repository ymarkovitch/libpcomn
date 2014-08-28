/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_GETOPT_H
#define __PCOMN_GETOPT_H
/*******************************************************************************
 FILE         :   pcomn_getopt.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Helpers for command-line utilities.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   24 Nov 2008
*******************************************************************************/
/** @file
 Command line handling.

 The header defines a set of macros for handling command-line arguments. The macros
 provide a "command-line utility building kit". They assume that there are defined the
 two following functions (or function-like macros):

 @li print_usage() should print utility usage
 @li print_version() should print utility version
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcommon.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#ifdef PCOMN_PL_UNIX
#include <getopt.h>
#else
#include "platform/getopt.h"
#endif

#if !defined(EXIT_USAGE)
/// Exit code for invalid usage (bad argument, etc.)
#define EXIT_USAGE 2
#elif EXIT_USAGE != 2
#error "Invalid redefinition of EXIT_USAGE"
#endif

/// Define standard options all command-line utilities should handle.
///
/// Place this macro after all options in struct option.
#define PCOMN_DEF_STDOPTS()                     \
    { "help",       0,  NULL,   '@' },          \
    { "version",    0,  NULL,   '#' },          \
    { NULL,         0,  NULL,   0   }

#define PCOMN_SETOPT_VAL(opt, var) case opt: (var) = optarg ; break

#define PCOMN_SETOPT_FLAG(opt, var) case opt: ++(var) ; break

/// Handle standard command-line options: "help" and "version".
///
/// Assumes there are usage_exit() and print_version() functions and that option::val
/// in the "--version" long option specification is '#'
#define PCOMN_HANDLE_STDOPTS()                                          \
   case '@': print_usage() ; exit(EXIT_SUCCESS) ;                       \
   case '?': pcomn::cli::exit_invalid_arg() ; break ;                   \
   case '#': print_version() ; exit(EXIT_SUCCESS) ; break ;             \
   default:                                                             \
      fprintf(stderr, "Internal error: undefined option\n") ; \
      abort()


#define PCOMN_CHECK_SUBCOMMAND_ARG(argc, argv)                       \
   {                                                                 \
      if (argc == 1)                                                 \
         pcomn::cli::exit_invalid_arg("No command specified.") ;     \
      else                                                           \
      {                                                              \
         static const char help_opt[] = "--help" ;                   \
         static const char version_opt[] = "--version" ;             \
         /* Behave as GNU getopt_long: partial option is enough */   \
         if (strstr(help_opt, argv[1]) == help_opt)                  \
         {                                                           \
            print_usage() ;                                          \
            exit(EXIT_SUCCESS) ;                                     \
         }                                                           \
         else if (strstr(version_opt, argv[1]) == version_opt)       \
         {                                                           \
            print_version() ;                                        \
            exit(EXIT_SUCCESS) ;                                     \
         }                                                           \
      }                                                              \
   }

/// Reset all global getopt variables to allow to start parsing new argc/argv
inline void getopt_reset()
{
   optarg = NULL ;
   optind = optopt = 0 ;
}

namespace pcomn {
namespace cli {

enum IsArgRequired {
   NO_ARGUMENT          = no_argument,
   REQUIRED_ARGUMENT    = required_argument,
   OPTIONAL_ARGUMENT    = optional_argument
} ;

/// Print an error message and exit with error code due to invalid command-line
/// parameters.
inline void exit_invalid_arg(const char *message = NULL)
{
   if (message)
      fprintf(stderr, "%s\nTry %s --help for more information.\n", message, PCOMN_PROGRAM_SHORTNAME) ;
   else
      fprintf(stderr, "Try %s --help for more information.\n", PCOMN_PROGRAM_SHORTNAME) ;
   exit(EXIT_USAGE) ;
}

/// Check remaining part of the command line after the last option (i.e. the last, or
/// "not-an-option arguments" part part of the command line).
inline void check_remaining_argcount(int argc, int optind, IsArgRequired required,
                                     int minargs = 0, int maxargs = INT_MAX)
{
   switch (required)
   {
      case NO_ARGUMENT:
         if (optind < argc)
            exit_invalid_arg("Extra arguments in the command line.") ;
         break ;

      case REQUIRED_ARGUMENT:
         if (argc - optind < (minargs ? minargs : 1))
         {
            exit_invalid_arg(argc - optind ? "More arguments required" : "Argument(s) required.") ;
            break ;
         }

      case OPTIONAL_ARGUMENT:
         if (argc - optind > maxargs)
            exit_invalid_arg("Extra arguments in the command line.") ;

      default: break ;
   }
}

} // end of namespace pcomn::cli
} // end of namespace pcomn

#endif /* __PCOMN_GETOPT_H */
