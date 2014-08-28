/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_mmap.cc
 COPYRIGHT    :   Yakov Markovitch, 2007-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Memory-mapped file platform-dependent code for UNIX.

 CREATION DATE:   18 May 2007
*******************************************************************************/
#include <pcomn_sys.h>
#include <sys/mman.h>

namespace pcomn {

/*******************************************************************************
 PMemMappedFile
*******************************************************************************/
PMemMappedFile::_mmfile_t::~_mmfile_t()
{
   NOXCHECK(_handle >= 0) ;
   NOXVERIFY(close(_handle) == 0) ;
}

intptr_t PMemMappedFile::_mmfile_t::get_handle(intptr_t file)
{
   struct stat buf ;
   ensure<system_error>(fstat(file, &buf) == 0) ;
   fd_safehandle dup_handle (ensure_gt<system_error>(dup(file), -1)) ;

   if (one_of<O_WRONLY, O_RDWR>::is(_mode) && _reqsize != (filesize_t)-1)
      ensure<system_error>
         // If the requested size if larger than the file's, try to expand the file
         (buf.st_size >= (ssize_t)_reqsize || ftruncate(dup_handle, _reqsize) == 0,
          "Cannot expand a memory-mapped file to the requested size") ;

   return dup_handle.release() ;
}

/*******************************************************************************
 PMemMapping::_mapping_t
*******************************************************************************/
filesize_t PMemMapping::full_file_size() const
{
   return ensure_ge<system_error>(sys::filesize(handle()), (off_t)0) ;
}

void *PMemMapping::map_file(filesize_t aligned_from, bigflag_t normalized_mode)
{
   NOXCHECK(_sizedata > aligned_from) ;
   NOXCHECK(!(aligned_from & pagemask())) ;
   // O_RDWR != (O_RDONLY | O_WRONLY)
   const int flags = normalized_mode == O_RDWR
      ? (PROT_WRITE | PROT_READ)
      : ((normalized_mode == O_RDONLY)
         ? PROT_READ
         : ((normalized_mode == O_WRONLY)
            ? PROT_WRITE
            : PROT_NONE)) ;

   void *result = mmap(NULL, _sizedata -= aligned_from, flags, MAP_SHARED, handle(), aligned_from) ;
   return result == MAP_FAILED ? NULL : result ;
}

void PMemMapping::unmap_file()
{
   NOXVERIFY(munmap(aligned_pointer(_pointer), _sizedata) == 0) ;
}

} // end of namespace pcomn
