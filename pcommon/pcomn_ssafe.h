/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
#ifndef __PCOMN_SSAFE_H
#define __PCOMN_SSAFE_H
/*******************************************************************************
 FILE         :   pcomn_ssafe.h
 COPYRIGHT    :   Yakov Markovitch, 2017

 DESCRIPTION  :   Simple, safe, and signal-safe functions.

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   18 Sep 2017
*******************************************************************************/
#include <pcomn_platform.h>
#include <pcommon.h>

#include <ostream>

#include <time.h>
#include <stdlib.h>
#include <stddef.h>

namespace pcomn {

enum Rfc3339Bufsize : size_t {
    RFC3339_DATE        = 11,
    RFC3339_DATETIME    = 20,
    RFC3339_FULL        = 26
} ;

/// Convert the passed time to RFC 3339 format (e.g. 2006-08-14 02:34:56+03:00)
/// Most usable bufsizes are 11, 20, 26
extern "C" char *ssafe_rfc3339_time(struct tm t, int is_utc, char *buf, size_t bufsize) noexcept ;
extern "C" ssize_t ssafe_read(const char *name, char *buf, size_t bufsize) noexcept ;
extern "C" const char *ssafe_progname(char *buf, size_t bufsize) noexcept ;

inline char *ssafe_rfc3339_time(time_t t, bool is_utc, char *buf, size_t bufsize) noexcept
{
    typedef struct tm tmstruct ;
    return ssafe_rfc3339_time
        (*localtime_r(&t, &as_mutable(tmstruct())), is_utc, buf, bufsize) ;
}

inline char *ssafe_rfc3339_localtime(time_t t, char *buf, size_t bufsize) noexcept
{
    return ssafe_rfc3339_time(t, false, buf, bufsize) ;
}

inline char *ssafe_rfc3339_gmtime(time_t t, char *buf, size_t bufsize) noexcept
{
    return ssafe_rfc3339_time(t, true, buf, bufsize) ;
}

template<typename T, size_t n>
inline char *ssafe_rfc3339_time(T t, bool is_utc, char (&buf)[n]) noexcept
{
    return ssafe_rfc3339_time(t, is_utc, buf, n) ;
}

template<size_t n>
inline char *ssafe_rfc3339_localtime(time_t t, char (&buf)[n]) noexcept
{
    return ssafe_rfc3339_localtime(t, buf, n) ;
}

template<size_t n>
inline char *ssafe_rfc3339_gmtime(time_t t, char (&buf)[n]) noexcept
{
    return ssafe_rfc3339_gmtime(t, buf, n) ;
}

inline const char *ssafe_reads(const char *name, char *buf, size_t bufsize) noexcept
{
    ssafe_read(name, buf, bufsize) ;
    return buf ? buf : "" ;
}

template<size_t n>
inline const char *ssafe_reads(const char *name, char (&buf)[n]) noexcept
{
    return ssafe_reads(name, buf, n) ;
}

template<size_t n>
inline const char *ssafe_progname(char (&buf)[n]) noexcept
{
    return ssafe_progname(buf, n) ;
}

/***************************************************************************//**
 Output stream with "embedded" stream buffer, the memory buffer is a C array
 the member of of the class.

 The template argument defines (fixed) size of the output buffer.
 This class never makes dynamic allocations.
*******************************************************************************/
template<size_t sz = 0>
class bufstr_ostream : private std::basic_streambuf<char>, public std::ostream {
public:
    /// Create the stream with embedded buffer of @a sz bytes.
    bufstr_ostream() noexcept ;

    /// Get the pouinter to internal buffer memory
    const char *str() const noexcept { return _buffer ; }
    char *str() noexcept { return _buffer ; }

    const char *begin() const noexcept { return str() ; }
    const char *end() const noexcept { return this->pptr() ; }
    size_t size() const noexcept { return end() - begin() ; }
    constexpr size_t max_size() const noexcept { return sz ; }

    bufstr_ostream &reset() noexcept
    {
        clear() ;
        setp(_buffer + 0, _buffer + sizeof _buffer - 1) ;
        return *this ;
    }

private:
    char _buffer[sz] ;
} ;

/***************************************************************************//**
 Output stream with external fixed-size stream buffer .
*******************************************************************************/
template<>
class bufstr_ostream<0> : private std::basic_streambuf<char>, public std::ostream {
    PCOMN_NONCOPYABLE(bufstr_ostream) ;
    PCOMN_NONASSIGNABLE(bufstr_ostream) ;
public:
    bufstr_ostream(char *buf, size_t bufsize) noexcept :
        std::ostream(static_cast<std::basic_streambuf<char> *>(this)),
        _buffer(buf), _bufsz(bufsize)
    {
        setp(_buffer, _buffer + _bufsz) ;
    }

    /// Get the pouinter to internal buffer memory
    const char *str() const noexcept { return _buffer ; }
    char *str() noexcept { return _buffer ; }

    const char *begin() const noexcept { return str() ; }
    const char *end() const noexcept { return this->pptr() ; }
    size_t size() const noexcept { return end() - begin() ; }
    size_t max_size() const noexcept { return _bufsz ; }

    bufstr_ostream &reset()
    {
        clear() ;
        setp(_buffer, _buffer + _bufsz) ;
        return *this ;
    }

private:
    char *   _buffer = nullptr ;
    size_t   _bufsz  = 0 ;
} ;

/*******************************************************************************
 bufstr_ostream<size_t>
*******************************************************************************/
template<size_t sz>
bufstr_ostream<sz>::bufstr_ostream() noexcept :
std::ostream(static_cast<std::basic_streambuf<char> *>(this))
{
    *_buffer = _buffer[sizeof _buffer - 1] = 0 ;
    reset() ;
}

} // end of namespace pcomn

#endif /* __PCOMN_SSAFE_H */
