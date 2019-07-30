/*-*- mode:c++;tab-width:4;indent-tabs-mode:nil;c-file-style:"stroustrup";c-file-offsets:((innamespace . 0)(inline-open . 0)(case-label . +)) -*-*/
#ifndef __PCOMN_NETADDR_H
#define __PCOMN_NETADDR_H
/*******************************************************************************
 FILE         :   pcomn_netaddr.h
 COPYRIGHT    :   Yakov Markovitch, 2008-2019. All rights reserved.
                  See LICENSE for information on usage/redistribution.

 DESCRIPTION  :   Internet address class(es)/functions.
 CREATION DATE:   27 Jan 2008
*******************************************************************************/
/** @file
  Classes and functions for network addresses handling.
*******************************************************************************/
#include <pcomn_utils.h>
#include <pcomn_hash.h>
#include <pcomn_string.h>
#include <pcomn_strnum.h>
#include <pcomn_strslice.h>

#include <string>

#ifdef PCOMN_PL_POSIX
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#pragma comment(lib, "Ws2_32.lib")
typedef uint32_t in_addr_t ;
#endif

#include <sys/types.h>

#include <unordered_map>
#include <array>

namespace pcomn {

class ipv4_addr ;
class ipv4_subnet ;
class ipv6_addr ;
class ipv6_subnet ;
class sock_address ;

/***************************************************************************//**
 IPv4 address.

 @note All comparison/relational operators are defined as free function, providing for
 symmetrical compare with all data types to/from which ipv4_addr defines implicit
 conversions.
*******************************************************************************/
class ipv4_addr {
    friend ipv6_addr ;
public:
    /// ipv4_addr construction mode flags
    enum CFlags {
        NO_EXCEPTION    = 0x0001, /**< Don't throw exception if construction failed,
                                     intialize ipv4_addr to 0 */
        ALLOW_EMPTY     = 0x0002, /**< Allow to pass an empty string (the resulting
                                     address will be 0.0.0.0) */

        USE_HOSTNAME    = 0x0100, /**< Attempt to interpret address string as a hostname */

        USE_IFACE       = 0x0200, /**< Attempt to interpret address string as a network
                                     interace name (e.g. "lo" or "eth0") */
        IGNORE_DOTDEC   = 0x0400, /**< Don't attempt to interpret address string as a
                                     dot-delimited IP address */

        ONLY_DOTDEC     = 0,
        ONLY_HOSTNAME   = USE_HOSTNAME|IGNORE_DOTDEC,
        ONLY_IFACE      = USE_IFACE|IGNORE_DOTDEC
    } ;

    /// Create default address (0.0.0.0).
    constexpr ipv4_addr() = default ;

    explicit constexpr ipv4_addr(uint32_t host_order_inetaddr) : _addr(host_order_inetaddr) {}

    constexpr ipv4_addr(const in_addr &addr) : _addr(value_from_big_endian(addr.s_addr)) {}

    constexpr ipv4_addr(uint8_t a, uint8_t b, uint8_t c, uint8_t d) :
        _addr(((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | d)
    {}

    /// Create an IP address from its human-readable text representation.
    /// @param address_string Dot-delimited IP address or interface name, like "eth0", or
    /// host name (tried in that order).
    /// @param flags        ORed ConstructFlags flags.
    /// @return Resolved IP address. If cannot resolve, throws system_error or constructs
    /// an empty ipv4_addr (i.e. ipaddr()==0), depending on NO_EXCEPTION flag.
    ipv4_addr(const strslice &address_string, CFlags flags = ONLY_DOTDEC) :
        _addr(from_string(address_string, flags))
    {}

    explicit constexpr operator bool() const { return !!_addr ; }

    static constexpr ipv4_addr localhost() { return {127, 0, 0, 1} ; }

    /// Get one octet of an IP address by octet index (0-3).
    constexpr uint8_t octet(unsigned ndx) const { return (_addr >> 8*(3 - ndx)) ; }

    /// Get all four octets of an IP address.
    constexpr std::array<uint8_t, 4> octets() const
    {
        union {
            uint32_t addr ;
            std::array<uint8_t, 4> result ;
        } netaddr = { value_to_big_endian(_addr) } ;
        return netaddr.result ;
    }

    /// Get an IP address as a 32-bit unsigned number in the host byte order.
    constexpr uint32_t ipaddr() const { return _addr ; }

    constexpr in_addr inaddr() const
    {
        union {
            uint32_t addr ;
            in_addr  result ;
        } netaddr = { value_to_big_endian(_addr) } ;
        return netaddr.result ;
    }
    constexpr operator struct in_addr() const { return inaddr() ; }
    /// Get an IP address as a 32-bit unsigned number in the host byte order.
    explicit constexpr operator uint32_t() const { return ipaddr() ; }

    constexpr ipv4_addr next() const { return ipv4_addr(ipaddr() + 1) ; }
    constexpr ipv4_addr prev() const { return ipv4_addr(ipaddr() - 1) ; }
    static constexpr ipv4_addr last() { return ipv4_addr((uint32_t)~0) ; }

    /// Get a hostname for the address.
    /// @throw nothing
    std::string hostname() const;

    /// Get the maximum length of dotted-decimal string representation of IPv4 address
    /// (non including terminating zero).
    static constexpr size_t slen() { return INET_ADDRSTRLEN - 1 ; }

    /// Get dotted-decimal representation of IP address.
    std::string dotted_decimal() const ;

    std::string str() const { return dotted_decimal() ; }

    template<typename OutputIterator>
    OutputIterator to_str(OutputIterator s) const
    {
        addr_strbuf buf ;
        to_strbuf(buf) ;
        for (const char *p = buf ; *p ; ++p, ++s) *s = *p ;
        return s ;
    }

    friend std::ostream &operator<<(std::ostream &os, const ipv4_addr &addr)
    {
        addr_strbuf buf ;
        return os << addr.to_strbuf(buf) ;
    }

private:
    uint32_t _addr = 0 ; /* IPv4 address in host byte order. */

private:
    typedef char addr_strbuf[INET_ADDRSTRLEN] ;

    static uint32_t from_string(const strslice &address_string, CFlags flags) ;

    const char *to_strbuf(char *buf) const
    {
        const in_addr netaddr = inaddr() ;
        return inet_ntop(AF_INET, &netaddr, buf, sizeof(addr_strbuf)) ;
    }
} ;

PCOMN_DEFINE_FLAG_ENUM(ipv4_addr::CFlags) ;

/// Get the loopback address.
constexpr inline ipv4_addr inaddr_loopback() { return ipv4_addr(INADDR_LOOPBACK) ; }

/// Get the broadcast address.
constexpr inline ipv4_addr inaddr_broadcast() { return ipv4_addr(INADDR_BROADCAST) ; }

/// Get the address of a network interface ("lo", "eth0", etc.).
///
/// Doesn't throw exception if there is no such interface, returns an empty ipv4_addr
inline ipv4_addr iface_addr(const strslice &iface_name)
{
    return ipv4_addr(iface_name, ipv4_addr::ONLY_IFACE|ipv4_addr::NO_EXCEPTION) ;
}

/*******************************************************************************
 ipv4_addr comparison operators
*******************************************************************************/
constexpr inline bool operator==(const ipv4_addr &x, const ipv4_addr &y)
{
    return x.ipaddr() == y.ipaddr() ;
}
constexpr inline bool operator<(const ipv4_addr &x, const ipv4_addr &y)
{
    return x.ipaddr() < y.ipaddr() ;
}

// Note that this line defines _all_ remaining operators (!=, <=, etc.)
PCOMN_DEFINE_RELOP_FUNCTIONS(constexpr, ipv4_addr) ;

/***************************************************************************//**
 Subnetwork address, i.e. IPV4 address + prefix length
*******************************************************************************/
class ipv4_subnet {
public:
    constexpr ipv4_subnet() = default ;

    ipv4_subnet(uint32_t host_order_inetaddr, unsigned prefix_length) :
        _pfxlen(ensure_pfxlen<std::invalid_argument>(prefix_length)),
        _addr(host_order_inetaddr)
    {}

    ipv4_subnet(const ipv4_addr &address, unsigned prefix_length) :
        ipv4_subnet(address.ipaddr(), prefix_length)
    {}

    ipv4_subnet(const in_addr &addr, unsigned prefix_length) :
        ipv4_subnet(ipv4_addr(addr), prefix_length)
    {}

    ipv4_subnet(uint8_t a, uint8_t b, uint8_t c, uint8_t d, unsigned prefix_length) :
        ipv4_subnet(ipv4_addr(a, b, c, d), prefix_length)
    {}

    /// Create an IP address from its human-readable text representation.
    /// @param address_string Dotted-decimal subnet mask, like "139.12.0.0/16"
    ipv4_subnet(const strslice &subnet_string, RaiseError raise_error = RAISE_ERROR) ;

    explicit operator bool() const { return !!raw() ; }

    const ipv4_addr &addr() const { return _addr ; }

    operator ipv4_addr() const { return _addr ; }

    ipv4_subnet subnet() const { return ipv4_subnet(addr().ipaddr() & netmask(), pfxlen()) ; }

    /// Get subnet prefix length
    constexpr unsigned pfxlen() const { return _pfxlen ; }

    /// Get subnet mask (host order)
    constexpr uint32_t netmask() const { return (uint32_t)(~0ULL << (32 - _pfxlen)) ; }

    constexpr bool is_host() const { return pfxlen() == 32 ; }
    constexpr bool is_any() const { return pfxlen() == 0 ; }

    /// Get the closed address interval for this subnetwork
    ///
    /// The resulting interval is @em closed, hence includes both its endpoints.
    /// This is so because it is impossible to specify "past-the-end" for a range
    /// ending with 255.255.255.255
    ///
    unipair<ipv4_addr> addr_range() const
    {
        const uint32_t first = addr().ipaddr() & netmask() ;
        const uint32_t last =  uint32_t(first + (0x100000000ULL >> pfxlen()) - 1) ;
        return {ipv4_addr(first), ipv4_addr(last)} ;
    }

    /// "Raw" value: IP address and network mask represented as a single 64-bit integer
    uint64_t raw() const { return (((uint64_t)addr().ipaddr() << 32) | pfxlen()) ; }

    std::string str() const
    {
        char buf[64] ;
        return std::string(buf + 0, to_str(buf)) ;
    }

    template<typename OutputIterator>
    OutputIterator to_str(OutputIterator s) const
    {
        s = addr().to_str(s) ;
        *s = '/' ;
        return numtoiter(pfxlen(), ++s) ;
    }

    friend std::ostream &operator<<(std::ostream &os, const ipv4_subnet &addr)
    {
        char buf[64] ;
        return os << strslice(buf + 0, addr.to_str(buf)) ;
    }

private:
    uint32_t  _pfxlen = 0 ;  /* Subnetwork prefix length */
    ipv4_addr _addr ;    /* IP address */

    template<typename X>
    static uint32_t ensure_pfxlen(unsigned prefix_length)
    {
        ensure<X>(prefix_length <= 32, "Subnetwork address prefix length exceeds 32") ;
        return prefix_length ;
    }
} ;

/*******************************************************************************
 ipv4_subnet comparison operators
*******************************************************************************/
inline bool operator==(const ipv4_subnet &x, const ipv4_subnet &y)
{
    return x.raw() == y.raw() ;
}

inline bool operator<(const ipv4_subnet &x, const ipv4_subnet &y)
{
    return x.raw() < y.raw() ;
}

PCOMN_DEFINE_RELOP_FUNCTIONS(, ipv4_subnet) ;

/***************************************************************************//**
 The completely-defined SF_INET socket address; specifies both the inet address
 and the port.

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
    sock_address(const strslice &addr, uint16_t port = 0)
    {
        init(ipv4_addr(addr, ipv4_addr::USE_HOSTNAME), port) ;
    }

    /// Create a socket address with specified inet address and port.
    /// @param addr Inet address.
    /// @param port Port number.
    sock_address(const ipv4_addr &addr, uint16_t port = 0) { init(addr, port) ; }

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

    ipv4_addr addr() const { return ipv4_addr(_sockaddr.sin_addr) ; }

    uint16_t port() const { return ntohs(_sockaddr.sin_port) ; }

    /// "Raw" value: IP address and port represented as a single 64-bit integer
    uint64_t raw() const { return (((uint64_t)addr().ipaddr() << 32) | port()) ; }

    bool is_null() const { return !raw() ; }

    explicit operator bool() const { return !is_null() ; }

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
        _sockaddr = sockaddr_in{} ;
        _sockaddr.sin_family = AF_INET ;
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
inline bool operator==(const sock_address &x, const sock_address &y)
{
    return x.port() == y.port() && x.addr() == y.addr() ;
}

inline bool operator<(const sock_address &x, const sock_address &y)
{
    return
        (((uint64_t)x.addr().ipaddr() << 16) | x.port()) <
        (((uint64_t)y.addr().ipaddr() << 16) | y.port()) ;
}

// Note that this line defines _all_ remaining operators (!=, <=, etc.)
PCOMN_DEFINE_RELOP_FUNCTIONS(, sock_address) ;

/***************************************************************************//**
 IPv6 address in network byte order.

 @note Implicitly castable to binary128_t in the network (BE) byte order.
*******************************************************************************/
class ipv6_addr : public binary128_t {
    typedef binary128_t ancestor ;
public:
    /// ipv6_addr construction mode flags
    enum CFlags {
        NO_EXCEPTION    = 0x0001, /**< Don't throw exception if construction failed,
                                     intialize ipv6_addr to :: */
        ALLOW_EMPTY     = 0x0002  /**< Allow to pass an empty string (the resulting
                                     address will be ::) */
    } ;

    /// Create the default address (::).
    constexpr ipv6_addr() = default ;

    explicit constexpr ipv6_addr(const binary128_t &net_order_inetaddr) :
        ancestor(net_order_inetaddr)
    {}

    /// Create IPv6 address from explicitly specified hextets.
    constexpr ipv6_addr(uint16_t h1, uint16_t h2, uint16_t h3, uint16_t h4,
                        uint16_t h5, uint16_t h6, uint16_t h7, uint16_t h8) :
        ancestor(h1, h2, h3, h4, h5, h6, h7, h8)
    {}

    /// Implicit conversion ipv4->ipv6
    constexpr ipv6_addr(const ipv4_addr &ipv4) :
        ancestor(0, 0, 0, 0, 0, 0xffffu, ipv4.ipaddr() >> 16, uint16_t(ipv4.ipaddr()))
    {}

    /// Create an IP address from its human-readable text representation.
    /// @param address_string Dot-delimited IP address or interface name, like "eth0", or
    /// host name (tried in that order).
    /// @param flags        ORed ConstructFlags flags.
    /// @return Resolved IP address. If cannot resolve, throws system_error or constructs
    /// an empty ipv6_addr (i.e. ipaddr()==0), depending on NO_EXCEPTION flag.
    ipv6_addr(const strslice &address_string, CFlags flags = {}) :
        ancestor(from_string(address_string, flags))
    {}

    explicit constexpr operator bool() const { return static_cast<bool>(data()) ; }

    static constexpr ipv6_addr localhost() { return {0, 0, 0, 0, 0, 0, 0, 1} ; }

    /// Get the nth hextet of the address.
    /// The hextet is returned as host-order word.
    using ancestor::hextet ;
    using ancestor::octet ;
    using ancestor::operator bool ;

    /// Get all the eight hextets of an IPv6 address.
    constexpr std::array<uint16_t, 8> hextets() const
    {
        return
            {hextet(0), hextet(1), hextet(2), hextet(3),
             hextet(4), hextet(5), hextet(6), hextet(7)} ;
    }

    constexpr in6_addr inaddr() const
    {
        union {
            binary128_t addr ;
            in6_addr    result ;
        } _ = { data() } ;
        return _.result ;
    }
    operator struct in6_addr() const { return inaddr() ; }

    constexpr bool is_mapped_ipv4() const
    {
        return !(_idata[0] | (_wdata[2] ^ value_to_big_endian(0xffffu))) ;
    }

    /// Get the maximum length of string representation of IPv6 address
    /// (non including terminating zero).
    static constexpr size_t slen() { return INET6_ADDRSTRLEN - 1 ; }

    template<typename OutputIterator>
    OutputIterator to_str(OutputIterator s) const
    {
        addr_strbuf buf ;
        to_strbuf(buf) ;
        for (const char *p = buf ; *p ; ++p, ++s) *s = *p ;
        return s ;
    }

    std::string str() const
    {
        addr_strbuf buf ;
        return std::string(to_strbuf(buf)) ;
    }

    friend constexpr bool operator==(const ipv6_addr &x, const ipv6_addr &y)
    {
        return x.data() == y.data() ;
    }

    friend constexpr bool operator<(const ipv6_addr &x, const ipv6_addr &y)
    {
        return x.data() < y.data() ;
    }

    friend std::ostream &operator<<(std::ostream &os, const ipv6_addr &addr)
    {
        char buf[64] ;
        return os << addr.to_strbuf(buf) ;
    }

private:
    typedef char addr_strbuf[INET6_ADDRSTRLEN] ;

    constexpr const binary128_t &data() const { return *this ; }

    static binary128_t from_string(const strslice &address_string, CFlags flags) ;

    const char *to_strbuf(addr_strbuf) const ;

    struct zero_run { short start = -1 ; short len = 0 ; } ;
    // Find the longest run of zeros in the address for :: shorthanding
    zero_run find_longest_zero_run() const ;

    static __noreturn __cold void invalid_address_string(const strslice &address_string) ;
} ;

/*******************************************************************************
 ipv6_addr comparison operators
*******************************************************************************/
// Note that this line defines _all_ remaining operators (!=, <=, etc.)
PCOMN_DEFINE_RELOP_FUNCTIONS(constexpr, ipv6_addr) ;

/*******************************************************************************
 Ostream output operators
*******************************************************************************/
inline std::ostream& operator<<(std::ostream &os, const sock_address &addr)
{
    return os << addr.str() ;
}

/***************************************************************************//**
 Backward compatibility typedef
*******************************************************************************/
/**@{*/
typedef ipv4_addr   inet_address ;
typedef ipv4_subnet subnet_address ;
/**@}*/

} // end of namespace pcomn

namespace std {
/***************************************************************************//**
 Network address hashing
*******************************************************************************/
/**@{*/
template<> struct hash<pcomn::ipv4_addr> {
    size_t operator()(const pcomn::ipv4_addr &addr) const
    {
        return pcomn::valhash(addr.ipaddr()) ;
    }
} ;
template<> struct hash<pcomn::sock_address> {
    size_t operator()(const pcomn::sock_address &addr) const
    {
        return pcomn::t1ha0_bin128(addr.port(), addr.addr().ipaddr()) ;
    }
} ;
/**@}*/
} // end of namespace std

#endif /* __PCOMN_NETADDR_H */
