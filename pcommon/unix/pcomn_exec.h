/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_EXEC_UNIX_H
#define __PCOMN_EXEC_UNIX_H
/*******************************************************************************
 FILE         :   pcomn_exec.h
 COPYRIGHT    :   Yakov Markovitch, 2011-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Process exec/spawn/popen routines for UNIX/Linux platform.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   31 May 2011
*******************************************************************************/
/** @file
 Process exec/spawn/popen routines for UNIX/Linux platform
*******************************************************************************/
#include <pcommon.h>

#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>

#include <string>

namespace pcomn {
namespace sys {

/******************************************************************************/
/** Fork the current process
*******************************************************************************/
struct _PCOMNEXP forkcmd {

      explicit forkcmd(bool wait_term = true) ;

      ~forkcmd() ;

      pid_t pid() const { return _pid ; }
      bool is_child() const { return !pid() ; }

      int close() ;

   private:
      const std::string _cmd ;
      pid_t             _pid ;
      int               _status ;
      bool              _wait ;

      int terminate() ;

      PCOMN_NONCOPYABLE(forkcmd) ;
      PCOMN_NONASSIGNABLE(forkcmd) ;
} ;

/******************************************************************************/
/** Spawn a shell command
*******************************************************************************/
struct _PCOMNEXP spawncmd {

      explicit spawncmd(const std::string &cmd, bool wait_term = true) ;

      ~spawncmd() ;

      pid_t pid() const { return _pid ; }

      int close() ;

   private:
      const std::string _cmd ;
      pid_t             _pid ;
      int               _status ;
      bool              _wait ;

      int terminate() ;

      PCOMN_NONCOPYABLE(spawncmd) ;
      PCOMN_NONASSIGNABLE(spawncmd) ;
} ;

/*******************************************************************************

*******************************************************************************/
struct _PCOMNEXP netpipe : private spawncmd {
      netpipe(unsigned inport, unsigned outport, bool wait_term = true) ;
} ;

} // end of namespace pcomn::sys

/*******************************************************************************
 shell_error
*******************************************************************************/
inline int shell_error::exit_status() const { return WEXITSTATUS(_exit_code) ; }

} // end of namespace pcomn

#endif /* __PCOMN_EXEC_UNIX_H */
