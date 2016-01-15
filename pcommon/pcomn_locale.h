/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_LOCALE_H
#define __PCOMN_LOCALE_H
/*******************************************************************************
 FILE         :   pcomn_locale.h
 COPYRIGHT    :   Yakov Markovitch, 2007-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Locale manipulations.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   5 Jul 2007
*******************************************************************************/
#include <pcommon.h>
#include <pcomn_except.h>
#include <locale.h>

namespace pcomn {

/*******************************************************************************
                     class locale_saver
*******************************************************************************/
/** Guard class, saves the current locale in the constructor and restores in the
    destructor.
    Objects of this class are intended to be constructed only on the stack and
    allow safe and easy temporary changing of locale.
    locale_saver also allows to set new locale together with saving current one.
*******************************************************************************/
class locale_saver {
      PCOMN_NONCOPYABLE(locale_saver) ;
      PCOMN_NONASSIGNABLE(locale_saver) ;
   public:
      /// Constructor saves the current locale and, possibly, sets another one.
      /// If @a category == -1 means "don't touch and/or save any locale category",
      /// thus making locale_saver a no-op.
      explicit locale_saver(int category, const char *newlocale = NULL) :
         _category(category)
      {
         if (category == -1)
            return ;
         const char * const prev = setlocale(category, newlocale) ;
         strncpy(_prevlocname, ensure_nonzero<system_error>(prev), sizeof _prevlocname) ;
         ensure<implimit_error>(!_prevlocname[sizeof _prevlocname - 1],
                                "The name of the current locale is too long.") ;
      }

      /// Destructor restores saved locale.
      ~locale_saver()
      {
         if (_category == -1)
            return ;
         setlocale(_category, _prevlocname) ;
      }

   private:
      const int   _category ;
      char        _prevlocname[128] ;
} ;

} // end of namespace pcomn

#endif /* __PCOMN_LOCALE_H */
