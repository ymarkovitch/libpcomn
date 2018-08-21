/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)(inlambda . 0)) -*-*/
#ifndef __PTRACE_H
#define __PTRACE_H
/*******************************************************************************
 FILE         :   pcomn_trace.h
 COPYRIGHT    :   Yakov Markovitch, 1996-2018. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Tracing paraphernalia (diagnostic groups, tracing macros, etc.)

 CREATION DATE:   17 Jun 1996
*******************************************************************************/
/** @file
  Logging and debugging trace framework.
*******************************************************************************/
#include <pcommon.h>
#include <pcomn_omanip.h>

#include <iostream>
#include <memory>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if defined(__PCOMN_TRACE) && !defined(__PCOMN_WARN)
#define __PCOMN_WARN
#endif

/******************************************************************************/
/** @addtogroup pcomn_trace Tracing and logging framework
 * @{
*******************************************************************************/

/// Diagnostics levels
const unsigned DBGL_ALWAYS  = 0 ;
const unsigned DBGL_HIGHLEV = 1 ;
const unsigned DBGL_SKETCHY = 10 ;
const unsigned DBGL_MIDLEV  = 50 ;
const unsigned DBGL_NORMAL  = 50 ;
const unsigned DBGL_LOWLEV  = 100 ;
const unsigned DBGL_EXTRA   = 127 ;
const unsigned DBGL_VERBOSE = 127 ;

/// The maximum level that can be specified for DIAG_DEFINE_GROUP
const unsigned DBGL_MAXLEVEL = DBGL_VERBOSE ;

namespace diag {

const unsigned MaxGroupsNum      = 512 ; /**< Maximum overall number of groups */
const unsigned MaxSuperGroupsNum = 256 ; /**< Maximum overall number of supergroups */
const char     GroupDelim        = '_' ; /**< A subgroup/supergroup name delimiter */
const unsigned MaxSuperGroupLen  = 15 ;  /**< Maximum supergroup name length */

/// Tracing mode flags.
enum DiagMode {
   DisableDebugOutput = 0x0001, /**< Turn off tracing completely */
   DisableDebuggerLog = 0x0002, /**< Don't output trace into debugger log (like Win32 OutpudDebugString) */
   DisableSyslog      = 0x0004, /**< Disable syslog output */
   AppendTrace        = 0x0008, /**< Don't truncate the trace file on open */
   EnableFullPath     = 0x0010, /**< Output full source file paths into the trace log */
   DisableLineNum     = 0x0020, /**< Don't output source line numbers into the trace log */
   ShowThreadId       = 0x0040, /**< Output thread IDs into the trace log */
   ShowProcessId      = 0x0080, /**< Output process IDs into the trace log */
   ShowLogLevel       = 0x0100  /**< Show log level of the message in the trace log */
} ;

constexpr const DiagMode UseThreadId   = ShowThreadId ;  /**< Backward compatibility */
constexpr const DiagMode UseProcessId  = ShowProcessId ; /**< Backward compatibility */

/// Log levels
enum LogLevel : unsigned {
   LOGL_ALERT,
   LOGL_CRIT,
   LOGL_ERROR,
   LOGL_WARNING,
   LOGL_NOTE,
   LOGL_INFO,
   LOGL_DEBUG,
   LOGL_TRACE
} ;

/// Callback type for writing syslog messages.
///
/// Registered through register_syslog_writer(). PDiagBase::syslog_message(), and so all
/// LOGPX macros, use this callback to output syslog messages.
typedef void(*syslog_writer)(void *data, LogLevel level, const char *fmt, ...) ;

/// Callback type for writing trace messages into debugger log.
typedef void(*dbglog_writer)(void *data, const char *msg) ;

class PDiagBase ;
class PTraceConfig ;
class PTraceSuperGroup ;

/******************************************************************************/
/** The base for PDiagGroup classes.
    Handles basic message output.
*******************************************************************************/
class _PCOMNEXP PDiagBase {
   public:
      // This structure is intended to be inserted into array of diag group properties
      struct Properties {

            const char *   _name ;
            bool           _enabled ;
            unsigned char  _level ;

            /// Turn on/off a diag group.
            void ena(bool enabled) { _enabled = enabled ; }

            /// Indicates whether this diagnostics group is enabled.
            bool enabled() const { return _enabled ; }

            /// Set diagnostics level for this diag group.
            /// A message is logged iff its diagnostics level is less or equal to the
            /// current level of the corresponding group.
            void level(unsigned char level) { _level = level ; }

            /// Get current diagnostics level for this diag group.
            unsigned level() const { return _level ; }

            /// Get the full group name (like FOO_Bar).
            const char *name() const { return _name ; }

            /// Get the supergroup name for this group.
            /// For instance, "FOO" for the "FOO_Bar" group.
            const char *superName() const ;
            /// Get the subgroup name for this group.
            /// For instance, "Bar" for the "FOO_Bar" group.
            const char *subName() const ;
      } ;

      struct _PCOMNEXP Lock {
            Lock() ;
            ~Lock() ;
            operator bool () const ;
         private:
            int _lasterr ;
      } ;

      /// Get the overall number of diagnostic groups in the process.
      static unsigned numOfGroups() ;

      /// Begin iterator of the global vector of dignostic groups, sorted by group names.
      static Properties **begin() ;

      /// End iterator of the global vector of dignostic groups, sorted by group names.
      static Properties **end() ;

      /// Set tracing mode.
      /// The 'flags' parameter is a bit combination of diag::DiagMode flags
      static void mode(unsigned flags, bool onOff = true) ;

      /// Get tracing mode.
      static unsigned mode() ;

      /// Get group properties by its full name.
      static Properties *get(const char *name) ;

      static std::ostream &stream(bool reset = false) ;

      static std::ostream &clearerr(std::ostream &) ;

      static const char *outstr() ;

      /// Set trace log stream.
      /// @note The tracer owns the stream, i.e. setting new stream forces call to delete
      /// the currently set stream object.
      /// @note The tracer recognizes descriptors of standard output streams (stdcout,
      /// stderr) and doesn't attempt to close them; thus it is safe to pass those
      /// descriptors to this method.
      static void setlog(int fd) ;

      static void setlog(int fd, bool own) ;

      /// Set trace log stream.
      /// The function recognises "special" names @b stdout, @b stderr, and @b stdlog
      /// and uses in these cases stdout, and stderr (both for "stderr" and "stdlog")
      static void setlog(const char *logname) ;

      static const char *logname() ;

   protected:
      PDiagBase(Properties *grp) ;

      static void trace_message(const char *type, const Properties *group, const char *msg,
                                const char *fname, unsigned line) ;

      static void syslog_message(LogLevel level, const Properties *group, const char *msg,
                                 const char *fname, unsigned line) ;

      static const PTraceSuperGroup &null_supergroup ;
} ;

/******************************************************************************/
/** Diagnostics supergroup.
*******************************************************************************/
class _PCOMNEXP PTraceSuperGroup {
      friend class PTraceConfig ;
   public:
      /// Constructor.
      /// @param full_name is a @em full group name, for instance, FOO_Bar.
      /// @param ena       specifies, whether to enable this supergroup immediately.
      explicit PTraceSuperGroup(const char *full_name, bool ena = true) ;
      constexpr PTraceSuperGroup() = default ;

      /// Get the supergroup name.
      const char *name() const { return _name ; }

      void ena(bool onOff) ;

      bool enabled() const { return _enabled ; }

      bool forceenable() const { return _force_enable ; }
      unsigned char forcelevel() const { return _force_level ; }

      /// Get the supergroup name for a given group name.
      static const char *parseName(const char *fullName) ;

   protected:
      bool          _force_enable = false ;
      unsigned char _force_level  = 0 ;

      bool _enabled = true ;
      char _name[diag::MaxSuperGroupLen+1] = {0} ;
} ;


/******************************************************************************/
/** Controls diag supergroups and handles tracing profiles (configuration files
    with trace configurations).
*******************************************************************************/
class _PCOMNEXP PTraceConfig {
      friend class PTraceSuperGroup ;
   public:
      typedef PTraceSuperGroup *iterator ;

      static iterator begin() ;
      static iterator end() ;

      static const char *profileFileName(const char *name) ;
      static const char *profileFileName() ;

      static bool readProfile() ;
      static bool writeProfile() ;

      static bool syncProfile()
      {
         return readProfile() && writeProfile() ;
      }

      static iterator get(const char *name) ;
      static iterator insert(const PTraceSuperGroup &) ;
} ;

/*******************************************************************************
 PDiagBase::Properties
*******************************************************************************/
inline const char *PDiagBase::Properties::superName() const
{
   return PTraceSuperGroup::parseName(name()) ;
}

inline const char *PDiagBase::Properties::subName() const
{
   const size_t len = strlen(PTraceSuperGroup::parseName(name())) ;
   return name() + len + !!len ;
}

/// Write a string both into the syslog and into a file descriptor.
/// @param level loglevel
/// @param fd    The file to write @a s into; tee_syslog appends '\n' to @a s while
/// writing to @a fd; if this parameter is -1, don't write @a s into a file (but
/// still write into the syslog)
/// @param msg   Message to output to the log
_PCOMNEXP void tee_syslog(LogLevel level, int fd, const char *msg) ;

_PCOMNEXP void register_dbglog_writer(dbglog_writer writer, void *data = NULL) ;

_PCOMNEXP void register_syslog_writer(syslog_writer writer, void *data= NULL) ;

_PCOMNEXP void register_syslog(int fd, LogLevel level = LOGL_WARNING) ;

_PCOMNEXP const char *syslog_ident() ;

}   // end of diag namespace

/******************************************************************************/
/** Diagnostics group declaration macro
*******************************************************************************/
#define DECLARE_DIAG_GROUP(GRP, ...)                                    \
namespace diag {                                                        \
namespace grp {                                                         \
class __VA_ARGS__ GRP : private PDiagBase                               \
{                                                                       \
  public:                                                               \
   GRP() ;                                                              \
   static void trace(const char *fname, unsigned line) ;                \
   static void warn(const char *fname, unsigned line) ;                 \
   static void slog(LogLevel lvl, const char *fname, unsigned line)     \
   {                                                                    \
      PDiagBase::syslog_message (lvl, properties(), outstr(), fname, line) ; \
   }                                                                    \
                                                                        \
   static bool IsSuperGroupEnabled() { return supergroup()->enabled() ; } \
                                                                        \
   static void Enable(bool enabled) { properties()->ena(enabled) ; }    \
   static bool IsEnabled()                                              \
   {                                                                    \
      const PTraceSuperGroup * const s = supergroup() ;                 \
      return                                                            \
         s->enabled() && (s->forceenable() || properties()->enabled()) ; \
   }                                                                    \
   static bool IsEnabled(unsigned level)                                \
   {                                                                    \
      const PTraceSuperGroup * const s = supergroup() ;                 \
      const Properties *p ;                                             \
      return s->enabled() &&                                            \
         ((p = properties())->enabled() || s->forceenable()) &&         \
         (p->level() >= level           || s->forcelevel() >= level) ;  \
   }                                                                    \
   static void SetLevel(unsigned level) { properties()->level(level) ; } \
   static unsigned GetLevel() { return properties()->level() ; }        \
                                                                        \
  private:                                                              \
   static Properties *properties() ;                                    \
   static const PTraceSuperGroup *supergroup() ;                        \
} ;                                                                     \
}}

#define DEFINE_DIAG_GROUP(GRP, ENA, LVL, ...)                           \
   DECLARE_DIAG_GROUP(GRP, ##__VA_ARGS__) ;                             \
   static const ::diag::PTraceSuperGroup *GRP##_supergroup ;            \
   static ::diag::grp::GRP __##GRP ;                                    \
   ::diag::grp::GRP::GRP() :                                            \
      ::diag::PDiagBase(properties())                                   \
   {                                                                    \
      GRP##_supergroup = PTraceConfig::insert(PTraceSuperGroup(#GRP)) ; \
   }                                                                    \
   const ::diag::PTraceSuperGroup *::diag::grp::GRP::supergroup()       \
   {                                                                    \
      return GRP##_supergroup ? GRP##_supergroup : &null_supergroup ;   \
   }                                                                    \
   ::diag::PDiagBase::Properties *::diag::grp::GRP::properties()        \
   {                                                                    \
      static PDiagBase::Properties props = { #GRP, (ENA), (LVL) } ;     \
      return &props ;                                                   \
   }                                                                    \
   void ::diag::grp::GRP::trace(const char *fname, unsigned line)       \
   {                                                                    \
      PDiagBase::trace_message                                          \
         (" ", properties(), outstr(), fname, line) ;                   \
   }                                                                    \
   void ::diag::grp::GRP::warn(const char *fname, unsigned line)        \
   {                                                                    \
      PDiagBase::trace_message                                          \
         ("!", properties(), outstr(), fname, line) ;                   \
   }

#define DIAG_DECLARE_GROUP(GRP)            DECLARE_DIAG_GROUP(GRP)
#define DIAG_DEFINE_GROUP(GRP, ENA, LVL)   DEFINE_DIAG_GROUP(GRP, ENA, LVL);

/*******************************************************************************
 Global trace-controlling function.
*******************************************************************************/
/// Enable tracing/logging for a diagnostics group
/// @param group Diagnostics group, like e.g. FOO_Bar
/// @param state Expression convertible to boolean; true - enable, false - disable
#define diag_enable(group, state)     (::diag::grp::group::Enable(!!(state)))

/// Indicate whether the diagnostics group is traced/logged
#define diag_isenabled(group)         (::diag::grp::group::IsEnabled())

/// Set diagnostics verbosity level for a group
#define diag_setlevel(group, level)   (::diag::grp::group::SetLevel((level)))

/// Get diagnostics verbosity level for a group
#define diag_getlevel(group)          (::diag::grp::group::GetLevel())

#define diag_isenabled_output(group, level) \
   (diag_isenabled_diag() && ::diag::grp::group::IsEnabled(level))

_PCOMNEXP bool diag_isenabled_diag() ;

/// Force diagnostics trace to enabled or disabled state regardless or mode flags.
/// Does _not_ change the mode flags. To reset enabled/disabled state to specified by
/// mode flags (e.g. by configuration file), call diag_unforce_diag().
_PCOMNEXP void diag_force_diag(bool ena) ;

_PCOMNEXP void diag_unforce_diag() ;

inline
void diag_enable_supergroup(const char *name, bool enabled)
{
   if (::diag::PTraceConfig::iterator supergroup = ::diag::PTraceConfig::get(name))
      supergroup->ena(enabled) ;
}

inline
bool diag_isenabled_supergroup(const char *name)
{
   const ::diag::PTraceConfig::iterator supergroup = ::diag::PTraceConfig::get(name) ;
   return supergroup && supergroup->enabled() ;
}

inline
void diag_setmode(unsigned mode, bool onoff) { ::diag::PDiagBase::mode(mode, onoff) ; }

inline
unsigned diag_getmode() { return ::diag::PDiagBase::mode() ; }

inline
void diag_setlog(int fd) { ::diag::PDiagBase::setlog(fd) ; }

inline
void diag_setlog(int fd, bool own) { ::diag::PDiagBase::setlog(fd, own) ; }

inline
bool diag_readprofile() { return ::diag::PTraceConfig::readProfile() ; }

inline
bool diag_writeprofile() { return ::diag::PTraceConfig::writeProfile() ; }

inline
void diag_syncprofile() { ::diag::PTraceConfig::syncProfile() ; }

inline
void diag_setprofile(const char *name) { ::diag::PTraceConfig::profileFileName(name) ; }

inline
void diag_inittrace(const char *name)
{
   diag_setprofile(name) ;
   diag_syncprofile() ;
}

/*******************************************************************************
 Trace-controlling macros, named the same way as global trace-controlling functions
 but in uppercase (diag_enable() -> DIAG_ENABLE(), etc.).
 The difference is that macros expand to corresponding calls only when tracing is on,
 i.e. __PCOMN_TRACE or/and __PCOMN_WARN is defined.

 Macros are the right thing to control debug tracing behaviour (TRACEPX/WARNPX);
 to control logging (LOGPX/LOGPXERR/LOGPXWARN) use  trace-controlling functions
 directly.
*******************************************************************************/
#if defined(__PCOMN_TRACE) || defined(__PCOMN_WARN)

#define DIAG_ENABLE(GRP, STATE)        (diag_enable(GRP, (STATE)))
#define DIAG_ISENABLED_DIAG()          (::diag_isenabled_diag())
#define DIAG_ISENABLED(GRP)            (diag_isenabled(GRP))
#define DIAG_ISENABLED_OUTPUT(GRP, LVL) (diag_isenabled_output(GRP, (LVL)))
#define DIAG_ISENABLED_SUPERGROUP(GRP) (::diag::grp::GRP::IsSuperGroupEnabled())
#define DIAG_SETLEVEL(GRP, LVL)        (diag_setlevel(GRP, (LVL)))
#define DIAG_GETLEVEL(GRP)             (diag_getlevel(GRP))
#define DIAG_SETMODE(mode, onoff)      (::diag_setmode((mode), (onoff)))
#define DIAG_GETMODE()                 (::diag_getmode())
#define DIAG_SETLOG(fd)                (::diag_setlog((fd)))
#define DIAG_READPROFILE()             (::diag_readprofile())
#define DIAG_WRITEPROFILE()            (::diag_writeprofile())
#define DIAG_SYNCPROFILE()             (::diag_syncprofile())
#define DIAG_SETPROFILE(filename)      (::diag_setprofile((filename)))
#define DIAG_INITTRACE(filename)       (::diag_inittrace(filename))

#if !defined(DIAG_BUILD) && !defined(DIAG_DEFGROUP_DECLARED)
#define DIAG_DEFGROUP_DECLARED
DECLARE_DIAG_GROUP(Def, _PCOMNEXP) ;
#endif

#else   // !defined(__PCOMN_TRACE) && !defined(__PCOMN_WARN)

#define DIAG_ENABLE(GRP, STATE)        ((void)0)
#define DIAG_ISENABLED_DIAG()          (false)
#define DIAG_ISENABLED(GRP)            (false)
#define DIAG_ISENABLED_OUTPUT(GRP, LVL) (false)
#define DIAG_ISENABLED_SUPERGROUP(GRP) (false)
#define DIAG_SETLEVEL(GRP, LVL)        ((void)0)
#define DIAG_GETLEVEL(GRP)             (0U)
#define DIAG_SETMODE(mode, onoff)      ((void)0)
#define DIAG_GETMODE()                 ((unsigned)::diag::DisableDebugOutput)
#define DIAG_SETLOG(fd)                ((void)0)
#define DIAG_READPROFILE()             (false)
#define DIAG_WRITEPROFILE()            (false)
#define DIAG_SYNCPROFILE()             ((void)0)
#define DIAG_SETPROFILE(filename)      ((void)0)
#define DIAG_INITTRACE(filename)       ((void)0)

#endif

/******************************************************************************/
/** @internal
*******************************************************************************/
#if defined(__PCOMN_TRACE) || defined(__PCOMN_WARN)
#  define OUTMSGPX(GRP, MSGTYP) (::diag::grp::GRP::MSGTYP(__FILE__, __LINE__), true)
#  define ACTIONPX(ACTION) ((ACTION), true)
#else
#  define OUTMSGPX(GRP, MSGTYP) (true)
#  define ACTIONPX(ACTION) (true)
#endif
#define MAKEMSGPX(MSG) \
   (::diag::PDiagBase::clearerr(::diag::PDiagBase::clearerr(::diag::PDiagBase::stream(true) << MSG) << std::ends))
#define LOGMSGPX(GRP, LOGL, MSG) \
   (MAKEMSGPX(MSG) && (::diag::grp::GRP::slog(::diag::LOGL_##LOGL, __FILE__, __LINE__), true))

/*******************************************************************************
 The family of tracing debug macros: TRACEPX/WARNPX
 Note that this macros expand to code only when __PCOMN_TRACE or/and __PCOMN_WARN is
 defined; otherwise they expand to an empty expression.
*******************************************************************************/
#if defined(__PCOMN_TRACE)
#  define TRACEP(MSG)                    TRACEPX(Def, 0, MSG)

#  define TRACEPX(GRP, LVL, MSG)                                        \
   (DIAG_ISENABLED_OUTPUT(GRP, LVL) && ::diag::PDiagBase::Lock() &&     \
    MAKEMSGPX(MSG) && OUTMSGPX(GRP, trace))

#else
#  define TRACEP(MSG)            ((void)0)
#  define TRACEPX(GRP, LVL, MSG) ((void)0)
#endif

#if defined(__PCOMN_WARN)
#  define WARNP(COND, MSG)                  WARNPX(Def, (COND), 0, MSG)

#  define WARNPX(GRP, COND, LVL, MSG)                                   \
   (DIAG_ISENABLED_OUTPUT(GRP, LVL) && ::diag::PDiagBase::Lock() &&     \
    (COND) &&                                                           \
    MAKEMSGPX(MSG) && OUTMSGPX(GRP, warn))

#else
#  define WARNP(COND, MSG)             ((void)0)
#  define WARNPX(GRP, COND, LVL, MSG)  ((void)0)
#endif

/*******************************************************************************
 The family of logging macros: LOGPXERR/LOGPXWARN/LOGPXINFO/LOGPXNOTE/LOGPXDBG/LOGPXTRACE
*******************************************************************************/
/// Output debug message both into the diagnostics trace and system log, if tracing and
/// specified diagnostics group are enabled.
///
/// This macro closely resembles TRACEPX, but in contrast to the latter its code doesn't
/// disappear from compiled code in release mode. While in release mode it never writes
/// into the diagnostics trace log, it can write into syslog, subject to @a GRP.
/// On Unix, writes into syslog with LOG_TRACE priority
///
#define LOGPXTRACE(GRP, MSG)                                            \
   (diag_isenabled_output(GRP, DBGL_ALWAYS) && ::diag::PDiagBase::Lock() && \
    LOGMSGPX(GRP, TRACE, MSG) && OUTMSGPX(GRP, trace))

/// Output DEBUG message
///   - into the system log if diagnostics output is overall enabled
///     (@see DiagMode::DisableDebugOutpu, @see diag_isenabled_diag()), and
///   - into the diagnostics trace if tracing and **supergroup** of the specified @a GRP
///     are enabled
///
#define LOGPXDBG(GRP, MSG)                                              \
   (::diag_isenabled_diag() && ::diag::PDiagBase::Lock() &&             \
    LOGMSGPX(GRP, DEBUG, MSG) &&                                        \
    DIAG_ISENABLED_SUPERGROUP(GRP) && OUTMSGPX(GRP, trace))

/// Output INFO message into the system log and, if *both* tracing and **supergroup**
/// of the specified @a GRP are enabled, also into diagnostics trace.
///
/// @note Doesn't take subgroup and diagnostics level into account (i.e. it is enough
/// that the **supergroup** is enabled).
///
#define LOGPXINFO(GRP, MSG)                                             \
   (::diag::PDiagBase::Lock() &&                                        \
    LOGMSGPX(GRP, INFO, MSG) &&                                         \
    DIAG_ISENABLED_DIAG() && DIAG_ISENABLED_SUPERGROUP(GRP) && OUTMSGPX(GRP, trace))

/// Output NOTICE message into the system log and, if *both* tracing and **supergroup**
/// of the specified @a GRP are enabled, also into diagnostics trace.
///
/// @note Doesn't take subgroup and diagnostics level into account (i.e. it is enough
/// that the **supergroup** is enabled).
///
#define LOGPXNOTE(GRP, MSG)                                             \
   (::diag::PDiagBase::Lock() &&                                        \
    LOGMSGPX(GRP, NOTE, MSG) &&                                         \
    DIAG_ISENABLED_DIAG() && DIAG_ISENABLED_SUPERGROUP(GRP) && OUTMSGPX(GRP, trace))

/// Output WARNING message into the system log and, if *both* tracing and **supergroup**
/// of the specified @a GRP are enabled, also into diagnostics trace.
/// @note On Unix, writes into syslog with LOG_WARNING priority.
///
#define LOGPXWARN(GRP, MSG)                                             \
   (::diag::PDiagBase::Lock() &&                                        \
    LOGMSGPX(GRP, WARNING, MSG) &&                                      \
    DIAG_ISENABLED_DIAG() && DIAG_ISENABLED_SUPERGROUP(GRP) && OUTMSGPX(GRP, warn))

/// Output  ERROR message into the system log and, if tracing enabled, also into
/// diagnostics trace.
/// @note On Unix, writes into syslog with LOG_ERR priority.
///
#define LOGPXERR(GRP, MSG)                                              \
   (::diag::PDiagBase::Lock() &&                                        \
    LOGMSGPX(GRP, ERROR, MSG) &&                                        \
    DIAG_ISENABLED_DIAG() && OUTMSGPX(GRP, warn))

/// Always output CRITICAL message into the system log and, if tracing is enabled, also
/// into diagnostics trace.
/// @note When writes to the diagnostics trace, takes neither group enabled state nore
/// diagnostics level into account (i.e. it is enough that the @em tracing is enabled).
///
#define LOGPXCRIT(GRP, MSG)                                             \
   (::diag::PDiagBase::Lock() &&                                        \
    LOGMSGPX(GRP, CRIT, MSG) &&                                         \
    DIAG_ISENABLED_DIAG() && OUTMSGPX(GRP, warn))

/// Always output ALERT message into the system log and, if tracing is enabled, also into
/// diagnostics trace.
/// @note When writes to the diagnostics trace, takes neither group enabled state nore
/// diagnostics level into account (i.e. it is enough that the @em tracing is enabled).
///
#define LOGPXALERT(GRP, MSG)                                            \
   (::diag::PDiagBase::Lock() &&                                        \
    LOGMSGPX(GRP, ALERT, MSG) &&                                        \
    DIAG_ISENABLED_DIAG() && OUTMSGPX(GRP, warn))

template<typename T>
inline const T& diag_cref(const T &v)
{
   return v ;
}

#define LOGPXWARN_CALL(CALL, GRP, MSG)                                   \
   (::diag::PDiagBase::Lock() &&                                        \
    (LOGMSGPX(GRP, WARNING, MSG) &&                                     \
     DIAG_ISENABLED_DIAG() && DIAG_ISENABLED(GRP) && OUTMSGPX(GRP, warn) || true) && \
    (CALL(diag_cref(::diag::PDiagBase::outstr())), true))

#define LOGPXERR_CALL(CALL, GRP, MSG)                                   \
   (::diag::PDiagBase::Lock() &&                                        \
    (LOGMSGPX(GRP, ERROR, MSG) &&                                       \
     DIAG_ISENABLED_DIAG() && DIAG_ISENABLED(GRP) && OUTMSGPX(GRP, warn) || true) && \
    (CALL(diag_cref(::diag::PDiagBase::outstr())), true))

#define LOGPXALERT_CALL(CALL, GRP, MSG)                                 \
   (::diag::PDiagBase::Lock() &&                                        \
    (LOGMSGPX(GRP, ALERT, MSG) &&                                       \
     DIAG_ISENABLED_DIAG() && OUTMSGPX(GRP, warn) || true) &&           \
    (CALL(::diag::PDiagBase::outstr()), true))

#define LOGPX(LOGL, FD, MSG)                                             \
   (::diag::PDiagBase::Lock() && MAKEMSGPX(MSG) &&                      \
    (::diag::tee_syslog(::diag::LOGL_##LOGL, (FD), ::diag::PDiagBase::outstr()), true))

/*******************************************************************************
 Diagnostic action macros
*******************************************************************************/
/// Perform @a ACTION if tracing and @a GRP diagnostics group are enabled and group's
/// diagnostics level is greater or equal to @a LVL
///
/// This macro closely resembles TRACEPX, but instead of output of arbitrary messages
/// into the trace log it allows to perform an arbitrary action when corresponding
/// diagnosticts group is enabled.
///
/// @note The code of this macro turns into literal true value in release mode.
///
#define DIAGPX(GRP, LVL, ACTION) (DIAG_ISENABLED_OUTPUT(GRP, LVL) && ACTIONPX((ACTION)))

/*******************************************************************************
 "Fix me" macros
 Make sure there is no FIX and ME appear together in those declarations to prevent grep
 from false positives while grepping throuh the text to find what to fix.
*******************************************************************************/
#define DECLARE_TRACEFIXME(SUPERGRP, ...) \
   DECLARE_DIAG_GROUP(SUPERGRP##_FIX##ME, __VA_ARGS__)

#define DEFINE_TRACEFIXME(SUPERGRP, ...) \
   DEFINE_DIAG_GROUP(SUPERGRP##_FIX##ME, true, DBGL_VERBOSE, __VA_ARGS__)

#define TRACEFIXME(SUPERGRP, TEXT) \
   WARNPX(SUPERGRP##_FIX##ME, true, DBGL_ALWAYS, ("FIX""ME: ") << TEXT)

/*******************************************************************************

*******************************************************************************/

#ifndef __PCOMN_ASSERT_H
#include <pcomn_assert.h>
#endif /* PCOMN_ASSERT.H */

/*******************************************************************************
 Diagnostics output of typed pointers
*******************************************************************************/
namespace diag {

template<typename T>
struct tptr_printer {
      static std::ostream &print(std::ostream &os, const T *ptr)
      {
         os << '(' << PCOMN_DEREFTYPENAME(ptr) << "*)" ;
         return
            ptr ? (os << ptr) : (os << "NULL") ;
      }
} ;

template<>
struct tptr_printer<char> {
      static std::ostream &print(std::ostream &os, const char *ptr)
      {
         return !ptr ? (os << "(char*)NULL") : (os << '"' << ptr << '"') ;
      }
} ;

template<typename P>
std::ostream &print_dereferenced(std::ostream &os, const P &ptr)
{
   return !ptr
      ? os << "((" << PCOMN_TYPENAME(P) << ")NULL)"
      : os << *ptr ;
}

inline std::ostream &print_file(std::ostream &os, FILE *file)
{
   if (!file || feof(file) || ferror(file))
      return os ;
   const long pos = ftell(file) ;
   if (pos < 0)
      return os ;

   char buf[4096] ;
   do os.write(buf, fread(buf, 1, sizeof buf, file)) ;
   while (!feof(file) && !ferror(file)) ;

   if (pos)
      fseek(file, pos, SEEK_SET) ;
   else
      rewind(file) ;
   return os ;
}

/*******************************************************************************

*******************************************************************************/
template<typename T>
inline auto otptr(const T *ptr) PCOMN_MAKE_OMANIP(tptr_printer<T>::print, ptr) ;

template<typename T>
inline auto otptr(const std::unique_ptr<T> &ptr) PCOMN_DERIVE_OMANIP(otptr(ptr.get())) ;

template<typename T>
inline auto otptr(const std::shared_ptr<T> &ptr) PCOMN_DERIVE_OMANIP(otptr(ptr.get())) ;

template<typename P>
inline auto oderef(const P &ptr) PCOMN_MAKE_OMANIP(print_dereferenced<P>, std::cref(ptr)) ;

inline auto ofile(FILE *file) PCOMN_MAKE_OMANIP(&print_file, file) ;

enum EndArgs { endargs } ;

struct ofncall_ {

      explicit ofncall_(std::ostream &os, const char *function_name) :
         _os(os),
         _name(function_name ? function_name : ""),
         _pcount(0)
      {}

      template<typename T>
      const ofncall_ &operator<<(T &&arg) const
      {
         if (!_pcount++)
            _os << _name << '(' ;
         else
            _os << ", " ;
         _os << std::forward<T>(arg) ;
         return *this ;
      }

      std::ostream &operator<<(EndArgs) const
      {
         return !_pcount ? (_os << _name << "()") : (_os << ')') ;
      }

   private:
      std::ostream &       _os ;
      const char * const   _name ;
      mutable unsigned     _pcount ; /* Already printed param count */
} ;

struct ofncall { explicit ofncall(const char *fn) : _fn(fn) {} ; const char *_fn ; } ;

inline ofncall_ operator<<(std::ostream &os, const ofncall &manip)
{
   return ofncall_(os, manip._fn) ;
}

inline std::ostream &operator<<(std::ostream &os, EndArgs) { return os ; }

using pcomn::oenum ;

inline auto ostrerror(int errn) PCOMN_MAKE_OMANIP(([](std::ostream &os, int errn) -> std::ostream &
{
   char buf[256] ;
   *buf = 0 ;
   strerror_r(errn, buf, sizeof(buf)) ;
   return os << buf ;
}), errn) ;

} // end of namespace diag

namespace pcomn {
using diag::oderef ;
using diag::otptr ;
using diag::ofncall ;
using diag::EndArgs ;
using diag::ostrerror ;
} ;

/*******************************************************************************
 Family of "output manipulator" macros for debug output of functions and methods calls
 FUNCOUT(NAME, ARGS)
 MEMFNOUT(NAME, ARGS)
 SCOPEFUNCOUT(ARGS)
 SCOPEMEMFNOUT(ARGS)
*******************************************************************************/

/// "Output manipulator" macro for outputting function call into an ostream.
/// @param NAME Function name (must be convertible to const char *)
/// @param ARGS Sequence of arguments, delimited by operator <<.
///
/// The following code will output "hello(world, 2)"
/// @code
/// std::cout << FUNCOUT("hello", "world" << 2) ;
/// @endcode
/// The following code will ouptut "foobar()"
/// @code
/// std::cout << FUNCOUT("foobar", diag::endargs) ;
/// @endcode
#define FUNCOUT(NAME, ARGS)   ::diag::ofncall((NAME)) << ARGS << ::diag::endargs

/// "Output manipulator" macro for outputting member function call into an ostream.
/// @param NAME Member function name (must be convertible to const char *)
/// @param ARGS Sequence of arguments, delimited by operator <<.
///
/// Let the type of "this" pointer be FooBar; then the following code will output
/// something like "(FooBar@0xbfdb8fd7)->hello(world, 2)"
/// @code
/// std::cout << MEMFNOUT("hello", "world" << 2) ;
/// @endcode
#define MEMFNOUT(NAME, ARGS)  '(' << ::diag::otptr(this) << ")->" << ::diag::ofncall((NAME)) << ARGS << ::diag::endargs

/// "Output manipulator" macro for outputting current function call into an ostream,
/// considering the function name if the name of the function, which scope we now in
/// (i.e. __FUNCTION__).
/// @param ARGS Sequence of arguments, delimited by operator <<.
#define SCOPEFUNCOUT(ARGS)    FUNCOUT(__FUNCTION__, ARGS)

/// "Output manipulator" macro for outputting current member function call into an
/// ostream, considering the function name if the name of the member function, which
/// scope we now in (i.e. __FUNCTION__).
/// @param ARGS Sequence of arguments, delimited by operator <<.

#define SCOPEMEMFNOUT(ARGS)   MEMFNOUT(__FUNCTION__, ARGS)

/******************************************************************************/
/** @} */ // End of pcomn::trace
/******************************************************************************/

#endif /* __PTRACE_H */
