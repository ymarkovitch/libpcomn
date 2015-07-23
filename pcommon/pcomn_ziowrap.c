/*-*- mode: c; tab-width: 3; indent-tabs-mode: nil; c-file-style: "ellemtel" -*-*/
/*******************************************************************************
 FILE         :   pcomn_ziowrap.c
 COPYRIGHT    :   Yakov Markovitch, 2004-2015. All rights reserved.
                  See LICENCE.pcommon.txt for information on usage/redistribution.
                  The original software is copyright (C) 1995-2003 Jean-Loup Gailly.

 DESCRIPTION  :   The abstract input/output layer for the 'zlib' general purpose
                  compression library.
                  Heavily based on gzio.c from the original zlib source distribution

 CREATION DATE:   15 Aug 2007
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcomn_ziowrap.h>

#include <stdio.h>
#include <malloc.h>
#include <string.h>

#if MAX_MEM_LEVEL >= 8
#  define DEF_MEM_LEVEL 8
#else
#  define DEF_MEM_LEVEL  MAX_MEM_LEVEL
#endif
/* default memLevel */

#ifdef PCOMN_PL_AMIGA
#  define OS_CODE  0x01
#elif defined(PCOMN_PL_VAXC) || defined(PCOMN_PL_VMS)
#  define OS_CODE  0x02
#elif defined(PCOMN_PL_ATARI)
#  define OS_CODE  0x05
#elif defined(PCOMN_PL_OS2)
#  define OS_CODE  0x06
#elif defined(PCOMN_PL_MACOS)
#  define OS_CODE  0x07
#elif defined(PCOMN_PL_TOPS20)
#  define OS_CODE  0x0a
#elif defined(PCOMN_PL_WINDOWS)
#  define OS_CODE  0x0b
#elif defined(PCOMN_PL_PRIMOS)
#  define OS_CODE  0x0f
#else
#  define OS_CODE  0x03  /* assume Unix */
#endif

#define Z_BUFSIZE 16384

#define ALLOC(size) malloc(size)
#define TRYFREE(p) {if (p) free(p);}
#define RETURN_APICALL(stream, call, params)            \
{                                                       \
    const zstream_api_t *api = (stream)->api ;          \
    return api && api->call ? (long)(api->call params) : -1 ;   \
}

static int const gz_magic[2] = {0x1f, 0x8b}; /* gzip magic header */

/* gzip flag byte */
#define ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define COMMENT      0x10 /* bit 4 set: file comment present */
#define RESERVED     0xE0 /* bits 5..7: reserved */

struct zstream_t {
    z_stream        stream;
    int             z_err;      /* error code for last stream operation */
    int             z_eof;      /* set if end of input file */
    zstreambuf_t   *streambuf;  /* underlying abstract input- or output-stream */
    Byte           *inbuf;      /* input buffer */
    Byte           *outbuf;     /* output buffer */
    uint32_t        crc;        /* crc32 of uncompressed data */
    char           *msg;        /* error message */
    int             transparent; /* 1 if the input stream is not a .gz stream */
    char            mode;       /* 'w' or 'r' */
    z_off_t         start;      /* start of compressed data in file (header skipped) */
    z_off_t         in;         /* bytes into deflate or inflate */
    z_off_t         out;        /* bytes out of deflate or inflate */
    int             back;       /* one character push-back */
    int             last;       /* true if push-back is last character */
} ;

static int do_flush       (GZSTREAM file, int flush);
static int    get_byte    (zstream_t *s);
static void   check_header(zstream_t *s);
static int    destroy     (zstream_t *s);
static void   putLong     (zstreambuf_t *buf, uint32_t x);
static uint32_t  getLong     (zstream_t *s);

static int bufread(zstreambuf_t *stream, void *buf, size_t size)
{
    RETURN_APICALL(stream, stream_read, (stream, buf, size)) ;
}

static int bufwrite(zstreambuf_t *stream, const void *buf, size_t size)
{
    RETURN_APICALL(stream, stream_write, (stream, buf, size)) ;
}

static long bufseek(zstreambuf_t *stream, fileoff_t offset, int origin)
{
    RETURN_APICALL(stream, stream_seek, (stream, offset, origin)) ;
}

static long buftell(zstreambuf_t *stream)
{
    return bufseek(stream, 0, SEEK_CUR) ;
}

static int buferror(zstreambuf_t *stream)
{
    RETURN_APICALL(stream, stream_errno, (stream, 0)) ;
}

/* ===========================================================================
     Opens a gzip (.gz) file for reading or writing. The mode parameter is as
   in fopen ("r" or "w", both "b" and "t" ignored). The file is given either
   by file descriptor or path name (if fd == -1).
     zopen returns NULL if the file could not be opened or if there was
   insufficient memory to allocate the (de)compression state; streambuf error
   state can be checked to distinguish the two cases (if the streambuf is OK, the
   zlib error is Z_MEM_ERROR).
*/

#define Z_CANNOT_OPEN(s) (destroy(s), (GZSTREAM)Z_NULL)

GZSTREAM zopen(zstreambuf_t *streambuf, const char *mode)
{
    int err;
    int level = Z_DEFAULT_COMPRESSION; /* compression level */
    int strategy = Z_DEFAULT_STRATEGY; /* compression strategy */
    char *p = (char*)mode;
    zstream_t *s;
    char fmode[80]; /* copy of mode, without the compression level */
    char *m = fmode;

    if (!streambuf || !streambuf->api) return Z_NULL;

    s = (zstream_t *)ALLOC(sizeof(zstream_t));
    if (!s) return Z_NULL;

    s->stream.zalloc = (alloc_func)0;
    s->stream.zfree = (free_func)0;
    s->stream.opaque = (voidpf)0;
    s->stream.next_in = s->inbuf = Z_NULL;
    s->stream.next_out = s->outbuf = Z_NULL;
    s->stream.avail_in = s->stream.avail_out = 0;
    s->streambuf = NULL;
    s->z_err = Z_OK;
    s->z_eof = 0;
    s->in = 0;
    s->out = 0;
    s->back = EOF;
    s->crc = crc32(0L, Z_NULL, 0);
    s->msg = NULL;
    s->transparent = 0;

    s->mode = '\0';
    do {
        if (*p == 'r') s->mode = 'r';
        if (*p == 'w' || *p == 'a') s->mode = 'w';
        if (*p >= '0' && *p <= '9') {
            level = *p - '0';
        } else if (*p == 'f') {
          strategy = Z_FILTERED;
        } else if (*p == 'h') {
          strategy = Z_HUFFMAN_ONLY;
        } else if (*p == 'R') {
          strategy = Z_RLE;
        } else {
            *m++ = *p; /* copy the mode */
        }
    } while (*p++ && m != fmode + sizeof(fmode));
    if (s->mode == '\0') return Z_CANNOT_OPEN(s);


    if (s->mode == 'w') {
        if (!streambuf->api->stream_write) return Z_CANNOT_OPEN(s);
        err = deflateInit2(&(s->stream), level,
                           Z_DEFLATED, -MAX_WBITS, DEF_MEM_LEVEL, strategy);
        /* windowBits is passed < 0 to suppress zlib header */

        s->stream.next_out = s->outbuf = (Byte*)ALLOC(Z_BUFSIZE);
        if (err != Z_OK || s->outbuf == Z_NULL) {
            return Z_CANNOT_OPEN(s);
        }
    } else {
        if (!streambuf->api->stream_read) return Z_CANNOT_OPEN(s);

        s->stream.next_in  = s->inbuf = (Byte*)ALLOC(Z_BUFSIZE);

        err = inflateInit2(&(s->stream), -MAX_WBITS);
        /* windowBits is passed < 0 to tell that there is no zlib header.
         * Note that in this case inflate *requires* an extra "dummy" byte
         * after the compressed stream in order to complete decompression and
         * return Z_STREAM_END. Here the gzip CRC32 ensures that 4 bytes are
         * present after the compressed stream.
         */
        if (err != Z_OK || s->inbuf == Z_NULL) {
            return Z_CANNOT_OPEN(s);
        }
    }
    s->stream.avail_out = Z_BUFSIZE;

    if (streambuf->api->stream_open && streambuf->api->stream_open(streambuf))
        return Z_CANNOT_OPEN(s);

    s->streambuf = streambuf ;

    if (s->mode == 'w') {
        /* Write a very simple .gz header:
         */
        char buf[11] ;
        sprintf(buf, "%c%c%c%c%c%c%c%c%c%c", gz_magic[0], gz_magic[1],
                Z_DEFLATED, 0 /*flags*/, 0,0,0,0 /*time*/, 0 /*xflags*/, OS_CODE);
        bufwrite(s->streambuf, buf, 10) ;
        s->start = 10L;
    } else {
        check_header(s); /* skip the .gz header */
        s->start = buftell(s->streambuf) - s->stream.avail_in;
    }

    return s;
}

/* ===========================================================================
 * Update the compression level and strategy
 */
int zsetparams(GZSTREAM s, int level, int strategy)
{
    if (s == NULL || s->mode != 'w') return Z_STREAM_ERROR;

    /* Make room to allow flushing */
    if (s->stream.avail_out == 0) {

        s->stream.next_out = s->outbuf;
        if (bufwrite(s->streambuf, s->outbuf, Z_BUFSIZE) != Z_BUFSIZE) {
            s->z_err = Z_ERRNO;
        }
        s->stream.avail_out = Z_BUFSIZE;
    }

    return deflateParams (&(s->stream), level, strategy);
}

/* ===========================================================================
     Read a byte from a zstream_t; update next_in and avail_in. Return EOF
   for end of file.
   IN assertion: the stream s has been sucessfully opened for reading.
*/
static int get_byte(zstream_t *s)
{
    if (s->z_eof) return EOF;
    if (s->stream.avail_in == 0) {
        s->stream.avail_in = bufread(s->streambuf, s->inbuf, Z_BUFSIZE);
        if (s->stream.avail_in == 0) {
            s->z_eof = 1;
            if (buferror(s->streambuf)) s->z_err = Z_ERRNO;
            return EOF;
        }
        s->stream.next_in = s->inbuf;
    }
    s->stream.avail_in--;
    return *(s->stream.next_in)++;
}

/* ===========================================================================
      Check the gzip header of a zstream_t opened for reading. Set the stream
    mode to transparent if the gzip magic header is not present; set s->err
    to Z_DATA_ERROR if the magic header is present but the rest of the header
    is incorrect.
    IN assertion: the stream s has already been created sucessfully;
       s->stream.avail_in is zero for the first time, but may be non-zero
       for concatenated .gz files.
*/
static void check_header(zstream_t *s)
{
    int method; /* method byte */
    int flags;  /* flags byte */
    uInt len;
    int c;

    /* Assure two bytes in the buffer so we can peek ahead -- handle case
       where first byte of header is at the end of the buffer after the last
       gzip segment */
    len = s->stream.avail_in;
    if (len < 2) {
        if (len) s->inbuf[0] = s->stream.next_in[0];
        len = bufread(s->streambuf, s->inbuf + len, Z_BUFSIZE >> len);
        if (len == 0 && buferror(s->streambuf)) s->z_err = Z_ERRNO;
        s->stream.avail_in += len;
        s->stream.next_in = s->inbuf;
        if (s->stream.avail_in < 2) {
            s->transparent = s->stream.avail_in;
            return;
        }
    }

    /* Peek ahead to check the gzip magic header */
    if (s->stream.next_in[0] != gz_magic[0] ||
        s->stream.next_in[1] != gz_magic[1]) {
        s->transparent = 1;
        return;
    }
    s->stream.avail_in -= 2;
    s->stream.next_in += 2;

    /* Check the rest of the gzip header */
    method = get_byte(s);
    flags = get_byte(s);
    if (method != Z_DEFLATED || (flags & RESERVED) != 0) {
        s->z_err = Z_DATA_ERROR;
        return;
    }

    /* Discard time, xflags and OS code: */
    for (len = 0; len < 6; len++) (void)get_byte(s);

    if ((flags & EXTRA_FIELD) != 0) { /* skip the extra field */
        len  =  (uInt)get_byte(s);
        len += ((uInt)get_byte(s))<<8;
        /* len is garbage if EOF but the loop below will quit anyway */
        while (len-- != 0 && get_byte(s) != EOF) ;
    }
    if ((flags & ORIG_NAME) != 0) { /* skip the original file name */
        while ((c = get_byte(s)) != 0 && c != EOF) ;
    }
    if ((flags & COMMENT) != 0) {   /* skip the .gz file comment */
        while ((c = get_byte(s)) != 0 && c != EOF) ;
    }
    if ((flags & HEAD_CRC) != 0) {  /* skip the header crc */
        for (len = 0; len < 2; len++) (void)get_byte(s);
    }
    s->z_err = s->z_eof ? Z_DATA_ERROR : Z_OK;
}

 /* ===========================================================================
 * Cleanup then free the given zstream_t. Return a zlib error code.
   Try freeing in the reverse order of allocations.
 */
static int destroy(zstream_t *s)
{
    int err = Z_OK;
    int zerr = Z_OK;
    zstreambuf_t *streambuf;

    if (!s) return Z_STREAM_ERROR;

    streambuf = s->streambuf ;

    TRYFREE(s->msg);

    if (streambuf && s->stream.state != NULL) {
        if (s->mode == 'w') {
            err = deflateEnd(&(s->stream));
        } else if (s->mode == 'r') {
            err = inflateEnd(&(s->stream));
        }
        zerr = s->z_err ;
    }
    /* Free all the memory _before_ attempting to close the underlying
     * streambuf to protect us from memory leaks in case of stream_close
     * calls into C++ code which can throw exception */
    TRYFREE(s->inbuf);
    TRYFREE(s->outbuf);
    TRYFREE(s);
    if (streambuf->api &&
        streambuf->api->stream_close &&
        streambuf->api->stream_close(streambuf))
        err = Z_ERRNO ;

    return zerr < 0 ? zerr : err ;
}

/* ===========================================================================
     Reads the given number of uncompressed bytes from the compressed file.
   gzread returns the number of bytes actually read (0 for end of file).
*/
ssize_t zread(GZSTREAM s, voidp buf, size_t buflen)
{
    Bytef *start = (Bytef*)buf; /* starting point for crc computation */
    Byte  *next_out; /* == stream.next_out but not forced far (for MSDOS) */
    unsigned len = (unsigned)buflen;

    if (s == NULL || s->mode != 'r') return Z_STREAM_ERROR;

    if (s->z_err == Z_DATA_ERROR || s->z_err == Z_ERRNO) return -1;
    if (s->z_err == Z_STREAM_END) return 0;  /* EOF */

    if (len != buflen) return Z_MEM_ERROR;

    next_out = (Byte*)buf;
    s->stream.next_out = (Bytef*)buf;
    s->stream.avail_out = len;

    if (s->stream.avail_out && s->back != EOF) {
        *next_out++ = s->back;
        s->stream.next_out++;
        s->stream.avail_out--;
        s->back = EOF;
        s->out++;
        if (s->last) {
            s->z_err = Z_STREAM_END;
            return 1;
        }
    }

    while (s->stream.avail_out != 0) {

        if (s->transparent) {
            /* Copy first the lookahead bytes: */
            uInt n = s->stream.avail_in;
            if (n > s->stream.avail_out) n = s->stream.avail_out;
            if (n > 0) {
                memcpy(s->stream.next_out, s->stream.next_in, n);
                next_out += n;
                s->stream.next_out = next_out;
                s->stream.next_in   += n;
                s->stream.avail_out -= n;
                s->stream.avail_in  -= n;
            }
            if (s->stream.avail_out > 0) {
                s->stream.avail_out -= bufread(s->streambuf,
                                               next_out,
                                               s->stream.avail_out);
            }
            len -= s->stream.avail_out;
            s->in  += len;
            s->out += len;
            if (len == 0) s->z_eof = 1;
            return (ssize_t)len;
        }
        if (s->stream.avail_in == 0 && !s->z_eof) {
            s->stream.avail_in = bufread(s->streambuf, s->inbuf, Z_BUFSIZE);
            if (s->stream.avail_in == 0) {
                s->z_eof = 1;
                if (buferror(s->streambuf)) {
                    s->z_err = Z_ERRNO;
                    break;
                }
            }
            s->stream.next_in = s->inbuf;
        }
        s->in += s->stream.avail_in;
        s->out += s->stream.avail_out;
        s->z_err = inflate(&(s->stream), Z_NO_FLUSH);
        s->in -= s->stream.avail_in;
        s->out -= s->stream.avail_out;

        if (s->z_err == Z_STREAM_END) {
            /* Check CRC and original size */
            s->crc = crc32(s->crc, start, (uInt)(s->stream.next_out - start));
            start = s->stream.next_out;

            if (getLong(s) != s->crc) {
                s->z_err = Z_DATA_ERROR;
            } else {
                (void)getLong(s);
                /* The uncompressed length returned by above getlong() may be
                 * different from s->out in case of concatenated .gz files.
                 * Check for such files:
                 */
                check_header(s);
                if (s->z_err == Z_OK) {
                    inflateReset(&(s->stream));
                    s->crc = crc32(0L, Z_NULL, 0);
                }
                else if (!s->stream.avail_in) {
                   s->z_err = Z_STREAM_END;
                }
            }
        }
        if (s->z_err != Z_OK || s->z_eof) break;
    }
    s->crc = crc32(s->crc, start, (uInt)(s->stream.next_out - start));

    return (ssize_t)(len - s->stream.avail_out);
}


/* ===========================================================================
      Reads one byte from the compressed file. zgetc returns that byte
      or -1 in case of end of file or error.
*/
int zgetc(GZSTREAM stream)
{
    unsigned char c;
    return zread(stream, &c, 1) == 1 ? c : -1;
}

/* ===========================================================================
      Push one byte back onto the stream.
*/
int zungetc(GZSTREAM s, int c)
{
    if (s == NULL || s->mode != 'r' || c == EOF || s->back != EOF) return EOF;
    s->back = c;
    s->out--;
    s->last = (s->z_err == Z_STREAM_END);
    if (s->last) s->z_err = Z_OK;
    s->z_eof = 0;
    return c;
}


/* ===========================================================================
      Reads bytes from the compressed file until len-1 characters are
   read, or a newline character is read and transferred to buf, or an
   end-of-file condition is encountered.  The string is then terminated
   with a null character.
      zgets returns buf, or Z_NULL in case of error.
*/
char * zgets(GZSTREAM stream, char *buf, int len)
{
    char *b = buf;
    if (buf == Z_NULL || len <= 0) return Z_NULL;

    while (--len > 0 && zread(stream, buf, 1) == 1 && *buf++ != '\n') ;
    *buf = '\0';
    return b == buf && len > 0 ? Z_NULL : b;
}

/* ===========================================================================
     Writes the given number of uncompressed bytes into the compressed file.
   gzwrite returns the number of bytes actually written (0 in case of error).
*/
ssize_t zwrite(GZSTREAM s, voidpc buf, size_t buflen)
{
    unsigned len = (unsigned)buflen;

    if (s == NULL || s->mode != 'w') return Z_STREAM_ERROR;

    if (len != buflen) return Z_MEM_ERROR;

    s->stream.next_in = (Bytef*)buf;
    s->stream.avail_in = len;

    while (s->stream.avail_in != 0) {

        if (s->stream.avail_out == 0) {

            s->stream.next_out = s->outbuf;
            if (bufwrite(s->streambuf, s->outbuf, Z_BUFSIZE) != Z_BUFSIZE) {
                s->z_err = Z_ERRNO;
                break;
            }
            s->stream.avail_out = Z_BUFSIZE;
        }
        s->in += s->stream.avail_in;
        s->out += s->stream.avail_out;
        s->z_err = deflate(&(s->stream), Z_NO_FLUSH);
        s->in -= s->stream.avail_in;
        s->out -= s->stream.avail_out;
        if (s->z_err != Z_OK) break;
    }
    s->crc = crc32(s->crc, (const Bytef *)buf, len);

    return (int)(len - s->stream.avail_in);
}

/* ===========================================================================
      Writes c, converted to an unsigned char, into the compressed file.
   zputc returns the value that was written, or -1 in case of error.
*/
int zputc(GZSTREAM s, int c)
{
    unsigned char cc = (unsigned char) c; /* required for big endian systems */
    return zwrite(s, &cc, 1) == 1 ? (int)cc : -1;
}


/* ===========================================================================
      Writes the given null-terminated string to the compressed file, excluding
   the terminating null character.
      zputs returns the number of characters written, or -1 in case of error.
*/
ssize_t zputs(GZSTREAM stream, const char *str)
{
    return zwrite(stream, (char*)str, (unsigned)strlen(str));
}

/* ===========================================================================
     Flushes all pending output into the compressed file. The parameter
   flush is as in the deflate() function.
*/
static int do_flush(GZSTREAM s, int flush)
{
    uInt len;
    int done = 0;

    if (s == NULL || s->mode != 'w') return Z_STREAM_ERROR;

    s->stream.avail_in = 0; /* should be zero already anyway */

    for (;;) {
        len = Z_BUFSIZE - s->stream.avail_out;

        if (len != 0) {
            if ((uInt)bufwrite(s->streambuf, s->outbuf, len) != len) {
                s->z_err = Z_ERRNO;
                return Z_ERRNO;
            }
            s->stream.next_out = s->outbuf;
            s->stream.avail_out = Z_BUFSIZE;
        }
        if (done) break;
        s->out += s->stream.avail_out;
        s->z_err = deflate(&(s->stream), flush);
        s->out -= s->stream.avail_out;

        /* Ignore the second of two consecutive flushes: */
        if (len == 0 && s->z_err == Z_BUF_ERROR) s->z_err = Z_OK;

        /* deflate has finished flushing only when it hasn't used up
         * all the available space in the output buffer:
         */
        done = (s->stream.avail_out != 0 || s->z_err == Z_STREAM_END);

        if (s->z_err != Z_OK && s->z_err != Z_STREAM_END) break;
    }
    return  s->z_err == Z_STREAM_END ? Z_OK : s->z_err;
}

int zflush(GZSTREAM s, int flush)
{
    int err = do_flush(s, flush);
    if (err) return err;
    return  s->z_err == Z_STREAM_END ? Z_OK : s->z_err;
}

/* ===========================================================================
      Sets the starting position for the next zread or zwrite on the given
      compressed file. The offset represents a number of bytes in the zseek
      returns the resulting offset location as measured in bytes from the
      beginning of the uncompressed stream, or -1 in case of error.

      SEEK_END is not implemented, returns error.

      In this version of the library, gzseek can be extremely slow.
*/
z_off_t zseek(GZSTREAM s, z_off_t offset, int whence)
{
    if (s == NULL || whence == SEEK_END ||
        s->z_err == Z_ERRNO || s->z_err == Z_DATA_ERROR) {
        return -1L;
    }

    if (s->mode == 'w') {
        if (whence == SEEK_SET) {
            offset -= s->in;
        }
        if (offset < 0) return -1L;

        /* At this point, offset is the number of zero bytes to write. */
        if (s->inbuf == Z_NULL) {
            s->inbuf = (Byte*)ALLOC(Z_BUFSIZE); /* for seeking */
            if (s->inbuf == Z_NULL) return -1L;
            memset(s->inbuf, 0, Z_BUFSIZE) ;
        }
        while (offset > 0)  {
            uInt size = Z_BUFSIZE;
            if (offset < Z_BUFSIZE) size = (uInt)offset;

            size = (uInt)zwrite(s, s->inbuf, size);
            if (size == 0) return -1L;

            offset -= size;
        }
        return s->in;
    }
    /* Rest of function is for reading only */

    /* compute absolute position */
    if (whence == SEEK_CUR) {
        offset += s->out;
    }
    if (offset < 0) return -1L;

    if (s->transparent) {
        /* map to fseek */
        s->back = EOF;
        s->stream.avail_in = 0;
        s->stream.next_in = s->inbuf;
        if (bufseek(s->streambuf, offset, SEEK_SET) < 0) return -1L;

        s->in = s->out = offset;
        return offset;
    }

    /* For a negative seek, rewind and use positive seek */
    if (offset >= s->out) {
        offset -= s->out;
    } else if (zrewind(s) < 0) {
        return -1L;
    }
    /* offset is now the number of bytes to skip. */

    if (offset != 0 && s->outbuf == Z_NULL) {
        s->outbuf = (Byte*)ALLOC(Z_BUFSIZE);
        if (s->outbuf == Z_NULL) return -1L;
    }
    if (offset && s->back != EOF) {
        s->back = EOF;
        s->out++;
        offset--;
        if (s->last) s->z_err = Z_STREAM_END;
    }
    while (offset > 0)  {
        ssize_t size = Z_BUFSIZE;
        if (offset < Z_BUFSIZE) size = offset;

        size = zread(s, s->outbuf, size);
        if (size <= 0) return -1L;
        offset -= (z_off_t)size;
    }
    return s->out;
}

/* ===========================================================================
     Rewinds input file.
*/
int zrewind(GZSTREAM s)
{
    if (s == NULL || s->mode != 'r') return -1;


    s->z_err = Z_OK;
    zclearerr(s) ;
    s->back = EOF;
    s->stream.avail_in = 0;
    s->stream.next_in = s->inbuf;
    s->crc = crc32(0L, Z_NULL, 0);
    if (!s->transparent) (void)inflateReset(&s->stream);
    s->in = 0;
    s->out = 0;
    return bufseek(s->streambuf, s->start, SEEK_SET);
}

/* ===========================================================================
     Returns the starting position for the next gzread or gzwrite on the
   given compressed file. This position represents a number of bytes in the
   uncompressed data stream.
*/
z_off_t ztell(GZSTREAM stream)
{
    return zseek(stream, 0L, SEEK_CUR);
}

/* ===========================================================================
     Returns 1 when EOF has previously been detected reading the given
   input stream, otherwise zero.
*/
int zeof(GZSTREAM stream)
{
    /* With concatenated compressed files that can have embedded
     * crc trailers, z_eof is no longer the only/best indicator of EOF
     * on a zstream_t. Handle end-of-stream error explicitly here.
     */
    return
        stream && stream->mode == 'r' &&
        (stream->z_eof || stream->z_err == Z_STREAM_END) ;
}

int zerror_(GZSTREAM stream)
{
    if (stream)
        return !stream->z_err && stream->z_eof && stream->mode == 'r'
            ? Z_STREAM_END
            : stream->z_err ;
    return Z_OK ;
}

/* ===========================================================================
     Clear the error and end-of-file flags, and do the same for the real stream.
*/
void zclearerr(GZSTREAM s)
{
    zstreambuf_t *streambuf ;
    if (s == NULL) return;
    streambuf = s->streambuf ;
    /* Attempt to clear the real stream's error first, since it can both throw
     * exception (e.g. ios::clear() when there are exception flags set) and
     * refuse the attempt to clear */
    if (streambuf && streambuf->api) {
        zstreambuf_error err = streambuf->api->stream_errno ;
        if (err && err(streambuf, 1) && err(streambuf, 0))
            return ;
    }
    if (s->z_err != Z_STREAM_END) s->z_err = Z_OK ;
    s->z_eof = 0 ;
}

/* ===========================================================================
   Outputs a long in LSB order to the given file
*/
static void putLong(zstreambuf_t *streambuf, uint32_t x)
{
    unsigned char buf[4] ;
    int pos ;
    for (pos = 0 ; pos < 4 ; ++pos)
        buf[pos] = (x >> 8 * pos) & 0xff ;
    bufwrite(streambuf, buf, 4) ;
}

/* ===========================================================================
   Reads a long in LSB order from the given zstream_t. Sets z_err in case
   of error.
*/
static uint32_t getLong(zstream_t *s)
{
    uint32_t x = (uint32_t)get_byte(s);
    int c;

    x += ((uint32_t)get_byte(s))<<8;
    x += ((uint32_t)get_byte(s))<<16;
    c = get_byte(s);
    if (c == EOF) s->z_err = Z_DATA_ERROR;
    x += ((uint32_t)c)<<24;
    return x;
}

/* ===========================================================================
     Flushes all pending output if necessary, closes the compressed file
   and deallocates all the (de)compression state.
*/
int zclose(GZSTREAM stream)
{
    if (stream == NULL) return Z_STREAM_ERROR;

    if (stream->streambuf && stream->mode == 'w') {
        if (do_flush(stream, Z_FINISH) != Z_OK) return destroy(stream);

        putLong(stream->streambuf, stream->crc);
        putLong(stream->streambuf, (uint32_t)(stream->in & 0xffffffff));
    }
    return destroy(stream);
}
