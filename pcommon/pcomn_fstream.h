/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_FSTREAM_H
#define __PCOMN_FSTREAM_H
/*******************************************************************************
 FILE         :   pcomn_fstream.h
 COPYRIGHT    :   Yakov Markovitch, 2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Implementation of binary iostreams for POSIX file descriptors.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   27 Nov 2008
*******************************************************************************/
/** @file
  Implementation of binary iostreams for POSIX file descriptors.
*******************************************************************************/
#include <pcomn_iostream.h>
#include <pcomn_utils.h>
#include <pcomn_unistd.h>

namespace pcomn {

/// @cond
namespace detail {

struct posix_fd {
      /// Get the underlying file handle.
      int fd() const { return _fd ; }
      bool owned() const { return _owned ; }
      bool bad() const { return _fd < 0 ; }
      bool good() const { return !bad() ; }
      bool close() throw()
      {
         return _fd < 0 || !::close(pcomn::xchange(_fd, -1)) ;
      }

   protected:
      posix_fd(int fd, bool owned) :
         _fd(fd),
         _owned(owned)
      {}

      ~posix_fd() { owned() && close() ; }

   private:
      int         _fd ; /* File descriptor */
      const bool  _owned ;

      PCOMN_NONCOPYABLE(posix_fd) ;
} ;

} // end of namespace pcomn::detail
/// @endcond

/******************************************************************************/
/** binary_istream over POSIX file descriptor.
*******************************************************************************/
class _PCOMNEXP binary_ifdstream : public detail::posix_fd, public binary_istream {
   public:
      explicit binary_ifdstream(int fd, bool owned = true) :
         detail::posix_fd(fd, owned)
      {}

   protected:
      size_t read_data(void *buf, size_t size)
      {
         return PCOMN_ENSURE_POSIX(::read(fd(), buf, size), "read") ;
      }
} ;

/******************************************************************************/
/** binary_ostream over POSIX file descriptor.
*******************************************************************************/
class _PCOMNEXP binary_ofdstream : public detail::posix_fd, public binary_ostream {
   public:
      explicit binary_ofdstream(int fd, bool owned = true) :
         detail::posix_fd(fd, owned)
      {}

   protected:
      size_t write_data(const void *buf, size_t size)
      {
         return PCOMN_ENSURE_POSIX(::write(fd(), buf, size), "write") ;
      }

} ;

} // end of namespace pcomn


#endif /* __PCOMN_FSTREAM_H */
