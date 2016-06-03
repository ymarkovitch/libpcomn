/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_ZSTREAM_H
#define __PCOMN_ZSTREAM_H
/*******************************************************************************
 FILE         :   pcomn_zstream.h
 COPYRIGHT    :   Yakov Markovitch, 2004-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   ZLib wrappers over raw_stream and raw_stream wrappers over
                  zstream

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   4 Mar 2004
*******************************************************************************/
#include <pcomn_rawstream.h>
#include <pcomn_safeptr.h>
#include <pcomn_ziowrap.h>
#include <pcomn_handle.h>

#include <stdio.h>

namespace pcomn {

/*******************************************************************************
 gzip stream handle traits.
*******************************************************************************/
/// gzip stream handle tag.
struct gz_handle_tag { typedef gzFile handle_type ; } ;

template<>
inline bool handle_traits<gz_handle_tag>::close(gzFile h)
{
   return ::gzclose(h) >= 0 ;
}
template<>
inline bool handle_traits<gz_handle_tag>::is_valid(gzFile h)
{
   return !!h ;
}
template<>
inline constexpr gzFile handle_traits<gz_handle_tag>::invalid_handle()
{
   return gzFile() ;
}

/*******************************************************************************
 iostream compress/decompress
*******************************************************************************/
/// Compress a buffer into an output stream (std::ostream).
int ostream_zcompress(std::ostream &dest, const char *source, size_t source_len, int level = Z_DEFAULT_COMPRESSION) ;
/// Uncompress an input stream (std::istream) into a buffer.
int istream_zuncompress(char *dest, size_t &dest_len, std::istream &source, size_t source_len) ;

/******************************************************************************/
/** Exception to indicate ZLIB errors.
*******************************************************************************/
class _PCOMNEXP zlib_error : public std::runtime_error {
   public:
      typedef std::runtime_error Parent ;

      explicit zlib_error(int code) :
         Parent(_errname(code)),
         _code(code)
      {}

      explicit zlib_error(gzFile erroneous) :
         Parent(_errdesc(erroneous, &_code))
      {}

      int code() const { return _code ; }

   private:
      int _code ;

      static const char *_errname(int code) ;
      static const char *_errdesc(gzFile erroneous, int *errcode) ;

} ;

/*******************************************************************************
                     class basic_zstreambuf
*******************************************************************************/
class _PCOMNEXP basic_zstreambuf : private zstreambuf_t {
   public:
      static GZSTREAM open(basic_zstreambuf *streambuf, const char *mode)
      {
         return ::zopen(streambuf, mode) ;
      }

   protected:
      basic_zstreambuf(bool destroy_on_close) ;
      virtual ~basic_zstreambuf() {}

      virtual ssize_t read(void *, size_t) { return -1 ; }
      virtual ssize_t write(const void *, size_t) { return -1 ; }
      virtual ssize_t seek(fileoff_t, int) { return -1 ; }
      virtual int open() { return 0 ; }
      virtual int close() { return 0 ; }

      // Please note that error() is the only member function derived classes
      // _shall_ implement, since there is no reasonable default
      // implementation. Of course to be usable a derived class would also
      // implement either read() or write() but this is at least not mandatory
      // to compile.
      virtual int error(bool clear) = 0 ;

   private:
      // Pointers to the following functions are placed in the structure the
      // 'api' member of zstreambuf_t base points to.
      // The 'self' parameter is a pointer to an object of class
      // basic_zstreambuf; the functions itself just delegate calls to
      // corresponding virtual function members of basic_zstreambuf
      // (e.g. _read calls ((basic_zstreambuf *)self)->read(), etc.)
      static int _open(GZSTREAMBUF self) ;

      // Depending on the value of 'destroy_on_close' parameter of
      // basic_zstreambuf constructor, the api structure has the stream_close
      // member to point either to _close or to _destroy
      static int _close(GZSTREAMBUF self) ;
      static int _destroy(GZSTREAMBUF self) ;

      static int _error(GZSTREAMBUF self, int clear) ;
      static ssize_t _read(GZSTREAMBUF self, void *buf, size_t size) ;
      static ssize_t _write(GZSTREAMBUF self, const void *buf, size_t size) ;
      static fileoff_t _seek(GZSTREAMBUF self, fileoff_t offset, int origin) ;
} ;

/*******************************************************************************
                     class basic_zstreamwrap
*******************************************************************************/
class _PCOMNEXP basic_zstreamwrap {
   public:
      basic_zstreamwrap() :
         _stream(0)
      {}

      basic_zstreamwrap(basic_zstreambuf *streambuf, const char *mode) :
         _stream(basic_zstreambuf::open(streambuf, mode))
      {
         ensure_stream() ;
      }

      ~basic_zstreamwrap()
      {
         close() ;
      }

      bool is_open() const { return !!_stream ; }

      void open(basic_zstreambuf *streambuf, const char *mode)
      {
         if (_stream)
            bad_state() ;
         _stream = basic_zstreambuf::open(streambuf, mode) ;
         ensure_stream() ;
      }

      // close() is idempotent and safe: it can be called arbitrary times for
      // both already closed and not yet open streams
      void close()
      {
         if (is_open())
         {
            ::zclose(_stream) ;
            _stream = 0 ;
         }
      }

      ssize_t setparams(int level, int strategy)
      {
         return ::zsetparams(ensure_stream(), level, strategy) ;
      }

      ssize_t read(void *buf, size_t len)
      {
         return ::zread(ensure_stream(), buf, len) ;
      }
      ssize_t write(const void *buf, size_t len)
      {
         return ::zwrite(ensure_stream(), buf, len) ;
      }

      ssize_t put_string(const char *str)
      {
         return ::zputs(ensure_stream(), str) ;
      }
      char *get_string(char *buf, int len)
      {
         return ::zgets(ensure_stream(), buf, len) ;
      }

      int put_char(int c) { return ::zputc(ensure_stream(), c) ; }
      int get_char() { return ::zgetc(ensure_stream()) ; }
      int unget_char(int c) { return ::zungetc(ensure_stream(), c) ; }

      int flush(int flushmode)
      {
         return ::zflush(ensure_stream(), flushmode) ;
      }

      fileoff_t seek(fileoff_t offset, int whence)
      {
         return ::zseek(ensure_stream(), (long)offset, whence) ;
      }
      int rewind() { return ::zrewind(ensure_stream()) ; }

      fileoff_t tell() const { return ::ztell(ensure_stream()) ; }
      bool eof() const { return ::zeof(ensure_stream()) ; }
      std::ios_base::iostate rdstate() const
      {
         switch (const int errcode = ::zerror(ensure_stream()))
         {
            case Z_OK:           return std::ios_base::goodbit ;
            case Z_STREAM_END:   return std::ios_base::eofbit ;
            default: (void)errcode ; return std::ios_base::badbit ;
         }
      }

      void clear() { ::zclearerr(ensure_stream()) ; }

   private:
      zstream_t *_stream ;

      zstream_t *ensure_stream() const
      {
         if (!_stream)
            bad_state() ;
         return _stream ;
      }

      void bad_state() const ;

      // Let's prohibit any kind of object copying
      basic_zstreamwrap(const basic_zstreamwrap &) ;
      void operator=(const basic_zstreamwrap &) ;
} ;

/*******************************************************************************
                     class zstreamwrap
*******************************************************************************/
class _PCOMNEXP zstreamwrap : public basic_zstreamwrap {
   public:
      zstreamwrap() {}

      template<class Stream>
      zstreamwrap(Stream &stream, const char *mode) :
         basic_zstreamwrap(create_zstreambuf(stream), mode)
      {}

      template<class Stream>
      void open(Stream &stream, const char *mode)
      {
         basic_zstreamwrap::open(create_zstreambuf(stream), mode) ;
      }
} ;

/*******************************************************************************
                     class rawstream_zstreambuf
*******************************************************************************/
class _PCOMNEXP rawstream_zstreambuf : public basic_zstreambuf {
   protected:
      rawstream_zstreambuf(pcomn::raw_ios &stream, bool destroy_on_close) :
         basic_zstreambuf(destroy_on_close),
         _stream(stream)
      {}

      int error(bool clear) override
      {
         using pcomn::raw_ios ;
         uintptr_t state = _stream.rdstate() ;
         int result =
            (!(state & raw_ios::badbit) &
             (!(state & raw_ios::failbit) | !!(state & raw_ios::eofbit))) ? 0 : -1 ;
         if (clear)
            _stream.setstate(raw_ios::statebit, false) ;
         return result ;
      }

      fileoff_t seek(fileoff_t offset, int origin) override
      {
         if (_stream.rdstate() == (pcomn::raw_ios::failbit|pcomn::raw_ios::eofbit))
            _stream.setstate(pcomn::raw_ios::failbit, false) ;
         pcomn::raw_ios::seekdir dir =
            origin == SEEK_CUR ? pcomn::raw_ios::cur :
            (origin == SEEK_END ? pcomn::raw_ios::end : pcomn::raw_ios::beg) ;

         return _stream.seek(offset, dir) ;
      }

   private:
      pcomn::raw_ios &_stream ;
} ;

/*******************************************************************************
                     class orawstream_zstreambuf
*******************************************************************************/
class _PCOMNEXP orawstream_zstreambuf : public rawstream_zstreambuf {
      typedef rawstream_zstreambuf ancestor ;
   public:
      orawstream_zstreambuf(pcomn::raw_ostream &stream, bool destroy_on_close) :
         ancestor(stream, destroy_on_close),
         _stream(stream)
      {}

   protected:
      fileoff_t write(const void *buf, size_t size) override
      {
         return
            !_stream.write(static_cast<const char *>(buf), size) ? -1 : size ;
      }

   private:
      pcomn::raw_ostream &_stream ;
} ;

/*******************************************************************************
                     template<class Stream>
                     class istdstream_zstreambuf
*******************************************************************************/
class _PCOMNEXP irawstream_zstreambuf : public rawstream_zstreambuf {
      typedef rawstream_zstreambuf ancestor ;
   public:
      irawstream_zstreambuf(pcomn::raw_istream &stream, bool destroy_on_close) :
         ancestor(stream, destroy_on_close),
         _stream(stream)
      {}
   protected:
      fileoff_t read(void *buf, size_t size) override
      {
         _stream.read(static_cast<char *>(buf), size) ;
         return _stream.last_read() ;
      }
   private:
      pcomn::raw_istream &_stream ;
} ;

/*******************************************************************************
                     template<class Stream, unsigned seekflag>
                     class stdstream_zstreambuf
*******************************************************************************/
template<class Stream>
class stdstream_zstreambuf : public basic_zstreambuf {
   protected:
      stdstream_zstreambuf(Stream &stream, bool destroy_on_close) :
         basic_zstreambuf(destroy_on_close),
         _stream(stream)
      {}

      int error(bool clear) override
      {
         if (_stream.good())
            return 0 ;
         if (clear)
            _stream.clear() ;
         return -1 ;
      }

   protected:
      Stream &_stream ;

      int do_seek(fileoff_t offset, int origin, unsigned seekflag)
      {
         if (_stream.rdstate() == (std::ios::failbit|std::ios::eofbit))
            _stream.clear(std::ios::eofbit) ;
         std::ios_base::seekdir dir =
            origin == SEEK_CUR ? std::ios_base::cur :
            (origin == SEEK_END ? std::ios_base::end : std::ios_base::beg) ;
         return _stream.rdbuf() && _stream ?
            (int)_stream.rdbuf()->pubseekoff(offset, dir, (std::ios_base::openmode)seekflag) : -1 ;
      }
} ;

/*******************************************************************************
                     template<class Stream>
                     class ostdstream_zstreambuf
*******************************************************************************/
template<class Stream>
class ostdstream_zstreambuf : public stdstream_zstreambuf<Stream> {
      typedef stdstream_zstreambuf<Stream> ancestor ;
   public:
      ostdstream_zstreambuf(Stream &stream, bool destroy_on_close) :
         ancestor(stream, destroy_on_close)
      {}

   protected:
      ssize_t write(const void *buf, size_t size) override
      {
         return
            !this->_stream.write(static_cast<const char *>(buf), size) ? -1 : size ;
      }
      fileoff_t seek(fileoff_t offset, int origin) override
      {
         return this->do_seek(offset, origin, std::ios_base::out) ;
      }
} ;

/*******************************************************************************
                     template<class Stream>
                     class istdstream_zstreambuf
*******************************************************************************/
template<class Stream>
class istdstream_zstreambuf : public stdstream_zstreambuf<Stream> {
      typedef stdstream_zstreambuf<Stream> ancestor ;
   public:
      istdstream_zstreambuf(Stream &stream, bool destroy_on_close) :
         ancestor(stream, destroy_on_close)
      {}
   protected:
      ssize_t read(void *buf, size_t size) override
      {
         return
            this->_stream.read(static_cast<char *>(buf), size).gcount() ;
      }
      fileoff_t seek(fileoff_t offset, int origin) override
      {
         return this->do_seek(offset, origin, std::ios_base::in) ;
      }
} ;

/*******************************************************************************
 Factory template functions, needed by zstreamwrap
*******************************************************************************/
inline orawstream_zstreambuf *create_zstreambuf(pcomn::raw_ostream &stream)
{
   return new orawstream_zstreambuf(stream, true) ;
}

inline irawstream_zstreambuf *create_zstreambuf(pcomn::raw_istream &stream)
{
   return new irawstream_zstreambuf(stream, true) ;
}

/*******************************************************************************
              template<class RawStream>
              class raw_zstream
*******************************************************************************/
template<class RawStream>
class raw_basic_zstream : public raw_stream_wrapper<basic_zstreamwrap, RawStream> {
      typedef raw_stream_wrapper<basic_zstreamwrap, RawStream> ancestor ;
   public:
      template<class Stream>
      explicit raw_basic_zstream(Stream &stream, const char *mode = 0) :
         ancestor(new zstreamwrap(stream, open_mode(mode)), true)
      {}

      template<class Stream>
      explicit raw_basic_zstream(Stream *stream, const char *mode = 0) :
         ancestor(new zstreamwrap(*PCOMN_ENSURE_ARG(stream), open_mode(mode)), true),
         _owned(stream)
      {}

   protected:
      raw_ios::pos_type seekoff(raw_ios::off_type offs, raw_ios::seekdir dir)
      {
         int origin =
            dir == raw_ios::cur ? SEEK_CUR : (dir == raw_ios::end ? SEEK_END : SEEK_SET) ;
         return this->stream().seek(offs, origin) ;
      }

      size_t do_read(void *buffer, size_t size)
      {
         return std::max<ssize_t>(0, this->stream().read(buffer, size)) ;
      }

      size_t do_write(const void *buffer, size_t size)
      {
         return std::max<ssize_t>(0, this->stream().write(buffer, size)) ;
      }

      static const char *open_mode(const char *mode)
      {
         return
            mode ? mode : (stream_openmode((RawStream *)0) == std::ios_base::out ? "w" : "r") ;
      }

   private:
      PTSafePtr<RawStream> _owned ;
} ;

typedef raw_basic_zstream<raw_istream> raw_izstream ;
typedef raw_basic_zstream<raw_ostream> raw_ozstream ;

} // end of namespace pcomn

namespace std {

template<typename E, typename T>
inline pcomn::ostdstream_zstreambuf<basic_ostream<E, T> > *
create_zstreambuf(basic_ostream<E, T> &stream)
{
   return new pcomn::ostdstream_zstreambuf<basic_ostream<E, T> >(stream, true) ;
}

template<typename E, typename T>
inline pcomn::istdstream_zstreambuf<basic_istream<E, T> > *
create_zstreambuf(basic_istream<E, T> &stream)
{
   return new pcomn::istdstream_zstreambuf<basic_istream<E, T> >(stream, true) ;
}

}

#endif /* __PCOMN_ZSTREAM_H */
