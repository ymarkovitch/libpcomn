/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_INDICATOR_H
#define __PCOMN_INDICATOR_H
/*******************************************************************************
 FILE         :   pcomn_indicator.h
 COPYRIGHT    :   Yakov Markovitch, 2006-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Indicator/observer classes.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   27 Nov 2006
*******************************************************************************/
#include <pcomn_weakref.h>
#include <pcomn_integer.h>
#include <pcomn_utils.h>
#include <pcomn_except.h>

namespace pcomn {

class change_indicator ;
class change_observer ;

class multi_indicator_base ;
class multi_observer_base ;

template<unsigned num_of_indicators>
class multi_indicator ;
template<bigflag_t observed_indicators>
class multi_observer ;

/*******************************************************************************
              class change_indicator
*******************************************************************************/
class change_indicator {
      PCOMN_WEAK_REFERENCEABLE(change_indicator) ;

      friend class change_observer ;
   public:
      change_indicator() :
         _generation(0)
      {}

      void change() { ++_generation ; }

   private:
      int _generation ;

      int generation() const { return _generation ; }
} ;

/*******************************************************************************
              class change_observer
*******************************************************************************/
class change_observer {
   public:
      static const bigflag_t IndicatorIsDead = (bigflag_t)-1 ;

      change_observer() :
         _generation(-1)
      {}

      explicit change_observer(const change_indicator *indicator) :
         _indicator(indicator),
         _generation(-1)
      {}

      bool is_indicator_alive() const { return !!_indicator.unsafe() ; }

      bigflag_t is_outofdate() const
      {
         const change_indicator *indicator = _indicator.unsafe() ;
         return !indicator ? IndicatorIsDead : !(indicator->generation() ^ _generation) ;
      }

      void invalidate()
      {
         const change_indicator *indicator = _indicator.unsafe() ;
         if (indicator)
            _generation = indicator->generation() - 1 ;
      }

      bool validate()
      {
         const change_indicator *indicator = _indicator.unsafe() ;
         if (!indicator)
            return false ;
         _generation = indicator->generation() ;
         return true ;
      }

      void reset(const change_indicator *indicator = NULL)
      {
         if (_indicator == indicator)
            return ;
         _indicator = indicator ;
         _generation = -1 ;
      }

   private:
      PTWeakReference<change_indicator> _indicator ;
      int                              _generation ;
}  ;

/*******************************************************************************
                     class multi_indicator_base
*******************************************************************************/
class multi_indicator_base {
      PCOMN_WEAK_REFERENCEABLE(multi_indicator_base) ;

      friend class multi_observer_base ;
   public:
      unsigned size() const { return (unsigned)_indicators_count ; }
      bigflag_t valid_flags() const
      {
         return flag_traits::ones >> (flag_traits::bitsize - size()) ;
      }

      void change_single(unsigned indicator_ndx)
      {
         // There should be no multithreaded write access to an indicator
         ++*(begin()
             + ensure_lt<std::out_of_range>(indicator_ndx, size(),
                                            "Indicator index is out of range")) ;
      }

      void change(bigflag_t indicators)
      {
         ensure<std::out_of_range>(
            !(indicators & ~valid_flags()),
             "Flags specifying multiple indicators are out of range") ;
         // It seems that to benefit from more advanced bit algorithms the
         // indicators bitmask should be rather sparse. So we are using simpler
         // approach - "Premature optimization is the root of all evil.", anyway!
         for (unsigned ndx = 0 ; indicators ; indicators >>= 1 )
         {
            while (!(indicators & 1))
            {
               indicators >>= 1 ;
               ++ndx ;
            }
            ++*(begin() + ndx++) ;
         }
      }

   protected:
      typedef int_traits<bigflag_t> flag_traits ;
      typedef int                   count_t ; /* Generation counter type */

      explicit multi_indicator_base(unsigned indicators_count) :
         _indicators_count
         (ensure_le<std::out_of_range>(indicators_count,
                                       flag_traits::bitsize,
                                       "Indicators count is out of range"))
      {}

      multi_indicator_base(const multi_indicator_base &src) :
         _indicators_count(src._indicators_count)
      {}

      // Disallow deleting a pointer to the base class
      ~multi_indicator_base() {}

   private:
      // Please note that assignment is disabled by design: _indicators_count
      // is const
      const uintptr_t _indicators_count ; /* Essentially there is no need in full integer here,
                                           * but indicators will be aligned at least to integer
                                           * anyway */

      count_t *begin()
      {
         return reinterpret_cast<count_t *>(this+1) ;
      }
      const count_t *begin() const
      {
         return const_cast<multi_indicator_base *>(this)->begin() ;
      }
      count_t *end() { return begin() + size() ; }
      const count_t *end() const { return begin() + size() ; }

      count_t generation(unsigned indicator_ndx) const
      {
         NOXCHECK(indicator_ndx < size()) ;
         return *(begin() + indicator_ndx) ;
      }
} ;

/*******************************************************************************
                     template<unsigned num_of_indicators>
                     class multi_indicator
 This class represents a set of sub-indicators, each of which can be both changed
 individually and "in batch". Batch change is specified through the set of
 bit flags (as bigflag_t value).
 The number of possible indicators is limited by 31 (the number of bits in
 bigflags_t minus 1, namely 31). The rightmost bit is reserved for
 multi_observer<> to indicate that its corresponding indicator is
 not more alive.
*******************************************************************************/
template<unsigned num_of_indicators>
class multi_indicator : public multi_indicator_base {
      typedef multi_indicator_base ancestor ;
      PCOMN_STATIC_CHECK(num_of_indicators > 0
                         && num_of_indicators < ancestor::flag_traits::bitsize) ;
   public:
      static const unsigned IndicatorsCount = num_of_indicators ;

      multi_indicator() :
         ancestor(num_of_indicators)
      {
         init() ;
      }
      // Please note that the copy constructor doesn't actually copy
      // indicators.
      multi_indicator(const multi_indicator &) :
         ancestor(num_of_indicators)
      {
         init() ;
      }

   private:
      int _data[num_of_indicators] ;

      void init() { memset(_data, 0, sizeof _data) ; }
} ;

/*******************************************************************************
 multi_indicator<unsigned>
*******************************************************************************/
template<unsigned num_of_indicators>
const unsigned multi_indicator<num_of_indicators>::IndicatorsCount ;

/*******************************************************************************
                     class multi_observer_base
*******************************************************************************/
class multi_observer_base {
   public:
      static const bigflag_t IndicatorIsDead = (bigflag_t)-1 ;

      bool is_indicator_alive() const { return !!unsafe_indicator() ; }

   protected:
      typedef multi_indicator_base::count_t count_t ;

      multi_observer_base() {}

      explicit multi_observer_base(const multi_indicator_base *indicator) :
         _indicator(indicator)
      {}

      // Disallow deleting a pointer to the base class
      ~multi_observer_base() {}

      const multi_indicator_base *unsafe_indicator() const
      {
         return _indicator.unsafe() ;
      }

      void reset(const multi_indicator_base *indicator = NULL)
      {
         _indicator = indicator ;
      }

      count_t indicator_generation(unsigned indicator_ndx) const
      {
         NOXCHECK(unsafe_indicator()) ;
         return unsafe_indicator()->generation(indicator_ndx) ;
      }

   private:
      PTWeakReference<multi_indicator_base> _indicator ;
} ;

/*******************************************************************************
                     template<bigflag_t observed>
                     class multi_observer
 'observed' parameter is a bitmask, which specifies what sub-indicators in the
 corresponding MultipleIndicator to observe. The position of every nonsero bit
 in 'observed' specifies corresponding sub-indicator number
 (e.g. multi_observer<0x41> will observe subindicators 0 and 6 ).
*******************************************************************************/
template<bigflag_t observed>
class multi_observer : public multi_observer_base {
      typedef multi_observer_base ancestor ;
      PCOMN_STATIC_CHECK(observed != 0) ;

   public:
      static const unsigned   IndicatorsCount = bitop::ct_bitcount<observed>::value ;
      static const bigflag_t  ObservedIndicators = observed ;

      multi_observer() { init() ; }

      template<unsigned num_of_indicators>
      multi_observer(const multi_indicator<num_of_indicators> *indicator) :
         ancestor(indicator)
      {
         PCOMN_STATIC_CHECK(IndicatorsCount <= num_of_indicators) ;
         init() ;
      }

      static unsigned size() { return IndicatorsCount ; }

      static bigflag_t observed_indicators() { return ObservedIndicators ; }

      // is_outofdate() -  returns the set of observed subindicators, for that
      //                   this observer is out-of-date, as a bit mask, where
      //                   position of ech nonsero bit specifies a subindicator
      //                   index.
      // If this observer is either not connected or the corresponding indicator
      // is dead (deleted), returns mask with _all_ bits set to 1 (i.e. (bigflag_t)-1).
      //
      bigflag_t is_outofdate() const ;

      bool validate() ;

      void invalidate() { init() ; }

      void reset()
      {
         if (!unsafe_indicator())
            return ;
         ancestor::reset() ;
         init() ;
      }

      template<unsigned num_of_indicators>
      void reset(const multi_indicator<num_of_indicators> *indicator)
      {
         PCOMN_STATIC_CHECK(IndicatorsCount <= num_of_indicators) ;
         if (indicator == unsafe_indicator())
            return ;
         ancestor::reset(indicator) ;
         init() ;
      }

   private:
      int _generation[IndicatorsCount] ;

      void init() { memset(_generation, -1, sizeof _generation) ; }
} ;

/*******************************************************************************
 multi_observer<bigflag_t>
*******************************************************************************/
template<bigflag_t observed>
const unsigned multi_observer<observed>::IndicatorsCount ;
template<bigflag_t observed>
const bigflag_t multi_observer<observed>::ObservedIndicators ;

template<bigflag_t observed>
bigflag_t multi_observer<observed>::is_outofdate() const
{
   using namespace bitop ;

   const multi_indicator_base *indicator = unsafe_indicator() ;
   if (!indicator)
      return IndicatorIsDead ;
   bigflag_t stale_indicators = 0 ;
   bigflag_t current_bit ;
   for (nzbit_iterator<bigflag_t> bititer (observed & indicator->valid_flags()) ;
        (current_bit = *bititer) != 0 ; ++bititer)
   {
      const bigflag_t mask = current_bit - 1 ;
      if (_generation[bitcount(observed & mask)]
          != indicator_generation(bitcount(mask)))
         stale_indicators |= current_bit ;
   }
   return stale_indicators ;
}

template<bigflag_t observed>
bool multi_observer<observed>::validate()
{
   using namespace bitop ;

   const multi_indicator_base *indicator = unsafe_indicator() ;
   if (!indicator)
      return false ;
   const bigflag_t valid_flags = indicator->valid_flags() ;
   for (nzbit_iterator<bigflag_t> bititer (observed & valid_flags) ;
        *bititer ; ++bititer)
   {
      const bigflag_t mask = *bititer - 1 ;
      _generation[bitcount(observed & mask)] = indicator_generation(bitcount(mask)) ;
   }
   return true ;
}

} // end of namespace pcomn

#endif /* __PCOMN_INDICATOR_H */
