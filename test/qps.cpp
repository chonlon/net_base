#include "base/chrono_helper.h"
#include "io/hook.h"
#include "io/io_manager.h"
#include "net/tcp/connection.h"
#include "net/tcp/tcp_server.h"

#include <string>
#include <fmt/os.h>

static bool async = false;
constexpr uint16_t port = 22223;
constexpr int query_time = 100000;
std::array<const char*, 10> query_result = {
    "111",
    "222",
    "3333",
    "44",
    "55555",
    "66666666",
    "7777777",
    "8",
    "999999999",
    "00000"
};

void server() {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    lon::net::Socket socket(fd);
    socket.bind(std::make_shared<lon::net::IPV4Address>("127.0.0.1", port));
    socket.setReuseAddr(true);
    socket.setReusePort(true);
    socket.setTcpNoDelay(true);
    socket.listen();
    
    auto connection = socket.accept();
    fmt::print("local:{}, peer:{}\n",
        connection->getLocalAddr()->toString(),
        connection->getPeerAddr()->toString());
    for(int i =0; i < query_time; ++i) {
        int query;
        [[maybe_unused]] auto n_r = connection->recv(&query, sizeof(query), 0);
        assert(n_r == sizeof(query));
        query %= static_cast<int>(query_result.size());
        [[maybe_unused]] auto n_w = connection->send(query_result[query], sizeof(query_result[query] -1), 0);
        assert(n_w ==sizeof(query_result[query] - 1));
    }
    if (async)
        lon::io::IOManager::getThreadLocal()->stop();
}

void client() {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    lon::net::Socket socket(fd);
    socket.setTcpNoDelay(true);


    auto connection = socket.connect(std::make_unique<lon::net::IPV4Address>("127.0.0.1", port));
    size_t time_span;
    {
        lon::measure::GetTimeSpan<> span(&time_span);
        for (int i = 0; i < query_time; ++i) {
            [[maybe_unused]] auto n_w = connection->send(&i, sizeof(i), 0);
            assert(n_w ==sizeof(i));
            char buf[1024];
            connection->recv(buf, sizeof(buf), 0);
        }
    }
    fmt::print("{:.3f} query per second", static_cast<double>(query_time) / static_cast<double>(time_span) * 1000.0);
}

int main(int argc, char** argv) {
    auto printUsage = []()
    {
        fmt::print("usage: qps [-a/-s](a for async, s for sync) [-s/-c](s for server, c for client");
    };
    if (argc != 3) {
        printUsage();
        return -1;
    }

    if (strcmp(argv[1], "-a") == 0) {
        async = true;
        
    }
    else if (strcmp(argv[1], "-s") == 0) {
        async = false;
        lon::io::setHookEnabled(false);
    }
    else {
        printUsage();
        return -1;
    }

    if (strcmp(argv[2], "-c") == 0) {
        client();
    }
    else if (strcmp(argv[2], "-s") == 0) {
        if (async) {
            lon::io::setHookEnabled(true);
            lon::io::IOManager::getThreadLocal()->addExecutor(std::make_shared<lon::coroutine::Executor>(server));
            lon::io::IOManager::getThreadLocal()->run();
        }
        else {
            server();
        }
    }

    return 0;
}