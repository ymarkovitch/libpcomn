/*-*- tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_exec.h
 COPYRIGHT    :   Yakov Markovitch, 2011-2018. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Process exec/spawn/popen routines for UNIX/Linux platform.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   1 Jun 2011
*******************************************************************************/
#include <pcomn_exec.h>
#include <pcomn_string.h>
#include <pcomn_except.h>
#include <pcomn_diag.h>

#include <array>

#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

namespace pcomn {
namespace sys {

namespace { struct chrstr : std::array<char, 2> { chrstr(char c) { (*this)[0] = c ; (*this)[1] = 0 ; } } ; }

/*******************************************************************************
 popencmd
*******************************************************************************/
popencmd::popencmd(const char *cmd, char mode) :
   _cmd(PCOMN_ENSURE_ARG(cmd)),
   _pipe(popen(cmd, chrstr(mode).begin())),
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
 forkcmd
*******************************************************************************/
forkcmd::forkcmd(bool wait_term) :
   _pid(fork()),
   _status(0),
   _wait(wait_term)
{
   if (pid())
   {
      std::cout << "Forked " << pid() << std::endl ;
      PCOMN_THROW_MSG_IF(pid() < 0, std::runtime_error, "Error while forking: %s", strerror(errno)) ;
   }
}

forkcmd::~forkcmd()
{
   if (pid() <= 0)
      return ;
   terminate() ;
}

int forkcmd::close()
{
   PCOMN_THROW_MSG_IF(!pid(), std::runtime_error, "Child is already terminated") ;
   PCOMN_THROW_MSG_IF(terminate() < 0, std::runtime_error, "Error while terminating: %s", strerror(errno)) ;
   return _status ;
}

int forkcmd::terminate()
{
   if (pid() <= 0)
      return 0 ;
   pid_t p = _pid ;
   _pid = 0 ;
   if (_wait)
      return waitpid(p, &_status, 0) ;
   if (int result = waitpid(p, &_status, WNOHANG))
      return result ;
   std::cout << "Killing " << p << std::endl ;
   if (int result = kill(p, SIGTERM))
      return result ;
   return waitpid(p, &_status, 0) ;
}

/*******************************************************************************
 spawncmd
*******************************************************************************/
spawncmd::spawncmd(const std::string &cmd, bool wait_term) :
   _cmd(cmd),
   _pid(fork()),
   _status(0),
   _wait(wait_term)
{
   if (pid())
   {
      std::cout << "Spawned " << pid() << std::endl ;
      PCOMN_THROW_MSG_IF(pid() < 0, std::runtime_error, "Error attempting to spawn shell command '%s': %s", cmd.c_str(), strerror(errno)) ;
   }
   else
   {
      setsid() ;
      execl("/bin/sh", "sh", "-c", cmd.c_str(), NULL) ;
      exit(127) ;
   }
}

spawncmd::~spawncmd()
{
   if (pid() <= 0)
      return ;
   terminate() ;
}

int spawncmd::close()
{
   PCOMN_THROW_MSG_IF(!pid(), std::runtime_error, "Child is already terminated") ;
   PCOMN_THROW_MSG_IF(terminate() < 0, std::runtime_error, "Error terminating shell command '%s': %s", _cmd.c_str(), strerror(errno)) ;
   PCOMN_THROW_MSG_IF(_status == 127, std::runtime_error, "Failure running the shell. Cannot run shell command '%s'", _cmd.c_str()) ;
   return _status ;
}

int spawncmd::terminate()
{
   if (pid() <= 0)
      return 0 ;
   pid_t p = _pid ;
   _pid = 0 ;
   if (_wait)
      return waitpid(p, &_status, 0) ;
   if (int result = waitpid(p, &_status, WNOHANG))
      return result ;
   std::cout << "Killing " << p << std::endl ;
   if (int result = kill(-getpgid(p), SIGTERM))
      return result ;
   return waitpid(p, &_status, 0) ;
}

/*******************************************************************************
 netpipe
*******************************************************************************/
netpipe::netpipe(unsigned inport, unsigned outport, bool wait_term) :
   spawncmd(strprintf("nc -vv -l -p %u | tee /dev/stderr | nc -vv localhost %u", inport, outport))
{
   sleep(1) ;
}

} // end of namespace pcomn::sys
} // end of namespace pcomn
