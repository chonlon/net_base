#pragma once

#include "../base/typedef.h"
#include "../base/lstring.h"
#include "../base/macro.h"

#include <iosfwd>
#include <netinet/in.h>
#include <sys/un.h>

#include <sys/socket.h>


namespace lon::net {
class SockAddress
{
public:
    using UniquePtr = std::unique_ptr<SockAddress>;
    using SharedPtr = std::shared_ptr<SockAddress>;

    virtual ~SockAddress() = default;

    LON_NODISCARD
    virtual socklen_t getAddrLen() const noexcept = 0;

    LON_NODISCARD
    sa_family_t getFamily() const noexcept;

    LON_NODISCARD
    bool isInetFamily() const noexcept;

    /**
     * @brief 对应的原生sockaddr指针, 如果未初始化返回地址类型为 AF_UNSPEC.
    */
    LON_NODISCARD
    virtual const sockaddr* getAddr() const noexcept = 0;

    /**
     * @brief 对应的可修改原生sockaddr指针, 如果未初始化返回地址类型为 AF_UNSPEC.
    */
    LON_NODISCARD
    virtual sockaddr* getAddrMutable() = 0;

    LON_NODISCARD
    virtual String toString() const = 0;

    bool operator<(const SockAddress& rhs) const;
    bool operator==(const SockAddress& rhs) const;
    bool operator!=(const SockAddress& rhs) const;

    friend std::ostream& operator<<(std::ostream& os, lon::net::SockAddress const& address);
};


class IPAddress : public SockAddress
{
public:
    using IPAddressUniquePtr = std::unique_ptr<IPAddress>;

    ~IPAddress() override = default;

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
    IPV4Address() noexcept;

    /**
     * @brief 构造IPV4 地址.
     * @param host_and_port  "host:port" style string.
     * @throw invalid_argument 如果不满足ipv4特征的参数.
    */
    IPV4Address(StringArg host_and_port);

    /**
     * @brief 构造IPV4 地址.
     * @param host host的string.
     * @param port port的string类型.
     * @throw invalid_argument 如果不满足ipv4特征的参数.
    */
    IPV4Address(StringArg host, StringArg port);

    /**
     * @brief 构造IPV4 地址.
     * @param host host的string.
     * @param port local编码的port.
     * @throw invalid_argument 如果不满足ipv4特征的参数.
    */
    IPV4Address(StringArg host, uint16_t port);

    /**
     * @brief 直接以原生sockaddr初始化地址.
    */
    explicit IPV4Address(const sockaddr_in& src) noexcept;

    /**
     * @brief see @IPV4Address::IPV4Address
    */
    void setByHostAndPort(StringArg host_and_port);
    /**
     * @brief see @IPV4Address::IPV4Address
    */
    void setByHostAndPort(StringArg host, StringArg port);
    /**
     * @brief see @IPV4Address::IPV4Address
    */
    void setByHostAndPort(StringArg host, uint16_t port);


    const sockaddr* getAddr() const noexcept override;
    sockaddr* getAddrMutable() override;
    String toString() const override;
    String getAddressStr() const override;
    void setPort(uint16_t port) override;
    uint16_t getPort() override;


    LON_NODISCARD
    socklen_t getAddrLen() const noexcept override;
private:
    

    sockaddr_in addr_;
};

class IPV6Address : public IPAddress
{
public:
    IPV6Address() noexcept;

    /**
     * @brief 构造IPV6 地址.
     * @param host_and_port  "host:port" style string.
     * @throw invalid_argument 如果不满足ipv4特征的参数.
    */
    IPV6Address(StringArg host_and_port);

    /**
     * @brief 构造IPV6 地址.
     * @param host host的string.
     * @param port port的string类型.
     * @throw invalid_argument 如果不满足ipv4特征的参数.
    */
    IPV6Address(StringArg host, StringArg port);

    /**
     * @brief 构造IPV4 地址.
     * @param host host的string.
     * @param port local编码的port.
     * @throw invalid_argument 如果不满足ipv4特征的参数.
    */
    IPV6Address(StringArg host, uint16_t port);
    /**
     * @brief 直接以原生sockaddr初始化地址.
    */
    explicit IPV6Address(const sockaddr_in6& src) noexcept;

    /**
     * @brief see @IPV6Address::IPV6Address
    */
    void setByHostAndPort(StringArg host_and_port);

    /**
     * @brief see @IPV6Address::IPV6Address
    */
    void setByHostAndPort(StringArg host, StringArg port);

    /**
     * @brief see @IPV6Address::IPV6Address
    */
    void setByHostAndPort(StringArg host, uint16_t port);

    const sockaddr* getAddr() const noexcept override;
    sockaddr* getAddrMutable() override;
    String toString() const override;
    String getAddressStr() const override;
    void setPort(uint16_t port) override;
    uint16_t getPort() override;


    LON_NODISCARD
    socklen_t getAddrLen() const noexcept override;
private:
    

    sockaddr_in6 addr_;
};

class UnixAddress : public SockAddress
{
public:
    UnixAddress() noexcept;

    const sockaddr* getAddr() const noexcept override;
    sockaddr* getAddrMutable() override;
    String toString() const override;


    socklen_t getAddrLen() const noexcept override;
private:
    
    sockaddr_un addr_;
};

}


std::ostream& operator<<(std::ostream& os, lon::net::SockAddress const& address);
