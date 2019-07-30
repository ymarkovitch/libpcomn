/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   test_rm.cpp
 COPYRIGHT    :   Yakov Markovitch, 2018-2019. All rights reserved.
 DESCRIPTION  :   Testing pcomn::sys::rm()
 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   13 Aug 2018
*******************************************************************************/
#include <pcomn_shutil.h>
#include <pcomn_getopt.h>

#include <iostream>
#include <stdio.h>

// Argument map
static const char short_options[] = "inrpL" ;
static const struct option long_options[] = {
   {"ignore-errors", 0, NULL, 'i'},
   {"ignore-nexist", 0, NULL, 'n'},
   {"recursive",     0, NULL, 'r'},
   {"allow-relpath", 0, NULL, 'p'},
   {"log",           0, NULL, 'L'},

   // --help, --version
   PCOMN_DEF_STDOPTS()
} ;

static inline void print_version()
{
   printf("pcomn::sys::rm test\n\n") ;
}

static inline void print_usage()
{
   print_version() ;
   printf("Usage: %1$s [OPTIONS] PATH\n"
          "       %1$s [--help|--version]\n"
          "\n"
          "Test pcomn::sys::rm.\n"
          "\n"
          "Options:\n"
          "  -i [--ignore-errors]       RM_IGNORE_ERRORS\n"
          "  -n [--ignore-nexist]       RM_IGNORE_NEXIST\n"
          "  -r [--recursive]           RM_RECURSIVE\n"
          "  -p [--allow-relpath]       RM_ALLOW_RELPATH\n"
          "  -L [--log] ARG             use skiplogger\n"
          "  --help                 display this help and exit\n"
          "  --version              output version information and exit\n\n"
          , PCOMN_PROGRAM_SHORTNAME) ;
}

static int rm_IGNORE_ERRORS ;
static int rm_IGNORE_NEXIST ;
static int rm_RECURSIVE ;
static int rm_ALLOW_RELPATH ;

static int rm_skiplogger ;

#define FLAG(name) (pcomn::flags_if(pcomn::sys::RM_##name, rm_##name))

/*******************************************************************************

********************************************************************************/
int main(int argc, char *argv[])
{
    using namespace pcomn::sys ;

    for (int lastopt ; (lastopt = getopt_long(argc, argv, short_options, long_options, NULL)) != -1 ;)
        switch(lastopt)
        {
            PCOMN_SETOPT_FLAG('i', rm_IGNORE_ERRORS) ;
            PCOMN_SETOPT_FLAG('n', rm_IGNORE_NEXIST) ;
            PCOMN_SETOPT_FLAG('r', rm_RECURSIVE) ;
            PCOMN_SETOPT_FLAG('p', rm_ALLOW_RELPATH) ;
            PCOMN_SETOPT_FLAG('L', rm_skiplogger) ;
            PCOMN_HANDLE_STDOPTS() ;
        }
    pcomn::cli::check_remaining_argcount(argc, optind, pcomn::cli::REQUIRED_ARGUMENT, 1, 1) ;

    const char * const path = argv[optind] ;
    const RmFlags flags = FLAG(IGNORE_ERRORS)|FLAG(IGNORE_NEXIST)|FLAG(RECURSIVE)|FLAG(ALLOW_RELPATH) ;

    try {
        const rmstat &result = rm(path, flags) ;

        std::cout << (result ? "OK" : "FAILURE") << '\n'
                  << "VISITED: " << result.visited() << '\n'
                  << "SKIPPED: " << result.skipped() << '\n'
                  << "REMOVED: " << result.removed() << '\n'
                  << "BYTES:   " << result.removed_bytes() << std::endl ;
    }
    catch (const std::exception &x)
    {
        std::cout << '\n' << STDEXCEPTOUT(x) << std::endl ;
        return 1 ;
    }
}
