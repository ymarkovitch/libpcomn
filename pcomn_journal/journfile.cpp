/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   journfile.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2017. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   pcomn::jrn::MMapStorage::RecFile,
                  pcomn::jrn::MMapStorage::CheckpointFile,
                  pcomn::jrn::MMapStorage::SegmentFile

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   12 Nov 2008
*******************************************************************************/
#include "journmmap.h"

#include <pcomn_string.h>
#include <pcomn_utils.h>
#include <pcomn_diag.h>
#include <pcomn_hash.h>
#include <pcomn_mmap.h>

#include <sys/uio.h>
#include <errno.h>

namespace pcomn {
namespace jrn {

static const magic_t ZERO_MAGIC = {{}} ;

/*******************************************************************************
 MMapStorage::RecFile
*******************************************************************************/
MMapStorage::RecFile::RecFile(int dirfd, const char *filename,
                              int64_t segid, generation_t generation,
                              unsigned mask, bool is_checkpoint) :

   _fd(PCOMN_ENSURE_POSIX(::openat(dirfd, PCOMN_ENSURE_ARG(filename), O_CREAT|O_EXCL|O_WRONLY, mask),
                          "creat")),

   _is_checkpoint(is_checkpoint),
   _crc32_mode(false),
   _state(ST_CREATED),
   _corruption(FMTERR_OK),

   _crc32(0),
   _opcount(0),
   _generation(generation),
   _uid(),
   _seg_id(segid),

   _user_magic(ZERO_MAGIC),

   _data_begin(0)
{
   PCOMN_ASSERT_ARG(segid >= -1) ;
   PCOMN_ASSERT_ARG(generation >= 0 && is_aligned(generation)) ;

   TRACEPX(PCOMN_Journmmap, DBGL_MIDLEV, "Created " << *this << " as '" << filename << "'") ;
}

MMapStorage::RecFile::RecFile(int fd, bool is_checkpoint) :
   _fd(fd),

   _is_checkpoint(is_checkpoint),
   _crc32_mode(false),
   _state(ST_READABLE),
   _corruption(FMTERR_OK),

   _crc32(0),
   _opcount(0),
   _generation(NOGEN),
   _uid(),
   _seg_id(-1),

   _user_magic(ZERO_MAGIC),

   _data_begin(0)
{
   PCOMN_ENSURE_POSIX(::lseek(fd, 0, SEEK_SET), "lseek") ;

   header_buffer<FileHeader> header ;

   const bool MOVE_POSITION = true ;
   const FileKind kind =
      file_kind(fd, &_user_magic, &header, MOVE_POSITION) ;

   conditional_throw<storage_error>
      (kind == KIND_UNKNOWN,
       "Not a journal file or the file header is corrupt", ERR_NOT_A_JOURNAL) ;

   static const char * const errmsg[] =
      { "Attempt to read a segment file as a checkpoint",
        "Attempt to read a checkpoint file as a segment" } ;

   static const JournalError errcode[] = { ERR_NOT_A_CHECKPOINT, ERR_NOT_A_SEGMENT } ;

   ensure<storage_error>((kind == KIND_CHECKPOINT) == is_checkpoint,
                          errmsg[is_checkpoint], errcode[is_checkpoint]) ;

   // Fill in data members from the header
   _generation = header.generation ;
   _uid = header.uid ;
   _seg_id = header.nextseg_id - 1 ;
   // Data is prepended with a file magic, user magic, and file header
   _data_begin = 2*sizeof(magic_t) + header.structure_size ;

   TRACEPX(PCOMN_Journmmap, DBGL_MIDLEV, "Opened for reading " << *this) ;
}

MMapStorage::FileKind MMapStorage::RecFile::file_kind(int fd,
                                                      magic_t *user_magic_ret,
                                                      header_buffer<FileHeader> *header_ret,
                                                      bool move_filepos)
{
   struct {
         magic_t magics[2] ;
         header_buffer<FileHeader> header ;
   } header_data ;

   const size_t init_size = sizeof header_data.magics + sizeof(FileHeader) ;

   const size_t readcount = move_filepos
      ? PCOMN_ENSURE_POSIX(::read(fd, &header_data, init_size), "read")
      : PCOMN_ENSURE_POSIX(::pread(fd, &header_data, init_size, 0), "pread") ;

   if (readcount != init_size)
      return KIND_UNKNOWN ;

   FileKind kind ;

   if (header_data.magics[0] == STORAGE_SEGMENT_MAGIC)
      kind = KIND_SEGMENT ;
   else if (header_data.magics[0] == STORAGE_CHECKPOINT_MAGIC)
      kind = KIND_CHECKPOINT ;
   else
      return KIND_UNKNOWN ;

   dtoh(header_data.header) ;

   size_t actual_headsize ;
   try {
      actual_headsize = ensure_header_size<FileHeader>
         (header_data.header.structure_size,
          kind == KIND_CHECKPOINT ? ERR_CHECKPOINT_CORRUPT : ERR_SEGMENT_CORRUPT) ;

      const size_t remsize = actual_headsize - sizeof(FileHeader) ;

      // Read remaining data, if such present
      if (remsize &&
          remsize != (size_t)PCOMN_ENSURE_POSIX(move_filepos
                                                ? ::read(fd, header_data.header._extra, remsize)
                                                : ::pread(fd, header_data.header._extra, remsize, init_size),
                                                move_filepos ? "read" : "pread"))
      {
         LOGPXERR(PCOMN_Journmmap, "Truncated file header in a journal file of " << enum_name(kind) << ". ") ;

         return KIND_UNKNOWN ;
      }
   }
   catch (const format_error &x)
   {
      LOGPXERR(PCOMN_Journmmap, "Corrupt file header in a journal file of " << enum_name(kind) << ". "
                << x.what()) ;

      return KIND_UNKNOWN ;
   }
   catch (const system_error &x)
   {
      LOGPXERR(PCOMN_Journmmap, "Error reading header of a journal file of " << enum_name(kind) << ". "
                << STDEXCEPTOUT(x)) ;

      return KIND_UNKNOWN ;
   }

   if (user_magic_ret)
      *user_magic_ret = header_data.magics[1] ;

   if (header_ret)
      memcpy(header_ret, &header_data.header, actual_headsize) ;

   return kind ;
}

void MMapStorage::RecFile::init(const magic_t &usermagic)
{
   FileHeader header ;
   init_header(header) ;

   header.format_version = FORMAT_VERSION ;
   header.generation = generation() ;
   header.nextseg_id = next_segment() ;
   header.uid = _uid ;

   htod(header) ;

   init(usermagic, &header, sizeof header) ;
}

void MMapStorage::RecFile::init(const magic_t &umagic, const void *init_record, unsigned size)
{
   TRACEPX(PCOMN_Journmmap, DBGL_MIDLEV, *this << "::init(" << init_record << ", " << size << ')') ;

   PCOMN_ENSURE_ARG(init_record) ;
   PCOMN_ENSURE_ARG(size) ;

   PCOMN_VERIFY(state() == ST_CREATED) ;

   NOXCHECK(_generation != NOGEN) ;
   NOXCHECK(_data_begin == 0) ;

   const iovec head[3] =
   {
      make_iovec(&storage_magic(), sizeof storage_magic()),
      make_iovec(&umagic, sizeof umagic),
      make_iovec(init_record, size)
   } ;

   _state = ST_TRANSIT ;

   _data_begin = writev(head) ;

   _user_magic = umagic ;

   set_opcount(0) ;

   _state = ST_WRITABLE ;

   TRACEPX(PCOMN_Journmmap, DBGL_MIDLEV, *this << " is ready for writing") ;
}

bool MMapStorage::RecFile::commit(const void *commit_record, unsigned size)
{
   TRACEPX(PCOMN_Journmmap, DBGL_MIDLEV, *this << "::commit(" << commit_record << ", " << size << ')') ;

   if (size)
   {
      PCOMN_ENSURE_ARG(commit_record) ;
      PCOMN_VERIFY(state() == ST_WRITABLE) ;
      PCOMN_VERIFY(is_aligned(size)) ;
   }
   else if (!commit_record)
   {
      if (state() == ST_CLOSED)
         return false ;

      TRACEPX(PCOMN_Journmmap, DBGL_MIDLEV, "Closing " << *this) ;
      if (state() == ST_WRITABLE)
         sys::hardflush(fd()) ;
      _fd.reset() ;
      _state = ST_CLOSED ;
      return true ;
   }

   const magic_t &magic = make_tail_magic(storage_magic()) ;

   const iovec tail[2] =
   {
      make_iovec(&magic, sizeof magic),
      make_iovec(commit_record, size)
   } ;

   _state = ST_TRANSIT ;

   // Writing the tail record (STORAGE_XXX_MAGIC modified to indicate the tail + tail
   // structure)
   writev(tail) ;

   // Flushing file data to disk
   sys::hardflush(fd()) ;

   _fd.reset() ;
   _state = ST_CLOSED ;

   TRACEPX(PCOMN_Journmmap, DBGL_MIDLEV, "Committed and closed " << *this) ;

   return true ;
}

size_t MMapStorage::RecFile::write_buffer(const void *buf, size_t sz)
{
   TRACEPX(PCOMN_Journmmap, DBGL_LOWLEV, *this << "::write_buffer(buf=" << buf << ", sz=" << sz << ')') ;

   PCOMN_ENSURE_ARG(buf) ;
   const size_t written = PCOMN_ENSURE_POSIX(::write(fd(), buf, sz), "write") ;
   probably_calc_crc32(buf, written) ;
   return written ;
}

size_t MMapStorage::RecFile::write_vector(const iovec_t *begin, const iovec_t *end)
{
   TRACEPX(PCOMN_Journmmap, DBGL_LOWLEV, *this << "::write_vector(begin=" << begin << ", end=" << end << ')') ;

   if (unlikely(begin == end))
      return 0 ;

   PCOMN_ASSERT_ARG(end > begin) ;

   const size_t written = PCOMN_ENSURE_POSIX(::writev(fd(), begin, end - begin), "writev") ;
   probably_calc_crc32(begin, end, written) ;

   TRACEPX(PCOMN_Journmmap, DBGL_LOWLEV, *this << " has vector-written " << written << " bytes") ;

   return written ;
}

size_t MMapStorage::RecFile::readv(const iovec_t *begin, const iovec_t *end)
{
   if (unlikely(begin == end))
      return 0 ;

   PCOMN_ASSERT_ARG(end > begin) ;
   return PCOMN_ENSURE_POSIX(::readv(fd(), begin, end - begin), "writev") ;
}

std::ostream &operator<<(std::ostream &os, const MMapStorage::RecFile &file)
{
   os << (file._is_checkpoint ? "<Checkpoint" : "<Segment")
      << " fd:" << file._fd.handle() << " st:" << oenum(file.state())
      << " gen:" << file.generation() << " nxseg:" << file.next_segment()
      << " uid:" << HEXOUT(file._uid) << " opcnt:" << file.opcount()
      << (file.crc32_mode() ? " crc32(on):" : " crc32(off):") << HEXOUT(file.crc32()) ;

   if (file.state() == MMapStorage::RecFile::ST_READABLE)
      os << " magic:" << file.user_magic()
         << " data:" << std::make_pair(file.data_begin(), file.data_end()) ;

   return os << '>' ;
}

/*******************************************************************************
 MMapStorage::CheckpointFile
*******************************************************************************/
MMapStorage::CheckpointFile::CheckpointFile(int fd) :
   ancestor(fd, true),
   _data_end(data_begin())
{
   TRACEPX(PCOMN_Journmmap, DBGL_MIDLEV, "Checking data consistency of " << *this) ;

   const fileoff_t fsz = filesize() ;
   const fileoff_t enddata = fsz - sizeof(CheckpointTail) - sizeof(magic_t) ;

   // Ensure file size sanity.
   // It should be aligned to 8 and allow for tail record
   ensure_size_alignment(fsz, "Checkpoint file", ERR_CHECKPOINT_CORRUPT) ;

   conditional_throw<format_error>
      (data_begin() > enddata,
       "Invalid checkpoint tail record or a checkpoint is not properly closed",
       ERR_CHECKPOINT_CORRUPT, FMTERR_BAD_HEADER) ;

   TRACEPX(PCOMN_Journmmap, DBGL_LOWLEV, "Calculating CRC32 for " << *this) ;

   // Ensure the checkpoint CRC32 matches the tail record value
   const PMemMapping cpmem (fd) ;
   const uint32_t ccrc = calc_crc32(0, cpmem.data(), fsz - 4) ;
   uint32_t wcrc = *(const uint32_t *)(cpmem.cdata() + (fsz - 4)) ;
   dtoh(wcrc) ;

   WARNPX(PCOMN_Journmmap, ccrc != wcrc, DBGL_ALWAYS, "CRC32 mismatch, file="
          << HEXOUT(wcrc) << ", actual=" << HEXOUT(ccrc) << " for " << *this) ;

   ensure<data_error>(ccrc == wcrc,
                      "Checkpoint CRC32 mismatch", ERR_CHECKPOINT_CORRUPT) ;
   set_crc32(ccrc) ;

   ensure<format_error>
      (*(const magic_t *)(cpmem.cdata() + enddata) == make_tail_magic(storage_magic()),
       "Checkpoint tail magic mismatch", ERR_CHECKPOINT_CORRUPT, FMTERR_MAGIC_MISMATCH) ;

   CheckpointTail tail = *(const CheckpointTail *)(cpmem.cdata() + enddata + sizeof(magic_t)) ;
   dtoh(tail) ;

   TRACEPX(PCOMN_Journmmap, DBGL_LOWLEV,
           "Checking tail: " << tail << ", filesize: " << fsz
           << " data begin:" << data_begin() << " end:" << enddata) ;

   // Ensure tail record sanity
   ensure<format_error>(tail.generation == generation(),
                        "Checkpoint generation mismatch",
                        ERR_CHECKPOINT_CORRUPT, FMTERR_GEN_MISMATCH) ;

   ensure<format_error>(tail.format_version == FORMAT_VERSION,
                        "Invalid journal format version in checkpoint tail",
                        ERR_CHECKPOINT_CORRUPT, FMTERR_VERSION_MISMATCH) ;

   ensure<format_error>(!tail.flags,
                        "Nonzero flags in checkpoint tail",
                        ERR_CHECKPOINT_CORRUPT, FMTERR_BAD_HEADER) ;

   ensure<format_error>(aligned_size(tail.data_size) == (size_t)(enddata - data_begin()),
                        "Data size in checkpoint tail doesn't match the actual data size",
                        ERR_CHECKPOINT_CORRUPT, FMTERR_SIZE_MISMATCH) ;

   _data_end = data_begin() + tail.data_size ;
}

bool MMapStorage::CheckpointFile::commit()
{
   if (state() == ST_CLOSED)
      return false ;

   CheckpointTail tail ;

   PCOMN_VERIFY(state() == ST_WRITABLE) ;
   NOXCHECK(data_begin() > 0) ;

   // Put down the end of data ("payload") part of the file
   _data_end = PCOMN_ENSURE_POSIX(lseek(fd(), 0, SEEK_END), "lseek") ;

   NOXCHECK(_data_end >= data_begin()) ;

   // Prepare tail structure
   init_tail(tail) ;

   tail.generation = generation() ;
   tail.data_size = datasize() ;
   tail.format_version = FORMAT_VERSION ;

   htod(tail) ;

   if (!is_aligned(_data_end))
   {
      // Pad file data with zeros to 8-bytes aligned size
      static const uint64_t zbytes = 0 ;
      const fileoff_t endrange = _data_end ;
      write_buffer(&zbytes, aligned_size(endrange) - endrange) ;
   }
   if (crc32_mode())
   {
      const magic_t &magic = make_tail_magic(storage_magic()) ;
      // Calculate the CRC32 _after_ the tail structure is converted to disk endianness: we
      // calculate CRC32 of the disk contents
      const uint32_t final_crc = calc_crc32(calc_crc32(crc32(), &magic, sizeof magic), &
                                            tail, offsetof(CheckpointTail, cpcrc32)) ;
      tail.cpcrc32 = final_crc ;
      set_crc32(final_crc) ;
   }
   else
      tail.cpcrc32 = crc32() ;

   htod(tail.cpcrc32) ;

   set_crc32_mode(false) ;

   return ancestor::commit(&tail, sizeof tail) ;
}

/*******************************************************************************
 MMapStorage::SegmentFile
*******************************************************************************/
bool MMapStorage::SegmentFile::commit()
{
   return ancestor::commit(NULL) ;
}

} // end of namespace pcomn::jrn
} // end of namespace pcomn
