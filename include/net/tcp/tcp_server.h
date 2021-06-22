#pragma once
#include "../../balancer/io/simple_balancer.h"
#include "../../base/nocopyable.h"
#include "../../io/io_manager.h"
#include "connection.h"
#include "net/socket.h"
#include <functional>

namespace lon::net {


class TcpServer
    : public std::enable_shared_from_this<TcpServer>
      , Noncopyable
{
public:
    using OnConnectionCallbackType =
    std::function<void(std::shared_ptr<TcpConnection> connection)>;

    using SocketInitCallbackType = std::function<void(Socket& socket)>;

    using Ptr = std::shared_ptr<TcpServer>;

public:
    TcpServer(OnConnectionCallbackType _on_connection,
              std::unique_ptr<io::IOWorkBalancer> _balancer =
                  std::make_unique<io::SimpleIOBalancer>());

    ~TcpServer();

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

    /**
     * @brief accept成功时分配动作;
     *  默认动作是交给均衡器来调度, 对于优先级调度器需要的优先级参数需要动态配置, 所以默认动作对于优先级均衡器无效.
    */
    virtual void onAccept(std::shared_ptr<TcpConnection> connection) {
        balancer_->schedule(std::make_shared<coroutine::Executor>(
            [on_connection = this->on_connection_, connection]() {
                on_connection(connection);
        }), 0);
    }

private:
    bool bindOne(SockAddress::SharedPtr local_address);

    OnConnectionCallbackType on_connection_ = nullptr;
    SocketInitCallbackType socket_initer_ = nullptr;

protected:
    bool serving_ = false;
    std::vector<Socket> listen_sockets_{};
    std::unique_ptr<io::IOWorkBalancer> balancer_ = nullptr;
};
} // namespace lon::net
