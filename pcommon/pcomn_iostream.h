/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_IOSTREAM_H
#define __PCOMN_IOSTREAM_H
/*******************************************************************************
 FILE         :   pcomn_iostream.h
 COPYRIGHT    :   Yakov Markovitch, 2007-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Simple binary I/O streams.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   10 Oct 2007
*******************************************************************************/
/** @file
    Simple binary I/O streams. even simpler than pcomn::raw_ios.

 PCommon streams provide "ranges" (http://rangelib.synesis.com.au/). If you want to use
 pcommon iostreams with STLSoft rangelib, include pcommon_iostream.h somewhere after
 pcomn_range.h in include sequence. It can safely be included both before pcomn_range.h
 and after in the same include sequence; the only requirement is that it is
 included after pcomn_range.h at least once.

 Also,
*******************************************************************************/
#include <pcommon.h>
#include <pcomn_assert.h>
#include <pcomn_except.h>
#include <pcomn_string.h>
#include <pcomn_safeptr.h>
#include <pcomn_range.h>
#include <pcomn_simplematrix.h>

#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <memory>
#include <limits>

#include <string.h>

namespace pcomn {

/******************************************************************************/
/** Exception class; thrown by binary_istream::read or binary_istream::get to report
 end-of-file condition, when binary_istream::throw_eof is set to true.
 @note By default, throw_eof is false.
*******************************************************************************/
class _PCOMNEXP eof_error : public std::runtime_error {
      typedef std::runtime_error ancestor ;
   public:
      explicit eof_error(const std::string &message) :
         ancestor(message)
      {}
} ;

/******************************************************************************/
/** An interface to a simple binary input stream.
*******************************************************************************/
class _PCOMNEXP binary_istream {
   public:
      /// Typedef to ensure minimal structure compatiblity with std::istream.
      /// Several template facilities use this dependent typedef, particularly pcomn::istream_range.
      typedef char char_type ;

      /// Empty virtual destructor.
      virtual ~binary_istream() {}

      bool eof() const { return _eof ; }

      /// Indicate whether the stream throws eof_error on end-of-file.
      bool throw_eof() const { return _throw_eof ; }

      /// Set whether the stream throws eof_error on end-of-file.
      /// @return Previous throw_eof state.
      /// @note Doesn't change current eof state. Calling throw_eof(true) for a
      /// stream with eof()==true doesn't throw exception.
      bool throw_eof(bool value)
      {
         bool old = _throw_eof ;
         _throw_eof = value ;
         return old ;
      }

      /// Read the next character from stream.
      /// @return The character that was read as an unsigned char cast to an int, or EOF
      /// on end of file.
      /// @throw Everything the read_data implementation throws. If throw_eof is true,
      /// throws eof_error on end-of-file (throw_eof is false by default).
      int get()
      {
         unsigned char result = 0 ;
         return read(&result, 1) ? result : EOF ;
      }

      /// Attempt to read size bytes from stream into a memory buffer.
      /// @throw Everything the read_data implementation throws. If throw_eof is true,
      /// throws eof_error on end-of-file (throw_eof is false by default).
      size_t read(void *buf, size_t size)
      {
         NOXVERIFY(set_readcount(read_data(PCOMN_ENSURE_ARG(buf), size), size) <= size) ;
         return _last_read ;
      }

      template<size_t n>
      size_t read(char (&buf)[n]) { return read(buf, n) ; }

      /// Read stream until the end-of-file and return data as std::string.
      std::string read() ;

      /// Get the count of bytes read by the last operation.
      size_t last_read() const { return _last_read ; }

      /// Get the total count of bytes read from the stream so far.
      size_t total_read() const { return _total_read ; }

   protected:
      explicit binary_istream(bool throw_eof = false) :
         _total_read(0),
         _last_read(0),
         _throw_eof(throw_eof),
         _eof(false)
      {}

      /// Should read data into a buffer.
      virtual size_t read_data(void *buf, size_t size) = 0 ;

      size_t set_readcount(ssize_t readcount, size_t requested_count)
      {
         _last_read = std::max<ssize_t>(readcount, 0) ;
         _total_read += readcount ;
         _eof = !_last_read && requested_count ;
         conditional_throw<eof_error>(_eof && throw_eof(), "Unexpected end of stream") ;
         return readcount ;
      }

   private:
      size_t   _total_read ;
      size_t   _last_read ;
      bool     _throw_eof ; /* Throw eof_error on EOF */
      bool     _eof ;
} ;

/******************************************************************************/
/** Guard class that allow temporary change throw_eof behaviour.
*******************************************************************************/
struct eof_guard {
      explicit eof_guard(binary_istream &stream, bool throw_eof) :
         _stream(stream),
         _throw_eof(stream.throw_eof(throw_eof))
      {}
      ~eof_guard()
      {
         _stream.throw_eof(_throw_eof) ;
      }

      binary_istream &stream() { return _stream ; }
   private:
      binary_istream &  _stream ;
      const bool        _throw_eof ;
} ;

/******************************************************************************/
/** An interface to a simple binary output stream.
*******************************************************************************/
class _PCOMNEXP binary_ostream {
   public:
      /// Empty virtual destructor.
      virtual ~binary_ostream() {}

      virtual void flush() {}

      size_t write(const void *data, size_t size)
      {
         return write_data(PCOMN_ENSURE_ARG(data), size) ;
      }

      template<typename S>
      typename enable_if_strchar<S, char, size_t>::type
      write(const S &s)
      {
         return write(str::cstr(s), str::len(s)) ;
      }

      binary_ostream &put(const char c)
      {
         NOXVERIFY(write(&c, 1) == 1) ;
         return *this ;
      }

   protected:
      virtual size_t write_data(const void *data, size_t size) = 0 ;
} ;

/******************************************************************************/
/** Abstract I/O stream interface: provides both binary_istream and binary_ostream.
*******************************************************************************/
class _PCOMNEXP binary_iostream :
         public virtual binary_istream,
         public virtual binary_ostream {
} ;

/******************************************************************************/
/** Buffered wrapper over binary_istream that provides binary_istream interface as well.
*******************************************************************************/
class _PCOMNEXP binary_ibufstream : public virtual binary_istream {
      typedef binary_istream ancestor ;
      PCOMN_NONCOPYABLE(binary_ibufstream) ;
      PCOMN_NONASSIGNABLE(binary_ibufstream) ;
   public:
      /// Create unowning buffered input stream over abstract binary istream; the
      /// resulting buffered input stream does @b not own the underlying istream.
      explicit binary_ibufstream(binary_istream &stream_ref, size_t capacity) ;

      /// Create owning buffered input stream over abstract binary istream; the resulting
      /// buffered input stream @b does own the underlying istream.
      explicit binary_ibufstream(binary_istream *stream_ptr, size_t capacity) ;

      ~binary_ibufstream() ;

      /// Reset the buffer, "flush" all the buffer data.
      /// Flushing means to simply ignore all the data already in the buffer.
      void flush() { _bufptr = bufend() ; }

      /// Get contents of initial part of the buffer.
      /// This is a debugging method. It never attempts to read from the stream or
      /// otherwise change the stream state.
      std::string debug_buffer(size_t headsize = 64) const
      {
         return std::string((const char *)_bufptr, (const char *)_bufptr + std::min(available_buffered(), headsize)) ;
      }

      /// Read the next character from stream and returns it as an unsigned char cast to
      /// an int, or EOF on end of file.
      int get()
      {
         return set_readcount(std::min<size_t>(1, ensure_buffer()), 1)
            ? *_bufptr++
            : EOF ;
      }

      /// Read and return the next character without extracting it, i.e. leaving it as the
      /// next character to be extracted from the stream.
      /// @return Exctracted character or EOF.
      /// @note This method doesn't change eof state and is not affected by throw_on_eof.
      int peek() { return ensure_buffer() ? *_bufptr : EOF ; }

      /// Push a character back to stream, cast to unsigned char, where it is available
      /// for subsequent read operations.
      /// Pushed-back characters are returned in reverse order; only one pushback is
      /// guaranteed.
      void putback(const char c)
      {
         PCOMN_THROW_MSG_IF(_bufptr == _buffer,
                            std::logic_error, "Attempt to pushback a character into a full buffer") ;
         NOXCHECK(_bufptr > _buffer) ;
         set_readcount(-1, 0) ;
         *--_bufptr = c ;
      }

      /// Get the count of data bytes already in the buffer.
      ///
      /// It is guaranteed that one can read up to this count of bytes without actually
      /// reading data from the underlying stream.
      size_t available_buffered() const { return bufend() - _bufptr ; }

      virtual bool is_data_available() { return available_buffered() != 0 ; }

      /// Get reference to underlying unbuffered stream.
      const binary_istream &unbuffered_stream() const { return _unbuffered ; }
      /// Get reference to underlying unbuffered stream.
      binary_istream &unbuffered_stream() { return _unbuffered ; }

      /// Get buffer capacity.
      /// The buffer capacity @em can be 0, but even then implementation provides place
      /// for at least one putback.
      size_t capacity() const { return _capacity ; }

      size_t set_bound(size_t bound) { return xchange(_databound, bound) ; }

   protected:
      /// Implements abstract function of binary_istream.
      size_t read_data(void *data, size_t size) ;
   private:
      safe_ref<binary_istream>   _unbuffered ;
      const size_t               _capacity ;
      size_t                     _databound ;
      uint8_t * const            _buffer ;
      uint8_t *                  _bufptr ;
      uint8_t *                  _bufend ;
      uint8_t                    _putback[2] ;

      uint8_t *bufstart() const
      {
         // Provide one extra character for putback
         return _buffer + 1 ;
      }

      uint8_t *bufend() const { return _bufend ; }

      size_t get_available_frombuf(void *data, size_t size)
      {
         // Get all available data from our internal buffer into the user buffer
         const size_t frombuf = std::min(available_buffered(), size) ;
         memcpy(data, _bufptr, frombuf) ;
         _bufptr += frombuf ;
         return frombuf ;
      }

      size_t ensure_buffer()
      {
         // Check for underflow
         const size_t available = available_buffered() ;
         return available ? available : refill_buffer() ;
      }

      size_t min_unbuffered_size() const { return capacity() / 4 ; }

      size_t bounded_size(size_t size) const
      {
         return _databound == (size_t)-1 ? size : std::min(size, _databound - total_read()) ;
      }

      size_t refill_buffer() ;
} ;

/******************************************************************************/
/** Buffered wrapper over binary_ostream that provides binary_ostream interface as well.
*******************************************************************************/
class _PCOMNEXP binary_obufstream : public virtual binary_ostream {
      typedef binary_ostream ancestor ;
   public:
      /// Create @b unowning buffered output stream over abstract binary ostream; the
      /// resulting buffered output stream does @b not own the underlying ostream.
      explicit binary_obufstream(binary_ostream &stream_ref, size_t capacity) ;

      /// Create @b owning buffered output stream over abstract binary ostream; the
      /// resulting buffered output stream @b does own the underlying ostream.
      explicit binary_obufstream(binary_ostream *stream_ptr, size_t capacity) ;

      /// Destructor attempts to flush data and intercepts all exceptions, since flush can
      /// throw an exception (or rather, its underlying unbufferd stream's write_data can).
      ~binary_obufstream() ;

      /// Get buffer capacity.
      /// @note The buffer capacity is @em never 0.
      size_t capacity() const { return _buffer.size() ; }

      binary_obufstream &put(const char c)
      {
         if (!available_capacity())
            flush_buffer() ;
         *_bufptr++ = c ;
         return *this ;
      }

      void flush()
      {
         flush_buffer() ;
         unbuffered_stream().flush() ;
      }

      /// Get reference to the underlying unbuffered stream.
      const binary_ostream &unbuffered_stream() const { return _unbuffered ; }
      /// @overload
      binary_ostream &unbuffered_stream() { return _unbuffered ; }

   protected:
      // Implement a virtual function which is abstract in the base class
      size_t write_data(const void *data, size_t size) ;

   private:
      safe_ref<binary_ostream>   _unbuffered ;
      simple_vector<uint8_t>     _buffer ;
      uint8_t *                  _bufptr ;

      uint8_t *bufstart() { return _buffer.begin() ; }
      uint8_t *bufend() { return _buffer.end() ; }
      size_t available_capacity() const { return _buffer.end() - _bufptr ; }
      void flush_buffer() ;
} ;

/******************************************************************************/
/** Delegating input stream.
*******************************************************************************/
class delegating_istream : public virtual binary_istream {
      typedef binary_istream ancestor ;
      PCOMN_NONCOPYABLE(delegating_istream) ;
      PCOMN_NONASSIGNABLE(delegating_istream) ;
   public:
      /// Create unowning delegating input stream over an abstract istream; the underlying
      /// stream is not deleted upon destruction of the delegating stream.
      explicit delegating_istream(binary_istream &stream_ref) :
         _istream(&stream_ref)
      {}

      /// Create owning delegating input stream over abstract istream; the underlying
      /// stream @em is deleted upon destruction of the delegating stream.
      /// @param stream_ptr Pointer to a stream to which all calls are delegeted.
      /// @throw std::invalid_argument @a stream_ptr is NULL.
      explicit delegating_istream(binary_istream *stream_ptr) :
         _isowner(PCOMN_ENSURE_ARG(stream_ptr)),
         _istream(stream_ptr)
      {}

      ~delegating_istream()
      {
         _istream = NULL ;
      }

      /// Set new unowned underlying stream.
      delegating_istream &reset(binary_istream &stream_ref)
      {
         if (!ensure_underlying(&stream_ref))
            return *this ;
         _isowner.reset(NULL) ;
         _istream = &stream_ref ;
         ensure_eofstate() ;
         return *this ;
      }

      /// Set new owned underlying stream.
      delegating_istream &reset(binary_istream *stream_ptr)
      {
         if (!ensure_underlying(PCOMN_ENSURE_ARG(stream_ptr)))
            return *this ;
         _isowner.reset(stream_ptr) ;
         _istream = stream_ptr ;
         ensure_eofstate() ;
         return *this ;
      }

      /// Get reference to underlying stream.
      binary_istream &get_istream() { return *_istream ; }
      /// Get reference to underlying stream.
      const binary_istream &get_istream() const { return *_istream ; }

   protected:
      // Implements abstract function of binary_istream
      size_t read_data(void *data, size_t size)
      {
         return get_istream().read(data, size) ;
      }

   private:
      PTSafePtr<binary_istream>  _isowner ;
      binary_istream *           _istream ;

      bool ensure_underlying(binary_istream *stream)
      {
         if (stream == _istream)
            return false ;
         // Prevent from self-delegation
         PCOMN_THROW_MSG_IF(stream == this, std::invalid_argument, "Self-reference in delegating stream.") ;
         return true ;
      }

      void ensure_eofstate()
      {
         if (eof() && !get_istream().eof())
            set_readcount(0, 0) ;
      }
} ;

/******************************************************************************/
/** Binary input stream over an input iterator.

 InputIterator::value_type should be convertible to char.

 @note istream_over_iterator is neither CopyConstructible nor Assignable, because
 copying InputIterator doesn't create independent copy in general case: imagine, for
 instance, that InputIterator is std::istream_iterator.
*******************************************************************************/
template<typename InputIterator>
class istream_over_iterator : public virtual binary_istream {
      PCOMN_NONCOPYABLE(istream_over_iterator) ;
      PCOMN_NONASSIGNABLE(istream_over_iterator) ;
   public:
      istream_over_iterator() :
         _first(),
         _last()
      {}

      istream_over_iterator(InputIterator first, InputIterator last) :
         _first(first),
         _last(last)
      {}

      int get()
      {
         int c ;
         ssize_t readcount ;
         if (_first != _last)
         {
            c = (unsigned char)*_first ;
            readcount = 1 ;
            ++_first ;
         }
         else
         {
            c = EOF ;
            readcount = 0 ;
         }
         set_readcount(readcount, 1) ;
         return c ;
      }

   protected:
      size_t read_data(void *where, size_t size)
      {
         size_t n = 0 ;
         for (char *out = static_cast<char *>(where) ; _first != _last && n < size ; ++_first, ++out, ++n)
            *out = *_first ;
         return n ;
      }

   private:
      InputIterator   _first ;
      InputIterator   _last ;
} ;

/******************************************************************************/
/** Binary output stream over an output iterator.
*******************************************************************************/
template<typename OutputIterator>
class ostream_over_iterator : public virtual binary_ostream {
   public:
      explicit ostream_over_iterator(const OutputIterator &out) :
         out_(out)
      {}

      ostream_over_iterator &put(char c)
      {
         *out_ = c ;
         ++out_ ;
         return *this ;
      }

   protected:
      size_t write_data(const void *data, size_t size)
      {
         out_ = std::copy(static_cast<const char *>(data), static_cast<const char *>(data) + size, out_) ;
         return size ;
      }

   private:
      OutputIterator out_ ;
} ;

/******************************************************************************/
/** Binary output stream over std::string.
*******************************************************************************/
class binary_ostrstream : public virtual binary_ostream {
   public:
      binary_ostrstream()
      {}

      template<class S>
      explicit binary_ostrstream(const S &initval,
                                 typename enable_if_strchar<S, char, unsigned>::type reserve = 0) :
         _data(str::cstr(initval), str::len(initval))
      {
         if (reserve)
            _data.reserve(_data.size() + reserve) ;
      }

      explicit binary_ostrstream(unsigned reserve)
      {
         _data.reserve(reserve) ;
      }

      binary_ostrstream &put(char c)
      {
         _data.push_back(c) ;
         return *this ;
      }

      const std::string &str() const
      {
         return _data ;
      }

      binary_ostrstream &clear()
      {
         _data = std::string() ;
         return *this ;
      }

   protected:
      size_t write_data(const void *data, size_t size)
      {
         NOXCHECK(data || !size) ;
         _data.append(static_cast<const char *>(data), size) ;
         return size ;
      }

   private:
      std::string _data ;
} ;

/*******************************************************************************
 Stream copy
*******************************************************************************/
template<size_t bufsz>
__noinline
size_t copy_stream(binary_istream &in, binary_ostream &out)
{
   char buf[bufsz] ;
   size_t written = 0 ;
   while (!in.eof() && in.read(buf))
   {
      NOXCHECK(in.last_read() && in.last_read() <= bufsz) ;
      written += out.write(buf + 0, in.last_read()) ;
   }
   return written ;
}

/// Read a buffer.
/// @ingroup TextIO
template<size_t n>
inline size_t read_buf(binary_istream &is, char (&buf)[n])
{
   return is.read(buf + 0, n) ;
}

/*******************************************************************************/
/** End-of-line kinds.
 @ingroup TextIO
*******************************************************************************/
enum EOLMode {
   eolmode_LF      = 0x0, /**< Unix (including Mac OSX/Darwin), "\n" */
   eolmode_CRLF    = 0x1  /**< DOS/Windows, HTTP end of entity headers, "\r\n" */
} ;

/// Skip an input stream until given character or EOF.
/// @par
/// Leaves the stream at the point immediately after the specified delimeiter, or
/// at the  end-of-file, if there is no such delimiter encounterd.
/// @ingroup TextIO_Skip
template<class IStream>
inline IStream &skip_to(IStream &is, char delimiter)
{
   for (int c ; (c = is.get()) != delimiter && c != EOF ; ) ;
   return is ;
}

/// Skip an input stream until given character sequence or EOF.
/// @par
/// Leaves the stream at the point immediately after the specified delimeiter, or
/// at the end-of-file, if there is no such delimiter encounterd.
/// @note Useful in network protocols, e.g. HTTP, when it is necessary to skip until
/// double CRLF: @code util::skip_to_delimiter(foostream, "\r\n\r\n") ; @endcode
/// @ingroup TextIO_Skip
template<class IStream>
inline IStream &skip_to(IStream &is, const char *delimiter)
{
   const char *pdelim = delimiter ;
   for (int c ; *pdelim && (c = is.get()) != EOF ; *pdelim == c ? ++pdelim : (pdelim = delimiter)) ;
   return is ;
}

/// Skip specified number of bytes from an input stream.
/// @ingroup TextIO_Skip
inline binary_istream &skip(binary_istream &is, size_t size)
{
   char dummy[8192] ;
   size_t remains = size ;
   for (size_t readcount ;
        (readcount = is.read(dummy, std::min(remains, sizeof dummy))) != 0 && (remains -= readcount) ; ) ;
   NOXCHECK(remains <= size) ;
   return is ;
}

/// Skip an input stream until EOF.
/// @ingroup TextIO_Skip
/// @note This function will never throw eof_error, even if is.throw_on_eof()==true.
inline binary_istream &skip(binary_istream &is)
{
   eof_guard guard (is, false) ;
   return skip(is, (size_t)-1) ;
}

/*******************************************************************************
 Line-oriented read
*******************************************************************************/
/// Read characters from an abstract istream and store them into an output iterator
/// until a newline or the EOF is reached.
/// @ingroup TextIO
/// @param is       An input stream.
/// @param maxsize  Maximal number of characters to store into the output. Note that
/// in case of @a eolmode = eol_CRLF this is generally @em not the maximal number of
/// characters to read.
/// @param out      An output iterator the line will be copied to char-by-char.
/// @param eolmode The kind of end-of-line character/sequence.
/// @note Retains '\n' in the result. If @a eoltype is eol_CRLF, replaces final CRLF
/// with LF.
template<typename OutputIterator>
OutputIterator readline(binary_ibufstream &is, size_t maxsize, OutputIterator out, EOLMode eolmode = eolmode_LF)
{
   size_t count = 0 ;
   if (eolmode == eolmode_LF)
      for (int c, prev = 0 ; count < maxsize && prev != '\n' && (c = is.get()) != EOF ; ++count, ++out, prev = c)
         *out = c ;
   else
      for (int c ; count < maxsize && (c = is.get()) != EOF ; ++count, *out = c, ++out)
         switch (c)
         {
            case '\r':
               if (is.peek() != '\n')
                  break ;
               c = is.get() ;
            case '\n':
               *out = c ;
               return ++out ;
         }
   return out ;
}

template<size_t n>
inline char *readline(binary_ibufstream &is, char (&buf)[n], EOLMode eolmode = eolmode_LF)
{
   if (n == 1)
   {
      *buf = 0 ;
      return buf ;
   }
   char *end = readline(is, n - 1, buf + 0, eolmode) ;
   if (end[-1] != '\n')
      if (end == buf + (n - 1))
      {
         end[-1] = '\n' ;
         // Skip to the end of line
         skip_to(is, '\n') ;
      }
      else
         *end++ = '\n' ;
   *end = 0 ;
   return buf ;
}

/// Read characters from an abstract istream until ent-of-line and return the resulting
/// line as std::string.
/// @note Retains '\n' in the result. If @a eoltype is eol_CRLF, replaces final CRLF with LF.
std::string readline(binary_ibufstream &is, EOLMode eolmode = eolmode_LF) ;

} // end of namespace pcomn

#endif /* __PCOMN_IOSTREAM_H */

/*******************************************************************************
 PCommon streams provide "ranges" (http://rangelib.synesis.com.au/).
 If you want to use pcommon iostreams with STLSoft rangelib, include pcommon_iostream.h
 somewhere after pcomn_range.h in include sequence. It can safely be included both before
 pcomn_range.h and after in the same include sequence; the only requirement is that it is
 included after pcomn_range.h at least once.
*******************************************************************************/
namespace pcomn {

struct istream_range_tag : public notional_range_tag {} ;

/******************************************************************************/
/** Notional range of characters on top of both pcomn::binary_istream and std::istream.
*******************************************************************************/
template<typename IStream>
class istream_range : public istream_range_tag {
   public:
      /// The range category tag type
      typedef istream_range_tag                 range_tag_type ;
      /// Stream type
      typedef IStream                           stream_type ;
      /// The value type is a stream character type.
      typedef typename stream_type::char_type   value_type ;

      /// The reference type: always const.
      typedef const value_type & reference ;

      PCOMN_STATIC_CHECK(std::numeric_limits<value_type>::is_integer && sizeof(value_type) <= sizeof(int)) ;

   public:
      istream_range() : _stream(), _value(EOF) {}

      /// Create a range over a stream.
      /// @note This is a conversion contructor, it is intentionally not "explicit".
      istream_range(stream_type &is) : _stream(&is)
      {
         if (is.eof())
            _value = EOF ;
         else
            advance() ;
      }

      /// Indicates whether the range is open.
      explicit operator bool() const { return _stream && !stream().eof() ; }

      /// Get the current value in the range.
      reference operator*() const
      {
         NOXCHECK(*this) ;
         return _value ;
      }

      /// Advance the current position in the range.
      void advance()
      {
         NOXCHECK(*this) ;
         _value = stream().get() ;
      }

      stream_type &stream() const
      {
         NOXCHECK(_stream) ;
         return *_stream ;
      }

      void release()
      {
         if (*this)
            stream().putback(_value) ;
      }

   private:
      stream_type *  _stream ;
      value_type     _value ;
} ;

} // end of namespace pcomn

/*******************************************************************************
 Provide pcomn::io::writer specializations for binary streams
*******************************************************************************/
#ifdef __PCOMN_IODEVICE_H
#ifndef __PCOMN_IOSTREAM_IODEVICE_H
#define __PCOMN_IOSTREAM_IODEVICE_H

namespace pcomn {
namespace io {

/******************************************************************************/
/** Writer specialization for pcomn::binary_ostream
*******************************************************************************/
template<>
struct writer<binary_ostream> : iodevice<binary_ostream &> {
      static ssize_t write(binary_ostream &stream, const char *from, const char *to)
      {
         NOXCHECK(from <= to) ;
         ssize_t written = 0 ;
         for (ssize_t remains = to - from ; remains ; )
         {
            const size_t wcount = stream.write(from + written, remains) ;
            remains -= wcount ;
            written += wcount ;
         }
         return written ;
      }
} ;

template<class Stream>
struct bstream_reader : iodevice<Stream &> {
      static ssize_t read(Stream &is, void *buf, size_t size) { return is.read(buf, size) ; }
      static int get_char(Stream &is) { return is.get() ; }
} ;

template<> struct reader<binary_istream> : bstream_reader<binary_istream> {} ;

template<> struct reader<binary_ibufstream> : bstream_reader<binary_ibufstream> {} ;

template<typename InputIterator>
struct reader<istream_over_iterator<InputIterator> > : bstream_reader<istream_over_iterator<InputIterator> > {} ;

} // end of namespace pcomn::io

inline io::writer<binary_ostream> get_writer(binary_ostream *) { return io::writer<binary_ostream>() ; }

inline io::reader<binary_istream> get_reader(binary_istream *) { return io::reader<binary_istream>() ; }
inline io::reader<binary_ibufstream> get_reader(binary_ibufstream *) { return io::reader<binary_ibufstream>() ; }

template<typename InputIterator>
inline io::reader<istream_over_iterator<InputIterator> > get_reader(istream_over_iterator<InputIterator> *)
{ return io::reader<istream_over_iterator<InputIterator> >() ; }

} // end of namespace pcomn

#endif /* __PCOMN_IOSTREAM_IODEVICE_H */
#endif /* __PCOMN_IODEVICE_H */
