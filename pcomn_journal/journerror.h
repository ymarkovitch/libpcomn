/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_JOURNERROR_H
#define __PCOMN_JOURNERROR_H
/*******************************************************************************
 FILE         :   journerror.h
 COPYRIGHT    :   Yakov Markovitch, 2010-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Journal exceptions.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   17 Feb 2009
*******************************************************************************/
/** @file
 Journal error codes and exception classes.
*******************************************************************************/
#include <pcomn_strslice.h>
#include <pcommon.h>

#include <stdexcept>
#include <string>

namespace pcomn {
namespace jrn {

/*******************************************************************************
 Journal exceptions
*******************************************************************************/
enum JournalError {
   ERR_ERROR = 1,       /* Generic error */
   ERR_INVALID_NAME,
   ERR_EXISTS,
   ERR_DOESNT_EXIST,
   ERR_NOT_A_JOURNAL,
   ERR_NOT_A_CHECKPOINT, /**< Checkpoint magic number mismatch */
   ERR_NOT_A_SEGMENT,    /**< Segment magic number mismatch */
   ERR_NOT_A_FORMAT,     /**< User-level magic number mismatch */
   ERR_CORRUPT,
   ERR_CHECKPOINT_CORRUPT,
   ERR_SEGMENT_CORRUPT,
   ERR_OPERATION_CORRUPT,
   ERR_OPCODE,
   ERR_OPVERSION,
   ERR_STORAGE_MODE,    /* Attempt to read a writable storage or write a readable one */
} ;

enum FormatError {
   FMTERR_OK = 0,

   FMTERR_MAGIC_MISMATCH,
   FMTERR_USER_MAGIC_MISMATCH,
   FMTERR_VERSION_MISMATCH,
   FMTERR_GEN_MISMATCH,
   FMTERR_CRC_MISMATCH,
   FMTERR_SIZE_MISMATCH,
   FMTERR_SIZE_INSANE,
   FMTERR_GEN_INSANE,
   FMTERR_BAD_HEADER
} ;

const FormatError FMTERR_COUNT_INSANE = FMTERR_SIZE_INSANE ;

/******************************************************************************/
/** Error while saving an operation to or reading from the roll-forward operation
    journal.
*******************************************************************************/
class _PCOMNEXP journal_error : public std::runtime_error {
      typedef std::runtime_error ancestor ;
   public:
      explicit journal_error(const std::string &s, JournalError errcode = ERR_ERROR) :
         ancestor(s),
         _errcode(errcode)
      {}

      JournalError code() const { return _errcode ; }

   private:
      const JournalError _errcode ;
} ;

/******************************************************************************/
/** Journal storage error (e.g. I/O error).
*******************************************************************************/
class _PCOMNEXP storage_error : public journal_error {
      typedef journal_error ancestor ;
   public:
      explicit storage_error(const std::string &s, JournalError errcode = ERR_ERROR) :
         ancestor(s, errcode)
      {}
} ;

/******************************************************************************/
/** Journal existence errors: either a journal already exists when must not, or doesn't
 exist when must present.
*******************************************************************************/
class _PCOMNEXP journal_existence_error : public storage_error {
      typedef storage_error ancestor ;
   public:
      ~journal_existence_error() throw() {}

      const std::string &path() const { return _path ; }
   protected:
      journal_existence_error(const strslice &path, bool exists) :
         ancestor(std::string("Journal '").append(path.begin(), path.end())
                  .append(exists ? "' already exists" : "' does not exist"),
                  exists ? ERR_EXISTS : ERR_DOESNT_EXIST),
         _path(path.begin(), path.end())
      {}
   private:
      std::string _path ;
} ;

class _PCOMNEXP journal_exists_error : public journal_existence_error {
   public:
      explicit journal_exists_error(const strslice &path) :
         journal_existence_error(path, true)
      {}
} ;

class _PCOMNEXP journal_notexists_error : public journal_existence_error {
   public:
      explicit journal_notexists_error(const strslice &path) :
         journal_existence_error(path, false)
      {}
} ;

/******************************************************************************/
/** Journal record format error.
*******************************************************************************/
class _PCOMNEXP format_error : public storage_error {
      typedef storage_error ancestor ;
   public:
      explicit format_error(const std::string &s,
                            JournalError errcode,
                            FormatError errkind) :
         ancestor(s, errcode),
         _errkind(errkind)
      {}

      FormatError kind() const { return _errkind ; }

   private:
      FormatError _errkind ;
} ;

/******************************************************************************/
/** Data checksum error.
*******************************************************************************/
class _PCOMNEXP data_error : public storage_error {
      typedef storage_error ancestor ;
   public:
      explicit data_error(const std::string &s, JournalError errcode) :
         ancestor(s, errcode)
      {}
} ;

/******************************************************************************/
/** Error due to attempt to perform an operation which is invalid in current
 Journallable::state().
*******************************************************************************/
class _PCOMNEXP state_error : public journal_error {
      typedef journal_error ancestor ;
   public:
      explicit state_error(const std::string &s) :
         ancestor(s)
      {}
} ;

} // end of namespace pcomn::jrn
} // end of namespace pcomn


#endif /* __PCOMN_JOURNERROR_H */
