/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((inline-open . 0)(case-label . +)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_fmemopen.c

 * Copyright (c) 2004-2005
 *  Bruce Korb.  All rights reserved.
 * Copyright (c) 2010-2017
 *  Yakov Markovitch.  All rights reserved.
 *
 *
 * This code was inspired from software written by
 *   Hanno Mueller, kontakt@hanno.de
 * and completely rewritten by Bruce Korb, bkorb@gnu.org
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the name of any other contributor
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.

 DESCRIPTION  :   pcomn_fmemopen() - a version of a memory stream for FreeBSD
                  inspired by the glibc version, but completely rewritten and
                  extended by Bruce Korb - bkorb@gnu.org

 CREATION DATE:   12 Oct 2010
*******************************************************************************/
#include <pcomn_fmemopen.h>

#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(HAVE_FUNOPEN)
   typedef size_t  fmem_off_t;
   typedef fpos_t  seek_pos_t;

   typedef int     (cookie_read_function_t )(void *, char *, int);
   typedef int     (cookie_write_function_t)(void *, const char *, int);
   typedef fpos_t  (cookie_seek_function_t )(void *, fpos_t, int);
   typedef int     (cookie_close_function_t)(void *);

#else
#  error  OOPS
#endif

#define PROP_TABLE \
_Prop_( read,       "Read from buffer"        ) \
_Prop_( write,      "Write to buffer"         ) \
_Prop_( append,     "Append to buffer okay"   ) \
_Prop_( binary,     "byte data - not string"  ) \
_Prop_( create,     "allocate the string"     ) \
_Prop_( truncate,   "start writing at start"  ) \
_Prop_( allocated,  "we allocated the buffer" ) \
_Prop_( fixed_size, "writes do not append"    )

#ifndef NUL
# define NUL '\0'
#endif

#define _Prop_(n,s)   BIT_ID_ ## n,
typedef enum { PROP_TABLE BIT_CT } fmem_flags_e;
#undef  _Prop_

#define FLAG_BIT(n)  (1 << BIT_ID_ ## n)

typedef unsigned long mode_bits_t;
typedef unsigned char buf_bytes_t;

typedef struct fmem_cookie_s fmem_cookie_t;
struct fmem_cookie_s {
    mode_bits_t    mode;
    buf_bytes_t   *buffer;
    size_t         buf_size;    /* Full size of buffer */
    size_t         next_ix;     /* Current position */
    size_t         eof;         /* End Of File */
    size_t         pg_size;     /* size of a memory page.
                                   Future architectures allow it to vary
                                   by memory region. */
};

/* static forward declarations */
static int
fmem_getmode( const char *pMode, mode_bits_t *pRes );

static int
fmem_extend( fmem_cookie_t *pFMC, size_t new_size );

static ssize_t
fmem_read( void *cookie, void *pBuf, size_t sz );

static ssize_t
fmem_write( void *cookie, const void *pBuf, size_t sz );

static seek_pos_t
fmem_seek (void *cookie, fmem_off_t *p_offset, int dir);

static int
fmem_close( void *cookie );

static int
fmem_getmode( const char *pMode, mode_bits_t *pRes )
{
    if (pMode == NULL)
        return 1;

    switch (*pMode) {
    case 'a': *pRes = FLAG_BIT(write) | FLAG_BIT(append);
              break;
    case 'w': *pRes = FLAG_BIT(write) | FLAG_BIT(truncate);
              break;
    case 'r': *pRes = FLAG_BIT(read);
              break;
    default:  return EINVAL;
    }

    /*
     *  If someone wants to supply a "wxxbxbbxbb+" pMode string, I don't care.
     */
    for (;;) {
        switch (*++pMode) {
        case '+': *pRes |= FLAG_BIT(read) | FLAG_BIT(write);
                  if (pMode[1] != NUL)
                      return EINVAL;
                  break;
        case NUL: break;
        case 'b': *pRes |= FLAG_BIT(binary); continue;
        case 'x': continue;
        default:  return EINVAL;
        }
        break;
    }

    return 0;
}

static int
fmem_extend( fmem_cookie_t *pFMC, size_t new_size )
{
    size_t ns = (new_size + (pFMC->pg_size - 1)) & (~(pFMC->pg_size - 1));

    /*
     *  We can expand the buffer only if we are in append mode.
     */
    if (pFMC->mode & FLAG_BIT(fixed_size))
        goto no_space;

    if ((pFMC->mode & FLAG_BIT(allocated)) == 0) {
        /*
         *  Previously, this was a user supplied buffer.  We now move to one
         *  of our own.  The user is responsible for the earlier memory.
         */
        void* bf = malloc( ns );
        if (bf == NULL)
            goto no_space;

        memcpy( bf, pFMC->buffer, pFMC->buf_size );
        pFMC->buffer = bf;
        pFMC->mode  |= FLAG_BIT(allocated);
    }
    else {
        void* bf = realloc( pFMC->buffer, ns );
        if (bf == NULL)
            goto no_space;

        pFMC->buffer = bf;
    }

    /*
     *  Unallocated file space is set to zeros.  Emulate that.
     */
    memset( pFMC->buffer + pFMC->buf_size, 0, ns - pFMC->buf_size );
    pFMC->buf_size = ns;
    return 0;

 no_space:
    errno = ENOSPC;
    return -1;
}

static ssize_t
fmem_read( void *cookie, void *pBuf, size_t sz )
{
    fmem_cookie_t *pFMC = cookie;

    if (pFMC->next_ix + sz > pFMC->eof) {
        if (pFMC->next_ix >= pFMC->eof)
            return (sz > 0) ? -1 : 0;
        sz = pFMC->eof - pFMC->next_ix;
    }

    memcpy(pBuf, pFMC->buffer + pFMC->next_ix, sz);

    pFMC->next_ix += sz;

    return sz;
}


static ssize_t
fmem_write( void *cookie, const void *pBuf, size_t sz )
{
    fmem_cookie_t *pFMC = cookie;
    int add_nul_char;

    /*
     *  In append mode, always seek to the end before writing.
     */
    if (pFMC->mode & FLAG_BIT(append))
        pFMC->next_ix = pFMC->eof;

    /*
     *  Only add a NUL character if:
     *
     *  * we are not in binary mode
     *  * there are data to write
     *  * the last character to write is not already NUL
     */
    add_nul_char =
           ((pFMC->mode & FLAG_BIT(binary)) != 0)
        && (sz > 0)
        && (((char*)pBuf)[sz - 1] != NUL);

    {
        size_t next_pos = pFMC->next_ix + sz + add_nul_char;
        if (next_pos > pFMC->buf_size) {
            if (fmem_extend( pFMC, next_pos ) != 0) {
                /*
                 *  We could not extend the memory.  Try to write some data.
                 *  Fail if we are either at the end or not writing data.
                 */
                if ((pFMC->next_ix >= pFMC->buf_size) || (sz == 0))
                    return -1; /* no space at all.  errno is set. */

                /*
                 *  Never add the NUL for a truncated write.  "sz" may be
                 *  unchanged or limited here.
                 */
                add_nul_char = 0;
                sz = pFMC->buf_size - pFMC->next_ix;
            }
        }
    }

    memcpy( pFMC->buffer + pFMC->next_ix, pBuf, sz);

    pFMC->next_ix += sz;

    /*
     *  Check for new high water mark and remember it.  Add a NUL if
     *  we do that and if we have a new high water mark.
     */
    if (pFMC->next_ix > pFMC->eof) {
        pFMC->eof = pFMC->next_ix;
        if (add_nul_char)
            /*
             *  There is space for this NUL.  The "add_nul_char" is not part of
             *  the "sz" that was added to "next_ix".
             */
            pFMC->buffer[ pFMC->eof ] = NUL;
    }

    return sz;
}


static seek_pos_t
fmem_seek (void *cookie, fmem_off_t *p_offset, int dir)
{
    fmem_off_t new_pos;
    fmem_cookie_t *pFMC = cookie;

    switch (dir) {
    case SEEK_SET: new_pos = *p_offset;  break;
    case SEEK_CUR: new_pos = pFMC->next_ix  + *p_offset;  break;
    case SEEK_END: new_pos = pFMC->eof - *p_offset;  break;

    default:
        goto seek_oops;
    }

    if ((signed)new_pos < 0)
        goto seek_oops;

    if (new_pos > pFMC->buf_size) {
        if (fmem_extend( pFMC, new_pos ))
            return -1; /* errno is set */
    }

    pFMC->next_ix = new_pos;
    return new_pos;

 seek_oops:
    errno = EINVAL;
    return -1;
}


static int
fmem_close( void *cookie )
{
    fmem_cookie_t *pFMC = cookie;

    if (pFMC->mode & FLAG_BIT(allocated))
        free( pFMC->buffer );
    free( pFMC );

    return 0;
}

/*=export_func pcomn_fmemopen
 *
 *  what:  Open a stream to a string
 *
 *  arg: + void*   + buf  + buffer to use for i/o +
 *  arg: + ssize_t + len  + size of the buffer +
 *  arg: + char*   + mode + mode string, a la fopen(3C) +
 *
 *  ret-type:  FILE*
 *  ret-desc:  a stdio FILE* pointer
 *
 *  err:  NULL is returned and errno is set to @code{EINVAL} or @code{ENOSPC}.
 *
 *  doc:
 *
 *  This function requires underlying @var{libc} functionality:
 *  either @code{fopencookie(3GNU)} or @code{funopen(3BSD)}.
 *
 *  If @var{buf} is @code{NULL}, then a buffer is allocated.
 *  The initial allocation is @var{len} bytes.  If @var{len} is
 *  less than zero, then the buffer will not be reallocated if more
 *  space is needed.  Any allocated memory is @code{free()}-ed
 *  when @code{fclose(3C)} is called.
 *
 *  If @code{buf} is not @code{NULL}, then @code{len} must not be zero.
 *  It may still be less than zero to indicate that the buffer is not to
 *  be reallocated.
 *
 *  The mode string is interpreted as follows.  If the first character of
 *  the mode is:
 *
 *  @table @code
 *  @item a
 *  Then the string is opened in "append" mode.  In binary mode, "appending"
 *  will begin from the end of the initial buffer.  Otherwise, appending will
 *  start at the first NUL character in the initial buffer (or the end of the
 *  buffer if there is no NUL character).  Do not use fixed size buffers
 *  (negative @var{len} lengths) in append mode.
 *
 *  @item w
 *  Then the string is opened in "write" mode.  Any initial buffer is presumed
 *  to be empty.
 *
 *  @item r
 *  Then the string is opened in "read" mode.
 *  @end table
 *
 *  @noindent
 *  If it is not one of these three, the open fails and @code{errno} is
 *  set to @code{EINVAL}.  These initial characters may be followed by:
 *
 *  @table @code
 *  @item +
 *  The buffer is marked as updatable and both reading and writing is enabled.
 *
 *  @item b
 *  The I/O is marked as "binary" and a trailing NUL will not be inserted into
 *  the buffer.  Without this mode flag, one will be inserted after the
 *  @code{EOF}, if it fits.  It will fit if the buffer is extensible (the
 *  provided @var{len} was not negative).  This mode flag has no effect if
 *  the buffer is opened in read-only mode.
 *
 *  @item x
 *  This is ignored.
 *  @end table
 *
 *  @noindent
 *  Any other letters following the inital 'a', 'w' or 'r' will cause an error.
=*/
FILE *
pcomn_fmemopen(void *buf, ssize_t len, const char *pMode)
{
    fmem_cookie_t *pFMC;

    {
        mode_bits_t mode;

        if (fmem_getmode(pMode, &mode) != 0) {
            return NULL;
        }

        pFMC = malloc(sizeof(fmem_cookie_t));
        if (pFMC == NULL) {
            errno = ENOMEM;
            return NULL;
        }

        pFMC->mode = mode;
    }

    /*
     *  Two more mode bits that do not come from the mode string:
     *  a negative size implies fixed size buffer and a NULL
     *  buffer pointer means we must allocate (and free) it.
     */
    if (len < 0) {
        len = -len;
        pFMC->mode |= FLAG_BIT(fixed_size);
    }

    else
        /*
         *  We only need page size if we might extend an allocation.
         */
        pFMC->pg_size = getpagesize();

    if (buf != NULL) {
        /*
         *  User allocated buffer.  User responsible for disposal.
         */
        if (len == 0) {
            free( pFMC );
            errno = EINVAL;
            return NULL;
        }

        /*  Figure out where our "next byte" and EOF are.
         *  Truncated files start at the beginning.
         */
        if (pFMC->mode & FLAG_BIT(truncate)) {
            /*
             *  "write" mode
             */
            pFMC->eof = \
            pFMC->next_ix = 0;
        }

        else if (pFMC->mode & FLAG_BIT(binary)) {
            pFMC->eof = len;
            pFMC->next_ix    = (pFMC->mode & FLAG_BIT(append)) ? len : 0;

        } else {
            /*
             * append or read text mode -- find the end of the buffer
             * (the first NUL character)
             */
            buf_bytes_t *p = pFMC->buffer = (buf_bytes_t*)buf;
            pFMC->eof = 0;
            while ((*p != NUL) && (++(pFMC->eof) < len))  p++;
            pFMC->next_ix =
                (pFMC->mode & FLAG_BIT(append)) ? pFMC->eof : 0;
        }

        /*
         *  text mode - NUL terminate buffer, if it fits.
         */
        if (  ((pFMC->mode & FLAG_BIT(binary)) == 0)
           && (pFMC->next_ix < len)) {
            pFMC->buffer[pFMC->next_ix] = NUL;
        }
    }

    else if ((pFMC->mode & (FLAG_BIT(append) | FLAG_BIT(truncate))) == 0) {
        /*
         *  Not appending and not truncating.  We must be reading.
         *  We also have no user supplied buffer.  Nonsense.
         */
        errno = EINVAL;
        free( pFMC );
        return NULL;
    }

    else {
        /*
         *  We must allocate the buffer.  If "len" is zero, set it to page size.
         */
        pFMC->mode |= FLAG_BIT(allocated);
        if (len == 0)
            len = pFMC->pg_size;

        /*
         *  Unallocated file space is set to NULs.  Emulate that.
         */
        pFMC->buffer = calloc(1, len);
        if (pFMC->buffer == NULL) {
            errno = ENOMEM;
            free( pFMC );
            return NULL;
        }

        /*
         *  We've allocated the buffer.  The end of file and next entry
         *  are both zero.
         */
        pFMC->next_ix = 0;
        pFMC->eof = 0;
    }

    pFMC->buf_size   = len;

    cookie_read_function_t* pRd = (pFMC->mode & FLAG_BIT(read))
        ?  (cookie_read_function_t*)fmem_read  : NULL;
    cookie_write_function_t* pWr = (pFMC->mode & FLAG_BIT(write))
        ? (cookie_write_function_t*)fmem_write : NULL;
    return funopen( pFMC, pRd, pWr,
                    (cookie_seek_function_t* )fmem_seek,
                    (cookie_close_function_t*)fmem_close );
}
