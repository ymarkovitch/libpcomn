/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_JOURNAL_H
#define __PCOMN_JOURNAL_H
/*******************************************************************************
 FILE         :   pcomn_journal.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Abstract journalling engine. Provides a framework for implementation of
                  log-structured systems.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   30 Oct 2008
*******************************************************************************/
/** @file
 Abstract journalling engine: provides a framework for implementation of log-structured
 systems.

 This engine provides the following parts for implementing a log-structured system:

 @li pcomn::jrn::Journallable
 @li pcomn::jrn::Operation
 @li pcomn::jrn::Port
 @li pcomn::jrn::Storage

 pcomn::jrn::Journallable is an interface, through which the journalling engine
 interacts with the journalled system.

 Journallable system defines operations that will be logged. Those operations should be
 based on pcomn::jrn::Operation and implement both marshalling/demarshalling and
 applying to the journallable system while roll-forwarding changes from the journal.

 pcomn::jrn::Port provides access to methods that control the journal, particularly
 journalling an operation; from the journalled system's point of view, Port @em is a
 journal.

 Journallable::restore_from()
      Journallable::restore_checkpoint() ;
      operation_ptr restored_op ;
      while (restored_op = Journallable::unlocked_journal()->next()
                           {
                                operation_ptr op = Journallable::create_operation() ;
                                op->restore() ;
                                return op ;
                           })
      {
         op->apply() ;
      }

 So, calling Journallable::restore_from() in terms of virtual calls:

 Storage::do_replay_checkpoint() ;
    Journallable::restore_checkpoint() ;
 while(there are operations in the journal)
     Storage::do_replay_record() ;
        Journallable::create_operation() ;
        Operation::restore() ;
     Operation::apply() ;

 From the point of view of the journal user (i.e. implementor of Journallable/Operation):

 Journallable::restore_checkpoint() ;
 while(there are operations in the journal)
     Journallable::create_operation() ;
     Operation::restore() ;
     Operation::apply() ;

 Pseudocode of Journallable::take_checkpoint():

 @code

 Journallable::start_checkpoint() ;

 Storage::create_checkpoint() ; // Storage::do_create_checkpoint() with write_guard

 Journallable::save_checkpoint() ;

 // Note that calls are interleaved, not nested:
 // start_checkpoint()->create_checkpoint()->finish_checkpoint()->close_checkpoint()
 Journallable::finish_checkpoint() ;

 Storage::close_checkpoint() ; // Storage::do_close_checkpoint() with write_guard

 @endcode

*******************************************************************************/
#include <pcomn_journerror.h>
#include <pcomn_smartptr.h>
#include <pcomn_platform.h>
#include <pcomn_iostream.h>
#include <pcomn_syncobj.h>
#include <pcomn_synccomplex.h>
#include <pcomn_diag.h>
#include <pcomn_buffer.h>
#include <pcomn_flgout.h>
#include <pcomn_binascii.h>
#include <pcomn_strslice.h>
#include <pcomn_meta.h>

#include <string>
#include <typeinfo>
#include <functional>

namespace pcomn {
namespace jrn {

/******************************************************************************/
/** Journal format version
*******************************************************************************/
const uint16_t FORMAT_VERSION = 1 ;

/*******************************************************************************
 Typedefs for POD journal types
*******************************************************************************/
typedef int32_t   opcode_t ;     /**< The code of journalled operation.

                                  It is defined and interpreded by the journalled
                                  system, but nonetheless stored into the operation
                                  header (written/read by the journal itself) */

typedef uint32_t  opversion_t ; /**< Operation version, provided to allow
                                  upgrade/migration of journaled system.

                                  It is defined and interpreded by the journalled
                                  system, but nonetheless stored into the operation
                                  header (written/read by the journal itself) */

typedef int64_t   generation_t ; /**< Journal generation number. */

/******************************************************************************/
/** "Magic number" - file or record type identifier
*******************************************************************************/
struct magic_t {
      char data[8] ;

      magic_t &clear()
      {
         memset(data, 0, sizeof data) ;
         return *this ;
      }

      bool operator==(const magic_t &rhs) const { return !memcmp(rhs.data, data, sizeof data) ; }
      bool operator!=(const magic_t &rhs) const { return !(*this == rhs) ; }
} ;

inline std::ostream &operator<<(std::ostream &os, const magic_t &magic)
{
   return b2a_cstring(os, &magic, sizeof magic) ;
}

/*******************************************************************************
 Tag constants
*******************************************************************************/
const ulonglong_t NOSIZE = (unsigned long long)-1LL ; /**< Not-a-size tag value */

const generation_t NOGEN  = (generation_t)-1LL ; /**< Not-a-generation tag value */

const size_t MAX_OPSIZE = 64*1024*1024 ; /**< Sanity check value: the maximal size of
                                          marshalled operation data */

const size_t MAX_HDRSIZE = 4096 ; /**< Size limit of any "fixed-size" header
                                   (e.g. OperationHeader) */

const size_t MAX_JNAME = 63 ; /**< Journal name length limit  */

/******************************************************************************/
/** Storage access modes
*******************************************************************************/
enum AccMode {
   MD_RDONLY   = 0x0001,   /**< Read-only mode (only restore journal data after open) */

   MD_WRONLY   = 0x0002,   /**< Write-only mode (don't restore data, create and write
                            checkpoint right away ) */
   MD_RDWR     = 0x0003    /**< Read-write mode () */
} ;

/******************************************************************************/
/** Storage open flags
*******************************************************************************/
enum OpenFlags {
   OF_CREAT    = 0x0100
} ;

/******************************************************************************/
/** Placeholder for test class specializations
*******************************************************************************/
template<class Target> struct Tester ;

/*******************************************************************************
 Forward declarations - classes
*******************************************************************************/
class Port ;
class Storage ;
class Journallable ;
class Operation ;

/*******************************************************************************
 Forward declarations - on-disk structures
*******************************************************************************/
struct OperationHeader ;
struct OperationTail ;

/*******************************************************************************
 Smartpointers
*******************************************************************************/
typedef shared_intrusive_ptr<Operation> operation_ptr ; /**< Shared pointer to a journal operation  */

/*******************************************************************************
 Operation errors
*******************************************************************************/
/******************************************************************************/
/** Invalid operation.
*******************************************************************************/
class _PCOMNEXP op_error : public journal_error {
      typedef journal_error ancestor ;
   protected:
      explicit op_error(opcode_t opcode, opversion_t version, JournalError errcode) ;
} ;

/******************************************************************************/
/** Invalid operation code.
*******************************************************************************/
class _PCOMNEXP opcode_error : public op_error {
   public:
      explicit opcode_error(opcode_t opcode, opversion_t version = 1) :
         op_error(opcode, version, ERR_OPCODE)
      {}
} ;

/******************************************************************************/
/** Invalid operation version.
*******************************************************************************/
class _PCOMNEXP opversion_error : public op_error {
   public:
      explicit opversion_error(opcode_t opcode, opversion_t version) :
         op_error(opcode, version, ERR_OPVERSION)
      {}
} ;

/******************************************************************************/
/** Basic description of a journallable operation.
*******************************************************************************/
struct Opdesc {
      friend class Port ;
   public:
      explicit Opdesc(opcode_t code = 0, opversion_t opversion = 0) :
         _opcode(code),
         _version(opversion)
      {}

      /// Get the code of the journalled operation.
      opcode_t code() const { return _opcode ; }
      /// Get operation version.
      opversion_t version() const { return _version ; }

   private:
      opcode_t             _opcode ;
      opversion_t          _version ;
} ;

inline std::ostream &operator<<(std::ostream &os, const Opdesc &opdesc)
{
   return os << "OP" << opdesc.code() << 'v' << opdesc.version() ;
}

/******************************************************************************/
/** Interface to journal underlying storage.

 Provides interface to journal segments and checkpoints. A storage can be in one of two
 mutual exclusive modes: read mode or write mode.

 Read mode allows to read from the last known consistent checkpoint and from all journal
 segments written after that checkpoint. The following methods are available in the read
 mode:
 @li do_replay_record()
 @li do_replay_checkpoint() ;

 Note that while only one thread is allowed to read the checkpoint, segments can be read
 concurrently.

 Write mode allows to append records to the end of the last journal segment and to commit
 checkpoints. The following methods are available in the write mode:

 @li append_record()
 @li create_checkpoint()
 @li close_checkpoint()

 A concrete storage class MUST implement:

 @li bool do_replay_record(const record_handler &handler)
 @li void do_replay_checkpoint(const checkpoint_handler &) ;
 @li void do_make_writable()
 @li size_t do_append_record(const iovec_t *begin, const iovec_t *end)
 @li binary_obufstream &create_checkpoint()
 @li void close_checkpoint(bool commit)

 A concrete storage class MAY implement:

 @li void std::ostream &debug_print(std::ostream &os) const

*******************************************************************************/
class _PCOMNEXP Storage {
   public:
      friend class Port ;
      friend class Journallable ;

      typedef std::function<bool(opcode_t, opversion_t, const void *, size_t)> record_handler ;
      typedef std::function<void(binary_ibufstream &, size_t)> checkpoint_handler ;

      enum State {
         SST_INITIAL,
         SST_CLOSED,
         SST_CREATED,     /**< The storage is just created in write mode (no headers yet) */
         SST_READABLE,    /**< The storage is open for reading and checked for sanity */
         SST_READONLY,
         SST_WRITABLE     /**< The storage is made writable */
      } ;

      virtual ~Storage()
      {}

      State state() const { return _state ; }

      bool close() ;

      const magic_t &user_magic() const { return _user_magic ; }

      bool replay_record(const record_handler &handler) ;

      void replay_checkpoint(const checkpoint_handler &handler) ;

      bool is_readable() const { return one_of<SST_READABLE, SST_READONLY>::is(state()) ; }
      bool is_writable() const { return state() == SST_WRITABLE ; }
      bool is_readonly() const { return state() == SST_READONLY ; }

      friend std::ostream &operator<<(std::ostream &os, const Storage &s)
      {
         return s.debug_print(os << '<') << '>' ;
      }

   protected:
      explicit Storage() :
         _state(SST_INITIAL)
      {}

      void set_state(State st) { _state = st ; }

      /// Toggle the storage into write mode.
      void make_writable() ;

      void set_user_magic(const magic_t &magic) { _user_magic = magic ; }

      size_t append_record(const iovec_t *begin, const iovec_t *end) ;

      size_t append_record(const void *buf, size_t count)
      {
         PCOMN_ENSURE_ARG(buf) ;
         if (unlikely(!count))
            return 0 ;

         const iovec_t &v = make_iovec(buf, count) ;
         return append_record(&v, &v + 1) ;
      }

      std::pair<binary_obufstream *, generation_t> create_checkpoint() ;

      void close_checkpoint(bool commit) ;

      /// Handle the contents of an actual checkpoint..
      ///
      /// This function is called only when is_writable() is false.
      virtual void do_replay_checkpoint(const checkpoint_handler &) = 0 ;

      /// Read or memory-map an operation record from the current journal segment and
      /// call the record handler
      ///
      /// Called from under the read lock.
      virtual bool do_replay_record(const record_handler &) = 0 ;

      /// Should set the storage in writable state.
      ///
      /// @note Only read-only->writable state transition is possible.
      virtual void do_make_writable() = 0 ;

      /// Should put a record to the end of the active journal segment.
      ///
      /// While calling this function, the journal is always in writable mode.
      virtual size_t do_append_record(const iovec_t *begin, const iovec_t *end) = 0 ;

      /// Should create new checkpoint and get an output stream connected to it.
      ///
      /// Should flush and close the active journal segment and create a new checkpoint
      /// storage.
      virtual std::pair<binary_obufstream *, generation_t> do_create_checkpoint() = 0 ;

      /// Should close the checkpoint.
      virtual void do_close_checkpoint(bool commit) = 0 ;

      /// Should close the storage.
      virtual bool do_close_storage() = 0 ;

      virtual std::ostream &debug_print(std::ostream &os) const ;

   private:
      shared_mutex   _lock ;
      State          _state ;
      magic_t        _user_magic ;

      // Note that we shouldn't acquire the writer lock when
      typedef shared_lock<shared_mutex> read_guard ;
      typedef std::lock_guard<shared_mutex> write_guard ;
} ;

/******************************************************************************/
/** Abstract journallable object.
*******************************************************************************/
class _PCOMNEXP Journallable {
   public:
      friend class Port ;
      friend class Operation ;

      /// Mutually exclusive states of the journallable object.
      enum State {
         ST_INITIAL,    /**< Initial state, the Journallable get to this upon construction */
         ST_RESTORING,  /**< Journallable is in the process of restoring from the journal */
         ST_RESTORED,   /**< Restored from the journal, no journal to write to */
         ST_ACTIVE,     /**< There is a journal to write operations to */
         ST_CHECKPOINT, /**< Making a checkpoint */

         ST_INVALID  = (atomic_t)~(atomic_t())
      } ;

      virtual ~Journallable()
      {}

      /// Get the human-readable name of an operation with the specified opcode
      std::string operation_name(opcode_t opcode, opversion_t version) const  ;

      /// Get the journal this object is being journalled to.
      /// @note Keep in mind that this operation is @em not thread-safe wrt restore_from()
      /// and set_journal(), while @em is thread-safe wrt all other public methods.
      Port *journal() const
      {
         read_guard lock (_lock) ;
         return unlocked_journal() ;
      }

      /// Restore the state of a journallable object from the journal.
      ///
      /// ST_INITIAL -> ST_RESTORED [-> ST_ACTIVE]
      void restore_from(Port &journal, bool set_journal) ;

      /// Set a journal.
      ///
      /// The system starts journalling to the new journal (note that upon construction
      /// the journal is not set). Since Journallable and Port have 1-to-1 relationship,
      /// attempt to set @a new_journal that already have connected Journallable object
      /// will throw exception.
      /// As system starts journalling to the new journal, it first makes initial
      /// checkpoint @em with @em its @em current @em generation @em number, so upon return
      /// from this call there is a checkpoint made.
      /// ST_INITIAL -> ST_ACTIVE
      /// ST_RESTORED -> ST_ACTIVE
      ///
      /// @param new_journal  Where the system to journal to; may be NULL.
      /// @return Previous journal.
      /// @note The Journallable object doesn't own the journal, i.e. @a new_journal
      /// will @em not be deleted automatically upon the Journallable destruction.
      Port *set_journal(Port *new_journal) ;

      /// Take a checkpoint and save it into the journal.
      ///
      /// ST_ACTIVE -> ST_CHECKPOINT
      generation_t take_checkpoint(bigflag_t flags = 0) ;

      State state() const { return _state ; }

      /// Check whether an operation is compatible with the (concrete) journal type.
      ///
      /// The default implementation simply compares typeid(*this) with operation's
      /// target_type()
      virtual bool is_op_compatible(const Operation &op) const ;

      /// Apply an operation to a journallable object.
      ///
      /// Checks whether @a op is compatible by type with the target by calling
      /// Journallable::is_op_compatible()
      void apply(const Operation &op) ;

      /// @overload
      void apply(const operation_ptr &optr) ;

      uatomic_t changecount() const { return atomic_op::get(&_changecnt) ; }

      friend std::ostream &operator<<(std::ostream &os, const Journallable &j)
      {
         return j.debug_print(os << '<') << '>' ;
      }

   protected:
      Journallable() ;

      virtual operation_ptr create_operation(opcode_t opcode, opversion_t version) const = 0 ;

      virtual void start_checkpoint() = 0 ;

      virtual void save_checkpoint(pcomn::binary_obufstream &checkpoint_storage) = 0 ;

      virtual void finish_checkpoint() throw() = 0 ;

      /// This is called from restore_from() and should restore object state.
      /// @note This is called only from restore_from() and at most once
      virtual void restore_checkpoint(pcomn::binary_ibufstream &checkpoint_data,
                                      size_t data_size) = 0 ;

      virtual void dispatch_exception(const std::exception *, State) const ;

      /// Fill the buffer with the user magic number
      ///
      /// A journal calls this method both when it starts writing its first checkpoint
      /// upon storage creation @em and when it starts reading a checkpoint upon opening
      /// for reading. The result returned in @a magic parameter is used both to write a
      /// "user magic number" at the start of a checkpoint or segment file on checkpoint
      /// or segment creation, and to check corresponding magic number while opening a
      /// checkpoint or a segment for reading.
      ///
      /// @note If this method returns false, the journal ignores the value of @a magic
      /// parameter and:
      ///  @li while writing, fills the corresponging magic number with zeros;
      ///  @li while reading, ignores the magic (i.e., accepts @em any user magic number)
      ///
      /// @param[out] magic The buffer for the result value.
      /// @return true if @a magic is filled; false if user magic number should be
      /// ignored.
      virtual bool fill_user_magic(magic_t &magic) const = 0 ;

      virtual std::string readable_opname(opcode_t, opversion_t) const { return std::string() ; }

      virtual std::ostream &debug_print(std::ostream &os) const
      {
         return os << diag::otptr(this) << ':' << state() ;
      }

   private:
      mutable shared_mutex _lock ;
      std::mutex           _cplock ;    /* Checkpoint lock */
      State                _state ;
      Port *               _journal ; /* The current journal (may change during object lifetime) */
      uatomic_t            _changecnt ;

      typedef shared_lock<shared_mutex> read_guard ;
      typedef std::lock_guard<shared_mutex> write_guard ;
      typedef std::lock_guard<std::mutex>    checkpoint_guard ;

      void load_checkpoint(pcomn::binary_ibufstream &checkpoint_data,
                           size_t data_size) ;
      bool load_operation(opcode_t opcode, opversion_t opversion,
                          const void *opdata, size_t data_size) ;

      generation_t take_checkpoint_unlocked(bigflag_t flags) ;

      void apply_restored(const Operation &op) ;
      void apply_created(const Operation &op) ;

      // Doesn't lock the journallable object - call from under the lock
      Port *unlocked_journal() const { return _journal ; }

      State valid_state() const ;

      // Dispatch an exception during saving checkpoint
      void dispatch_checkpoint_exception(const std::exception *x,
                                         Storage *checkpoint_created) ;

} ;

/******************************************************************************/
/** Abstract journalled operation.
*******************************************************************************/
class _PCOMNEXP Operation : public PRefCount {
      PCOMN_NONCOPYABLE(Operation) ;
      PCOMN_NONASSIGNABLE(Operation) ;

      friend class Journallable ;
      friend class Port ;
   public:
      virtual ~Operation()
      {}

      /// Get the code of the journalled operation.
      opcode_t code() const { return _opdesc.code() ; }

      /// Get operation version.
      opversion_t version() const { return _opdesc.version() ; }

      /// Get operation code name, for instance, "MOVE_FILE".
      /// @return Operation code name. Is opcode is invalid, returns an empry string.
      /// @throw nothing
      std::string name() const ;

      /// Some operations may have no body at all, only opcode and version.
      virtual bool has_body() const { return true ; }

      /// Get the actual type of Journallable this operaion apply
      const std::type_info &target_type() const { return _target_type ; }

      void save(binary_obufstream &storage) const { do_save(storage) ; }

      friend std::ostream &operator<<(std::ostream &os, const Operation &op)
      {
         return op.debug_print(os << '<') << '>' ;
      }

   protected:
      Operation(const Journallable &target, opcode_t opccode, opversion_t opversion = 1) :
         _opdesc(opccode, opversion),
         _name(target.readable_opname(opccode, opversion)),
         _target_type(typeid(target))
      {}

      /// Lock the target before journalling and unlock after applying.
      ///
      /// This method is @em not called as the target is being restored from the
      /// journal. Below is pseudocode:
      /// @code
      /// if (!target.being_restored)
      /// {
      ///    lock_target(target, true) ;
      ///    save(target.storage()) ;
      ///    apply(target) ;
      ///    lock_target(target, false) ;
      /// }
      /// else
      ///    apply(target) ;
      /// @endcode
      virtual void lock_target(Journallable &, bool /* acquire */) const {}

      virtual void apply(Journallable &) const = 0 ;

      /// Check if an exception thrown by apply() while rolling forward (restoring)
      /// journalled operation may be safely ignored.
      ///
      /// An operation it is written _before_ applying; so, although do_lock_target
      /// (which is called _before_ storing the operation) can check whether the
      /// operation is applicable, such check may be noncomprehensive, e.g. due to
      /// performance/locking issues. As a result, the operation may fail after having
      /// been saved (though leaving the Journallable in consistent state).
      /// Such operation will inevitably fail while being restored but such failure may
      /// (and should!) be ignored.
      virtual bool is_ignorable_exception(const std::runtime_error &) const
      {
         return false ;
      }

      /// Should write an operation into the journal.
      virtual void do_save(binary_obufstream &storage) const = 0 ;

      /// Should restore an operation from the memory buffer.
      ///
      /// This operation is inverse to do_save()
      virtual void do_restore(const void *buffer, size_t size) = 0 ;

      virtual std::ostream &debug_print(std::ostream &os) const
      {
         return os << diag::otptr(this) << ':' << code() << '(' << name() << ')' ;
      }

   private:
      const Opdesc            _opdesc ;
      const std::string       _name ;        /* Human-readable operation name */
      const std::type_info &  _target_type ; /* The actual type of Journallable this
                                              * operaion apply */

      void restore(const void *buffer, size_t size) ;
} ;


/******************************************************************************/
/** Abstract bodyless operation (i.e. an operation with opcode and opversion only).
*******************************************************************************/
class _PCOMNEXP BodylessOperation : public Operation {
      typedef Operation ancestor ;
   public:
      /// Indicate we have no data
      bool has_body() const { return false ; }

   protected:
      BodylessOperation(const Journallable &target, opcode_t opccode, opversion_t opversion = 1) :
         ancestor(target, opccode, opversion)
      {}

      /// Should never be called.
      void do_save(binary_obufstream &) const
      {
         NOXFAIL("BodylessOperation::do_save() is called, should have never been") ;
      }

      /// Should never be called.
      void do_restore(const void *, size_t)
      {
         NOXFAIL("BodylessOperation::do_restore() is called, should have never been") ;
      }
} ;

/******************************************************************************/
/** Journalled operation template parameterized by the target type.
*******************************************************************************/
template<class Target, class Op = Operation>
struct TargetOperation : public Op {
      // Leave maintenance connection
      friend class Tester<Target> ;
   protected:
      explicit TargetOperation(const Target &target, opcode_t opcode, opversion_t opversion = 1) :
         Op(target, opcode, opversion)
      {}

      virtual void do_apply(Target &target) const = 0 ;

      virtual void do_lock_target(Target & /*target*/, bool /*acquire*/) const {}

   private:
      void apply(Journallable &target) const
      {
         do_apply(*static_cast<Target *>(&target)) ;
      }

      void lock_target(Journallable &target, bool acquire) const
      {
         do_lock_target(*static_cast<Target *>(&target), acquire) ;
      }
} ;

/******************************************************************************/
/** Base operation type selector
*******************************************************************************/
template<bool has_body> struct select_baseop {
      typedef typename std::conditional<has_body, Operation, BodylessOperation>::type type ;
} ;

/******************************************************************************/
/** Interface to a journal, journal descriptor.
 All operations with the journal are performed through its port.
*******************************************************************************/
class _PCOMNEXP Port {
      PCOMN_NONCOPYABLE(Port) ;
      PCOMN_NONASSIGNABLE(Port) ;
   public:
      friend class Journallable ;

      /// Create a journal port and link to it a journal storage.
      ///
      /// A port object "owns" the passed storage, i.e. Port's destructor deletes it;
      /// besides, only one port can be linked with a storage.
      ///
      /// @param journal_storage The journal storage this port should provide access to;
      /// this parameter cannot be NULL. Note also that Port destructor will call
      /// operator delete for this pointer.
      ///
      explicit Port(Storage *journal_storage) ;

      virtual ~Port() ;

      /// Read the next operation description from the journal and skip operation data
      /// @note In contrast to next() this method doesn't require the port to be
      /// connected to a Journallable.
      /// @param result If not NULL, points to the buffer where skip() places the loaded
      /// operation description.
      /// @return The full size of the operation in the journal, including the magic
      /// prefix, header and trailing CRC; at the end of the journal returns 0.
      ///
      size_t skip(Opdesc *result = NULL) ;

      /// Read the next operation description from the journal and create an operation.
      ///
      /// Requires a Journallable object to be connected to the port: asks the connected
      /// Journallable to create an operation (see Journallable::create_operation) and
      /// then passes operation's data to Operation::restore().
      /// @return A smartpointer to the restored operation; at the end of the journal
      /// returns NULL.
      ///
      /// @note This operation is @em NOT thread-safe with respect to
      /// Journallable::set_journal() and Journallable::restore_from()
      ///
      operation_ptr next() ;

      /// Put an operation into a journal.
      ///
      /// Checks whether @a op is compatible by type with target()
      size_t store(const Operation &op) ;

      /// @overload
      size_t store(const operation_ptr &optr)
      {
         return store(*PCOMN_ENSURE_ARG(optr)) ;
      }

      /// Get current generation of the journal.
      /// @note For two successive generations G(i) and G(i+1) always holds G(i+1)>G(i),
      /// but _not_ necessary G(i+1)-G(i)==1 (i.e., generation increment may vary).
      generation_t generation() const ;

      /// Get storage state
      Storage::State storage_state() const { return storage().state() ; }

      friend std::ostream &operator<<(std::ostream &os, const Port &p)
      {
         return os << "<port " << &p << " to " << diag::oderef(p._storage) << '>' ;
      }

   private:
      PTSafePtr<Storage> _storage ;
      Journallable *     _target ;
      std::mutex         _lock ;

      typedef std::lock_guard<std::mutex> guard ;

      void close() {}

      // Called in safe environment, shouldn't lock or check anything
      size_t store_operation(const Operation &op) ;

      // Called in safe environment, shouldn't lock or check anything
      // begin_data, end_data designate _only_ operation data, Port appends header/tail
      // itself (we use "vector" representation for operation data as it is more
      // universal and allows for "scatter" buffers).
      size_t store_operation(opcode_t code, opversion_t version,
                             const iovec_t *begin_data, const iovec_t *end_data) ;

      Storage *unsafe_storage() const { return _storage.get() ; }
      Journallable *target() const { return _target ; }

      Storage &storage() const
      {
         return *ensure_nonzero<object_closed>(unsafe_storage(), "Journal storage") ;
      }
} ;

/*******************************************************************************
 Journallable
*******************************************************************************/
inline bool Journallable::is_op_compatible(const Operation &op) const
{
   // Default is trivial: the same target type
   return op.target_type() == typeid(*this) ;
}

inline void Journallable::apply(const operation_ptr &optr)
{
   apply(*PCOMN_ENSURE_ARG(optr)) ;
}

} // end of namespace pcomn::jrn
} // end of namespace pcomn

PCOMN_DEFINE_ENUM_ELEMENTS(pcomn::jrn::Journallable, State, 6,
                           ST_INITIAL,
                           ST_RESTORING,
                           ST_RESTORED,
                           ST_ACTIVE,
                           ST_CHECKPOINT,
                           ST_INVALID) ;


PCOMN_DEFINE_ENUM_ELEMENTS(pcomn::jrn::Storage, State,
                           6,
                           SST_INITIAL,
                           SST_CLOSED,
                           SST_CREATED,
                           SST_READABLE,
                           SST_READONLY,
                           SST_WRITABLE) ;

inline std::ostream &operator<<(std::ostream &os, pcomn::jrn::Journallable::State s)
{
   return os << pcomn::oenum(s) ;
}

#endif /* __PCOMN_JOURNAL_H */
