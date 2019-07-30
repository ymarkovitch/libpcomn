/*-*- mode: c++; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __UNITTEST_JOURNAL_H
#define __UNITTEST_JOURNAL_H
/*******************************************************************************
 FILE         :   unittest_journal.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Test fixture for testing pcomn::jrn classes.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   18 Dec 2008
*******************************************************************************/
#include <pcomn_unittest.h>
#include <pcomn_journal/journal.h>
#include <pcomn_string.h>
#include <pcomn_handle.h>
#include <pcomn_except.h>
#include <pcomn_trace.h>
#include <pcomn_sys.h>
#include <pcomn_alloca.h>
#include <pcomn_unistd.h>

extern const char JOURNAL_FIXTURE[] = "journal" ;

/*******************************************************************************
                            struct JournalFixture
*******************************************************************************/
class JournalFixture : public pcomn::unit::TestFixture<JOURNAL_FIXTURE> {
      typedef pcomn::unit::TestFixture<JOURNAL_FIXTURE> ancestor ;
   public:
      void cleanupDirs()
      {
         ancestor::cleanupDirs() ;
         _dirfd.reset(PCOMN_ENSURE_POSIX(pcomn::sys::opendirfd(dataDir().c_str()), "opendir")) ;
      }

      void tearDown()
      {
         _dirfd.reset() ;
         ancestor::tearDown() ;
      }

      template<typename S>
      pcomn::enable_if_strchar_t<S, char, std::string>
      journalPath(const S &path)
      {
         return  (*pcomn::str::cstr(path) == PCOMN_PATH_NATIVE_DELIM)
            ? std::string(pcomn::str::cstr(path))
            : std::string(dataDir()).append(1, PCOMN_PATH_NATIVE_DELIM).append(pcomn::str::cstr(path)) ;
      }

      int dirfd() const { return _dirfd.handle() ; }

      template<typename S>
      static pcomn::enable_if_strchar_t<S, char, struct stat>
      filestat(const S &path)
      {
         return pcomn::sys::filestat(pcomn::str::cstr(path), pcomn::RAISE_ERROR) ;
      }

      template<typename S>
      static pcomn::enable_if_strchar_t<S, char, struct stat>
      linkstat(const S &path)
      {
         return pcomn::sys::linkstat(pcomn::str::cstr(path)) ;
      }

      template<typename S>
      static pcomn::enable_if_strchar_t<S, char, std::string>
      linkdata(const S &path)
      {
         const size_t sz = PATH_MAX + 1 ;
         char *buf = P_ALLOCA(char, sz) ;
         buf[PCOMN_ENSURE_POSIX(::readlink(pcomn::str::cstr(path), buf, sz - 1), "readlink")] = 0 ;
         return std::string(buf) ;
      }

      template<typename S>
      static pcomn::enable_if_strchar_t<S, char, std::set<std::string>>
      ls(const S &path)
      {
         typedef std::set<std::string> result_type ;

         pcomn::dir_safehandle dir (::opendir(pcomn::str::cstr(path))) ;
         PCOMN_THROW_IF(!dir, pcomn::system_error,
                        "Cannot open directory '%s' for reading", pcomn::str::cstr(path)) ;

         result_type result ;
         for (struct dirent *entry ; (entry = ::readdir(dir)) != NULL ; result.insert(entry->d_name)) ;
         return result ;
      }

   private:
      pcomn::fd_safehandle _dirfd ;
} ;

#endif /* __UNITTEST_JOURNAL_H */
