/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_LANES_H
#define __PCOMN_LANES_H
/*******************************************************************************
 FILE         :   pcomn_lanes.h
 COPYRIGHT    :   Yakov Markovitch, 2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Provide the possibility of running multiple single-threaded states
                  in parallel, a state per thread.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   15 Jan 2014
*******************************************************************************/
/** @file
  Provide the possibility of running multiple single-threaded states in parallel,
  a state per thread.
*******************************************************************************/
#include <pcomn_meta.h>
#include <pcomn_counter.h>
#include <pcomn_syncobj.h>
#include <pcomn_assert.h>

#include <thread>
#include <deque>
#include <utility>
#include <memory>
#include <type_traits>

namespace pcomn {

/******************************************************************************/
/** Provide the possibility of running multiple single-threaded states in parallel,
 a state per thread
*******************************************************************************/
template<typename State>
class lanes {
      // The state must be copyable: we create a sample state, and then create state
      // for every lane copy-constructing that sample
      PCOMN_STATIC_CHECK(std::is_copy_constructible<State>::value) ;
   public:
      typedef State state_type ;
      class         state_token ;

      /************************************************************************/
      /** A pseudo-reference to a state
      *************************************************************************/
      class state_token {
         public:
            typedef state_type element_type ;

            /// Create an empty token
            ///
            /// Operator bool() for an empty token returns false; dereferencing an empty
            /// token through * or -> will throw std::logic_error.
            ///
            state_token() : _ndx(0), _generation(0) {}

            /// Move constructor
            ///
            /// After moving, @a src becomes empty.
            ///
            state_token(state_token &&src) noexcept :
               _ndx(src._ndx),
               _generation(src._generation)
            {
               src._ndx = 0 ;
               src._generation = 0 ;
            }

            /// Create a new state
            ///
            /// Takes and forwards State constructor arguments.
            ///
            template<typename... Args>
            state_token(Args&&... args) ;

            ~state_token() ;

            /// Move assignment, makes @a src empty
            state_token &operator=(state_token &&src) noexcept
            {
               using std::swap ;
               NOXCHECK(this != &src) ;

               state_token expiring (std::move(*this)) ;
               swap(*this, src) ;
               return *this ;
            }

            /// Indicate whether the token refers to some state or is empty
            bool valid() const { return !!_generation ; }

            /// Indicate whether the token refers to some state or is empty
            explicit operator bool() const { return valid() ; }

            state_type &operator*() const { return *get_element() ; }
            state_type *operator->() const { return get_element() ; }

            friend std::ostream &operator<<(std::ostream &os, const state_token &v)
            {
               return os << "state_token{" << v._ndx << ',' << v._generation << '}' ;
            }

         private:
            size_t  _ndx ;       /* Index in _lanes_registry */
            int64_t _generation ;

            state_type *get_element() const
            {
               pcomn::ensure<std::logic_error>
                  (valid(), "Attempt to get a state pointer from empty lane state token") ;

               state_array &states = _thread_lane._states ;
               if (_ndx >= states.size())
                  states.resize(_ndx + 1) ;

               auto &element = states[_ndx] ;
               if (!element)
               {
                  allocate_state() ;
                  NOXCHECK(element) ;
               }
               return element.state.get() ;
            }

            void allocate_state() const ;
      } ;

   private:
      /*************************************************************************
       Global state data
      *************************************************************************/
      struct state_ref ;
      // Must not invalidate iterators on appending items, thus deque instead
      // of a vector
      typedef std::deque<state_ref> state_array ;

      struct state_ref {
            state_array *prev ;
            state_array *next ;
            std::unique_ptr<state_type> state ;

            explicit operator bool() const { return !!state ; }

            state_ref() = default ;

            state_ref(state_ref &) = delete ;
            void operator=(state_ref &) = delete ;
      } ;

      /*************************************************************************
       Per-thread data
      *************************************************************************/
      class thread_lane {
            thread_lane() = default ;
            ~thread_lane() { for (unsigned i = 0 ; i < _states.size() ; clear_state(i++)) ; }

            void clear_state(unsigned ndx)
            {
               state_ref &s = _states[ndx] ;
               if (!s.state)
               {
                  NOXCHECK(!s.prev) ;
                  NOXCHECK(!s.next) ;
                  return ;
               }
               // Remove from the doubly-linked list of states with the same index in
               // other lanes
               if (s.prev)
                  s.prev->next = s.next ;
               if (s.next)
                  s.next->prev = s.prev ;
               s.prev = s.next = NULL ;
               // Destroy the state object
               s.state.reset() ;
            }

            state_array _states ;
      } ;

   private:
      static shared_mutex  _state_lock ;
      static state_array   _lanes_registry ;

      static thread_local thread_lane _thread_lane ;
} ;

template<typename State>
thread_local typename lanes<State>::thread_lane lanes<State>::_thread_lane ;

/*******************************************************************************
 lanes<State>::state_token
*******************************************************************************/
template<typename State>
__noinline void lanes<State>::state_token::allocate_state() const
{
}

} // end of namespace pcomn

#endif /* __PCOMN_LANES_H */
