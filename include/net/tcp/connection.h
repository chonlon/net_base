#pragma once

#include "../socket.h"

namespace lon::net {
	class TcpConnection
	{
	public:
		using SockAddressPtr = SockAddress::UniquePtr;
        TcpConnection() = default;


        TcpConnection(TcpConnection&& _other) noexcept = default;
        auto operator=(TcpConnection&& _other) noexcept -> TcpConnection& = default;

        explicit TcpConnection(Socket _sock,
                               SockAddressPtr _peer_addr)
            : connected_{true},
              socket_{_sock},
              peer_addr_{std::move(_peer_addr)} {
        }

        ~TcpConnection() = default;

		LON_NODISCARD
	    bool connected() const { return connected_; }

		LON_NODISCARD
		const SockAddress* getLocalAddr() const {return socket_.getLocalAddress().get();}

		LON_NODISCARD
		const SockAddress* getPeerAddr() const {return  peer_addr_.get();}

		LON_NODISCARD
		Socket& getSocket() {return socket_;}

        ssize_t send(const void* buffer, size_t length, int flags = 0) const;
        ssize_t send(StringPiece message, int flags = 0) const;
        ssize_t send(iovec* buffers, size_t length, int flags = 0) const;
        ssize_t recv(void* buffer, size_t length, int flags = 0) const;
        ssize_t recv(iovec* buffers, size_t length, int flags = 0) const;

	private:
		bool connected_ = false;
		Socket socket_{};
		SockAddressPtr peer_addr_ = nullptr;
	};

}
