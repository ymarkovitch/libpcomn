/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"ellemtel";c-basic-offset:4;c-file-offsets:((innamespace . 0)(inlambda . 0)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_ssafe.cpp
 COPYRIGHT    :   Yakov Markovitch, 2018

 DESCRIPTION  :

 PROGRAMMED BY:   Yakov Markovitch
 CREATION DATE:   18 Sep 2017
*******************************************************************************/
#include <pcomn_ssafe.h>
#include <pcomn_unistd.h>
#include <pcomn_utils.h>

#include <errno.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

namespace pcomn {

extern "C" ssize_t ssafe_read(const char *name, char *buf, size_t bufsize) noexcept
{
    if (!name || !buf && bufsize)
    {
        errno = EINVAL ;
        return -1 ;
    }
    const int fd = open(name, O_RDONLY) ;
    if (fd < 0)
        return -1 ;

    const ssize_t result = bufsize ? read(fd, buf, bufsize) : 0 ;
    if (bufsize)
        buf[std::min<ssize_t>(std::max<ssize_t>(result, 1), bufsize) - 1] = 0 ;

    close(fd) ;
    return result ;
}

extern "C" const char *ssafe_progname(char *buf, size_t bufsize) noexcept
{
    static const char empty[] = {0} ;
    if (!buf || !bufsize)
    {
        if (!buf)
            errno = EINVAL ;
        return empty ;
    }
    readlink("/proc/self/exe", (char *)memset(buf, 0, bufsize), bufsize - 1) ;
    return buf ;
}

extern "C" char *ssafe_rfc3339_time(struct tm t, int is_utc, char *buf, size_t bufsize) noexcept
{
    if (!buf || bufsize < 2)
    {
        if (buf && bufsize)
            *buf = 0 ;
        return buf ;
    }

    const char offsign = timezone < 0 ? '+' : '-' ;
    const long offsec = is_utc ? 0 : labs(timezone) ;

    char s[] =
    {
        char('0' + (t.tm_year + 1900) / 1000),
        char('0' + (t.tm_year / 100 + 19) % 10),
        char('0' + (t.tm_year / 10) % 10),
        char('0' + t.tm_year % 10),
        '-',
        char('0' + (t.tm_mon + 1) / 10),
        char('0' + (t.tm_mon + 1) % 10),
        '-',
        char('0' + t.tm_mday / 10),
        char('0' + t.tm_mday % 10),

        ' ',
        char('0' + t.tm_hour / 10),
        char('0' + t.tm_hour % 10),
        ':',
        char('0' + t.tm_min / 10),
        char('0' + t.tm_min % 10),
        ':',
        char('0' + t.tm_sec / 10),
        char('0' + t.tm_sec % 10),

        offsign,
        char('0' + offsec / 36000),
        char('0' + (offsec / 3600) % 10),
        ':',
        char('0' + ((offsec / 60) % 60) / 10),
        char('0' + ((offsec / 60) % 60) % 10),
    } ;

    const size_t sz = std::min(bufsize - 1, sizeof s) ;
    memcpy(buf, s, sz) ;
    buf[sz] = 0 ;
    return buf ;
}

} // end of namespace pcomn
