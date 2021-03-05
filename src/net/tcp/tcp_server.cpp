#include "net/tcp/tcp_server.h"

namespace lon::net {
static auto G_logger = LogManager::getInstance()->getLogger("system");
TcpServer::TcpServer(OnConnectionCallbackType _on_connection,
                     std::unique_ptr<io::IOWorkBalancer> _balancer)
    : on_connection_{std::move(_on_connection)},
      balancer_{std::move(_balancer)} {
    if(balancer_ == nullptr) 
        balancer_ = std::make_unique<io::SimpleIOBalancer>();
}

TcpServer::~TcpServer() {
    if(serving_) { LON_LOG_ERROR(G_logger) << "tcp server is being destroy while serving";}
    else { LON_LOG_INFO(G_logger) << "notion: tcp server destroying"; }
}

auto TcpServer::setSocketIniter(SocketInitCallbackType _socket_initer) -> void {
    socket_initer_ = std::move(_socket_initer);
}

bool TcpServer::bind(SockAddress::SharedPtr local_address) {
    if (listen_sockets_.empty()) {
        return bindOne(local_address);
    } else {
        LON_LOG_ERROR(G_logger) << fmt::format("server has bind socket");
        return false;
    }
}

std::optional<std::vector<SockAddress::SharedPtr>> TcpServer::bind(
    const std::vector<SockAddress::SharedPtr>& local_addresses) {
    if (listen_sockets_.empty()) {
        std::vector<SockAddress::SharedPtr> bind_failed_addresses;
        for (auto& local_address : local_addresses) {
            if (bindOne(local_address)) {
                bind_failed_addresses.push_back(local_address);
            } else {
                break;
            }
        }
        if (bind_failed_addresses.empty()) {
            return std::nullopt;
        } else {
            return bind_failed_addresses;
        }
    } else {
        LON_LOG_ERROR(G_logger) << fmt::format("server has bind socket");
        return local_addresses;
    }
    return std::nullopt;
}

bool TcpServer::addBindAddr(SockAddress::SharedPtr local_address) {
    return bindOne(local_address);
}

bool TcpServer::startServe() {
    if (serving_)
        return true;
    // while accepting, hold this.
    auto hold_this = this->shared_from_this();
    for (auto socket : listen_sockets_) {
        io::IOManager::getThreadLocal()->addExecutor(
            std::make_shared<coroutine::Executor>([this, hold_this, socket]() {
                while (serving_) {
                    std::shared_ptr<TcpConnection> connection = socket.accept();
                    if (connection) {
                       
                        balancer_->schedule(std::make_shared<coroutine::Executor>(
                            [on_connection = this->on_connection_, connection]() {
                                on_connection(connection);
                            }), 0);
                    } else {
                        LON_LOG_WARN(
                            LogManager::getInstance()->getLogger("system"))
                            << fmt::format(
                                   "accept failed; wrong fd or addr; fd:{}, "
                                   "addr:{}",
                                   socket.fd(),
                                   socket.getLocalAddress()->toString());
                        continue;
                    }
                }
            }));
    }
    serving_ = true;

    return true;
}

bool TcpServer::stopServe() {
    if (!serving_)
        return true;
    serving_  = false;
    auto self = shared_from_this();
    io::IOManager::getThreadLocal()->addExecutor(
        std::make_shared<coroutine::Executor>([this, self]() {
            for (auto& socket : listen_sockets_) {
                socket.shutdownWrite();
                socket.stopRead();
                socket.close();
            }
        }));
    return true;
}


bool TcpServer::bindOne(SockAddress::SharedPtr local_address) {
    int fd = ::socket(local_address->getFamily(), SOCK_STREAM, 0);
    if (fd == -1) {
        LON_LOG_ERROR(G_logger) << fmt::format("create socket failed");
        return false;
    }
    Socket socket(fd);
    if (socket_initer_) {
        socket_initer_(socket);
    }

    if (int bind_ret = socket.bind(local_address); bind_ret == -1) {
        LON_LOG_ERROR(G_logger)
            << fmt::format("bind socket failed, addr:{}, errno:{}({})",
                           local_address->toString(),
                           std::strerror(errno),
                           errno);
        return false;
    }
    if (int listen_ret = socket.listen(); listen_ret == -1) {
        LON_LOG_ERROR(G_logger)
            << fmt::format("listen failed, addr:{}, errno:{}({})",
                           local_address->toString(),
                           std::strerror(errno),
                           errno);
        return false;
    }


    listen_sockets_.push_back(socket);
    LON_LOG_INFO(G_logger) << fmt::format("server bind addr succeed, addr:{}",
                                          local_address->toString());
    return true;
}
}  // namespace lon::net
