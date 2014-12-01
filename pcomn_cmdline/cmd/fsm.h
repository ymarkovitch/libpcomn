//------------------------------------------------------------------------
// ^FILE: fsm.h - define a finite state machine
//
// ^DESCRIPTION:
//     This file defines a finite state machine tailored to the task of
//     parsing syntax strings for command-line arguments.
//
// ^HISTORY:
//    03/27/92	Brad Appleton	<bradapp@enteract.com>	Created
//-^^---------------------------------------------------------------------

class SyntaxFSM {
public:
   enum state_t {
      START,    // start-state
      OPTION,   // just parsed an option-spec
      KEYWORD,  // just parsed a keyword-spec
      VALUE,    // just parsed a value-spec
      LIST,     // just parsed "..."
      FINAL,    // final-state
      ERROR,    // syntax error encountered
   } ;

   struct token_t {
      const char * start;  // start address of token
      unsigned     len;    // length of token

      token_t() : start(0), len(0) {}

      void
      set(const char * s, unsigned l) { start = s, len = l; }
   } ;

   SyntaxFSM() : ntoks(0), nbpairs(0), lev(0), fsm_state(START) {}

      // Reset the FSM
   void
   reset() { ntoks = 0; nbpairs = 0; lev = 0; fsm_state = START; }

      // Return the number of tokens parsed thus far.
   unsigned
   num_tokens() const  { return  ntoks; }

      // Return the number of balanced brace-pairs parsed thus far.
   unsigned
   num_braces() const  { return  nbpairs; }

      // Return the current nesting level of brace-pairs
   int
   level() const  { return  lev; }

      // Return the current machine state
   state_t
   state() const  { return  fsm_state; }

      // Get the next token from "input" and place it in "token"
      // (consuming characters from "input").
      //
      // Return 0 if the resulting state is FINAL;
      // otherwise return NON-zero.
      //
   int
   operator()(const char * & input, token_t & token);

protected:
   unsigned  ntoks;      // number of tokens parsed thus far
   unsigned  nbpairs;    // number of balanced brace-pairs parsed thus far
   int       lev;        // current nesting level of brace-pairs
   state_t   fsm_state;  // current machine state

private:
   void
   skip(const char * & input);

   void
   parse_token(const char * & input);
} ;
