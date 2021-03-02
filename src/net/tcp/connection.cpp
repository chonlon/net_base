#include "net/tcp/connection.h"

namespace lon::net {

int TcpConnection::send(const void* buffer, size_t length, int flags) const {
	return ::send(socket_.fd(), buffer, length, flags);
}

int TcpConnection::send(StringPiece message, int flags) const {
	return sockopt::send(socket_.fd(), message, flags);
}

int TcpConnection::send(const iovec* buffers, size_t length, int flags) const {
	return sockopt::send(socket_.fd(), buffers, length, flags);
}

int TcpConnection::recv(void* buffer, size_t length, int flags) const {
	return ::recv(socket_.fd(), buffer, length, flags);
}

int TcpConnection::recv(iovec* buffers, size_t length, int flags) const {
	return sockopt::recv(socket_.fd(), buffers, length, flags);
}
}
