/*-*- mode: c++; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_JOURNMMAP_H
#define __PCOMN_JOURNMMAP_H
/*******************************************************************************
 FILE         :   journmmap.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Journalling engine storage implemented on memory-mappable filesystem.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   12 Nov 2008
*******************************************************************************/
/** @file
 Journalling engine storage implementation on memory-mappable filesystem.
*******************************************************************************/
#include "journal.h"
#include "journstorage.h"

#include <pcomn_string.h>
#include <pcomn_strslice.h>
#include <pcomn_integer.h>
#include <pcomn_path.h>
#include <pcomn_handle.h>
#include <pcomn_except.h>
#include <pcomn_sys.h>
#include <pcomn_hash.h>
#include <pcomn_flgout.h>
#include <pcomn_atomic.h>
#include <pcomn_ivector.h>
#include <pcomn_unistd.h>

#include <vector>

#include <stddef.h>

namespace pcomn {
namespace jrn {

/*******************************************************************************
 Some filename extensions
*******************************************************************************/
/// Checkpoint filename extension.
/// @note See also MMapStorage::EXT_CHECKPOINT
#define PJRNMMAP_EXT_CHKPOINT "pchkp"
/// Segment filename extension.
/// @note See also MMapStorage::EXT_SEGMENT
#define PJRNMMAP_EXT_SEGMENT  "pseg"

/*******************************************************************************
 Size limits
*******************************************************************************/
/// Size limit for journal file name extension (includes '.'), i.e. the length limit for
/// EXT_SEGMENT, EXT_SEGDIR, EXT_CHECKPOINT
const size_t MAX_JEXT = 10 ;

/// Size limit for generation part of a segment or checkpoint file name
const size_t MAX_JGEN = 20 ; // 1 for '.' + 19 for 64-bit integer

/// name.generation.ext
const size_t MAX_JFILE = MAX_JNAME + MAX_JGEN + MAX_JEXT ;

/******************************************************************************/
/** Memory-mappable storage error.
*******************************************************************************/
class _PCOMNEXP mmap_storage_error : public storage_error {
      typedef storage_error ancestor ;
   public:
      explicit mmap_storage_error(const std::string &s, JournalError errcode = ERR_CORRUPT) :
         ancestor(s, errcode)
      {}
} ;

/******************************************************************************/
/** Journal storage over memory-mappable files.

 The storage implemented as a set of files in a directory of a conventional file
 system. The file system must support memory-mapping and should support it efficiently,
 so e.g. while it is possible to create a journal in a directory on NFS file system, such
 journal will work at best slow and at worst incorrect.

 The storage is specified with a path where the directory part must specify an existent
 directory in the filesystem and the filename part should specify the journal name. Since
 the storage is implemented as a set of files, such name is actually a prefix every file
 in the set starts with. Journal names are more restrictive than regular file names, they:
 @li MUST NOT contain whitespaces
 @li MUST contain @em only printable ASCII characters except for '&', '~', '\\', '/',
 '`', '*', '?', '^'

 Examples of journal paths:

 Journal "bar" in directory "/home/foo":
 /home/foo/bar

 Journal "foobar" in the current working directory:
 foobar

 Journal "foobar.today" in the parent directory:
 ../foobar.today


 A storage implementation consists of two file types: @em checkpoint files and @em segment
 files.

 The name of a checkpoint file looks like follows:
 \<journal name>.pchkp (for the last good checkpoint)
 \<journal name>.\<id>.pchkp (for the checkpoint currently being taken)

 The name of a segment file looks like follows:
 \<journal name>.\<id>.pseg

 The name of a symlink to a checkpoint directory looks like follows:
 \<journal name>.segments

 At every moment in time there exist at least 1 and at most 2 checkpoint files for a
 storage: one for a consistent checkpoint and/or one for a checkpoint currently being
 written. For a normally (w/o errors) closed storage there exists only one (consistent)
 checkpoint. There can be arbitrary number of journal segments.
*******************************************************************************/
class _PCOMNEXP MMapStorage : public Storage {
      typedef Storage ancestor ;
   public:
      /// Kinds of filenames
      enum FilenameKind {
         NK_UNKNOWN,
         NK_CHECKPOINT,
         NK_SEGDIR,
         NK_SEGMENT
      } ;

      /// Kinds of files
      enum FileKind {
         KIND_UNKNOWN,
         KIND_SEGMENT,
         KIND_CHECKPOINT
      } ;

      /// Open mode flags
      enum OpenFlags {
         OF_NOBAKSEG = 0x1000,   /**< When in MD_WRONLY/MD_RDWR mode, don't create backup
                                    files while creating new segments */
         OF_NOSEGDIR = 0x2000    /**< Don't attempt to search a segments directory while
                                    opening in MD_RDONLY/MD_RDWR, use checkpoint directory */
      } ;

      /// File suffix of segment files (includes initial '.')
      static const char EXT_SEGMENT[] ;

      /// File suffix of a symlink to a segment directory  (includes initial '.')
      static const char EXT_SEGDIR[] ;

      /// File suffix of a checkpoint file (includes initial '.')
      static const char EXT_CHECKPOINT[] ;

      /// Second file suffix of a checkpoint currently being taken; such checkpoint file
      /// has two suffices (e.g. "foo.pchkp.taking")
      static const char EXT_TAKING[] ;

      /************************************************************************/
      /** Checkpoint or segment file information.
      *************************************************************************/
      struct FileStat {
            FileKind       kind ;         /**< File kind (segment/checkpoint/unknown) */
            FormatError    corruption ;   /**< Is file OK or, if not, what exactly */
            unsigned       opcount ;      /**< Operation count, if kind is KIND_SEGMENT */
            generation_t   generation ;   /**< Checkpoint generation or segment start */
            uint64_t       datalength ;   /**< Data length */
            magic_t        user_magic ;   /**< User magic number */
      } ;

      static FilenameKind filekind_to_namekind(FileKind kind)
      {
         return
            kind == KIND_SEGMENT ? NK_SEGMENT :
            (kind == KIND_CHECKPOINT ? NK_CHECKPOINT : NK_UNKNOWN) ;
      }

      /// Open existing journal storage
      /// @param journal_path Path to the journal
      /// @param access_mode     Access mode (read-only/write-only/read-write)
      /// @param open_flags      Open flags, OR-comination of jrn::OpenFlags and  (read-only/write-only/read-write)
      /// @param cpstream_bufsz
      explicit MMapStorage(const strslice &journal_path, AccMode access_mode,
                           unsigned open_flags = 0, size_t cpstream_bufsz = 64*KiB) ;

      /// Create a new journal.
      /// @param journal_path    Path to the journal.
      /// @param segdir_path     Location (a directory) for journal segments: may be
      /// empty or NULL, both "" and NULL means to place segments into the journal
      /// (==checkpoint) directory.
      /// @param open_flags      Open flags, OR-comination of jrn::OpenFlags and (read-only/write-only/read-write)
      /// @param cpstream_bufsz
      ///
      /// This constructor specified two paths: journal path (@a journal_path) and the
      /// path to the journal segments directory. A journal storage consists of two kinds
      /// of files: periodically taken @em snaphots, which keep "consolidated" records,
      /// and journal segments, which store sequence of operations since the newest
      /// checkpoint.
      ///
      /// It is better to place the journal directory (AKA checkpoint directory,
      /// @a journal_path) and the segments directory onto different physical disks.
      ///
      /// @note This constructor attempts to create a journal in "exclusive" mode,
      /// i.e. if there already exists a journal @a journal_path, the constructor will
      /// fail (w/exception).
      MMapStorage(const strslice &journal_path, const strslice &segdir_path,
                  unsigned open_flags = 0, size_t cpstream_bufsz = 64*KiB) ;

      ~MMapStorage() ;

      /// Get the journal name
      const std::string &name() const { return _name ; }

      const std::string &dirname() const { return _dirname ; }

      /// Get the file descriptor of the checkpoint directory
      int dirfd() const { return _cpdirfd ; }

      /// Don't create/use/check segments sudirectory, always use the journal
      /// (checkpoint) directory for placing/reading segments
      bool nosegdir() const { return _nosegdir ; }

      /// Don't backup existing segment files, always overwrite
      bool nobakseg() const { return _nobakseg ; }

      std::string segment_dirname() const
      {
         return nosegdir() ? dirname() : journal_abspath(make_filename(name(), EXT_SEGDIR)) ;
      }

      std::string segment_name(uint64_t id) const
      {
         return make_filename(name(), EXT_SEGMENT, &id) ;
      }

      std::string segment_abspath(const strslice &filename) const
      {
         return segment_dirname()
            .append(1, PCOMN_PATH_NATIVE_DELIM)
            .append(filename.begin(), filename.end()) ;
      }

      template<typename I>
      typename if_integer<I, std::string>::type
      segment_abspath(I id) const { return segment_abspath(segment_name(id)) ; }

      std::string checkpoint_name() const
      {
         return make_filename(name(), EXT_CHECKPOINT) ;
      }

      std::string journal_abspath(const strslice &filename) const
      {
         std::string result (dirname()) ;
         return result
            .append(1, PCOMN_PATH_NATIVE_DELIM)
            .append(filename.begin(), filename.end()) ;
      }

      std::string checkpoint_abspath() const { return journal_abspath(checkpoint_name()) ; }

      /// Indicate whether the argument is a valid journal name
      /// @param journal_name Journal name without a path
      static bool is_valid_name(const strslice &journal_name) ;

      /// Extract the journal name from a range specifying the path.
      /// @return If the range ends with a valid journal name, returns a slice with the
      /// name; overwise returns an empty slice.
      static strslice journal_name_from_path(const strslice &path) ;

      /// Extract a directory from a path; the path should end with a valid journal
      /// name.
      /// @throw std::invalid_argument @a path is not a valid journal path
      static std::string journal_dir_from_path(const strslice &path)
      {
         return journal_dir_nocheck(path.begin(), ensure_name_form_path(path).begin()) ;
      }

      /// Determine whethe a filename represents a journal part and, if so, split it to
      /// components.
      /// @param[in]  filename      The name of a file (without a directory part)
      /// @param[out] journal_name  The buffer for a journal name extracted from
      /// @a filename.
      /// @param[out] id            The buffer for id value extracted from @a filename.
      template<typename S>
      static typename enable_if_strchar<S, char, FilenameKind>::type
      parse_filename(const S &filename, std::string *journal_name = NULL, uint64_t *id = NULL)
      {
         return parse_internal(str::cstr(filename), journal_name, id) ;
      }

      /// Get the filename of a journal component.
      static std::string build_filename(const strslice &journal_name,
                                        FilenameKind kind, uint64_t id = 0)
      {
         return is_valid_name(journal_name)
            ? make_filename(journal_name, get_extension(kind),
                            kind == NK_SEGDIR || kind == NK_CHECKPOINT ? NULL : &id)
            : std::string() ;
      }

      /// Find out the file kind by reading its magic number and file header.
      /// @note This function does not change file offser of @a fd
      static FileKind file_kind(int fd,
                                magic_t *user_magic_ret = NULL,
                                header_buffer<FileHeader> *header_ret = NULL)
      {
         return RecFile::file_kind(fd, user_magic_ret, header_ret, false) ;
      }

      /// Return information about a journal component (checkpoint or segment).
      ///
      /// If @a fd is a valid file descriptor which is neither a checkpoint nor a segment
      /// file, doesn't throw exception and returns FileStat with 'kind' field set to
      /// KIND_UNKNOWN.
      /// @note Read permission is required on the file.
      static FileStat file_stat(int fd) ;

      /************************************************************************/
      /** Record file: the base class for both CheckpointFile and SegmentFile
      *************************************************************************/
      class RecFile {
            PCOMN_NONCOPYABLE(RecFile) ;
            PCOMN_NONASSIGNABLE(RecFile) ;
         public:
            /// States of CheckpointFile and SegmentFile.
            ///
            /// There are two possible transition sequences:
            /// @li The file is created for writing: ST_CREATED->ST_WRITABLE->ST_CLOSED
            /// @li The file is open for reading: ST_READABLE->ST_CLOSED
            enum FileState {
               ST_TRANSIT = -1,
               ST_CLOSED,     /**< The file is already closed */
               ST_READABLE,   /**< The file is open for reading and checked for sanity */
               ST_CREATED,    /**< The file is just created and has no headers */
               ST_WRITABLE
            } ;

            virtual ~RecFile() {}

            int fd() const { return ensure_open()._fd.handle() ; }

            generation_t generation() const { return _generation ; }

            int64_t next_segment() const { return _seg_id + 1 ; }

            fileoff_t data_begin() const { return _data_begin ; }

            virtual fileoff_t data_end() const { return filesize() ; }

            FileState state() const { return _state ; }

            const magic_t &storage_magic() const
            {
               return _is_checkpoint ? STORAGE_CHECKPOINT_MAGIC : STORAGE_SEGMENT_MAGIC ;
            }

            const magic_t &user_magic() const { return _user_magic ; }

            bool close() { return commit(NULL) ; }

            /// Write the initial record (magic + user magic + file header).
            /// @param usermagic
            void init(const magic_t &usermagic) ;

            template<size_t n>
            size_t readv(const iovec_t (&vec)[n]) { return readv(vec + 0, vec + n) ; }

            size_t readv(const iovec_t *begin, const iovec_t *end) ;

            template<size_t n>
            size_t writev(const iovec_t (&vec)[n]) { return writev(vec + 0, vec + n) ; }

            size_t writev(const iovec_t *begin, const iovec_t *end)
            {
               const size_t written = end - begin == 1 ?
                  write_buffer(begin->iov_base, begin->iov_len) : write_vector(begin, end) ;

               atomic_op::inc(&_opcount) ;
               return written ;
            }

            size_t write(const void *buf, size_t sz)
            {
               const iovec_t &v = make_iovec(buf, sz) ;
               return writev(&v, &v + 1) ;
            }

            /// Indicate whether CRC32 calculation mode is on
            bool crc32_mode() const { return _crc32_mode ; }

            /// Set CRC32 calculation mode
            bool set_crc32_mode(bool mode) { return pcomn::xchange(_crc32_mode, mode) ; }

            /// Get CRC32 calculated so far
            uint32_t crc32() const { return _crc32 ; }

            /// Set new CRC32 value
            uint32_t set_crc32(uint32_t value) { return pcomn::xchange(_crc32, value) ; }

            unsigned opcount() const { return _opcount ; }

            unsigned set_opcount(unsigned count)
            {
               return atomic_op::xchg(&_opcount, (uatomic_t)count) ;
            }

            uint64_t uid() const { return _uid ; }

            fileoff_t filesize() const
            {
               return PCOMN_ENSURE_POSIX(sys::filesize(fd()), "fstat") ;
            }

            size_t datasize() const { return data_end() - data_begin() ; }

            /// Find out the file kind by reading its magic number and file header.
            static FileKind file_kind(int fd,
                                      magic_t *user_magic_ret,
                                      header_buffer<FileHeader> *header_ret,
                                      bool move_filepos) ;

            friend std::ostream &operator<<(std::ostream &os, const RecFile &file) ;

         protected:
            /// Create an empty writable record file
            RecFile(int dirfd, const char *filename,
                    int64_t segid, generation_t generation,
                    unsigned mask, bool is_checkpoint) ;

            /// Create an empty writable record file
            RecFile(int fd, bool is_checkpoint) ;

            RecFile &ensure_open() const
            {
               pcomn::ensure<std::logic_error> (state() != ST_CLOSED, _is_checkpoint
                                                ? "Checkpoint file is already closed"
                                                : "Segment file is already closed") ;
               return *const_cast<RecFile *>(this) ;
            }

            RecFile &ensure_readable() const
            {
               pcomn::ensure<std::logic_error>(state() == ST_READABLE, _is_checkpoint
                                               ? "Checkpoint file is not readable"
                                               : "Segment file is not readable") ;
               return *const_cast<RecFile *>(this) ;
            }

            /// Close the file if it is readable or commit and close if writable.
            ///
            /// commit(NULL) closes the file; commit(&tail, sizeof(tail)) commits @em and
            /// closes the file, if it is writable, or throws exception, if not.
            /// @note This method @em may throw exception - don't make unattended calls
            /// from destructors!
            bool commit(const void *commit_record, unsigned size = 0) ;

            void init(const magic_t &usermagic, const void *init_record, unsigned size) ;

            size_t write_vector(const iovec_t *begin, const iovec_t *end) ;
            size_t write_buffer(const void *buf, size_t sz) ;

         private:
            fd_safehandle  _fd ;

            const bool     _is_checkpoint ;
            bool           _crc32_mode ;
            FileState      _state ;
            FormatError    _corruption ;

            uint32_t       _crc32 ;
            uatomic_t      _opcount ;
            generation_t   _generation ;
            uint64_t       _uid ;
            int64_t        _seg_id ;

            magic_t        _user_magic ;
            fileoff_t          _data_begin ;

            void probably_calc_crc32(const void *buf, size_t sz)
            {
               if (crc32_mode())
                  _crc32 = calc_crc32(_crc32, buf, sz) ;
            }
            void probably_calc_crc32(const iovec_t *begin, const iovec_t *end, size_t sz)
            {
               if (!crc32_mode())
                  return ;

               ssize_t remained = sz ;
               for (const iovec_t *i = begin ; remained > 0 && i != end ; remained -= i++->iov_len)
                  _crc32 = calc_crc32(_crc32, i->iov_base, std::min<ssize_t>(remained, i->iov_len)) ;

               NOXCHECK(remained <= 0) ;
            }
      } ;

      /************************************************************************/
      /** Checkpoint file
      *************************************************************************/
      class CheckpointFile : public RecFile {
            typedef RecFile ancestor ;
         public:
            /// Create an empty writable checkpoint file
            template<typename S>
            CheckpointFile(int dirfd, const S &filename,
                           int64_t segid, generation_t generation,
                           typename enable_if_strchar<S, char, unsigned>::type mask) :

               ancestor(dirfd, str::cstr(filename), segid, generation, mask, true),
               _data_end(data_begin())
            {
               // Calculate crc32 all over the checkpoint
               set_crc32_mode(true) ;
            }

            /// Create a read-only CheckpointFile object from a file descriptor open in
            /// read mode.
            ///
            /// Scans the checkpoint data, calculate CRC32 and checks whether it matches;
            /// if not, throws data_error.
            /// @note Sets file position to the start of actual checkpoint data.
            explicit CheckpointFile(int fd) ;

            fileoff_t data_end() const { return _data_end ; }

            bool commit() ;

         private:
            fileoff_t _data_end ;
      } ;

      /************************************************************************/
      /** Segment file
      *************************************************************************/
      class SegmentFile : public RecFile {
            typedef RecFile ancestor ;
         public:
            /// Create an empty writable checkpoint file
            template<typename S>
            SegmentFile(int dirfd, const S &filename,
                        int64_t segid, generation_t generation,
                        typename enable_if_strchar<S, char, unsigned>::type mask) :

               ancestor(dirfd, str::cstr(filename), segid, generation, mask, false)
            {}

            /// Create a read-only CheckpointFile object from a file descriptor open in
            /// read mode.
            explicit SegmentFile(int fd) :
               ancestor(fd, false)
            {}

            bool commit() ;

            int64_t this_segment() const { return next_segment() - 1 ; }
      } ;

   protected:
      void do_replay_checkpoint(const checkpoint_handler &handler) ;
      bool do_replay_record(const record_handler &handler) ;

      void do_make_writable() ;

      std::pair<binary_obufstream *, generation_t> do_create_checkpoint() ;
      void do_close_checkpoint(bool commit) ;
      size_t do_append_record(const iovec_t *begin, const iovec_t *end) ;
      /// Close the storage.
      ///
      /// If the storage is readable, closes all open segments
      bool do_close_storage() ;

      std::ostream &debug_print(std::ostream &os) const ;

   private:
      const std::string    _name ;     /* Journal name */

      const std::string    _dirname ;  /* Journal directory: checkpoints are placed there. */

      const fd_safehandle  _cpdirfd ;  /* Descriptor of a directory for checkpoints */

      fd_safehandle        _segdirfd ; /* Descriptor of a directory for journal
                                        * segments */
      PTSafePtr<SegmentFile> _segment ; /* File descriptor of the active segment (being
                                         * written to) */
      PTSafePtr<CheckpointFile>  _checkpoint ; /* Active checkpoint descriptor (being
                                                * written to); set only after journal
                                                * creation and during checkpointing */
      PTSafePtr<binary_obufstream> _cpstream ; /* The last checkpoint stream */

      ivector<SegmentFile> _segments ; /* Descriptors of files that represent segments */

      int64_t              _last_id ;

      generation_t         _lastgen ;   /* Generation of the end of the storage */

      const size_t         _cpstream_bufsz ;

      const bool           _nosegdir ; /* Don't create/use/check segments sudirectory */

      const bool           _nobakseg ; /* Don't backup existing segment files,
                                        * overwrite */

   private:
      enum CreateStage {
         CST_INIT,
         CST_SYMLINK,    /* Symlink to the segments directory created */
         CST_CHECKPOINT, /* Zero checkpoint created */
         CST_SEGMENT,    /* Zero segment created */
         CST_COMPLETE    /* The journal created completely */
      } ;

      void create_storage(const char *segments_dir) ;
      void create_storage(const char *segdirname, CreateStage &stage) ;

      void open_storage(bool rdonly) ;

      bool open_segdir(bool raise_on_error) ;

      // Returns true if there is a segments directory and it is opened OK
      bool open_segments() ;

      void cleanup_storage(CreateStage) ;

      void cleanup_part(CreateStage, uint64_t id) ;

      std::string create_segdir_symlink(const char *segdirname) ;

      // Create a new segment file and set it as a current active segment. Upon the call
      // _segment is set to the newly created open in write-only mode empty segment file
      // with a header.
      SegmentFile *new_segment_file(int64_t seg_id) ;

      // Create a new checkpoint file and set it as a current active checkpoint. There
      // should be no active checkpoints while calling this method. Upon the call
      // _checkpoint is set to the newly created open in write-only mode empty
      // checkpoint file.
      CheckpointFile *new_checkpoint_file(int64_t nextseg_id) ;

      // Should be called from under the lock
      generation_t current_generation() const { return _lastgen ; }

      static strslice ensure_name_form_path(const strslice &path) ;

      static const char *ensure_journal_name(const char *name)
      {
         PCOMN_THROW_IF(!is_valid_name(PCOMN_ENSURE_ARG(name)),
                        std::invalid_argument, "Invalid journal name '%s'", name) ;
         return name ;
      }

      static FilenameKind parse_internal(const char *filename,
                                         std::string *journal_name,
                                         uint64_t *id) ;

      static char *make_filename(char *buf,
                                 const strslice &journal_name,
                                 const char *ext,
                                 const uint64_t *id = NULL)
      {
         id ? snprintf(buf, MAX_JFILE + 1, "%.*s.%llu%s",
                       P_STRSLICEV(journal_name), (ulonglong_t)*id, ext)

            : snprintf(buf, MAX_JFILE + 1, "%.*s%s", P_STRSLICEV(journal_name), ext) ;
         return buf ;
      }

      static std::string make_filename(const strslice &journal_name, const char *ext,
                                       const uint64_t *id = NULL)
      {
         char buf[MAX_JFILE + 1] ;
         return std::string(make_filename(buf, journal_name, ext, id)) ;
      }

      static const char *get_extension(FilenameKind kind)
      {
         switch (kind)
         {
            case NK_SEGDIR    : return EXT_SEGDIR ;
            case NK_SEGMENT   : return EXT_SEGMENT ;
            case NK_CHECKPOINT: return EXT_CHECKPOINT ;
            default:
               throw_exception<std::invalid_argument>("Invalid filename kind specified") ;
         }
         return NULL ;
      }

      static std::string journal_dir_nocheck(const char *begin, const char *end)
      {
         return end == begin
            ? std::string(1, '.') // No directory specified, use the cwd
            : std::string(begin, end - (end - begin > 1)) ;
      }

      // Call from the constructor _only_
      std::string journal_dir_abspath(const strslice &journal_path) const
      {
         return path::abspath<std::string>
            (journal_dir_nocheck(journal_path.begin(), journal_path.end() - _name.size())) ;
      }

      int open_dir() const
      {
         TRACEPX(PCOMN_Journmmap, DBGL_MIDLEV,
                 "Opening directory '" << _dirname << "' for journal '" << name()) ;
         return
            PCOMN_ENSURE_POSIX(sys::opendirfd(str::cstr(_dirname)), "open") ;
      }

      void sync_cpdir() { PCOMN_ENSURE_POSIX(sys::hardflush(dirfd()), "fsync") ; }

      void sync_segdir() { PCOMN_ENSURE_POSIX(sys::hardflush(_segdirfd), "fsync") ; }

      void cleanup_segment(const std::string &filename) ;

      void cleanup_uncommitted_checkpoint() ;

      void cleanup_obsolete_segments(int64_t begin, int64_t end) ;

      size_t read_record(SegmentFile &segment, const record_handler &handler) ;

      void close_segments() ;

      // Get last readable segment (may be NULL)
      SegmentFile *last_segment() const
      {
         return _segments.empty() ? NULL : _segments.back() ;
      }

      bool is_first_checkpoint() const { return !_segment ; }
} ;

} // end of namespace pcomn::jrn
} // end of namespace pcomn

/*******************************************************************************
 Debug output
*******************************************************************************/
PCOMN_DEFINE_ENUM_ELEMENTS(pcomn::jrn::MMapStorage::RecFile, FileState,
                           5,
                           ST_TRANSIT,
                           ST_CLOSED,
                           ST_READABLE,
                           ST_CREATED,
                           ST_WRITABLE) ;

PCOMN_DEFINE_ENUM_ELEMENTS(pcomn::jrn::MMapStorage, FilenameKind,
                           4,
                           NK_UNKNOWN,
                           NK_CHECKPOINT,
                           NK_SEGDIR,
                           NK_SEGMENT) ;

PCOMN_DEFINE_ENUM_ELEMENTS(pcomn::jrn::MMapStorage, FileKind,
                           3,
                           KIND_UNKNOWN,
                           KIND_SEGMENT,
                           KIND_CHECKPOINT) ;

#endif /* __PCOMN_JOURNMMAP_H */
