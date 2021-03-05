#pragma once
#include "balancer/io/simple_balancer.h"
#include "base/nocopyable.h"
#include "connection.h"
#include "io/io_manager.h"
#include "net/socket.h"
#include <functional>

namespace lon::net {
auto G_logger = LogManager::getInstance()->getLogger("system");

class TcpServer
    : public std::enable_shared_from_this<TcpServer>
      , Noncopyable
{
public:
    using OnConnectionCallbackType =
    std::function<void(std::shared_ptr<TcpConnection> connection)>;

    using SocketInitCallbackType = std::function<void(Socket& socket)>;

public:
    TcpServer(OnConnectionCallbackType _on_connection,
              std::unique_ptr<io::IOWorkBalancer> _balancer =
                  std::make_unique<io::SimpleIOBalancer>());

    /**
     * @brief 设置listen socket.
     * @param _socket_initer listen socket 初始化器.
    */
    auto setSocketIniter(SocketInitCallbackType _socket_initer) -> void;

    /**
     * @brief 绑定地址到tcp server.
     * @param local_address 需要绑定的本地地址.
     * @return 绑定是否成功.
    */
    bool bind(SockAddress::SharedPtr local_address);

    /**
     * @brief 绑定地址到tcp server.
     * @param local_addresses 需要绑定的本地地址列表.
     * @return 绑定成功返回nullopt, 否则返回绑定失败列表.
    */
    std::optional<std::vector<SockAddress::SharedPtr>> bind(
        const std::vector<SockAddress::SharedPtr>& local_addresses);

    /**
     * @brief 添加绑定地址到tcp server.
     * @param local_address 需要绑定的本地地址.
     * @return 绑定是否成功.
    */
    bool addBindAddr(SockAddress::SharedPtr local_address);

    /**
     * @brief 开启服务.
     * @return 开启服务是否成功.
    */
    bool startServe();

    /**
     * @brief 关闭服务.
     * @return 关闭服务是否成功.
    */
    bool stopServe();

    LON_NODISCARD LON_ALWAYS_INLINE
    bool serving() const noexcept {
        return serving_;
    }

private:
    bool bindOne(SockAddress::SharedPtr local_address);

    OnConnectionCallbackType on_connection_ = nullptr;
    SocketInitCallbackType socket_initer_ = nullptr;
    
    bool serving_ = false;
    std::vector<Socket> listen_sockets_{};
    std::unique_ptr<io::IOWorkBalancer> balancer_;
};
} // namespace lon::net
