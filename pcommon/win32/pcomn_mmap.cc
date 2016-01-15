/*-*- tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((innamespace . 0)(inclass . ++)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_mmap.cc
 COPYRIGHT    :   Yakov Markovitch, 2001-2016. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Memory-mapped file platform-dependent code for Win32 platform

 CREATION DATE:   29 Jan 2001
*******************************************************************************/
#include <windows.h>
#include <pcomn_integer.h>

namespace pcomn {

/*******************************************************************************
 PMemMappedFile
*******************************************************************************/
PMemMappedFile::_mmfile_t::~_mmfile_t()
{
   NOXCHECK(_handle && (HANDLE)_handle != INVALID_HANDLE_VALUE) ;
   NOXVERIFY(CloseHandle((HANDLE)_handle)) ;
}

intptr_t PMemMappedFile::_mmfile_t::get_handle(intptr_t file)
{
   const intptr_t hfile = file == -1 || file > std::numeric_limits<int>::max()
	   ? file
	   : (file < 0
          ? (file && (~(uintptr_t()) >> 1))
         : _get_osfhandle((int)file)) ;

   if (hfile == -1)
      throw system_error("Attempt to open memory mapping on a bad file handle", EBADF) ;

   const DWORD protection =
      one_of<O_WRONLY, O_RDWR>::is(_mode) ? PAGE_READWRITE : PAGE_READONLY ;
   LARGE_INTEGER sz ;
   sz.QuadPart = _reqsize == (filesize_t)-1 ? 0 : _reqsize ;
   return ensure_nonzero<system_error>
      ((intptr_t)CreateFileMappingA((HANDLE)hfile, NULL, protection, sz.HighPart, sz.LowPart, NULL)) ;
}

/*******************************************************************************
 PMemMapping
*******************************************************************************/
filesize_t PMemMapping::full_file_size() const
{
   return (filesize_t)-1 ;
}

void *PMemMapping::map_file(filesize_t aligned_from, bigflag_t normalized_mode)
{
   NOXCHECK(_sizedata > aligned_from) ;
   NOXCHECK(!(aligned_from & pagemask())) ;
   // O_RDWR is _not_ a bit combination of O_RDONLY and O_WRONLY
   const DWORD protection =
      one_of<O_WRONLY, O_RDWR>::is(normalized_mode) ? FILE_MAP_WRITE : FILE_MAP_READ ;
   const filesize_t size = _sizedata == (size_t)-1 ? 0 : _sizedata ;
   LARGE_INTEGER offset ;
   // When requested to map the whole file, MapViewOfFile will ignore the offset.
   if (size)
   {
      offset.QuadPart = aligned_from ;
      _sizedata = 0 ;
   }
   else
   {
      offset.QuadPart = 0 ;
      _sizedata = aligned_from ;
   }
   return
      pcomn::padd(MapViewOfFile((HANDLE)handle(), protection, offset.HighPart, offset.LowPart, size),
                  _sizedata) ;
}

void PMemMapping::unmap_file()
{
   NOXVERIFY(UnmapViewOfFile(aligned_pointer(_pointer))) ;
}

} // end of namepace pcomn
