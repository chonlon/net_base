#include "base/chrono_helper.h"
#include "io/hook.h"
#include "io/io_manager.h"
#include "net/tcp/connection.h"
#include "net/tcp/tcp_server.h"

#include <string>
#include <fmt/os.h>

static bool async = true;

struct SessionMessage
{
    int32_t number;
    int32_t length;
} __attribute__((__packed__));

struct PayloadMessage
{
    int32_t length;
    char data[0];
};

constexpr uint16_t port = 22222;
constexpr int number = 81920;
constexpr int message_length = 1000;
constexpr double total_mb = 1.0 * (message_length + sizeof(int32_t)) * number / lon::data::M;

void client() {
    int sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    lon::net::Socket socket(sockfd);
    auto connection = socket.connect(std::make_unique<lon::net::IPV4Address>("127.0.0.1", port));
    if(!connection) fmt::print("connection failed\n");
    fmt::print("connected\n");
    size_t time_span;
    SessionMessage session_message{0,0};
    session_message.number = ::htonl(number);
    session_message.length = ::htonl(message_length);
    if(connection->send(&session_message, sizeof(SessionMessage), 0) != sizeof(SessionMessage)) {
        fmt::print("write session message failed\n");
    }

    const int total_len = static_cast<int>(sizeof(int32_t) + message_length);
    PayloadMessage* payload = static_cast<PayloadMessage*>(::malloc(total_len));
    assert(payload);
    payload->length = ::htonl(message_length);
    for (int i = 0; i < message_length; ++i) {
        payload->data[i] = "0123456789ABCDEF"[i % 16];
    }
    {
        lon::measure::GetTimeSpan<> SpanMeasure(&time_span);
        for(int i = 0; i < number; ++i) {
            [[maybe_unused]] ssize_t n_w = connection->send(payload, total_len, 0);
            assert(n_w == total_len);

            int ack = 0;
            [[maybe_unused]] ssize_t n_r = connection->recv(&ack, sizeof(ack), 0);
            ack = ::ntohl(ack);
            assert(n_r == sizeof(ack));
            assert(ack == message_length);
        }
    }
    ::free(payload);
    ::close(sockfd);
    double seconds = static_cast<double>(time_span) / 1000.0;
    fmt::print("{:.3f} seconds, {:.3f} Mib/s", seconds, total_mb / seconds);
    // connection->send();
    if (async)
        lon::io::IOManager::getThreadLocal()->stop();
}

void server() {
    int sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    lon::net::Socket socket(sockfd);
    socket.bind(std::make_shared<lon::net::IPV4Address>("127.0.0.1", port));
    socket.setReuseAddr(true);
    socket.listen();
    auto connection = socket.accept();

    struct SessionMessage sessionMessage = { 0, 0 };
    if (connection->recv(&sessionMessage, sizeof(sessionMessage), 0) != sizeof(sessionMessage))
    {
        fmt::print("read SessionMessage\n");
        exit(1);
    }

    sessionMessage.number = ntohl(sessionMessage.number);
    sessionMessage.length = ntohl(sessionMessage.length);
    printf("receive number = %d\nreceive length = %d\n",
        sessionMessage.number, sessionMessage.length);
    const int total_len = static_cast<int>(sizeof(int32_t) + sessionMessage.length);
    PayloadMessage* payload = static_cast<PayloadMessage*>(::malloc(total_len));
    assert(payload);

    for (int i = 0; i < sessionMessage.number; ++i)
    {
        payload->length = 0;
        if (connection->recv(&payload->length, sizeof(payload->length)) != sizeof(payload->length))
        {
            fmt::print("read length\n");
            exit(1);
        }
        payload->length = ntohl(payload->length);
        assert(payload->length == sessionMessage.length);
        if (connection->recv(payload->data, payload->length) != payload->length) //这里直接用的recv, 所以如果超过mtu, tcp将数据包拆开会出错(client会回到rst).
        {
            fmt::print("read payload data\n");
            exit(1);
        }
        int32_t ack = htonl(payload->length);
        if (connection->send(&ack, sizeof(ack)) != sizeof(ack))
        {
            fmt::print("write ack\n");
            exit(1);
        }
    }
    ::free(payload);
    ::close(sockfd);
    if(async)
        lon::io::IOManager::getThreadLocal()->stop();
}

int main(int argc, char** argv) {
    auto printUsage = []()
    {
        fmt::print("usage: ttcp [-a/-s](a for async, s for sync) [-s/-c](s for server, c for client");
    };
    if (argc != 3) {
        printUsage();
        return -1;
    }

    if (strcmp(argv[1], "-a") == 0) {
        async = true;
        lon::io::setHookEnabled(true);
    } else if (strcmp(argv[1], "-s") == 0) {
        async = false;
        lon::io::setHookEnabled(false);
    } else {
        printUsage();
        return -1;
    }

    if (strcmp(argv[2], "-c") == 0) {
        if(async) {
            lon::io::IOManager::getThreadLocal()->addExecutor(std::make_shared<lon::coroutine::Executor>(client));
            lon::io::IOManager::getThreadLocal()->run();
        } else {
            client();
        }
    }
    else if (strcmp(argv[2], "-s") == 0) {
        if (async) {
            lon::io::IOManager::getThreadLocal()->addExecutor(std::make_shared<lon::coroutine::Executor>(server));
            lon::io::IOManager::getThreadLocal()->run();
        }
        else {
            server();
        }
    }
    else {
        printUsage();
        return -1;
    }
}