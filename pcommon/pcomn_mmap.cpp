/*-*- tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel"; c-file-offsets: ((innamespace . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_mmap.cpp
 COPYRIGHT    :   Yakov Markovitch, 2001-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Memory-mapped file implementation for Win32 platforms

 CREATION DATE:   29 Jan 2001
*******************************************************************************/
#include <pcomn_mmap.h>
#include <pcomn_except.h>
#include <pcomn_integer.h>
#include <pcomn_handle.h>
#include <pcomn_sys.h>
#include <pcomn_unistd.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

static filesize_t pagemask()
{
   static const filesize_t mask = pcomn::bitop::getrzbseq(pcomn::sys::pagesize()) ;
   return mask ;
}

static inline void *aligned_pointer(void *pointer)
{
   return (void *)((uintptr_t)pointer & ~pagemask()) ;
}

static inline uintptr_t pointer_offset(const void *pointer)
{
   return ((uintptr_t)pointer & pagemask()) ;
}

/*******************************************************************************
 Platform-dependent code
*******************************************************************************/
#include PCOMN_PLATFORM_HEADER(pcomn_mmap.cc)

/*******************************************************************************
 Platform-independent code
*******************************************************************************/
namespace pcomn {

/*******************************************************************************
 PMemMappedFile
*******************************************************************************/
intptr_t PMemMappedFile::_mmfile_t::get_handle(const char *filename)
{
   int protmode = 0 ;
   int smode = 0 ;
   if (one_of<O_WRONLY, O_RDWR>::is(_mode))
   {
      protmode = O_CREAT ;
      smode = S_IWRITE ;
   }
   return get_handle
      (fd_safehandle(ensure_ge<system_error>
                     (open(filename, _mode|protmode|O_BINARY, smode|S_IREAD), 0)).handle()) ;
}

/*******************************************************************************
 PMemMapping
*******************************************************************************/
void *PMemMapping::ensure_map_file(filesize_t from, unsigned mode)
{
   // Align the beginning of mapping to a page boundary
   const filesize_t aligned_from = from & ~pagemask() ;
   // Normalize mapping open mode
   const unsigned normalized_mode =
      (mode & O_RDWR) ? O_RDWR : ((mode & O_WRONLY) ? O_WRONLY : O_RDONLY) ;

   ensure_precondition
      // O_RDWR file is "universal" and compatible with any mapping mode
      (filemode() == O_RDWR || normalized_mode == filemode(),
       "Protection modes of a memory mapped file and the mapping are incompatible") ;

   // Adjust the mapping size
   if ((_sizedata != (size_t)-1) ||
       (_sizedata = requested_size()) != (size_t)-1 ||
       (_sizedata = full_file_size()) != (size_t)-1)

      ensure_le<std::out_of_range, filesize_t>
         (_sizedata - aligned_from, std::numeric_limits<size_t>::max(),
          "The mapping bounds are out of range") ;

   // Mapping of a zero-length file is considered valid but NULL
   if (!_sizedata)
      return NULL ;

   void * const aligned_pointer = ensure_nonzero<system_error>
      (map_file(aligned_from, normalized_mode),
       "Cannot create file mapping") ;
   return padd(aligned_pointer, from - aligned_from) ;
}

size_t PMemMapping::size() const
{
   NOXCHECK(_sizedata >= pointer_offset(_pointer)) ;
   return _sizedata - pointer_offset(_pointer) ;
}

} // end of namepace pcomn
