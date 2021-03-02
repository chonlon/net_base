#pragma once

#include "../socket.h"

namespace lon::net {
	class TcpConnection
	{
	public:
		using SockAddressPtr = SockAddress::UniquePtr;
        TcpConnection() = default;

        explicit TcpConnection(Socket _sock,
			SockAddressPtr _peer_addr)
            : connected_{true},
              socket_{_sock},
              peer_addr_{std::move(_peer_addr)} {
        }

		LON_NODISCARD
	    bool connected() const { return connected_; }

		LON_NODISCARD
		const SockAddress* getLocalAddr() const {return socket_.getLocalAddress().get();}

		LON_NODISCARD
		const SockAddress* getPeerAddr() const {return  peer_addr_.get();}

		LON_NODISCARD
		Socket& getSocket() {return socket_;}

		int send(const void* buffer, size_t length, int flags = 0) const;
        int send(StringPiece message, int flags = 0) const;
		int send(const iovec* buffers, size_t length, int flags = 0) const;
		int recv(void* buffer, size_t length, int flags) const;
		int recv(iovec* buffers, size_t length, int flags) const;

	private:
		bool connected_ = false;
		Socket socket_{};
		SockAddressPtr peer_addr_ = nullptr;
	};

}
