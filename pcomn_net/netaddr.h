/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
#ifndef __NET_NETADDR_H
#define __NET_NETADDR_H
/*******************************************************************************
 FILE         :   netaddr.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2014. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Internet address class(es)/functions.

 CREATION DATE:   27 Jan 2008
*******************************************************************************/
/** @file
  Classes and functions for working with network addresses.
*******************************************************************************/
#include <pcomn_net/netexcept.h>
#include <pcomn_utils.h>
#include <pcomn_hash.h>
#include <pcomn_string.h>
#include <pcomn_strslice.h>

#include <string>

#include <netinet/in.h>
#include <sys/types.h>

#include <unordered_map>
#include <array>

namespace pcomn {
namespace net {

/******************************************************************************/
/** IP address.

    @note All comparison/relational operators are defined as free function, providing for
    symmetrical compare with all data types to/from which inet_address defines implicit
    conversions.
*******************************************************************************/
class inet_address {
public:
    /// inet_address construction mode flags
    enum ConstructFlags {
        IGNORE_HOSTNAME = 0x0001, /**< Don't attempt to interpret address string as
                                     a hostname */
        IGNORE_DOTDEC   = 0x0002, /**< Don't attempt to interpret address string as
                                     a dot-delimited IP address */
        USE_IFACE       = 0x0004, /**< Attempt to interpret address string as a network
                                     interace name (e.g. "lo" or "eth0") */
        ALLOW_EMPTY     = 0x0008, /**< Allow to pass an empty string (the resulting
                                     address will be 0.0.0.0) */
        NO_EXCEPTION    = 0x1000, /**< Don't throw exception if construction failed,
                                     intialize inet_address to 0 */

        ONLY_HOSTNAME   = IGNORE_DOTDEC,
        ONLY_DOTDEC     = IGNORE_HOSTNAME,
        ONLY_IFACE      = IGNORE_DOTDEC | IGNORE_HOSTNAME | USE_IFACE,
        FROM_IFACE      = IGNORE_HOSTNAME | USE_IFACE
    } ;

    /// Create default address (0.0.0.0).
    constexpr inet_address() : _addr(0) {}

    /// Create an IP address from its human-readable text representation.
    /// @param address_string Dot-delimited IP address or interface name, like "eth0", or
    /// host name (tried in that order).
    /// @param flags        ORed ConstructFlags flags.
    /// @return Resolved IP address. If cannot resolve, throws inaddr_error or constructs
    /// an empty inet_address (i.e. ipaddr()==0), depending on NO_EXCEPTION flag.
    inet_address(const strslice &address_string, unsigned flags = 0) :
        _addr(from_string(address_string, flags))
    {}

    explicit constexpr inet_address(uint32_t host_order_inetaddr) :
        _addr(host_order_inetaddr)
    {}

    inet_address(const in_addr &addr) :
        _addr(ntohl(addr.s_addr))
    {}

    inet_address(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
    {
        union {
            in_addr_t   addr ;
            uint8_t     octets[4] ;
        } netaddr ;
        netaddr.octets[0] = a ;
        netaddr.octets[1] = b ;
        netaddr.octets[2] = c ;
        netaddr.octets[3] = d ;
        _addr = ntohl(netaddr.addr) ;
    }

    explicit constexpr operator bool() { return !!_addr ; }

    /// Get one octet of an IP address by octet index (0-3).
    uint8_t octet(unsigned ndx) const
    {
        NOXCHECK(ndx < 4) ;
        union {
            in_addr_t   addr ;
            uint8_t     octets[4] ;
        } netaddr = { htonl(_addr) } ;
        return netaddr.octets[ndx] ;
    }

    /// Get all four octets of an IP address.
    std::array<uint8_t, 4> octets() const
    {
        union {
            in_addr_t addr ;
            std::array<uint8_t, 4> result ;
        } netaddr = { htonl(_addr) } ;
        return netaddr.result ;
    }

    /// Get an IP address as a 32-bit unsigned number in the host byte order.
    constexpr uint32_t ipaddr() { return _addr ; }

    in_addr inaddr() const
    {
        const in_addr result = { htonl(_addr) } ;
        return result ;
    }

    operator struct in_addr() const { return inaddr() ; }

    /// Get a hostname for the address.
    /// @throw nothing
    std::string hostname() const;

    /// Get dotted-decimal representation of IP address.
    std::string dotted_decimal() const ;

    std::string str() const { return dotted_decimal() ; }

    friend std::ostream &operator<<(std::ostream &os, const inet_address &addr)
    {
        char buf[64] ;
        return os << addr.to_strbuf(buf) ;
    }

private:
    uint32_t _addr ; /* The IP address in host byte order. */

    static uint32_t from_string(const strslice &address_string, unsigned flags) ;
    char *to_strbuf(char *buf) const
    {
        // Alas! inet_ntoa isn't thread-safe on Linux!
        sprintf(buf, "%d.%d.%d.%d", int(octet(0)), int(octet(1)), int(octet(2)), int(octet(3))) ;
        return buf ;
    }
} ;

/// Get the loopback address.
inline inet_address inaddr_loopback() { return inet_address(INADDR_LOOPBACK) ; }

/// Get the broadcast address.
inline inet_address inaddr_broadcast() { return inet_address(INADDR_BROADCAST) ; }

/// Get the address of a network interface ("lo", "eth0", etc.).
///
/// Doesn't throw exception if there is no such interface, returns an empty inet_address
inline inet_address iface_addr(const strslice &iface_name)
{
    return inet_address(iface_name, inet_address::FROM_IFACE|net::inet_address::NO_EXCEPTION) ;
}

/*******************************************************************************
 inet_address comparison operators
*******************************************************************************/
inline bool operator==(const inet_address &lhs, const inet_address &rhs)
{
    return lhs.ipaddr() == rhs.ipaddr() ;
}
inline bool operator<(const inet_address &lhs, const inet_address &rhs)
{
    return lhs.ipaddr() < rhs.ipaddr() ;
}

// Note that this line defines _all_ remaining operators (!=, <=, etc.)
PCOMN_DEFINE_RELOP_FUNCTIONS(, inet_address) ;

/******************************************************************************/
/** The completely-defined SF_INET socket address; specifies both the inet address and the
 port.

 This is a wrapper around the socaddr_in structure: you can pass a pointer returned by the
 as_sockaddr_in or as_sockaddr to @em both as input @em and as output parameters to socket
 APIs.
*******************************************************************************/
class sock_address {
public:
    /// Create an empty socket address.
    /// After this constructor, both addr().ipaddr() and port() are 0, is_null() is true.
    sock_address() { init() ; }

    /// Create a socket address with specified inet address and port.
    /// @param addr Inet address.
    /// @param port Port number.
    sock_address(const strslice &addr, uint16_t port = 0) { init(inet_address(addr), port) ; }

    /// Create a socket address with specified inet address and port.
    /// @param addr Inet address.
    /// @param port Port number.
    sock_address(const inet_address &addr, uint16_t port = 0) { init(addr, port) ; }

    /// Create a socket address on a loopback interface with specified port.
    /// @param port Port number.
    explicit sock_address(uint16_t port) { init(inaddr_loopback(), port) ; }

    /// Create a socket address from a filled sockaddr structure.
    /// @param sa Should have AF_INET address family.
    /// @throw std::invalid_argument Invalid address family.
    sock_address(const sockaddr &sa) : _sockaddr(ensure_family(&sa)) {}

    /// Create a socket address from a filled sockaddr_in structure.
    /// @param sin should have AF_INET address family.
    /// @throw std::invalid_argument Invalid address family.
    sock_address(const sockaddr_in &sin) : _sockaddr(ensure_family((const sockaddr *)&sin)) {}

    inet_address addr() const { return inet_address(_sockaddr.sin_addr) ; }

    uint16_t port() const { return ntohs(_sockaddr.sin_port) ; }

    bool is_null() const { return !addr().ipaddr() && !port() ; }

    std::string str() const ;

    const sockaddr_in *as_sockaddr_in() const { return &_sockaddr ; }
    sockaddr_in *as_sockaddr_in() { return &_sockaddr ; }

    const sockaddr *as_sockaddr() const { return reinterpret_cast<const sockaddr *>(&_sockaddr) ; }
    sockaddr *as_sockaddr() { return reinterpret_cast<sockaddr *>(&_sockaddr) ; }

    static size_t addrsize() { return sizeof(sockaddr_in) ; }

private:
    sockaddr_in _sockaddr ;

    static const sockaddr_in &ensure_family(const sockaddr *sa)
    {
        PCOMN_THROW_MSG_IF(sa->sa_family != AF_INET, std::invalid_argument, "Invalid socket family, only AF_INET allowed.") ;
        return *reinterpret_cast<const sockaddr_in *>(sa) ;
    }
    void init()
    {
        memset(&_sockaddr, 0, sizeof _sockaddr) ;
        _sockaddr.sin_family = AF_INET ;
        #ifndef  __linux
        _sockaddr.sin_len = sizeof _sockaddr ;
        #endif
    }
    void init(const in_addr &addr, uint16_t port)
    {
        init() ;
        _sockaddr.sin_port = htons(port) ;
        _sockaddr.sin_addr = addr ;
    }
} ;

/*******************************************************************************
 sock_address comparison operators
*******************************************************************************/
inline bool operator==(const sock_address &lhs, const sock_address &rhs)
{
    return lhs.port() == rhs.port() && lhs.addr() == rhs.addr() ;
}

inline bool operator<(const sock_address &lhs, const sock_address &rhs)
{
    return
        (((uint64_t)lhs.addr().ipaddr() << 16) | lhs.port()) <
        (((uint64_t)rhs.addr().ipaddr() << 16) | rhs.port()) ;
}

// Note that this line defines _all_ remaining operators (!=, <=, etc.)
PCOMN_DEFINE_RELOP_FUNCTIONS(, sock_address) ;

/*******************************************************************************
 Network address hashing
*******************************************************************************/
inline size_t hasher(const inet_address &addr)
{
    using pcomn::hasher ;
    return hasher(addr.ipaddr()) ;
}

inline size_t hasher(const sock_address &addr)
{
    using pcomn::hasher ;
    return hasher(std::make_pair(addr.addr().ipaddr(), addr.port())) ;
}

/*******************************************************************************
 Ostream output operators
*******************************************************************************/
inline std::ostream& operator<<(std::ostream &os, const sock_address &addr)
{
    return os << addr.str() ;
}

} // end of namespace pcomn::net
} // end of namespace pcomn

namespace std {

template<> struct hash<pcomn::net::inet_address> :
        public std::unary_function<pcomn::net::inet_address, std::size_t> {

    std::size_t operator()(const pcomn::net::inet_address &addr) const
    {
        using pcomn::hasher ;
        return hasher(addr) ;
    }
} ;

template<> struct hash<pcomn::net::sock_address> :
        public std::unary_function<pcomn::net::sock_address, std::size_t> {

    std::size_t operator()(const pcomn::net::sock_address &addr) const
    {
        using pcomn::hasher ;
        return hasher(addr) ;
    }
} ;

} // end of namespace std

#endif /* __NET_NETADDR_H */
