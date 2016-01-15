/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)) -*-*/
//------------------------------------------------------------------------
// ^FILE: cmdline.cpp - implement CmdLine member functions.
//
// ^DESCRIPTION:
//    Many of the more basic member functions of a CmdLine are implemented
//    in this file. They are as follows:
//
//       Contructors
//       Destructors
//       CmdLine::name()
//       CmdLine::error()
//       CmdLine::append
//
// ^HISTORY:
//    03/21/92	Brad Appleton	<bradapp@enteract.com>	Created
//
//    03/01/93	Brad Appleton	<bradapp@enteract.com>
//    - Added cmd_nargs_parsed field to CmdLine
//    - Added cmd_description field to CmdLine
//    - Added exit_handler() and quit() member-functions to CmdLine
//-^^---------------------------------------------------------------------

#include <iostream>

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "cmdline.h"
#include "cmdargs.h"
#include "fifolist.h"
#include "states.h"

#define  va_CmdArgP(ap)  va_arg(ap, CmdArg *)

//------------------------------------------------------------------- init_args

//-------------------
// ^FUNCTION: init_args - initialize the arg_list member of a CmdLine
//
// ^SYNOPSIS:
//    init_args(list)
//
// ^PARAMETERS:
//    CmdArgListList & * list;
//    -- a reference to the list that is to be initialized.
//
// ^DESCRIPTION:
//    Allocate space for the list of command-arguments and insert
//    The default arguments onto the list.
//
// ^REQUIREMENTS:
//    list should be NULL upon entry
//
// ^SIDE-EFFECTS:
//    creates the arg-list and makes "list" point to it.
//
// ^RETURN-VALUE:
//    None.
//
// ^ALGORITHM:
//    - Create a new arg-list (sure it is NOT self-cleaning, the user is
//                             responsible for deleting inserted items)
//    - Insert the default arguments onto the list
//    - Make list point to the newly created list
//-^^----------------
static void
init_args(CmdArgListList * & list)
{
   static  CmdArgUsage  default_help('\0', "help",    "display this help and exit");

   list = new CmdArgListList ;
   list->self_cleaning(1);

   CmdArgList * arg_list = new CmdArgList;
   arg_list->self_cleaning(0);

   CmdArgList * default_list = new CmdArgList;
   default_list->self_cleaning(0);
   default_list->add(&default_help);

   list->add(arg_list);
   list->add(default_list);
}

//---------------------------------------------------------------- filebasename

//-------
// ^FUNCTION: filebasename
//
// ^SYNOPSIS:
//    static const char * filebasename(filename);
//
// ^PARAMETERS:
//    const char * filename;
//    -- the filename to get the "base" of.
//
// ^DESCRIPTION:
//    Extract and return the basename of "filename".
//
// ^REQUIREMENTS:
//    "filename" should be non-NULL and non-empty.
//
// ^SIDE-EFFECTS:
//    On VAX/VMS, MS-DOS, and OS/2 systems space is allocated (using malloc)
//    for the returned value.
//
// ^RETURN-VALUE:
//    The basename portion of the filename.
//
// ^ALGORITHM:
//    For Unix systems:
//       return everything following the rightmost '/'
//
//    For VAX/VMS systems:
//       make a copy of filename.
//       strip off any device name, any directory name.
//       strip off any "." extension.
//       strip off any version number.
//
//    For MS-DOS systems:
//       make a copy of filename.
//       strip off any drive and/or directory path.
//       strip off any "." extension.
//-^^----
static const char *
filebasename(const char * filename)
{
   if (filename == NULL)  return  filename ;

   // remove leading directory and/or drive name
   const char *p1 = ::strrchr(filename, '/');
   const char *p2 = ::strrchr(filename, '\\');
   const char * start ;

   if ((p1 == NULL) && (p2 == NULL)) {
      start = filename ;
   } else if (p1 && (p2 == NULL)) {
      start = p1 + 1;
   } else if (p2 && (p1 == NULL)) {
      start = p2 + 1;
   } else {
      start = ((p1 > p2) ? p1 : p2) + 1;
   }

   char * str = ::strcpy(new char[strlen(start) + 1], start) ;

#if (defined(msdos) || defined(os2))
   // remove the extension
   char *ext = ::strrchr(str, '.');
   if (ext)  *ext = '\0' ;
#endif /* if (msdos || os2) */

   return  str ;
}

//--------------------------------------------------------------- class CmdLine

  // Contructor with a command-name
CmdLine::CmdLine(const char * cmdname) :
   cmd_parse_state(cmd_START_STATE),
   cmd_state(cmd_START_STATE),
   cmd_flags(DEFAULT_CMDFLAGS),
   cmd_status(CmdLine::CMDSTAT_OK),
   cmd_nargs_parsed(0),
   cmd_usage_level(VERBOSE_USAGE),
   cmd_name(NULL),
   cmd_description(""),
   cmd_fulldesc(""),
   cmd_matched_arg(NULL),
   cmd_args(NULL),
   cmd_unknown_arg(NULL),
   cmd_err(NULL),
   cmd_quit_handler(NULL)
{
   name(cmdname);
   ::init_args(cmd_args);
}

   // Constructor with a name and CmdArgs
CmdLine::CmdLine(const char * cmdname, CmdArg * cmdarg1 ...) :
   CmdLine(cmdname)
{
   CmdArgListListIter  iter(cmd_args);
   CmdArgList * arg_list = iter();

   va_list  ap;
   va_start(ap, cmdarg1);
   for (CmdArg * cmdarg = cmdarg1 ; cmdarg ; cmdarg = va_CmdArgP(ap)) {
      arg_list->add(cmdarg);
   }
   va_end(ap);
}


   // Constructor with CmdArgs
CmdLine::CmdLine(CmdArg * cmdarg1 ...) :
   CmdLine((const char *)nullptr)
{
   if (!cmdarg1)  return;

   CmdArgListListIter  iter(cmd_args);
   CmdArgList * arg_list = iter();

   va_list  ap;
   va_start(ap, cmdarg1);
   for (CmdArg * cmdarg = cmdarg1 ; cmdarg ; cmdarg = va_CmdArgP(ap)) {
      arg_list->add(cmdarg);
   }
   va_end(ap);
}

   // Destructor
CmdLine::~CmdLine()
{
   delete  cmd_args;
   delete  cmd_unknown_arg;
   delete [] cmd_name;
   if (*cmd_description || *cmd_fulldesc) delete [] cmd_description ;
}

   // Set the name of the command
void
CmdLine::name(const char * progname)
{
   static  const char unknown_progname[] = "<unknown-program>" ;
   delete [] cmd_name;
   cmd_name = NULL ;
   cmd_name = ::filebasename((progname) ? progname : unknown_progname);
}

   // Print an error message prefix and return a reference to the
   // error output stream for this command
std::ostream &
CmdLine::error(unsigned  quiet) const
{
   std::ostream * os = (cmd_err) ? cmd_err : &std::cerr ;
   if (cmd_name && *cmd_name && !quiet)  *os << cmd_name << ": " ;
   return  *os;
}


  // Add an argument to the current list of CmdArgs
CmdLine &
CmdLine::append(CmdArg * cmdarg)
{
   CmdArgListListIter  iter(cmd_args);
   CmdArgList * arg_list = iter();
   arg_list->add(cmdarg);

   return  *this ;
}

   // terminate parsing altogether
void
CmdLine::quit(int status) {
   if (cmd_quit_handler != NULL) {
      (*cmd_quit_handler)(status);
   } else {
      ::exit(status);
   }
}

CmdArg *
CmdLine::get_unknown_arg() const
{
   if (!cmd_unknown_arg)
      cmd_unknown_arg = new CmdArgStr(0, "", "", "", CmdArg::isVALOPT) ;
   return cmd_unknown_arg ;
}

void
CmdLine::description(const char *new_description)
{
   if (!new_description)
      new_description = "" ;
   // Strip leading spaces
   while (isspace(*new_description)) ++new_description;
   if (!*new_description)
   {
      if (*cmd_description)
      {
         delete [] cmd_description ;
         cmd_description = "" ;
      }
      cmd_fulldesc = "" ;
      return ;
   }

   // Check for long descritpion presence
   const char * const end = new_description + strlen(new_description) ;
   const char *fdesc_begin = new_description ;
   // Search brief/long description separator "\n\n"
   while ((fdesc_begin = (const char *)memchr(fdesc_begin, '\n', end - fdesc_begin)) != NULL)
     if (*++fdesc_begin == '\n')
     {
         ++fdesc_begin ;
         break ;
     }

   const char *sdesc_end = end ;
   if (fdesc_begin)
   {
     for (sdesc_end = fdesc_begin - 2 ; sdesc_end != new_description && isspace(*(sdesc_end - 1)) ; --sdesc_end) ;
     while (isspace(*fdesc_begin)) ++fdesc_begin ;
   }
   else
     fdesc_begin = "" ;

   const size_t sdesc_len = sdesc_end - new_description ;
   const size_t fdesc_len = strlen(fdesc_begin) ;

   char *sdesc_begin ;
   if (sdesc_len + fdesc_len == 0)
      fdesc_begin = sdesc_begin = (char *)"" ;
   else
   {
      sdesc_begin = new char[sdesc_len + 1 + fdesc_len + !!fdesc_len] ;
      memcpy(sdesc_begin, new_description, sdesc_len) ;
      sdesc_begin[sdesc_len] = 0 ;
      fdesc_begin = fdesc_len ? strcpy(sdesc_begin + sdesc_len + 1, fdesc_begin) : sdesc_begin + sdesc_len ;
   }
   if (*cmd_description || *cmd_fulldesc) delete [] cmd_description ;
   cmd_description = sdesc_begin ;
   cmd_fulldesc = fdesc_begin ;
}

//---------------------------------------------------------- CmdLineCmdArgIter

   // Constructors and Destructors

CmdLineCmdArgIter::CmdLineCmdArgIter(CmdLine & cmd)
   : iter(NULL)
{
   if (cmd.cmd_args) {
      CmdArgListListIter  listlist_iter(cmd.cmd_args);
      CmdArgList  * list = listlist_iter();
      if (list)  iter = new CmdArgListIter(list);
   }
}

CmdLineCmdArgIter::CmdLineCmdArgIter(CmdLine * cmd)
   : iter(NULL)
{
   if (cmd->cmd_args) {
      CmdArgListListIter  listlist_iter(cmd->cmd_args);
      CmdArgList  * list = listlist_iter();
      if (list)  iter = new CmdArgListIter(list);
   }
}

CmdLineCmdArgIter::~CmdLineCmdArgIter()
{
   delete  iter;
}

   // Return the current argument and advance to the next one.
   // Returns NULL if we are already at the end of the list.
   //
CmdArg *
CmdLineCmdArgIter::operator()()
{
   return  (iter) ? iter->operator()() : NULL ;
}
