/*-*- tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_exec.h
 COPYRIGHT    :   Yakov Markovitch, 2011-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Process exec/popen routines for all platforms.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   1 Jun 2011
*******************************************************************************/
#include <pcomn_exec.h>
#include <pcomn_string.h>
#include <pcomn_except.h>
#include <pcomn_diag.h>

#include <pcomn_unistd.h>

#include <array>

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>

namespace pcomn {
namespace sys {

/*******************************************************************************
 popencmd
*******************************************************************************/
popencmd::popencmd(const char *cmd, char mode) :
   _cmd(PCOMN_ENSURE_ARG(cmd)),
   _pipe(popen(cmd, (std::array<char, 2>{{mode, 0}}).data())),
   _status(0)
{
   TRACEPX(PCOMN_Exec, DBGL_ALWAYS, "% " << cmd) ;

   PCOMN_ASSERT_ARG(mode == 'r' || mode == 'w') ;
   PCOMN_THROW_IF(!_pipe, pcomn::system_error, "Error attempting to run shell command '%s'", cmd) ;
}

int popencmd::close()
{
   const int status = unchecked_close() ;

   PCOMN_CHECK_POSIX(status, "Error closing pipe to shell command '%s'", _cmd.c_str()) ;
   TRACEPX(PCOMN_Exec, DBGL_NORMAL, (WIFEXITED(status) ? "Exited: " : "Signaled: ") << WEXITSTATUS(status)
           << " (" << _cmd << ')') ;

   return status ;
}

/*******************************************************************************
 redircmd
*******************************************************************************/
redircmd::redircmd(const char *cmd) :
   _saved_stdout(PCOMN_ENSURE_POSIX(dup(fileno(stdout)), "dup")),
   _saved_stderr(PCOMN_ENSURE_POSIX(dup(fileno(stderr)), "dup")),
   _cmd(cmd, 'w')
{
   const int pipeno = fileno(_cmd.pipe()) ;
   dup2(pipeno, fileno(stdout)) ;
   dup2(pipeno, fileno(stderr)) ;
}

void redircmd::restore_standard_ostreams()
{
   dup2(_saved_stdout, fileno(stdout)) ;
   dup2(_saved_stderr, fileno(stderr)) ;
   _saved_stdout.close() ;
   _saved_stderr.close() ;
}

int redircmd::close()
{
   if (!is_closed())
   {
      fflush(NULL) ;
      std::cout.flush() ;
      std::cerr.flush() ;
      restore_standard_ostreams() ;
   }
   return _cmd.close() ;
}

/*******************************************************************************
 Functions
*******************************************************************************/
shellcmd_result shellcmd(const char *cmd, RaiseError raise, size_t out_limit)
{
   popencmd runner (cmd, 'r') ;
   char buf[8192] ;
   std::string stdout_content ;

   for (size_t lastread, remains ;
        (remains = out_limit - stdout_content.size()) != 0 &&
        (lastread = fread(buf, 1, std::min(remains, sizeof buf), runner.pipe())) != 0 ;
        stdout_content.append(buf, lastread)) ;

   const int status = runner.close() ;
   if (raise && status)
   {
      if (stdout_content.empty() && (WIFEXITED(status) && WEXITSTATUS(status) == 127 || errno == ENOENT))
         // Shell failure (e.g. "No such file or directory" or like)
         stdout_content.append("Failure running the shell. Cannot run '").append(cmd).append("'") ;
      throw_exception<shell_error>(status, stdout_content) ;
   }
   return
      shellcmd_result(status, stdout_content) ;
}

shellcmd_result vshellcmd(RaiseError raise, size_t out_limit, const char *format, va_list args)
{
   PCOMN_ENSURE_ARG(format) ;
   return shellcmd(strvprintf(format, args), raise, out_limit) ;
}

} // end of namespace pcomn::sys
} // end of namespace pcomn
