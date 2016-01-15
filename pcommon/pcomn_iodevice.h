/*-*- mode:c++;tab-width:3;indent-tabs-mode:nil;c-file-style:"ellemtel";c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_IODEVICE_H
#define __PCOMN_IODEVICE_H
/*******************************************************************************
 FILE         :   pcomn_iodevice.h
 COPYRIGHT    :   Yakov Markovitch, 2007-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Abstract input and output devices, supporting character
                  operations.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   18 Jul 2007
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcomn_meta.h>
#include <pcomn_unistd.h>
#include <pcomn_strslice.h>

#include <cstdio>
#include <iostream>

namespace pcomn {
namespace io {

/******************************************************************************/
/** Base class for both pcomn::io::reader and pcomn::io::writer specializations.

 Provides device_type typedef.
*******************************************************************************/
template<typename Device>
struct iodevice {
      typedef Device device_type ;
} ;

/*******************************************************************************
 The family of "writer devices"
*******************************************************************************/
template<typename Device> struct writer ;

template<typename Device>
struct writer<Device &> : public writer<Device> {} ;

template<typename Device>
struct is_writable :
         bool_constant<std::is_base_of<iodevice<Device>, writer<Device> >::value ||
                       std::is_base_of<iodevice<Device &>, writer<Device> >::value> {} ;

template<typename Device>
struct is_writable<Device &> : is_writable<Device> {} ;

template<typename Device, typename T>
struct enable_if_writable : std::enable_if<is_writable<Device>::value, T> {} ;

/******************************************************************************/
/** Null writer: an infinite sink
*******************************************************************************/
template<>
struct writer<void *> {
   template<typename Char>
   static ssize_t write(void *, const Char *from, const Char *to)
   {
      NOXCHECK(from <= to) ;
      return sizeof(Char) * (to - from) ;
   }
} ;

inline writer<void *> get_writer(void * const *) { return writer<void *>() ; }

/******************************************************************************/
/** Writer specialization for ISO C stdio.
*******************************************************************************/
template<>
struct writer<std::FILE *> : public iodevice<std::FILE *> {
   template<typename Char>
   static ssize_t write(std::FILE *file, const Char *from, const Char *to)
   {
      NOXCHECK(from <= to) ;
      return std::fwrite(from, sizeof(Char), to - from, file) ;
   }
} ;

inline writer<std::FILE *> get_writer(std::FILE * const *) { return writer<std::FILE *>() ; }

/******************************************************************************/
/**  Writer specialization for POSIX file descriptors like open() returns.
*******************************************************************************/
template<>
struct writer<int> {
   template<typename Char>
   static ssize_t write(int fd, const Char *from, const Char *to)
   {
      NOXCHECK(from <= to) ;
      return ::write(fd, from, (unsigned)((to - from) * sizeof(Char))) ;
   }
} ;

inline writer<int> get_writer(int const *) { return writer<int>() ; }

/******************************************************************************/
/** std::basic_string<> writer
*******************************************************************************/
template<typename C>
struct writer<std::basic_string<C> > : iodevice<std::basic_string<C> &> {
   // Write to a string
   static ssize_t write(std::basic_string<C> &s, const C *from, const C *to)
   {
      NOXCHECK(from <= to) ;
      s.append(from, to) ;
      return to - from ;
   }
} ;

/******************************************************************************/
/** Fixed-size character buffer writer
*******************************************************************************/
template<typename C>
struct writer<std::pair<C *, size_t> > : iodevice<std::pair<C *, size_t> > {
   // Write to a fixed-size buffer
   static ssize_t write(std::pair<C *, size_t> &s, const C *from, const C *to)
   {
      NOXCHECK(from <= to) ;
      // Ensure there is a place for both data and terminating zero.
      if (!s.second)
         return -1 ;
      const size_t szrequired = to - from ;
      const size_t szactual = std::min<size_t>(szrequired, s.second - 1) ;
      s.first = raw_copy(from, from + szactual, s.first) ;
      // Always teminate with zero.
      *s.first = C() ;
      s.second -= szactual ;
      return szactual ;
   }
} ;

/******************************************************************************/
/** Fixed-size memory buffer writer (actually, the same as string buffer writer)
*******************************************************************************/
template<>
struct writer<std::pair<void *, size_t> > : iodevice<std::pair<void *, size_t> > {
   static ssize_t write(std::pair<void *, size_t> &s, const char *from, const char *to)
   {
      typedef std::pair<char *, size_t> actual_device ;
      return writer<actual_device>::write(*reinterpret_cast<actual_device *>(&s), from, to) ;
   }
} ;

/******************************************************************************/
/** std::streambuf writer
*******************************************************************************/
template<>
struct writer<std::streambuf> : iodevice<std::streambuf &> {
   static ssize_t write(std::streambuf &stream, const char *from, const char *to)
   {
      NOXCHECK(from <= to) ;
      return stream.sputn(from, to - from) ;
   }
} ;

/******************************************************************************/
/** std::ostream writer
*******************************************************************************/
template<>
struct writer<std::ostream> : iodevice<std::ostream &> {
   static ssize_t write(std::ostream &stream, const char *from, const char *to)
   {
      NOXCHECK(from <= to) ;
      const size_t sz = to - from ;
      return !!stream.write(from, sz) * sz ;
   }
} ;

/*******************************************************************************
 The family of "reader devices"
*******************************************************************************/
template<typename Device> struct reader ;

template<typename Device>
struct reader<Device &> : public reader<Device> {} ;

template<typename Device>
struct is_readable :
         bool_constant<std::is_base_of<iodevice<Device>, reader<Device> >::value ||
                       std::is_base_of<iodevice<Device &>, reader<Device> >::value> {} ;

template<typename Device>
struct is_readable<Device &> : is_readable<Device> {} ;

template<typename Device, typename T>
struct enable_if_readable : std::enable_if<is_readable<Device>::value, T> {} ;

/******************************************************************************/
/** ISO C stdio FILE reader
*******************************************************************************/
template<>
struct reader<std::FILE *> : public iodevice<std::FILE *> {
   static ssize_t read(std::FILE *file, void *buf, size_t bufsize)
   {
      NOXCHECK(buf || !bufsize) ;
      return std::fread(buf, 1, bufsize, file) ;
   }
   static int get_char(std::FILE *file) { return getc(file) ; }
} ;

inline reader<std::FILE *> get_reader(std::FILE * const *) { return reader<std::FILE *>() ; }

/******************************************************************************/
/** std::streambuf reader
*******************************************************************************/
template<>
struct reader<std::streambuf> : public iodevice<std::streambuf &> {
   static ssize_t read(std::streambuf &stream, void *buf, size_t bufsize)
   {
      NOXCHECK(buf || !bufsize) ;
      return stream.sgetn(static_cast<char *>(buf), bufsize) ;
   }
   static int get_char(std::streambuf &stream) { return stream.snextc() ; }
} ;

/******************************************************************************/
/** std::istream reader
*******************************************************************************/
template<>
struct reader<std::istream> : public iodevice<std::istream &> {
   static ssize_t read(std::istream &stream, void *buf, size_t bufsize)
   {
      NOXCHECK(buf || !bufsize) ;
      return stream.read(static_cast<char *>(buf), bufsize).gcount() ; ;
   }
   static int get_char(std::istream &stream) { return stream.get() ; }
} ;

/*******************************************************************************
 The family of "string readers"
*******************************************************************************/
template<typename C>
struct char_reader : iodevice<std::pair<C *, size_t> > {
      typedef typename iodevice<std::pair<C *, size_t> >::device_type device_type ;

      static ssize_t read(device_type &device, void *buf, size_t bufsize)
      {
         const size_t sz = std::min(bufsize, device.second) ;
         memcpy(buf, device.first, sz) ;
         device.first += sz ;
         device.second -= sz ;
         return sz ;
      }

      static int get_char(device_type &device)
      {
         if (!device.second)
            return -1 ;
         --device.second ;
         return *device.first++ ;
      }

   private:
      PCOMN_STATIC_CHECK(std::is_pod<C>::value && sizeof(C) == 1) ;
} ;

template<>
struct reader<std::pair<const char *, size_t> > : char_reader<const char> {} ;

template<>
struct reader<std::pair<char *, size_t> > : char_reader<char> {} ;

template<typename C>
struct reader<basic_strslice<C> > : iodevice<basic_strslice<C> > {
      typedef typename iodevice<basic_strslice<C> >::device_type device_type ;

      static ssize_t read(device_type &device, void *buf, size_t bufsize)
      {
         const size_t sz = std::min(bufsize, device.size()) ;
         memcpy(buf, device.begin(), sz) ;
         device = device(sz) ;
         return sz ;
      }

      static int get_char(device_type &device)
      {
         if (device.empty())
            return -1 ;
         const C result = device.front() ;
         device = device(1) ;
         return result ;
      }

   private:
      PCOMN_STATIC_CHECK(sizeof(C) == 1) ;
} ;

template<typename C>
inline reader<basic_strslice<C> > get_reader(basic_strslice<C> *) { return reader<basic_strslice<C> >() ; }

/*******************************************************************************
 "Universal write function"
*******************************************************************************/
template<typename T, typename W>
inline ssize_t __write_data(T *device_ptr, writer<W>, const void *data, size_t size)
{
   ssize_t written = 0 ;
   for (const char *begin = static_cast<const char *>(data), *end = begin + size ; begin != end ;)
   {
      const ssize_t wcount = writer<W>::write(*device_ptr, begin, end) ;
      if (wcount <= 0)
      {
         if (!written)
            return wcount ;
         break ;
      }
      begin += wcount ;
      written += wcount ;
   }
   return written ;
}

template<typename T>
inline std::enable_if_t<std::is_pod<T>::value, ssize_t>
write_data(T device, const void *data, size_t size)
{
   return __write_data(&device, get_writer(&device), data, size) ;
}

template<typename T>
inline disable_if_t<std::is_pod<T>::value, ssize_t>
write_data(T &device, const void *data, size_t size)
{
   return __write_data(&device, get_writer(&device), data, size) ;
}

template<typename T>
inline std::enable_if_t<std::is_pod<T>::value, ssize_t>
write_data(T device, const strslice &slice)
{
   return __write_data(&device, get_writer(&device), slice.begin(), slice.size()) ;
}

template<typename T>
inline disable_if_t<std::is_pod<T>::value, ssize_t>
write_data(T &device, const strslice &slice)
{
   return __write_data(&device, get_writer(&device), slice.begin(), slice.size()) ;
}

/*******************************************************************************
 "Universal read function"
*******************************************************************************/
template<typename T, typename R>
inline ssize_t __read_data(T *device_ptr, reader<R>, void *data, size_t size)
{
   return reader<R>::read(*device_ptr, data, size) ;
}

template<typename T>
inline std::enable_if_t<std::is_pod<T>::value, ssize_t>
read_data(T device, void *buf, size_t size)
{
   return __read_data(&device, get_reader(&device), buf, size) ;
}

template<typename T>
inline disable_if_t<std::is_pod<T>::value, ssize_t>
read_data(T &device, void *buf, size_t size)
{
   return __read_data(&device, get_reader(&device), buf, size) ;
}

template<typename T>
inline std::enable_if_t<std::is_pod<T>::value, int>
get_char(T device) { return get_reader(&device).get_char(device) ; }

template<typename T>
inline disable_if_t<std::is_pod<T>::value, int>
get_char(T &device) { return get_reader(&device).get_char(device) ; }

} // end of namespace pcomn::io
} // end of namespace pcomn

/*******************************************************************************
 Place get_writer() and get_reader functions for std members into the namespace std
 to ensure ADL in write_data/read_data to work
*******************************************************************************/
namespace std {

template<typename C>
inline pcomn::io::writer<pair<C *, size_t> > get_writer(const pair<C *, size_t> *) { return pcomn::io::writer<pair<C *, size_t> >() ; }

template<typename C>
inline pcomn::io::writer<basic_string<C> > get_writer(basic_string<C> *) { return pcomn::io::writer<basic_string<C> >() ; }

inline pcomn::io::writer<streambuf> get_writer(streambuf *) { return pcomn::io::writer<streambuf>() ; }
inline pcomn::io::reader<streambuf> get_reader(streambuf *) { return pcomn::io::reader<streambuf>() ; }

inline pcomn::io::writer<ostream> get_writer(ostream *) { return pcomn::io::writer<ostream>() ; }
inline pcomn::io::reader<istream> get_reader(istream *) { return pcomn::io::reader<istream>() ; }

inline pcomn::io::reader<pair<const char *, size_t> > get_reader(pair<const char *, size_t> *)
{
   return pcomn::io::reader<pair<const char *, size_t> >() ;
}

inline pcomn::io::reader<pair<char *, size_t> > get_reader(pair<char *, size_t> *)
{
   return pcomn::io::reader<pair<char *, size_t> >() ;
}

}

#endif /* __PCOMN_IODEVICE_H */
