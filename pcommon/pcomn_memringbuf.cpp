/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_memringbuf.cpp
 COPYRIGHT    :   Yakov Markovitch, 2022

 DESCRIPTION  :   Fixed-size single-threaded ring memory buffer.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   10 May 2022
*******************************************************************************/
#include "pcomn_memringbuf.h"
#include "pcomn_bitops.h"
#include "pcomn_except.h"

#include <unistd.h>
#include <sys/mman.h>

namespace pcomn {

static constexpr size_t PAGESIZE = sizeof(mempage_t) ;

PCOMN_STATIC_CHECK(bitop::tstpow2(PAGESIZE)) ;

/*******************************************************************************
 memory_ring_buffer
*******************************************************************************/
memory_ring_buffer::memory_ring_buffer(size_t capacity_bytes) :
    _capacity_mask(bitop::round2z((capacity_bytes + PAGESIZE - 1)/PAGESIZE) * PAGESIZE - 1)
{
    if (!capacity())
        return ;

    // While memfd_create has been introduced long long ago (kernel 3.17) it's quite
    // likely absent in glibc; don't rely on glibc then.
    auto syscall_memfd_create = [](const char *name, int flags)
    {
        constexpr int memfd_create_syscall_num = 319 ;
        return ::syscall(memfd_create_syscall_num, name, flags) ;
    } ;

    // Reserve address space of 2*capacity() bytes
    _memory = (uint8_t *)mmap(NULL, 2*capacity(), PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) ;

    PCOMN_CHECK_POSIX((intptr_t)_memory, "Cannot reserve %zu bytes of address space for memory ring", 2*capacity()) ;

    // Create anonymous memory file of capacity() bytes.
    // Note the file name passed to memfd_create() needn't be unique.
    const int memfd = syscall_memfd_create("memring", 0) ;

    finalizer on_failure ([this,memfd]
    {
        if (memfd >= 0)
            ::close(memfd) ;
        munmap(_memory, 2*capacity()) ;
    }) ;

    PCOMN_CHECK_POSIX(memfd, "Cannot create a memory file descriptor for a memory ring") ;

    // Allocate the actual memory: extend the memory file to capacity()
    PCOMN_CHECK_POSIX(ftruncate(memfd, capacity()),
                      "Cannot extend a memory file for a memory ring to %zu bytes", capacity()) ;

    // Map the memory file twice, one after one contigously
    for (uint8_t * const ptr: {_memory, _memory + capacity()})
    {
        PCOMN_CHECK_POSIX((intptr_t)mmap(ptr, capacity(), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_FIXED, memfd, 0),
                          "Cannot map ring buffer descriptor %d of size %zu to %p", memfd, capacity(), ptr) ;
    }

    on_failure.release() ;

    ::close(memfd) ;
}

memory_ring_buffer::~memory_ring_buffer()
{
    if (_memory)
        munmap(_memory, 2*capacity()) ;
}

__noreturn void memory_ring_buffer::allocation_failed(size_t bytes) const
{
    PCOMN_THROW_MSGF(std::runtime_error,
                     "Attempt to allocate %zu bytes from the memory ring at %p of capacity %zu with %zu bytes available",
                     bytes, _memory, capacity(), available_capacity()) ;
}

} // end of namespace pcomn
