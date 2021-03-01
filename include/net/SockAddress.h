#pragma once
#include <iosfwd>
#include <netinet/in.h>
#include <sys/un.h>

#include <sys/socket.h>



namespace lon::net {
    class Address
    {
    public:
        const sockaddr* getAddr();
        socklen_t getSockLen() const;
        virtual std::ostream& dumpTo(std::ostream& os) = 0;
        bool operator==();
        bool operator!=();

    protected:
        typedef union AddrStorage
        {
            in_addr in4_addr_;
            in6_addr in6_addr_;
            sockaddr_un un_addr_;
        } addr_;

    };
    class IPAddress : public Address
    {
        
    };
	class IPV4Address : public IPAddress {};
    class IPV6Address : public IPAddress {};
    class UnixAddress : public Address{};
}
