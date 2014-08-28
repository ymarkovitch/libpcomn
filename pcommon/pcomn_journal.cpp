/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_journal.cpp
 COPYRIGHT    :   Yakov Markovitch, 2010-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Journalling engine implementation.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   6 Nov 2008
*******************************************************************************/
#include <pcomn_journal.h>
#include <pcomn_journstorage.h>
#include <pcomn_diag.h>
#include <pcomn_string.h>
#include <pcomn_utils.h>
#include <pcomn_integer.h>
#include <pcomn_alloca.h>

#include <functional>

#define LOGINFO(output) LOGPXINFO(PCOMN_Journal, output)
#define LOGDBG(output)  LOGPXDBG(PCOMN_Journal, DBGL_ALWAYS, output)

namespace pcomn {
namespace jrn {

using namespace std::placeholders ;

/*******************************************************************************
 Port
*******************************************************************************/
Port::Port(Storage *journal_storage) :
   _storage(PCOMN_ENSURE_ARG(journal_storage)),
   _target(NULL)
{
   TRACEPX(PCOMN_Journal, DBGL_ALWAYS, "Created " << *this) ;
}

Port::~Port()
{
   TRACEPX(PCOMN_Journal, DBGL_ALWAYS, "Destructing " << *this) ;
}

size_t Port::store_operation(const Operation &op)
{
   if (op.has_body())
   {
      // FIXME: Avoid double-buffering! Efficient memory stream needed.
      binary_ostrstream buf ;
      binary_obufstream os (buf, 4096) ;
      op.save(os) ;
      os.flush() ;

      const std::string &s = buf.str() ;
      if (!s.empty())
      {
         const iovec_t &v = make_iovec(s.c_str(), s.size()) ;
         return store_operation(op.code(), op.version(), &v, &v + 1) ;
      }
   }

   return store_operation(op.code(), op.version(), NULL, NULL) ;
}

size_t Port::store_operation(opcode_t code, opversion_t version,
                             const iovec_t *begin_data, const iovec_t *end_data)
{
   TRACEPX(PCOMN_Journal, DBGL_LOWLEV, "Port " << *this
           << " stores operation " << code << " version " << version) ;

   NOXCHECK(_storage) ;
   // There should be place for the head and tail
   PCOMN_VERIFY((size_t)(end_data - begin_data) < MAX_IOVEC_COUNT - 2) ;

   struct {
         magic_t           opmagic ;
         OperationHeader   header ;
         OperationTail     tail ;
   }
   wrapping ;

   PCOMN_STATIC_CHECK(sizeof wrapping ==
                      sizeof wrapping.opmagic + sizeof wrapping.header + sizeof wrapping.tail) ;

   const size_t data_size = bufsizev(begin_data, end_data) ;

   wrapping.opmagic = STORAGE_OPERATION_MAGIC ;
   init_header(wrapping.header) ;
   init_tail(wrapping.tail) ;

   wrapping.header.opcode = code ;
   wrapping.header.opversion = version ;
   wrapping.tail.data_size = wrapping.header.data_size = data_size ;

   htod(wrapping.header) ;
   htod(wrapping.tail) ;

   // Optimize for bodiless operation
   if (!data_size)
   {
      wrapping.tail.crc32 =
         calc_crc32(0, &wrapping.header, sizeof wrapping.header + offsetof(OperationTail, crc32)) ;

      htod(wrapping.tail.crc32) ;

      const iovec_t &v = make_iovec(&wrapping, sizeof wrapping) ;
      return
         unsafe_storage()->append_record(&v, &v + 1) ;
   }

   // "Wrap" the original data vector with both the operation head and tail, add
   // 3 extra items: head, padding, tail
   iovec_t *datavec_begin =
      P_ALLOCA(iovec_t, end_data - begin_data + 3) ;

   *datavec_begin = make_iovec(&wrapping, (char *)&wrapping.tail - (char *)&wrapping) ;
   iovec_t *datavec_end = raw_copy(begin_data, end_data, datavec_begin + 1) ;

   // The operation data, as it is written to a file, should be aligned to 8
   static const char padding[7] = "" ;
   const iovec_t &pv = make_iovec(&padding, aligned_size(data_size) - data_size) ;

   if (pv.iov_len)
      // Add padding, if needed
      *datavec_end++ = pv ;
   *datavec_end++ = make_iovec(&wrapping.tail, sizeof wrapping.tail) ;

   // The whole operation data should be aligned to 8
   NOXCHECK(is_aligned(bufsizev(datavec_begin, datavec_end))) ;

   wrapping.tail.crc32 =
      calc_crc32(calc_crc32v(calc_crc32(0, &wrapping.header, sizeof wrapping.header),
                             datavec_begin + 1, datavec_end - 1),
                 4, *(datavec_end - 1)) ;

   htod(wrapping.tail.crc32) ;

   return
      unsafe_storage()->append_record(datavec_begin, datavec_end) ;
}

/*******************************************************************************
 Journallable
*******************************************************************************/
#define ENSURE_STATE(action_text, ...)                                  \
   {                                                                    \
      const State s = valid_state() ;                                   \
      if (!pcomn::one_of<__VA_ARGS__ >::is(s))                          \
         LOGPXERR_CALL(throw_exception<state_error>, PCOMN_Journal,     \
                       "Attempt to " << action_text << " on " << *this  \
                       << " from illegal state " << pcomn::oenum(s)) ;  \
   }

Journallable::Journallable() :
   _state(ST_INITIAL),
   _journal(NULL),
   _changecnt(0)
{}

Journallable::State Journallable::valid_state() const
{
   return state() ;
}

static inline std::string forged_opname(opcode_t opcode, opversion_t version)
{
   // The default implementation will return someting like "OP13v0"
   char buf[64] ;
   bufprintf(buf, "OP%dv%u", (int)opcode, (unsigned)version) ;
   return std::string(buf) ;
}

std::string Journallable::operation_name(opcode_t opcode, opversion_t version) const
{
   const std::string &name = readable_opname(opcode, version) ;

   return !name.empty() ? name : forged_opname(opcode, version) ;
}

void Journallable::restore_from(Port &journal, bool set_journal)
{
   write_guard lock (_lock) ;

   LOGDBG("Restoring " << *this << " from " << journal) ;

   ENSURE_STATE("restore from a journal", ST_INITIAL) ;

   Port::guard journal_lock (journal._lock) ;

   Storage &storage = journal.storage() ;

   // Make sure in the case of exception we remain in detectable invalid state
   _state = ST_INVALID ;

   vsaver<State> state_guard (_state, ST_RESTORING) ;

   // Restore the last readable checkpoint
   storage.
      replay_checkpoint(std::bind(&Journallable::load_checkpoint, this, _1, _2)) ;

   LOGDBG("Loading operations for " << *this) ;

   unsigned opcount ;
   for (opcount = 0 ;
        storage.
           replay_record(std::bind(&Journallable::load_operation, this, _1, _2, _3, _4)) ;
        ++opcount) ;

   LOGDBG("Successfully loaded " << opcount << " operations for " << *this) ;

   state_guard.release() ;

   _state = ST_RESTORED ;

   LOGDBG("Successfully restored " << *this) ;

   if (!set_journal)
      return ;

   TRACEPX(PCOMN_Journal, DBGL_ALWAYS, "Set journal " << journal << " to " << *this) ;

   // Connect port to the journal
   pcomn::vsaver<Port *> journal_guard (_journal, &journal) ;
   pcomn::vsaver<Journallable *> target_guard (journal._target, this) ;

   // Make the storage writable
   journal.storage().make_writable() ;

   target_guard.release() ;
   journal_guard.release() ;

   _state = ST_ACTIVE ;
}

void Journallable::load_checkpoint(pcomn::binary_ibufstream &checkpoint_stream,
                                   size_t data_size)
{
   LOGDBG("Loading " << data_size << " bytes of checkpoint data for " << *this) ;

   restore_checkpoint(checkpoint_stream, data_size) ;
}

bool Journallable::load_operation(opcode_t opcode, opversion_t opversion,
                                  const void *opdata, size_t data_size)
{
   TRACEPX(PCOMN_Journal, DBGL_VERBOSE, "Loading operation "
           << operation_name(opcode, opversion) << " data="
           << opdata << " size=" << data_size) ;

   NOXCHECK(opdata || !data_size) ;

   const operation_ptr op = create_operation(opcode, opversion) ;
   // Restore (if necessary) and apply the operation
   op->restore(opdata, data_size) ;
   apply_restored(*op) ;

   return true ;
}

void Journallable::apply(const Operation &op)
{
   PCOMN_THROW_IF(!is_op_compatible(op), std::invalid_argument,
                  "Operation %s is not compatible with journallable of type %s",
                  op.name().c_str(), PCOMN_TYPENAME(*this)) ;

   // FIXME: exclusive lock
   write_guard lock (_lock) ;

   ENSURE_STATE("apply a new operation", ST_RESTORED, ST_ACTIVE, ST_CHECKPOINT) ;

   apply_created(op) ;
}

void Journallable::apply_restored(const Operation &op)
{
   TRACEPX(PCOMN_Journal, DBGL_MIDLEV, "Applying restored " << op << " to " << *this) ;

   NOXCHECK(state() == ST_RESTORING) ;
   NOXCHECK(is_op_compatible(op)) ;

   try {
      // Needn't lock, needn't store
      op.apply(*this) ;

      atomic_op::inc(&_changecnt) ;
   }
   catch (const std::runtime_error &x)
   {
      // We may choose to ignore an exception: when an operation is being stored into
      // the journal, it is written _before_ applying; so, although do_lock_target
      // (which is called _before_ storing the operation) can check whether the
      // operation is applicable, such check may be noncomprehensive, e.g. due to
      // performance/locking issues. As a result, the operation may fail after having
      // been saved (though leaving the Journallable in consistent state).
      // Such operation will also inevitably fail while being restored but failure may
      // (and should!) be ignored.
      if (!op.is_ignorable_exception(x))
      {
         LOGPXERR(PCOMN_Journal, "Non-ignorable exception restoring " << op
                   << " to " << *this << ": " << STDEXCEPTOUT(x)) ;
         throw ;
      }
      LOGINFO("Ignorable exception restoring " << op << " to " << *this << ": " << STDEXCEPTOUT(x)) ;
   }

   TRACEPX(PCOMN_Journal, DBGL_MIDLEV, "OK applied restored " << op) ;
}

void Journallable::apply_created(const Operation &op)
{
   TRACEPX(PCOMN_Journal, DBGL_MIDLEV, "Applying new " << op << " to " << *this) ;

   NOXCHECK((pcomn::one_of<ST_RESTORED, ST_ACTIVE, ST_CHECKPOINT>::is(state()))) ;
   NOXCHECK(is_op_compatible(op)) ;

   op.lock_target(*this, true) ;

   try {
      if (state() != ST_RESTORED)
      {
         TRACEPX(PCOMN_Journal, DBGL_LOWLEV, "Storing " << op << " to " << *unlocked_journal()) ;

         unlocked_journal()->store_operation(op) ;
      }
      TRACEPX(PCOMN_Journal, DBGL_VERBOSE, "Actually applying " << op) ;

      op.apply(*this) ;

      atomic_op::inc(&_changecnt) ;
   }
   catch (const std::exception &x) {
      LOGPXERR_CALL(NOXFAIL, PCOMN_Journal, STDEXCEPTOUT(x) << "\nwhile applying " << op
                    << "\nto " << *unlocked_journal() << ".\nThe operation is NOT applied") ;
      return ;
   }
   catch (...) {
      LOGPXERR_CALL(NOXFAIL, PCOMN_Journal, "Unknown exception while applying " << op
                    << "\nto " << *unlocked_journal() << ".\nThe operation is NOT applied") ;
      return ;
   }

   op.lock_target(*this, false) ;

   TRACEPX(PCOMN_Journal, DBGL_MIDLEV, "OK applied new " << op) ;
}

Port *Journallable::set_journal(Port *new_journal)
{
   TRACEPX(PCOMN_Journal, DBGL_ALWAYS,
           "Set journal " << diag::oderef(new_journal) << " to " << *this) ;

   write_guard lock (_lock) ;

   const State s = valid_state() ;
   if (new_journal)
   {
      ENSURE_STATE("set the journal for writing", ST_INITIAL, ST_RESTORED) ;
      PCOMN_VERIFY(!_journal) ;

      pcomn::vsaver<Port *> journal_guard (_journal, new_journal) ;
      pcomn::vsaver<Journallable *> target_guard (new_journal->_target) ;

      // Connect port to the journal
      {
         Port::guard lock (new_journal->_lock) ;
         Storage &storage = new_journal->storage() ;

         // The new journal should be both open and not yet connected
         PCOMN_ASSERT_ARG(!new_journal->target()) ;
         new_journal->_target = this ;

         // Set storage user magic
         magic_t umagic ;
         fill_user_magic(umagic) ;
         storage.set_user_magic(umagic) ;

         // Make the storage writable
         storage.make_writable() ;
      }

      // Take a checkpoint
      take_checkpoint_unlocked(0) ;

      target_guard.release() ;
      return journal_guard.release() ;
   }
   else
   {
      if (pcomn::one_of<ST_INITIAL, ST_RESTORED>::is(s))
         return NULL ;

      ENSURE_STATE("drop the journal", ST_ACTIVE) ;

      NOXCHECK(_journal) ;
      NOXCHECK(_journal->target() == this) ;

      _journal->close() ;
      _journal->_target = NULL ;
      _state = ST_RESTORED ;

      return pcomn::xchange(_journal, nullptr) ;
   }
}

generation_t Journallable::take_checkpoint(bigflag_t flags)
{
   TRACEPX(PCOMN_Journal, DBGL_ALWAYS, "Take checkpoint of " << *this << ", flags " << HEXOUT(flags)) ;

   read_guard lock (_lock) ;
   ENSURE_STATE("take a checkpoint", ST_ACTIVE) ;

   return take_checkpoint_unlocked(flags) ;
}

void Journallable::dispatch_exception(const std::exception *x, State s) const
{
   LOGPXDBG(PCOMN_Journal, DBGL_ALWAYS, "Dispatch_exception called at state " << s
            << " due to " << PCOMN_DEREFTYPENAME(x) << ": " << (x ? x->what() : "UNKNOWN ERROR")) ;
}

void Journallable::dispatch_checkpoint_exception(const std::exception *x,
                                                 Storage *checkpoint_created)
{
   const char * const exception_text = x ? x->what() : "UNKNOWN ERROR" ;

   LOGPXERR(PCOMN_Journal, "Exception " << PCOMN_DEREFTYPENAME(x) << ": " << exception_text) ;

   if (checkpoint_created)
   {
      try {
         checkpoint_created->close_checkpoint(false) ;
      }
      catch (const std::exception &other) {
         LOGPXERR(PCOMN_Journal, "Exception " << PCOMN_DEREFTYPENAME(x)
                   << " while rolling back checkpoint storage: " << other.what()) ;
      }
      catch (...) {
         LOGPXERR(PCOMN_Journal, "Unknown exception while rolling back checkpoint storage") ;
      }
      if (state() == ST_INITIAL)
         checkpoint_created->close() ;
   }

   finish_checkpoint() ;
   // dispatch_exception may rethrow exceptions
   dispatch_exception(x, ST_CHECKPOINT) ;
}

generation_t Journallable::take_checkpoint_unlocked(bigflag_t flags)
{
   TRACEPX(PCOMN_Journal, DBGL_ALWAYS, "Unsafe take checkpoint of "
           << *this << ", flags " << HEXOUT(flags)) ;
   PCOMN_USE(flags) ;

   NOXCHECK(unlocked_journal()) ;
   NOXCHECK(unlocked_journal()->storage().is_writable()) ;

   LOGDBG("Taking checkpoint of " << *this) ;

   // Checkpointing is taking place under its own lock
   checkpoint_guard cpguard (_cplock) ;

   pcomn::vsaver<State> state_guard (_state, ST_CHECKPOINT) ;

   Storage &storage = unlocked_journal()->storage() ;

   TRACEPX(PCOMN_Journal, DBGL_HIGHLEV, "Starting checkpointing " << *this << " to " << storage) ;

   // Notify the Journallable we start checkpointing; note it is called _before_ the
   // storage.create_checkpoint().
   // Journallable may rely upon that finish_checkpoint() will be called no matter
   // whether checkpoint writing was successful or not.
   start_checkpoint() ;

   Storage *checkpoint_created = NULL ;
   const char *stage = "creating checkpoint storage" ;

   try {
      TRACEPX(PCOMN_Journal, DBGL_HIGHLEV, *this << ": " << stage) ;

      const std::pair<binary_obufstream *, generation_t>
         checkpoint_storage = storage.create_checkpoint() ;

      NOXCHECK(checkpoint_storage.first) ;

      checkpoint_created = &storage ;

      stage = "saving checkpoint" ;

      LOGDBG("Created checkpoint storage at generation "
             << checkpoint_storage.second << ", " << stage) ;

      // Call virtual function that should save this checkpoint
      save_checkpoint(*checkpoint_storage.first) ;

      stage = "committing checkpoint storage" ;

      LOGDBG("Checkpoint saved, " << stage << " of " << *this) ;

      // Close and commit checkpoint storage
      storage.close_checkpoint(true) ;

      stage = "finishing checkpoint" ;

      // Note that finish_checkpoint() has empty throw() specification and cannot throw
      // exceptions
      finish_checkpoint() ;

      state_guard.release() ;

      _state = ST_ACTIVE ;

      LOGDBG("Successfully taken checkpoint of " << *this) ;

      // Return checkpoint generation
      return checkpoint_storage.second ;
   }
   catch (const std::exception &x)
   {
      dispatch_checkpoint_exception(&x, checkpoint_created) ;
      throw ;
   }
   catch (...)
   {
      dispatch_checkpoint_exception(NULL, checkpoint_created) ;
      throw ;
   }
}

/*******************************************************************************
 Operation
*******************************************************************************/
std::string Operation::name() const
{
   return _name.empty() ? forged_opname(code(), version()) : _name ;
}

void Operation::restore(const void *buffer, size_t size)
{
   const bool body = has_body() ;
   // Sanity check: there should be no data for bodyless operations and there _must_ be
   // data for all others
   PCOMN_ENSURE(!body == !size, body
                ? "Non-bodyless journal operation must have data to load"
                : "Bodyless journal operation shall not have data to load") ;

   // Restore (if necessary)
   if (body)
      do_restore(buffer, size) ;
}

/*******************************************************************************
 op_error
*******************************************************************************/
op_error::op_error(opcode_t opcode, opversion_t version, JournalError errcode) :
   ancestor(strprintf(errcode == ERR_OPCODE
                      ? "Invalid opcode: opcode=%d, opversion=%u"
                      : "Invalid opversion: opcode=%d, opversion=%u",
                      (int)opcode, (unsigned)version))
{}

} // end of namespace pcomn::jrn
} // end of namespace pcomn
