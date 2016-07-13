/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_EXEC_H
#define __PCOMN_EXEC_H
/*******************************************************************************
 FILE         :   pcomn_exec.h
 COPYRIGHT    :   Yakov Markovitch, 2011-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Process exec/spawn/popen.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   31 May 2011
*******************************************************************************/
/** @file
 Process exec/spawn/popen
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcomn_except.h>
#include <pcomn_unistd.h>
#include <pcomn_handle.h>

#include <stdarg.h>

namespace pcomn {

/******************************************************************************/
/** Indicates shell command execution error.
*******************************************************************************/
class _PCOMNEXP shell_error : public virtual environment_error {
   public:
      explicit shell_error(int exitcode) :
         _exit_code(exitcode)
      {
         set_message("Nonzero exit status") ;
      }
      explicit shell_error(int exitcode, const std::string &message) :
         _exit_code(exitcode)
      {
         set_message(message) ;
      }

      int exit_code() const { return _exit_code ; }
      int exit_status() const ;

   private:
      int _exit_code ;
} ;

namespace sys {

/// The default output buffer limit for a shell command run
const size_t DEFAULT_MAXSHELLOUT = 8*MiB ;

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
         return _status = pclose(p) ;
      }
} ;

/******************************************************************************/
/** Start a pipe and redirect stdin and/or stderr into it

 Destructor or close() restores original stderr and/or stdout
*******************************************************************************/
struct _PCOMNEXP redircmd {
      redircmd(const char *cmd) ;

      ~redircmd() { restore_standard_ostreams() ; }

      /// Indicate if the command pipe is closed
      bool is_closed() const { return _cmd.is_closed() ; }

      /// Wait until the command finished execution and close the command pipe.
      /// @return Exit status of the command pipe.
      ///
      int close() ;

   private:
      fd_safehandle _saved_stdout ;
      fd_safehandle _saved_stderr ;
      popencmd _cmd ;

      void restore_standard_ostreams() ;
} ;

/******************************************************************************/
/** The result type of a shell command run.

 The result is (exit_status, stdout_content)
*******************************************************************************/
typedef std::pair<int, std::string> shellcmd_result ;

/*******************************************************************************
 Functions
*******************************************************************************/
/// Execute a shell command or pipe.
///
/// @return (exit_status,stdout_content)
///
/// @param cmd       Shell command.
/// @param raise     Whether to raise exception on nonzero exist status.
/// @param out_limit The maximum size of stdout contents that can be returned from the
/// function: makes sense to restrict this value to avoid memory exhausting by
/// e.g. prining in infinite loop in @a cmd.
///
/// @throw shell_error if @a raise is RAISE_ERROR and the shell returns nonzero status;
/// in such case, shell_error::code() is the exist status, shell_error::what() is stdout
/// content.
///
_PCOMNEXP shellcmd_result shellcmd(const char *cmd, RaiseError raise, size_t out_limit = DEFAULT_MAXSHELLOUT) ;

/// @overload
inline shellcmd_result shellcmd(const std::string &cmd, RaiseError raise, size_t out_limit = DEFAULT_MAXSHELLOUT)
{
   return shellcmd(cmd.c_str(), raise, out_limit) ;
}

_PCOMNEXP shellcmd_result vshellcmd(RaiseError raise, size_t out_limit, const char *format, va_list args) ;

inline shellcmd_result vshellcmd(RaiseError raise, const char *format, va_list args)
{
   return vshellcmd(raise, DEFAULT_MAXSHELLOUT, format, args) ;
}

shellcmd_result shellcmdf(RaiseError raise, size_t out_limit, const char *format, ...) PCOMN_ATTR_PRINTF(3, 4) ;
shellcmd_result shellcmdf(RaiseError raise, const char *format, ...) PCOMN_ATTR_PRINTF(2, 3) ;

inline shellcmd_result shellcmdf(RaiseError raise, size_t out_limit, const char *format, ...)
{
   va_list args ;
   va_start(args, format) ;
   const shellcmd_result &result = vshellcmd(raise, out_limit, format, args) ;
   va_end(args) ;
   return result ;
}

inline shellcmd_result shellcmdf(RaiseError raise, const char *format, ...)
{
   va_list args ;
   va_start(args, format) ;
   const shellcmd_result &result = vshellcmd(raise, format, args) ;
   va_end(args) ;
   return result ;
}

} // end of namespace pcomn::sys
} // end of namespace pcomn

/*******************************************************************************
 Include platform header (UNIX or Windows)
*******************************************************************************/
#include PCOMN_PLATFORM_HEADER(pcomn_exec.h)

#endif /* __PCOMN_EXEC_H */
