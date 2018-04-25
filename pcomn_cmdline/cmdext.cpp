/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)) -*-*/
/*******************************************************************************
 FILE         :   cmdext.cpp
 COPYRIGHT    :   Yakov Markovitch, 2009-2017. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Extensions for Brad Aplleton's CmdLine library

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   24 Feb 2009
*******************************************************************************/
#include "cmdext.h"

#ifdef __noinline
#undef __noinline
#endif
#ifdef _MSC_VER
#define __noinline __declspec(noinline)
#else
#define __noinline __attribute__((__noinline__))
#endif

namespace cmdl {

static __noinline CmdLine &global_cmdline()
{
   static CmdLine cmdline ;
   return cmdline ;
}

namespace global {

void set_description(const char *desc)
{
   global_cmdline().description(desc) ;
}

void set_name(const char *name)
{
   global_cmdline().name(name) ;
}

const char *get_name()
{
   return global_cmdline().name() ;
}

CmdArg &register_arg(CmdArg &arg)
{
   global_cmdline().append(arg) ;
   return arg ;
}

unsigned parse_cmdline(int argc, const char * const *argv,
                       unsigned flags, unsigned mask,
                       CmdLine::arglogger logger, void *logger_data)
{
   if (!argc || !argv || !*argv)
      return (unsigned)-1 ;

   struct local {
      local(const char *progname) : saved_name(), saved_flags(global_cmdline().flags())
      {
         if (!*global_cmdline().name() || *global_cmdline().name() == '<')
         {
            saved_name = strcpy(new char[strlen(global_cmdline().name()) + 1], global_cmdline().name()) ;
            global_cmdline().name(progname) ;
         }
      }
      ~local()
      {
         if (saved_name)
         {
            global_cmdline().name(saved_name) ;
            delete [] saved_name ;
         }
         global_cmdline().flags(saved_flags) ;
      }
      const char *saved_name ;
      unsigned    saved_flags ;
   } sentry (*argv) ;

   global_cmdline().set(flags & mask) ;
   global_cmdline().clear(~flags & mask) ;

   CmdArgvIter argv_iter (--argc, ++argv) ;
   return global_cmdline().parse(argv_iter, logger, logger_data) ;
}

unsigned parse_cmdline(int argc, const char * const *argv)
{
   return parse_cmdline(argc, argv, 0, 0) ;
}

} // end of namespace cmdl::global
} // end of namespace cmdl
