/*-*- tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_zstream.cpp
 COPYRIGHT    :   Yakov Markovitch, 2004-2017. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   ZLib "abstract stream" classes (NOT std::stream!)

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   4 Mar 2004
*******************************************************************************/
#include <pcomn_zstream.h>
#include <pcomn_assert.h>

#include <stdexcept>
#include <algorithm>
#include <stdlib.h>

namespace pcomn {

/*******************************************************************************
 zlib_error
*******************************************************************************/
const char *zlib_error::_errdesc(gzFile erroneous, int *errcode)
{
   int errnum ;
   const char *result = gzerror(erroneous, &errnum) ;
   if (errnum == Z_ERRNO)
      result = _errname(errnum) ;
   if (errcode)
      *errcode = errnum ;
   return result ;
}

const char *zlib_error::_errname(int code)
{
   switch(code)
   {
      case Z_NEED_DICT: return "need dictionary" ;
      case Z_STREAM_END: return "stream end" ;
      case Z_OK: return "" ;
      case Z_ERRNO: return strerror(errno) ;
      case Z_STREAM_ERROR: return "stream error" ;
      case Z_DATA_ERROR: return "data error" ;
      case Z_MEM_ERROR: return "insufficient memory" ;
      case Z_BUF_ERROR: return "buffer error" ;
      case Z_VERSION_ERROR: return "incompatible version" ;
   }
   return "unknown" ;
}

/*******************************************************************************
 basic_zstreambuf
*******************************************************************************/
basic_zstreambuf::basic_zstreambuf(bool destroy_on_close)
{
   static const zstream_api_t api_kind[2] =
      {
         {_open, _destroy, _read, _write, _seek, _error},
         {_open, _close, _read, _write, _seek, _error}
      } ;
   api = api_kind + !destroy_on_close ;
}

int basic_zstreambuf::_open(GZSTREAMBUF self)
{
   NOXCHECK(self) ;
   try {
      return ((basic_zstreambuf *)self)->open() ;
   }
   catch (...)
   {
      if (self->api->stream_close == _destroy)
         delete ((basic_zstreambuf *)self) ;
      // The zstream being opened is not completely open so far and its
      // streambuf member is NULL at this point, so we can safely call
      // zclose()
      //::zclose(self) ;
      throw ;
   }
}

int basic_zstreambuf::_close(GZSTREAMBUF self)
{
   NOXCHECK(self) ;
   // Catch them all to prevent from an exceptions from being thrown
   // inside a destructor
   try {
      return ((basic_zstreambuf *)self)->close() ;
   }
   catch (...)
   {
      perror("Exception in basic_zstreambuf::close()") ;
      abort() ;
   }
   // Pacify poor compiler, which doesn't understand that there is no
   // way out of the abort()
   return 0 ;
}

int basic_zstreambuf::_destroy(GZSTREAMBUF self)
{
   int result = _close(self) ;
   delete ((basic_zstreambuf *)self) ;
   return result ;
}

int basic_zstreambuf::_error(GZSTREAMBUF self, int clear)
{
   NOXCHECK(self) ;
   return ((basic_zstreambuf *)self)->error(!!clear) ;
}

ssize_t basic_zstreambuf::_read(GZSTREAMBUF self, void *buf, size_t size)
{
   NOXCHECK(self) ;
   return ((basic_zstreambuf *)self)->read(buf, size) ;
}

ssize_t basic_zstreambuf::_write(GZSTREAMBUF self, const void *buf, size_t size)
{
   NOXCHECK(self) ;
   return ((basic_zstreambuf *)self)->write(buf, size) ;
}

fileoff_t basic_zstreambuf::_seek(GZSTREAMBUF self, fileoff_t offset, int origin)
{
   NOXCHECK(self) ;
   return ((basic_zstreambuf *)self)->seek(offset, origin) ;
}

/*******************************************************************************
 basic_zstreamwrap
*******************************************************************************/
void basic_zstreamwrap::bad_state() const
{
   throw
      std::logic_error(_stream ?
                       "Cannot perform operation on open zstream" :
                       "Cannot perform operation on closed zstream") ;
}

/*******************************************************************************
 ostream_zcompress
 istream_zuncompress
*******************************************************************************/
static const unsigned compress_chunk = 64 * 1024 ;

// _flush_out()   -  flushes (bufsize-stream.avail_out) bytes into the output stream
//                   and resets stream.avail_out to bufsize.
// Returns:
//    Former value of stream.avail_out
static inline void _flush_out(std::ostream &dest, z_stream &stream, unsigned bufsize)
{
   size_t write_size = bufsize - stream.avail_out ;
   stream.next_out -= write_size ;
   dest.write(reinterpret_cast<const char *>(stream.next_out), write_size) ;
   stream.avail_out = bufsize ;
}

int ostream_zcompress(std::ostream &dest, const char *source, size_t source_size, int level)
{
   z_stream stream ;
   int err ;
   const unsigned source_len = (unsigned)source_size ;
   const unsigned bufsize = (unsigned)std::min<size_t>(compress_chunk, (source_len * 11ULL)/10 + 13) ;

   if (source_len != source_size) return Z_MEM_ERROR ;

   stream.next_in = (uint8_t *)(source) ;
   stream.avail_in = source_len ;
   stream.zalloc = NULL ;
   stream.zfree = NULL ;
   stream.opaque = NULL ;

   PTVSafePtr<char> buffer (new char[bufsize]) ;

   stream.next_out = (uint8_t *)buffer.get() ;
   stream.avail_out = bufsize ;

   err = deflateInit(&stream, level) ;
   if (err != Z_OK)
      return err ;

   do {
      err = deflate(&stream, stream.avail_in ? Z_NO_FLUSH : Z_FINISH) ;
      _flush_out(dest, stream, bufsize) ;
   }
   while(err == Z_OK) ;

   switch(err)
   {
      case Z_STREAM_END:
         _flush_out(dest, stream, bufsize) ;
      case Z_OK:
         err = deflateEnd(&stream) ;
         break ;

      default:
         deflateEnd(&stream) ;
   }

   return err ;
}

int istream_zuncompress(char *dest, size_t &dest_len, std::istream &source, size_t source_size)
{
   NOXPRECONDITION(dest != NULL && source) ;

   z_stream stream ;
   int err ;
   const unsigned source_len = (unsigned)source_size ;
   const unsigned bufsize = std::min(compress_chunk, source_len) ;

   if (source_len != source_size) return Z_MEM_ERROR ;
   if (dest_len != (unsigned)dest_len) return Z_MEM_ERROR ;

   stream.next_out = (uint8_t *)dest ;
   stream.avail_out = (unsigned)dest_len ;
   stream.zalloc = NULL ;
   stream.zfree = NULL ;
   stream.opaque = NULL ;

   PTVSafePtr<char> buffer (new char[bufsize]) ;
   size_t remains = source_len ;

   stream.next_in = (uint8_t *)buffer.get() ;
   stream.avail_in = bufsize ;

   err = inflateInit(&stream) ;
   if (err != Z_OK)
      return err ;

   do
   {
      stream.next_in = (uint8_t *)buffer.get() ;
      stream.avail_in = (unsigned)source.read(reinterpret_cast<char *>(stream.next_in),
                                              std::min(bufsize, (unsigned)remains)).gcount() ;

      err = inflate(&stream, remains >= stream.avail_in ? Z_NO_FLUSH : Z_FINISH) ;

      remains -= stream.next_in - (uint8_t *)buffer.get() ;
   }
   while(err == Z_OK && stream.avail_out && source) ;

   switch(err)
   {
      case Z_STREAM_END:
         err = inflateEnd(&stream) ;
         dest_len -= remains ;
         break ;

      case Z_OK:
         err = !stream.avail_out ? Z_BUF_ERROR : Z_STREAM_ERROR ;
      default:
         deflateEnd(&stream) ;
   }

   return err ;
}

} // end of namespace pcomn
