//------------------------------------------------------------------------
// ^FILE: states.h - state definitions for the CmdLine library
//
// ^DESCRIPTION:
//     This file contains the definitions for the various values of the
//  state and parse-state of a command-line object. It also contains
//  any definitions that are dependent upon the command-line syntax
//  (i.e. unix_style or vms_style).
//
// ^HISTORY:
//    03/26/92	Brad Appleton	<bradapp@enteract.com>	Created
//-^^---------------------------------------------------------------------

#ifndef _states_h
#define _states_h

#include "cmdline.h"

static const unsigned DEFAULT_CMDFLAGS =
#ifdef vms_style
   // Default command-flags for a vms-command
  0
#else
   // Default command-flags for a unix-command
  CmdLine::OPTS_FIRST
#endif
   ;

//
// cmd_state_t -- Define the bitmasks used to record the command state
//
enum cmd_state_t {
   cmd_END_OF_OPTIONS  = 0x01,  // no more options/keywords?
   cmd_OPTIONS_USED    = 0x02,  // were options used on cmdline?
   cmd_KEYWORDS_USED   = 0x04,  // were keywords used on cmdline?
   cmd_GUESSING        = 0x08,  // are we currently trying to guess?
} ;

//
// cmd_parse_state_t -- Define the possible parse-states for the command
//
// We use "START_STATE" to reset the state. Only one of the NEED*
// states may be set at a time. For any of the NEED* states, TOK_REQUIRED
// may or may not be set. TOK_REQUIRED should NOT be set if none of the
// NEED* states is set.
//
// Note: we have the "states" set up so that one can test for WANT or NEED
// by a bitwise & with a WANT flag. One can test if the particular "WANT"
// is truly "NEEDED" by a bitwise & with the TOK_REQUIRED_FLAG. For
// convenience, each WANT_XXX that is truly REQUIRED may also be
// represented by NEED_XXX.
//
enum cmd_parse_state_t {
   cmd_START_STATE  = 0x00,  // start-state (this MUST be 0)

   cmd_TOK_REQUIRED = 0x01,  // is the "wanted" token required?

   cmd_WANT_VAL     = 0x02,  // are we expecting a value?
   cmd_NEED_VAL     = (cmd_WANT_VAL | cmd_TOK_REQUIRED),

#ifdef vms_style
   cmd_WANT_VALSEP  = 0x04,  // are we expecting ':' or '='
   cmd_NEED_VALSEP  = (cmd_WANT_VALSEP | cmd_TOK_REQUIRED),

   cmd_WANT_LISTSEP = 0x08,  // are we expecting ',' or '+'
   cmd_NEED_LISTSEP = (cmd_WANT_LISTSEP | cmd_TOK_REQUIRED),
#endif
} ;


#endif /* _states_h */
