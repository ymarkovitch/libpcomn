/*-*- mode:c++; tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_HANDLE_H
#define __PCOMN_HANDLE_H
/*******************************************************************************
 FILE         :   pcomn_handle.h
 COPYRIGHT    :   Yakov Markovitch, 2000-2018. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Handle processing utility classes and functions

 CREATION DATE:   12 Dec 2000
*******************************************************************************/
#include <pcommon.h>
#include <pcomn_atomic.h>
#include <pcomn_meta.h>
#include <pcomn_unistd.h>

#include <stdio.h>

namespace pcomn {

/******************************************************************************/
/** Traits, describing any "handle".
    The handle is a POD object - a representative of some complex object, i.e.
    a handle is a kind of opaque reference. File descriptor in unistd API
    (open/close/read/write, etc.), Windows HANDLE - all are handles.
    There are two operations possible on every handle: every handle can be
    tested for validity, and every valid handle can be closed.
*******************************************************************************/
template<typename HandleTag>
struct handle_traits {
      typedef typename HandleTag::handle_type handle_type ;
      static bool close(handle_type h) ;
      static bool is_valid(handle_type h) ;
      static constexpr handle_type invalid_handle() ;
} ;

/*******************************************************************************
 _set_handle
*******************************************************************************/
template<typename Handle>
struct _set_handle {
      static Handle exec(Handle &storage, Handle newval)
      {
         Handle old = storage ;
         storage = newval ;
         return old ;
      }
} ;

template<typename Handle>
struct _atomic_set_handle {
      static Handle exec(Handle &storage, Handle newval)
      {
         return atomic_op::xchg(&storage, newval) ;
      }
} ;

template<typename T> struct _set_handle<T *> : _atomic_set_handle<T *> {} ;
template<> struct _set_handle<int>           : _atomic_set_handle<int> {} ;
template<> struct _set_handle<unsigned>      : _atomic_set_handle<unsigned> {} ;
template<> struct _set_handle<long>          : _atomic_set_handle<long> {} ;
template<> struct _set_handle<unsigned long> : _atomic_set_handle<unsigned long> {} ;
template<> struct _set_handle<long long>     : _atomic_set_handle<long long> {} ;
template<> struct _set_handle<unsigned long long> : _atomic_set_handle<unsigned long long> {} ;

/******************************************************************************/
/** POSIX I/O file descriptor handle traits.
*******************************************************************************/
struct fd_handle_tag {
      typedef int handle_type ;
} ;

template<>
inline bool handle_traits<fd_handle_tag>::close(int h)
{
   return ::close(h) == 0 ;
}
template<>
inline bool handle_traits<fd_handle_tag>::is_valid(int h)
{
   return h >= 0 ;
}
template<>
inline constexpr int handle_traits<fd_handle_tag>::invalid_handle()
{
   return -1 ;
}

/******************************************************************************/
/** Standard C I/O FILE * handle traits.
*******************************************************************************/
struct FILE_handle_tag { typedef FILE * handle_type ; } ;

template<>
inline bool handle_traits<FILE_handle_tag>::close(FILE *f)
{
   return fclose(f) == 0 ;
}
template<>
inline bool handle_traits<FILE_handle_tag>::is_valid(FILE *f)
{
   return !!f ;
}
template<>
inline constexpr FILE *handle_traits<FILE_handle_tag>::invalid_handle()
{
   return NULL ;
}

/******************************************************************************/
/** dirent's DIR descriptor traits.
*******************************************************************************/
struct dir_handle_tag {
      typedef DIR * handle_type ;
} ;

template<>
inline bool handle_traits<dir_handle_tag>::close(DIR *dir)
{
   return ::closedir(dir) == 0 ;
}
template<>
inline bool handle_traits<dir_handle_tag>::is_valid(DIR *dir)
{
   return !!dir ;
}
template<>
inline constexpr DIR *handle_traits<dir_handle_tag>::invalid_handle()
{
   return NULL ;
}

/******************************************************************************/
/** Base for all "handle" classes.
*******************************************************************************/
template<typename Handle>
struct _handle_holder {
      /// Get the underlying handle.
      constexpr Handle handle() const { return _handle ; }

      /// Same as _handle_holder::handle, for compliancy with STL smart/auto pointers.
      constexpr Handle get() const { return _handle ; }

      constexpr operator Handle() const { return handle() ; }

   protected:
      explicit constexpr _handle_holder(Handle h) : _handle(h) {}

      Handle set_handle(Handle newhandle)
      {
         return _set_handle<Handle>::exec(_handle, newhandle) ;
      }

   private:
      Handle _handle ;
} ;

template<typename Handle>
struct handle_holder : _handle_holder<Handle> {
   protected:
      explicit constexpr handle_holder(Handle h) : _handle_holder<Handle>(h) {}
} ;

template<typename T>
struct handle_holder<T *> : _handle_holder<T *> {

      constexpr T *operator->() const { return this->handle() ; }
      constexpr T &operator*() const { return *(this->handle()) ; }

   protected:
      explicit constexpr handle_holder(T * h) : _handle_holder<T *>(h) {}
} ;

template<>
struct handle_holder<void *> : _handle_holder<void *> {
   protected:
      explicit constexpr handle_holder(void * h) : _handle_holder<void *>(h) {}
} ;

template<>
struct handle_holder<const void *> : _handle_holder<const void *> {
   protected:
      explicit constexpr handle_holder(const void * h) : _handle_holder<const void *>(h) {}
} ;

/******************************************************************************/
/** Safe handle: handle wrapper, closes its contained "actual" handle according to
 corresponding handle traits.
*******************************************************************************/
template<typename HandleTag, class HandleTraits = pcomn::handle_traits<HandleTag> >
class safe_handle : public handle_holder<typename HandleTraits::handle_type> {
      typedef handle_holder<typename HandleTraits::handle_type> ancestor ;
      // safe_handle is neither copiable nor assignable
      PCOMN_NONCOPYABLE(safe_handle) ;
      PCOMN_NONASSIGNABLE(safe_handle) ;

   public:
      typedef HandleTraits                         traits_type ;
      typedef typename HandleTraits::handle_type   handle_type ;

      /// Default constructor creates a safe handle initialized with an invalid
      /// handle value.
      /// The invalid handle value is provided by the handle_traits::invalid_handle.
      constexpr safe_handle() : ancestor(HandleTraits::invalid_handle()) {}
      safe_handle(safe_handle &&other) : safe_handle() { swap(other) ; }

      explicit safe_handle(handle_type h) : ancestor(h) {}

      /// Destructor closes the handle if it is valid.
      /// If the handle is invalid, does nothing.
      ~safe_handle() { _close(this->handle()) ; }

      handle_type release() throw()
      {
         return this->set_handle(HandleTraits::invalid_handle()) ;
      }

      bool close() throw() { return _close(release()) ; }

      safe_handle &reset(handle_type h = HandleTraits::invalid_handle())
      {
         if (h != this->handle())
            _close(this->set_handle(h)) ;
         return *this ;
      }

      safe_handle &operator=(handle_type h) { return reset(h) ; }

      void swap(safe_handle &other)
      {
         other.set_handle(this->set_handle(other.handle())) ;
      }

      friend void swap(safe_handle &left, safe_handle &right) { left.swap(right) ; }

      bool bad() const { return !HandleTraits::is_valid(this->handle()) ; }
      bool good() const { return !bad() ; }

   private:
      static bool _close(handle_type h) throw()
      {
         return HandleTraits::is_valid(h) && HandleTraits::close(h) ;
      }
} ;

/*******************************************************************************
 Prederined safe handle typedefs
*******************************************************************************/
/// A safe file handle.
/// The underlying handle is a unistd file descriptor (which is returned by open()).
typedef safe_handle<fd_handle_tag> fd_safehandle ;

/// A safe directory handle.
typedef safe_handle<dir_handle_tag> dir_safehandle ;

/// A safe FILE * handle.
typedef safe_handle<FILE_handle_tag> FILE_safehandle ;

/******************************************************************************/
/** Wrapper around integral or pointer handle,

 @note As the 'Tag' template argument is used only as a tag, the type needn't be
 complete, i.e. it is enough if it is only declared but not defined.
*******************************************************************************/
template<typename Handle, class Tag>
struct ihandle {
      typedef Handle type ;
      typedef Handle handle_type ;

      union {
            handle_type raw ; /**< Raw handle value */

            typename std::aligned_storage
            <sizeof(handle_type), sizeof(handle_type)>::type _ ; /* Alignment */
      } ;

      /// Create a handle initialized with an invalid handle value.
      ///
      /// By default, initial value is 0; it is possible to overload this constructor.
      /// @note This constructor is deliberately made 'explicit' to avoid unintended
      /// conversions from integer to the handle type.
      explicit ihandle(handle_type h = type()) : raw(h) {}

      handle_type value() const { return raw ; }

      operator handle_type() const { return value() ; }
} ;

template<typename Handle, class Tag>
inline bool operator==(const ihandle<Handle, Tag> &lhs, const ihandle<Handle, Tag> &rhs)
{
   return lhs.value() == rhs.value() ;
}

template<typename Handle, class Tag>
inline bool operator<(const ihandle<Handle, Tag> &lhs, const ihandle<Handle, Tag> &rhs)
{
   return lhs.value() < rhs.value() ;
}

// Define "derived" operators (!=, >, etc.)
PCOMN_DEFINE_RELOP_FUNCTIONS(P_PASS(template<typename Handle, class Tag>),
                             P_PASS(ihandle<Handle, Tag>)) ;

} // end of namespace pcomn

#endif /* __PCOMN_HANDLE_H */
