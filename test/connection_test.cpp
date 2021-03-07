#include "net/socket.h"
#include "net/socket_opt.h"
#include "net/tcp/connection.h"
#include <string>
#include <unordered_set>
#include <gtest/gtest.h>
#include "config/yaml_convert_def.h"
#include <fmt/ranges.h>
#include <netinet/tcp.h>

using namespace lon::net;

TEST(ConnectionTest, TcpInit) {
    TcpConnection connection;

    EXPECT_EQ(connection.getSocket().fd(), -1);
    EXPECT_EQ(connection.getSocket().getLocalAddress(), nullptr);
    
    EXPECT_EQ(connection.connected(), false);
    
    EXPECT_EQ(connection.getPeerAddr(), nullptr);
}

TEST(ConnectionTest, TcpMove) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    lon::String ip_string = "0.0.0.0";
    uint16_t port = 8080;
    Socket or_socket(fd);
    auto local_addr = std::make_shared<IPV4Address>(ip_string, port);
    auto peer_addr = std::make_unique<IPV4Address>("192.168.124.222", 22);
    or_socket.bind(local_addr);
    

    {
        TcpConnection or_connection(or_socket, std::make_unique<IPV4Address>(*peer_addr));
        TcpConnection m_connection(std::move(or_connection));
        EXPECT_EQ(m_connection.connected(), true);
        EXPECT_EQ(m_connection.getSocket().fd(), fd);
        EXPECT_EQ(*m_connection.getLocalAddr(), *local_addr);
        EXPECT_EQ(*m_connection.getPeerAddr(), *peer_addr);
    }

    {
        TcpConnection or_connection(or_socket, std::make_unique<IPV4Address>(*peer_addr));
        TcpConnection ma_connection = std::move(or_connection);
        EXPECT_EQ(ma_connection.connected(), true);
        EXPECT_EQ(ma_connection.getSocket().fd(), fd);
        EXPECT_EQ(*ma_connection.getLocalAddr(), *local_addr);
        EXPECT_EQ(*ma_connection.getPeerAddr(), *peer_addr);
    }
}

//connection的网络相关测试移步../runner/runner_tcp.cpp

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}