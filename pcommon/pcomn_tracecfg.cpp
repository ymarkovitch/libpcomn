/*-*- tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_tracecfg.cpp
 COPYRIGHT    :   Yakov Markovitch, 1995-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Trace configuration

 CREATION DATE:   26 Mar 1995
*******************************************************************************/
#include <pcomn_trace.h>
#include <pcomn_cfgparser.h>
#include <pcomn_strnum.h>

#include <utility>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef PCOMN_PL_WINDOWS
#include <windows.h>
#endif

static constexpr const size_t PCOMN_MAXPATH = 512 ;

static constexpr const char SECTION_ENABLED[] = "ENABLED" ;
static constexpr const char SECTION_DEFAULT[] = "DEFAULT" ;
static constexpr const char SECTION_EXT[]     = "diag" ;

static constexpr const char TRACE_GROUP[] = "TRACE" ;
static constexpr const char SHOW_GROUP[]  = "SHOW" ;

static constexpr const char KEY_LOGNAME[]    = "LOG" ;
static constexpr const char KEY_ENABLED[]    = "ENABLED" ;
static constexpr const char KEY_APPEND[]     = "APPEND" ;
static constexpr const char KEY_FULLPATH[]   = "FULLPATH" ;
static constexpr const char KEY_LINENUM[]    = "LINENUM" ;
static constexpr const char KEY_THREADID[]   = "THREADID" ;
static constexpr const char KEY_PID[]        = "PID" ;
static constexpr const char KEY_LEVEL[]      = "LEVEL" ;
static constexpr const char KEY_FORCELEVEL[] = "FORCELEVEL" ;

using namespace pcomn ;

namespace diag {

void LOCK_CONTEXT() ;
void UNLOCK_CONTEXT() ;

/*******************************************************************************
 PTraceSuperGroup
*******************************************************************************/
PTraceSuperGroup::PTraceSuperGroup (const char *fullName, bool ena) :
   _enabled (ena)
{
   *(strncpy(_name, parseName(fullName), MaxSuperGroupLen) + MaxSuperGroupLen) = '\0' ;
}

void PTraceSuperGroup::ena(bool onOff)
{
   _enabled = onOff ;
   PTraceSuperGroup *grp = const_cast<PTraceSuperGroup *>(PTraceConfig::get (name())) ;
   if (grp)
      grp->_enabled = onOff ;
}

const char *PTraceSuperGroup::parseName(const char *fullName)
{
   static thread_local_trivial char buf[MaxSuperGroupLen+1] ;

   const char *_ = strchr (fullName, '_') ;
   size_t l ;

   if (_ && (l = _-fullName) != 0 && l <= MaxSuperGroupLen)
   {
      buf[l] = '\0' ;
      return strncpy (buf, fullName, l) ;
   }

   return &(*buf = '\0') ;
}

/*******************************************************************************
 PTraceConfig
*******************************************************************************/
static char *  pPFileName = NULL ;
static PTraceSuperGroup _groups[MaxSuperGroupsNum] ;
static unsigned         _num ;

static char *createProfileName(char *) ;

static const char *sectionName (const char *grpName)
{
   static char buf[32] ;
   if (!*grpName)
     grpName = SECTION_DEFAULT ;
   return strcat (strcat (strcpy (buf, grpName), "."), SECTION_EXT) ;
}

inline PTraceConfig::iterator PTraceConfig::begin()
{
   return _groups ;
}

inline PTraceConfig::iterator PTraceConfig::end()
{
   return _groups + _num ;
}

bool PTraceConfig::readProfile()
{
   LOCK_CONTEXT() ;

   char buf[PCOMN_MAXPATH] ;

   const char *cfgfile = profileFileName() ;

#define DIAG_READ_FLAG(flag, group, keyname, yes_no_op)                  \
   PDiagBase::mode(flag, yes_no_op(cfgfile_get_intval(cfgfile, group##_GROUP, keyname, yes_no_op(PDiagBase::mode() & flag))))

   // Do we output trace at all?
   DIAG_READ_FLAG(DisableDebugOutput, TRACE, KEY_ENABLED,    !) ;
   DIAG_READ_FLAG(AppendTrace,        TRACE, KEY_APPEND,     !!) ;

   DIAG_READ_FLAG(EnableFullPath,     SHOW, KEY_FULLPATH,   !!) ;
   DIAG_READ_FLAG(DisableLineNum,     SHOW, KEY_LINENUM,    !) ;
   DIAG_READ_FLAG(ShowThreadId,       SHOW, KEY_THREADID,   !!) ;
   DIAG_READ_FLAG(ShowProcessId,      SHOW, KEY_PID,        !!) ;
   DIAG_READ_FLAG(ShowLogLevel,       SHOW, KEY_LEVEL,      !!) ;

   // Where to write the trace into?
   // If there is no corresponding profile item, don't change that was before
   // the call to readProfile
   if (cfgfile_get_value(cfgfile, TRACE_GROUP, KEY_LOGNAME, buf, ""))
      PDiagBase::setlog(buf) ;

   std::pair<bool, unsigned char> level ;

   const auto get_level = [&](const char *section, const char *key)
   {
      if (!cfgfile_get_value(cfgfile, section, key, buf, ""))
         return false ;

      unsigned ena, lev ;
      sscanf(buf, "%u %u", &(ena = 0), &(lev = DBGL_LOWLEV)) ;
      level = {!!ena, (unsigned char)lev} ;
      return true ;
   } ;

   for (PTraceSuperGroup *sg = begin() ; sg != end() ; ++sg)
   {
      sg->ena(!!cfgfile_get_intval(cfgfile, sectionName(sg->name()), SECTION_ENABLED, sg->enabled())) ;
      if (get_level(sectionName(sg->name()), KEY_FORCELEVEL))
      {
         sg->_force_enable = level.first ;
         sg->_force_level  = level.second ;
      }
   }

   for (PDiagBase::Properties **p = PDiagBase::begin() ; p != PDiagBase::end() ; ++p)
      if (get_level(sectionName((*p)->superName()), (*p)->subName()))
      {
         (*p)->ena(level.first) ;
         (*p)->level(level.second) ;
      }

   UNLOCK_CONTEXT() ;

   return true ;
}

bool PTraceConfig::writeProfile()
{
   LOCK_CONTEXT() ;

   char buf[64] ;

   cfgfile_write_value(profileFileName(), TRACE_GROUP, KEY_LOGNAME,
                       PDiagBase::logname()) ;

#define DIAG_WRITE_FLAG(flag, group, keyname, yes_no_op)                \
   cfgfile_write_value(profileFileName(), group##_GROUP, keyname,       \
                       numtostr(yes_no_op(PDiagBase::mode() & flag), buf))

   DIAG_WRITE_FLAG(DisableDebugOutput, TRACE, KEY_ENABLED,   !) ;
   DIAG_WRITE_FLAG(AppendTrace,        TRACE, KEY_APPEND,    !!) ;

   DIAG_WRITE_FLAG(EnableFullPath,     SHOW, KEY_FULLPATH,  !!) ;
   DIAG_WRITE_FLAG(DisableLineNum,     SHOW, KEY_LINENUM,   !) ;
   DIAG_WRITE_FLAG(ShowThreadId,       SHOW, KEY_THREADID,  !!) ;
   DIAG_WRITE_FLAG(ShowProcessId,      SHOW, KEY_PID,       !!) ;
   DIAG_WRITE_FLAG(ShowLogLevel,       SHOW, KEY_LEVEL,     !!) ;

   // Write enabled/disabled status for every supergroup
   for (iterator iter = begin() ; iter != end() ; ++iter)
      cfgfile_write_value(profileFileName(), sectionName((*iter).name()), SECTION_ENABLED,
                           numtostr((unsigned)(*iter).enabled(), buf)) ;

   // Write enabled/disabled status and effective diags level for every group
   for (PDiagBase::Properties **p = PDiagBase::begin() ; p != PDiagBase::end() ; ++p)
   {
      sprintf(buf, "%u %u", (unsigned)(*p)->enabled(), (unsigned)(*p)->level()) ;
      cfgfile_write_value(profileFileName(), sectionName((*p)->superName()), (*p)->subName(), buf) ;
   }

   UNLOCK_CONTEXT() ;

   return true ;
}

PTraceConfig::iterator PTraceConfig::get(const char *name)
{
   // Search the specified supergroup by name
   for (iterator iter = begin() ; iter != end() ; ++iter)
      if (!strcmp((*iter).name(), name))
         return iter ;
   return NULL ;
}

PTraceConfig::iterator PTraceConfig::insert(const PTraceSuperGroup &grp)
{
   iterator result = get (grp.name()) ;

   if (result)
     (*result).ena (grp.enabled()) ;
   else if (_num < MaxSuperGroupsNum)
     result = &(_groups[_num++] = grp) ;

   return result ;
}

const char *PTraceConfig::profileFileName(const char *name)
{
   static char pFileName[PCOMN_MAXPATH] ;

   if (!name)
      pPFileName = createProfileName(pFileName) ;
   else if (name != pPFileName)
   {
      pPFileName = strncpy(pFileName, name, sizeof pFileName - 1) ;
      // Fix path delimiters
      for (char *p = pPFileName ; *p ; ++p)
         if (*p == PCOMN_PATH_FOREIGN_DELIM)
            *p = PCOMN_PATH_NATIVE_DELIM ;
   }

   return pPFileName ;
}

const char *PTraceConfig::profileFileName()
{
   return profileFileName(pPFileName) ;
}

static char *createProfileName(char *buf)
{
   // The profile file has name PROGRAM_NAME.trace.ini, where PROGRAM_NAME is the current
   // executable name.
   // On Unix the profile is created in the current working directory.
   // On Windows the profile is created in the executable file directory.

#ifdef PCOMN_PL_WINDOWS
   char *ppos ;
   GetModuleFileNameA(NULL, buf, PCOMN_MAXPATH) ;
   ppos = strrchr(buf, '.') ;
   if (!ppos || ppos < strrchr(buf, PCOMN_PATH_NATIVE_DELIM))
      ppos = buf + strlen(buf) ;
   strcpy(ppos, ".trace.ini") ;
#else
   snprintf(buf, PCOMN_MAXPATH-1, "%s.trace.ini", program_invocation_short_name)  ;
#endif
   return buf ;
}

} // end of namespace diag
