//------------------------------------------------------------------------
// ^FILE: cmdtest.cpp - test program for the CmdLine library
//
// ^DESCRIPTION:
//    This program tests as many features of command-line as possible.
//
// ^HISTORY:
//    03/18/92	Brad Appleton	<bradapp@enteract.com>	Created
//
//    03/01/93	Brad Appleton	<bradapp@enteract.com>
//    - Attached a description to the command.
//-^^---------------------------------------------------------------------
#include "cmdargs.h"

#include <iostream>

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
                unsigned     syntax_flags = isVALOPT);

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

static CmdArgStr       str('s', "str", "[string]", "string to parse");
static CmdArgInt       debug ('D', "Debug", "[level]", "turn on debugging",
                              CmdArg::isVALSTICKY);
static CmdArgBool      infile('p', "parse", "parse from cin");

static CmdArgSet       xflag('x', "x", ";turn on X-rated mode");
static CmdArgClearRef  nxflag(xflag, 'n', "nx", ";turn off X-rated mode");
static CmdArgInt       count('c', "count", "number", "number of copies");
static CmdArgChar      delim('d', "delimiter", "char", "delimiter character");
static CmdArgChar      ext('e', "ext", "[char]", "extension to use",
                                                 CmdArg::isVALSTICKY);
static CmdArgChar      code('C', "Code", "char", "code to use",
                                                 CmdArg::isVALSTICKY);
static CmdArgStr       why('y', "why", "[reason]", "specify the reason why",
                                                   CmdArg::isVALSEP);
static CmdArgStr       who('w', "who", "logname", "the user responsible",
                                                  CmdArg::isVALSEP);
static CmdArgIntList   ints('i', "int", "number ...", "list of ints");
static CmdArgStrList   grps('g', "groups", "newsgroup", "list of newsgroups");
static CmdArgDummy     dummy("--", "denote end of options");
static CmdArgStr       name('n', "name", "name", "name of document",
                                                 CmdArg::isPOS);
static CmdArgStrList   files("[files ...]", "files to process");

//------------------------------------------------------------------ print_args

static void
print_args() {
   std::cout << "xflag=" << (xflag ? "ON" : "OFF") << std::endl ;
   std::cout << "count=" << count << std::endl ;

   unsigned  sflags = str.flags();
   if ((sflags & CmdArg::GIVEN) && (! (sflags & CmdArg::VALGIVEN))) {
      std::cout << "No string given on command-line!" << std::endl ;
   } else {
      std::cout << "str=\"" << str << "\"" << std::endl ;
   }
   std::cout << "delim='" << delim << "'" << std::endl ;
   std::cout << "ext='" << ext << "'" << std::endl ;
   std::cout << "code='" << code << "'" << std::endl ;
   std::cout << "why=\"" << why << "\"" << std::endl ;
   std::cout << "who=\"" << who << "\"" << std::endl ;

   unsigned  nints = ints.count();
   for (unsigned i = 0; i < nints ; i++) {
      std::cout << "int[" << i << "]=" << ints[i] << std::endl ;
   }

   unsigned  ngrps = grps.count();
   for (unsigned i = 0; i < ngrps ; i++) {
      std::cout << "groups[" << i << "]=\"" << grps[i] << "\"" << std::endl ;
   }

   std::cout << "name=\"" << name << "\"" << std::endl ;

   unsigned  nfiles = files.count();
   for (unsigned i = 0; i < nfiles ; i++) {
      std::cout << "files[" << i << "]=\"" << files[i] << "\"" << std::endl ;
   }
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

//------------------------------------------------------------------------ main

int
main(int argc, char * argv[]) {
   CmdLine  cmd(*argv,
                & fflag,
                & str,
                & infile,
                & debug,
                & xflag,
                & nxflag,
                & count,
                & delim,
                & ext,
                & code,
                & why,
                & who,
                & ints,
                & grps,
                & dummy,
                & name,
                & files,
                NULL);

   CmdArgvIter  argv_iter(--argc, ++argv);

   cmd.description(
"This program is intended to statically and dynamically test \
the CmdLine(3C++) class library."
   );

   std::cout << "Test of " << CmdLine::ident() << std::endl ;

   xflag = 0;
   count = 1;
   str = NULL;
   delim = '\t';
   name = NULL;

   std::cout << "Parsing the command-line ..." << std::endl ;
   unsigned status = cmd.parse(argv_iter);
   if (status)  cmd.error() << "parsing errors occurred!" << std::endl ;

   print_args();

   unsigned dbg_flags = debug.flags();
   if ((dbg_flags & CmdArg::GIVEN) && (! (dbg_flags & CmdArg::VALGIVEN))) {
      debug = 1;
   }

   dump(cmd);

   int  parse_cin = infile;

   // Parse arguments from a string
   if (! str.isNULL()) {
      CmdStrTokIter  tok_iter(str);

      xflag = 0;
      count = 1;
      str = NULL;
      delim = '\t';
      name = NULL;

      std::cout << "\n\nParsing the string ..." << std::endl ;
      status = cmd.parse(tok_iter);
      print_args();
      dump(cmd);
   }


   // Parse arguments from a file
   if (parse_cin) {
      xflag = 0;
      count = 1;
      str = NULL;
      delim = '\t';
      name = NULL;

      CmdIstreamIter  file_iter(std::cin);
      std::cout << "\n\nParsing from cin ..." << std::endl ;
      status = cmd.parse(file_iter);
      print_args();
      dump(cmd);
   }

   return  0;
}
