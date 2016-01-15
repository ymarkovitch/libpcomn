/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_binstream.cpp
 COPYRIGHT    :   Yakov Markovitch, 2007-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Simple binary I/O streams.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   10 Oct 2007
*******************************************************************************/
#include <pcomn_binstream.h>
#include <pcomn_diag.h>

namespace pcomn {

// Buffered stream buffer size sanity check
static inline size_t ensure_sane_capacity(size_t buf_capacity)
{
   conditional_throw<std::invalid_argument>((ssize_t)buf_capacity < 0,
                                            "Requested buffer buf_capacity is too big") ;
   return buf_capacity ;
}

static const size_t UNBOUNDED = (size_t)-1 ;

/*******************************************************************************
 binary_istream
*******************************************************************************/
std::string binary_istream::read()
{
   char buf[8192] ;
   if (!read_buf(*this, buf))
      return std::string() ;

   std::string result (buf + 0, buf + _last_read) ;
   for (size_t readcount ; (readcount = read_data(buf, sizeof buf)) != 0 ; result.append(buf + 0, buf + readcount))
   {
      _last_read += readcount ;
      _total_read += readcount ;
   }
   return result ;
}

/*******************************************************************************
 binary_ibufstream
*******************************************************************************/
binary_ibufstream::binary_ibufstream(binary_istream &s, size_t buf_capacity) :
   _unbuffered(s),
   _capacity(std::max(buf_capacity, (size_t)1)),
   _databound(UNBOUNDED),
   _buffer(_capacity > 1 ? new uint8_t[ensure_sane_capacity(buf_capacity) + 1] : _putback),
   _bufptr(_buffer + 1),
   _bufend(_bufptr)
{}

binary_ibufstream::binary_ibufstream(binary_istream *s, size_t buf_capacity) :
   _unbuffered(s),
   _capacity(std::max(buf_capacity, (size_t)1)),
   _databound(UNBOUNDED),
   _buffer(_capacity > 1 ? new uint8_t[ensure_sane_capacity(buf_capacity) + 1] : _putback),
   _bufptr(_buffer + 1),
   _bufend(_bufptr)
{}

binary_ibufstream::~binary_ibufstream()
{
   if (_buffer != _putback)
      delete [] _buffer ;
}

size_t binary_ibufstream::read_data(void *data, size_t size)
{
   NOXCHECK(data) ;

   size = bounded_size(size) ;

   // Get all available data from our internal buffer into the user buffer
   const size_t frombuf = get_available_frombuf(data, size) ;
   size_t size_so_far = frombuf ;

   if (const size_t remains_to_read = size - size_so_far)
      if (remains_to_read > min_unbuffered_size())
      {
         TRACEPX(PCOMN_BinaryStream, DBGL_LOWLEV, "Read " << size - size_so_far
                 << " unbufferred from " << PCOMN_TYPENAME(unbuffered_stream())) ;

         eof_guard guard (unbuffered_stream(), false) ;
         for (size_t prevsize = size_so_far ;
              (size_so_far += unbuffered_stream().read((char *)data + size_so_far, size - size_so_far)) < size &&
                 size_so_far != prevsize  ;
              prevsize = size_so_far) ;
      }
      else
         for (size_t prevsize = size_so_far ; ensure_buffer(), // Force the stream to fill buffer
                 (size_so_far += get_available_frombuf((char *)data + size_so_far, size - size_so_far)) < size &&
                 size_so_far != prevsize ;

              prevsize = size_so_far) ;

   return size_so_far ;
}

size_t binary_ibufstream::refill_buffer()
{
   NOXCHECK(_bufptr == _bufend) ;
   TRACEPX(PCOMN_BinaryStream, DBGL_HIGHLEV, "Refill buffer of size " << capacity()
           << " from " << PCOMN_TYPENAME(unbuffered_stream())) ;

   const size_t fillsize = bounded_size(capacity()) ;
   if (!fillsize)
      return 0 ;

   // Ensure that the underlying stream will not throw eof_error
   eof_guard guard (unbuffered_stream(), false) ;

   const size_t readsize = unbuffered_stream().read(bufstart(), fillsize) ;

   TRACEPX(PCOMN_BinaryStream, DBGL_HIGHLEV, readsize << " bytes placed into the buffer from "
           << PCOMN_TYPENAME(unbuffered_stream())) ;
   NOXCHECK(readsize <= fillsize) ;

   _bufptr = bufstart() ;
   _bufend = _bufptr + readsize ;
   return readsize ;
}

/*******************************************************************************
 binary_obufstream
*******************************************************************************/
binary_obufstream::binary_obufstream(binary_ostream &s, size_t buf_capacity) :
   _unbuffered(s),
   _buffer(ensure_sane_capacity(PCOMN_ENSURE_ARG(buf_capacity))),
   _bufptr(_buffer.begin())
{}

binary_obufstream::binary_obufstream(binary_ostream *s, size_t buf_capacity) :
   _unbuffered(s),
   _buffer(ensure_sane_capacity(PCOMN_ENSURE_ARG(buf_capacity))),
   _bufptr(_buffer.begin())
{}

binary_obufstream::~binary_obufstream()
{
   try {
      flush() ;
   }
   catch (const std::exception &x) {
      LOGPXWARN(PCOMN_BinaryStream,
                "Exception " << PCOMN_TYPENAME(x) << " in " << PCOMN_PRETTY_FUNCTION << ": " << x.what()) ;
   }
   catch (...) {
      LOGPXWARN(PCOMN_BinaryStream, "Unknown exception in " << PCOMN_PRETTY_FUNCTION) ;
   }
}

void binary_obufstream::flush_buffer()
{
   const uint8_t *end = _bufptr ;
   _bufptr = bufstart() ;
   for (const uint8_t *ptr = _bufptr ; ptr != end ;)
   {
      const size_t lastcount = unbuffered_stream().write(ptr, end - ptr) ;
      NOXCHECK(lastcount) ;
      NOXCHECK(lastcount <= (size_t)(end - ptr)) ;
      ptr += lastcount ;
   }
}

size_t binary_obufstream::write_data(const void *data, size_t size)
{
   const size_t available = available_capacity() ;
   size_t remains = size ;
   if (size > available)
   {
      memcpy(_bufptr, data, available) ;
      _bufptr = bufend() ;
      flush_buffer() ;
      remains -= available ;
      preinc(data, available) ;
      if (remains >= capacity())
      {
         unbuffered_stream().write(data, remains) ;
         return size ;
      }
   }
   memcpy(_bufptr, data, remains) ;
   _bufptr += remains ;
   return size ;
}

/*******************************************************************************
 Text I/O
*******************************************************************************/
std::string readline(binary_ibufstream &is, EOLMode eolmode)
{
   std::string result ;
   char buf[8192] ;
   char * const end = buf + sizeof buf ;
   char *out = buf ;
   if (eolmode == eolmode_LF)
      for (int c = 0 ; c != '\n' && (c = is.get()) != EOF ;)
      {
         *out = c ;
         if (++out == end)
         {
            result.append(buf + 0, end) ;
            out = buf ;
         }
      }
   else
      for (int c = 0 ; c != '\n' && (c = is.get()) != EOF ; *out++ = c)
      {
         if (out == end)
         {
            result.append(buf + 0, end) ;
            out = buf ;
         }
         if (c == '\r' && is.peek() == '\n')
            c = is.get() ;
      }
   return result.append(buf + 0, out) ;
}

} // end of namespace pcomn
