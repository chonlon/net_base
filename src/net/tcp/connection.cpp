#include "net/tcp/connection.h"

namespace lon::net {

ssize_t TcpConnection::send(const void* buffer, size_t length, int flags) const {
	return ::send(socket_.fd(), buffer, length, flags);
}

ssize_t TcpConnection::send(StringPiece message, int flags) const {
	return sockopt::send(socket_.fd(), message, flags);
}

ssize_t TcpConnection::send(iovec* buffers,
                            size_t length,
                            int flags) const {
	return sockopt::send(socket_.fd(), buffers, length, flags);
}

ssize_t TcpConnection::recv(void* buffer, size_t length, int flags) const {
	return ::recv(socket_.fd(), buffer, length, flags);
}

ssize_t TcpConnection::recv(iovec* buffers, size_t length, int flags) const {
	return sockopt::recv(socket_.fd(), buffers, length, flags);
}
}
