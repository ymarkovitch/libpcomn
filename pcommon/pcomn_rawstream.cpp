/*-*- tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_rawstream.cpp
 COPYRIGHT    :   Yakov Markovitch, 2002-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   The implementation of raw I/O streams

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   11 Oct 2002
*******************************************************************************/
#include <pcomn_rawstream.h>
#include <pcomn_assert.h>
#include <pcomn_integer.h>

#include <memory>

namespace pcomn {

/*******************************************************************************
 raw_ios
*******************************************************************************/
static const char _err_stream_closed[] =
   "failure opening a raw stream or attempt to perform a read/write/seek on a closed raw stream" ;
static const char _err_stream_failed[] = "raw stream object has failbit set" ;
static const char _err_stream_bad[] = "raw stream object has badbit set" ;
static const char _err_stream_eof[] = "end-of-file condition on a raw stream object" ;
static const char _err_stream_unknown[] = "raw stream object has illegal state bit set" ;

void raw_ios::_throw_failure()
{
   if (!is_open())
      throw failure_exception(_err_stream_closed, (iostate)closebit) ;

   iostate error_state = (iostate)(rdstate() & exceptions()) ;
   const char *errmsg = _err_stream_unknown ;

   if (error_state & badbit)        { error_state = badbit ; errmsg = _err_stream_bad ; }
   else if (error_state & eofbit)   { error_state = eofbit ; errmsg = _err_stream_eof ; }
   else if (error_state & failbit)  { error_state = failbit ; errmsg = _err_stream_failed ; }

   throw failure_exception(errmsg, error_state) ;
}

void raw_ios::exceptions(unsigned state)
{
   _exceptions = state ;
   if (is_open())
      _check_throw() ;
}

void raw_ios::process_exception()
{
   if (!((_state |= external_state()) & raw_ios::statebit))
       _state = badbit ;
   if (_state & exceptions())
      throw ;
}

void raw_ios::close() throw()
{
   if (is_open())
   {
      do_close() ;
      setstate_nothrow(closebit, true) ;
   }
}

void raw_ios::do_close()
{}

raw_ios::pos_type raw_ios::seekoff(off_type, seekdir)
{
   return pos_type(-1) ;
}

/*******************************************************************************
 raw_istream
*******************************************************************************/
raw_istream &raw_istream::read(void *buffer, size_t size)
{
   _last_read = 0 ;
   if (rdstate() & (eofbit|badbit))
      raw_ios::resetstate(rdstate()) ;
   else
   {
      setstate_nothrow(failbit, false) ;
      if (!size)
         return *this ;
      try {
         // Temporary mask all exceptions
         sentry guard(*this) ;
         _last_read = do_read(buffer, size) ;
         setstate_nothrow(external_state(), true) ;
         NOXCHECK(_last_read <= size) ;
      }
      catch(...)
      {
         process_exception() ;
         return *this ;
      }
      // Throw pending exceptions, if present
      unsigned newstate = rdstate() ;
      if (_last_read < size)
         newstate |= failbit ;
      if (newstate)
         resetstate(newstate) ;
   }
   return *this ;
}

/*******************************************************************************
 raw_ostream
*******************************************************************************/
raw_ostream &raw_ostream::write(const void *buffer, size_t size)
{
   _last_written = 0 ;
   if (rdstate() & ~failbit)
      raw_ios::resetstate(rdstate()) ;
   else
   {
      setstate_nothrow(failbit, false) ;
      if (!size)
         return *this ;
      try {
         // Temporary mask all exceptions
         sentry guard(*this) ;
         _last_written = do_write(buffer, size) ;
         setstate_nothrow(external_state(), true) ;
         NOXCHECK(_last_written <= size) ;
      }
      catch(...)
      {
         process_exception() ;
         return *this ;
      }
      // Throw pending exceptions, if present
      resetstate(rdstate() | (_last_written < size ? (unsigned)failbit : 0)) ;
   }
   return *this ;
}

/*******************************************************************************
 raw_fstreambase
*******************************************************************************/
template<class RawStream>
raw_ios::pos_type raw_fstreambase<RawStream>::seekoff(raw_ios::off_type offs,
                                                      raw_ios::seekdir dir)
{
   int whence ;
   switch (dir)
   {
      case raw_ios::cur: whence = SEEK_CUR ; break ;
      case raw_ios::beg:
         if (!offs)
            return std::ftell(fptr()) ;
         whence = SEEK_SET ;
         break ;
      case raw_ios::end: whence = SEEK_END ; break ;

      default: return raw_ios::pos_type(-1) ;
   }
   return fseek_i(fptr(), offs, whence) ? -1 : std::ftell(fptr()) ;
}

template<class RawStream>
void raw_fstreambase<RawStream>::do_close()
{
   if (_owns)
      std::fclose(_file) ;
   _file = NULL ;
}

template class raw_fstreambase<raw_istream> ;
template class raw_fstreambase<raw_ostream> ;

/*******************************************************************************
 raw_ifstream
*******************************************************************************/
size_t raw_ifstream::do_read(void *buffer, size_t size)
{
   return std::fread(buffer, 1, size, fptr()) ;
}

/*******************************************************************************
 raw_ofstream
*******************************************************************************/
size_t raw_ofstream::do_write(const void *buffer, size_t size)
{
   return std::fwrite(buffer, 1, size, fptr()) ;
}

/*******************************************************************************
 raw_imemstream
*******************************************************************************/
raw_ios::pos_type raw_imemstream::seekoff(off_type offs, seekdir dir)
{
   setstate(eofbit, false) ;
   const pos_type from =
      dir == cur ? _pos : (dir == beg ? pos_type() : (pos_type)_size) ;
   return _pos = midval<pos_type>(0, _size, from + offs) ;
}

size_t raw_imemstream::do_read(void *buffer, size_t bufsize)
{
   NOXCHECK((size_t)_pos <= _size) ;
   const size_t remain = _size - _pos ;
   if (remain >= bufsize)
   {
      memcpy(buffer, gptr(), bufsize) ;
      _pos += bufsize ;
      return bufsize ;
   }
   memcpy(buffer, gptr(), remain) ;
   _pos = _size ;
   setstate_nothrow(eofbit, true) ;
   return remain ;
}

/*******************************************************************************
 raw_omemstream
*******************************************************************************/
raw_ios::pos_type raw_omemstream::seekoff(off_type offs, seekdir dir)
{
   const pos_type from =
      dir == cur ? _pos : (dir == beg ? pos_type() : _endpos) ;
   const pos_type newpos = from + offs ;
   return
      newpos <= maxsize() || expand(newpos) ? (_pos = newpos) : pos_type(-1) ;
}

size_t raw_omemstream::do_write(const void *buffer, size_t bufsize)
{
   size_t newpos = _pos + bufsize ;
   if (newpos > _buffer.size())
   {
      const bool toobig = newpos > maxsize() ;
      if (toobig)
         newpos = maxsize() ;
      if (!expand(newpos) || toobig)
      {
         setstate_nothrow(failbit, true) ;
         newpos = _buffer.size() ;
         NOXCHECK(newpos < _pos + bufsize) ;
      }
   }
   const size_t written_size = newpos - _pos ;
   memmove(pcomn::padd(data(), _pos), buffer, written_size) ;
   if ((_pos = newpos) > _endpos)
      _endpos = _pos ;
   return written_size ;
}

void raw_omemstream::do_close()
{
   _buffer.reset() ;
   _pos = 0 ;
}

bool raw_omemstream::expand(size_t newsize)
{
   const size_t prevsize = size() ;
   try {
      if (_buffer.grow(newsize))
      {
         memset(pcomn::padd(data(), prevsize), 0, _buffer.size() - prevsize) ;
         return true ;
      }
   }
   catch (const std::bad_alloc &) {}
   return false ;
}

/*******************************************************************************
 raw_icachestream
*******************************************************************************/
raw_icachestream::raw_icachestream(raw_istream *source_stream, bool owns_source,
                                   off_type stated_init_pos) :
   _source(ensure_nonzero<std::invalid_argument>
           (source_stream, "Attempt to initialize reference counter delegate with NULL aggregate pointer")),
   _bufstart(stated_init_pos),
   _caching(0),
   _owns_source(owns_source)
{
   if (stated_init_pos < 0)
   {
      std::unique_ptr<raw_istream> guard (owns_source ? source_stream : (raw_istream *)NULL) ;
      // Please note this is the _only_ place where tell() can be called
      // (and _only_ if stated_init_pos < 0). Otherwise the only method called on the
      // source stream is read()
      _bufstart = source().tell() ;
      guard.release() ;
   }
   _bufend = _position = _bufstart ;
}

raw_ios::pos_type raw_icachestream::seekoff(off_type offset, seekdir origin)
{
   off_type newpos = offset ;
   // Note that 'end' origin is prohibited, since the source string is unidirectional
   // by definition and thus to seek from its end we should cache it completely.
   switch (origin)
   {
      case cur:
         // Shortcut for the fast 'tell()' call
         if (!offset)
            return _position ;
         newpos += _position ;
      case beg:
         break ;

      case end:
         read_forward(NULL, (size_t)-1) ;
         newpos += cache_endpos() ;
         break ;

      default:
         PCOMN_FAIL("Invalid 'origin' parameter to raw_icachestream::seekoff.") ;
   }
   if (newpos > (off_type)cache_endpos())
      read_forward(NULL, newpos - cache_endpos()) ;
   if ((pos_type)newpos <= cache_endpos())
      return _position = newpos ;
   return (pos_type)-1 ;
}

size_t raw_icachestream::do_read(void *buffer, size_t bufsize)
{
   NOXCHECK(!caching() || _position <= cache_endpos()) ;
   if (_position < cache_startpos())
      // Attempt to read a cached string from the position before the cached region.
      return 0 ;

   const size_t from_cache = std::min<size_t>(cache_endpos() - _position, bufsize) ;
   const size_t from_stream = bufsize - from_cache ;
   memcpy(buffer, pcomn::padd(_cache.data(), _position - cache_startpos()), from_cache) ;
   const size_t actually_read = read_forward(pcomn::padd(buffer, from_cache), from_stream) ;
   const size_t read_bytes = from_cache + actually_read ;
   _position += read_bytes ;
   return read_bytes ;
}

size_t raw_icachestream::read_chunk(void *buffer, size_t bufsize)
{
   const size_t actually_read = source().read(buffer, bufsize).last_read() ;
   const pos_type new_bufend = _bufend + actually_read ;
   if (caching())
   {
      const size_t oldsize = _bufend - _bufstart ;
      memcpy(pcomn::padd(_cache.grow(oldsize + actually_read), oldsize), buffer, actually_read) ;
   }
   else
      _bufstart = new_bufend ;
   _bufend = new_bufend ;
   return actually_read ;
}

size_t raw_icachestream::read_forward(void *buffer, size_t bufsize)
{
   if (!bufsize)
      return 0 ;

   if (!caching())
      _cache.reset() ;

   if (buffer)
      return read_chunk(buffer, bufsize) ;
   char tmpbuf[4096] ;
   size_t read_so_far = 0 ;
   size_t chunksize ;
   do {
      chunksize = read_chunk(tmpbuf, std::min<size_t>(bufsize - read_so_far, sizeof tmpbuf)) ;
      read_so_far += chunksize ;
   }
   while (chunksize == sizeof tmpbuf && read_so_far < bufsize) ;
   return read_so_far ;
}

void raw_icachestream::do_close()
{
   _caching = 0 ;
   _bufstart = _bufend = _position = 0 ;
   _cache.reset() ;
   if (_owns_source)
   {
      NOXCHECK(_source) ;
      _owns_source = false ;
      source().close() ;
      delete _source ;
   }

}

} // end of namespace pcomn
