/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   journmmap.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Journalling engine storage implemented on memory-mappable filesystem.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   12 Nov 2008
*******************************************************************************/
#include "journmmap.h"

#include <pcomn_mmap.h>
#include <pcomn_string.h>
#include <pcomn_utils.h>
#include <pcomn_diag.h>
#include <pcomn_strnum.h>
#include <pcomn_strslice.h>
#include <pcomn_regex.h>
#include <pcomn_hash.h>
#include <pcomn_alloca.h>
#include <pcomn_fstream.h>
#include <pcomn_trace.h>

#include <pcstring.h>

#include <sys/uio.h>
#include <errno.h>

#define JNAME_VALID_CHARCLASS "][{}()a-zA-Z0-9_@+=~.,-"

#define LOGERR(output)  LOGPXERR(PCOMN_Journmmap, output)
#define LOGWARN(output) LOGPXWARN(PCOMN_Journmmap, output)
#define LOGINFO(output) LOGPXINFO(PCOMN_Journmmap, output)
#define LOGDBG(output)  LOGPXDBG(PCOMN_Journmmap, DBGL_ALWAYS, output)

namespace pcomn {
namespace jrn {

static const size_t MAX_ALLOCA = 16384 ; /* Alloca limit (allocate larger memory pieces
                                          * with malloc) */

static const magic_t ZERO_MAGIC = {{}} ;

template<typename S>
enable_if_strchar_t<S, char, std::string>
backup_name(const S &path, unsigned n, const char *add_ext = NULL)
{
   if (!add_ext)
      add_ext = "" ;
   return n
      ? strprintf("%s.%u%s", str::cstr(path), n, add_ext)
      : strprintf("%s%s", str::cstr(path), add_ext)  ;
}

/******************************************************************************/
/** binary_ostream over MMapStorage::RecFile
*******************************************************************************/
class binary_orecstream : public binary_ostream {
   public:
      explicit binary_orecstream(MMapStorage::RecFile &file) :
         _file(file)
      {}

   protected:
      size_t write_data(const void *buf, size_t size) { return _file.write(buf, size) ; }
   private:
      MMapStorage::RecFile &_file ;
} ;

/*******************************************************************************
 MMapStorage
*******************************************************************************/
const char MMapStorage::EXT_CHECKPOINT[]  = "." PJRNMMAP_EXT_CHKPOINT ;
const char MMapStorage::EXT_SEGMENT[]     = "." PJRNMMAP_EXT_SEGMENT ;
const char MMapStorage::EXT_SEGDIR[]      = ".segments" ;
const char MMapStorage::EXT_TAKING[]      = ".taking" ;

MMapStorage::MMapStorage(const strslice &journal_path,
                         AccMode access_mode, unsigned open_flags,
                         size_t cpstream_bufsz) :

   _name(ensure_name_form_path(journal_path).stdstring()),
   _dirname(journal_dir_abspath(journal_path)),
   _cpdirfd(open_dir()),
   _segments(0, true),
   _last_id(0),
   _lastgen(0),
   _cpstream_bufsz(cpstream_bufsz),

   _nosegdir((open_flags & OF_NOSEGDIR) ||
             access_mode == MD_WRONLY ||
             // Check for the presence of segments sudirectory (possibly, a symbolic link)
             faccessat(dirfd(), pcomn::str::cstr(make_filename(name(), EXT_SEGDIR)), F_OK, 0)),

   _nobakseg(!!(open_flags & OF_NOBAKSEG))
{
   if (access_mode == MD_WRONLY ||

       access_mode == MD_RDWR &&
       (open_flags & OF_CREAT) &&
       faccessat(dirfd(), pcomn::str::cstr(checkpoint_name()), F_OK, AT_EACCESS))

      create_storage("") ;
   else
      open_storage(access_mode == MD_RDONLY) ;
}

// This constructor behave as if MD_WRONLY is set
MMapStorage::MMapStorage(const strslice &journal_path, const strslice &segdir_path,
                         unsigned open_flags,
                         size_t cpstream_bufsz) :

   _name(ensure_name_form_path(journal_path).stdstring()),
   _dirname(journal_dir_abspath(journal_path)),
   _cpdirfd(open_dir()),
   _segments(0, true),
   _last_id(0),
   _lastgen(0),
   _cpstream_bufsz(cpstream_bufsz),
   // TODO: should normalize the path
   _nosegdir((open_flags & OF_NOSEGDIR) || segdir_path.empty() || segdir_path == strslice(".")),
   _nobakseg(!!(open_flags & OF_NOBAKSEG))
{
   // If the segment directory name is not specified, use the checkpoint directory
   create_storage(segdir_path.stdstring().c_str()) ;
}

MMapStorage::~MMapStorage()
{
   TRACEPX(PCOMN_Journmmap, DBGL_ALWAYS, "Destructing " << *this) ;
   close() ;
}

bool MMapStorage::do_close_storage()
{
   TRACEPX(PCOMN_Journmmap, DBGL_ALWAYS, "Close " << *this) ;

   if (!_checkpoint && !_segment)
      return false ;

   switch (state())
   {
      case SST_CREATED:
         cleanup_storage(CST_COMPLETE) ;
         break ;

      case SST_READABLE:
      case SST_READONLY:
         // Close all readable segments
         close_segments() ;
         if (_checkpoint)
            _checkpoint->close() ;
         break ;


      case SST_WRITABLE:
         if (!_checkpoint)
         {
            NOXCHECK(_segment) ;
            if (!_segment->datasize())
               // Remove an empty segment
               cleanup_part(CST_SEGMENT, _segment->this_segment()) ;
         }
         else
         {
            const int64_t cpid = _checkpoint->next_segment() ;

            if (!one_of<RecFile::ST_CLOSED, RecFile::ST_READABLE>::is(_checkpoint->state()))
            {
               // While not properly closed segment is OK, nonclosed checkpoints must be removed
               cleanup_part(CST_CHECKPOINT, 0) ;

               if (!cpid)
               {
                  if (_segment)
                  {
                     NOXCHECK(!_segment->this_segment()) ;
                     cleanup_part(CST_SEGMENT, _segment->this_segment()) ;
                  }
                  // Zero generation and both the segment and checkpoint are already deleted;
                  // delete the journal completely
                  cleanup_part(CST_SYMLINK, 0) ;
               }
            }
         }
         break ;

      default:
         PCOMN_FAIL("Invalid MMapStorage state while closing a storage") ;
   }

   _segment.reset() ;
   _checkpoint.reset() ;

   return true ;
}

std::ostream &MMapStorage::debug_print(std::ostream &os) const
{
   return ancestor::debug_print(os)
      << ' ' << oenum(state()) << " '" << name() << "' gen " << _lastgen ;
}

/*******************************************************************************
 MMapStorage: name handling
*******************************************************************************/
#define PCOMN_ENSURE_ARGRANGE(begin, end)                         \
   (pcomn::ensure_arg<std::invalid_argument>                      \
    (!!(begin) ^ !(end), (begin) ? #end : #begin, __FUNCTION__))

strslice MMapStorage::ensure_name_form_path(const strslice &path)
{
   const strslice &name = journal_name_from_path(path) ;
   PCOMN_THROW_IF(!name, std::invalid_argument,
                  "Invalid journal path or name " P_STRSLICEQF, P_STRSLICEV(path)) ;

   return name ;
}

static const regex journalname_re ("^[" JNAME_VALID_CHARCLASS "]+$") ;

strslice MMapStorage::journal_name_from_path(const strslice &path)
{
   PCOMN_ENSURE_ARGRANGE(path.begin(), path.end()) ;

   const char *name_begin = (const char *)memrchr(path.begin(), '/', path.size()) ;
   if (!name_begin)
      name_begin = path.begin() ;
   else
      ++name_begin ;

   const size_t sz = path.end() - name_begin ;
   char namebuf[MAX_JNAME + 1] ;

   return
      inrange<size_t>(sz, 1, MAX_JNAME) && journalname_re.is_matched(strncpyz(namebuf, name_begin, sz + 1)) ?
      strslice(name_begin, path.end()) : strslice() ;
}

bool MMapStorage::is_valid_name(const strslice &journal_name)
{
   PCOMN_ENSURE_ARGRANGE(journal_name.begin(), journal_name.end()) ;

   const size_t sz = journal_name.size() ;
   char namebuf[MAX_JNAME + 1] ;
   return
      pcomn::inrange<size_t>(sz, 1, MAX_JNAME) &&
      journalname_re.is_matched(strslicecpy(namebuf, journal_name)) ;
}

static const regex splitname_re ("^([" JNAME_VALID_CHARCLASS "]+)(\\.[a-z]+)$") ;

static const regex splitpfx_re ("^(.+)(\\.([0-9]+))\\.[a-z]+$") ;

MMapStorage::FilenameKind MMapStorage::parse_internal(const char *filename,
                                                      std::string *journal_name,
                                                      uint64_t *id)
{
   PCOMN_ENSURE_ARG(filename) ;
   reg_match m[4] ;

   if (splitname_re.match(filename, m) == m ||
       // Check sizes of name part
       PSUBEXP_LENGTH(m[2]) > (int)MAX_JEXT)

      return NK_UNKNOWN ;

   const strslice ext (filename, m[2]) ;

   FilenameKind result ;

   if (ext == strslice(EXT_SEGDIR))
      result = NK_SEGDIR ;

   else if (ext == strslice(EXT_SEGMENT))
      result = NK_SEGMENT ;

   else if (ext == strslice(EXT_CHECKPOINT))
      result = NK_CHECKPOINT ;

   else
      return NK_UNKNOWN ;

   PSUBEXP_RESET(m[2]) ;

   if (result != NK_SEGDIR &&
       (splitpfx_re.match(filename, m) == m || PSUBEXP_LENGTH(m[2]) > (int)MAX_JGEN) ||
       PSUBEXP_LENGTH(m[1]) > (int)MAX_JNAME)

      return NK_UNKNOWN ;

   if (journal_name)
      std::string(PSUBEXP_BEGIN(filename, m[1]), PSUBEXP_END(filename, m[1])).swap(*journal_name) ;

   if (id)
      if (PSUBEXP_EMPTY(m[2]))
         *id = NOGEN ;
      else
      {
         char buf[MAX_JGEN + 1] ;
         // Add 1 to skip initial '.'
         strtonum(regstrcpy(buf, filename, m + 3), *id) ;
      }

   return result ;
}

/*******************************************************************************
 MMapStorage: storage initialization, create new
*******************************************************************************/
void MMapStorage::create_storage(const char *segdir_path)
{
   CreateStage stage = CST_INIT ;
   _lastgen = 0 ;
   try {
      create_storage(segdir_path, stage) ;
   }
   catch (const std::exception &x)
   {
      LOGERR("Error creating '" << name() << "': " << STDEXCEPTOUT(x)) ;
      cleanup_storage(stage) ;
      throw ;
   }
   catch (...)
   {
      LOGERR("Unknown exception creating '" << name() << "'") ;
      cleanup_storage(stage) ;
      throw ;
   }
}

static void cleanup_item(int dirfd, const char *name, const char *dirname, const char *item_kind)
{
   LOGDBG("Removing " << item_kind << " '" << name << "' from '" << dirname << "'") ;

   if (unlinkat(dirfd, name, 0) != 0)

      LOGWARN(strerror(errno) << " while removing " << item_kind << " '" << name
              << "' from '" << dirname << "'") ;
}

void MMapStorage::cleanup_part(CreateStage stage, uint64_t id)
{
   switch (stage)
   {
      case CST_SEGMENT:
         cleanup_item(_segdirfd, str::cstr(segment_name(id)),
                      str::cstr(segment_dirname()), "segment file") ;
         break ;

      case CST_CHECKPOINT:
         cleanup_item(dirfd(), str::cstr(checkpoint_name()),
                      str::cstr(dirname()), "checkpoint file") ;
         break ;

      case CST_SYMLINK:
         if (!nosegdir())
            cleanup_item(dirfd(), str::cstr(segment_dirname()),
                         str::cstr(dirname()), "symbolic link") ;
         break ;

      default: ;
   }
}

// Cleanup environment after failed attempt to create a storage
void MMapStorage::cleanup_storage(CreateStage last_stage)
{
   if (!last_stage)
      return ;

   if (last_stage != CST_COMPLETE)
      LOGWARN("Cleanup '" << dirname() << "' after failed attempt to create journal '"
              << name() << "' at stage " << last_stage) ;
   else
      LOGWARN("Storage for journal '" << name() << " at '" << dirname()
              << "' has never been written to. Removing storage files") ;

   for (CreateStage stage = last_stage ; stage > CST_INIT ; stage = (CreateStage)(stage - 1))
      cleanup_part(stage, 0) ;
}

void MMapStorage::create_storage(const char *segdirname, CreateStage &stage)
{
   NOXCHECK(segdirname) ;
   NOXCHECK(state() == SST_INITIAL) ;

   stage = CST_INIT ;

   LOGDBG("Creating " << *this << " at '" << dirname()
          << "' with '" << segdirname << "' as a segments directory") ;

   // First create a symbolic link to the segments directory.
   // We don't create the link if segdirname is "" or "."
   create_segdir_symlink(segdirname) ;

   stage = CST_SYMLINK ;

   // Attempt to create a checkpoint file for generation 0: since it is created in
   // O_EXCL mode, the creation will fail if such checkpoint is already present.
   new_checkpoint_file(0) ;

   stage = CST_CHECKPOINT ;

   // Attempt to open the segments directory
   open_segdir(true) ;

   LOGDBG(*this << " created OK") ;

   // Since there is neither checkpoints no segments yet, the generation is 0
   _lastgen = 0 ;

   set_state(SST_CREATED) ;
}

std::string MMapStorage::create_segdir_symlink(const char *segdirname)
{
   if (nosegdir())
      return std::string() ;

   const std::string &linkpath = segment_dirname() ;
   const std::string &segdirpath = path::abspath<std::string>
      (path::posix::is_rooted(segdirname)
       ? std::string(segdirname)
       : journal_abspath(segdirname)) ;

   if (linkpath == segdirpath)
      LOGDBG("No need for symlink, segments directory path and link path are equal: '" << linkpath) ;
   else
   {
      LOGDBG("Creating symlink to the segments directory '"
             << linkpath << "' -> '" << segdirpath << "' for " << *this) ;

      // If the full (absolute) journal directory path (i.e. the directory the checkpoint
      // file is placed to) is a prefix to the absolute segments directory path, make a
      // symbolic link using the relative path (segments directory relative wrt the journal
      // directory); otherwise, make a symlink to the absolute path.

      const char *segdir_relpath = segdirpath.c_str() ;
      if (str::startswith(segdirpath, _dirname))
         if (segdirpath.size() == _dirname.size())
            segdir_relpath = "." ;
         else if (*(segdir_relpath + _dirname.size()) == '/')
            segdir_relpath = segdir_relpath + _dirname.size() + 1 ;

      if (symlink(segdir_relpath, str::cstr(linkpath)))
         if (errno = EEXIST)
            throw_exception<journal_exists_error>(journal_abspath(name())) ;
         else
            throw_exception<system_error>
               (system_error::platform_specific,
                strprintf("Attempting to create a symlink '%s'->'%s'",
                          str::cstr(linkpath), segdir_relpath)) ;
   }

   return linkpath ;
}

MMapStorage::SegmentFile *MMapStorage::new_segment_file(int64_t id)
{
   std::string segment_filename = segment_name(id) ;
   std::string filename (segment_filename) ;

   LOGDBG("Create segment '" << segment_filename << "' for journal '" << name() << "'") ;

   PTSafePtr<SegmentFile> new_segment ;
   for (int i = 1 ; !new_segment ; ++i)
      // Repeat attempts to create a segment file until a free name is found
      try {
         TRACEPX(PCOMN_Journmmap, DBGL_ALWAYS, "Attempting to create '" << filename << "'") ;

         new_segment.reset(new SegmentFile(_segdirfd, filename, id, current_generation(), 0600)) ;
      }
      catch (const system_error &x)
      {
         if (x.posix_code() != EEXIST)
         {
            LOGERR("Cannot create '" << filename << "': " << x.posix_code() << ' ' << x.what()) ;
            throw ;
         }

         char buf[64] ;
         filename = segment_filename ;
         filename.append(1, '.').append(numtostr(i, buf)) ;
      }

   try {
      LOGDBG("Created '" << filename << "''") ;

      // Initialize new segment
      new_segment->init(user_magic()) ;

      // Check whether the actual name of the just created segment (filename) matches the
      // requested (segment_filename).
      if (filename != segment_filename)
      {
         // We weren't able to create the segment with its proper name, most probably
         // because the file with such name already exists
         const std::string &fullpath = segment_abspath(segment_filename) ;

         std::string bak ;
         if (!nobakseg())
         {
            // Create a hard link with a backup name to the existing segment
            for (unsigned i = 0 ; ::link(str::cstr(fullpath),
                                         str::cstr(bak = backup_name(fullpath, i, ".bak"))) ; ++i)
               if (errno != EEXIST)
                  PCOMN_THROW_SYSERROR("link") ;
         }

         if (::rename(str::cstr(segment_abspath(filename)), str::cstr(fullpath)))
         {
            if (!bak.empty())
            {
               const int err = errno ;
               unlink(str::cstr(bak)) ;
               errno = err ;
            }
            PCOMN_THROW_SYSERROR("rename") ;
         }
      }
   }
   catch (...)
   {
      cleanup_segment(filename) ;
      throw ;
   }

   // Commit the last segment
   if (_segment)
      _segment->commit() ;

   pcomn_swap(_segment, new_segment) ;

   return _segment.get() ;
}

MMapStorage::CheckpointFile *MMapStorage::new_checkpoint_file(int64_t nextseg_id)
{
   NOXCHECK(!_checkpoint) ;

   const std::string &filename = checkpoint_name() + (is_first_checkpoint() ? "" : EXT_TAKING) ;

   LOGDBG("Create checkpoint '" << filename << "' for journal '" << name()
          << "', the next segment is " << nextseg_id) ;

   _checkpoint.reset
      (new CheckpointFile(dirfd(), filename, nextseg_id - 1, current_generation(), 0600)) ;

   return _checkpoint.get() ;
}

/*******************************************************************************
 MMapStorage: storage initialization, open existing
*******************************************************************************/
void MMapStorage::open_storage(bool rdonly)
{
   _lastgen = NOGEN ;

   LOGDBG("Open " << (rdonly ? "read-only " : "") << "storage " << *this << " at '" << dirname() << "'") ;

   NOXCHECK(state() == SST_INITIAL) ;
   NOXCHECK(_cpdirfd.good()) ;
   NOXCHECK(_segdirfd.bad()) ;
   NOXCHECK(!_checkpoint) ;
   NOXCHECK(_segments.empty()) ;

   try {
      // Open a checkpoint
      _checkpoint.reset
         (new CheckpointFile(PCOMN_ENSURE_POSIX(::open(str::cstr(checkpoint_abspath()), O_RDONLY), "open"))) ;

      // Set the user magic
      set_user_magic(_checkpoint->user_magic()) ;

      _last_id = _checkpoint->next_segment() ;
      _lastgen = _checkpoint->generation() ;

      // So far so good - open a segment directory
      set_state(!open_segments() || rdonly ? SST_READONLY : SST_READABLE) ;
   }
   catch (const std::exception &x)
   {
      LOGERR("Error opening '" << name() << "': " << STDEXCEPTOUT(x)) ;
      throw ;
   }
   catch(...)
   {
      LOGERR("Unknown exception opening '" << name() << "'") ;
      throw ;
   }
}

bool MMapStorage::open_segdir(bool raise_on_error)
{
   NOXCHECK(_segdirfd.bad()) ;

   if (nosegdir())
   {
      TRACEPX(PCOMN_Journmmap, DBGL_HIGHLEV,
              "No separate segments directory, duplicating handle to'" << dirname() << "'") ;

      _segdirfd.reset(::dup(dirfd())) ;
   }
   else
   {
      const std::string &segpath = segment_dirname() ;

      TRACEPX(PCOMN_Journmmap, DBGL_HIGHLEV, "Opening segments directory '" << segpath << "'") ;

      // Try to open the segments directory. If failed, force the storage into read-only
      // mode.
      _segdirfd.reset(sys::opendirfd(str::cstr(segpath))) ;
   }

   if (_segdirfd.bad())
   {
      system_error syserror
         (std::string("Cannot open segments directory '").append(segment_dirname()).append("'")) ;
      if (raise_on_error)
         throw syserror ;

      LOGERR(syserror.what()) ;
      LOGERR("Forcing journal '" << name() << "' to read-only mode") ;
      return false ;
   }

   return true ;
}

bool MMapStorage::open_segments()
{
   if (!open_segdir(false))
      return false ;

   TRACEPX(PCOMN_Journmmap, DBGL_ALWAYS, "Opening segments for reading of " << *this) ;
   NOXCHECK(_segments.empty()) ;

   int64_t segid = _checkpoint->next_segment() ;
   generation_t prevgen = _checkpoint->generation() ;

   try {
      PTSafePtr<SegmentFile> psegment ;

      for (int fd ; (fd = ::openat(_segdirfd, str::cstr(segment_abspath(segid)), O_RDONLY)) >= 0 ;)
      {
         // FIXME: may leak fd on bad_alloc
         psegment.reset(new SegmentFile(fd)) ;

         if (psegment->user_magic() != user_magic())
         {
            LOGWARN("Segment '" << segment_abspath(segid)
                    << "' doesn't belong to journal '" << journal_abspath(name()) << "'") ;
            break ;
         }

         if (psegment->this_segment() != segid)
         {
            LOGWARN("Segment name of '" << segment_abspath(segid)
                    << "' doesn't match its id: " << psegment->this_segment()) ;
            break ;
         }

         if (_segments.empty())
         {
            // The first segment after the checkpoint must have generation equal to the
            // checkpoint's
            if (psegment->generation() != prevgen)
            {
               LOGWARN("Generation of segment '" << segment_abspath(segid)
                       << "' doesn't match the generation of the checkpoint") ;
               break ;
            }
         }

         /* FIXME: invalid generation calculations, rdoffs required!
         else if (psegment->generation() == prevgen)
            // Skip an empty segment
            _segments.pop_back() ;

         else if (psegment->generation() < prevgen ||
                  psegment->generation() > prevgen + _segments.back()->datasize())
         {
            LOGWARN("Invalid generation " << psegment->generation() << " in segment "
                    << segid << " of journal '" << journal_abspath(name()) << "'") ;
            break ;
         }
         */

         segid = psegment->next_segment() ;

         if (!psegment->datasize())
            TRACEPX(PCOMN_Journmmap, DBGL_ALWAYS, "Skipping empty " << *psegment) ;
         else
         {
            // Reserve in advance to ensure _segments.push_back() won't throw bad_alloc
            _segments.reserve(_segments.size() + 1) ;
            prevgen = psegment->generation() ;

            // Place the segment into the vector of readable segments
            _segments.push_back(psegment.release()) ;
         }
      }
   }
   catch(const std::bad_alloc &)
   {
      close() ;
      throw ;
   }
   catch(const std::exception &x)
   {
      LOGWARN("Invalid or corrupt segment file '" << segment_abspath(segid)
              << "' of journal='" << journal_abspath(name()) << "'") ;
   }

   TRACEPX(PCOMN_Journmmap, DBGL_HIGHLEV, "There are " << _segments.size()
           << " segments open for " << *this) ;

   std::reverse(_segments.begin(), _segments.end()) ;
   return true ;
}

/*******************************************************************************
 MMapStorage: Storage read methods
*******************************************************************************/
size_t MMapStorage::read_record(SegmentFile &segment, const record_handler &handler)
{
   const size_t header_size = sizeof(magic_t) + sizeof(OperationHeader) ;

   magic_t magic ;

   header_buffer<OperationHeader> header ;

   // Read the operation header
   // Immediately read sizeof(OperationHeader) part of the header data: actual header
   // size cannot (MUST not) be less than this anyway, though it _can_ be greater
   // according to header_size member; chances are, it is equal to
   // sizeof(OperationHeader) and we needn't the second read operation.
   const iovec_t iov_head[] =
      {
         make_iovec(&magic, sizeof magic),
         make_iovec(&header, sizeof (OperationHeader))
      } ;

   const size_t sz_head = segment.readv(iov_head) ;

   size_t sz_full = sz_head ;

   if (!sz_head)
   {
      TRACEPX(PCOMN_Journmmap, DBGL_ALWAYS, "End of segment " << segment) ;
      return 0 ;
   }

   PCOMN_VERIFY(sz_head <= header_size) ;

   if (sz_head < header_size || magic != STORAGE_OPERATION_MAGIC)
   {
      // Premature end of file or invalid operation header, the segment was not properly
      // closed
      LOGDBG(segment << " was not properly closed") ;
      if (sz_head)
         LOGWARN("The tail of " << segment << " is corrupt") ;

      return 0 ;
   }

   PTVSafePtr<char> data_guard ;
   char *data_buf ;

   try {
      uint32_t opcrc = calc_crc32(0, &header, sizeof(OperationHeader)) ;

      dtoh(header) ;

      const unsigned remheader_size =
         ensure_header_size<OperationHeader>(header.structure_size, ERR_OPERATION_CORRUPT)
         - sizeof(OperationHeader) ;

      ensure_size_sanity(header.data_size, MAX_OPSIZE, "Operation data", ERR_OPERATION_CORRUPT) ;

      const unsigned aligned_datasize = aligned_size(header.data_size) ;

      OperationTail tail ;

      // Allocate a buffer: if it is not too big, allocate it on the stack, otherwise in
      // the heap
      if (aligned_datasize <= MAX_ALLOCA)
         data_buf = P_ALLOCA(char, aligned_datasize) ;
      else
         data_guard.reset(data_buf = new char [aligned_datasize]) ;

      const iovec_t iov_data[] =
         {
            make_iovec(header._extra, remheader_size),
            make_iovec(data_buf, aligned_datasize),
            make_iovec(&tail, sizeof tail)
         } ;
      const iovec_t *iov_begin = iov_data + !iov_data->iov_len ;
      const iovec_t *iov_end = iov_data + P_ARRAY_COUNT(iov_data) ;

      const size_t sz_body = bufsizev(iov_begin, iov_end) ;

      if (segment.readv(iov_begin, iov_end) != sz_body)
      {
         LOGWARN("The tail of " << segment << " is truncated, the segment was not properly closed") ;
         return 0 ;
      }

      // Validate operation tail, ignore the last 4 bytes that hold crc32
      opcrc = calc_crc32(calc_crc32v(opcrc, iov_begin, iov_end - 1), 4, *(iov_end - 1)) ;
      dtoh(tail) ;

      if (tail.data_size != header.data_size || tail.crc32 != opcrc)
      {
         LOGWARN("Operation CRC32 or data size mismatch, the tail of " << segment << " is corrupt") ;
         return 0 ;
      }

      sz_full += sz_body ;
   }
   catch (const format_error &x)
   {
      LOGWARN("Corrupt operation record encountered in " << segment << ". " << x.what()) ;
      return 0 ;
   }
   catch (const std::bad_alloc &) { throw ; }
   catch (const system_error &x)
   {
      LOGWARN("Error reading operation record from " << segment << ". " << STDEXCEPTOUT(x)) ;
      return 0 ;
   }

   // Handle the operation data
   if (!handler(header.opcode, header.opversion,
                header.data_size ? data_buf : NULL, header.data_size))
      return 0 ;

   return sz_full ;
}

bool MMapStorage::do_replay_record(const record_handler &handler)
{
   size_t recsize = 0 ;
   while (!_segments.empty() && (recsize = read_record(*_segments.back(), handler)) == 0)
      _segments.pop_back() ;

   if (!recsize)
      return false ;

   _last_id = _segments.back()->next_segment() ;
   _lastgen += recsize ;

   return true ;
}

void MMapStorage::do_replay_checkpoint(const checkpoint_handler &handler)
{
   NOXCHECK((one_of<SST_READONLY, SST_READABLE>::is(state()))) ;

   PCOMN_ENSURE_POSIX(lseek(_checkpoint->fd(), _checkpoint->data_begin(), SEEK_SET), "lseek") ;

   const size_t datasz = _checkpoint->datasize() ;
   binary_ifdstream checkpoint (_checkpoint->fd(), false) ;
   binary_ibufstream checkpoint_data (checkpoint, std::min(_cpstream_bufsz, datasz)) ;

   checkpoint_data.set_bound(datasz) ;

   handler(checkpoint_data, datasz) ;
}

/*******************************************************************************
 MMapStorage: Storage write methods
*******************************************************************************/
void MMapStorage::close_segments()
{
   TRACEPX(PCOMN_Journmmap, DBGL_MIDLEV, "Closing " << _segments.size() << " readable segments " << *this) ;
   _segments.clear() ;
}

void MMapStorage::do_make_writable()
{
   TRACEPX(PCOMN_Journmmap, DBGL_ALWAYS, "Make writable " << *this) ;

   switch (state())
   {
      case SST_CREATED:
         // After the journal has been created it already has an empty checkpoint file
         // open for writing; besides, it needn't a segment file yet because the next
         // call will be inevitably do_create_checkpoint, which should take care of a new
         // segment
         PCOMN_VERIFY(_segments.empty()) ;
         PCOMN_VERIFY(_lastgen == 0) ;
         PCOMN_VERIFY(_checkpoint) ;
         PCOMN_VERIFY(!_checkpoint->filesize()) ;
         break ;

      case SST_READABLE:
         NOXCHECK(_checkpoint) ;
         NOXCHECK(_checkpoint->filesize()) ;

         set_state(SST_CLOSED) ;
         TRACEPX(PCOMN_Journmmap, DBGL_HIGHLEV, "Creata a new writable segment #" << _last_id) ;
         new_segment_file(_last_id) ;
         _last_id = _checkpoint->next_segment() ;

         // Close all readable segments
         close_segments() ;
         _checkpoint.reset() ;
         break ;

      default:
         PCOMN_VERIFY(state() == SST_CREATED || state() == SST_READABLE) ;
   }
}

size_t MMapStorage::do_append_record(const iovec_t *begin, const iovec_t *end)
{
   PCOMN_VERIFY(_segment) ;

   const size_t written = _segment->writev(begin, end) ;
   _lastgen += written ;

   return written ;
}

std::pair<binary_obufstream *, generation_t> MMapStorage::do_create_checkpoint()
{
   TRACEPX(PCOMN_Journmmap, DBGL_ALWAYS, "Create checkpoint for " << *this) ;

   // This method is called from under the writer lock, it is "single-threaded"
   NOXCHECK(state() == SST_WRITABLE) ;
   NOXCHECK(!_cpstream) ;

   NOXCHECK(is_first_checkpoint() == !!_checkpoint) ;

   // TODO: Take care of bad_alloc!
   if (!is_first_checkpoint())
   {
      // Not the first checkpoint (when the first checkpoint is being made, _checkpoint
      // is not NULL, it contains a pointer to the new writable checkpoint file)
      NOXCHECK(_segment) ;
      NOXCHECK(!_checkpoint) ;

      // Swap segments: create a new segment, commit the current one, set the new segment
      // as the active segment of this storage
      new_segment_file(_segment->next_segment()) ;

      // Advance _seg_id: if this is an initial checkpoint, _seg_id will become 0, which
      // is right
      new_checkpoint_file(_segment->this_segment()) ;
   }

   _checkpoint->init(user_magic()) ;
   _cpstream.reset(new binary_obufstream(new binary_orecstream(*_checkpoint), _cpstream_bufsz)) ;

   return std::make_pair(_cpstream.get(), _checkpoint->generation()) ;
}

void MMapStorage::cleanup_uncommitted_checkpoint()
{
   if (is_first_checkpoint())
      cleanup_item(dirfd(), str::cstr(checkpoint_name()),
                   str::cstr(dirname()), "the journal") ;
   else
      cleanup_item(dirfd(), str::cstr(checkpoint_name() + EXT_TAKING),
                   str::cstr(dirname()), "new checkpoint file") ;

   sync_cpdir() ;
}

void MMapStorage::do_close_checkpoint(bool commit)
{
   // This method is called from under the writer lock, it is "single-threaded"
   // After do_close_checkpoint() _checkpoint is always NULL
   if (!commit)
   {
      // Rollback
      cleanup_uncommitted_checkpoint() ;
      _cpstream.reset() ;
      _checkpoint.reset() ;
      return ;
   }

   NOXCHECK(_cpstream && _checkpoint) ;

   // Commit
   const std::string &cp_path = checkpoint_abspath() ;
   const std::string &newcp_path = cp_path + EXT_TAKING ;

   try {
      PTSafePtr<CheckpointFile> cp (_checkpoint.release()) ;
      PTSafePtr<binary_obufstream> stream (_cpstream.release()) ;

      stream->flush() ;
      stream.reset() ;

      cp->commit() ;

      if (is_first_checkpoint())
      {
         // Create the first segment
         LOGDBG("The first checkpoint of '" << name()
                << "' has been committed, creating the first segment") ;

         new_segment_file(0) ;
      }
      else
      {
         // Atomically replace the last valid checkpoint file with just committed one
         LOGDBG("Replace '" << cp_path << "' with '" << newcp_path << "'") ;

         if (::rename(newcp_path.c_str(), cp_path.c_str()))
         {
            const int err = errno ;
            LOGERR("Cannot replace checkpoint file '" << cp_path
                   << "' with '" << newcp_path << "': " << strerror(err)) ;

            errno = err ;
            PCOMN_THROW_SYSERROR("rename") ;
         }
      }
   }
   catch (const std::exception &x)
   {
      // Cannot commit or swap, remove the new checkpoint
      cleanup_uncommitted_checkpoint() ;
      throw ;
   }
   // Ensure checkpoint commit is properly reflected in directory metadata
   sync_cpdir() ;

   // Remove obsolete segments
   cleanup_obsolete_segments(_last_id, _segment->this_segment()) ;
   _last_id = _segment->this_segment() ;
}

void MMapStorage::cleanup_segment(const std::string &filename)
{
   unlinkat(_segdirfd, str::cstr(filename), 0) ;
}

void MMapStorage::cleanup_obsolete_segments(int64_t begin, int64_t end)
{
   TRACEPX(PCOMN_Journmmap, DBGL_HIGHLEV,
           "Removing obsolete segments from " << begin << " to " << end-1 << " of " << *this) ;

   while (begin < end)
      cleanup_segment(segment_abspath(begin++)) ;
}

} // end of namespace pcomn::jrn
} // end of namespace pcomn
