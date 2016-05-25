/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_BUFFER_H
#define __PCOMN_BUFFER_H
/*******************************************************************************
 FILE         :   pcomn_buffer.h
 COPYRIGHT    :   Yakov Markovitch, 1996-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Raw memory buffers (copy-on-write buffer and always shared
                  buffer).

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   16 Apr 1996
*******************************************************************************/
#include <pcomn_smartptr.h>
#include <pcomn_except.h>
#include <pcomn_integer.h>
#include <pcomn_meta.h>

#include <string.h>
#include <cstdlib>
#include <algorithm>

#ifdef PCOMN_PL_UNIX
#include <sys/uio.h> /* struct iovec */
#else
struct iovec {
      void *   iov_base ;
      size_t   iov_len ;
} ;
#endif

namespace pcomn {

typedef struct iovec iovec_t ;
typedef std::pair<const void *, size_t> cmemvec_t ;
typedef std::pair<void *, size_t>       memvec_t ;

/******************************************************************************/
/** Growable memory buffer
*******************************************************************************/
class basic_buffer {
      PCOMN_NONASSIGNABLE(basic_buffer) ;
      PCOMN_NONCOPYABLE(basic_buffer) ;
   public:
      constexpr basic_buffer() :
         _maxsize((size_t)-1),
         _avail(0),
         _size(0),
         _data(nullptr)
      {}

      explicit basic_buffer(size_t size, size_t maxsize = (size_t)-1) :
         _maxsize(maxsize),
         _avail(size),
         _size(ensure_le<std::invalid_argument>
               (size, _maxsize, "Cannot grow memory buffer beneath maxsize.")),
         _data(ensure_nonzero<std::bad_alloc>(std::malloc(size)))
      {}

      basic_buffer(void *data, size_t datasize) :
         _maxsize(0),
         _avail(datasize),
         _size(datasize),
         _data(data)
      {
         ensure<std::invalid_argument>
            (data || !datasize,
             "NULL buffer with nonzero size passed to basic_buffer constructor.") ;
      }

      basic_buffer(basic_buffer &&other) noexcept : basic_buffer()
      {
         swap(other) ;
      }

      ~basic_buffer()
      {
         if (owns_data())
            std::free(_data) ;
      }

      basic_buffer &operator=(basic_buffer &&other) noexcept
      {
         if (&other != this)
         {
            swap(other) ;
            basic_buffer().swap(other) ;
         }
         return *this ;
      }

      const void *data() const { return _data ; }
      void *data() { return _data ; }

      const void *get() const { return data() ; }
      void *get() { return data() ; }

      size_t size() const { return _size ; }
      size_t maxsize() const { return _maxsize ? _maxsize : _size ; }

      bool owns_data() const { return !!_maxsize ; }

      void reset()
      {
         // Shortcut
         if (!_data)
            return ;
         _size = _avail = 0 ;
         if (_maxsize)
            std::free(_data) ;
         _data = 0 ;
      }

      void *grow(size_t newsize)
      {
         if (newsize > _avail)
         {
            ensure_le<std::invalid_argument>
               (newsize, _maxsize, "Cannot grow memory buffer above maxsize.") ;

            const size_t available = std::min(next_size(newsize), _maxsize) ;
            _data = ensure_nonzero<std::bad_alloc>(std::realloc(_data, available)) ;
            _avail = available ;
         }
         _size = newsize ;
         return _data ;
      }

      void *append(const void *data, size_t sz)
      {
         if (!sz)
            return _data ;
         PCOMN_ENSURE_ARG(data) ;
         const size_t offset = size() ;
         return
            memcpy(pcomn::padd(grow(offset + sz), offset), data, sz) ;
      }

      void swap(basic_buffer &other) noexcept
      {
         std::swap(other._maxsize, _maxsize) ;
         std::swap(other._avail, _avail) ;
         std::swap(other._size, _size) ;
         std::swap(other._data, _data) ;
      }

   private:
      size_t   _maxsize ;
      size_t   _avail ;
      size_t   _size ;
      void *   _data ;

      static size_t next_size(size_t newsize)
      {
         if (newsize & int_traits<size_t>::signbit)
            return int_traits<size_t>::ones ;
         for (size_t nextsize = bitop::clrrnzb(newsize <<= 1U) ; nextsize ; nextsize = bitop::clrrnzb(nextsize))
            newsize = nextsize ;
         return newsize ;
      }
} ;

/******************************************************************************/
/** Reference-counted growable buffer.
*******************************************************************************/
class shared_buffer {
   public:
      explicit shared_buffer(size_t sz = 0) : _buffer(new (sz) buf(sz)) {}

      shared_buffer(const void *srcdata, size_t sz) : _buffer(new (sz) buf(sz))
      {
         memcpy(data(), srcdata, sz) ;
      }

      shared_buffer(const shared_buffer &) = default ;
      shared_buffer(shared_buffer &&) = default ;

      shared_buffer &operator=(const shared_buffer &) = default ;
      shared_buffer &operator=(shared_buffer &&) = default ;

      /// Get buffer size.
      size_t size() const { return _buffer->_size ; }

      /// Check whether the buffer has zero size.
      bool empty() const { return !size() ; }

      const void *data() const { return _buffer->_data ; }
      void *data() { return _buffer->_data ; }

      /// Get the pointer to buffer's memory.
      void *get() { return _buffer->_data ; }
      /// Get the pointer to buffer's memory.
      const void *get() const { return _buffer->_data ; }

      /// Get the pointer to buffer's memory.
      operator const void *() const { return data() ; }
      operator void *() { return data() ; }

      void *resize(size_t size)
      {
         // Increment refcount to ensure the buffer will not be deleted
         buf *buffer = static_cast<buf *>(inc_ref(_buffer.get())) ;
         _buffer = 0 ;
         _buffer = buffer->_realloc(size) ;
         dec_ref(_buffer.get()) ;
         return data() ;
      }

      void swap(shared_buffer &other) { other._buffer.swap(_buffer) ; }

   private:
      struct buf : PRefCount {

            buf(size_t sz) : _size(sz), _data(sz ? this + 1 : NULL) {}

            static void *operator new(size_t sz, size_t asz)
            {
               NOXCHECK(sz+asz >= sz) ;
               return ensure_nonzero<std::bad_alloc>(std::malloc(sz+asz)) ;
            }
            static void operator delete(void *p, size_t) { std::free(p) ; }

            buf *_realloc(size_t size)
            {
               _size = size ;
               buf *self = static_cast<buf *>(std::realloc(this, size + sizeof *this)) ;
               self->_data = self + 1 ;
               return self ;
            }

            size_t _size ;
            void * _data ;

         private:
            // Declare this to become the "usual deallocation function" for this class,
            // to avoid interpreting operator delete(void *, size_t) as such "usual
            // deallocation function"
            static void operator delete(void *p) { std::free(p) ; }
      } ;

   private:
      shared_intrusive_ptr<buf> _buffer ;
} ;

/******************************************************************************/
/** Reference-counted copy-on-write fixed-size buffer
*******************************************************************************/
class cow_buffer {
   public:
      /// Constructor creates buffer of given size.
      ///
      /// cow_buffer uses 'lasy memory allocation', hence right after creation the buffer
      /// is empty: empty() for such buffer returns true even though size() is not zero.
      /// Memory is allocated after first call of non-constant get() (or, the same,
      /// non-constant operator void *)
      explicit constexpr cow_buffer(size_t sz = 0) noexcept : _size(sz) {}

      cow_buffer(const void *data, size_t sz) : _size(sz)
      {
         memcpy(get(), data, sz) ;
      }

      cow_buffer(const cow_buffer &) = default ;

      cow_buffer(cow_buffer &&src) :
         _size(src._size),
         _buffer(std::move(src._buffer))
      {
         src._size = 0 ;
      }

      cow_buffer &operator=(const cow_buffer &) = default ;
      cow_buffer &operator=(cow_buffer &&src)
      {
         cow_buffer(std::move(src)).swap(*this) ;
         return *this ;
      }

      /// Get buffer size (that has been specified in constructor call)
      size_t size() const { return _size ; }

      /// Check whether the buffer has zero size (or was never written)
      bool empty() const { return !_buffer ; }

      /// Get pointer to buffer data.
      void *get() { return cow() ; }

      /// Get pointer to buffer data.
      /// If the buffer has never been written to (non-const get is not yet called) or
      /// size() == 0, returns NULL.
      const void *get () const { return !_buffer ? nullptr : _buffer->data ; }

      /// The same as get() const
      operator const void *() const { return get() ; }

      /// Remove buffer contents (free buffer's memory)
      void reset() { _buffer.reset() ; }

      void swap(cow_buffer &other) noexcept
      {
         std::swap(other._size, _size) ;
         other._buffer.swap(_buffer) ;
      }

   private:
      struct buf : public PRefCount {
            // Declare this to become the "usual deallocation function" for this class,
            // to avoid interpreting operator delete(void *, size_t) as such "usual
            // deallocation function". Note this function is not implemented, only declared.
            static void operator delete(void *p) { ::operator delete (p) ; }

            static void *operator new (size_t sz, size_t data_sz)
            {
               return ::operator new (sz + data_sz/sizeof(uintptr_t)*sizeof(uintptr_t)) ;
            }
            static void operator delete (void *p, size_t) { ::operator delete (p) ; }

            uintptr_t data[1] ;
      } ;

      typedef shared_intrusive_ptr<buf> data_pointer ;

      void *cow()
      {
         if (!_size)
            return nullptr ;

         if (_buffer.instances() != 1)
         {
            data_pointer new_data {new (_size) buf} ;
            if (void * const data = !_buffer ? nullptr : _buffer->data)
               memcpy(new_data->data, data, _size) ;
            _buffer.swap(new_data) ;
         }
         return _buffer->data ;
      }

   private:
      size_t         _size ;
      data_pointer   _buffer ;
} ;

/******************************************************************************/
/** A 'typed' buffer.
*******************************************************************************/
template<typename T, class Buffer>
class typed_buffer : private Buffer {
   public:
      using Buffer::size ;
      using Buffer::empty ;
      using Buffer::operator const void * ;

      // Creates a buffer of size param*sizeof (T)
      explicit typed_buffer(size_t nitems = 0) : Buffer(nitems*sizeof(T)) {}

      typed_buffer(const typed_buffer &) = default ;
      typed_buffer(typed_buffer &&) = default ;

      typed_buffer &operator=(const typed_buffer &) = default ;
      typed_buffer &operator=(typed_buffer &&) = default ;

      size_t nitems() const { return this->size()/sizeof(T) ; }

      const T *get() const { return static_cast<const T *> (Buffer::get()) ; }
      T *get() { return static_cast<T *> (Buffer::get()) ; }

      const T *data() const { return get() ; }
      T *data() { return get() ; }

      operator const T *() const { return get() ; }
      const T &operator[] (int ndx) const { return *(get()+ndx) ; }
      T &operator[] (int ndx) { return *(get()+ndx) ; }

      void swap(typed_buffer &other) { Buffer::swap(other) ; }
} ;

/*******************************************************************************
 swap overloads for various buffer classes
*******************************************************************************/
PCOMN_DEFINE_SWAP(cow_buffer) ;
PCOMN_DEFINE_SWAP(shared_buffer) ;
PCOMN_DEFINE_SWAP(basic_buffer) ;
PCOMN_DEFINE_SWAP(typed_buffer<P_PASS(T, B)>, template<typename T, class B>) ;

/*******************************************************************************

*******************************************************************************/
inline iovec_t make_iovec(const void *base = NULL, size_t len = 0)
{
    iovec_t result ;
    result.iov_base = const_cast<void *>(base) ;
    result.iov_len = len ;
    return result ;
}

template<typename T> struct membuf_traits { typedef char undefined ; } ;

/// @cond
namespace detail {
template<typename T>
static typename membuf_traits<T>::undefined _is_buffer(T**) ;
template<typename T>
static typename membuf_traits<T>::type *_is_buffer(T**) ;

template<typename T>
struct _bufelem_size : std::integral_constant<size_t, sizeof(T)> {} ;
template<>
struct _bufelem_size<void> : std::integral_constant<size_t, 1> {} ;
} // end of namespace pcomn::detail
/// @endcond

template<typename T> using
is_buffer = bool_constant<sizeof detail::_is_buffer(autoval<T**>()) == sizeof(void *)> ;

template<typename T, typename Type> struct
enable_if_buffer : std::enable_if<is_buffer<T>::value, Type> {} ;

template<typename T, typename Type> struct
disable_if_buffer : std::enable_if<!is_buffer<T>::value, Type> {} ;

template<typename T, typename Type>
using enable_if_buffer_t = typename enable_if_buffer<T, Type>::type ;

template<typename T, typename Type>
using disable_if_buffer_t = typename disable_if_buffer<T, Type>::type ;

/*******************************************************************************
                     template<typename C>
                     struct pbuf_traits
*******************************************************************************/
template<typename T>
struct pbuf_traits {
      typedef T type ;

      static size_t size(const T &buffer) { return buffer.size() ; }
      static const void *cdata(const T &buffer) { return buffer.get() ; }
      static void *data(T &buffer) { return buffer.get() ; }
} ;

template<> struct membuf_traits<shared_buffer>   : pbuf_traits<shared_buffer> {} ;
template<> struct membuf_traits<cow_buffer>   : pbuf_traits<cow_buffer> {} ;
template<> struct membuf_traits<basic_buffer> : pbuf_traits<basic_buffer> {} ;

template<typename, bool>
struct memvec_traits : membuf_traits<void> {} ;

template<typename E>
struct memvec_traits<std::pair<E *, size_t>, true> {
      typedef std::pair<E *, size_t> type ;

      static constexpr size_t size(const std::pair<E *, size_t> &buffer)
      {
         return buffer.second * detail::_bufelem_size<E>::value ;
      }
      static constexpr const void *cdata(const std::pair<E *, size_t> &buffer)
      {
         return buffer.first ;
      }
      template<typename = std::enable_if<!std::is_const<E>::value>>
      static constexpr void *data(const std::pair<E *, size_t> &buffer)
      {
         return buffer.first ;
      }
} ;

template<typename E>
struct membuf_traits<std::pair<E *, size_t>> :
         memvec_traits<std::pair<E *, size_t>, std::is_pod<E>::value>
{} ;

template<> struct membuf_traits<iovec_t> {
      typedef iovec_t type ;

      static size_t size(const iovec_t &buffer) { return buffer.iov_len ; }
      static const void *cdata(const iovec_t &buffer) { return buffer.iov_base ; }
      static void *data(iovec_t &buffer) { return buffer.iov_base ; }
} ;

template<size_t n> struct membuf_traits<const char[n]> {
      typedef const void *type ;

      static size_t size(const char(&)[n]) { return n ; }
      static const void *cdata(const char(&buf)[n]) { return buf + 0 ; }
} ;

template<size_t n> struct membuf_traits<char[n]> : membuf_traits<const char[n]> {
      typedef void *type ;
      static void *data(const char(&buf)[n]) { return buf + 0 ; }
} ;

/*******************************************************************************
 pcomn::buf
*******************************************************************************/
namespace buf {

template<typename T>
inline const void *cdata(const T &buffer)
{
   return ::pcomn::membuf_traits<T>::cdata(buffer) ;
}

template<typename T>
inline void *data(T &buffer)
{
   return ::pcomn::membuf_traits<typename std::remove_const<T>::type>::data(buffer) ;
}

template<typename T>
inline size_t size(const T &buffer) { return ::pcomn::membuf_traits<T>::size(buffer) ; }

template<typename T1, typename T2>
inline bool eq(const T1 &left, const T2 &right)
{
   const size_t sz = ::pcomn::buf::size(left) ;
   if (sz != ::pcomn::buf::size(right))
      return false ;
   const void * const lp = ::pcomn::buf::cdata(left) ;
   const void * const rp = ::pcomn::buf::cdata(right) ;
   return lp == rp || !memcmp(lp, rp, sz) ;
}

template<typename T>
inline cmemvec_t cmemvec(const T &buffer)
{
   const void * const data = ::pcomn::buf::cdata(buffer) ;
   return cmemvec_t(data, ::pcomn::buf::size(buffer)) ;
}

template<typename T>
inline cmemvec_t cmemvec(const T &buffer, size_t offs, size_t len = -1)
{
   const void * const data = ::pcomn::buf::cdata(buffer) ;
   const size_t bufsize = ::pcomn::buf::size(buffer) ;
   const size_t bufoffs = std::min(bufsize, offs) ;
   return cmemvec_t(pcomn::padd(data, bufoffs), std::min(bufsize - bufoffs, len)) ;
}

template<typename T>
inline memvec_t memvec(T &buffer)
{
   void * const data = ::pcomn::buf::data(buffer) ;
   return memvec_t(data, ::pcomn::buf::size(buffer)) ;
}

template<typename T>
inline memvec_t memvec(T &buffer, size_t offs, size_t len = -1)
{
   void * const data = ::pcomn::buf::data(buffer) ;
   const size_t bufsize = ::pcomn::buf::size(buffer) ;
   const size_t bufoffs = std::min(bufsize, offs) ;
   return memvec_t(pcomn::padd(data, bufoffs), std::min(bufsize - bufoffs, len)) ;
}

} // end of namespace pcomn::buf
} // end of namespace pcomn

inline std::ostream &operator<<(std::ostream &os, const pcomn::iovec_t &v)
{
   return os << pcomn::buf::cmemvec(v) ;
}

#endif /* __PCOMN_BUFFER_H */
