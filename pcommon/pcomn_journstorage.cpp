/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_journstorage.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Abstract storage for journalling engine.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   6 Nov 2008
*******************************************************************************/
#include <pcomn_journstorage.h>
#include <pcomn_diag.h>
#include <pcomn_string.h>
#include <pcomn_utils.h>
#include <pcomn_integer.h>

namespace pcomn {
namespace jrn {

const magic_t STORAGE_CHECKPOINT_MAGIC = {{ '#', 'Y', 'M', 'c', 'p', '1', '\r', '\n' }} ;
const magic_t STORAGE_SEGMENT_MAGIC    = {{ '#', 'Y', 'M', 's', 'g', '1', '\r', '\n' }} ;
const magic_t STORAGE_OPERATION_MAGIC  = {{ '#', 'Y', 'M', 'o', 'p', '1', '\r', '\n' }} ;

static void throw_state_error(const Storage &storage, Storage::State state, const char *action)
{
   LOGPXALERT_CALL(pcomn::throw_exception<state_error>, PCOMN_Journal,
                   "Attempt to " << action << " on " << storage
                   << " from illegal state " << oenum(state)) ;
}

#define ENSURE_STATE(action_text, ...)                                  \
   do {                                                                 \
      const State s = state() ;                                         \
      if (!pcomn::one_of<__VA_ARGS__ >::is(s))                          \
         throw_state_error(*this, s, (action_text)) ;                   \
   } while(false)

#define ENSURE_READABLE(action_text) ENSURE_STATE((action_text), SST_READABLE, SST_READONLY)

/*******************************************************************************
 Storage: read
*******************************************************************************/
bool Storage::close()
{
   write_guard lock (_lock) ;

   const State prevstate = state() ;
   if (prevstate == SST_CLOSED)
      return false ;

   // Ensure _state be SST_CLOSED upon exit in any case
   _state = SST_CLOSED ;
   vsaver<State> state_guard (_state, prevstate) ;

   return do_close_storage() ;
}

bool Storage::replay_record(const record_handler &handler)
{
   TRACEPX(PCOMN_Journal, DBGL_LOWLEV, (handler ? "Replay " : "Skip ")
           << " an operation record of " << *this) ;

   read_guard guard (_lock) ;

   ENSURE_READABLE("read an operation record") ;

   return do_replay_record(handler) ;
}
/*******************************************************************************
 Storage: write
*******************************************************************************/
void Storage::make_writable()
{
   TRACEPX(PCOMN_Journal, DBGL_ALWAYS, "Making writable " << *this) ;

   write_guard guard (_lock) ;

   ENSURE_STATE("make the journal storage writable", SST_CREATED, SST_READABLE) ;

   do_make_writable() ;

   set_state(SST_WRITABLE) ;
}

size_t Storage::append_record(const iovec_t *begin, const iovec_t *end)
{
   if (begin == end)
      return 0 ;

   PCOMN_ENSURE_ARG(begin) ;
   PCOMN_ENSURE_ARG(end) ;

   read_guard guard (_lock) ;

   return do_append_record(begin, end) ;
}

std::pair<binary_obufstream *, generation_t> Storage::create_checkpoint()
{
   write_guard guard (_lock) ;

   return do_create_checkpoint() ;
}

void Storage::close_checkpoint(bool commit)
{
   write_guard guard (_lock) ;

   LOGPXDBG(PCOMN_Journal, DBGL_HIGHLEV, (commit ? "Commit" : "Rollback") << " checkpoint for " << *this) ;

   do_close_checkpoint(commit) ;
}

void Storage::replay_checkpoint(const checkpoint_handler &handler)
{
   ENSURE_READABLE("restore a checkpoint") ;

   do_replay_checkpoint(handler) ;
}

std::ostream &Storage::debug_print(std::ostream &os) const
{
   return os << diag::otptr(this) << ':' << (is_writable() ? "w" : (is_readonly() ? "ro" : "r")) ;
}

} // end of namespace pcomn::jrn
} // end of namespace pcomn
