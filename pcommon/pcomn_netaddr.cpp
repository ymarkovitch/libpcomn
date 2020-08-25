/*-*- tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
/*******************************************************************************
 FILE         :   pcomn_netaddr.cpp
 COPYRIGHT    :   Yakov Markovitch, 2008-2020. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Internet address class(es)/functions.

 CREATION DATE:   27 Jan 2008
*******************************************************************************/
#include "pcomn_netaddr.h"
#include "pcomn_unistd.h"
#include "pcomn_utils.h"
#include "pcomn_meta.h"
#include "pcomn_atomic.h"
#include "pcomn_string.h"
#include "pcomn_strslice.h"
#include "pcomn_range.h"

#ifdef PCOMN_PL_POSIX
/*******************************************************************************
 UNIX
*******************************************************************************/

#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netdb.h>

#else
/*******************************************************************************
 Winsocks
*******************************************************************************/

#include <win32/pcomn_w32util.h>

#include <winsock2.h>
#include <ws2tcpip.h>

static const unsigned IFNAMSIZ = 256 ;

inline std::string hstrerror(long sockerr) { return pcomn::sys_error_text(sockerr) ; }

MS_IGNORE_WARNING(4267)
#endif

namespace pcomn {

static inline std::pair<ipv4_addr, bool> ipv4_from_dotdec(const strslice &addrstr) noexcept
{
    enum State { Dot, Digit } ;

    size_t dotcount = 0 ;
    State state = Dot ;
    unsigned last_octet ;
    uint8_t octets[3] ;

    for (const char c: addrstr)
    {
        switch (state)
        {
            case Dot:
                if (!std::isdigit(c)) return {} ;
                last_octet = c - '0' ;
                state = Digit ;
                break ;

            case Digit:
                if (std::isdigit(c))
                {
                    last_octet = last_octet * 10 + (c - '0') ;
                    if (last_octet > 255) return {} ;
                }
                else if (c == '.')
                {
                    if (dotcount > 2) return {} ;
                    octets[dotcount] = last_octet ;
                    last_octet = 0 ;
                    ++dotcount ;
                    state = Dot ;
                }
                else
                    return {} ;
        }
    }
    if (state != Digit || dotcount != 3)
        return {} ;

    return {ipv4_addr(octets[0], octets[1], octets[2], last_octet), true} ;
}

#ifndef PCOMN_PL_POSIX
static inline std::pair<uint32_t, bool> ipv4_from_ifaddr(const char *, bool) { return {} ; }
#else
static __noinline std::pair<uint32_t, bool> ipv4_from_ifaddr(const char *addr, bool raise)
{
    try {
        static int sockd = PCOMN_ENSURE_POSIX(::socket(PF_INET, SOCK_STREAM, 0), "socket") ;
        ifreq request ;
        strcpy(request.ifr_name, addr) ;

        if (ioctl(sockd, SIOCGIFADDR, &request) != -1)
            return {value_from_big_endian(reinterpret_cast<sockaddr_in *>(&request.ifr_addr)->sin_addr.s_addr), true} ;
    }
    catch (const std::exception &x)
    {
        if (raise)
            throw ;
    }
    return {} ;
}
#endif /* PCOMN_PL_POSIX */

/*******************************************************************************
 ipv4_addr
*******************************************************************************/
uint32_t ipv4_addr::from_string(const strslice &addrstr, std::errc *ec, CFlags flags)
{
    const unsigned maxdot = 16 ;
    const unsigned maxsz =
        ct_max<unsigned, ct_max<unsigned, NI_MAXHOST, maxdot>::value, IFNAMSIZ>::value ;

    PCOMN_THROW_MSG_IF((flags & (IGNORE_DOTDEC|USE_HOSTNAME|USE_IFACE)) == IGNORE_DOTDEC,
                       std::invalid_argument, "Invalid flags: flags combination completely disables address construction.") ;

    const bool raise_error = !ec && !(flags & NO_EXCEPTION) ;
    if (ec)
        *ec = {} ;

    #define IPV4_ENSURE(cond, exception, ...) do if (!(cond)) {         \
            if (ec) *ec = (std::is_base_of<std::invalid_argument, exception>()) \
                        ? std::errc::invalid_argument : (std::errc)errno ; \
            PCOMN_THROW_MSG_IF(raise_error, exception, __VA_ARGS__) ;   \
            return 0 ;                                                  \
        } while(false)


    if (addrstr.empty())
    {
        IPV4_ENSURE(flags & ALLOW_EMPTY, invalid_str_repr, "Empty IPv4 address string.") ;
        return 0 ;
    }

    char strbuf[maxsz + 16] ;

    const unsigned len = std::min<size_t>(addrstr.size(), sizeof strbuf - 1) ;
    memcpy(strbuf, addrstr.begin(), len) ;
    strbuf[len] = 0 ;

    IPV4_ENSURE(addrstr.size() < maxsz, invalid_str_repr, "IPv4 address string '%s' is too long.", strbuf) ;

    // First try to interpret the address as dot-decimal.
    if (!(flags & IGNORE_DOTDEC))
    {
        const auto from_dotdec = ipv4_from_dotdec(addrstr) ;
        if (from_dotdec.second)
            return from_dotdec.first.ipaddr() ;

        IPV4_ENSURE(flags & (USE_HOSTNAME|USE_IFACE), invalid_str_repr, "Invalid dot decimal IP address '%s'.", strbuf) ;
    }

    if (flags & USE_IFACE)
    {
        const auto &addr = ipv4_from_ifaddr(strbuf, raise_error) ;
        if (addr.second)
            return addr.first ;

        IPV4_ENSURE(flags & USE_HOSTNAME, system_error,
                    "Cannot retrieve address for network interface '%s'", strbuf) ;
    }

    // OK, maybe it's a host name?
    // gethostbyname is thread-safe at least in glibc (when libpthreads is used)
    if (const struct hostent * const host = gethostbyname(strbuf))
        return value_from_big_endian(*(const uint32_t *)host->h_addr_list[0]) ;

    const long err = h_errno ;
    errno = EINVAL ;

    IPV4_ENSURE(false, system_error,
                "Cannot resolve hostname '%s'. %s", strbuf, str::cstr(hstrerror(err))) ;
}

std::string ipv4_addr::dotted_decimal() const
{
    addr_strbuf buf ;
    return std::string(to_strbuf(buf)) ;
}

std::string ipv4_addr::hostname() const
{
    char name[NI_MAXHOST] ;
    sock_address sa ;
    sa.as_sockaddr_in()->sin_addr = *this ;
    PCOMN_THROW_MSG_IF(getnameinfo(sa.as_sockaddr(), sa.addrsize(), name, sizeof(name), 0, 0, 0),
                       system_error, "Failed to resolve domain name for %s", str().c_str()) ;
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
 ipv4_subnet
*******************************************************************************/
ipv4_subnet::ipv4_subnet(const strslice &subnet_string, RaiseError raise_error)
{
    const auto &s = strsplit(subnet_string, '/') ;

    if (s.first && s.second)
        try {
            _pfxlen = ensure_pfxlen<invalid_str_repr>(strtonum<uint8_t>(make_strslice_range(s.second))) ;
            _addr = ipv4_addr(s.first, ipv4_addr::ONLY_DOTDEC|flags_if(ipv4_addr::NO_EXCEPTION, !raise_error)) ;
            return ;
        }
        catch (const std::exception &)
        { _pfxlen = 0 ; }

    PCOMN_THROW_MSG_IF(raise_error, invalid_str_repr,
                       "Invalid IPv4 network prefix specification: " P_STRSLICEQF, P_STRSLICEV(subnet_string)) ;
}

/*******************************************************************************
 ipv6_addr
*******************************************************************************/
binary128_t ipv6_addr::from_string(const strslice &address_string, CFlags flags)
{
    #define IPV6_STRING_ERROR() do { if (raise_error) invalid_address_string(address_string) ; return {} ; } while(false)
    #define IPV6_STRING_ENSURE(cond) while (unlikely(!(cond))) { IPV6_STRING_ERROR() ; }

    enum State {
        Begin,
        HeadColon,
        DelimColon,
        Hextet
    } ;

    union result_ptr { uint16_t *phextet ; uint32_t *pv4 ; } ;

    const bool raise_error = !(flags & NO_EXCEPTION) ;
    const bool allow_dotdec = !(flags & IGNORE_DOTDEC) ;

    ipv6_addr result ;
    result_ptr dest = {result._hdata} ;

    // Start of the run of 0x00s
    result_ptr zrun_begin = {nullptr} ;
    // The buffer to use after the run of 0x00s
    uint16_t after_zrun[8] ;

    size_t hextet_count = 0 ;
    uint32_t current_hextet = 0 ;

    State state = Begin ;

    size_t string_pos = 0 ;
    size_t begin_hextet_pos ;

    const auto start_hextet = [&](char c)
    {
        const int digit = hexchartoi(c) ;
        if (digit < 0)
            return false ;
        current_hextet = digit ;
        begin_hextet_pos = string_pos ;

        state = Hextet ;
        return true ;
    } ;

    for (const char c: address_string)
    {
        switch (state)
        {
            case Begin:
                if (c == ':')
                {
                    // Shortcut for "unspecified address" (AKA "::", all-zeros)
                    if (std::size(address_string) == 2)
                    {
                        IPV6_STRING_ENSURE(address_string.back() == ':') ;
                        return {} ;
                    }
                    state = HeadColon ;
                }
                else
                    IPV6_STRING_ENSURE(start_hextet(c)) ;
                break ;

            case DelimColon:
                // There can be only a digit or ':' after the ':'
                if (start_hextet(c))
                {
                    state = Hextet ;
                    break ;
                }

                // '::' inside the address may occur at most once
                IPV6_STRING_ENSURE(!zrun_begin.phextet & ++hextet_count < 8) ;

                // Fall through
            case HeadColon:
                IPV6_STRING_ENSURE(c == ':') ;
                ++hextet_count ;
                zrun_begin = dest ;
                // Write hextets into the temporary buffer from now
                dest.phextet = after_zrun ;

                state = DelimColon ;
                break ;

            case Hextet:
            {
                const int digit = hexchartoi(c) ;
                if (digit >= 0)
                {
                    current_hextet = (current_hextet << 4) | digit ;
                    IPV6_STRING_ENSURE(current_hextet <= 0xffffu) ;
                    // Keep the Hextet state.
                    break ;
                }

                IPV6_STRING_ENSURE(((c == ':') | (c == '.')) & ++hextet_count < 8) ;

                if (c == ':')
                {
                    *dest.phextet++ = value_to_big_endian((uint16_t)current_hextet) ;
                    current_hextet = 0 ;
                    state = DelimColon ;
                    break ;
                }
                // '.', must be dot-decimal IPv4 at the end of the string; check if
                // allowed at all.
                IPV6_STRING_ENSURE(allow_dotdec) ;

                const auto ipv4_opt = ipv4_from_dotdec(address_string(begin_hextet_pos)) ;

                IPV6_STRING_ENSURE(ipv4_opt.second) ;

                if (!begin_hextet_pos)
                    return ipv4_opt.first ? ipv6_addr(ipv4_opt.first) : ipv6_addr() ;

                *dest.pv4++ = value_to_big_endian(ipv4_opt.first.ipaddr()) ;

                goto end ;
            }

        } /* End of switch(c) */

        ++string_pos ;

    } /* End of for(c) */

    switch (state)
    {
        case Begin:      IPV6_STRING_ENSURE(flags & ALLOW_EMPTY) ; return {} ;

        case DelimColon: IPV6_STRING_ENSURE(*(address_string.end() - 2) == ':') ; break ;
        case Hextet:     *dest.phextet++ = value_to_big_endian((uint16_t)current_hextet) ; break ;
        default: break ;
    }

end:
    if (zrun_begin.phextet)
    {
        // We've encountered '::', fixup sequence of zeros
        NOXCHECK(pcomn::xinrange(zrun_begin.phextet, std::begin(result._hdata), std::end(result._hdata))) ;
        NOXCHECK(pcomn::xinrange(dest.phextet, std::begin(after_zrun), std::end(after_zrun))) ;

        const size_t after_zrun_hextets_count = dest.phextet - after_zrun ;
        NOXCHECK((zrun_begin.phextet - result._hdata) + after_zrun_hextets_count < 8) ;

        std::copy(std::begin(after_zrun), dest.phextet,
                  std::end(result._hdata) - after_zrun_hextets_count) ;
    }

    return result ;
}

inline
ipv6_addr::zero_run ipv6_addr::find_longest_zero_run() const
{
    zero_run current ;
    zero_run longest ;

    for (short i = 0 ; i < 8 ; ++i)
    {
        if (!hextet(i))
        {

            if (current.start == -1)
                current = {i, 1} ;
            else
                ++current.len ;
            continue ;
        }

        if (current.len > longest.len)
            longest = current ;
        current.start = -1 ;
    }

    if (current.len > longest.len)
        longest = current ;

    return longest.len >= 2 ? longest : zero_run() ;
}

// Heavily modified code from inet_ntop6 from Apache ASF library
const char *ipv6_addr::to_strbuf(addr_strbuf output) const
{
    // Is this an encapsulated IPv4 address?
    if (is_ipv4_mapped())
    {
        // Make a distinction between
        //  - "universal unspecified address", AKA DENIL, which is equal by its binary
        //    representation for _both_ the ipv4 and v6 and is all-zeros 128-bit binary,
        //  - and ipv4 unspecified address, which is ::ffff:0.0.0.0
        //
        _wdata[3]
            ? ipv4_addr(value_from_big_endian(_wdata[3])).to_strbuf(output)
            : strcpy(output, "::ffff:0.0.0.0") ;
        return output ;
    }

    const zero_run longest_zero_run = find_longest_zero_run() ;
    const int longest_end = longest_zero_run.start + longest_zero_run.len ;

    char *dest = output ;
    for (int i = 0 ; i < 8 ; ++i)
    {
        // Are we inside the best run of 0x00's?
        if (i < longest_end && i >= longest_zero_run.start)
        {
            *dest++ = ':' ;
            // Will be incremented at the loop header
            i = longest_end - 1 ;
            continue ;
        }

        // Are we following the initial run of 0x00s or any real hextet?
        if (i)
            *dest++ = ':' ;

        const uint16_t hx = hextet(i) ;
        // Suppress hextet's leading zeros
        unsigned digicount = 3 - ((hx <= 0x0fffu) + (hx <= 0x00ffu) + (hx <= 0x000fu)) ;
        do { *dest++ = itohexchar((hx >> 4*digicount) & 0xf) ; }
        while (digicount--) ;
    }

    // Was it a trailing run of 0x00's?
    if (longest_end == 8)
        *dest++ = ':' ;

    *dest++ = '\0' ;

    return output ;
}

__noreturn __cold void ipv6_addr::invalid_address_string(const strslice &address_string)
{
    if (!address_string)
        throw_exception<invalid_str_repr>("Empty IPv6 address string.") ;
    else
        throwf<invalid_str_repr>("Invalid IPv6 address string " P_STRSLICEQF ".", P_STRSLICEV(address_string)) ;
}

/*******************************************************************************
 ipv6_subnet
*******************************************************************************/
ipv6_subnet::ipv6_subnet(const strslice &subnet_string, RaiseError raise_error)
{
    const auto &s = strsplit(subnet_string, '/') ;

    if (s.first && s.second)
        try {
            _pfxlen = ensure_pfxlen<invalid_str_repr>(strtonum<uint8_t>(make_strslice_range(s.second))) ;
            *static_cast<ancestor*>(this) = ipv6_addr(s.first, flags_if(ipv6_addr::NO_EXCEPTION, !raise_error)|ipv6_addr::IGNORE_DOTDEC) ;
            return ;
        }
        catch (const std::exception &)
        { _pfxlen = 0 ; }

    PCOMN_THROW_MSG_IF(raise_error, invalid_str_repr,
                       "Invalid IPv6 network prefix specification: " P_STRSLICEQF, P_STRSLICEV(subnet_string)) ;
}

} // end of namespace pcomn
