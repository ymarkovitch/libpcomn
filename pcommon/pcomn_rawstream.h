/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_RAWSTREAM_H
#define __PCOMN_RAWSTREAM_H
/*******************************************************************************
 FILE         :   pcomn_rawstream.h
 COPYRIGHT    :   Yakov Markovitch, 2002-2018. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Binary input and output streams.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   10 Oct 2002
*******************************************************************************/
/** @file
    Binary input and output streams.

 Standard C++ iostream library is inherently inefficient and overcomplicated when the
 matter concerns sequential reading or/and writing raw (binary) data.
 So something much more like the POSIX handle-oriented I/O library would be appropriate.
*******************************************************************************/
#include <pcommon.h>
#include <pcomn_assert.h>
#include <pcomn_iodevice.h>
#include <pcomn_except.h>
#include <pcomn_buffer.h>
#include <pcomn_unistd.h>
#include <pcomn_strslice.h>

#include <stdexcept>
#include <cstdio>
#include <iostream>

namespace pcomn {

class failure_exception ;

/*******************************************************************************
                     class raw_ios
*******************************************************************************/
/** A base class for all raw streams.
*******************************************************************************/
class _PCOMNEXP raw_ios {
      PCOMN_NONCOPYABLE(raw_ios) ;
      PCOMN_NONASSIGNABLE(raw_ios) ;
   public:
      /*************************************************************************
                           enum IOState
      *************************************************************************/
      /** I/O state bits.
          Every value corresponds (and is equal) to the analogous std::ios_base
          value.
      *************************************************************************/
      enum iostate
      {
         badbit = std::ios_base::badbit,
         eofbit = std::ios_base::eofbit,
         failbit = std::ios_base::failbit,
         goodbit = std::ios_base::goodbit
      } ;
      static const unsigned statebit = badbit | eofbit | failbit ;
      static const unsigned closebit = ~(unsigned)0 ;

      /*************************************************************************
       Seek direction
      *************************************************************************/
      enum seekdir {
         beg   = std::ios_base::beg,
         cur   = std::ios_base::cur,
         end   = std::ios_base::end
      } ;

      typedef ssize_t off_type ;
      typedef size_t  pos_type ;
      typedef failure_exception failure ;

      raw_ios() :
         _state(0),
         _exceptions(0),
         _throwable(~0)
      {}

      virtual ~raw_ios() {}

      // This function is called from a destructor, so empty throw specification is desirable
      void close() throw() ;

      bool eof() const { return !!(rdstate() & eofbit) ; }
      bool bad() const { return !!(rdstate() & badbit) ; }
      bool fail() const { return !!(rdstate() & (badbit | failbit)) ; }
      bool good() const { return !rdstate() ; }

      bool is_open() const { return rdstate() != closebit ; }

      /// Check whether this ios object is in a good state.
      /// @return true if the object is in a good state and false otherwise.
      ///
      explicit operator bool() const { return !fail() ; }

      /// Get status flags (eofbit, badbit, etc.).
      unsigned rdstate() const { return _state ; }

      unsigned setstate(unsigned state, bool onoff = true)
      {
         if (is_open())
            resetstate(onoff ? (rdstate() | state) : (rdstate() & ~state)) ;
         return rdstate() ;
      }

      virtual void resetstate(unsigned state = goodbit)
      {
         _state = state ;
         _check_throw() ;
      }

      /// Set bits that cause throwing of std::ios_bas::failure.
      /// This function is like std::ios_base::exceptions()
      void exceptions(unsigned state) ;
      unsigned exceptions() const { return _exceptions ; }

      pos_type seek(off_type off, seekdir dir = beg)
      {
         if (!(_state & badbit))
            resetstate() ;
         pos_type position = seekoff(off, dir) ;
         if (!_state && position == pos_type(-1))
            resetstate(external_state() | failbit) ;
         return position ;
      }

      pos_type tell() { return seek(0, cur) ; }

   protected:
      unsigned setstate_nothrow(unsigned state, bool onoff)
      {
         return onoff ? (_state |= state) : (_state &= ~state) ;
      }

      // Please call this from catch() clause ONLY!
      void process_exception() ;

      virtual void do_close() ;
      virtual pos_type seekoff(off_type, seekdir) ;
      virtual void throw_failure() {}
      virtual unsigned external_state() const { return 0 ; }

      class sentry ;
      friend class sentry ;
      class sentry {
         public:
            sentry(raw_ios &io, unsigned mask_exceptions = ~0) :
               _io(io),
               _throwable(io._throwable)
            {
               io._throwable &= ~mask_exceptions ;
            }
            ~sentry() { _io._throwable = _throwable ; }

         private:
            raw_ios &   _io ;
            unsigned   _throwable ;

            void *operator new(size_t) throw() { return NULL ; }
            void operator delete(void *, size_t) {}
      } ;

   private:
      unsigned _state ;
      unsigned _exceptions ;
      unsigned _throwable ;

      void _check_throw()
      {
         if (exceptions() & rdstate() & _throwable)
         {
            throw_failure() ;
            _throw_failure() ;
         }
      }

      void _throw_failure() ;
} ;

/******************************************************************************/
/** The exception, which is thrown by default by pcomn::raw_ios descendants
 when exception bits are matched.

 @note Please note that, since raw_ios::throw_exception can be overriden, there is no
 warranty that a descendant class throws only this type of  exceptions.
*******************************************************************************/
class _PCOMNEXP failure_exception : public std::ios_base::failure {
      typedef std::ios_base::failure ancestor ;
   public:
      failure_exception(const char *message, raw_ios::iostate state) :
         ancestor(message),
         _state(state)
      {}

      raw_ios::iostate code() const { return _state ; }
   private:
      raw_ios::iostate _state ;
} ;

/*******************************************************************************
                     class raw_istream
*******************************************************************************/
class _PCOMNEXP raw_istream : public virtual raw_ios {
   public:
      // read()   -  read raw data from a stream
      //
      raw_istream &read(void *buffer, size_t size) ;

      raw_istream &seekg(off_type off, seekdir dir = beg)
      {
         seek(off, dir) ;
         return *this ;
      }

      pos_type tellg() { return tell() ; }

      /// Get the number of bytes that have been read by last read operation.
      /// @return 0 before the first read, if the last time it has been
      /// requested 0 bytes or if the last read failured or EOF condition
      /// encountered.
      size_t last_read() const { return _last_read ; }

   protected:
      raw_istream() :
         _last_read(0)
      {}

      virtual size_t do_read(void *buffer, size_t size) = 0 ;

   private:
      size_t _last_read ;
} ;

/*******************************************************************************
                     class raw_istream
*******************************************************************************/
class _PCOMNEXP raw_ostream : public virtual raw_ios {
   public:
      /// Write raw data into a stream.
      raw_ostream &write(const void *buffer, size_t size) ;

      raw_ostream &seekp(off_type off, seekdir dir = beg)
      {
         seek(off, dir) ;
         return *this ;
      }

      pos_type tellp() { return tell() ; }

      /// Get the number of bytes written by the last writing operation.
      size_t last_written() const { return _last_written ; }

   protected:
      raw_ostream() :
         _last_written(0)
      {}

      virtual size_t do_write(const void *buffer, size_t size) = 0 ;

   private:
      size_t _last_written ;
} ;

/*******************************************************************************
 Wrappers around pointers to other streams
*******************************************************************************/

/*******************************************************************************
                     template <class Stream, class RawStream>
                     class raw_stream_wrapper
*******************************************************************************/
template <class Stream, class RawStream>
class raw_stream_wrapper : public RawStream {
      typedef RawStream ancestor ;
   public:
      typedef Stream wrapped_stream_t ;

      /// The destructor is necessary to provide a right context for a virtual call
      /// to do_close().
      ~raw_stream_wrapper() { this->close() ; }

      raw_stream_wrapper &open(wrapped_stream_t *stream)
      {
         if (stream != _stream)
         {
            this->close() ;
            _stream = stream ;
            this->setstate_nothrow(raw_ios::closebit, !_stream) ;
         }
         return *this ;
      }

      wrapped_stream_t &stream() { return *_get_wrapped() ; }
      const wrapped_stream_t &stream() const { return *_get_wrapped() ; }

      /// Check whether this buffer owns its underlying stream.
      bool owns() const { return _owns ; }
      bool owns(bool does_own)
      {
         bool old = _owns ;
         _owns = does_own ;
         return old ;
      }

   protected:
      explicit raw_stream_wrapper(wrapped_stream_t *stream = 0, bool owns = false) :
         _stream(stream),
         _owns(owns)
      {
         this->setstate_nothrow(raw_ios::closebit, !_stream) ;
      }

      void do_close() ;

      unsigned external_state() const { return stream().rdstate() ; }

   private:
      wrapped_stream_t *_stream ;   /* A pointer to a wrapped stream */
      bool              _owns ;     /* Indicates whether the wrapper should delete the wrapped
                                     * stream when closing it. */

      wrapped_stream_t *_get_wrapped() const
      {
         NOXCHECK(_stream) ;
         return _stream ;
      }
} ;

/*******************************************************************************
 raw_stream_wrapper<Stream, RawStream>
*******************************************************************************/
template <class Stream, class RawStream>
void raw_stream_wrapper<Stream, RawStream>::do_close()
{
   // Please note, since do_close() can be called iff the stream is open, there is no need
   // in checking this
   if (owns())
   {
      wrapped_stream_t *tmp = _stream ;
      _stream = NULL ;
      delete tmp ;
   }
}

inline std::ios_base::openmode stream_openmode(const raw_istream *) { return std::ios_base::in ; }
inline std::ios_base::openmode stream_openmode(const raw_ostream *) { return std::ios_base::out ; }

/*******************************************************************************
                     template <typename C, class T>
                     class raw_basic_stream
*******************************************************************************/
template <class Stream, class RawStream>
class raw_basic_stream : public raw_stream_wrapper<Stream, RawStream> {
      typedef raw_stream_wrapper<Stream, RawStream> ancestor ;
   public:
      typedef typename ancestor::wrapped_stream_t wrapped_stream_t ;

      void resetstate(unsigned state = raw_ios::goodbit) ;

   protected:
      raw_basic_stream(wrapped_stream_t *stream, bool owns) :
         ancestor(stream, owns)
      {
         if (this->is_open())
            this->setstate_nothrow(stream->rdstate(), true) ;
      }

      raw_ios::pos_type seekoff(raw_ios::off_type, raw_ios::seekdir) ;
} ;

/*******************************************************************************
                     template <typename C, class T>
                     class raw_basic_istream
*******************************************************************************/
template <typename C, class T = std::char_traits<C> >
class raw_basic_istream : public raw_basic_stream<std::basic_istream<C, T>, raw_istream> {
      typedef raw_basic_stream<std::basic_istream<C, T>, raw_istream> ancestor ;
   public:
      typedef typename ancestor::wrapped_stream_t wrapped_stream_t ;

      explicit raw_basic_istream(wrapped_stream_t *stream = NULL, bool owns = false) :
         ancestor(stream, owns)
      {}

   protected:
      size_t do_read(void *buffer, size_t size) ;
} ;

/*******************************************************************************
                     template <typename C, class T>
                     class raw_basic_ostream
*******************************************************************************/
template <typename C, class T = std::char_traits<C> >
class raw_basic_ostream : public raw_basic_stream<std::basic_ostream<C, T>, raw_ostream> {
      typedef raw_basic_stream<std::basic_ostream<C, T>, raw_ostream> ancestor ;
   public:
      typedef typename ancestor::wrapped_stream_t wrapped_stream_t ;

      explicit raw_basic_ostream(wrapped_stream_t *stream = NULL, bool owns = false) :
         ancestor(stream, owns)
      {}

   protected:
      size_t do_write(const void *buffer, size_t size) ;
} ;

/*******************************************************************************
 raw_basic_stream<Stream, RawStream>
*******************************************************************************/
template <class Stream, class RawStream>
raw_ios::pos_type raw_basic_stream<Stream, RawStream>::seekoff(raw_ios::off_type off,
                                                               raw_ios::seekdir dir)
{
   wrapped_stream_t &wstream = this->stream() ;
   return wstream.rdbuf() && *this
      ? ((raw_ios::pos_type)wstream.rdbuf()->
         pubseekoff(off, (std::ios_base::seekdir)dir, (std::ios_base::openmode)stream_openmode(this)))
      : -1 ;
}

template <class Stream, class RawStream>
void raw_basic_stream<Stream, RawStream>::resetstate(unsigned state)
{
   ancestor::resetstate(state) ;
   this->stream().clear((std::ios_base::iostate)state) ;
}

/*******************************************************************************
 raw_basic_istream<C, T>
*******************************************************************************/
template <typename C, class T>
size_t raw_basic_istream<C, T>::do_read(void *buffer, size_t size)
{
   // Don't check anything like good(), bad(), etc. - at this point
   // everything is checked
   return
      this->stream().read(static_cast<typename T::char_type *>(buffer), size).gcount() * sizeof(C) ;
}

/*******************************************************************************
 raw_basic_ostream<C, T>
*******************************************************************************/
template <typename C, class T>
size_t raw_basic_ostream<C, T>::do_write(const void *buffer, size_t size)
{
   return
      size *
      !!(this->stream().write(static_cast<const typename T::char_type *>(buffer), size)) ;
}

inline std::ios_base::openmode stream_openmode(const std::istream *) { return std::ios_base::in ; }
inline std::ios_base::openmode stream_openmode(const std::ostream *) { return std::ios_base::out ; }

/*******************************************************************************
                     class raw_fstreambase
*******************************************************************************/
template<class RawStream>
class _PCOMNEXP raw_fstreambase : public RawStream {
      typedef RawStream ancestor ;
   protected:
      raw_fstreambase(FILE *file, bool owns) :
         _file(file),
         _owns(owns && file)
      {
         this->setstate_nothrow(raw_ios::closebit, !fptr()) ;
      }

      template<typename S>
      explicit raw_fstreambase(const S &name, const char *mode) :
         _file(std::fopen(pcomn::str::cstr(name), mode)),
         _owns(!!_file)
      {
         ancestor::resetstate(fptr() ? (unsigned)raw_ios::goodbit : (unsigned)raw_ios::closebit) ;
      }

      ~raw_fstreambase()
      {
         this->close() ;
      }

      void open(FILE *file, bool owns)
      {
         if (file == _file)
            return ;
         this->close() ;
         _file = file ;
         _owns = owns && file ;
         ancestor::resetstate(_file ? (unsigned)raw_ios::goodbit : raw_ios::closebit) ;
      }

      void open(const char *name, const char *mode)
      {
         open(std::fopen(name, mode), true) ;
      }

      FILE *fptr() const { return _file ; }

      void resetstate(unsigned state)
      {
         ancestor::resetstate(state) ;
         if ((state & raw_ios::statebit) && fptr())
            std::clearerr(fptr()) ;
      }

      raw_ios::pos_type seekoff(raw_ios::off_type offs, raw_ios::seekdir dir) ;
      void do_close() ;

      unsigned external_state() const
      {
         const unsigned state = feof(fptr()) ? (unsigned)raw_ios::eofbit : (unsigned)0 ;
         return ferror(fptr()) ? (state | raw_ios::badbit) : state ;
      }

   private:
      FILE *_file ;
      bool  _owns ;
} ;

/*******************************************************************************
                     class raw_ifstream
*******************************************************************************/
/** Binary intput stream using ISO C FILE as an underlying implementation.
*******************************************************************************/
class _PCOMNEXP raw_ifstream : public raw_fstreambase<raw_istream> {
      typedef raw_fstreambase<raw_istream> ancestor ;
   public:
      raw_ifstream() :
         ancestor((FILE *)NULL, false)
      {}

      raw_ifstream(FILE *file, bool owns) :
         ancestor(file, owns)
      {}

      template<typename S>
      explicit raw_ifstream(const S &name, enable_if_strchar_t<S, char, double> = 0) :
         ancestor(name, "rb")
      {}

      raw_ifstream &open(FILE *file, bool owns)
      {
         ancestor::open(file, owns) ;
         return *this ;
      }

      template<typename S>
      enable_if_strchar_t<S, char, raw_ifstream &> open(const S &name)
      {
         ancestor::open(pcomn::str::cstr(name), "rb") ;
         return *this ;
      }

   protected:
      size_t do_read(void *buffer, size_t size) ;
} ;

/*******************************************************************************
                     class raw_ofstream
*******************************************************************************/
/** Binary output stream using ISO C FILE as an underlying implementation.
*******************************************************************************/
class _PCOMNEXP raw_ofstream : public raw_fstreambase<raw_ostream> {
      typedef raw_fstreambase<raw_ostream> ancestor ;
   public:
      raw_ofstream() :
         ancestor((FILE *)NULL, false)
      {}

      raw_ofstream(FILE *file, bool owns) :
         ancestor(file, owns)
      {}

      template<typename S>
      explicit raw_ofstream(const S &name, enable_if_strchar_t<S, char, bool> append = 0) :
         ancestor(name, append ? "w+b" : "wb")
      {}

      raw_ofstream &open(FILE *file, bool owns)
      {
         ancestor::open(file, owns) ;
         return *this ;
      }

      template<typename S>
      enable_if_strchar_t<S, char, raw_ifstream &>
      open(const S &name, bool append = false)
      {
         ancestor::open(pcomn::str::cstr(name), append ? "w+b" : "wb") ;
         return *this ;
      }

   protected:
      size_t do_write(const void *buffer, size_t size) ;
} ;

/*******************************************************************************
                     class raw_guarded_ofstream
*******************************************************************************/
class raw_guarded_ofstream : public raw_ofstream {
      typedef raw_ofstream ancestor ;
   public:
      template<typename S>
      explicit raw_guarded_ofstream(const S &name, enable_if_strchar_t<S, char, bool> append = 0) :
         ancestor(name, append),
         _filename(pcomn::str::cstr(name)),
         _lock(fptr() && !append)
      {}

      ~raw_guarded_ofstream()
      {
         if (fptr() && _lock > 0)
         {
            close() ;
            unlink(pcomn::str::cstr(_filename)) ;
         }
      }

      void lock() { ++_lock ; }
      void unlock() { --_lock ; }
   private:
      std::string _filename ;
      int         _lock ;
} ;

/*******************************************************************************
                     class raw_imemstream
*******************************************************************************/
/** Memory input raw stream.
    Provides binary input from memory buffer.
*******************************************************************************/
class _PCOMNEXP raw_imemstream : public raw_istream {
      typedef raw_istream ancestor ;
   public:
      raw_imemstream() = default ;

      raw_imemstream(const void *buf, size_t bufsize) :
         _data(buf),
         _size(bufsize),
         _pos(0)
      {
         ensure<std::invalid_argument>
            (buf || !bufsize, "NULL buffer with nonzero size passed to pcomn::raw_imemstream constructor.") ;
      }

      explicit raw_imemstream(const strslice &buf) : raw_imemstream(buf.begin(), buf.size()) {}

      const void *gptr() const { return pcomn::padd(_data, _pos) ; }
      const void *data() const { return _data ; }
      size_t size() const { return _size ; }
   protected:
      pos_type seekoff(off_type offs, seekdir dir) ;
      size_t do_read(void *buffer, size_t bufsize) ;

   private:
      const void * const _data = nullptr ;
      const size_t       _size = 0 ;
      pos_type           _pos  = 0 ;
} ;

/******************************************************************************/
/** Memory output raw stream: provides binary output into a memory buffer.

 The memory buffer can be owned ("external") or unowned by the stream.
 Owned buffer may be fixed- as well as variable-length; unowned may be only
 fixed-length.

 For fixed-length buffer raw_omemstream sets failbit on attempt to write more than
 maxsize() data,
*******************************************************************************/
class _PCOMNEXP raw_omemstream : public raw_ostream {
      typedef raw_ostream ancestor ;
   public:
      /// Create a stream with owned buffer.
      ///
      /// @param bufsize The maximum length of the buffer; if -1, the buffer is
      /// variable-length and can grow infinitely.
      ///
      explicit raw_omemstream(size_t bufsize = (size_t)-1) :
         _buffer(size_t(), bufsize),
         _pos(0),
         _endpos(0)
      {}

      /// Create a stream over external ("unowned") buffer.
      ///
      /// @param buf       Pointer to a buffer
      /// @param bufsize   The length of the buffer, see maxsize().
      ///
      raw_omemstream(void *buf, size_t bufsize) :
         _buffer(buf, bufsize),
         _pos(0),
         _endpos(0)
      {}

      const void *data() const { return _buffer.data() ; }
      void *data() { return _buffer.data() ; }

      /// Get the size of already written data.
      /// @note For fixed-length buffer, this is @em not the length of the buffer, for
      /// the length of the buffer see maxsize().
      ///
      size_t size() const { return _endpos ; }

      /// Get the maiximum buffer length.
      /// @return Maiximum buffer length or size_t(-1) if the buffer is variable-length.
      ///
      size_t maxsize() const { return _buffer.maxsize() ; }

   protected:
      pos_type seekoff(off_type offs, seekdir dir) ;
      size_t do_write(const void *buffer, size_t bufsize) ;
      void do_close() ;

   private:
      basic_buffer   _buffer ;
      pos_type       _pos ;
      pos_type       _endpos ;

      bool expand(size_t newsize) ;
} ;


/*******************************************************************************
              class raw_icachestream
*******************************************************************************/
/** Proxy input stream - a wrapper for an unidirectional input stream.

  Buffers a region of the wrapped stream, thus making it possible to seeking
  inside such a region even though the wrapped stream doesn't allow seek
  operation at all.

  The cached string provides a cached range, initially empty. As the stream
  got read, the underlying cache is filled with read data, thus making
  backward seeking possible. Backward seeking is possible even if the
  resulting position lies before the cache start position (providing that
  the position is >= 0). However, reading from such a before-start position
  will fail.

  Seeking beneath the end of current cached region is possible and causes
  reading of the underlying file and filling of the cache up until the
  requested seek position. Particularly, seeking relative to the end,
  i.e. when the 'direction' parameter passed to raw_istream::seek is equal
  to raw_ios::end, initiates reading of the underlying file until eof and
  then seeeking inside the cached region.

  The stream set as a source of raw_icachestream can have working tell()
  implementation (i.e. its seekoff(0, raw_ios::cur) returns some meaningful
  value), but this is fully optional, since raw_icachestream constructor
  accepts the init_pos parameter, which, if >= 0, simply states the initial
  source stream position and prevents any calls to tell().
*******************************************************************************/
class _PCOMNEXP raw_icachestream : public raw_istream {
      typedef raw_istream ancestor ;
   public:
      raw_icachestream(raw_istream *source, bool owns_source,
                       off_type stated_init_pos = -1) ;

      ~raw_icachestream()
      {
         close() ;
      }

      void start_caching() { ++_caching ; }
      void stop_caching() { (void)(_caching && !--_caching) ; }

      bool caching() const { return !!_caching ; }

      /// Get the starting position of the cached region.
      pos_type cache_startpos() const { return _bufstart ; }
      /// Get the past-the-end position of the cached region.
      pos_type cache_endpos() const { return _bufend ; }

      void resetstate(unsigned state = raw_ios::goodbit)
      {
         ancestor::resetstate(state) ;
         source().resetstate(state) ;
      }

   protected:
      pos_type seekoff(off_type offs, seekdir dir) ;
      size_t do_read(void *buffer, size_t bufsize) ;
      void do_close() ;
      unsigned external_state() const { return source().rdstate() ; }

   private:
      raw_istream *  _source ;
      basic_buffer   _cache ;
      pos_type       _bufstart ;
      pos_type       _bufend ;
      pos_type       _position ;
      int            _caching ;
      bool           _owns_source ;

      raw_istream &source() const { return *_source ; }
      size_t read_forward(void *buffer, size_t bufsize) ;
      size_t read_chunk(void *buffer, size_t bufsize) ;
} ;

/*******************************************************************************
 io::writer specializations for pcomn::raw_ostream
 io::reader specializations for pcomn::raw_istream
*******************************************************************************/
namespace io {

/******************************************************************************/
/** pcomn::raw_ostream writer
*******************************************************************************/
template<>
struct writer<raw_ostream> : iodevice<raw_ostream &> {
   static ssize_t write(raw_ostream &stream, const char *from, const char *to)
   {
      NOXCHECK(from <= to) ;
      stream.write(from, to - from) ;
      return stream.last_written() ;
   }
} ;

inline writer<raw_ostream> get_writer(raw_ostream *) { return writer<raw_ostream>() ; }

/******************************************************************************/
/** pcomn::raw_istream reader
*******************************************************************************/
template<>
struct reader<raw_istream> : public iodevice<raw_istream &> {
   static ssize_t read(raw_istream &stream, void *buf, size_t bufsize)
   {
      NOXCHECK(buf || !bufsize) ;
      stream.read(buf, bufsize) ;
      return stream.last_read() ;
   }
   static int get_char(raw_istream &stream)
   {
      unsigned char result ;
      stream.read(&result, 1) ;
      return (stream.last_read() == 1) ? (int)result : -1 ;
   }
} ;

} // end of namespace pcomn::io

/*******************************************************************************
 Typedefs
*******************************************************************************/
typedef raw_basic_istream<char> raw_stdistream ;
typedef raw_basic_ostream<char> raw_stdostream ;

} ; // end of namespace pcomn

#endif /* __PCOMN_RAWSTREAM_H */
