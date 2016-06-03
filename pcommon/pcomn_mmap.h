/*-*- mode: c++; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
#ifndef __PCOMN_MMAP_H
#define __PCOMN_MMAP_H
/*******************************************************************************
 FILE         :   pcomn_mmap.h
 COPYRIGHT    :   Yakov Markovitch, 2001-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Memory-mapped file objects

 CREATION DATE:   17 Jan 2001
*******************************************************************************/
#include <pcomn_def.h>
#include <pcomn_utils.h>
#include <pcomn_smartptr.h>
#include <pcomn_unistd.h>

#include <limits>

#include <stdio.h>
#include <fcntl.h>

namespace pcomn {

/*******************************************************************************
                     class PMemMappedFile

 Should be implemented by the platform:
  intptr_t _mmfile_t::get_handle(intptr_t file) ;
  intptr_t _mmfile_t::get_handle(const char *filename) ;
*******************************************************************************/
/** Platform-independent memory-mapped file class.
    This class provides an object for use by pcomn::PMemMapping, which is an
    actual memory-mapping.
*******************************************************************************/
class PMemMappedFile {
   public:
      PMemMappedFile() {}

      /// Constructor.
      /// @param file  the handle of a file to map.
      /// @param size  the length of a file part that can/should be maped:
      ///               @li <0  use the whole file;
      ///               @li >0  use the first @a size bytes of a file.
      ///               @li ==0 invalid value.
      /// @param mode  the protection flags from the <fcntl.h>:
      ///               @li O_RDONLY open the file in read-only mode;
      ///                   (memory can be read but can't be written);
      ///               @li O_RDWR open the file in read-write mode
      ///                   (memory can be read and written);
      ///               @li O_WRONLY open the file in write-only mode;
      ///                   on most platforms is equivalent to O_RDWR.
      ///
      /// @note If @a mode is either O_RDWR or O_WRONLY and @a size >0 and the size of a
      /// file is less than @a size, the file is expanded to fit the size. The file never
      /// shrinks. If @a mode is O_RDONLY, the file is not expanded.
      ///
      explicit PMemMappedFile(int file, filesize_t size = (filesize_t)-1,
                              unsigned mode = O_RDONLY) :
         _mmfile(new _mmfile_t(file, size, mode))
      {}

      explicit PMemMappedFile(const char *filename, filesize_t size = (filesize_t)-1,
                              unsigned mode = O_RDONLY) :
         _mmfile(new _mmfile_t(filename, size, mode))
      {}

#ifdef PCOMN_PL_WINDOWS
      /// Windows-specific constructor.
      /// Differs from the generic constructor in that the @a file parameter should
      /// be a HANDLE returned from Win32 file API (e.g. CreateFile).
      PMemMappedFile(void *file, filesize_t size = (filesize_t)-1,
                     unsigned mode = O_RDONLY) :
         // Set the sign bit of 'file' argument to indicate this is a Windows file handle
         _mmfile(new _mmfile_t((intptr_t)file | ~(~(uintptr_t()) >> 1), size, mode))
      {}
#endif

      explicit operator bool() const { return !!_mmfile ; }

      /// Get the OS file mapping handle.
      /// @return A file mapping descriptor, "native" for the platform.
      /// @note While this function returns a file descriptor on Unix (can be used
      /// in POSIX I/O functions, like read()), it returns a memory-mapped file HANDLE
      /// on Windows, returned by the CreateFileMapping call, which is suitable to pass
      /// into Win32 memory-mapped file APIs.
      intptr_t handle() const { return mmfile()._handle ; }

      // The initial requested size.
      /// @internal
      filesize_t requested_size() const { return mmfile()._reqsize ; }

      unsigned filemode() const { return mmfile()._mode ; }

      void swap(PMemMappedFile &other)
      {
         _mmfile.swap(other._mmfile) ;
      }
   private:
      struct _PCOMNEXP _mmfile_t : public PRefCount {
            _mmfile_t(intptr_t file, filesize_t size, unsigned mode) :
               _mode(normalize_mode(mode)),
               _reqsize(size),
               _handle(get_handle(file))
            {}

            _mmfile_t(const char *filename, filesize_t size, unsigned mode) :
               _mode(normalize_mode(mode)),
               _reqsize(size),
               _handle(get_handle(filename))
            {}

            ~_mmfile_t() ;

            const unsigned   _mode ;     /* File protection flags (read/write permissions, O_READ, O_WRITE) */
            const filesize_t  _reqsize ;  /* The initial size of file mapping */
            const intptr_t    _handle ;   /* File mapping descriptor (on Unix - file descriptor) */

         private:
            intptr_t get_handle(intptr_t file) ;
            intptr_t get_handle(const char *filename) ;

            static unsigned normalize_mode(unsigned mode)
            {
               return (mode & O_RDWR) ? O_RDWR : ((mode & O_WRONLY) ? O_WRONLY : O_RDONLY) ;
            }
      } ;

      shared_intrusive_ptr<const _mmfile_t> _mmfile ;

      const _mmfile_t &mmfile() const
      {
         NOXCHECK(_mmfile) ;
         return *_mmfile ;
      }
} ;

/*******************************************************************************
                     class PMemMapping

 Should be implemented by the platform:
   filesize_t PMemMapping::full_file_size() const ;
   void *PMemMapping::map_file(filesize_t aligned_from, unsigned normalized_mode) ;
   void PMemMapping::unmap_file() ;
*******************************************************************************/
/** Memory mapping.
*******************************************************************************/
class PMemMapping : public PMemMappedFile {
      typedef PMemMappedFile ancestor ;
      PCOMN_NONASSIGNABLE(PMemMapping) ;
   public:
      PMemMapping() : _sizedata(0), _pointer(NULL) {}

      PMemMapping(PMemMapping &&other) : PMemMapping()
      {
         swap(other) ;
      }

      /// Constructor; creates a memory mapping for a particular range of a file.
      /// @param mmfile a memory-mapped file which to create mapping on.
      /// @param from   the file position the mapping shall begin from.
      /// @param to     the file position the mapping shall extend to
      ///               (not inclusive, much like the end iterator in STL)
      ///               @li ((filesize_t)-1) map to the end of file.
      /// @param mode   the protection flags from the <fcntl.h>
      ///               @li O_RDONLY open the mapping in read-only mode
      ///                   (memory can be read but can't be written);
      ///               @li O_RDWR   open the mapping in read-write mode
      ///                   (memory can be read and written);
      ///               @li O_WRONLY on most systems, the same as O_RDWR;
      ///
      /// Neither @a from nor @a to shall exceed @a mmfile->size().
      ///
      /// @note While on most platforms a memory-mapping should be aligned to the pagesize
      /// boundary, the PMemMapping allows to specify any offset for @a from, ensuring
      /// that PMemMapping::data will point to the given offset (instead of the "real"
      /// beginning of a mapping, which is always at the page boundary).
      ///
      /// @note Please note using fileoff_t type for @a from and @a to instead of size_t. On some
      /// 32-bit systems files can be 64-bit, thus file offsets can be out of size_t
      /// limits. It is much safer to use fileoff_t, which always reflects the actual file
      /// offset type.
      PMemMapping(const PMemMappedFile &mmfile, filesize_t from, filesize_t to, unsigned mode = O_RDONLY) :
         ancestor(mmfile),
         _sizedata(to),
         _pointer(ensure_map_file(from, mode))
      {}

      /// Constructor that creates a memory mapping over the entire file.
      /// @param mmfile a memory-mapped file which to create mapping on.
      /// @param mode   the protection flags from the <fcntl.h>
      ///
      /// The resulting mapping starts from the beginning of @a mmfile and spans
      /// mmfile.size() bytes.
      PMemMapping(const PMemMappedFile &mmfile, unsigned mode = O_RDONLY) :
         ancestor(mmfile),
         _sizedata((size_t)-1),
         _pointer(ensure_map_file(0, mode))
      {}

      /// Constructor that opens a file and creates a memory mapping over its range.
      /// @param filename  the name of a file to open.
      /// @param from      the file position the mapping shall begin from.
      /// @param to        the file position the mapping shall extend to
      ///                  (not inclusive, much like the end iterator in STL).
      /// @param mode      the protection flags from the <fcntl.h>
      ///
      /// Both the file and the mapping are opened in @a mode. If @a mode is writable
      /// (either O_RDWR or O_WRONLY) and the opened file has size less than @a to,
      /// the file is automatically extended.
      PMemMapping(const char *filename, filesize_t from, filesize_t to, unsigned mode = O_RDONLY) :
         ancestor(filename, to, mode),
         _sizedata(to),
         _pointer(ensure_map_file(from, mode))
      {}

      /// @overload
      PMemMapping(int fd, filesize_t from, filesize_t to, unsigned mode = O_RDONLY) :
         ancestor(fd, to, mode),
         _sizedata(to),
         _pointer(ensure_map_file(from, mode))
      {}

      /// Constructor that opens a file and creates a memory mapping over the whole file.
      /// @param filename  the name of a file to open.
      /// @param mode      the protection flags from the <fcntl.h>
      /// Both the file and the mapping are opened in @a mode.
      explicit PMemMapping(const char *filename, unsigned mode = O_RDONLY) :
         ancestor(filename, -1, mode),
         _sizedata((size_t)-1),
         _pointer(ensure_map_file(0, mode))
      {}

      /// @overload
      explicit PMemMapping(int fd, unsigned mode = O_RDONLY) :
         ancestor(fd, -1, mode),
         _sizedata((size_t)-1),
         _pointer(ensure_map_file(0, mode))
      {}

      ~PMemMapping()
      {
         if (_pointer)
            unmap_file() ;
      }

      PMemMapping &operator=(PMemMapping &&other)
      {
         PMemMapping dummy (std::move(other)) ;
         swap(dummy) ;
         return *this ;
      }

      /// Get the pointer to the beginning of memory map.
      ///
      /// PMemMapping::data retruns NULL if and only if the mapping is default
      /// constructed, since it is impossible to construct an invalid memory mapping
      /// object (on such attempt the constructor throws an exception, hence if we have a
      /// nondefault-constructed memory mapping object it @em is valid by design),
      const void *data() const { return _pointer ; }
      /// @overload
      void *data() { return _pointer ; }

      /// Get the pointer to the beginning of memory map, casted to a char pointer.
      /// @return The same address as PMemMapping::data, only casted to char pointer.
      /// @see data
      const char *cdata() const { return static_cast<const char *>(data()) ; }
      /// @overload
      char *cdata() { return static_cast<char *>(data()) ; }

      _PCOMNEXP size_t size() const ;

      void swap(PMemMapping &other)
      {
         ancestor::swap(other) ;
         std::swap(_sizedata, other._sizedata) ;
         std::swap(_pointer, other._pointer) ;
      }

      friend bool operator==(const PMemMapping &x, nullptr_t) { return !x.data() ; }
      friend bool operator==(nullptr_t, const PMemMapping &x) { return !x.data() ; }
      friend bool operator!=(const PMemMapping &x, nullptr_t) { return x.data() ; }
      friend bool operator!=(nullptr_t, const PMemMapping &x) { return x.data() ; }

   private:
      size_t _sizedata ;
      void * _pointer ;

      _PCOMNEXP void *ensure_map_file(filesize_t from, unsigned mode) ;
      _PCOMNEXP void unmap_file() ;

      void *map_file(filesize_t aligned_from, unsigned normalized_mode) ;
      filesize_t full_file_size() const ;
} ;

/*******************************************************************************
 Define global swap()
*******************************************************************************/
PCOMN_DEFINE_SWAP(PMemMappedFile) ;
PCOMN_DEFINE_SWAP(PMemMapping) ;

/*******************************************************************************
 Buffer traits
*******************************************************************************/
template<typename T> struct membuf_traits ;
template<>
struct membuf_traits<PMemMapping> {
      typedef PMemMapping type ;
      static size_t size(const PMemMapping &buffer) { return buffer.size() ; }
      static const void *cdata(const PMemMapping &buffer) { return buffer.data() ; }
      static void *data(PMemMapping &buffer) { return buffer.data() ; }
} ;
} // end of namespace pcomn

#endif /* __PCOMN_MMAP_H */
