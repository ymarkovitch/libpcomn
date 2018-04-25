/*-*- tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_trace.cpp
 COPYRIGHT    :   Yakov Markovitch, 1995-2017. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Tracing engine

 CREATION DATE:   12 May 1995
*******************************************************************************/
#define DIAG_DEFGROUP_DECLARED  /* Prevent pcomn_trace.h from declaring Def diag group. */
#include <pcomn_trace.h>
#include <pcomn_atomic.h>
#include <pcomn_algorithm.h>
#include <pcomn_utils.h>
#include <pcomn_unistd.h>
#include <pcstring.h>

#include <new>
#include <iomanip>
#include <utility>
#include <thread>

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>

#if defined(PCOMN_PL_WINDOWS)
#  include <windows.h>

#  define PCOMN_CRITICAL_SECTION(name) CRITICAL_SECTION name
#  define INIT_CRITICAL_SECTION(name) (InitializeCriticalSection (&name))
#  define DEL_CRITICAL_SECTION(name) (DeleteCriticalSection (&name))
#  define ENTER_CRITICAL_SECTION(name) (EnterCriticalSection (&name))
#  define LEAVE_CRITICAL_SECTION(name) (LeaveCriticalSection (&name))

#else
#include <pthread.h>

#  define PCOMN_CRITICAL_SECTION(name) pthread_mutex_t name
#  define INIT_CRITICAL_SECTION(name) {const pthread_mutex_t __##name = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP ; name = __##name ;}
#  define DEL_CRITICAL_SECTION(name) (pthread_mutex_destroy(&name))
#  define ENTER_CRITICAL_SECTION(name) (pthread_mutex_lock(&name))
#  define LEAVE_CRITICAL_SECTION(name) (pthread_mutex_unlock(&name))

#endif

#ifdef PCOMN_PL_WINDOWS
/*******************************************************************************
 Windows
*******************************************************************************/
#include <windows.h> // OutputDebugString, GetFileInformationByHandle
static inline int get_last_error() { return GetLastError() ; }
static inline void set_last_error(int err) { SetLastError(err) ; }

static bool check_diag_fd(int fd)
{
   BY_HANDLE_FILE_INFORMATION finfo = {} ;

   const DWORD BAD_ATTRS = FILE_ATTRIBUTE_READONLY
      | FILE_ATTRIBUTE_INTEGRITY_STREAM | FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_OFFLINE
      | FILE_ATTRIBUTE_SYSTEM ;

   if (!GetFileInformationByHandle((HANDLE)_get_osfhandle(fd), &finfo))
      perror("Failure while setting diagnostics trace stream") ;
   else if (finfo.dwFileAttributes & BAD_ATTRS)
   {
      static const char errtxt[] = "Failure while setting diagnostics log: file descriptor does not allow writing.\n" ;
      ::write(STDERR_FILENO, errtxt, sizeof errtxt - 1) ;
   }
   else
      return true ;
   return false ;
}

#else
/*******************************************************************************
 Unix
*******************************************************************************/
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <syslog.h>
#include <errno.h>
static inline int get_last_error() { return errno ; }
static inline void set_last_error(int err) { errno = err ; }

static bool check_diag_fd(int fd)
{
   const int flags = fcntl(fd, F_GETFL) ;
   if (flags == -1)
      perror("Failure while setting diagnostics trace stream") ;
   else if ((flags & O_ACCMODE) != O_WRONLY && (flags & O_ACCMODE) != O_RDWR)
   {
      static const char errtxt[] = "Failure while setting diagnostics log: file descriptor does not allow writing.\n" ;
      ::write(STDERR_FILENO, errtxt, sizeof errtxt - 1) ;
   }
   else
      return true ;
   return false ;
}

#endif

namespace diag {

/*******************************************************************************
 PDiagBase: groups
*******************************************************************************/
// A vector of diagnostic groups' properties
static struct {

      PDiagBase::Properties * groups[MaxGroupsNum] ;
      bool                    sorted ;
      unsigned                num ;

} diag_props = {

   { NULL },
   true,
   0
} ;

static unsigned global_mode ;

static constexpr PTraceSuperGroup dummy_supergroup ;
const PTraceSuperGroup &PDiagBase::null_supergroup = dummy_supergroup ;

PDiagBase::PDiagBase(Properties *grp)
{
   if (grp && numOfGroups() < diag::MaxGroupsNum)
   {
      diag_props.groups[diag_props.num++] = grp ;
      diag_props.sorted = false ;
   }
}

PDiagBase::Properties *PDiagBase::get(const char *name)
{
   typedef int (*qsort_compare)(const void *, const void *) ;
   struct local {
         static int pnamecmp(const Properties * const *t1, const Properties * const *t2)
         {
            return strcmp ((*t1)->name(), (*t2)->name()) ;
         }
   } ;

   Properties key = { (char *)name, false, 0 } ;
   Properties *pkey = &key ;

   if (!diag_props.sorted)
   {
      qsort(begin(), diag_props.num, sizeof pkey, (qsort_compare)local::pnamecmp) ;
      diag_props.sorted = true ;
   }

   Properties **result =
      (Properties **)bsearch(&pkey, begin(), diag_props.num, sizeof pkey, (qsort_compare)local::pnamecmp) ;

   return result ? *result : NULL ;
}

unsigned PDiagBase::numOfGroups()
{
   return diag_props.num ;
}

PDiagBase::Properties **PDiagBase::begin()
{
   return diag_props.groups ;
}

PDiagBase::Properties **PDiagBase::end()
{
   return diag_props.groups+diag_props.num ;
}

void PDiagBase::mode(unsigned flg, bool onOff)
{
   if (onOff)
      global_mode |= flg ;
   else
      global_mode &= ~flg ;
}

unsigned PDiagBase::mode() { return global_mode ; }

/*******************************************************************************
 PDiagBase: trace context initialization and locking
*******************************************************************************/
static constexpr const unsigned DIAG_LOGMAXPATH  = 2048 ;
static constexpr const unsigned DIAG_MAXMESSAGE  = 4096 ; /* Buffer size for diagnostic messages */
static constexpr const unsigned DIAG_MAXPREFIX   = 256 ;

// How often (in seconds) diag_isenabled_diag() should check if the tracing
// configuration changed.
constexpr unsigned long long DIAG_CFGCHECK_INTERVAL = 2 ;

/*******************************************************************************
 Temporary diagnostics streamm, never allocates its buffer dynamically.
*******************************************************************************/
typedef pcomn::bufstr_ostream<DIAG_MAXMESSAGE - DIAG_MAXPREFIX> trace_stream ;

/*******************************************************************************
 Default debug log writer and syslog writer
*******************************************************************************/
static void output_debug_msg(void *, const char *msg) ;
static void output_syslog_msg(void *, LogLevel level, const char *fmt, ...) ;
static void output_fdlog_msg(void *fd, LogLevel level, const char *fmt, ...) ;

/*******************************************************************************
 Trace context, holds tracing stream.
 This context is thread-local and is initiated once per thread first time it is used in
 that thread.
*******************************************************************************/
struct trace_context {

      trace_stream &os()
      {
         if (!os_ptr)
         {
            os_ptr = new (static_cast<void *>(os_buf + 0)) trace_stream ;
            *os_ptr << std::boolalpha ;

            // Store initial foramtting settings, restored every time
            // PDiagBase::stream(true) is called.
            os_fmtflags = os_ptr->flags() ;
            os_precision = os_ptr->precision() ;
            os_width = os_ptr->width() ;
         }
         return *os_ptr ;
      }

      int            tracing ; /* Indicator to prevent recursive tracing */

      trace_stream * os_ptr ;  /* Stream that output string is formed in.
                                * Allocated in os_buf */

      char           os_buf[sizeof(trace_stream)] ; /* The buffer in which the _os stream is allocated by
                                                     * the "placement new" operator.
                                                     * Properly aligned (follows the os_ptr pointer). */
      std::ios_base::fmtflags os_fmtflags ;
      size_t                  os_precision ;
      size_t                  os_width ;

      /*************************************************************************
       Global trace context
      *************************************************************************/
      static int  log_fd ;                      /* Logging ostream per se */
      static bool log_owned ;                   /* Is it necessary to close fd */
      static char log_name[DIAG_LOGMAXPATH] ;   /* Logname (may also be a "special" name,
                                                 * like "stderr", etc) */
      static dbglog_writer dbglog_write ;
      static void *        dbglog_data ;

      static syslog_writer syslog_write ;
      static void *        syslog_data ;
      static const char *  syslog_ident ;

      static time_t        last_cfgcheck ; /* The time of last configuration check */
      static struct stat   last_cfgstat ;

      #ifdef PCOMN_PL_WINDOWS
      static CRITICAL_SECTION &context_mutex()
      {
         static CRITICAL_SECTION mutex ;
         static CRITICAL_SECTION *result = (InitializeCriticalSection(&mutex), &mutex) ;
         return mutex ;
      }

      #else

      static pthread_mutex_t &context_mutex()
      {
         static pthread_mutex_t mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP ;
         return mutex ;
      }
      #endif

      static void LOCK() { ENTER_CRITICAL_SECTION(context_mutex()) ; }
      static void UNLOCK() { LEAVE_CRITICAL_SECTION(context_mutex()) ; }

      static bool needs_configuration_check()
      {
         return (unsigned long long)(time(NULL) - last_cfgcheck) >= DIAG_CFGCHECK_INTERVAL ;
      }

      static bool is_configuration_changed()
      {
         const char *cfgpathname = PTraceConfig::profileFileName() ;
         typedef struct stat stat_t ;
         stat_t st ;
         auto mtimep =
         #ifdef PCOMN_PL_LINUX
            &stat_t::st_mtim
         #else
            &stat_t::st_mtime
         #endif
            ;
         return
            cfgpathname
            && stat(cfgpathname, &st) == 0
            && !(st.st_size == last_cfgstat.st_size && st.st_dev == last_cfgstat.st_dev
                 && memcmp(&(st.*mtimep), &(last_cfgstat.*mtimep), sizeof (st.*mtimep)) == 0) ;
      }

      // Call only when the context is locked
      static void configuration_checked()
      {
         typedef struct stat stat_t ;
         const char *cfgpathname = PTraceConfig::profileFileName() ;
         stat_t buf ;

         last_cfgstat = cfgpathname && stat(cfgpathname, &buf) == 0 ? buf : stat_t() ;
         last_cfgcheck = time(NULL) ;
      }

      static void check_configuration_changes()
      {
         if (!needs_configuration_check())
            return ;

         LOCK() ;
         if (needs_configuration_check())
            if (is_configuration_changed())
               diag_readprofile() ;
            else
               last_cfgcheck = time(NULL) ;
         UNLOCK() ;
      }
} ;

int trace_context::log_fd  = -1 ;
bool trace_context::log_owned = false ;
char trace_context::log_name[DIAG_LOGMAXPATH] ;

time_t        trace_context::last_cfgcheck ;
struct stat   trace_context::last_cfgstat ;

dbglog_writer trace_context::dbglog_write = output_debug_msg ;
void *        trace_context::dbglog_data ;

syslog_writer trace_context::syslog_write = output_syslog_msg ;
void *        trace_context::syslog_data ;
const char *  trace_context::syslog_ident = nullptr ;

typedef trace_context ctx ;

/*******************************************************************************
 PDiagBase::Lock
*******************************************************************************/
/* Context is thread-local */
static thread_local_trivial trace_context context ;

PDiagBase::Lock::Lock() : _lasterr(get_last_error()) { ++context.tracing ; }

PDiagBase::Lock::~Lock() { --context.tracing ; set_last_error(_lasterr) ; }

PDiagBase::Lock::operator bool() const { return context.tracing == 1 ; }

/*******************************************************************************
 Global functions to unconditionally lock and unlock the trace context, for use in the
 trace configurator.
*******************************************************************************/
void LOCK_CONTEXT()
{
   ctx::LOCK() ;
}

void UNLOCK_CONTEXT()
{
   ctx::configuration_checked() ;
   ctx::UNLOCK() ;
}

/*******************************************************************************
 Debugging log and syslog writers registration
*******************************************************************************/
void register_dbglog_writer(dbglog_writer writer, void *data)
{
   ctx::LOCK() ;
   if (!writer)
   {
      ctx::dbglog_write = output_debug_msg ;
      ctx::dbglog_data = NULL ;
   }
   else
   {
      ctx::dbglog_write = writer ;
      ctx::dbglog_data = data ;
   }
   ctx::UNLOCK() ;
}

void register_syslog_writer(syslog_writer writer, void *data)
{
   ctx::LOCK() ;
   if (!writer)
   {
      ctx::syslog_write = output_syslog_msg ;
      ctx::syslog_data = NULL ;
   }
   else
   {
      ctx::syslog_write = writer ;
      ctx::syslog_data = data ;
   }
   ctx::UNLOCK() ;
}

static inline void *fdlog_data(int fd, LogLevel level)
{
   return (void *)((uintptr_t)fd | ((level & 0xfU) << 28)) ;
}

static inline std::pair<int, LogLevel> fdlog_args(void *data)
{
   return {(unsigned)(uintptr_t)data & 0xfffffffU, (LogLevel)(((uintptr_t)data & 0xf0000000U) >> 28)} ;
}

void register_syslog(int fd, LogLevel level)
{
   if (fd >= 0)
      register_syslog_writer(output_fdlog_msg, fdlog_data(fd, level)) ;
   else
   {
      struct local { static void nolog(void *, LogLevel, const char *, ...) {} } ;
      register_syslog_writer(local::nolog) ;
   }
}

const char *syslog_ident()
{
   return ctx::syslog_ident ;
}

/*******************************************************************************
 Debug output
*******************************************************************************/
#ifdef PCOMN_PL_WINDOWS
static void output_debug_msg(void *, const char *msg)
{
   char nmsg[DIAG_MAXMESSAGE] ;
   *(strncpy (nmsg, msg, sizeof nmsg - 1)+ (sizeof nmsg - 1)) = '\0' ;

   for (char *tok = strtok (nmsg, "\n") ; tok ; tok = strtok (NULL, "\n"))
      ::OutputDebugStringA(tok) ;
}

#else
// UNIX

// Note this function is called only once and from under the implicit lock (see
// output_debug_msg), so we needn't lock the mutex
static const int *open_debug_msg_log()
{
   static int dbglog_fd = -1 ;

   char namebuf[DIAG_LOGMAXPATH] ;

   if (dbglog_fd >= 0)
      return &dbglog_fd ;

   if (getcwd(namebuf, sizeof namebuf))
   {
      const size_t wdlen = strlen(namebuf) ;
      snprintf(namebuf + wdlen, sizeof namebuf - wdlen, "/%s.%u.trace.log",
               PCOMN_PROGRAM_SHORTNAME, (unsigned)getpid()) ;

      if (dbglog_fd < 0)
      {
         int fd = ::creat(namebuf, 0644) ;
         if (fd >= 0)
            dbglog_fd = fd ;
      }
   }
   if (dbglog_fd < 0)
   {
      if (dbglog_fd < 0)
         dbglog_fd = STDERR_FILENO ;
   }

   return &dbglog_fd ;
}

static void output_debug_msg(void *, const char *msg)
{
   static const int *debuglog = open_debug_msg_log() ;
   ::write(*debuglog, msg, strlen(msg)) ;
}

static int syslog_priority(LogLevel level)
{
   switch (level)
   {
      case LOGL_ALERT:  return LOG_ALERT ;
      case LOGL_CRIT:   return LOG_CRIT ;
      case LOGL_ERROR:  return LOG_ERR ;
      case LOGL_WARNING:return LOG_WARNING ;
      case LOGL_NOTE:   return LOG_NOTICE ;
      case LOGL_INFO:   return LOG_INFO ;

      case LOGL_DEBUG:
      case LOGL_TRACE:
      default: return LOG_DEBUG ;
   }
}

#endif

static const std::thread::id main_thread_id = std::this_thread::get_id() ;

static char *threadidtostr(char *buf)
{
   const std::thread::id id = std::this_thread::get_id() ;
   if (id == main_thread_id)
      return strcpy(buf, "<mainthrd>") ;

   #if PCOMN_PL_LINUX
   sprintf(buf, "%010lx", (unsigned long)pthread_self()) ;
   memmove(buf, buf + (strlen(buf) - 10), 10) ;
   #else
   pcomn::bufstr_ostream<0> out (buf, 16) ;
   out << std::right << std::setw(10) << id ;
   #endif
   buf[10] = 0 ;

   return buf ;
}

/*******************************************************************************
 Syslog writer
*******************************************************************************/
static void output_syslog_msg(void *, LogLevel level, const char *fmt, ...)
{
   va_list args ;
   va_start(args, fmt) ;

#ifdef PCOMN_PL_WINDOWS
   char msgbuf[DIAG_MAXMESSAGE] = "" ;

   vsnprintf(msgbuf, sizeof msgbuf, fmt, args) ;
   output_debug_msg(NULL, msgbuf) ;

#else // UNIX
   vsyslog(syslog_priority(level), fmt, args) ;
#endif
   va_end(args) ;
}

static void output_fdlog_msg(void *data, LogLevel level, const char *fmt, ...)
{
   va_list args ;
   va_start(args, fmt) ;

   const auto fd_and_level = fdlog_args(data) ;

   if (fd_and_level.second < level)
   {
      va_end(args) ;
      return ;
   }

   char msgbuf[DIAG_MAXMESSAGE] ;
   *msgbuf = 0 ;

   vsnprintf(msgbuf, sizeof msgbuf - 1, fmt, args) ;
   output_debug_msg(NULL, msgbuf) ;

   const size_t sz = strlen(msgbuf) ;
   msgbuf[sz] = '\n' ;
   msgbuf[sz+1] = '\0' ;
   ::write(fd_and_level.first, msgbuf, (unsigned)sz+1) ;

   va_end(args) ;
}

/*******************************************************************************
 PDiagBase: log handling and trace output
*******************************************************************************/
#define UNLOCK_RETURN   do { ctx::UNLOCK() ; return ; } while(false)

void PDiagBase::setlog(int fd)
{
   setlog(fd, fd != STDERR_FILENO && fd != STDOUT_FILENO) ;
}

void PDiagBase::setlog(int fd, bool owned)
{
   ctx::LOCK() ;

   if (fd == ctx::log_fd)
   {
      ctx::log_owned = owned ;
      UNLOCK_RETURN ;
   }

   *ctx::log_name = 0 ;

   const int  prev_fd = ctx::log_fd ;
   const bool prev_owned = ctx::log_owned ;

   ctx::log_fd = -1 ;
   ctx::log_owned = false ;

   if (prev_fd >= 0 && prev_owned)
   {
      if (fd < 0)
         ctx::UNLOCK() ;

      ::close(prev_fd) ;
      if (fd < 0)
         return ;
   }
   else if (fd < 0)
      UNLOCK_RETURN ;

   if (check_diag_fd(fd))
   {
      ctx::log_fd = fd ;
      ctx::log_owned = owned ;

      if (ctx::log_fd == STDERR_FILENO)
         strcpy(ctx::log_name, "stderr") ;
      else if (ctx::log_fd == STDOUT_FILENO)
         strcpy(ctx::log_name, "stdout") ;
   }

   UNLOCK_RETURN ;
}

const char *PDiagBase::logname()
{
   return ctx::log_name ;
}

void PDiagBase::setlog(const char *logname)
{
   ctx::LOCK() ;

   if (!logname || !*logname)
      setlog(-1) ;

   else if (!stricmp(logname, "stdout"))
      setlog(STDOUT_FILENO) ;

   else if (!stricmp(logname, "stderr") || !stricmp(logname, "stdlog"))
      setlog(STDERR_FILENO) ;

   else
   {
      const int fd = open(logname,
                          O_WRONLY|O_CREAT|((mode() & AppendTrace) ? O_APPEND : O_TRUNC),
                          S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH) ;
      if (fd < 0)
         fprintf(stderr, "Failure while setting diagnostics trace stream '%s': %s", logname, strerror(errno)) ;
      else
      {
         strncpy(ctx::log_name, logname, sizeof ctx::log_name - 1) ;
         ctx::log_name[sizeof ctx::log_name - 1] = 0 ;
         setlog(fd) ;
      }
   }

   ctx::UNLOCK() ;
}

// Format and output a debugging message.
// Note that the formatted message is limited to DIAG_MAXMESSAGE characters.
//
void PDiagBase::trace_message(const char *type,
                              const Properties *group, const char *msg,
                              const char *fname, unsigned line)
{
   if (!(mode() & EnableFullPath))
   {
      const char *fn = strrchr(fname, PCOMN_PATH_NATIVE_DELIM) ;
      if (fn
#ifndef PCOMN_PL_UNIX
          || (fn = strrchr(fname, '/')) != NULL || (fn = strrchr(fname, ':')) != NULL
#endif
         )
         fname = fn + 1 ;
   }

   pcomn::bufstr_ostream<DIAG_MAXMESSAGE> out ;

   if (!(mode() & DisableLineNum))
      // Keep place for line number
      out << "       " ;

   out << type << ' ' ;

   if (mode() & ShowProcessId)
   {
      char buf[32] ;
      sprintf(buf, "%4.4u:", (unsigned)getpid()) ;
      out << buf ;
   }
   if (mode() & ShowThreadId)
   {
      char buf[32] ;
      out << threadidtostr(buf) << ": " ;
   }
   char grouplevel[8] ;
   if (mode() & ShowLogLevel)
      sprintf(grouplevel, "=%u", group->level()) ;
   else
      *grouplevel = 0 ;

   out << fname << ':' << line << ": [" << group->name() << grouplevel << "]: " << msg << '\n' << std::ends ;

   ctx::LOCK() ;

   static unsigned line_count ;
   char *outstr = out.str() ;

   if (!(mode() & DisableLineNum))
   {
      snprintf(outstr, 7, "%6.6u", ++line_count) ;
      outstr[6] = ':' ;
   }

   if(!(mode() & DisableDebuggerLog) && ctx::log_fd < 0)
      ctx::dbglog_write(ctx::dbglog_data, outstr) ;
   else if (ctx::log_fd >= 0)
      ::write(ctx::log_fd, outstr, (unsigned)strlen(outstr)) ;

   ctx::UNLOCK() ;

   // It is quite possible that some output could force the stream into the "bad" state.
   // Restore the stream "good" state.
   clearerr(context.os()) ;
}

// Output a debugging message into the system log.
// On Unix, use syslog; on Windows, use event log (?) or OutputDebugString
//
void PDiagBase::syslog_message(LogLevel level,
                               const Properties *group, const char *msg,
                               const char * /*fname*/, unsigned /*line*/)
{
   if (mode() & DisableSyslog)
      return ;

   pcomn::vsaver<const char *> current_ident (ctx::syslog_ident, group->subName()) ;

   ctx::syslog_write(ctx::syslog_data, level, "%s", msg) ;
}

std::ostream &PDiagBase::stream(bool reset)
{
   trace_stream &os = context.os() ;
   if (reset)
   {
      os.reset() ;
      os.flags(context.os_fmtflags) ;
      os.precision(context.os_precision) ;
      os.width(context.os_width) ;
   }
   return os ;
}

std::ostream &PDiagBase::clearerr(std::ostream &os)
{
   if (os.bad())
      os.clear() ;
   return os ;
}

const char *PDiagBase::outstr()
{
   return context.os().str() ;
}

void tee_syslog(LogLevel level, int fd, const char *s)
{
   if (!s)
      return ;

   ctx::syslog_write(ctx::syslog_data, level, "%s", s) ;

   if (fd >= 0 && ctx::syslog_write != output_fdlog_msg && (intptr_t)ctx::syslog_data != fd)
      output_fdlog_msg((void *)(intptr_t)fd, level, "%s", s) ;
}
} // end of namespace diag

/*******************************************************************************
 diag_isenabled_diag()
 Among other things, periodically checks if the trace configuration has been changed
 since last read.
*******************************************************************************/
static int force_enabled = 0 ;

bool diag_isenabled_diag()
{
   if (force_enabled < 0)
      return false ;

   diag::ctx::check_configuration_changes() ;
   return force_enabled > 0 || !(diag_getmode() & diag::DisableDebugOutput) ;
}

void diag_force_diag(bool ena)
{
   force_enabled = 1 | (-(int)!ena) ;
}

void diag_unforce_diag()
{
   force_enabled = 0 ;
}

// Definition of the default diagnostic group "Def" (expands to PDiagGroupDef)
DEFINE_DIAG_GROUP(Def, 1, 0, _PCOMNEXP) ;
