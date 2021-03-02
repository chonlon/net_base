#pragma once
#include "net/socket.h"
#include "base/nocopyable.h"

namespace lon::net {
    class IOWorkBalancer;

	class TcpServer : public std::enable_shared_from_this<TcpServer>, Noncopyable
	{
	public:
		bool bind(SockAddress::SharedPtr local_address);

		std::optional<std::vector<SockAddress::SharedPtr>> bind(const std::vector<SockAddress::SharedPtr>& local_addresses);

		bool startServe();
		bool stopServe();

		LON_NODISCARD LON_ALWAYS_INLINE
		bool serving() const { return serving_;}
	private:

		bool serving_ = false;
		Socket listen_socket_{};
		std::shared_ptr<IOWorkBalancer> balancer_;
	};
}
