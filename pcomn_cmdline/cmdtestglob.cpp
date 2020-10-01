/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)) -*-*/
/*******************************************************************************
 FILE         :   cmdtestglob.cpp
 COPYRIGHT    :   Yakov Markovitch, 2009-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   CmdLine global functions test

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   24 Feb 2009
*******************************************************************************/
#include "cmdext.h"

#include <iostream>
#include <vector>

#include <stdlib.h>
#include <ctype.h>

//---------------------------------------------------------------- CmdArgModCmd

   // CmdArgModCmd is a special argument that we use for testing.
   // The argument actually modifies the flags of the associated
   // command before it has finished parsing the arguments, hence
   // the new flags take effect for all remaining arguments.
   //
   // This argument takes a value (which is optional). If no value
   // is given, then the flags are unset, otherwise the value is
   // a list of characters, each of which corresponds to a CmdFlag
   // to turn on.
   //
class  CmdArgModCmd : public CmdArg {
public:
   CmdArgModCmd(char         optchar,
                const char * keyword,
                const char * value,
                const char * description,
                unsigned     syntax_flags =CmdArg::isVALOPT);

   virtual
   ~CmdArgModCmd();

   virtual  int
   operator()(const char * & arg, CmdLine & cmd);
} ;

CmdArgModCmd::CmdArgModCmd(char         optchar,
                           const char * keyword,
                           const char * value,
                           const char * description,
                           unsigned     syntax_flags)
   : CmdArg(optchar, keyword, value, description, syntax_flags) {}

CmdArgModCmd::~CmdArgModCmd() {}

int CmdArgModCmd::operator()(const char * & arg, CmdLine & cmd)
{
   unsigned  new_flags = 0;
   for (const char * p = arg ; *p ; p++) {
      switch (tolower(*p)) {
         case 'c' : new_flags |= CmdLine::ANY_CASE_OPTS;  break;

         case 'p' : new_flags |= CmdLine::PROMPT_USER;    break;

         case 'n' : new_flags |= CmdLine::NO_ABORT;       break;

         case 'f' : new_flags |= CmdLine::OPTS_FIRST;     break;

         case 'o' : new_flags |= CmdLine::OPTS_ONLY;      break;

         case 'k' : new_flags |= CmdLine::KWDS_ONLY;      break;

         case 'q' : new_flags |= CmdLine::QUIET;          break;

         case 'g' : new_flags |= CmdLine::GUESS;          break;

         case '+' : new_flags |= CmdLine::ALLOW_PLUS;     break;

         default  : break;
      } //switch
   } //for
   cmd.flags(new_flags);
   arg = NULL;
   return  0;
}


//------------------------------------------------------ Command Line Arguments

static CmdArgModCmd    fflag('f', "flags", "[cpnfoktqg]",
   "Use this argument to change the behavior of \
parsing for all remaining arguments.  If no \
value is given   then the command-flags are \
cleared.  Otherwise each letter specifies a flag \
to set:\n\
   'c' = any-Case-opts\n\
   'p' = Prompt-user\n\
   'n' = No-abort\n\
   'f' = options-First\n\
   'o' = Opts-only\n\
   'k' = Keywords-only\n\
   'q' = Quiet!\n\
   'g' = Guess\n\
   '+' = allow-plus\n\
This-is-a-very-long-line-containing-no-whitespace-\
characters-and-I-just-want-to-see-if-it-gets-\
formatted-appropriately!"
   );

CMDL_GLOBAL_OPT(str,    std::string,   "", 's', "str",   "[string]", "string to parse") ;
CMDL_GLOBAL_OPT(debug,  int,           0,  'D', "Debug", "[level]",  "turn on debugging",
                CmdArg::isVALSTICKY) ;

CMDL_GLOBAL_FLAG(infile, 'p', "parse", "parse from cin") ;

static CmdArgSet       xflag('x', "x", ";turn on X-rated mode");
static CmdArgClearRef  nxflag(xflag, 'n', "nx", ";turn off X-rated mode");

CMDL_GLOBAL_OPT(count,        int,              1,    'c', "count", "number", "number of copies") ;
CMDL_GLOBAL_OPT(largecount,   long long,        -13,   0,  "largecount", "number", "64-bit signed") ;
CMDL_GLOBAL_OPT(hugecount,    unsigned long long, 0,  'h',  NULL, "number", "64-bit unsigned") ;

CMDL_GLOBAL_OPT(delim, char, '\t', 'd', "delimiter", "char", "delimiter character") ;
CMDL_GLOBAL_OPT(ext,   char, '\0', 'e', "ext", "[char]", "extension to use", CmdArg::isVALSTICKY) ;
CMDL_GLOBAL_OPT(code,  char, '\0', 'C', "Code", "char", "code to use", CmdArg::isVALSTICKY) ;

CMDL_GLOBAL_OPT(why, std::string, "", 'y', "why", "[reason]", "specify the reason why",
                CmdArg::isVALSEP) ;
CMDL_GLOBAL_OPT(who, std::string, "", 'w', "who", "logname", "the user responsible",
                CmdArg::isVALSEP) ;

//CMDL_GLOBAL_OPT(ints, std::vector<int>, std::vector<int>(), 'i', "int", "number ...", "list of ints") ;

static CmdArgStrList   grps('g', "groups", "newsgroup", "list of newsgroups");
static CmdArgDummy     dummy("--", "denote end of options");

CMDL_GLOBAL_OPT(name,  std::string, "", 'n', "name", "name", "name of document", CmdArg::isPOS) ;
static CmdArgStrList   files("[files ...]", "files to process");

class local_help {} ;

namespace cmdl {
template<>
class Arg<local_help> : public CmdArgUsage {
public:
   Arg() : CmdArgUsage('\0', "help", "; print 'Hello, world!'") {}

   int operator()(const char *& , CmdLine &)
   {
      *ostream_ptr() << "\nHello, world!" << std::endl ;
      return 0 ;
   }
   local_help &value() { static local_help h ; return h ; }
} ;
}

static cmdl::Arg<local_help> hello ;
CMDL_REGISTER_GLOBAL(local_help, hello) ;

//------------------------------------------------------------------ print_args

#define OUT_NARG(name) (#name "=") << ARG_##name << '\n'
#define OUT_SARG(name) (#name "='") << ARG_##name << "'\n"

static void
print_args() {
   std::cout << "xflag=" << (xflag ? "ON" : "OFF") << '\n'
             << OUT_NARG(count)
             << OUT_NARG(largecount)
             << OUT_NARG(hugecount)
             << std::flush ;

   unsigned  sflags = str.flags();
   if ((sflags & CmdArg::GIVEN) && (! (sflags & CmdArg::VALGIVEN))) {
      std::cout << "No string given on command-line!" << std::endl ;
   } else {
      std::cout << "str=\"" << str << "\"" << std::endl ;
   }
   std::cout << OUT_SARG(delim)
             << OUT_SARG(ext)
             << OUT_SARG(code)
             << OUT_SARG(why)
             << OUT_SARG(who)
             << std::flush ;

   /*
   unsigned nints = ints.count();
   for (unsigned i = 0; i < nints ; i++) {
      std::cout << "int[" << i << "]=" << ints[i] << std::endl ;
   }
   */

   unsigned ngrps = grps.count();
   for (unsigned i = 0; i < ngrps ; i++) {
      std::cout << "groups[" << i << "]=\"" << grps[i] << "\"" << std::endl ;
   }

   std::cout << "name=\"" << name << "\"" << std::endl ;

   unsigned nfiles = files.count();
   for (unsigned i = 0; i < nfiles ; i++) {
      std::cout << "files[" << i << "]=\"" << files[i] << "\"" << std::endl ;
   }
}

//------------------------------------------------------------------------ main

int main(int argc, char * argv[])
{
   cmdl::global::set_description
      ("This program is intended to statically and dynamically test "
       "the CmdLine(3C++) class library.");

   std::cout << "Test of " << CmdLine::ident() << std::endl ;

   std::cout << "Parsing the global command-line ..." << std::endl ;
   unsigned status = cmdl::global::parse_cmdline(argc, argv) ;

   if (status)
      std::cerr << "parsing errors occurred!" << std::endl ;

   print_args() ;

   return 0 ;
}
