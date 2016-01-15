/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)) -*-*/
//------------------------------------------------------------------------
// ^FILE: dump.c - debugging/dumping functions of the CmdLine library
//
// ^DESCRIPTION:
//    Functions that print out debugging information for a CmdLine
//    and a CmdArg,
//
// ^HISTORY:
//    04/01/92	Brad Appleton	<bradapp@enteract.com>	Created
//
//    03/01/93	Brad Appleton	<bradapp@enteract.com>
//    - Added arg_sequence field to CmdArg
//    - Added cmd_nargs_parsed field to CmdLine
//    - Added cmd_description field to CmdLine
//-^^---------------------------------------------------------------------

#include  "cmdline.h"

# include <iostream>
# include  <string.h>

# include "fifolist.h"
# include  "states.h"

   // Indent a line corresponding to a given indent level.
   // The number of spaces to indent is 3x the indent level
   //
static std::ostream &
indent(std::ostream & os, unsigned level)
{
   os.width(level * 3);
   return  (os << "");
}

   // Dump the arg_syntax field of a CmdArg in a mnemonic format
   //
static std::ostream &
dump_arg_syntax(std::ostream & os, unsigned  syntax)
{
   os << ((syntax & CmdArg::isREQ) ? "isREQ" : "isOPT") ;

   if (syntax & CmdArg::isVALREQ) {
      os << " | isVALREQ" ;
   }
   if (syntax & CmdArg::isVALOPT) {
      os << " | isVALOPT" ;
   }
   if (syntax & CmdArg::isVALSEP) {
      os << " | isVALSEP" ;
   }
   if (syntax & CmdArg::isVALSTICKY) {
      os << " | isVALSTICKY" ;
   }
   if (syntax & CmdArg::isLIST) {
      os << " | isLIST" ;
   }
   if (syntax & CmdArg::isPOS) {
      os << " | isPOS" ;
   }
   if (syntax & CmdArg::isHIDDEN) {
      os << " | isHID" ;
   }

   return  os;
}


   // Dump the arg_flags field of a CmdArg in a mnemonic format
static std::ostream &
dump_arg_flags(std::ostream & os, unsigned  flags)
{
   if (flags & CmdArg::GIVEN) {
      os << "GIVEN" ;
   } else {
      os << "NOTGIVEN";
   }
   if (flags & CmdArg::VALGIVEN) {
      os << " | VALGIVEN";
   }
   if (flags & CmdArg::OPTION) {
      os << " | OPTION";
   }
   if (flags & CmdArg::KEYWORD) {
      os << " | KEYWORD";
   }
   if (flags & CmdArg::POSITIONAL) {
      os << " | POSITIONAL";
   }
   if (flags & CmdArg::VALSEP) {
      os << " | VALSEP";
   } else if (flags & CmdArg::VALGIVEN) {
      os << " | VALSAME";
   }

   return  os;
}

#define CMD_OUT_FLAG(flag) ((val & CmdLine::flag) && (os << " | "#flag))

   // Dump the cmd_flags field of a CmdLine in a mnemonic format
static std::ostream &
dump_cmd_flags(std::ostream & os, unsigned flags)
{
   if (flags & CmdLine::NO_ABORT) {
      os << "NO_ABORT" ;
   } else {
      os << "ABORT" ;
   }
   const unsigned val = flags ;
   CMD_OUT_FLAG(ANY_CASE_OPTS) ;
   CMD_OUT_FLAG(PROMPT_USER) ;
   CMD_OUT_FLAG(OPTS_FIRST) ;
   CMD_OUT_FLAG(OPTS_ONLY) ;
   CMD_OUT_FLAG(KWDS_ONLY) ;
   CMD_OUT_FLAG(QUIET) ;
   CMD_OUT_FLAG(GUESS) ;
   CMD_OUT_FLAG(ALLOW_PLUS) ;
   CMD_OUT_FLAG(SKIP_UNKNWN) ;

   return  os;
}

   // Dump the status of a CmdLine in a mnemonic format
static std::ostream &
dump_cmd_status(std::ostream & os, unsigned  status)
{
   if (! status) {
      os << "CMDSTAT_OK";
      return  os;
   } else {
      os << "ERROR";
   }
   const unsigned val = status ;
   CMD_OUT_FLAG(ARG_MISSING) ;
   CMD_OUT_FLAG(VAL_MISSING) ;
   CMD_OUT_FLAG(VAL_NOTSTICKY) ;
   CMD_OUT_FLAG(VAL_NOTSEP) ;
   CMD_OUT_FLAG(KWD_AMBIGUOUS) ;
   CMD_OUT_FLAG(BAD_OPTION) ;
   CMD_OUT_FLAG(BAD_KEYWORD) ;
   CMD_OUT_FLAG(BAD_VALUE) ;
   CMD_OUT_FLAG(TOO_MANY_ARGS) ;

   return  os;
}

#undef CMD_OUT_FLAG

   // Dump the state of a CmdLine in a mnemonic format
static std::ostream &
dump_cmd_state(std::ostream & os, unsigned  state)
{
   if (! state) {
      os << "NO_OPTIONS";
   } else {
      os << "ARGS";
   }
   if (state & cmd_END_OF_OPTIONS) {
      os << " | ENDOPTS";
   }
   if (state & cmd_OPTIONS_USED) {
      os << " | OPTS_USED";
   }
   if (state & cmd_KEYWORDS_USED) {
      os << " | KWDS_USED";
   }
   if (state & cmd_GUESSING) {
      os << " | GUESSING";
   }

   return  os;
}


   // Dump the parse_state of a CmdLine in a mnemonic format
static std::ostream &
dump_cmd_parse_state(std::ostream & os, unsigned parse_state)
{
   switch (parse_state) {
   case cmd_START_STATE :
      os << "START_STATE";
      break;

   case cmd_TOK_REQUIRED :
      os << "TOK_REQUIRED";
      break;

   case cmd_WANT_VAL :
      os << "WANT_VAL";
      break;

   case cmd_NEED_VAL :
      os << "NEED_VAL";
      break;

   default :
      os << parse_state;
   }

   return  os;
}


   // Dump the arguments (including the default arguments) in an arg_list
static std::ostream &
dump_cmd_args(std::ostream & os, CmdArgListList * arg_list, unsigned level)
{
   ::indent(os, level) << "CmdLine::cmd_args {\n" ;

   CmdArgListListIter  list_iter(arg_list);
   for (CmdArgList * alist = list_iter() ; alist ; alist = list_iter()) {
      CmdArgListIter iter(alist);
      for (CmdArg * cmdarg = iter() ; cmdarg ; cmdarg = iter()) {
         cmdarg->dump(os, level + 1);
      }
   }

   ::indent(os, level) << "}" << std::endl;
   return  os;
}

   // Dump a CmdArg
void
CmdArg::dump(std::ostream &os, unsigned level) const
{
   ::indent(os, level) << "CmdArg {\n" ;

   ::indent(os, level + 1) << "option='" << char(arg_char_name) << "'" ;
   if (arg_keyword_name)
      os << ", keyword=\"" << arg_keyword_name << '\"' ;
   if (arg_value_name)
      os << ", value=\"" << arg_value_name << "\"\n" ;

   ::indent(os, level + 1) << "syntax=" ;
   dump_arg_syntax(os, arg_syntax) << "\n";

   ::indent(os, level + 1) << "flags=" ;
   dump_arg_flags(os, arg_flags) << "\n";

   ::indent(os, level + 1) << "sequence=" << arg_sequence << "\n";

   ::indent(os, level) << "}" << std::endl;
}


   // Dump a CmdLine
void
CmdLine::dump(std::ostream &os, unsigned level) const
{
   ::indent(os, level) << "CmdLine {\n" ;

   ::indent(os, level + 1) << "name=\"" << cmd_name << "\"\n";

   ::indent(os, level + 1) << "description=\"" << description() << "\"\n";

   ::indent(os, level + 1) << "fulldesc=\"" << full_description() << "\"\n";

   ::indent(os, level + 1) << "flags=" ;
   dump_cmd_flags(os, cmd_flags) << "\n";

   ::indent(os, level + 1) << "status=" ;
   dump_cmd_status(os, cmd_status) << "\n";

   ::indent(os, level + 1) << "state=" ;
   dump_cmd_state(os, cmd_state) << "\n";

   ::indent(os, level + 1) << "parse_state=" ;
   dump_cmd_parse_state(os, cmd_parse_state) << "\n";

   ::indent(os, level + 1);
   if (cmd_matched_arg == NULL) {
      os << "matched_arg=NULL\n";
   } else {
      os << "matched_arg=" << (void *)cmd_matched_arg << "\n";
   }

   ::indent(os, level + 1) << "# valid-args-parsed=" << cmd_nargs_parsed << "\n" ;

   ::indent(os, level) << "}" << std::endl;
}

   // Dump the arguments of a CmdLine
void
CmdLine::dump_args(std::ostream &os, unsigned level) const
{
   dump_cmd_args(os, cmd_args, level);
}
