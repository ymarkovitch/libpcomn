/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_EXEC_UNIX_H
#define __PCOMN_EXEC_UNIX_H
/*******************************************************************************
 FILE         :   pcomn_exec.h
 COPYRIGHT    :   Yakov Markovitch, 2011-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Process exec/spawn/popen routines for UNIX/Linux platform.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   31 May 2011
*******************************************************************************/
/** @file
 Process exec/spawn/popen routines for UNIX/Linux platform
*******************************************************************************/
#include <pcomn_def.h>

#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>

#include <string>

namespace pcomn {
namespace sys {

/******************************************************************************/
/** Popen object.
*******************************************************************************/
struct _PCOMNEXP popencmd {
      /// Create a command pipe.
      /// @param cmd    Shell command; may contain pipe redirections, etc.
      /// @param mode   Pipe mode: 'r' - read from the command's stdout, 'w' - write to
      /// stdin.
      ///
      popencmd(const char *cmd, char mode = 'r') ;

      ~popencmd() { unchecked_close() ; }

      FILE *pipe() { return _pipe ; }

      /// Indicate if the command pipe is closed
      bool is_closed() const { return !_pipe ; }

      /// Wait until the command finished execution and close the command pipe.
      /// @return Exit status of the command pipe.
      ///
      int close() ;

   private:
      const std::string _cmd ;
      FILE *            _pipe ;
      int               _status ;

      int unchecked_close()
      {
         if (is_closed())
            return _status ;
         FILE *p = _pipe ;
         _pipe = NULL ;
         return _status = ::pclose(p) ;
      }
} ;

} // end of namespace pcomn::sys

/*******************************************************************************
 shell_error
*******************************************************************************/
inline int shell_error::exit_status() const { return WEXITSTATUS(_exit_code) ; }

} // end of namespace pcomn

#endif /* __PCOMN_EXEC_UNIX_H */
