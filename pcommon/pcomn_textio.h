/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_TEXTIO_H
#define __PCOMN_TEXTIO_H
/*******************************************************************************
 FILE         :   pcomn_textio.h
 COPYRIGHT    :   Yakov Markovitch, 2007-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Universal newline text reader/writer

 PROGRAMMED BY:   Vadim Hohlov, Yakov Markovitch
 CREATION DATE:   10 Apr 2007
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcomn_integer.h>
#include <pcomn_iodevice.h>

#include <utility>
#include <limits>

namespace pcomn {

/******************************************************************************/
/** End-of-line type of a file.
*******************************************************************************/
enum EOLType {
   eol_Undefined = 0x0,
   eol_LF        = 0x1, /** Unix (including Mac OSX/Darwin), "\n" */
   eol_CR        = 0x2, /** Mac OS old, "\r" */
   eol_CRLF      = 0x4  /** DOS/Windows, "\r\n" */
} ;

const EOLType eol_Unix = eol_LF ;
const EOLType eol_Windows = eol_CRLF ;
const EOLType eol_MacOld = eol_CR ;

/// The native newline for the platform.
const EOLType eol_Native =
#if   defined(PCOMN_PL_UNIX)
   eol_Unix
#elif defined(PCOMN_PL_WINDOWS)
   eol_Windows
#elif defined(PCOMN_PL_MAC)
   eol_MacOld
#else
#error "Unknown platform"
#endif
   ;

/******************************************************************************/
/** A wrapper class around pcomn::io::reader that provides for both line by line
reading of text data and end-of-line translation.

Provides universal newline support, i.e. gracefully handles files, which either have
end-of-lines not native for the platform or even have "mixed" newlines.
All newlines are converted to '\n' character.
*******************************************************************************/
class _PCOMNEXP text_reader {
      template<typename Char, typename Size>
      static Size buf_size(const std::pair<Char *, Size> *device)
      {
         return device->second ;
      }
      static ssize_t buf_size(void *) { return -1 ; }

   public:
      /// An empty virtual destructor.
      /// Provided in order to allow proper derived classes destruction.
      virtual ~text_reader()
      {}

      /// Get the description of newline types encountered during reading.
      /// @return The bit mask consisting of EOLType flags.
      /// @note There can be several flags set at the same time, since the input
      ///       can have "mixed" newlines.
      unsigned eoltype() const { return _eoltype ; }

      /// Indicates whether the end-of-file is reached.
      bool eof() const { return _lastchar < 0 ; }

      int getchar()
      {
         const int last = _lastchar ;
         _lastread_txt = _lastread_bin = 0 ;
         int c = get_char() ;
         _lastchar = c ;
         if (c < 0)
            return c ;
         ++_lastread_bin ;
         if (c == '\n')
         {
            if (last == '\r')
            {
               _eoltype |= eol_CRLF ;
               return getchar() ;
            }
            _eoltype |= eol_LF ;
         }
         else if (c == '\r')
            c = '\n' ;
         else if (last == '\r')
            _eoltype |= eol_CR ;
         ++_lastread_txt ;
         return c ;
      }

      size_t charcount() const { return _lastread_txt ; }

      size_t bytecount() const { return _lastread_bin ; }

      template<typename OutputDevice>
      typename io::enable_if_writable<OutputDevice, size_t>::type
      read(OutputDevice &device)
      {
         return read_buffer(device, false) ;
      }

      /// Read the whole data from the source device, as a string.
      template<typename S>
      typename enable_if_strchar<S, char, S>::type read()
      {
         S result ;
         read_data(result, -1, false) ;
         return result ;
      }

      size_t read(char *buf, size_t bufsize)
      {
         std::pair<char *, size_t> output_device(buf, bufsize) ;
         return read_buffer(output_device, false) ;
      }

      template<size_t n>
      size_t read(char (&buf)[n]) { return read(buf, n) ; }

      /// Read the next line from the source device into an output device.
      /// Retains newline.
      template<typename OutputDevice>
      typename io::enable_if_writable<OutputDevice, size_t>::type
      readline(OutputDevice &device)
      {
         return read_buffer(device, true) ;
      }

      /// Read the next line from the source device into a character buffer.
      /// Retains newline.
      size_t readline(char *buf, size_t bufsize)
      {
         std::pair<char *, size_t> output_device(buf, bufsize) ;
         return read_buffer(output_device, true) ;
      }

      template<size_t n>
      size_t readline(char (&buf)[n]) { return readline(buf, n) ; }

      /// Read the next line from the source device, as a string.
      /// Retains newline.
      template<typename S>
      typename enable_if_strchar<S, char, S>::type readline()
      {
         S result ;
         read_data(result, -1, true) ;
         return result ;
      }

   protected:
      text_reader() :
         _eoltype(eol_Undefined),
         _lastread_txt(0),
         _lastread_bin(0),
         _lastchar(0)
      {}

   private:
      unsigned _eoltype ;
      size_t   _lastread_txt ;
      size_t   _lastread_bin ;
      int      _lastchar ;

      virtual int get_char() = 0 ;

      template<typename OutputDevice>
      size_t read_buffer(OutputDevice &buf, bool single_line)
      {
         const ssize_t bufsize = buf_size(&buf) ;
         if ((size_t)bufsize > 1)
            return read_data(buf, bufsize - 1, single_line) ;
         _lastread_txt = _lastread_bin = 0 ;
         return 0 ;
      }

      template<typename OutputDevice>
      size_t read_data(OutputDevice &result, ssize_t outsize, bool single_line) ;

      template<typename OutputDevice>
      bool put_buffer(OutputDevice &result, const char *begin, const char *end)
      {
         NOXCHECK(end > begin) ;
         const ssize_t written = io::writer<OutputDevice>::write(result, begin, end) ;
         if (written > 0)
         {
            _lastread_txt += written ;
            return true ;
         }
         return false ;
      }
} ;


/*******************************************************************************
 text_reader
*******************************************************************************/
template<typename OutputDevice>
size_t text_reader::read_data(OutputDevice &result, ssize_t outsize, bool single_line)
{
   char buf[2048] ;
   int c = 0 ;
   char *bufptr = buf ;
   char * const bufend = buf + sizeof buf ;

   _lastread_txt = _lastread_bin = 0 ;
   while (outsize && (c = get_char()) >= 0)
   {
      ++_lastread_bin ;
      if (_lastchar == '\r')
         if (c == '\n')
         {
            _lastchar = '\n' ;
            // '\n' immediately follows '\r' (_prevcharcr is true)
            _eoltype |= eol_CRLF ;
            continue ;
         }
         else
            _eoltype |= eol_CR ;

      switch (_lastchar = c)
      {
         case '\r':
            c = '\n' ;
            break ;

         case '\n': _eoltype |= eol_LF ; break ;
      }

      --outsize ;
      *bufptr++ = c ;
      if (c == '\n' && single_line)
      {
         put_buffer(result, buf, bufptr) ;
         return _lastread_txt ;
      }

      if (bufptr == bufend)
         if (put_buffer(result, buf, bufend))
            bufptr = buf ;
         else
            break ;
   }
   if (c < 0)
   {
      if (_lastchar == '\r')
         // The last encountered character was '\r'
         _eoltype |= eol_CR ;
      _lastchar = -1 ;
   }
   if (bufptr != buf)
      put_buffer(result, buf, bufptr) ;
   return _lastread_txt ;
}

/******************************************************************************/
/** A wrapper class around pcomn::io::reader that provides proper end-of-line
 translation.
*******************************************************************************/
template<typename Device>
class universal_text_reader : public text_reader {
      typedef text_reader ancestor ;
   public:
      typedef typename ::pcomn::io::reader<Device>::device_type device_type ;

      explicit universal_text_reader(device_type device) :
         _device(device)
      {}

   private:
      device_type _device ;

      int get_char() { return io::reader<Device>::get_char(_device) ; }
} ;

/*******************************************************************************
                     struct newline_t
*******************************************************************************/
struct newline_t {
      newline_t(EOLType kind = eol_Native) :
         _type((EOLType)pcomn::bitop::getrnzb(kind & (eol_CRLF | eol_CR | eol_LF)))
      {
         _text[1] = _text[2] = 0 ;
         if (!(_type & (eol_CRLF | eol_CR)))
            _text[0] = '\n' ;
         else
         {
            _text[0] = '\r' ;
            if (_type == eol_CRLF)
               _text[1] = '\n' ;
         }
      }

      const EOLType  _type ;
      char           _text[3] ;
} ;


/*******************************************************************************
                     class text_writer
*******************************************************************************/
/** An abstract text writer that provides proper end-of-line translation.
    Please note that both write() and writeline() member functions return the
    number of "text" bytes written, not considering newline translation.
    So for the most cases (aside from write error) the return value of these
    functions is simply the length of a passed argument string (or the value of
    "count" argument, for flavours that accept count), so that behaviour is
    consistent.
    To get the number of bytes actually put into underlying device, one should
    use text_writer::outcount.
*******************************************************************************/
class _PCOMNEXP text_writer {
   public:
      /// An empty virtual destructor.
      /// Provided in order to allow proper derived classes destruction.
      virtual ~text_writer()
      {}

      /// Get the newline type of this writer.
      EOLType eoltype() const { return _newline._type ; }

      /// Get the total number of written source bytes so far, not considering
      /// newline translation.
      /// So, e.g., writing "foo\n" will increase this counter by 4 no matter
      /// which end-of-line type is in effect.
      size_t charcount() const { return _written_text ; }

      /// Get the total number of binary (target) bytes written so far.
      /// In contrast to text_writer::charcount, this functions considers newline
      /// translation, returning the actual nubmer of bytes written.
      /// So, e.g., writing "foo\n" will increase this counter by 4 for
      /// pcomn::eol_Unix and by 5 for pcomn::eol_Windows.
      size_t bytecount() const { return _written_binary ; }

      size_t writeline()
      {
         const ssize_t written =
            write_raw(_newline._text, _newline._text + (2 - !_newline._text[1])) ;

         if (written <= 0)
            return 0 ;

         ++_written_text ;
         _written_binary += written ;
         return 1 ;
      }

      template<typename S>
      typename enable_if_strchar<S, char, size_t>::type
      write(const S &buf, size_t count)
      {
         return write_data(pcomn::str::cstr(buf), count) ;
      }

      template<typename S>
      typename enable_if_strchar<S, char, size_t>::type
      write(const S &buf)
      {
         return write_data(pcomn::str::cstr(buf), pcomn::str::len(buf)) ;
      }

      template<typename S>
      typename enable_if_strchar<S, char, size_t>::type
      writeline(const S &buf, size_t count)
      {
         const size_t result = write(buf, count) ;
         return result < count ? result : result + writeline() ;
      }

      template<typename S>
      typename enable_if_strchar<S, char, size_t>::type
      writeline(const S &buf)
      {
         return writeline(buf, pcomn::str::len(buf)) ;
      }

   protected:
      explicit text_writer(EOLType newline) :
         _newline(newline),
         _written_text(0),
         _written_binary(0)
      {}

   private:
      const newline_t   _newline ;
      size_t            _written_text ;
      size_t            _written_binary ;

      bool translation_required() const { return !(eoltype() & eol_LF) ; }

      size_t write_directly(const char *begin, size_t count)
      {
         const ssize_t lastwritten =
            std::max(write_raw(begin, begin + count), (ssize_t)0) ;
         _written_text += lastwritten ;
         _written_binary += lastwritten ;
         return lastwritten ;
      }

      size_t write_data(const char *begin, size_t count)
      {
         return translation_required()
            ? write_translate(begin, count)
            : write_directly(begin, count) ;
      }

      size_t write_translate(const char *begin, size_t count) ;

      size_t count_inbytes(const char *begin, size_t outbytes_count) const ;

      virtual ssize_t write_raw(const char *begin, const char *end) = 0 ;
} ;

/*******************************************************************************
                     class universal_text_writer
*******************************************************************************/
/** A wrapper class around pcomn::io::writer that provides proper end-of-line
    translation.
*******************************************************************************/
template<typename Device>
class universal_text_writer : public text_writer {
      typedef text_writer ancestor ;
   public:
      typedef typename ::pcomn::io::writer<Device>::device_type device_type ;

      explicit universal_text_writer(device_type device, EOLType newline = eol_Native) :
         ancestor(newline),
         _device(device)
      {}

   private:
      device_type _device ;

      ssize_t write_raw(const char *begin, const char *end)
      {
         return ::pcomn::io::writer<Device>::write(_device, begin, end) ;
      }
} ;

namespace io {
/// Writer specialization for pcomn::text_writer.
template<>
struct writer<pcomn::text_writer> : public iodevice<pcomn::text_writer> {
   static ssize_t write(pcomn::text_writer &txtwriter, const char *from, const char *to)
   {
      NOXCHECK(from <= to) ;
      return txtwriter.write(from, to - from) ;
   }
} ;
} // end of namespace pcomn::io

} // end of namespace pcomn

#endif /* __PCOMN_TEXTIO_H */
