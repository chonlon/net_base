#include "net/socket.h"
#include "net/socket_opt.h"

#include <string>
#include <unordered_set>
#include <gtest/gtest.h>
#include "config/yaml_convert_def.h"
#include <fmt/ranges.h>
#include <netinet/tcp.h>

using namespace lon::net;

TEST(SocketTest, SocketOperation) {
    using namespace  lon::sockopt;
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    setTcpNoDelay(fd, true);
    int val = 0;
    auto len = static_cast<socklen_t>(sizeof(val));
    ::getsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, &len);
    EXPECT_EQ(val, 1);
    setTcpNoDelay(fd, false);
    ::getsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &val, &len);
    EXPECT_EQ(val, 0);
    
    setReusePort(fd, true);
    ::getsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &val, &len);
    EXPECT_EQ(val, 1);
    setReusePort(fd, false);
    ::getsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &val, &len);
    EXPECT_EQ(val, 0);

    setReuseAddr(fd, true);
    ::getsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, &len);
    EXPECT_EQ(val, 1);
    setReuseAddr(fd, false);
    ::getsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, &len);
    EXPECT_EQ(val, 0);

    setKeepAlive(fd, true);
    ::getsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, &len);
    EXPECT_EQ(val, 1);
    setKeepAlive(fd, false);
    ::getsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &val, &len);
    EXPECT_EQ(val, 0);
}

TEST(SocketTest, SocketInit) {
    Socket socket;
    EXPECT_EQ(socket.fd(), -1);
    EXPECT_EQ(socket.getLocalAddress(), nullptr);

    int fd = 4;
    lon::String ip_string = "0.0.0.0";
    uint16_t port = 22220;//不常用端口, 避免被占用.
    lon::String addr_string = fmt::format("{}:{}",ip_string, port);

    EXPECT_EQ(socket.bind(std::make_shared<IPV4Address>(ip_string, port)), -1);

    socket.setFd(4);
    EXPECT_EQ(socket.fd(), fd);

    EXPECT_EQ(socket.bind(std::make_shared<IPV4Address>(ip_string, port)), -1);
    EXPECT_EQ(socket.getLocalAddress()->toString(), addr_string);

    fd = ::socket(AF_INET, SOCK_STREAM, 0);
    socket.setFd(fd);
    if(int ret = socket.bind(std::make_shared<IPV4Address>(ip_string, port)); ret == -1 && errno == 98) {//可能在端口被占用的情况下失败.
    } else {
        EXPECT_EQ(ret, 0);    
    }
    
    EXPECT_EQ(socket.getLocalAddress()->toString(), addr_string);
}


TEST(SocketTest, CopyAndMove) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    lon::String ip_string = "0.0.0.0";
    uint16_t port = 80;
    Socket or_socket(fd);
    or_socket.bind(std::make_shared<IPV4Address>(ip_string, port));

#define  expectEqual(lhs, rhs) { \
        EXPECT_EQ((lhs).fd(), (rhs).fd()); \
        EXPECT_EQ(*(lhs).getLocalAddress(), *(rhs).getLocalAddress()); \
    }

    {
        Socket socket(fd);
        socket.bind(std::make_shared<IPV4Address>(ip_string, port));
        Socket c_socket(socket);
        expectEqual(socket, c_socket);
    }
    
    {
        Socket socket(fd);
        socket.bind(std::make_shared<IPV4Address>(ip_string, port));
        Socket ca_socket = socket;
        expectEqual(socket, ca_socket);
    }

    {
        Socket socket(fd);
        socket.bind(std::make_shared<IPV4Address>(ip_string, port));
        Socket m_socket(std::move(socket));
        expectEqual(or_socket, m_socket);
    }
    {
        Socket socket(fd);
        socket.bind(std::make_shared<IPV4Address>(ip_string, port));
        Socket ma_socket = std::move(socket);
        expectEqual(or_socket, ma_socket);
    }
    
}



int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}