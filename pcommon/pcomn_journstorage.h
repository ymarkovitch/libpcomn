/*-*- mode: c++; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_JOURSTORAGE_H
#define __PCOMN_JOURSTORAGE_H
/*******************************************************************************
 FILE         :   pcomn_journstorage.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Abstract storage for journalling engine.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   30 Oct 2008
*******************************************************************************/
/** @file
 Abstract storage for journalling engine.

 journal-segment   ::= STORAGE_SEGMENT_MAGIC operation-records
 operation-records ::= empty | operation-record operation-records
 operation-record  ::= STORAGE_OPERATION_MAGIC operation-header operation-data operation-tail
 operation-header  ::= OperationHeader
 operation-tail    ::= OperationTail

 journal-checkpoint  ::= STORAGE_CHECKPOINT_MAGIC STORAGE_USER_MAGIC checkpoint-header
                         checkpoint-data checkpoint-tail
 checkpoint-data     ::= empty | byte-sequence
 checkpoint-header   ::= OperationHeader
 operation-tail      ::= OperationTail

*******************************************************************************/
#include <pcomn_journal.h>
#include <pcomn_integer.h>
#include <pcomn_buffer.h>
#include <pcomn_hash.h>
#include <pcomn_string.h>

namespace pcomn {
namespace jrn {

const size_t MAX_IOVEC_COUNT = 511 ; /**< The limit of iovec items in an I/O operation */

/*******************************************************************************
 On-disk representation of journal storage
*******************************************************************************/
extern const magic_t STORAGE_CHECKPOINT_MAGIC ; /**< "#YMcp1\r\n" */
extern const magic_t STORAGE_SEGMENT_MAGIC ;    /**< "#YMsg1\r\n" */
extern const magic_t STORAGE_OPERATION_MAGIC ;  /**< "#YMop1\r\n" */

inline magic_t make_tail_magic(const magic_t &head_magic)
{
   magic_t tail_magic (head_magic) ;
   tail_magic.data[0] = '$' ;
   return tail_magic ;
}

inline size_t bufsizev(const iovec_t *begin, const iovec_t *end)
{
   size_t result = 0 ;
   while(begin != end)
      result += begin++->iov_len ;
   return result ;
}

inline uint32_t calc_crc32v(uint32_t init_crc, const iovec_t *begin, const iovec_t *end)
{
   while(begin != end)
      init_crc = calc_crc32(init_crc, *begin++) ;
   return init_crc ;
}

/******************************************************************************/
/** On-disk structure of the both checkpoint and segment header.

 Follows STORAGE_CHECKPOINT_MAGIC and the user magic number in the checkpoint file.
*******************************************************************************/
struct FileHeader {
      uint32_le structure_size ; /**< Full structure size */

      uint16_le format_version ; /**< Journal format version */

      uint16_le flags ;          /**< Reserved, must be 0 */

      int64_le  generation ;     /**< Journal generation the checkpoint is made for or
                                      the start generation of the segment */
      uint64_le uid ;            /**< Pseudo-uid: all segments/checkpoints of the same
                                    journal should have this uid the same */
      int64_le  nextseg_id ;     /**< Pointer to the next segment: its "id" name part */
} ;

/******************************************************************************/
/** On-disk structure for the checkpoint header: follows STORAGE_CHECKPOINT_MAGIC
 and the user magic number in a checkpoint file.
*******************************************************************************/
typedef FileHeader CheckpointHeader ;

/******************************************************************************/
/** On-disk structure for the segment header: follows STORAGE_SEGMENT_MAGIC
 and the user magic number in a segment file.
*******************************************************************************/
typedef FileHeader SegmentHeader ;

/******************************************************************************/
/** On-disk structure of the checkpoint tail.

 Follows checkpoint data in the checkpoint file.
*******************************************************************************/
struct CheckpointTail {
      int64_le  generation ;     /**< Journal generation the checkpoint is made for */

      uint64_le data_size ;      /**< Checkpoint data size */

      uint16_le flags ;          /**< Reserved, must be 0 */

      uint16_le format_version ; /**< Journal format version */

      uint32_le cpcrc32 ;     /**< Checkpoint CRC32 (includes the checkpoint header) */
} ;

/******************************************************************************/
/** On-disk header of a journallable operation record.

 Note an operation record in a journal segment is prepended with
 STORAGE_OPERATION_MAGIC.
*******************************************************************************/
struct OperationHeader {
      uint32_le structure_size ; /**< Full structure size, must be aligned to 8 bytes */

      int32_le  opcode ;         /**< Operation code */

      uint32_le opversion ;      /**< Operation version */

      uint32_le data_size ;      /**< The size of operation data that follow the header */
} ;

/******************************************************************************/
/** On-disk tail of the journallable operation.

 Follows the operation data and finishes the operation record.
*******************************************************************************/
struct OperationTail {
      uint32_le data_size ;   /**< Same as OperationHeader::data_size (must match) */

      uint32_le crc32 ;       /**< Operation CRC32 (includes the operation header) */
} ;

/*******************************************************************************
 Header "constructors"
*******************************************************************************/
template<typename Header>
inline Header &init_header(Header &header)
{
   fill_mem(header, 0) ;
   header.structure_size = sizeof header ;
   return header ;
}

template<typename Tail>
inline Tail &init_tail(Tail &header)
{
   fill_mem(header, 0) ;
   return header ;
}

/*******************************************************************************
 Alignment
*******************************************************************************/
template<typename I>
inline typename if_integer<I>::type aligned_size(I size)
{
   return (size + 7) & ~(I)7 ;
}

template<typename I>
inline typename if_integer<I, bool>::type is_aligned(I size)
{
   return !(size & 7) ;
}

/*******************************************************************************

*******************************************************************************/
const unsigned MIN_OPSIZE =
   sizeof(STORAGE_OPERATION_MAGIC) +
   sizeof(OperationHeader) +
   sizeof(OperationTail) ;

inline size_t operation_size(const OperationHeader &header)
{
   return
      sizeof STORAGE_OPERATION_MAGIC
      + header.structure_size
      + header.data_size
      + sizeof (OperationTail) ;
}

inline size_t ensure_size_sanity(size_t size, size_t maxsize,
                                 const char *part_name, JournalError errcode)
{
   if (size > maxsize)
      throw_exception<format_error>
         (PCOMN_BUFPRINTF(256, "%s size=%llu is greater than allowed maximum of %llu",
                          part_name, (ulonglong_t)size, (ulonglong_t)maxsize),
          errcode, FMTERR_SIZE_INSANE) ;

   return size ;
}

inline size_t ensure_size_alignment(size_t size, const char *part_name,
                                    JournalError errcode)
{
   if (!is_aligned(size))
      throw_exception<format_error>
         (PCOMN_BUFPRINTF(256, "%s size=%llu is not properly aligned",
                          part_name, (ulonglong_t)size),
          errcode, FMTERR_SIZE_INSANE) ;

   return size ;
}

template<typename H>
unsigned ensure_header_size(unsigned size, JournalError errcode)
{
   // Should be big enough for at least the fixed part of a header
   if (size < sizeof(H))
      throw format_error
         (PCOMN_BUFPRINTF(256, "%s::structure_size=%u is less than sizeof=%u",
                          PCOMN_TYPENAME(H), size, (unsigned)sizeof(H)),
          errcode, FMTERR_SIZE_INSANE) ;

   ensure_size_sanity(size, MAX_HDRSIZE, PCOMN_TYPENAME(H), errcode) ;

   // Should be 8-byte aligned
   ensure_size_alignment(size, PCOMN_TYPENAME(H), errcode) ;

   return size ;
}

template<typename H>
struct header_buffer : H {
      char _extra[MAX_HDRSIZE - sizeof(H)] ;
} ;

/*******************************************************************************
 "htod" == "host-to-disk"
 "dtoh" == "disk-to-host"
*******************************************************************************/
template<typename I>
inline typename if_integer<I>::type &htod(I &value)
{
   return to_little_endian(value) ;
}

template<typename I>
inline typename if_not_integer<I>::type &htod(I &value)
{
   return value ;
}

template<typename I>
inline typename if_integer<I>::type &dtoh(I &value)
{
   return from_little_endian(value) ;
}

template<typename I>
inline typename if_not_integer<I>::type &dtoh(I &value)
{
   return value ;
}

/*******************************************************************************
 Journal headers host<->disk
*******************************************************************************/
#define PCOMN_CHECK_VERSION_SANITY_HTOD(type, var)                      \
{                                                                       \
   pcomn::conditional_throw<std::logic_error>                           \
      ((var).format_version != FORMAT_VERSION, "Invalid"  #type "::format_version") ; \
   pcomn::conditional_throw<std::logic_error>                           \
      ((var).flags, "Nonzero " #type "::flags") ;                      \
}

#define PCOMN_CHECK_SIZE_SANITY_HTOD(type, var)                         \
{                                                                       \
   pcomn::conditional_throw<std::logic_error>                           \
      ((var).structure_size < sizeof(var) || !pcomn::jrn::is_aligned((var).structure_size), \
       "Invalid " #type "::structure_size") ;                           \
}

#define PCOMN_CHECK_GENERATION_SANITY_HTOD(type, var, member)     \
{                                                                 \
   pcomn::conditional_throw<std::logic_error>                     \
      ((generation_t)(var).member == NOGEN, #type "::" #member " is not set") ;  \
   pcomn::conditional_throw<std::logic_error>                     \
      (!is_aligned((generation_t)(var).member), "Misaligned " #type "::" #member) ; \
}

/*******************************************************************************
 host<->disk
  FileHeader
*******************************************************************************/
inline FileHeader &htod(FileHeader &header)
{
   // We should be absolutely paranoid and check everything
   PCOMN_CHECK_SIZE_SANITY_HTOD(FileHeader, header) ;

   PCOMN_CHECK_VERSION_SANITY_HTOD(FileHeader, header) ;

   PCOMN_CHECK_GENERATION_SANITY_HTOD(FileHeader, header, generation) ;

   pcomn::conditional_throw<std::logic_error>
      (header.nextseg_id < 0, "Invlalid nextseg_id") ;

   htod(header.structure_size) ;
   htod(header.format_version) ;
   htod(header.flags) ;
   htod(header.generation) ;
   htod(header.uid) ;
   htod(header.nextseg_id) ;

   return header ;
}

inline FileHeader &dtoh(FileHeader &header)
{
   dtoh(header.structure_size) ;
   dtoh(header.format_version) ;
   dtoh(header.flags) ;
   dtoh(header.generation) ;
   dtoh(header.uid) ;
   dtoh(header.nextseg_id) ;

   return header ;
}

inline FileHeader &check_sanity(FileHeader &header)
{
   pcomn::conditional_throw<format_error>
      (header.format_version != FORMAT_VERSION,
       "Invalid journal format version", ERR_CORRUPT, FMTERR_VERSION_MISMATCH) ;

   pcomn::conditional_throw<format_error>
      (header.flags, "Nonzero flags", ERR_CORRUPT, FMTERR_BAD_HEADER) ;

   // Generation shoul be aligned (since size of any operation _is_ aligned)
   pcomn::conditional_throw<format_error>
      (header.generation < 0 || !is_aligned(header.generation),
       "Invalid file generation in the file header", ERR_CORRUPT, FMTERR_GEN_INSANE) ;

   pcomn::conditional_throw<format_error>
      (header.nextseg_id < 0,
       "Invalid next segment ID in the file header", ERR_CORRUPT, FMTERR_BAD_HEADER) ;

   return header ;
}

/*******************************************************************************
 host<->disk
  CheckpointTail
*******************************************************************************/
inline CheckpointTail &htod(CheckpointTail &header)
{
   PCOMN_CHECK_VERSION_SANITY_HTOD(CheckpointTail, header) ;

   PCOMN_CHECK_GENERATION_SANITY_HTOD(CheckpointTail, header, generation) ;

   htod(header.generation) ;
   htod(header.data_size) ;
   htod(header.format_version) ;
   htod(header.flags) ;
   htod(header.cpcrc32) ;

   return header ;
}

inline CheckpointTail &dtoh(CheckpointTail &header)
{
   dtoh(header.generation) ;
   dtoh(header.format_version) ;
   dtoh(header.flags) ;
   dtoh(header.cpcrc32) ;

   return header ;
}

/*******************************************************************************
 host<->disk
  OperationHeader
  OperationTail
*******************************************************************************/
#define PCOMN_CHECK_OPSIZE_SANITY_HTOD(type, var)     \
{                                                     \
   pcomn::conditional_throw<std::logic_error>         \
      ((var).data_size > MAX_OPSIZE,                  \
       "Invalid " #type "::opdata_size") ;            \
}

inline OperationHeader &htod(OperationHeader &header)
{
   PCOMN_CHECK_SIZE_SANITY_HTOD(OperationHeader, header) ;

   PCOMN_CHECK_OPSIZE_SANITY_HTOD(OperationHeader, header) ;

   htod(header.structure_size) ;
   htod(header.opcode) ;
   htod(header.opversion) ;
   htod(header.data_size) ;

   return header ;
}

inline OperationHeader &dtoh(OperationHeader &header)
{
   dtoh(header.structure_size) ;
   dtoh(header.opcode) ;
   dtoh(header.opversion) ;
   dtoh(header.data_size) ;

   return header ;
}

inline OperationTail &htod(OperationTail &header)
{
   PCOMN_CHECK_OPSIZE_SANITY_HTOD(OperationTail, header) ;

   htod(header.data_size) ;
   htod(header.crc32) ;

   return header ;
}

inline OperationTail &dtoh(OperationTail &header)
{
   dtoh(header.data_size) ;
   dtoh(header.crc32) ;

   return header ;
}

#undef PCOMN_CHECK_OPSIZE_SANITY_HTOD
#undef PCOMN_CHECK_VERSION_SANITY_HTOD
#undef PCOMN_CHECK_SIZE_SANITY_HTOD
#undef PCOMN_CHECK_GENERATION_SANITY_HTOD

/*******************************************************************************
 Debug output
*******************************************************************************/
inline std::ostream &operator<<(std::ostream &os, const FileHeader &h)
{
   return os
      << "<ssize:" << h.structure_size
      << " fmtver:" << h.format_version
      << " flags:" << h.flags
      << " gen:" << h.generation
      << " uid:" << HEXOUT(h.uid)
      << " nextseg:" << h.nextseg_id
      << '>' ;
}

inline std::ostream &operator<<(std::ostream &os, const CheckpointTail &h)
{
   return os
      << "<gen:" << h.generation
      << " dsize:" << h.data_size
      << " flags:" << h.flags
      << " fmtver:" << h.format_version
      << " crc32:" << HEXOUT(h.cpcrc32)
      << '>' ;
}

inline std::ostream &operator<<(std::ostream &os, const OperationHeader &h)
{
   return os
      << "<ssize:" << h.structure_size
      << " opcode:" << h.opcode
      << " opver:" << h.opversion
      << " dsize:" << h.data_size
      << '>' ;
}

inline std::ostream &operator<<(std::ostream &os, const OperationTail &h)
{
   return os
      << "<dsize:" << h.data_size
      << " crc32:" << HEXOUT(h.crc32)
      << '>' ;
}

} // end of namespace pcomn::jrn
} // end of namespace pcomn

#endif /* __PCOMN_JOURSTORAGE_H */
