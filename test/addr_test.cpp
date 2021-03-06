// part of test from folly/SocketAddressTest.

#include <gtest/gtest.h>
#include <string>
#include <unordered_set>

#include "net/address.h"

#include <algorithm>
#include <fmt/ranges.h>
using namespace lon::net;

TEST(SockAddrTest, exception) {
    EXPECT_THROW(IPV4Address("0.0.0.", 1), std::invalid_argument);

    EXPECT_THROW(IPV4Address("0.0.0.0:", 1), std::invalid_argument);
    EXPECT_THROW(IPV4Address("0.0.0.0:1", 1), std::invalid_argument);

    EXPECT_THROW(IPV4Address("0.256.0.0", 1), std::invalid_argument);
    EXPECT_THROW(IPV4Address("0.0.256.0", 1), std::invalid_argument);
    EXPECT_THROW(IPV4Address("0.0.0.256", 1), std::invalid_argument);

    EXPECT_THROW(IPV4Address("1.2.3.4:-1"), std::invalid_argument);
    EXPECT_THROW(IPV4Address("1.2.3.4:65536"), std::invalid_argument);
    EXPECT_THROW(IPV4Address("1.2.3.4::20000"), std::invalid_argument);
    EXPECT_THROW(IPV4Address("1.2.3.4:100000"), std::invalid_argument);

    EXPECT_THROW(IPV4Address("localhost:10"), std::invalid_argument);


    EXPECT_NO_THROW(IPV4Address("1.2.3.4:65535"));
    EXPECT_NO_THROW(IPV4Address("127.0.0.1:80"));
    EXPECT_NO_THROW(IPV4Address("127.0.0.1", 80));
    EXPECT_NO_THROW(IPV4Address("0.0.0.0", "80"));
}

TEST(SockAddrTest, init) {
    EXPECT_EQ(IPV4Address().getAddr()->sa_family, AF_UNSPEC);
    EXPECT_EQ(IPV6Address().getAddr()->sa_family, AF_UNSPEC);
}

TEST(SockAddrTest, Ipv4Addr) {
    IPAddress* addr = new IPV4Address("1.2.3.4", 4321);
    EXPECT_EQ(addr->getFamily(), AF_INET);
    EXPECT_EQ(addr->getAddressStr(), "1.2.3.4");
    EXPECT_EQ(addr->getPort(), 4321);
    const sockaddr_in* inaddr =
        reinterpret_cast<const sockaddr_in*>(addr->getAddr());
    EXPECT_EQ(inaddr->sin_addr.s_addr, htonl(0x01020304));
    EXPECT_EQ(inaddr->sin_port, htons(4321));
    delete addr;
}

TEST(SockAddrTest, Ipv4AddrToStr) {
    IPV4Address address;
    for (int pos = 0; pos < 4; ++pos) {
        for (int i = 0; i < 256; ++i) {
            std::array<int, 4> fragments{5, 5, 5, 5};
            fragments[pos]       = i;
            lon::String ipString = lon::join(".", fragments);
            address.setByHostAndPort(ipString, 1);
            EXPECT_EQ(address.getAddressStr(), ipString);
        }
    }
}

TEST(SockAddrTest, IPV6SetByHostAndPort) {
    IPV6Address addr6("2620:0:1cfe:face:b00c::3", 8888);
    EXPECT_EQ(addr6.getFamily(), AF_INET6);
    EXPECT_EQ(addr6.getAddressStr(), "2620:0:1cfe:face:b00c::3");
    EXPECT_EQ(addr6.getPort(), 8888);
}

TEST(SockAddrTest, CheckComparatorBehavior) {
    {
        IPV4Address first, second;
        first.setByHostAndPort("128.0.0.0", 0);
        second.setByHostAndPort("128.0.0.0", 0xffff);
        EXPECT_TRUE(first < second);

        first.setByHostAndPort("128.0.0.100", 0);
        second.setByHostAndPort("128.0.0.0", 0xffff);
        EXPECT_TRUE(first < second);

        first.setByHostAndPort("128.0.0.0", 10);
        second.setByHostAndPort("128.0.0.100", 10);
        EXPECT_TRUE(first < second);

        first.setByHostAndPort("128.0.0.0", 10);
        second.setByHostAndPort("128.0.0.0", 10);
        EXPECT_TRUE(first == second);
    }


    {
        SockAddress *first, *second;
        first  = new IPV4Address("128.0.0.0", 0);
        second = new IPV6Address("::ffff:127.0.0.1", 0);
        EXPECT_TRUE(*first < *second);
    }

    {
        SockAddress *first, *second;
        first  = new IPV4Address("128.0.0.0", 100);
        second = new IPV6Address("::ffff:127.0.0.1", 0);
        EXPECT_TRUE(*first < *second);
    }
    {
        SockAddress *first, *second;
        first  = new IPV6Address("::0", 0);
        second  = new IPV6Address("::0", 0xFFFF);
        EXPECT_TRUE(*first < *second);
    }
    {
        SockAddress *first, *second;
        first  = new IPV6Address("::0", 0);
        second  = new IPV6Address("::1", 0xFFFF);
        EXPECT_TRUE(*first < *second);
    }
    {
        SockAddress *first, *second;
        first  = new IPV6Address("::0", 10);
        second  = new IPV6Address("::1", 10);
        EXPECT_TRUE(*first < *second);
    }
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
