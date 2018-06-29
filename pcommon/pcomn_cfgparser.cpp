/*-*- tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_cfgparser.cpp
 COPYRIGHT    :   Yakov Markovitch, 1998-2017. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Portable configuration file functions a la Windows
                  profile files API

 CREATION DATE:   29 May 1998
*******************************************************************************/
#include <pcomn_cfgparser.h>
#include <pcomn_string.h>
#include <pcomn_strslice.h>
#include <pcomn_regex.h>
#include <pcomn_handle.h>
#include <pcomn_integer.h>
#include <pcomn_safeptr.h>
#include <pcomn_mmap.h>
#include <pcomn_utils.h>
#include <pcomn_assert.h>
#include <pcomn_sys.h>
#include <pcomn_except.h>

#include <pcstring.h>

#include <set>

#include <stdio.h>
#include <stdlib.h>
#include <pcomn_unistd.h>
#include <ctype.h>

using namespace pcomn ;

#define ENSURE_FILEOP(call, err) { if ((call) < 0) { return (err) ; } }

typedef char linebuf_t[PCOMN_CFGPARSER_MAXLINEBUF] ;

static const char PATTERN_SCAN[] =
   "^[\t\f\v ]*("                                  // Skip whitespaces at the beginning
   "([;#][^\n\r]*)"                                // Comment (2)
   "|(\\[([^]\f\v\n\r]+)\\])"                      // Section (3-4, name==4)
   "|(([^\t\f\v\n\r =]+)[\t ]*=[\t ]*([^\n\r]*))"  // Value (5-7, key==6, value==7, value is left-unstripped)
   "|([\t\f\v ]*)"
   ")[\t\f\v ]*[\n\r]"
   ;

static const unsigned PATTERN_SCAN_NGROUPS   = 8 ;
static const unsigned PATTERN_SCAN_COMMENT   = 2 ;
static const unsigned PATTERN_SCAN_SECTION   = 4 ;
static const unsigned PATTERN_SCAN_KEY       = 6 ;
static const unsigned PATTERN_SCAN_VALUE     = 7 ;

enum LineKind {
   LineInvalid = 0,
   LineEmpty   = 1,
   LineComment = PATTERN_SCAN_COMMENT,
   LineSection = PATTERN_SCAN_SECTION,
   LineValue   = PATTERN_SCAN_KEY - 1
} ;

static pcomn_regex_t pattern_scan ;

static int initPatterns(void)
{
   static int pattern_init (!pcomn_regcomp(&pattern_scan, PATTERN_SCAN, 0)) ;
   NOXCHECK(pattern_init != 0) ;
   return pattern_init ;
}

static inline reg_match &rstrip_match(const char *line, reg_match &value)
{
   while(PSUBEXP_LENGTH(value) && isspace(PSUBEXP_END(line, value)[-1]))
      --value.rm_len ;
   return value ;
}

static LineKind lineKind(const char *begin_line, const char *end_line,
                         reg_match &name,
                         reg_match &value)
{
   reg_match groups[PATTERN_SCAN_NGROUPS] ;
   static const reg_match empty = {-1, -1} ;
   name = value = empty ;

   NOXPRECONDITION(begin_line && end_line && begin_line <= end_line) ;
   NOXPRECONDITION((one_of<'\n', '\r'>::is(*end_line))) ;

   if (begin_line == end_line)
      return LineEmpty ;

   if (!regmatch(&pattern_scan, begin_line, groups))
      return LineInvalid ;

   if (!PSUBEXP_EMPTY(groups[PATTERN_SCAN_SECTION]))
   {
      name = groups[PATTERN_SCAN_SECTION] ;
      return LineSection ;
   }

   if (!PSUBEXP_EMPTY(groups[PATTERN_SCAN_KEY]))
   {
      name = groups[PATTERN_SCAN_KEY] ;
      value = groups[PATTERN_SCAN_VALUE] ;
      rstrip_match(begin_line, value) ;
      return LineValue ;
   }

   if (!PSUBEXP_EMPTY(groups[PATTERN_SCAN_COMMENT]))
   {
      value = groups[PATTERN_SCAN_COMMENT] ;
      return LineComment ;
   }
   return LineEmpty ;
}

static inline char *atName(LineKind kind, const char *line, char *name)
{
   reg_match namegrp, dummmygrp ;
   NOXPRECONDITION(line && name) ;
   return
      // The line _shall_ end with '\n' or '\r'
      (!*line || lineKind(line, line + strlen(line) - 1, namegrp, dummmygrp) != kind)
      ? NULL
      : regstrcpy(name, line, &namegrp) ;
}

static inline char *atValue(const char *line, char *keyname)
{
   return atName(LineValue, line, keyname) ;
}

static inline char *atSectionHeader(const char *line, char *sectionname)
{
   return atName(LineSection, line, sectionname) ;
}

static inline bool isSection(const char *found_section, const char *sectname)
{
   return !stricmp(found_section, sectname) ;
}

static inline char *atValue(const char *line, const char *key, char *value)
{
   NOXPRECONDITION(line && key && value) ;

   reg_match keygrp, valgrp ;
   // The line _shall_ end with '\n' or '\r'
   if (!*line || lineKind(line, line + strlen(line) - 1, keygrp, valgrp) != LineValue
       || (ssize_t)strlen(key) != PSUBEXP_LENGTH(keygrp)
       // Note memicmp(), the keys are case-insensitive (modulo ASCII)
       || memicmp(PSUBEXP_BEGIN(line, keygrp), key, PSUBEXP_LENGTH(keygrp)))
      return NULL ;

   return regstrcpy(value, line, &valgrp) ;
}

static inline char *line_normalize(char *line)
{
   const size_t len = strlen(line) ;
   if (!one_of<'\n', '\r'>::is(line[len - 1]))
   {
      line[len] = '\n' ;
      line[len + 1] = 0 ;
   }
   return line ;
}

template<size_t bufsize>
static inline char *getline_normalize(FILE *f, char (&buf)[bufsize])
{
   return !fgets(buf, bufsize-1, f) ? (char *)NULL : line_normalize(buf) ;
}

static size_t addResultToBuf(char **bufp, size_t *remains, const strslice &result, std::set<std::string> *found)
{
   // Check for the first appearance
   if (found && !found->insert(str::stdstr(result)).second)
      return 0 ;
   const size_t sz = std::min(result.size() + 1, *remains) ;
   char * const buf = *bufp ;

   memmove(buf, result.begin(), sz - 1) ;
   buf[sz - 1] = 0 ;
   *remains -= sz ;
   *bufp += sz ;
   return sz ;
}

static size_t readAllSectionNames(FILE *f, char *buf, size_t bufsize)
{
   NOXPRECONDITION(f && bufsize && buf) ;

   char *bufp = buf ;

   linebuf_t linebuf ;
   std::set<std::string> found ;
   size_t remains = bufsize ;
   while (remains && getline_normalize(f, linebuf))
      if (atSectionHeader(linebuf, linebuf))
         addResultToBuf(&bufp, &remains, linebuf, &found) ;

   // If there was enough space, return the buffer size - 1 (don't count the final null);
   // if the buffer is too small, return the _full_ buffer size

   bufp[remains ? 0 : -2] = 0 ;
   return bufp - buf ;
}

static size_t readAllSectionValues(FILE *f, const char *section, char *buf, size_t bufsize)
{
   NOXPRECONDITION(f && bufsize && buf && !*buf) ;
   NOXPRECONDITION(section) ;

   char *bufp = buf ;
   bool in_section = !*section ;

   linebuf_t linebuf ;
   std::set<std::string> found ;
   size_t remains = bufsize ;
   while (remains && getline_normalize(f, linebuf))
      if (atSectionHeader(linebuf, linebuf))
         in_section = isSection(linebuf, section) ;
      else if (in_section)
      {
         reg_match keygrp, valgrp ;
         if (lineKind(linebuf, linebuf + strlen(linebuf) - 1, keygrp, valgrp) == LineValue &&
             addResultToBuf(&bufp, &remains, strslice(linebuf, keygrp), &found) &&
             remains)
         {
            bufp[-1] = '=' ;
            *bufp = 0 ;
            addResultToBuf(&bufp, &remains, strslice(linebuf, valgrp), NULL) ;
         }
      }

   bufp[remains ? 0 : -2] = 0 ;
   return bufp - buf ;
}

size_t cfgfile_get_value(const char *filename,
                         const char *section, const char *key,
                         char *buf, size_t bufsize,
                         const char *defval)
{
   if (!buf || !bufsize)
      return 0 ;
   // Just in case, the output buffer should in any case be terminated.
   buf[bufsize-1] = buf[bufsize > 1] = buf[0] = 0 ;

   const char *result = defval ? defval : "" ;
   if (!filename)
      return strlen(strncpy(buf, result, bufsize - 1)) ;

   initPatterns() ;

   safe_handle<FILE_handle_tag> f (fopen(filename, "r")) ;

   if (!f)
      return 0 ;
   else if (!section)
      return readAllSectionNames(f, buf, bufsize) ;
   else if (!key)
      return readAllSectionValues(f, section, buf, bufsize) ;

   linebuf_t linebuf ;

   /* key=value pairs that follow before the first section belong to the "empty" section */
   for (bool in_section = !*section ; getline_normalize(f, linebuf) ;)
      if (atSectionHeader(linebuf, linebuf))
         in_section = isSection(linebuf, section) ;
      else if (in_section && atValue(linebuf, key, linebuf))
      {
         result = linebuf ;
         break ;
      }

   return strlen(strncpy(buf, result, bufsize - 1)) ;
} ;

int cfgfile_get_intval(const char *filename, const char *section, const char *key, int defval)
{
   char buf[64] ;
   return (section && key && filename && cfgfile_get_value(filename, section, key, buf))
      ? atoi(buf)
      : defval ;
}

static bool substPart(FILE *file, const char *newstr, fileoff_t range_begin, fileoff_t range_end)
{
   NOXCHECK(range_begin >= 0 && range_end >= 0) ;
   NOXCHECK(range_begin <= range_end) ;

   setbuf(file, NULL) ;
   const fileoff_t fsz = sys::filesize(fileno(file)) ;
   ENSURE_FILEOP(fsz, false) ;
   const ssize_t slen = newstr ? strlen(newstr) : 0 ;
   const fileoff_t newfsz = fsz + (slen - (range_end - range_begin)) ;
   if (newfsz > fsz || !newfsz)
   {
      ENSURE_FILEOP(ftruncate(fileno(file), newfsz), false) ;
      if (!newfsz)
         return true ;
   }
   try {
      PMemMapping mem (fileno(file), O_RDWR) ;
      memmove(mem.cdata() + (range_end + (newfsz - fsz)),
              mem.cdata() + range_end,
              fsz - range_end) ;
      memmove(mem.cdata() + range_begin, newstr, slen) ;
   }
   catch (const std::exception &)
   {
      return false ;
   }
   if (newfsz < fsz)
      ENSURE_FILEOP(ftruncate(fileno(file), newfsz), false) ;
   return true ;
}

static bool delPart(FILE *file, fpos_t range_begin, fpos_t range_end)
{
   fileoff_t begin, end ;
   ENSURE_FILEOP(fsetpos(file, &range_end), false) ;
   ENSURE_FILEOP(end = ftell(file), false) ;
   ENSURE_FILEOP(fsetpos(file, &range_begin), false) ;
   ENSURE_FILEOP(begin = ftell(file), false) ;
   return
      substPart(file, NULL, begin, end) ;
}

static bool deleteSection(FILE *file, const char *section)
{
   if (!section)
      section = "" ;

   linebuf_t linebuf ;
   bool in_section = !*section ;

   fpos_t pos ;
   ENSURE_FILEOP(fgetpos(file, &pos), false) ;
   fpos_t startpos = pos ;
   fpos_t endpos   = pos ;
   while (getline_normalize(file, linebuf))
   {
      if (atSectionHeader(linebuf, linebuf))
      {
         if (!isSection(linebuf, section))
         {
            if (in_section)
            {
               if (!*section)
                  return delPart(file, startpos, pos) ;
               if (!delPart(file, startpos, endpos))
                  return false ;
               in_section = false ;
            }
         }
         else
         {
            ENSURE_FILEOP(fgetpos(file, &endpos), false) ;
            if (!in_section)
            {
               in_section = true ;
               startpos = pos ;
            }
         }
      }
      else if (in_section && atValue(linebuf, linebuf))
         ENSURE_FILEOP(fgetpos(file, &endpos), false) ;

      ENSURE_FILEOP(fgetpos(file, &pos), false) ;
   }

   return
      !in_section || delPart(file, startpos, endpos) ;
}

static bool deleteValue(FILE *file, const char *section, const char *key)
{
   NOXCHECK(key) ;
   linebuf_t linebuf ;
   fpos_t pos ;
   int status ;
   bool in_section = !*section ;
   while ((status = fgetpos(file, &pos)) == 0 && getline_normalize(file, linebuf))
      if (atSectionHeader(linebuf, linebuf))
         if (*section)
            in_section = isSection(linebuf, section) ;
         else
            break ;
      else if (in_section && atValue(linebuf, key, linebuf))
      {
         fpos_t endpos ;
         ENSURE_FILEOP(fgetpos(file, &endpos), false) ;
         if (!delPart(file, pos, endpos))
            return false ;
      }
   return status == 0 ;
}

static inline bool isPrevLf(FILE *file)
{
   return fseek(file, -1, SEEK_CUR) < 0 || fgetc(file) == '\n' ;
}

int cfgfile_write_value(const char *filename, const char *section, const char *key, const char *value)
{
   initPatterns() ;

   if (!key || !value)
   {
      safe_handle<FILE_handle_tag> file (fopen (filename, "r+")) ;
      return file &&
         (!key && deleteSection(file, section) ||
          !value && deleteValue(file, section, key)) ;
   }

   safe_handle<FILE_handle_tag> file (fdopen(open(filename,
                                                  O_RDWR|O_CREAT|O_TEXT,
                                                  S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH), "w+t")) ;
   if (!file)
      return 0 ;

   if (!section)
      section = "" ;

   linebuf_t linebuf ;
   fpos_t prevpos, sectpos ;
   int status = fgetpos(file, &prevpos) ;
   sectpos = prevpos ;
   bool in_section    = !*section ;
   bool section_found = in_section ;
   bool nonempty_line = false ;

   for (sectpos = prevpos ; !status && getline_normalize(file, linebuf) ; status = fgetpos(file, &prevpos))
      if ((nonempty_line = atSectionHeader(linebuf, linebuf)) != false)
      {
         if (!*section)
            break ;
         in_section = isSection(linebuf, section) ;
         if (in_section && !section_found)
         {
            ENSURE_FILEOP(fgetpos(file, &sectpos), 0) ;
            section_found = true ;
         }
      }
      else
      {
         const size_t oldlen = strlen(linebuf) - 1 ; // Take '\n' into account
         nonempty_line = atValue(linebuf, linebuf) ;
         if (in_section && nonempty_line)
         {
            if (stricmp(linebuf, key))
            {
               ENSURE_FILEOP(fgetpos(file, &sectpos), 0) ;
            }
            else
            {
               // Requested key found, replace the old value with the new.
               ENSURE_FILEOP(fsetpos(file, &prevpos), 0) ;
               const fileoff_t start = ftell(file) ;
               ENSURE_FILEOP(start, 0) ;
               snprintf(linebuf, sizeof linebuf, "%s = %s", key, value) ;
               return
                  substPart(file, linebuf, start, start + oldlen) ;
            }
         }
      }
   if (!section_found)
   {
      ENSURE_FILEOP(fgetpos(file, &sectpos), false) ;
      nonempty_line = nonempty_line || !isPrevLf(file) ;
      ENSURE_FILEOP(fsetpos(file, &sectpos), false) ;
      return fprintf
         (file, "%s[%s]\n%s = %s\n\n", nonempty_line ? "\n" : "", section, key, value) > 0 ;
   }

   // There is a section but there is no such key in it.
   // Insert the key = value pair after the last value.
   ENSURE_FILEOP(fsetpos(file, &sectpos), 0) ;
   const fileoff_t start = ftell(file) ;
   ENSURE_FILEOP(start, 0) ;
   snprintf(linebuf, sizeof linebuf,
            "%s%s = %s%s", start && !isPrevLf(file) ? PCOMN_EOL_NATIVE : "", key, value,
            !start ? (PCOMN_EOL_NATIVE PCOMN_EOL_NATIVE) : PCOMN_EOL_NATIVE) ;
   return
      substPart(file, linebuf, start, start) ;
} ;

static char *cfgfile_get_sequence(const char *filename, const char *section, size_t buf_initsz)
{
   pcomn::malloc_ptr<char[]> buf ;
   for (size_t bufsz = buf_initsz ;
        buf.reset(pcomn::ensure_allocated((char *)malloc(bufsz))),
           cfgfile_get_value(filename, section, NULL, buf.get(), bufsz, NULL) == bufsz ;
        buf.reset(), bufsz *= 2) ;

   return buf.release() ;
}

char *cfgfile_get_sectnames(const char *filename)
{
   return cfgfile_get_sequence(filename, NULL, 256) ;
}

char *cfgfile_get_section(const char *filename, const char *section)
{
   return cfgfile_get_sequence(filename, section, 4096) ;
}
