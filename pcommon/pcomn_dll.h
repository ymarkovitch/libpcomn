/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_DLL_H
#define __PCOMN_DLL_H
/*******************************************************************************
 FILE         :   pcomn_dll.h
 COPYRIGHT    :   Yakov Markovitch, 1995-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   DLL dynamic loader portable interface classes

 CREATION DATE:   09 Sep 1995
*******************************************************************************/
/** @file
  Dynamic library loader classes.
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcomn_except.h>

#if defined(PCOMN_PL_UNIX)
// UNIX
#include <unix/pcomn_dlhandle.h>
namespace pcomn { typedef dlopen_safehandle dll_safehandle ; }

// End of UNIX
#elif defined(PCOMN_PL_WINDOWS)
// Windows
#include <win32/pcomn_w32handle.h>
namespace pcomn { typedef win32dll_safehandle dll_safehandle ; }

// End of Windows
#else
#  error Dll is not supported on this platform
#endif

namespace pcomn {

enum DllLoadFlags {
   DLF_NOLOAD     =  0x0001,
   DLF_ABRTINIT   =  0x0002,
   DLF_RAISEINIT  =  0x0004,
} ;

class Dll ;
template<typename> class PTDllSymbol ;

/*******************************************************************************
 Dynamic library exceptions
*******************************************************************************/
struct _PCOMNEXP dll_error : public virtual environment_error {
      explicit dll_error(const std::string &msg = std::string())
      {
         set_message(msg) ;
      }
} ;

struct _PCOMNEXP dlopen_error : dll_error {
      explicit dlopen_error(const std::string &msg = std::string()) : dll_error(msg) {}
} ;

struct _PCOMNEXP dlsym_error : dll_error {
      explicit dlsym_error(const std::string &msg = std::string()) : dll_error(msg) {}
} ;

/******************************************************************************/
/** Dynamic/shared library pointer
*******************************************************************************/
class Dll {
      PCOMN_NONCOPYABLE(Dll) ;
      PCOMN_NONASSIGNABLE(Dll) ;
   public:
      // Automatically loads DLL (or gets a handle to it)
      // Parameters:
      //    name  -  library name
      explicit Dll(const char *name, unsigned flags = 0) :
         _flags((flags)),
         _handle(load_library(name, _flags)),
         _loaderr(loaded() ? std::string() : lasterr())
      {
         ensure_init() ;
      }

      explicit Dll(const std::string &name, unsigned flags = 0) :
         Dll(name.c_str(), flags) {}

      virtual ~Dll()
      {
         if (_flags & DLF_NOLOAD)
            _handle.release() ;
      }

      bool loaded() const { return !_handle.bad() ; }

      const Dll &ensure_loaded() const
      {
         ensure<dlopen_error>(loaded(), _loaderr) ;
         return *this ;
      }

      /// Get address of an exported symbol by its name
      ///
      /// Returns NULL if the DLL is not loaded or the entry is not found.
      void *unsafe_symbol(const char *name) const
      {
         PCOMN_ENSURE_ARG(name) ;
         return loaded() ? resolve_symbol(name) : NULL ;
      }
      void *unsafe_symbol(const std::string &name) const
      {
         return unsafe_symbol(name.c_str()) ;
      }

      /// Get address of an exported symbol by its name
      ///
      /// Throws exception if the DLL is not loaded or the entry is not found.
      void *safe_symbol(const char *name) const
      {
         void * const symbol = ensure_loaded().unsafe_symbol(name) ;
         if (!symbol)
            throw_exception<dlsym_error>(lasterr()) ;
         return symbol ;
      }
      void *safe_symbol(const std::string &name) const
      {
         return safe_symbol(name.c_str()) ;
      }

      class Symbol ;
      friend class Symbol ;
      /*******************************************************************************
                     class Dll::Symbol
       Class reperesents a pointer to an item imported from DLL
      *******************************************************************************/
      class Symbol {
         public:
            const void *unsafe_data() const { return _data ; }

            const void *safe_data() const
            {
               ensure<dlsym_error>(loaded(), _loaderr) ;
               return unsafe_data() ;
            }

            bool loaded() const { return !!_data ; }

         protected:
            /// Load pointer to symbol from the DLL
            ///
            /// Can substitute default value for the pointer if the symbol is not found
            /// in the DLL.
            /// @param module  the module that is item importing from
            /// @param name    the name of an item being loaded (in exactly the same form
            /// as exported from the DLL)
            /// @param defval
            /// @param flags
            Symbol(const Dll &module, const char *name, void *defval, unsigned flags) :
               _data(module.unsafe_symbol(name)),
               _loaderr(loaded() ? std::string() : module.lasterr())
            {
               if (!loaded())
                  if (!(flags & (DLF_ABRTINIT|DLF_RAISEINIT)))
                     _data = defval ;
                  else
                  {
                     PCOMN_ENSURE(!(flags & DLF_ABRTINIT), _loaderr.c_str()) ;
                     safe_data() ;
                  }
            }

         private:
            const void *      _data ;
            const std::string _loaderr ;
      } ;

   private:
      const unsigned       _flags ;
      dll_safehandle       _handle ;
      const std::string    _loaderr ;

      void ensure_init() const
      {
         if (_flags & (DLF_ABRTINIT|DLF_RAISEINIT))
            try {
               ensure_loaded() ;
            }
            catch (const dll_error &x) {
               PCOMN_ENSURE(!(_flags & DLF_ABRTINIT), x.what()) ;
               throw ;
            }
      }
   private:
      static dll_safehandle::handle_type load_library(const char *name, unsigned flags) ;
      void *resolve_symbol(const char *name) const ;
      std::string lasterr() const ;
} ;


/******************************************************************************/
/** Dynamic library exported symbol
*******************************************************************************/
template<typename T>
class PTDllSymbol<T *> : private Dll::Symbol {
      typedef Dll::Symbol ancestor ;
   public:
      typedef T *element_type ;
      using ancestor::loaded ;

      PTDllSymbol(const Dll &module, const char *name,
                  element_type defval = element_type(), unsigned flags = 0) :
         ancestor(module, name, reinterpret_cast<void *>(defval), flags)
      {}
      PTDllSymbol(const Dll &module, const std::string &name,
                  element_type defval = element_type(), unsigned flags = 0) :
         ancestor(module, name.c_str(), reinterpret_cast<void *>(defval), flags)
      {}

      PTDllSymbol() : ancestor(NULL) {}

      element_type unsafe_data() const { return (element_type)ancestor::unsafe_data() ; }
      element_type safe_data() const { return (element_type)ancestor::safe_data() ; }
      operator element_type() const { return unsafe_data() ; }
      element_type operator*() const { return safe_data() ; }
      element_type operator->() const { return safe_data() ; }
} ;


/*******************************************************************************
 Dll
*******************************************************************************/
#if defined(PCOMN_PL_UNIX)
// UNIX
inline dll_safehandle::handle_type Dll::load_library(const char *name, unsigned flags)
{
   NOXCHECK(name) ;
   return dlopen(name, RTLD_NOW|((flags & DLF_NOLOAD) ? RTLD_NOLOAD : 0)) ;
}

inline void *Dll::resolve_symbol(const char *name) const
{
   NOXCHECK(loaded()) ;
   return dlsym(_handle, name) ;
}

inline std::string Dll::lasterr() const
{
   const char * const err = dlerror() ;
   if (!err)
      return std::string() ;
   return err ;
}
// End of UNIX

#elif defined(PCOMN_PL_WINDOWS)
// Windows
inline dll_safehandle::handle_type Dll::load_library(const char *name, unsigned flags)
{
   ::LoadLibraryEx(name, NULL, flags) ;
}

inline void *Dll::resolve_symbol(const char *name) const
{
   return ::GetProcAddress(ensure_loaded()._handle, name) ;
}

// End of Windows
#endif

} // end of namespace pcomn

#endif /* __PCOMN_DLL_H */
