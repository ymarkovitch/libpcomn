/*-*- tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_shutil.cpp
 COPYRIGHT    :   Yakov Markovitch, 2011-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   UNIX/Linux implementation of high-level operations on files and
                  collections of files.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   2 Jun 2011
*******************************************************************************/
#include <pcomn_shutil.h>
#include <pcomn_exec.h>
#include <pcomn_path.h>
#include <pcomn_except.h>
#include <pcomn_sys.h>

#include <unistd.h>
#include <sys/types.h>
#include <ftw.h>

namespace pcomn {
namespace sys {

namespace {
struct need_quote {
      bool operator()(char c) const { return c <= ' ' || c == '$' || c == '\\' || c == '"' || c == '\'' ; }
} ;
}

static std::string &append_semiquoted(std::string &cmdline, const pcomn::strslice &name)
{
   cmdline.reserve(cmdline.size() + name.size()) ;
   const char *b, *e ;
   for (b = name.begin() ; (e = std::find_if(b, name.end(), need_quote())) != name.end() ; b = ++e)
      cmdline.append(b, e).append(1, '\\').append(1, *e) ;

   return cmdline.append(b, name.end()) ;
}

/*******************************************************************************
 cp
*******************************************************************************/
static const unsigned CP_SOURCE_DIR = 0x8000 ;

static bool exec_cp(const char *cmd, const pcomn::strslice &source, const pcomn::strslice &dest, unsigned flags)
{
   PCOMN_ENSURE_ARG(source) ;
   PCOMN_ENSURE_ARG(dest) ;

   std::string cmdline ;
   cmdline.reserve(source.size() + dest.size() + 64) ;

   cmdline.append(cmd) ;
   if (flags & CP_SRC_ALLOW_DIR)
      cmdline.append(" -R") ;
   if (!(flags & CP_DONT_PRESERVE))
      cmdline.append(" -p") ;
   if (flags & CP_FOLLOW_ALL_LINKS)
      cmdline.append(" -L") ;
   else if (flags & CP_FOLLOW_SRC_LINKS)
      cmdline.append(" -H") ;
   else
      // Don't follow any links
      cmdline.append(" -P") ;

   append_semiquoted(cmdline.append(" \""), source) ;
   if ((flags & CP_SOURCE_DIR) && source.back() != '/')
      cmdline.append(1, '/') ;

   append_semiquoted(cmdline.append("\" \""), dest) ;
   if ((flags & CP_DST_REQUIRE_DIR) && dest.back() != '/')
      cmdline.append(1, '/') ;
   cmdline.append(1, '"') ;

   // Swallow stdout, capture stderr
   cmdline.append(" 2>&1 1>/dev/null") ;

   const pcomn::RaiseError raise_error =
      (flags & CP_IGNORE_ERRORS) ? pcomn::DONT_RAISE_ERROR : pcomn::RAISE_ERROR ;

   return
      shellcmd(cmdline, raise_error, 64*KiB).first == 0 ;
}

bool copyfile(const pcomn::strslice &source, const pcomn::strslice &dest, unsigned flags)
{
   return exec_cp("cp", source, dest, flags &~ CP_SOURCE_DIR) ;
}

bool copytree(const pcomn::strslice &sourcedir, const pcomn::strslice &destdir, unsigned flags)
{
   return exec_cp("cp", sourcedir, destdir, flags | CP_SOURCE_DIR | CP_SRC_ALLOW_DIR) ;
}

/*******************************************************************************
 rm
*******************************************************************************/
typedef std::function<void(int, const char *, const fsstat &)> skip_logger ;

namespace {
struct rm_handler {
      rm_handler(RmFlags flags, const skip_logger &logger) :
         _flags(flags), _skiplogger(logger)
      {}

      const rmstat &info() const { return _info ; }
      rmstat &info() { return _info ; }
      const std::exception_ptr &last_exception() { return _exception ; }

      int rmfile(const char *fpath, const fsstat &s, int typeflag)
      {
         int lasterr = 0 ;

         auto unlink_file = [this](const char *fpath, const fsstat &s)
         {
            const int err = posix_errno(unlink(fpath)) ;
            _info._rm_size += s.st_size * !err * !!S_ISREG(s.st_mode) ;
            return err ;
         } ;

         ++_info._visit_count ;

         if (typeflag == FTW_DP)
         {
            // directory, after processing contents
            lasterr = posix_errno(rmdir(fpath)) ;
            if (lasterr == ENOTDIR)
               lasterr = unlink_file(fpath, filestat(fpath)) ;
         }
         else
         {
            lasterr = unlink_file(fpath, s) ;
            if (lasterr == EISDIR)
               lasterr = posix_errno(rmdir(fpath)) ;
         }

         if (!lasterr)
            return 0 ;

         if (lasterr == ENOENT && (_flags & RM_IGNORE_NEXIST))
            --_info._visit_count ;
         else
         {
            ++_info._skip_count ;
            append_exception(lasterr, fpath, s) ;
            errno = lasterr ;
         }
         return -!!_exception  ;
      }

   private:
      RmFlags     _flags ;
      skip_logger _skiplogger ;
      rmstat      _info ;
      std::string _xinfo ;
      std::exception_ptr _exception ;

   private:
      void append_exception(int err, const char *fpath, const fsstat &s) noexcept
      {
         if (_skiplogger)
            try { _skiplogger(err, fpath, s) ; } catch (...) {} ;

         if (_flags & RM_IGNORE_ERRORS)
            return ;

         try {
            PCOMN_CHECK_POSIX((errno = err) ? -1 : 0,
                              _exception ? "%s\n%s '%s'" : "%s%s '%s'",
                              _xinfo.c_str(), "Error while attempting to remove", fpath) ;
         }
         catch (std::exception &x)
         {
            _exception = std::current_exception() ;
            _xinfo = x.what() ;
         }
      }
} ;
} // end of unnamed namespace

rmstat rm(const pcomn::strslice &path, const skip_logger &skiplogger, RmFlags flags)
{
   PCOMN_ENSURE_ARG(path) ;

   using std::invalid_argument ;
   const pcomn::RaiseError raise_error =
      (flags & RM_IGNORE_ERRORS) ? pcomn::DONT_RAISE_ERROR : pcomn::RAISE_ERROR ;

   const std::string &rpath = path.stdstring() ;
   if (!(flags & RM_ALLOW_RELPATH) && !path::is_absolute(path))
   {
      PCOMN_THROW_IF(raise_error, invalid_argument,
                     "Calling rm without RM_ALLOW_RELPATH for relative path '%s' is not allowed", rpath.c_str()) ;
      return false ;
   }

   const std::string &spath = path::abspath<std::string>(rpath) ;
   if (!(flags & RM_ALLOW_ROOTDIR) && std::count(spath.begin(), spath.end(), '/') < 2)
   {
      PCOMN_THROW_IF(raise_error, invalid_argument,
                     "Calling rm without RM_ALLOW_ROOTDIR to delete file(s) '%s' "
                     "directly from the root directory is not allowed", spath.c_str()) ;
      return false ;
   }

   /****************************************************************************
    Walk along the directory tree
   ****************************************************************************/
   rm_handler handler (flags, skiplogger) ;

   static thread_local rm_handler *current_handler = nullptr ;
   const vsaver<rm_handler *> set_handler (current_handler, &handler) ;

   const fsstat &topstat = filestat(spath) ;

   const int typeflag = !topstat                   ? FTW_NS :
                        S_ISDIR(topstat.st_mode)   ? FTW_DP :
                        S_ISLNK(topstat.st_mode)   ? FTW_SL :
                        FTW_F ;

   const int lasterr = typeflag == FTW_NS
      ? errno
      : posix_errno(typeflag == FTW_DP && (flags & RM_RECURSIVE)

                    // The specified path is a directory: walk through the directory tree
                    ? nftw(spath.c_str(),
                           [](const char *fpath, const struct stat *sb, int typeflag, FTW *)
                           {
                              return current_handler->rmfile(fpath, *sb, typeflag) ;
                           },
                           128, FTW_DEPTH | FTW_PHYS)

                    // The specified path is a file: remove it immediatrely
                    : handler.rmfile(spath.c_str(), topstat, typeflag)) ;

   if (handler.last_exception())
      std::rethrow_exception(handler.last_exception()) ;

   switch (lasterr)
   {
      case 0: break ;
      case ENOENT: if (flags & RM_IGNORE_NEXIST) break ;

      default:
         handler.info()._skip_count = std::max(handler.info()._skip_count, 1U) ;
         PCOMN_CHECK_POSIX(raise_error ? -1 : 0, "Error while removing '%s'", spath.c_str()) ;
   }

   return handler.info() ;
}

} // end of namespace pcomn::sys
} // end of namespace pcomn
