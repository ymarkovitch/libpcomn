/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
/*******************************************************************************
 FILE         :   netaddr.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Internet address class(es)/functions.

 CREATION DATE:   27 Jan 2008
*******************************************************************************/
#include <pcomn_net/netaddr.h>
#include <pcomn_utils.h>
#include <pcomn_meta.h>
#include <pcomn_atomic.h>

#include <netinet/in.h>
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <unistd.h>

namespace pcomn {
namespace net {

/*******************************************************************************
 inet_address
*******************************************************************************/
uint32_t inet_address::from_string(const strslice &addrstr, unsigned flags)
{
    const unsigned maxdot = 16 ;
    const unsigned maxsz =
        ct_max<unsigned, ct_max<unsigned, NI_MAXHOST, maxdot>::value, IFNAMSIZ>::value ;

    if (addrstr.empty())
    {
        PCOMN_THROW_MSG_IF(!(flags & ALLOW_EMPTY), invalid_str_repr, "Empty network address string") ;
        return 0 ;
    }

    struct in_addr addr ;
    char strbuf[maxsz + 16] ;

    const unsigned len = std::min<size_t>(addrstr.size(), sizeof strbuf - 1) ;
    memcpy(strbuf, addrstr.begin(), len) ;
    strbuf[len] = 0 ;

    PCOMN_THROW_MSG_IF(addrstr.size() >= maxsz, invalid_str_repr,
                       "The address string '%s' is too long.", strbuf) ;

    PCOMN_THROW_MSG_IF(
        (flags & (IGNORE_DOTDEC|IGNORE_HOSTNAME|USE_IFACE)) == (IGNORE_DOTDEC|IGNORE_HOSTNAME),
        std::invalid_argument, "Invalid flags: flags combination completely disables address construction") ;

    const bool usexc = !(flags & NO_EXCEPTION) ;

    // First try to interpret the address as dot-decimal.
    if (!(flags & IGNORE_DOTDEC))
    {
        if (addrstr.size() < maxdot
            && std::count(addrstr.begin(), addrstr.end(), '.') == 3
            && inet_aton(strbuf, &addr))

            return ntohl(addr.s_addr) ;

        if ((flags & (IGNORE_HOSTNAME|USE_IFACE)) == IGNORE_HOSTNAME)
        {
            PCOMN_THROW_MSG_IF(usexc, invalid_str_repr, "Invalid dot decimal IP address '%s'.", strbuf) ;
            return 0 ;
        }
    }

    if (flags & USE_IFACE)
    {
        if (addrstr.size() < IFNAMSIZ)
        {
            static int sockd = -1 ;
            if (sockd == -1)
            {
                int s = ::socket(PF_INET, SOCK_STREAM, 0) ;
                PCOMN_THROW_MSG_IF(s == -1, network_error, "Cannot create a socket.") ;
                if (!atomic_op::cas(&sockd, -1, s))
                    ::close(s) ;
            }

            ifreq request ;
            strcpy(request.ifr_name, strbuf) ;

            if (ioctl(sockd, SIOCGIFADDR, &request) != -1)
                return ntohl(reinterpret_cast<sockaddr_in *>(&request.ifr_addr)->sin_addr.s_addr) ;
        }
        if (flags & IGNORE_HOSTNAME)
        {
            PCOMN_THROW_MSG_IF(usexc, inaddr_error,
                               "Cannot retrieve address for network interface '%s'.", strbuf) ;
            return 0 ;
        }
    }

    // OK, maybe it's a host name?
    // gethostbyname is thread-safe at least in glibc (when libpthreads is used)
    if (const struct hostent * const host = gethostbyname(strbuf))
        return ntohl(*(const uint32_t *)host->h_addr_list[0]) ;

    if (usexc)
    {
        const int err = h_errno ;
        errno = EINVAL ;
        PCOMN_THROWF(inaddr_error, "Cannot resolve hostname '%s'. %s", strbuf, hstrerror(err)) ;
    }
    return 0 ;
}

std::string inet_address::dotted_decimal() const
{
    char buf[64] ;
    return std::string(to_strbuf(buf)) ;
}

std::string inet_address::hostname() const
{
    char name[NI_MAXHOST] ;
    sock_address sa ;
    sa.as_sockaddr_in()->sin_addr = *this ;
    PCOMN_THROW_MSG_IF(getnameinfo(sa.as_sockaddr(), sa.addrsize(), name, sizeof(name), 0, 0, 0),
                       inaddr_error, "Failed to resolve domain name for %s.", str().c_str()) ;
    return name ;
}

/*******************************************************************************
 sock_address
*******************************************************************************/
std::string sock_address::str() const
{
    char buf[64] ;
    snprintf(buf, sizeof buf, "%d.%d.%d.%d:%d",
             (int)addr().octet(0), (int)addr().octet(1), (int)addr().octet(2), (int)addr().octet(3), port()) ;
    return buf ;
}

/*******************************************************************************
 subnet_address
*******************************************************************************/
subnet_address::subnet_address(const strslice &subnet_string, RaiseError raise_error) :
    _pfxlen(0)
{
    const auto &s = strsplit(subnet_string, '/') ;

    if (s.first && s.second)
        try {
            _pfxlen = ensure_pfxlen<invalid_str_repr>(strtonum<uint8_t>(make_strslice_range(s.second))) ;
            _addr = inet_address
                (inet_address(s.first, inet_address::ONLY_DOTDEC | (-(int)!raise_error & inet_address::NO_EXCEPTION)).ipaddr() & netmask()) ;
            return ;
        }
        catch (const std::exception &)
        { _pfxlen = 0 ; }

    PCOMN_THROW_MSG_IF(raise_error, invalid_str_repr,
                       "Invalid subnet specification: " P_STRSLICEQF, P_STRSLICEV(subnet_string)) ;
}

} // end of namespace pcomn::net
} // end of namespace pcomn
