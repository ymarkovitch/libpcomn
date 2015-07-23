/*-*- mode: c; tab-width:3; indent-tabs-mode:nil; c-file-style:"ellemtel"; c-file-offsets:((inclass . ++)) -*-*/
#ifndef __PCOMN_ZIOWRAP_H
#define __PCOMN_ZIOWRAP_H
/*******************************************************************************
 FILE         :   pcomn_ziowrap.h
 COPYRIGHT    :   Yakov Markovitch. 2004-2015. All rights reserved.
                  See LICENSE for information on usage/redistribution.
                  The original software is copyright (C) 1995-2003 Jean-Loup Gailly and Mark Adler.

 DESCRIPTION  :   The abstract input/output layer for the 'zlib' general purpose
                  compression library.
                  Heavily based on gzio.c from the original zlib source distribution

 CREATION DATE:   15 Aug 2007
*******************************************************************************/
#include "zlib.h"
#include <pcomn_platform.h>
#include <pcomn_def.h>

#ifdef __cplusplus
extern "C" {
#endif

struct zstream_api_t ;
typedef struct zstream_api_t zstream_api_t ;
struct zstream_t ;
typedef struct zstream_t zstream_t ;

typedef struct zstreambuf_t {
    const zstream_api_t *api ;
} zstreambuf_t ;

typedef zstreambuf_t *GZSTREAMBUF ;
typedef zstream_t    *GZSTREAM ;

typedef int (*zstreambuf_init)  (GZSTREAMBUF) ;
typedef ssize_t   (*zstreambuf_read)  (GZSTREAMBUF, void *, size_t) ;
typedef ssize_t   (*zstreambuf_write) (GZSTREAMBUF, const void *, size_t) ;
typedef fileoff_t (*zstreambuf_seek)  (GZSTREAMBUF, fileoff_t offset, int origin) ;
typedef int (*zstreambuf_error) (GZSTREAMBUF, int clear) ;

struct zstream_api_t {
    zstreambuf_init  stream_open ;
    zstreambuf_init  stream_close ;
    zstreambuf_read  stream_read ;
    zstreambuf_write stream_write ;
    zstreambuf_seek  stream_seek ;
    zstreambuf_error stream_errno ;
} ;

/** Open a compressed stream for reading or writing.
    The mode parameter is as in
    fopen ("r" or "w", "b" and "t" ignored) but can also includes a compression
    level ("w9") or a strategy: 'f' for filtered data as in "w6f", 'h' for
    Huffman only compression as in "w1h", or 'R' for run-length encoding as in
    "w1R". (See the description of deflateInit2 for more information about the
    strategy parameter.)

    zopen can be used to read a stream which is not in gzip format; in this
    case zread will directly read from the file without decompression.

    zopen returns NULL if the file could not be opened or if there was
    insufficient memory to allocate the (de)compression state; errno
    can be checked to distinguish the two cases (if errno is zero, the
    zlib error is Z_MEM_ERROR).
*/
_PCOMNEXP GZSTREAM zopen(GZSTREAMBUF streambuf, const char *mode) ;

/** Dynamically update the compression level or strategy.
    See the description of deflateInit2 for the meaning of these parameters.
    zsetparams returns Z_OK if success, or Z_STREAM_ERROR if the file was not
    opened for writing.
*/
_PCOMNEXP int zsetparams(GZSTREAM stream, int level, int strategy) ;

/** Read the given number of uncompressed bytes from the compressed stream.
    If the input stream was not in gzip format, zread copies the given number
    of bytes into the buffer.
    zread returns the number of uncompressed bytes actually read (0 for
    end of file, -1 for error).
*/
_PCOMNEXP ssize_t zread(GZSTREAM stream, void *buf, size_t len) ;

/** Write the given number of uncompressed bytes into the compressed stream.
    zwrite returns the number of uncompressed bytes actually written
    (0 in case of error).
*/
_PCOMNEXP ssize_t zwrite(GZSTREAM stream, const void *buf, size_t len) ;

/** Write the given null-terminated string to the compressed stream, excluding
    the terminating null character.
    zputs returns the number of characters written, or -1 in case of error.
*/
_PCOMNEXP ssize_t zputs(GZSTREAM stream, const char *s) ;

/** Read bytes from the compressed stream until len-1 characters are read, or
    a newline character is read and transferred to buf, or an end-of-file
    condition is encountered.
    The string is then terminated with a null character.
    zgets returns buf, or Z_NULL in case of error.
*/
_PCOMNEXP char *zgets(GZSTREAM stream, char *buf, int len) ;

/** Write a character into the compressed stream.
    @param stream Stream to write a character to.
    @param c      A character to write; converted to unsigned char.
    @return the value that was written, or -1 in case of error.
*/
_PCOMNEXP int zputc(GZSTREAM stream, int c) ;

/** Read one byte from the compressed stream.
    zgetc returns this byte or -1 in case of end of file or error.
*/
_PCOMNEXP int zgetc(GZSTREAM stream) ;

/** Push one character back onto the stream to be read again later.
    Only one character of push-back is allowed.  zungetc() returns the
    character pushed, or -1 on failure.  zungetc() will fail if a character has
    been pushed but not read yet, or if c is -1. The pushed character will be
    discarded if the stream is repositioned with zseek() or zrewind().
*/
_PCOMNEXP int zungetc(GZSTREAM stream, int c) ;

/** Flush all pending output into the compressed stream.
    @param stream Stream to flash.
    @param flush  As in the deflate() function.
    @return the zlib error number.
    zflush returns Z_OK if the flush parameter is Z_FINISH and all output
    could be flushed.
    zflush should be called only when strictly necessary because it can
    degrade compression.
    @see zerror
*/
_PCOMNEXP int zflush(GZSTREAM stream, int flush) ;

/** Set the starting position for the next zread or zwrite on the
    given compressed stream.
    The offset represents a number of bytes in the
    uncompressed data stream. The whence parameter is defined as in lseek(2);
    the value SEEK_END is not supported.
    If the stream is opened for reading, this function is emulated but can be
    extremely slow. If the file is opened for writing, only forward seeks are
    supported; zseek then compresses a sequence of zeroes up to the new
    starting position.

    zseek returns the resulting offset location as measured in bytes from
    the beginning of the uncompressed stream, or -1 in case of error, in
    particular if the file is opened for writing and the new starting position
    would be before the current position.
*/
_PCOMNEXP z_off_t zseek(GZSTREAM stream, z_off_t offset, int whence) ;

/** Rewind the given file.
    This function is supported only for reading.
    zrewind(file) is equivalent to (int)zseek(file, 0L, SEEK_SET)
*/
_PCOMNEXP int zrewind(GZSTREAM stream) ;

/** Get the starting position for the next zread or zwrite on the
    given compressed stream.
    This position represents a number of bytes in the uncompressed data stream.

    ztell(file) is equivalent to zseek(file, 0L, SEEK_CUR)
*/
_PCOMNEXP z_off_t ztell(GZSTREAM stream) ;

/** Indicate whether EOF condition has previously been detected.
    @return 1 when EOF has previously been detected reading the given
    input stream, otherwise zero.
*/
_PCOMNEXP int zeof(GZSTREAM stream) ;

/** Flush all pending output if necessary, close the compressed stream
    and deallocate all the (de)compression state.
    The return value is the zlib error number.
    @see zerror.
*/
_PCOMNEXP int zclose(GZSTREAM stream) ;

_PCOMNEXP int zerror_(GZSTREAM stream) ;

#define zerror(stream) zerror_((stream))

/** Clear the error state for both the zstream and underlying stream.
    This is useful for continuing to read a compressed file that is being
    written concurrently or to seek back after the end-of-file occured.
*/
_PCOMNEXP void zclearerr(GZSTREAM stream) ;

#ifdef __cplusplus
}
#endif

#endif /* __PCOMN_ZIOWRAP_H */
