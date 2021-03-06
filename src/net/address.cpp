#include "net/address.h"

#include "base/bytes.h"


#include <cassert>
#include <netdb.h>
#include <cinttypes>
#include <stdexcept>
#include <arpa/inet.h>
#include <net/if.h>
#include <fmt/core.h>

namespace lon::net {
// Host和Port的RAII类, 由于不管是inet_pton还是getaddrinfo都需要c风格的str, 所以需要在host和port中打断.
struct HostAndPortParser
{
    HostAndPortParser(const char* host_and_port)
        : host(nullptr),
          port(nullptr),
          allocated(nullptr) {
        // Look for the last colon
        const char* colon = strrchr(host_and_port, ':');
        if (colon == nullptr) {
            // No colon, just a port number.
            throw std::invalid_argument(
                "expected a host and port string of the "
                "form \"<host>:<port>\"");

        }

        // We have to make a copy of the string so we can modify it
        // and change the colon to a NUL terminator.
        allocated = strdup(host_and_port);
        if (!allocated) {
            throw std::bad_alloc();
        }

        char* allocatedColon = allocated + (colon - host_and_port);
        *allocatedColon      = '\0';
        host                 = allocated;
        port                 = allocatedColon + 1;
        // bracketed IPv6 address, remove the brackets
        // allocatedColon[-1] is fine, as allocatedColon >= host and
        // *allocatedColon != *host therefore allocatedColon > host
        if (*host == '[' && allocatedColon[-1] == ']') {
            allocatedColon[-1] = '\0';
            ++host;
        }
    }

    ~HostAndPortParser() {
        free(allocated);
    }

    const char* host;
    const char* port;
    char* allocated;
};

struct ScopedAddrInfo
{
    explicit ScopedAddrInfo(struct addrinfo* addrinfo)
        : info(addrinfo) {
    }

    ~ScopedAddrInfo() {
        freeaddrinfo(info);
    }

    struct addrinfo* info;
};


static struct addrinfo* getAddrInfo(StringArg host,
                                    StringArg port,
                                    int flags) {
    struct addrinfo hints;
    bzero(&hints, sizeof(addrinfo));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE | AI_NUMERICSERV | flags;
    struct addrinfo* result;
    if ((::getaddrinfo(host.str, port.str, &hints, &result)) != 0) {
        throw std::invalid_argument(fmt::format(
            "resolve address failed, for host:{} port:{}, with {}(errno={})",
            host.str,
            port.str,
            strerror(errno),
            errno));
    }
    return result;
}

static uint16_t getPortFromString(StringArg port) {
    uint16_t port_num = 0;
    if (strlen(port.str) > sizeof("65535") - 1)
        throw std::invalid_argument(
            fmt::format("invalid port format, port str:{}", port.str));
    for (int i = 0; port.str[i] != '\0'; ++i) {
        if (port.str[i] > '9' || port.str[i] < '0') {
            throw std::invalid_argument(
                fmt::format("invalid port format, port str:{}", port.str));
        }
        port_num = static_cast<uint16_t>(port_num * 10 + port.str[i] - '0');
    }
    return port_num;
}


sa_family_t SockAddress::getFamily() const noexcept {
    return getAddr()->sa_family;
}

sa_family_t SockAddress::isInetFamily() const noexcept {
    return getFamily() == AF_INET ? true : getFamily() == AF_INET6 ? true : false;
}


bool SockAddress::operator<(const SockAddress& rhs) const {
    socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen());
    int result = memcmp(getAddr(), rhs.getAddr(), minlen);
    if (result < 0) {
        return true;
    }
    else if (result > 0) {
        return false;
    }
    else if (getAddrLen() < rhs.getAddrLen()) {
        return true;
    }
    return false;
}

bool SockAddress::operator==(const SockAddress& rhs) const {
    return getAddrLen() == rhs.getAddrLen()
        && memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
}

bool SockAddress::operator!=(const SockAddress& rhs) const {
    return !this->operator==(rhs);
}

IPAddress::IPAddressUniquePtr IPAddress::
create(StringArg host, uint16_t port) {
    char port_str[sizeof("65535")];
    snprintf(port_str, sizeof(port_str), "%" PRIu16, port);
    return create(host, port_str);
}

IPAddress::IPAddressUniquePtr IPAddress::create(StringArg host,
                                                StringArg port) {

    ScopedAddrInfo addr_info(getAddrInfo(host, port, 0));
    if (addr_info.info->ai_family == AF_INET) {
        if (addr_info.info->ai_addrlen < sizeof(sockaddr_in)) {
            throw std::invalid_argument("result sockaddr len too short");
        }
        return std::make_unique<IPV4Address>(
            *reinterpret_cast<sockaddr_in*>(addr_info.info->ai_addr));
    } else if (addr_info.info->ai_family == AF_INET6) {
        if (addr_info.info->ai_addrlen < sizeof(sockaddr_in6)) {
            throw std::invalid_argument("result sockaddr len too short");
        }
        return std::make_unique<IPV6Address>(
            *reinterpret_cast<sockaddr_in6*>(addr_info.info->ai_addr));
    }
    return nullptr;
}

IPAddress::IPAddressUniquePtr IPAddress::create(StringArg host_and_port) {
    HostAndPortParser host_and_port_parser(host_and_port.str);
    return create(host_and_port_parser.host, host_and_port_parser.port);
}

IPV4Address::IPV4Address(StringArg host_and_port)
    : IPV4Address() {
    setByHostAndPort(host_and_port);
}

IPV4Address::IPV4Address(StringArg host, StringArg port)
    : IPV4Address() {
    setByHostAndPort(host, port);
}

IPV4Address::IPV4Address(StringArg host, uint16_t port)
    : IPV4Address() {
    setByHostAndPort(host, port);
}

IPV4Address::IPV4Address(const sockaddr_in& src) noexcept
    : addr_{src} {
}

void IPV4Address::setByHostAndPort(StringArg host_and_port) {
    HostAndPortParser host_and_port_parser(host_and_port.str);
    setByHostAndPort(host_and_port_parser.host, host_and_port_parser.port);
}

void IPV4Address::setByHostAndPort(StringArg host, StringArg port) {
    uint16_t port_num = getPortFromString(port);
    setByHostAndPort(host, port_num);
}

void IPV4Address::setByHostAndPort(StringArg host, uint16_t port) {
    if (int ret = ::inet_pton(AF_INET, host.str, &addr_.sin_addr); ret != 1) {
        assert(ret != -1); // 第一个参数不可能错误.
        throw std::invalid_argument(fmt::format(
            "host:{} does not contain a character string representing a valid network address in the specified address family",
            host.str));
    }
    addr_.sin_port   = ::htons(port);
    addr_.sin_family = AF_INET;
}


IPV4Address::IPV4Address() noexcept {
    bzero(&addr_, sizeof(sockaddr_in));
}

const sockaddr* IPV4Address::getAddr() const noexcept {
    return reinterpret_cast<const sockaddr*>(&addr_);
}

sockaddr* IPV4Address::getAddrMutable() {
    return reinterpret_cast<sockaddr*>(&addr_);
}

String IPV4Address::toString() const {
    return fmt::format("{}:{}", getAddressStr(), ::ntohs(addr_.sin_port));
}


String IPV4Address::getAddressStr() const {
    uint32_t addr_num = ::ntohl(addr_.sin_addr.s_addr);

    return fmt::format("{}.{}.{}.{}", (addr_num >> 24) & 0xff, (addr_num >> 16) & 0xff, (addr_num >> 8) & 0xff, addr_num & 0xff);
}

void IPV4Address::setPort(uint16_t port) {
    addr_.sin_port = port;
}

uint16_t IPV4Address::getPort() {
    return addr_.sin_port;
}

socklen_t IPV4Address::getAddrLen() const noexcept {
    return sizeof(addr_);
}


IPV6Address::IPV6Address(StringArg host_and_port)
    : IPV6Address() {
    setByHostAndPort(host_and_port);
}

IPV6Address::IPV6Address(StringArg host, StringArg port)
    : IPV6Address() {
    setByHostAndPort(host, port);
}

IPV6Address::IPV6Address(StringArg host, uint16_t port)
    : IPV6Address() {
    setByHostAndPort(host, port);
}

IPV6Address::IPV6Address(const sockaddr_in6& src) noexcept : addr_{src} {
}

IPV6Address::IPV6Address() noexcept {
    bzero(&addr_, sizeof(sockaddr_in6));
}

void IPV6Address::setByHostAndPort(StringArg host_and_port) {
    HostAndPortParser host_and_port_parser(host_and_port.str);
    setByHostAndPort(host_and_port_parser.host, host_and_port_parser.port);
}

void IPV6Address::setByHostAndPort(StringArg host, StringArg port) {
    uint16_t port_num = getPortFromString(port);
    setByHostAndPort(host, port_num);
}

void IPV6Address::setByHostAndPort(StringArg host, uint16_t port) {
    if (int ret = ::inet_pton(AF_INET6, host.str, &addr_.sin6_addr); ret != 1) {
        assert(ret != -1); // 第一个参数不可能错误.
        throw std::invalid_argument(fmt::format(
            "host:{} does not contain a character string representing a valid network address in the specified address family",
            host.str));
    }
    addr_.sin6_port = ::htons(port);
    addr_.sin6_family = AF_INET6;
}

const sockaddr* IPV6Address::getAddr() const noexcept {
    return reinterpret_cast<const sockaddr*>(&addr_);
}

sockaddr* IPV6Address::getAddrMutable() {
    return reinterpret_cast<sockaddr*>(&addr_);
}

String IPV6Address::toString() const {
    char buffer[INET6_ADDRSTRLEN + IFNAMSIZ + 1];
    if(!inet_ntop(AF_INET6, addr_.sin6_addr.s6_addr , buffer, INET_ADDRSTRLEN)) {
        return fmt::format("invalid ipv6 address with hex:{}", toHex(addr_.sin6_addr.s6_addr, sizeof(addr_.sin6_addr.s6_addr)));
    }
    if (addr_.sin6_scope_id != 0) {
        auto len = strlen(buffer);
        buffer[len] = '%';

        auto errsv = errno;
        if (!if_indextoname(addr_.sin6_scope_id, buffer + len + 1)) {
            // if we can't map the if because eg. it no longer exists,
            // append the if index instead
            snprintf(buffer + len + 1, IFNAMSIZ, "%" PRIu32, addr_.sin6_scope_id);
        }
        errno = errsv;
    }

    return String(buffer);
}


String IPV6Address::getAddressStr() const {
    char buffer[INET6_ADDRSTRLEN + 1];
    if (!inet_ntop(AF_INET6, addr_.sin6_addr.s6_addr, buffer, INET_ADDRSTRLEN)) {
        return fmt::format("invalid ipv6 address with hex:{}", toHex(addr_.sin6_addr.s6_addr, sizeof(addr_.sin6_addr.s6_addr)));
    }
    return String(buffer);
}

void IPV6Address::setPort(uint16_t port) {
    addr_.sin6_port = port;
}

uint16_t IPV6Address::getPort() {
    return addr_.sin6_port;
}

socklen_t IPV6Address::getAddrLen() const noexcept {
    return sizeof(addr_);
}

UnixAddress::UnixAddress() noexcept {
    bzero(&addr_, sizeof(sockaddr_un));
}

const sockaddr* UnixAddress::getAddr() const noexcept {
    return reinterpret_cast<const sockaddr*>(&addr_);
}

sockaddr* UnixAddress::getAddrMutable() {
    return reinterpret_cast<sockaddr*>(&addr_);
}

String UnixAddress::toString() const {
    return fmt::format("\\0{}",addr_.sun_path);
}

socklen_t UnixAddress::getAddrLen() const noexcept {
    return sizeof(addr_);
}


}

std::ostream& operator<<(std::ostream& os, lon::net::SockAddress const& address) {
    os << address.toString();
    return os;
}