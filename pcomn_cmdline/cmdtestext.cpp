/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)) -*-*/
/*******************************************************************************
 FILE         :   cmdtestext.cpp
 COPYRIGHT    :   Yakov Markovitch, 2009-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Test program for CmdLine library extensions

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   24 Feb 2009
*******************************************************************************/
#include <string>
#include <utility>
#include <iostream>
#include <stdexcept>
/*******************************************************************************
 For Arg<std::pair<> > tests
*******************************************************************************/
namespace std {
template<typename T, typename V>
inline ostream &operator<<(ostream &os, const std::pair<T, V> &v)
{
   return os << "(" << v.first << ", " << v.second << ")" ;
}
}

#include "cmdext.h"

#include <iostream>
#include <algorithm>
#include <iterator>

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

         case 'i' : new_flags |= CmdLine::SKIP_UNKNWN;    break;

         case '+' : new_flags |= CmdLine::ALLOW_PLUS;     break;

         default  : break;
      } //switch
   } //for
   cmd.flags(new_flags);
   arg = NULL;
   return  0;
}

//------------------------------------------------------ Command Line Arguments

static CmdArgModCmd    fflag('f', "flags", "[cpnfoktqgi]",
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
   'i' = Ignore-unknown\n\
   '+' = allow-plus\n\
This-is-a-very-long-line-containing-no-whitespace-\
characters-and-I-just-want-to-see-if-it-gets-\
formatted-appropriately!"
   );

static CMDL_OPT(str,    std::string,   "", 's', "str",   "[string]", "string to parse") ;
static CMDL_OPT(debug,  int,           0,  'D', "Debug", "[level]",  "turn on debugging",
                CmdArg::isVALSTICKY) ;

static CMDL_FLAG(infile, 'p', "parse", "parse from cin") ;

static CmdArgSet       xflag('x', "x", ";turn on X-rated mode");
static CmdArgClearRef  nxflag(xflag, 'n', "nx", ";turn off X-rated mode");

static CMDL_OPT(tinycount,    unsigned char,    15,   0,   "tinycount", "number", "8-bit unsigned") ;
static CMDL_OPT(count,        int,              1,    'c', "count", "number", "number of copies") ;
static CMDL_OPT(largecount,   long long,        -13,   0,  "largecount", "number", "64-bit signed") ;
static CMDL_OPT(hugecount,    unsigned long long, 0,  'h',  NULL, "number", "64-bit unsigned") ;

static CMDL_OPT(delim, char, '\t', 'd', "delimiter", "char", "delimiter character") ;
static CMDL_OPT(ext,   char, '\0', 'e', "ext", "[char]", "extension to use", CmdArg::isVALSTICKY) ;
static CMDL_OPT(code,  char, '\0', 'C', "Code", "char", "code to use", CmdArg::isVALSTICKY) ;

static CMDL_OPT(why, std::string, "", 'y', "why", "[reason]", "specify the reason why",
                CmdArg::isVALSEP) ;
static CMDL_OPT(who, std::string, "", 'w', "who", "logname", "the user responsible",
                CmdArg::isVALSEP) ;

typedef std::pair<std::string, std::string> string_keyopt ;
typedef std::pair<std::string, int> int_keyopt ;

static CMDL_OPT(option, string_keyopt, string_keyopt(), 'o', "option", "KEY=STRING", "keyed options",
                CmdArg::isVALSEP) ;

static CMDL_OPT(noption, int_keyopt, int_keyopt(), 'N', "num-option", "KEY=NUM", "numeric keyed options",
                CmdArg::isVALSEP) ;

static CMDL_LISTOPT(ints, std::vector<int>, 'i', "int", "number ...", "list of ints") ;
static CMDL_LISTOPT(grps, std::list<std::string>, 'g', "groups", "newsgroup", "list of newsgroups") ;
static CmdArgDummy dummy("--", "denote end of options") ;

static CMDL_OPT(name,  std::string, "", 'n', "name", "name", "name of document", CmdArg::isPOS) ;

static CMDL_LISTARG(files, std::set<std::string>, "[files ...]", "files to process") ;

static cmdl::ArgCounter verbosity ('v', "verbose", "verbosity level") ;

static cmdl::ArgEnum<int> msglvl (std::make_pair("std", 1),  'm',  "msglvl", "level", "none, std, verbose") ;

//------------------------------------------------------------------ print_args

#define OUT_NARG(name) (#name "=") << name << "; value=" << name.value() << '\n'
#define OUT_SARG(name) (#name "='") << name << "'\n"
#define OUT_ARG(name) (#name "=") << name << '\n'

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

             << OUT_NARG(option)
             << OUT_NARG(noption)
             << OUT_NARG(msglvl)
             << OUT_ARG(ints)
             << OUT_ARG(grps)
             << OUT_SARG(name)
             << OUT_ARG(files)
             << OUT_ARG(verbosity)
             << std::flush ;
}

//------------------------------------------------------------------------ dump

static void
dump(CmdLine & cmd)
{
   if (debug) {
      cmd.dump(std::cout);
      if (debug > 1) cmd.dump_args(std::cout);
   }
}

struct arglogger {
   std::string passed ;
   std::string skipped ;

   unsigned parse_cmdline(CmdLine &cmd, CmdLineArgIter &iter)
   {
      return cmd.parse(iter, &log, this) ;
   }
private:
   static void log(const char *arg, bool argskipped, void *self)
   {
      if (!self)
         throw std::invalid_argument("NULL 'self' passed to a logger") ;
      if (!arg)
         throw std::invalid_argument("NULL 'arg' passed to a logger") ;

      arglogger *me = static_cast<arglogger *>(self) ;
      (argskipped ? &me->skipped : &me->passed)->append(1, ' ').append(arg) ;
   }
} ;

//------------------------------------------------------------------------ main

int
main(int argc, char * argv[]) {
   msglvl
      .append("verbose", 2)
      .append(std::make_pair("none", 0)) ;

   CmdLine  cmd(*argv,
                & fflag,
                & str,
                & infile,
                & debug,
                & xflag,
                & nxflag,
                & tinycount,
                & count,
                & largecount,
                & hugecount,
                & delim,
                & ext,
                & code,
                & option,
                & noption,
                & why,
                & who,
                & msglvl,
                & ints,
                & grps,
                & dummy,
                & name,
                & files,
                & verbosity,
                NULL);

   CmdArgvIter  argv_iter(--argc, ++argv);

   cmd.description(
"   This program is intended to statically and dynamically test "
"the CmdLine(3C++) class library."
"\n\n"
"This program tests as many features of command-line as possible. "
"It presents almost all possible argument types CmdLine library supports "
"and allows to change the behavior of parsing for arguments dynamically.\n"
   );

   std::cout << "Test of " << CmdLine::ident() << std::endl ;

   std::cout << "Parsing the command-line ..." << std::endl ;
   unsigned status = cmd.parse(argv_iter);
   if (status)  cmd.error() << "parsing errors occurred!" << std::endl ;

   msglvl.append("verbose", 3) ;

   print_args();

   unsigned dbg_flags = debug.flags();
   if ((dbg_flags & CmdArg::GIVEN) && (! (dbg_flags & CmdArg::VALGIVEN))) {
      debug = 1;
   }

   dump(cmd);

   int  parse_cin = infile;

   // Parse arguments from a string
   if (str.flags() & CmdArg::VALGIVEN) {
      arglogger logger ;

      CmdStrTokIter  tok_iter(str);

      xflag = 0;
      count = 1;
      str = std::string();
      delim = '\t';
      name = std::string();

      std::cout << "\n\nParsing the string ..." << std::endl ;
      status = logger.parse_cmdline(cmd, tok_iter);
      print_args();
      dump(cmd);

      std::cout
         << "Passed:  '" << logger.passed << "'\n"
         << "Skipped: '" << logger.skipped << "'" << std::endl ;
   }


   // Parse arguments from a file
   if (parse_cin) {
      xflag = 0;
      count = 1;
      str = std::string();
      delim = '\t';
      name = std::string();

      CmdIstreamIter  file_iter(std::cin);
      std::cout << "\n\nParsing from cin ..." << std::endl ;
      status = cmd.parse(file_iter);
      print_args();
      dump(cmd);
   }

   return  0;
}
