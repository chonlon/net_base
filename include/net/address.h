#pragma once
#include <iosfwd>
#include <netinet/in.h>
#include <sys/un.h>
#include <base/typedef.h>
#include <base/string_piece.h>
#include <base/macro.h>
#include <sys/socket.h>


namespace lon::net {
class SockAddress
{
public:
    using UniquePtr = std::unique_ptr<SockAddress>;
    using SharedPtr = std::shared_ptr<SockAddress>;

    LON_NODISCARD
    virtual socklen_t getSockLen() const = 0;

    LON_NODISCARD
    sa_family_t getFamily() const;

    LON_NODISCARD
    sa_family_t isInetFamily() const;

    LON_NODISCARD
    virtual const sockaddr* getAddr() const = 0;

    LON_NODISCARD
    virtual sockaddr* getAddrMutable() = 0;

    LON_NODISCARD
    virtual String toString() const = 0;

    bool operator<(const SockAddress& rhs) const;
    bool operator==(const SockAddress& rhs) const;
    bool operator!=(const SockAddress& rhs) const;


};


class IPAddress : public SockAddress
{
public:
    using IPAddressUniquePtr = std::unique_ptr<IPAddress>;

    static IPAddressUniquePtr create(StringArg host, uint16_t port);
    static IPAddressUniquePtr create(StringArg host, StringArg port);

    static IPAddressUniquePtr create(StringArg host_and_port);

    virtual String getAddressStr() const = 0;

    virtual void setPort(uint16_t port) = 0;
    virtual uint16_t getPort() = 0;
};

class IPV4Address : public IPAddress
{
public:


    IPV4Address(StringArg host_and_port);

    IPV4Address(StringArg host, StringArg port);

    IPV4Address(StringArg host, uint16_t port);

    explicit IPV4Address(const sockaddr_in& src) noexcept;


    void setByHostAndPort(StringArg host_and_port);
    void setByHostAndPort(StringArg host, StringArg port);
    void setByHostAndPort(StringArg host, uint16_t port);


    const sockaddr* getAddr() const override;
    sockaddr* getAddrMutable() override;
    String toString() const override;
    String getAddressStr() const override;
    void setPort(uint16_t port) override;
    uint16_t getPort() override;


    LON_NODISCARD
    socklen_t getSockLen() const override;
private:
    IPV4Address() noexcept;

    sockaddr_in addr_;
};

class IPV6Address : public IPAddress
{
public:
    IPV6Address(StringArg host_and_port);;

    IPV6Address(StringArg host, StringArg port);;

    IPV6Address(StringArg host, uint16_t port);;

    explicit IPV6Address(const sockaddr_in6& src) noexcept;


    void setByHostAndPort(StringArg host_and_port);
    void setByHostAndPort(StringArg host, StringArg port);
    void setByHostAndPort(StringArg host, uint16_t port);

    const sockaddr* getAddr() const override;
    sockaddr* getAddrMutable() override;
    String toString() const override;
    String getAddressStr() const override;
    void setPort(uint16_t port) override;
    uint16_t getPort() override;


    LON_NODISCARD
    socklen_t getSockLen() const override;
private:
    IPV6Address() noexcept;

    sockaddr_in6 addr_;
};

class UnixAddress : public SockAddress
{
public:

    const sockaddr* getAddr() const override;
    sockaddr* getAddrMutable() override;
    String toString() const override;


    socklen_t getSockLen() const override;
private:
    UnixAddress() noexcept;

    sockaddr_un addr_;
};

}


std::ostream& operator<<(std::ostream& os, lon::net::SockAddress const& address);
